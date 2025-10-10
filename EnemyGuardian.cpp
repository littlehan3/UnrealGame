#include "EnemyGuardian.h"
#include "EnemyGuardianAnimInstance.h" // �ִ� �ν��Ͻ� Ŭ����
#include "GameFramework/Actor.h" // AActor Ŭ����
#include "Kismet/GameplayStatics.h" // �����÷��� ��ƿ��Ƽ �Լ�
#include "EnemyGuardianShield.h" // ���� Ŭ����
#include "EnemyGuardianAIController.h" // AI ��Ʈ�ѷ� Ŭ����
#include "EnemyGuardianBaton.h" // ���к� Ŭ����
#include "GameFramework/CharacterMovementComponent.h" // ĳ���� �����Ʈ ������Ʈ
#include "MainGameModeBase.h" // ���Ӹ�� ���� (�� ��� �˸���)

AEnemyGuardian::AEnemyGuardian()
{
	PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ� ȣ�� Ȱ��ȭ
	GetCharacterMovement()->MaxWalkSpeed = 300.0f; // �⺻ �̵� �ӵ� ����

	// ȸ�� ����: AI ��Ʈ�ѷ��� ������ ���� ĳ���Ͱ� ȸ���ϵ��� ����
	GetCharacterMovement()->bOrientRotationToMovement = false; // �̵� �������� �ڵ� ȸ�� ��Ȱ��ȭ
	GetCharacterMovement()->bUseControllerDesiredRotation = false; // ��Ʈ�ѷ��� ��� ȸ���� ��� ��Ȱ��ȭ
	bUseControllerRotationYaw = true; // ��Ʈ�ѷ��� Yaw ȸ������ ĳ���Ϳ� ���� ����
	AIControllerClass = AEnemyGuardianAIController::StaticClass(); // ����� AI ��Ʈ�ѷ� ����
}

void AEnemyGuardian::BeginPlay()
{
	Super::BeginPlay(); // �θ� Ŭ���� BeginPlay ȣ��

	if (!GetController()) // AI ��Ʈ�ѷ��� �Ҵ���� �ʾҴٸ�
	{
		// �� ��Ʈ�ѷ��� �����Ͽ� �� ĳ���Ϳ� ���ǽ�Ŵ
		AEnemyGuardianAIController* NewController = GetWorld()->SpawnActor<AEnemyGuardianAIController>();
		if (NewController)
		{
			NewController->Possess(this);
		}
	}

	// ���� ���� �� ����
	if (ShieldClass)
	{
		EquippedShield = GetWorld()->SpawnActor<AEnemyGuardianShield>(ShieldClass); // ���忡 ���� ���� ����
		if (EquippedShield)
		{
			EquippedShield->SetOwner(this); // ������ �����ڸ� �� ĳ���ͷ� ����
			USkeletalMeshComponent* MeshComp = GetMesh();
			if (MeshComp)
			{
				// ĳ���� �޽��� Ư�� ���Ͽ� ���и� ����
				FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
				EquippedShield->AttachToComponent(MeshComp, AttachmentRules, FName("EnemyGuardianShieldSocket"));
			}
		}
	}

	// ���к� ���� �� ����
	if (BatonClass)
	{
		EquippedBaton = GetWorld()->SpawnActor<AEnemyGuardianBaton>(BatonClass); // ���忡 ���к� ���� ����
		if (EquippedBaton)
		{
			EquippedBaton->SetOwner(this); // ���к��� �����ڸ� �� ĳ���ͷ� ����
			USkeletalMeshComponent* MeshComp = GetMesh();
			if (MeshComp)
			{
				// ĳ���� �޽��� Ư�� ���Ͽ� ���к��� ����
				FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
				EquippedBaton->AttachToComponent(
					MeshComp, AttachmentRules, FName("EnemyGuardianBatonSocket")
				);
			}
		}
	}

	SetUpAI(); // AI �ʱ� ���� �Լ� ȣ��
	PlaySpawnIntroAnimation(); // ���� ��Ʈ�� �ִϸ��̼� ���
}

void AEnemyGuardian::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemyGuardian::PlaySpawnIntroAnimation()
{
	UEnemyGuardianAnimInstance* AnimInstance = Cast<UEnemyGuardianAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance) return;

	bIsPlayingIntro = true; // ��Ʈ�� ��� �� ���·� ��ȯ
	bCanAttack = false; // ���� �߿��� ���� �Ұ�

	AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement(); // ���� �ִϸ��̼� ���� �̵� ����
	}

	PlayAnimMontage(SpawnIntroMontage); // ���� ��Ʈ�� ��Ÿ�� ���

	// ��Ÿ�ְ� ������ OnIntroMontageEnded �Լ��� ȣ��ǵ��� ��������Ʈ ���ε�
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyGuardian::OnIntroMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, SpawnIntroMontage);
}

void AEnemyGuardian::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsPlayingIntro = false; // ��Ʈ�� ��� ���� ����
	bCanAttack = true; // ���� ���� ���·� ��ȯ
}

void AEnemyGuardian::SetUpAI()
{
	if (GetController())
	{
		UE_LOG(LogTemp, Warning, TEXT("AI Controller is set up"));
	}
}

void AEnemyGuardian::PlayShieldAttackAnimation()
{
	// ������ �Ұ����� ����(��Ʈ��, �����ı�, �ٸ�������, ����, ���)�̸� �Լ� ����
	if (ShieldAttackMontages.Num() == 0 || bIsPlayingIntro || bIsShieldDestroyed || bIsShieldAttacking || bIsBatonAttacking || bIsStunned || bIsDead)
		return;

	int32 RandomIndex = FMath::RandRange(0, ShieldAttackMontages.Num() - 1); // ���� ���� ��Ÿ�� �� �ϳ��� ���� ����
	UAnimMontage* SelectedMontage = ShieldAttackMontages[RandomIndex];

	if (SelectedMontage)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			bIsShieldAttacking = true; // ���� ���� �� ���·� ��ȯ

			AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
			if (AICon)
			{
				AICon->StopMovement(); // ���� �ִϸ��̼� ���� �̵� ����
			}

			AnimInstance->Montage_Play(SelectedMontage, 0.7f); // ���õ� ��Ÿ�ָ� 0.7������� ���
			EquippedShield->StartShieldAttack(); // ������ ���� ���� Ȱ��ȭ

			// ��Ÿ�� ���� �� OnShieldAttackMontageEnded ȣ���ϵ��� ��������Ʈ ���ε�
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AEnemyGuardian::OnShieldAttackMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
		}
	}
}

void AEnemyGuardian::OnShieldAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsShieldAttacking = false; // ���� ���� �� ���� ����
	if (EquippedShield)
	{
		EquippedShield->EndShieldAttack(); // ������ ���� ���� ��Ȱ��ȭ
	}
}

void AEnemyGuardian::PlayNormalAttackAnimation()
{
	// ������ �Ұ����� ����(���� ���ı�, �ٸ������� ��)�̸� �Լ� ����
	if (NormalAttackMontages.Num() == 0 || bIsPlayingIntro || bIsBatonAttacking || !bIsShieldDestroyed || bIsShieldAttacking || bIsStunned || bIsDead)
		return;

	int32 RandomIndex = FMath::RandRange(0, NormalAttackMontages.Num() - 1); // ���к� ���� ��Ÿ�� �� �ϳ��� ���� ����
	UAnimMontage* SelectedMontage = NormalAttackMontages[RandomIndex];

	if (SelectedMontage)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			bIsBatonAttacking = true; // ���к� ���� �� ���·� ��ȯ

			AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
			if (AICon)
			{
				AICon->StopMovement(); // �̵� ����
			}

			AnimInstance->Montage_Play(SelectedMontage, 1.0f); // ��Ÿ�� ���
			if (EquippedBaton)
			{
				EquippedBaton->StartAttack(); // ���к��� ���� ���� Ȱ��ȭ
			}

			// ��Ÿ�� ���� �� OnNormalAttackMontageEnded ȣ���ϵ��� ��������Ʈ ���ε�
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AEnemyGuardian::OnNormalAttackMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
		}
	}
}
void AEnemyGuardian::OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsBatonAttacking = false; // ���к� ���� �� ���� ����
	if (EquippedBaton)
	{
		EquippedBaton->EndAttack(); // ���к��� ���� ���� ��Ȱ��ȭ
	}
}

void AEnemyGuardian::PlayBlockAnimation()
{
	if (BlockMontage && !bIsPlayingIntro)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		// �̹� ��� �ִϸ��̼��� ��� ���� �ƴ϶�� ���
		if (AnimInstance && !AnimInstance->Montage_IsPlaying(BlockMontage))
		{
			AnimInstance->Montage_Play(BlockMontage);
		}
	}
}

void AEnemyGuardian::Stun()
{
	if (StunMontage && !bIsPlayingIntro)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && !AnimInstance->Montage_IsPlaying(StunMontage))
		{
			bIsStunned = true; // ���� ���·� ��ȯ
			AnimInstance->Montage_Play(StunMontage); // ���� ��Ÿ�� ���

			// ��Ÿ�� ���� �� OnStunMontageEnded ȣ���ϵ��� ��������Ʈ ���ε�
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AEnemyGuardian::OnStunMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, StunMontage);
		}
	}
}

void AEnemyGuardian::OnStunMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == StunMontage)
	{
		bIsStunned = false; // ���� ���� ����
	}
}

float AEnemyGuardian::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || bIsPlayingIntro) return 0.0f;

	// ���а� �ı����� �ʾҰ� �����Ǿ� �ִٸ�, �������� ���п� ����
	if (!bIsShieldDestroyed && EquippedShield)
	{
		PlayBlockAnimation(); // ��� �ִϸ��̼� ���
		EquippedShield->ShieldHealth -= DamageAmount; // ���� ü�� ����

		if (EquippedShield->ShieldHealth <= 0) // ���� ü���� 0 ���ϰ� �Ǹ�
		{
			EquippedShield->SetActorHiddenInGame(true); // ���и� ����
			bIsShieldDestroyed = true; // ���� �ı� ���·� ��ȯ
			Stun(); // ���� ���¿� ����
		}
		return 0.0f; // ����� ��ü�� ���ظ� ���� ����
	}

	// ���к� ���� ���� ���� �ǰ� �ִϸ��̼� ���� ü�¸� ����
	if (bIsBatonAttacking)
	{
		Health -= DamageAmount;
		if (Health <= 0) // ���ظ� �԰� ü���� 0 ���ϰ� �Ǹ�
		{
			// ���� ������ ��� �ߴ�
			bIsBatonAttacking = false;
			if (EquippedBaton)
			{
				EquippedBaton->EndAttack();
			}
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				AnimInstance->Montage_Stop(0.1f); // ������� ��Ÿ�ָ� �ε巴�� ����
			}
			Die(); // ��� ó��
		}
		return DamageAmount;
	}

	// ���� ���� ���� �ǰ� �ִϸ��̼� ���� ü�¸� ����
	if (bIsStunned)
	{
		Health -= DamageAmount;
		if (Health <= 0)
		{
			Die();
		}
		return DamageAmount;
	}

	// ���а� �ı��� �Ŀ��� ����� ��ü�� ���� ���ظ� ����
	Health -= DamageAmount;
	if (HitMontage && Health > 0) // ü���� �����ִٸ� �ǰ� ��Ÿ�� ���
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && !AnimInstance->Montage_IsPlaying(HitMontage))
		{
			AnimInstance->Montage_Play(HitMontage);
		}
	}

	if (Health <= 0) // ü���� 0 ���ϸ� ���
	{
		Die();
	}

	return DamageAmount; // ���� ����� ������ �� ��ȯ
}

void AEnemyGuardian::Die()
{
	if (bIsDead) return; // �ߺ� ��� ó�� ����
	bIsDead = true; // ��� ���·� ��ȯ
	UEnemyGuardianAnimInstance* AnimInstance = Cast<UEnemyGuardianAnimInstance>(GetMesh()->GetAnimInstance());

	if (!AnimInstance || DeadMontages.Num() == 0) // �ִϸ��̼� ����� �Ұ����� ���
	{
		HideEnemy(); // ��� ���� �Լ� ȣ��
		return;
	}

	int32 RandIndex = FMath::RandRange(0, DeadMontages.Num() - 1); // ��� ��Ÿ�� �� �ϳ��� ���� ����
	UAnimMontage* SelectedMontage = DeadMontages[RandIndex];
	float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // ��Ÿ�� ���

	if (PlayResult > 0.0f) // ��Ÿ�� ����� �����ߴٸ�
	{
		float MontageLength = SelectedMontage->GetPlayLength(); // ��Ÿ���� ��ü ����
		float HidePercent = 0.9f; // ��Ÿ���� 90% �������� ���� ����
		float HideTime = MontageLength * HidePercent; // ���� �ð� ���

		// ���� �ð� �Ŀ� HideEnemy �Լ��� ȣ���ϵ��� Ÿ�̸� ����
		GetWorld()->GetTimerManager().SetTimer(
			DeathTimerHandle, this, &AEnemyGuardian::HideEnemy, HideTime, false
		);
	}
	else // ��Ÿ�� ����� �����ߴٸ�
	{
		HideEnemy(); // ��� ����
	}

	AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (AICon)
	{
		// AICon->StopAI(); // AI ���� �ߴ� (���� �Լ����� ó��)
	}

	// �̵� ���� ������Ʈ ��Ȱ��ȭ
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	SetActorTickEnabled(false); // ������ ���� ���� ƽ ��Ȱ��ȭ
}

void AEnemyGuardian::ApplyBaseWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = 250.0f;
	GetCharacterMovement()->MaxAcceleration = 5000.0f;
}

void AEnemyGuardian::HideEnemy()
{
	if (!bIsDead) return; // ��� ���°� �ƴ϶�� ���� �� ��

	// ���Ӹ�忡 ���� �ı��Ǿ����� �˸�
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->OnEnemyDestroyed(this);
	}

	// 1. �̺�Ʈ �� ��������Ʈ ����
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // �� ��ü�� ������ ��� Ÿ�̸� ����

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && IsValid(AnimInstance))
	{
		// ��������Ʈ ���ε� ���� ���� (�޸� ���� ����)
		AnimInstance->OnMontageEnded.RemoveAll(this);
		AnimInstance->OnMontageBlendingOut.RemoveAll(this);
		AnimInstance->OnMontageStarted.RemoveAll(this);
	}

	// 2. AI �ý��� ���� ����
	AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (AICon && IsValid(AICon))
	{
		// AICon->StopAI(); // AI ���� �ߴ�
		AICon->UnPossess(); // ��Ʈ�ѷ��� ���� ���� ����
		AICon->Destroy(); // AI ��Ʈ�ѷ� ���� ��ü�� �ı�
	}

	// 3. ���� �ý��� ����
	if (EquippedShield && IsValid(EquippedShield))
	{
		EquippedShield->HideShield(); // ���� ���� �� ���� �Լ� ȣ��
		EquippedShield = nullptr; // ���� ���� ����
	}
	if (EquippedBaton && IsValid(EquippedBaton))
	{
		EquippedBaton->HideBaton(); // ���к� ���� �� ���� �Լ� ȣ��
		EquippedBaton = nullptr; // ���к� ���� ����
	}

	// 4. �����Ʈ �ý��� ����
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp && IsValid(MovementComp))
	{
		MovementComp->DisableMovement(); // �̵� ��Ȱ��ȭ
		MovementComp->StopMovementImmediately(); // ���� �̵� ��� �ߴ�
		MovementComp->SetMovementMode(EMovementMode::MOVE_None); // �׺���̼� �ý��ۿ��� ����
		MovementComp->SetComponentTickEnabled(false); // ������Ʈ ƽ ��Ȱ��ȭ
	}

	// 5. �޽� ������Ʈ ����
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp && IsValid(MeshComp))
	{
		MeshComp->SetVisibility(false); // ���������� �ʵ��� ���ü� ��Ȱ��ȭ
		MeshComp->SetHiddenInGame(true); // ���� ������ ���� ó��
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �浹 �˻� ��Ȱ��ȭ
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // ��� ä�ο� ���� �浹 ���� ����
		MeshComp->SetComponentTickEnabled(false); // ������Ʈ ƽ ��Ȱ��ȭ
		MeshComp->SetAnimInstanceClass(nullptr); // ����� �ִ� �ν��Ͻ� ���� ����
		MeshComp->SetSkeletalMesh(nullptr); // ����� ���̷�Ż �޽� ���� ����
	}

	// 6. ���� ��ü�� �ý��� ����
	SetActorHiddenInGame(true); // ���͸� ���� ������ ����
	SetActorEnableCollision(false); // ������ �浹 ��Ȱ��ȭ
	SetActorTickEnabled(false); // ������ Tick ��Ȱ��ȭ
	SetCanBeDamaged(false); // �������� ���� �� ������ ����

	// 7. ���� �����ӿ� �����ϰ� ���� ���� (ũ���� ����)
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyGuardian>(this)]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
			{
				WeakThis->Destroy(); // ���͸� ���忡�� ������ ����
			}
		});
}

void AEnemyGuardian::StartAttack()
{
	if (EquippedBaton)
	{
		EquippedBaton->EnableAttackHitDetection(); // ���к� ���� ���� Ȱ��ȭ
	}
}

void AEnemyGuardian::EndAttack()
{
	if (EquippedBaton)
	{
		EquippedBaton->DisableAttackHitDetection(); // ���к� ���� ���� ��Ȱ��ȭ
	}
}