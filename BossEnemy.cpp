#include "BossEnemy.h"
#include "BossEnemyAnimInstance.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "BossEnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "MainGameModeBase.h"

ABossEnemy::ABossEnemy()
{
	PrimaryActorTick.bCanEverTick = true; // tick Ȱ��ȭ
	bCanBossAttack = true; // ���� ���� ���� true
	AIControllerClass = ABossEnemyAIController::StaticClass(); // AI ��Ʈ�ѷ� ����

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; //���� �ÿ��� AI ��Ʈ�ѷ� �ڵ� �Ҵ�

	GetMesh()->SetAnimInstanceClass(UBossEnemyAnimInstance::StaticClass()); // �ִ� �ν��Ͻ� ����
	if (!AIControllerClass) // ��Ʈ�ѷ��� ���ٸ�
	{
		UE_LOG(LogTemp, Error, TEXT("Boss AI Controller NULL"));
	}
}

void ABossEnemy::BeginPlay()
{
	Super::BeginPlay();
	SetCanBeDamaged(true); // ���ظ� ���� �� �ִ��� ���� true

	// AI ��Ʈ�ѷ� ���� �Ҵ�
	if (!GetController())
	{
		SpawnDefaultController();
		UE_LOG(LogTemp, Warning, TEXT("Boss AI Controller manually spawned"));
	}

	AAIController* AICon = Cast<AAIController>(GetController()); // AI ��Ʈ�ѷ��� �ҷ��� ĳ��Ʈ
	if (AICon) // ��Ʈ�ѷ��� �����Ѵٸ�
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss AI Controller Assigend")); // ��Ʈ�ѷ� �Ҵ��
	}
	else // �ƴѰ��
	{
		UE_LOG(LogTemp, Error, TEXT("Boss AI Controller NULL")); // ��Ʈ�ѷ� NULL
	}
	SetUpBossAI(); // AI �����Լ� ȣ��
}

void ABossEnemy::SetUpBossAI()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking); // ĳ���� �����Ʈ�� ������ �����忡 �ִ� �׺� ��ŷ���� ����
}

void ABossEnemy::PostInitializeComponents()
{
	Super::PostInitializeComponents(); // ������Ʈ �ʱ�ȭ ���� �߰� �۾��� ���� �θ��Լ� ȣ��

	GetWorld()->GetTimerManager().SetTimerForNextTick([this]() // ���� Tick���� AI ��Ʈ�ѷ� �Ҵ� ���θ� Ȯ���ϴ� ���� ���
		{
			AAIController* AICon = Cast<AAIController>(GetController()); // �ѹ��� AI��Ʈ�ѷ��� ĳ�����ؼ� �޾ƿ�
			if (AICon) // ��Ʈ�ѷ��� �����Ѵٸ�
			{
				UE_LOG(LogTemp, Warning, TEXT("Boss AICon Assigned Later")); // ��Ʈ�ѷ� �Ҵ��
			}
			else // �ƴѰ��
			{
				UE_LOG(LogTemp, Error, TEXT("Boss AICon Still NULL")); // ��Ʈ�ѷ� NULL
			}
		});
}

void ABossEnemy::PlayBossNormalAttackAnimation()
{
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance()); // �ֽ� �ִ��ν��Ͻ��� ĳ�����ؼ� �޾ƿ�
	if (!AnimInstance || BossNormalAttackMontages.Num() == 0) return; // �ִ��ν��Ͻ��� ���ų� �Ϲݰ��� ��Ÿ�ְ� ���ٸ� ���� 
	AnimInstance->bUseUpperBodyBlend = false; // ����ü �и� ���� false
	AnimInstance->Montage_Stop(0.1f, nullptr); // �ٸ����� ��Ÿ�� ����
	if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return; // �ִ��ν��Ͻ��� �ְ� �ִ��ν��Ͻ��� ��Ÿ�ְ� �������̶�� ����

	int32 RandomIndex = FMath::RandRange(0, BossNormalAttackMontages.Num() - 1); // �Ϲݰ��� ��Ÿ�ֹ迭�߿� �������� �����ϴ� �����ε��� ����
	UAnimMontage* SelectedMontage = BossNormalAttackMontages[RandomIndex]; // �������ؽ����� ������ ��Ÿ�ָ� ������
	if (SelectedMontage) // ��Ÿ�ְ� ���� �Ǿ��ٸ�
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectedMontage: %s is playing"), *SelectedMontage->GetName());
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // �÷��̰���� �ִ��ν����� ��Ÿ�� �÷��̸� 1.0������� ���
		if (PlayResult == 0.0f) // �÷��� ����� 0�̶��
		{
			UE_LOG(LogTemp, Warning, TEXT("Montage Play Failed")); // ��Ÿ�� ��� ����
			// ��Ÿ�� ��� ���� �� ��� ���� ���� ���·� ����
			bCanBossAttack = true;
			bIsFullBodyAttacking = false;
		}
		else
		{
			// ��Ÿ�� ���� ��������Ʈ ���ε�
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ABossEnemy::OnNormalAttackMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
			UE_LOG(LogTemp, Warning, TEXT("Montage Succesfully Playing")); // ��Ÿ�� �����
		}

		ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController()); // ��Ʈ�ѷ��� ĳ��Ʈ�ؼ� ������
		if (AICon) // ��Ʈ�ѷ��� ������ ���
		{
			AICon->StopMovement(); // ��Ʈ�ѷ��� �̵����� �Լ��� ȣ��
			UE_LOG(LogTemp, Warning, TEXT("Enemy Stopped Moving to Attack"));
		}

		bCanBossAttack = false; // ���� ���̹Ƿ� ���� �Ұ� false
		bIsFullBodyAttacking = true; 
		DisableBossMovement();
	}
	if (BossNormalAttackSound) // ���尡 �ִٸ�
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossNormalAttackSound, GetActorLocation()); // �ش� ������ ��ġ���� �Ҹ� ���
	}
}

void ABossEnemy::OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Normal Attack Montage Ended"));

	// ���� �����
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // �⺻������ ����
	}
	// ���Ű��� ���� �� �̵� ���
	bIsFullBodyAttacking = false;
	EnableBossMovement(); // �̵� ���

	// AI ��Ʈ�ѷ��� ���� ���� �˸�
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true; // ���� ���� ���·� ����
}

// �̵� ���� �Լ�
void ABossEnemy::DisableBossMovement()
{
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->DisableMovement(); // �̵� ���� ����
		MovementComp->StopMovementImmediately(); // ���� �̵� ��� ����
	}

	UE_LOG(LogTemp, Warning, TEXT("Boss Movement Disabled"));
}

// �̵� ��� �Լ�
void ABossEnemy::EnableBossMovement()
{
	if (bIsBossDead) return; // ����� ��� �̵� ������� ����

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->SetMovementMode(EMovementMode::MOVE_NavWalking); // �׺���̼� ��ŷ ���� ����
	}

	UE_LOG(LogTemp, Warning, TEXT("Boss Movement Enabled"));
}

void ABossEnemy::PlayBossUpperBodyAttack()
{
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance || BossUpperBodyMontages.Num() == 0) return;

	AnimInstance->bUseUpperBodyBlend = true; // ����ü�и� ���� true

	int32 RandomIndex = FMath::RandRange(0, BossUpperBodyMontages.Num() - 1);
	UAnimMontage* SelectedMontage = BossUpperBodyMontages[RandomIndex];

	if (SelectedMontage)
	{
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

		if (PlayResult > 0.0f)
		{
			// ��ü ���� ���� ��������Ʈ ���ε�
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ABossEnemy::OnUpperBodyAttackMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);

			UE_LOG(LogTemp, Warning, TEXT("Upper Body Attack Playing"));
		}
		else
		{
			bCanBossAttack = true; // ��� ���� �� ��� ����
		}

		bCanBossAttack = false; // ���� ���̹Ƿ� ���� �Ұ�
	}
}

void ABossEnemy::OnUpperBodyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Upper Body Attack Montage Ended"));

	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // �⺻������ ����
	}

	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true; // ���� ���� ���·� ����
}

float ABossEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsBossDead) return 0.0f; // �̹� ����ߴٸ� ������ ����
	float DamageApplied = FMath::Min(BossHealth, DamageAmount); // ������ ü�°� �������� �ҷ���
	BossHealth -= DamageApplied; // ü�¿��� ��������ŭ ����
	UE_LOG(LogTemp, Warning, TEXT("Boss took %f damage, Health remaining: %f"), DamageAmount, BossHealth);

	if (BossHitSound) // ���尡 �ִٸ�
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossHitSound, GetActorLocation()); // �ش� ������ ��ġ���� �Ҹ����
	}

	if (BossHitReactionMontages.Num() > 0)
	{
		bIsBossHit = true; // ��Ʈ�� ���� true
		bCanBossAttack = false; // ���ݰ��� ���� false

		// AI �̵� ����
		ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
		if (AICon)
		{
			AICon->StopMovement();
		}

		UBossEnemyAnimInstance* BossAnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
		if (BossAnimInstance)
		{
			BossAnimInstance->bUseUpperBodyBlend = false;
			BossAnimInstance->Montage_Stop(0.5f); // ���� ��Ÿ�� ����
		}

		int32 RandomIndex = FMath::RandRange(0, BossHitReactionMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossHitReactionMontages[RandomIndex];
		if (SelectedMontage)
		{
			UAnimInstance* HitAnimInstance = GetMesh()->GetAnimInstance(); // �ٸ� �̸� ���
			if (HitAnimInstance)
			{
				UE_LOG(LogTemp, Warning, TEXT("AnimInstance is valid before playing hit montage"));
				float PlayResult = HitAnimInstance->Montage_Play(SelectedMontage, 1.0f);

				// �ǰ� ��Ÿ�� ���� ��������Ʈ �߰�
				if (PlayResult > 0.0f)
				{
					FOnMontageEnded HitEndDelegate;
					HitEndDelegate.BindUObject(this, &ABossEnemy::OnHitReactionMontageEnded);
					HitAnimInstance->Montage_SetEndDelegate(HitEndDelegate, SelectedMontage);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("AnimInstance is NULL before playing hit montage"));
				bIsBossHit = false; // ��Ʈ�� ���� �ʱ�ȭ
				bCanBossAttack = true; // ���ݰ��� ���� �ʱ�ȭ
			}
		}
	}

	if (BossHealth <= 0.0f) // ü���� 0������ ���
	{
		BossDie(); // ����Լ� ȣ��
	}
	
	return DamageApplied; // ���� ���� �ʱ�ȭ
}

void ABossEnemy::OnHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Hit Reaction Montage Ended"));

	bIsBossHit = false; // ��Ʈ�� ���� false

	// �ǰ� �ִϸ��̼� ���� �� ���� ����
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // �⺻������ ����
	}

	// AI ��Ʈ�ѷ��� ��Ʈ ���� �˸�
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded(); // ���� ���� ���� ����
	}

	bCanBossAttack = true; // ���ݰ��� ���� �ʱ�ȭ
}

void ABossEnemy::BossDie()
{
	if (bIsBossDead) return; // �̹� ����� ��� ����
	bIsBossDead = true; // ������� Ʈ��

	// GameMode�� ���� ��� �˸�
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->OnBossDead();
	}

	// ��� ���� ���
	if (BossDieSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossDieSound, GetActorLocation());
	}

	StopBossActions();

	UBossEnemyAnimInstance* BossAnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (BossAnimInstance)
	{
		BossAnimInstance->bUseUpperBodyBlend = false;
		BossAnimInstance->Montage_Stop(0.0f); // ��Ÿ�� ��� ����
	}

	if (BossDeadMontages.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, BossDeadMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossDeadMontages[RandomIndex];
		if (SelectedMontage)
		{
			UAnimInstance* DeathAnimInstance = GetMesh()->GetAnimInstance();
			if (DeathAnimInstance)
			{
				float PlayResult = DeathAnimInstance->Montage_Play(SelectedMontage, 1.0f);

				if (PlayResult > 0.0f)
				{
					// ��Ÿ�� ������ ������ ������ ���� ����
					float MontageLength = SelectedMontage->GetPlayLength();
					float HideTime = MontageLength * 0.9f; // 90% ����

					GetWorld()->GetTimerManager().SetTimer(
						BossDeathHideTimerHandle,
						this,
						&ABossEnemy::HideBossEnemy,
						HideTime, // ������ ������ ����
						false
					);

					UE_LOG(LogTemp, Warning, TEXT("Death montage playing - will hide at 99%% (%.2f seconds)"), HideTime);
					return; // Ÿ�̸Ӱ� ����� ������ ���
				}
			}
		}
	}
	// ��Ÿ�ְ� ���ų� ��� ���н�
	HideBossEnemy(); // ������ ����� �Լ� ȣ��
}

void ABossEnemy::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Death Montage Ended"));
	HideBossEnemy(); // ��� ��Ÿ�� ���� �� ���� ����
}

void ABossEnemy::StopBossActions()
{
	GetCharacterMovement()->DisableMovement(); // ��� �̵� ����
	GetCharacterMovement()->StopMovementImmediately(); // ��� �̵� ��� ����
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None); // �̵� ��� ��Ȱ��ȭ

	// AI ��Ʈ�ѷ� ��� ����
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->StopBossAI(); // AI ����
	}

	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance()); // �ֽ� �ִ��ν��Ͻ��� ĳ�����ؼ� �޾ƿ�
	if (AnimInstance) // �ִ� �ν��Ͻ���
	{
		AnimInstance->Montage_Stop(0.0f); // ��� ��Ÿ�� ����
	}

	// �����
	if (bIsBossDead)
	{
		SetActorTickEnabled(false); // ƽ ��Ȱ��ȭ

		// ��������Ʈ ����
		UAnimInstance* BaseAnimInstance = GetMesh()->GetAnimInstance();
		if (BaseAnimInstance)
		{
			BaseAnimInstance->OnMontageEnded.RemoveAll(this);
		}
	}
}

void ABossEnemy::HideBossEnemy()
{
	if (!bIsBossDead) return; // �̹� ����� ��� ����

	UE_LOG(LogTemp, Warning, TEXT("Hiding Boss Enemy - Memory Cleanup"));

	// 1. �̺�Ʈ �� ��������Ʈ ���� (�ֿ켱)
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // ��� Ÿ�̸� ����

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // �ִ� �ν��Ͻ� ���� �޾ƿ�
	if (AnimInstance && IsValid(AnimInstance)) // �ִ� �ν��Ͻ��� �ְ� ��ȿ�ϴٸ�
	{
		// �ִϸ��̼� �̺�Ʈ ���ε� ���� ����
		AnimInstance->OnMontageEnded.RemoveAll(this); // ��Ÿ�� ���� �̺�Ʈ ���ε� ����
		AnimInstance->OnMontageBlendingOut.RemoveAll(this); // ��Ÿ�� ���� �ƿ� ���ε� ����
		AnimInstance->OnMontageStarted.RemoveAll(this); // ��Ÿ�� ���� �̺�Ʈ ���ε� ��ü
	}

	// 2. AI �ý��� ���� ���� 
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController()); // AI ��Ʈ�ѷ� ���� �޾ƿ�
	if (AICon && IsValid(AICon)) // AI ��Ʈ�ѷ��� �ְ� ��ȿ�ϴٸ�
	{
		AICon->StopBossAI(); // AI ���� �ߴ�
		AICon->UnPossess(); // ��Ʈ�ѷ�-�� ���� ����
		AICon->Destroy(); // AI ��Ʈ�ѷ� ���� ����
	}

	// 3. �����Ʈ �ý��� ����
	UCharacterMovementComponent* MovementComp = GetCharacterMovement(); // ĳ���� �����Ʈ ������Ʈ ���� �޾ƿ�
	if (MovementComp && IsValid(MovementComp)) // �����Ʈ ������Ʈ�� �ְ� ��ȿ�ϴٸ�
	{
		MovementComp->DisableMovement(); // �̵� ��Ȱ��ȭ
		MovementComp->StopMovementImmediately(); // ���� �̵� ��� �ߴ�
		MovementComp->SetMovementMode(EMovementMode::MOVE_None); // Move��� None �������� �׺���̼ǿ��� ����
		MovementComp->SetComponentTickEnabled(false); // �����Ʈ ������Ʈ Tick ��Ȱ��ȭ
	}

	// 4. �޽� ������Ʈ ����
	USkeletalMeshComponent* MeshComp = GetMesh(); // ���̷�Ż �޽� ������Ʈ ���� �޾ƿ�
	if (MeshComp && IsValid(MeshComp)) // �޽� ������Ʈ�� �ְ� ��ȿ�ϴٸ�
	{
		// ������ �ý��� ��Ȱ��ȭ
		MeshComp->SetVisibility(false); // �޽� ���ü� ��Ȱ��ȭ
		MeshComp->SetHiddenInGame(true); // ���� �� ���� ó��
 
		// ���� �ý��� ��Ȱ��ȭ
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // NoCollision �������� �浹�˻� ��Ȱ��ȭ
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // ECR_Ignore �������� �浹���� ����

		// ������Ʈ �ý��� ��Ȱ��ȭ
		MeshComp->SetComponentTickEnabled(false); // �޽� ������Ʈ Tick ��Ȱ��ȭ

		// �ִϸ��̼� ���� ����
		MeshComp->SetAnimInstanceClass(nullptr); // ABP ���� ����
		MeshComp->SetSkeletalMesh(nullptr); // ���̷�Ż �޽� ���� ����

	}

	// 5. ���� ���� �ý��� ����
	SetActorHiddenInGame(true); // ���� ������ ��Ȱ��ȭ
	SetActorEnableCollision(false); // ���� �浹 ��Ȱ��ȭ
	SetActorTickEnabled(false); // ���� Tick ��Ȱ��ȭ
	SetCanBeDamaged(false); // ������ ó�� ��Ȱ��ȭ

	// 6. ���� ������ ó�� �Ϸ� �� ���� �����ӿ� �����ϰ� ���� ���� (ũ���� ����)
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<ABossEnemy>(this)]() // ����Ʈ ������ WeakObjectPtr�� ���� ������ ����Ͽ� �����ϰ� ���� ����
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // ���� ������ ���Ͱ� ��ȿ�ϰ� �ı����� �ʾҴٸ�
			{
				WeakThis->Destroy(); // ���� ���� ����
				UE_LOG(LogTemp, Warning, TEXT("EnemyBoss Successfully Destroyed."));
			}
		});
}