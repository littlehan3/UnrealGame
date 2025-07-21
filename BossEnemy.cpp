#include "BossEnemy.h"
#include "BossEnemyAnimInstance.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "BossEnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "MainGameModeBase.h"
#include "BossProjectile.h"

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

	PlayBossSpawnIntroAnimation(); // ���� ���� �ִϸ��̼� ���
}

void ABossEnemy::PlayBossSpawnIntroAnimation()
{
	if (BossSpawnIntroMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No boss intro montages found - skipping intro animation"));
		return;
	}

	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Boss AnimInstance not found"));
		return;
	}

	bIsPlayingBossIntro = true;
	bCanBossAttack = false; // ���� �߿��� ���� �Ұ�
	bIsFullBodyAttacking = true; // ���� �ִϸ��̼����� ó��

	// AI �̵� ����
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement();
	}

	// �̵� ����
	DisableBossMovement();

	// ���� ��Ÿ�� ���
	AnimInstance->bUseUpperBodyBlend = false; // ���� �ִϸ��̼�
	AnimInstance->Montage_Stop(0.1f, nullptr); // �ٸ� ��Ÿ�� ����

	int32 RandomIndex = FMath::RandRange(0, BossSpawnIntroMontages.Num() - 1);
	UAnimMontage* SelectedMontage = BossSpawnIntroMontages[RandomIndex];

	if (SelectedMontage)
	{
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

		if (PlayResult > 0.0f)
		{
			// ���� ��Ÿ�� ���� ��������Ʈ ���ε�
			FOnMontageEnded IntroEndDelegate;
			IntroEndDelegate.BindUObject(this, &ABossEnemy::OnBossIntroMontageEnded);
			AnimInstance->Montage_SetEndDelegate(IntroEndDelegate, SelectedMontage);

			UE_LOG(LogTemp, Warning, TEXT("Boss intro montage playing: %s"), *SelectedMontage->GetName());
		}
		else
		{
			// ��Ÿ�� ��� ���� �� ��� Ȱ��ȭ
			UE_LOG(LogTemp, Error, TEXT("Boss intro montage failed to play"));
			bIsPlayingBossIntro = false;
			bCanBossAttack = true;
			bIsFullBodyAttacking = false;
			EnableBossMovement();
		}
	}
}

void ABossEnemy::OnBossIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Boss Intro Montage Ended"));

	// �ִϸ��̼� ���� ����
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false;
	}

	// ���Ű��� ���� �� �̵� ���
	bIsFullBodyAttacking = false;
	bIsPlayingBossIntro = false;
	EnableBossMovement();

	// AI ��Ʈ�ѷ��� ���� ���� �˸�
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true;
	UE_LOG(LogTemp, Warning, TEXT("Boss ready for combat after intro"));
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

	// ABP���� �÷��̾� �ٶ󺸱� Ȱ��ȭ
	AnimInstance->bShouldLookAtPlayer = true;
	AnimInstance->LookAtSpeed = 8.0f; // ���� �ÿ��� �� ���� ȸ��

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
			AnimInstance->bShouldLookAtPlayer = false; // ���н� �ٶ󺸱� ��Ȱ��ȭ
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
		// �÷��̾� �ٶ󺸱� ��Ȱ��ȭ
		AnimInstance->bShouldLookAtPlayer = false;
		AnimInstance->LookAtSpeed = 5.0f; // �⺻ �ӵ��� ����
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

void ABossEnemy::PlayBossUpperBodyAttackAnimation()
{
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance || BossUpperBodyMontages.Num() == 0) return;

	// �÷��̾� �ٶ󺸱� Ȱ��ȭ
	AnimInstance->bShouldLookAtPlayer = true;
	AnimInstance->LookAtSpeed = 6.0f; // ��ü ���ݽ� �ӵ�
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
			AnimInstance->bShouldLookAtPlayer = false; // ���н� ��Ȱ��ȭ
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
		// �÷��̾� �ٶ󺸱� ��Ȱ��ȭ
		AnimInstance->bShouldLookAtPlayer = false;
		AnimInstance->LookAtSpeed = 5.0f;
	}

	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true; // ���� ���� ���·� ����
}

void ABossEnemy::PlayBossTeleportAnimation()
{
	if (!bCanTeleport || bIsBossTeleporting || bIsBossDead) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;

	// �ڷ���Ʈ ��Ÿ�ְ� ���ٸ� �ڷ���Ʈ ���
	if (BossTeleportMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No teleport montages available - teleport cancelled"));
		return;
	}

	bIsBossTeleporting = true;
	bCanBossAttack = false;
	bIsFullBodyAttacking = true; // ���� �ִϸ��̼����� ó��
	bIsInvincible = true; // ���� ���� Ȱ��ȭ

	// AI �̵� ����
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement();
	}

	// �̵� ����
	DisableBossMovement();

	// �ڷ���Ʈ ��Ÿ�� ���
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // ���� �ִϸ��̼�
		AnimInstance->Montage_Stop(0.1f, nullptr); // �ٸ� ��Ÿ�� ����
		// ABP���� �÷��̾� �ٶ󺸱� Ȱ��ȭ
		AnimInstance->bShouldLookAtPlayer = true;
		AnimInstance->LookAtSpeed = 8.0f; // ���� �ÿ��� �� ���� ȸ��

		int32 RandomIndex = FMath::RandRange(0, BossTeleportMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossTeleportMontages[RandomIndex];

		if (SelectedMontage)
		{
			float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

			if (PlayResult > 0.0f)
			{
				// �ڷ���Ʈ ��Ÿ�� ���� ��������Ʈ ���ε�
				FOnMontageEnded TeleportEndDelegate;
				TeleportEndDelegate.BindUObject(this, &ABossEnemy::OnTeleportMontageEnded);
				AnimInstance->Montage_SetEndDelegate(TeleportEndDelegate, SelectedMontage);

				// ��Ÿ�� �߰� Ÿ�̹�(50%)���� ���� �ڷ���Ʈ ����
				float MontageLength = SelectedMontage->GetPlayLength();
				float TeleportTiming = MontageLength * 0.5f; // 50% �������� �ڷ���Ʈ

				GetWorld()->GetTimerManager().SetTimer(
					TeleportExecutionTimer, // �ڷ���Ʈ �̵� Ÿ�̸�
					this,
					&ABossEnemy::ExecuteTeleport,
					TeleportTiming,
					false
				);

				UE_LOG(LogTemp, Warning, TEXT("Teleport montage playing - will teleport at 50%% (%.2f seconds)"), TeleportTiming);
			}
			else
			{
				// ��Ÿ�� ��� ���н� �ڷ���Ʈ ���
				UE_LOG(LogTemp, Error, TEXT("Teleport montage failed to play"));
				bIsBossTeleporting = false;
				bCanBossAttack = true;
				bIsFullBodyAttacking = false;
				bIsInvincible = false;
				EnableBossMovement();
			}
		}
		else
		{
			// ���õ� ��Ÿ�ְ� ������ �ڷ���Ʈ ���
			UE_LOG(LogTemp, Error, TEXT("Selected teleport montage is null"));
			bIsBossTeleporting = false;
			bCanBossAttack = true;
			bIsFullBodyAttacking = false;
			EnableBossMovement();
		}
	}
	else
	{
		// �ִϸ��̼� �ν��Ͻ��� ������ �ڷ���Ʈ ���
		UE_LOG(LogTemp, Error, TEXT("Animation instance not found - teleport cancelled"));
		bIsBossTeleporting = false;
		bCanBossAttack = true;
		bIsFullBodyAttacking = false;
		EnableBossMovement();
	}
}

FVector ABossEnemy::CalculateTeleportLocation()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return GetActorLocation();

	FVector BossLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector DirectionAwayFromPlayer = (BossLocation - PlayerLocation).GetSafeNormal();
	FVector TeleportLocation = BossLocation + (DirectionAwayFromPlayer * TeleportDistance);

	// �׺���̼� �ý����� ����� ��ȿ�� ��ġ ã��
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSystem)
	{
		FNavLocation ValidLocation;
		if (NavSystem->ProjectPointToNavigation(TeleportLocation, ValidLocation, FVector(200.0f, 200.0f, 500.0f)))
		{
			// ĳ���� ���� ���� �߰�
			return AdjustHeightForCharacter(ValidLocation.Location);
		}
		else
		{
			// ��ȿ�� ��ġ�� ã�� ���� ��� �ٸ� ����� �õ�
			TArray<FVector> AlternativeDirections = {
				FVector(1, 0, 0), FVector(-1, 0, 0), FVector(0, 1, 0), FVector(0, -1, 0),
				FVector(0.707f, 0.707f, 0), FVector(-0.707f, 0.707f, 0),
				FVector(0.707f, -0.707f, 0), FVector(-0.707f, -0.707f, 0)
			};

			for (const FVector& Direction : AlternativeDirections)
			{
				FVector TestLocation = BossLocation + (Direction * TeleportDistance);
				if (NavSystem->ProjectPointToNavigation(TestLocation, ValidLocation, FVector(200.0f, 200.0f, 500.0f)))
				{
					// ĳ���� ���� ���� �߰�
					return AdjustHeightForCharacter(ValidLocation.Location);
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("Could not find valid teleport location - staying in place"));
			return BossLocation; // ��ȿ�� ��ġ�� ã�� ���� ��� ���� ��ġ ��ȯ
		}
	}

	// �׺���̼� �ý����� ���� ��쿡�� ���� ���� ����
	return AdjustHeightForCharacter(TeleportLocation);
}

FVector ABossEnemy::AdjustHeightForCharacter(const FVector& TargetLocation)
{
	// ĳ������ ĸ�� ������Ʈ���� ���� ���� ��������
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	if (!CapsuleComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("No capsule component found - using default height adjustment"));
		return TargetLocation + FVector(0, 0, 90.0f); // �⺻������ 90 ���� ����
	}

	float CapsuleHalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();

	// ���� Ʈ���̽��� ��Ȯ�� ���� ���� ã��
	FVector StartLocation = TargetLocation + FVector(0, 0, 500.0f); // ���������� ����
	FVector EndLocation = TargetLocation + FVector(0, 0, -500.0f);  // �Ʒ����� �˻�

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // �ڱ� �ڽ��� ����

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_WorldStatic, // �������� �浹�� �˻�
		QueryParams
	);

	if (bHit)
	{
		// ���鿡�� ĳ���� ���� ���̸�ŭ ���� ��ġ
		FVector AdjustedLocation = HitResult.Location + FVector(0, 0, CapsuleHalfHeight + 5.0f); // 5.0f�� ���� ����

		UE_LOG(LogTemp, Warning, TEXT("Height adjusted teleport - Ground: %s, Final: %s"),
			*HitResult.Location.ToString(), *AdjustedLocation.ToString());

		return AdjustedLocation;
	}
	else
	{
		// ���� Ʈ���̽� ���н� �⺻ ���� ����
		FVector AdjustedLocation = TargetLocation + FVector(0, 0, CapsuleHalfHeight + 5.0f);

		UE_LOG(LogTemp, Warning, TEXT("Line trace failed - using default height adjustment: %s"),
			*AdjustedLocation.ToString());

		return AdjustedLocation;
	}
}

void ABossEnemy::ExecuteTeleport()
{
	FVector TeleportLocation = CalculateTeleportLocation();

	// ���� �ڷ���Ʈ ����
	SetActorLocation(TeleportLocation);
	UE_LOG(LogTemp, Warning, TEXT("Boss teleported during montage to: %s"), *TeleportLocation.ToString());
}

void ABossEnemy::OnTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Retreat Teleport Montage Ended - Starting pause before next action"));

	// �ִϸ��̼� ���� ����
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false;
	}

	bIsInvincible = false;

	// �ڷ���Ʈ �� ��� ���� (PostTeleportPauseTime��ŭ)
	// ���� �߿��� ���� �̵��� ������� ����
	GetWorld()->GetTimerManager().SetTimer(
		PostTeleportPauseTimer,
		this,
		&ABossEnemy::OnPostTeleportPauseEnd,
		PostTeleportPauseTime,
		false
	);

	// �ڷ���Ʈ ��Ÿ�� ����
	bCanTeleport = false;
	GetWorld()->GetTimerManager().SetTimer(
		TeleportCooldownTimer,
		this,
		&ABossEnemy::OnTeleportCooldownEnd,
		TeleportCooldown,
		false
	);

	// ���� bCanBossAttack�� false�� ���� (���� �� ������ ���� ������)
}


void ABossEnemy::OnTeleportCooldownEnd()
{
	bCanTeleport = true;
	UE_LOG(LogTemp, Warning, TEXT("Boss can teleport again"));
}

void ABossEnemy::PlayBossAttackTeleportAnimation()
{
	if (bIsBossAttackTeleporting || bIsBossDead) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;

	// ���ݿ� �ڷ���Ʈ ��Ÿ�ְ� ���ٸ� ���
	if (BossAttackTeleportMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No attack teleport montages available - teleport cancelled"));
		return;
	}

	bIsBossAttackTeleporting = true;
	bCanBossAttack = false;
	bIsFullBodyAttacking = true; // ���� �ִϸ��̼����� ó��
	bIsInvincible = true;

	// AI �̵� ����
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement();
	}

	// �̵� ����
	DisableBossMovement();

	// ���ݿ� �ڷ���Ʈ ��Ÿ�� ���
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // ���� �ִϸ��̼�
		AnimInstance->Montage_Stop(0.1f, nullptr); // �ٸ� ��Ÿ�� ����
		// ABP���� �÷��̾� �ٶ󺸱� Ȱ��ȭ
		AnimInstance->bShouldLookAtPlayer = true;
		AnimInstance->LookAtSpeed = 8.0f; // ���� �ÿ��� �� ���� ȸ��

		int32 RandomIndex = FMath::RandRange(0, BossAttackTeleportMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossAttackTeleportMontages[RandomIndex];

		if (SelectedMontage)
		{
			float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

			if (PlayResult > 0.0f)
			{
				// ���ݿ� �ڷ���Ʈ ��Ÿ�� ���� ��������Ʈ ���ε�
				FOnMontageEnded AttackTeleportEndDelegate;
				AttackTeleportEndDelegate.BindUObject(this, &ABossEnemy::OnAttackTeleportMontageEnded);
				AnimInstance->Montage_SetEndDelegate(AttackTeleportEndDelegate, SelectedMontage);

				// ��Ÿ�� �߰� Ÿ�̹�(40%)���� ���� �ڷ���Ʈ ���� (���ݿ��̹Ƿ� ���� ������)
				float MontageLength = SelectedMontage->GetPlayLength();
				float TeleportTiming = MontageLength * 0.4f; // 40% �������� �ڷ���Ʈ

				GetWorld()->GetTimerManager().SetTimer(
					AttackTeleportExecutionTimer, // ���� �ڷ���Ʈ �̵� Ÿ�̸�
					this,
					&ABossEnemy::ExecuteAttackTeleport,
					TeleportTiming,
					false
				);

				UE_LOG(LogTemp, Warning, TEXT("Attack teleport montage playing - will teleport at 40%% (%.2f seconds)"), TeleportTiming);
			}
			else
			{
				// ��Ÿ�� ��� ���н� ���
				UE_LOG(LogTemp, Error, TEXT("Attack teleport montage failed to play"));
				bIsBossAttackTeleporting = false;
				bCanBossAttack = true;
				bIsFullBodyAttacking = false;
				bIsInvincible = false;
				EnableBossMovement();
			}
		}
	}
}

FVector ABossEnemy::CalculateAttackTeleportLocation()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return GetActorLocation();

	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector BossLocation = GetActorLocation();

	// �÷��̾ �ٶ󺸴� ���� (Forward Vector)
	FVector PlayerForward = PlayerPawn->GetActorForwardVector();

	// �÷��̾��� ���� ���� ���
	FVector BehindPlayerDirection = -PlayerForward; // �ݴ� ����

	// ���� ���⿡ �ణ�� ������ �߰� (�¿�� �ణ ��ȭ)
	float RandomOffset = FMath::RandRange(-45.0f, 45.0f); // -45�� ~ +45��
	FVector RotatedDirection = BehindPlayerDirection.RotateAngleAxis(RandomOffset, FVector(0, 0, 1));

	// �켱������ �������� �ڷ���Ʈ �õ�
	FVector TeleportLocation = PlayerLocation + (RotatedDirection * AttackTeleportRange);

	// �׺���̼� �ý������� ��ȿ�� ��ġ Ȯ��
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSystem)
	{
		FNavLocation ValidLocation;
		if (NavSystem->ProjectPointToNavigation(TeleportLocation, ValidLocation, FVector(200.0f, 200.0f, 500.0f)))
		{
			UE_LOG(LogTemp, Warning, TEXT("Attack teleport to behind player successful"));
			return AdjustHeightForCharacter(ValidLocation.Location);
		}

		// ������ �ȵǸ� ���� ���� ����� �õ�
		TArray<FVector> AlternativeDirections = {
			FVector(1, 0, 0), FVector(-1, 0, 0), FVector(0, 1, 0), FVector(0, -1, 0),
			FVector(0.707f, 0.707f, 0), FVector(-0.707f, 0.707f, 0),
			FVector(0.707f, -0.707f, 0), FVector(-0.707f, -0.707f, 0)
		};

		for (const FVector& Direction : AlternativeDirections)
		{
			FVector TestLocation = PlayerLocation + (Direction * AttackTeleportRange);
			if (NavSystem->ProjectPointToNavigation(TestLocation, ValidLocation, FVector(200.0f, 200.0f, 500.0f)))
			{
				UE_LOG(LogTemp, Warning, TEXT("Attack teleport to alternative direction"));
				return AdjustHeightForCharacter(ValidLocation.Location);
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Could not find valid attack teleport location - staying in place"));
		return BossLocation;
	}

	return AdjustHeightForCharacter(TeleportLocation);
}

void ABossEnemy::ExecuteAttackTeleport()
{
	FVector TeleportLocation = CalculateAttackTeleportLocation();

	// ���� �ڷ���Ʈ ����
	SetActorLocation(TeleportLocation);
	UE_LOG(LogTemp, Warning, TEXT("Boss attack teleported to: %s"), *TeleportLocation.ToString());
}

void ABossEnemy::OnAttackTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Attack Teleport Montage Ended - Resuming combat immediately"));

	// �ִϸ��̼� ���� ����
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false;
	}

	// ���ݿ� �ڷ���Ʈ �Ŀ��� ���� ���� ��� ���� �簳
	bIsFullBodyAttacking = false;
	bIsBossAttackTeleporting = false;
	bIsInvincible = false;
	EnableBossMovement();

	// AI ��Ʈ�ѷ��� ���ݿ� �ڷ���Ʈ ���� �˸�
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossAttackTeleportEnded(); // ��� ���� �簳
	}

	bCanBossAttack = true;
}

void ABossEnemy::OnPostTeleportPauseEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("Post teleport pause ended - choosing next action"));

	// ���� �� 2���� ������: ��� ĳ���Ϳ��� �ڷ���Ʈ OR ���Ÿ� ����
	bool bShouldUseAttackTeleport = FMath::RandBool(); // 50% Ȯ��

	if (bShouldUseAttackTeleport)
	{
		UE_LOG(LogTemp, Warning, TEXT("Chose attack teleport after retreat pause"));
		// ��� ĳ���Ϳ��� �ڷ���Ʈ (���� �ִϸ��̼� ����)
		PlayBossAttackTeleportAnimation();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Chose ranged attack after retreat pause"));
		// ���Ÿ� ���� ����
		PlayBossRangedAttackAnimation();
	}

	// ���Ű��� ���� �� �̵� ����� �� ���õ� �ൿ�� ���� �� ó����
	bIsFullBodyAttacking = false;
	bIsBossTeleporting = false;
	bIsInvincible = false;
	EnableBossMovement();
}

void ABossEnemy::PlayBossRangedAttackAnimation()
{
	if (bIsBossRangedAttacking || bIsBossDead) return;

	// ���Ÿ� ���� ��Ÿ�ְ� ���ٸ� ���
	if (BossRangedAttackMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No ranged attack montages available - attack cancelled"));
		return;
	}

	bIsBossRangedAttacking = true;
	bCanBossAttack = false;
	bIsFullBodyAttacking = true; // ���� �ִϸ��̼����� ó��

	// AI �̵� ����
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement();
	}

	// �̵� ����
	DisableBossMovement();

	// ���Ÿ� ���� ��Ÿ�� ���
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // ���� �ִϸ��̼�
		AnimInstance->Montage_Stop(0.1f, nullptr); // �ٸ� ��Ÿ�� ����
		// ABP���� �÷��̾� �ٶ󺸱� Ȱ��ȭ
		AnimInstance->bShouldLookAtPlayer = true;
		AnimInstance->LookAtSpeed = 8.0f; // ���� �ÿ��� �� ���� ȸ��

		int32 RandomIndex = FMath::RandRange(0, BossRangedAttackMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossRangedAttackMontages[RandomIndex];

		if (SelectedMontage)
		{
			float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

			if (PlayResult > 0.0f)
			{
				// ���Ÿ� ���� ��Ÿ�� ���� ��������Ʈ ���ε�
				FOnMontageEnded RangedAttackEndDelegate;
				RangedAttackEndDelegate.BindUObject(this, &ABossEnemy::OnRangedAttackMontageEnded);
				AnimInstance->Montage_SetEndDelegate(RangedAttackEndDelegate, SelectedMontage);

				UE_LOG(LogTemp, Warning, TEXT("Ranged attack montage playing (DUMMY IMPLEMENTATION)"));

				// TODO: ��Ÿ�� �߰��� ����ü �߻� ���� �߰� ����
				// ��Ÿ�� ���� �ۼ�Ʈ �������� ����ü ����
			}
			else
			{
				// ��Ÿ�� ��� ���н� ���
				UE_LOG(LogTemp, Error, TEXT("Ranged attack montage failed to play"));
				bIsBossRangedAttacking = false;
				bCanBossAttack = true;
				bIsFullBodyAttacking = false;
				EnableBossMovement();
			}
		}
	}
}

#include "BossProjectile.h"   // Ŭ���� ���� ����

void ABossEnemy::SpawnBossProjectile()
{
	if (!BossProjectileClass) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;

	// �߻� ��ġ�� ����
	const FVector SpawnLocation = GetActorLocation()
		+ GetActorForwardVector() * MuzzleOffset.X
		+ GetActorRightVector() * MuzzleOffset.Y
		+ FVector(0, 0, MuzzleOffset.Z);

	const FVector TargetLocation = PlayerPawn->GetActorLocation();
	const FVector ShootDirection = (TargetLocation - SpawnLocation).GetSafeNormal();

	FTransform SpawnTM(ShootDirection.Rotation(), SpawnLocation);

	// ���ø� �Ķ���Ϳ� ���� Ÿ�� ��ġ
	ABossProjectile* Projectile = GetWorld()->SpawnActorDeferred<ABossProjectile>(
		BossProjectileClass,   // TSubclassOf<ABossProjectile>
		SpawnTM,               // FTransform
		this,                  // Owner
		this,                  // Instigator (APawn*)
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (Projectile)
	{
		Projectile->SetShooter(this);
		UGameplayStatics::FinishSpawningActor(Projectile, SpawnTM);
		Projectile->FireInDirection(ShootDirection);
	}
}

void ABossEnemy::OnRangedAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Ranged Attack Montage Ended - Resuming chase logic"));

	// �ִϸ��̼� ���� ����
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false;
	}

	// ���Ű��� ���� �� �̵� ���
	bIsFullBodyAttacking = false;
	bIsBossRangedAttacking = false;
	EnableBossMovement();

	// AI ��Ʈ�ѷ��� ���Ÿ� ���� ���� �˸�
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossRangedAttackEnded();
	}

	bCanBossAttack = true;
}

void ABossEnemy::PlayBossStealthAttackAnimation()
{
    if (!StealthStartMontage) return;
    
    CurrentStealthPhase = 1;
    bIsStealthStarting = true;
	bIsInvincible = true; // ����
    
    PlayAnimMontage(StealthStartMontage);
    
    // ��Ÿ�� ���� ��������Ʈ ���ε�
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &ABossEnemy::OnStealthStartMontageEnded);
    GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, StealthStartMontage);
    
    UE_LOG(LogTemp, Warning, TEXT("Stealth Phase 1: Start Animation"));

	// AI ��Ʈ�ѷ��� ���ڽ� ���� �˸�
	ABossEnemyAIController* BossAI = Cast<ABossEnemyAIController>(GetController());
	if (BossAI)
	{
		BossAI->HandleStealthPhaseTransition(1);
	}
}

void ABossEnemy::OnStealthStartMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (bInterrupted) return;
    StartStealthDivePhase();
}

void ABossEnemy::StartStealthDivePhase()
{
	if (!StealthDiveMontage) return;

	CurrentStealthPhase = 2;
	bIsStealthStarting = false;
	bIsStealthDiving = true;
	bIsInvincible = true; // ���� ����

	PlayAnimMontage(StealthDiveMontage); // �پ��� ��Ÿ�� ���

	// 100% �Ϸ� �� ������ġ ��������Ʈ (���� ��Ȳ ���)
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ABossEnemy::OnStealthDiveMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, StealthDiveMontage);

	// **Ư�� �������� ���� �ܰ�� �Ѿ�� Ÿ�̸�**
	float MontageLength = StealthDiveMontage->GetPlayLength(); // ��Ÿ�� ��ü ���� ��������
	float TransitionTiming = MontageLength * 0.8f; // ���� ���

	GetWorld()->GetTimerManager().SetTimer(
		StealthDiveTransitionTimer, // Ŭ���� ��� Ÿ�̸� �ڵ� ���
		this, // ȣ���� ��ü
		&ABossEnemy::StartStealthInvisiblePhase, // 90% �������� ȣ���� �Լ�
		TransitionTiming, // 90% Ÿ�ֿ̹� ����
		false // �ݺ� ����
	);

	UE_LOG(LogTemp, Warning, TEXT("Stealth Phase 2: Dive Animation - will transition at 90%% (%.2f seconds)"), TransitionTiming);
}



void ABossEnemy::OnStealthDiveMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted) return;
	StartStealthInvisiblePhase();
}

void ABossEnemy::StartStealthInvisiblePhase()
{
	CurrentStealthPhase = 3;
	bIsStealthDiving = false;
	bIsStealthInvisible = true;
	bIsInvincible = true;

	// ���� ���� ó��
	SetActorHiddenInGame(true);      // ������ ����
	// �Ǵ� ��Ƽ���� ������ 0���� ����

	// �ڷ���Ʈ ��ġ ���
	CalculatedTeleportLocation = CalculateRandomTeleportLocation();

	// 5�� ��� Ÿ�̸� ����
	GetWorld()->GetTimerManager().SetTimer(
		StealthWaitTimer,
		this,
		&ABossEnemy::ExecuteStealthKick,
		5.0f,
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("Stealth Phase 3: Invisible - Waiting 5 seconds"));

	ABossEnemyAIController* BossAI = Cast<ABossEnemyAIController>(GetController());
	if (BossAI)
	{
		BossAI->HandleStealthPhaseTransition(3);
	}
}

FVector ABossEnemy::CalculateRandomTeleportLocation()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return GetActorLocation();

	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector PlayerForward = PlayerPawn->GetActorForwardVector();
	FVector PlayerRight = PlayerPawn->GetActorRightVector();

	// ���� ���� ���� (����, �Ĺ�, ����, ����)
	TArray<FVector> Directions;
	Directions.Add(PlayerForward);          // ����
	Directions.Add(-PlayerForward);         // �Ĺ�
	Directions.Add(PlayerRight);            // ����
	Directions.Add(-PlayerRight);           // ����

	int32 RandomIndex = FMath::RandRange(0, Directions.Num() - 1);
	FVector ChosenDirection = Directions[RandomIndex];

	// �ھ� �Ÿ� (100 ����)
	float DistanceFromPlayer = 100.0f;
	FVector TeleportLocation = PlayerLocation + (ChosenDirection * DistanceFromPlayer);

	return AdjustHeightForCharacter(TeleportLocation);
}

void ABossEnemy::ExecuteStealthKick()
{
	CurrentStealthPhase = 5;
	bIsStealthInvisible = false;
	bIsStealthKicking = true;
	bIsInvincible = true;

	// ��� ���� ����
	SetActorHiddenInGame(false);

	// ���� ��ġ�� �ڷ���Ʈ
	SetActorLocation(CalculatedTeleportLocation);

	// �÷��̾� �������� ȸ��
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (PlayerPawn)
	{
		FVector Direction = PlayerPawn->GetActorLocation() - GetActorLocation();
		Direction.Z = 0;
		SetActorRotation(Direction.Rotation());
	}

	// ű ��Ÿ�� ���
	if (StealthKickMontage)
	{
		PlayAnimMontage(StealthKickMontage);

		// ű Ÿ�ֿ̹� ���� ����ĳ��Ʈ (��Ÿ�� 50% �������� ����)
		float KickTiming = StealthKickMontage->GetPlayLength() * 0.5f;

		FTimerHandle KickRaycastTimer;
		GetWorld()->GetTimerManager().SetTimer(
			KickRaycastTimer,
			this,
			&ABossEnemy::ExecuteStealthKickRaycast,
			KickTiming,
			false
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("Stealth Phase 5: Kick Attack"));

	ABossEnemyAIController* BossAI = Cast<ABossEnemyAIController>(GetController());
	if (BossAI)
	{
		BossAI->HandleStealthPhaseTransition(5);
	}
}

void ABossEnemy::ExecuteStealthKickRaycast()
{
	FVector StartLocation = GetActorLocation();
	FVector EndLocation = StartLocation + (GetActorForwardVector() * 200.0f); // ű ��Ÿ�

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Pawn,
		CollisionParams
	);

	if (bHit && HitResult.GetActor())
	{
		APawn* HitPawn = Cast<APawn>(HitResult.GetActor());
		if (HitPawn && HitPawn->IsPlayerControlled())
		{
			// �÷��̾�� ű ������ (20)
			UGameplayStatics::ApplyPointDamage(
				HitPawn, 20.0f, StartLocation, HitResult, nullptr, this, nullptr
			);

			// �÷��̾ 200��ŭ ���� �߻�
			LaunchPlayerIntoAir(HitPawn, 200.0f);

			// 5�� �� �ǴϽ� ���� ����
			GetWorld()->GetTimerManager().SetTimer(
				PlayerAirborneTimer,
				this,
				&ABossEnemy::ExecuteStealthFinish,
				0.1f, // ��� �ǴϽ� ����
				false
			);

			UE_LOG(LogTemp, Warning, TEXT("Stealth Kick Hit! Launching player"));
		}
	}
	else
	{
		// ������ - ���ڽ� ���� ����
		EndStealthAttack();
		UE_LOG(LogTemp, Warning, TEXT("Stealth Kick Missed - Attack End"));
	}
}

void ABossEnemy::LaunchPlayerIntoAir(APawn* PlayerPawn, float LaunchHeight)
{
	if (!PlayerPawn) return;

	ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn);
	if (PlayerCharacter)
	{
		// ���� �߻�
		FVector LaunchVelocity(0, 0, LaunchHeight);
		PlayerCharacter->LaunchCharacter(LaunchVelocity, false, true);

		// 5�� ���� ���߿� �ӹ��� �ϱ� (�߷� ��ȿȭ)
		UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement();
		if (Movement)
		{
			Movement->GravityScale = 0.0f; // �߷� ����

			// 5�� �� �߷� ����
			FTimerHandle GravityRestoreTimer;
			GetWorld()->GetTimerManager().SetTimer(
				GravityRestoreTimer,
				[Movement]()
				{
					if (Movement)
					{
						Movement->GravityScale = 1.0f; // �߷� ����
					}
				},
				5.0f,
				false
			);
		}
	}
}

void ABossEnemy::ExecuteStealthFinish()
{
	CurrentStealthPhase = 6;
	bIsStealthKicking = false;
	bIsStealthFinishing = true;
	bIsInvincible = true;

	if (StealthFinishMontage)
	{
		PlayAnimMontage(StealthFinishMontage);

		// ���� �߻� Ÿ�̹� (��Ÿ�� 70% ����)
		float CannonTiming = StealthFinishMontage->GetPlayLength() * 0.7f;

		FTimerHandle CannonTimer;
		GetWorld()->GetTimerManager().SetTimer(
			CannonTimer,
			this,
			&ABossEnemy::ExecuteStealthFinishRaycast,
			CannonTiming,
			false
		);

		// ��Ÿ�� ���� �� ���ڽ� ���� ���� ����
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &ABossEnemy::OnStealthFinishMontageEnded);
		GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, StealthFinishMontage);
	}

	UE_LOG(LogTemp, Warning, TEXT("Stealth Phase 6: Finish Cannon Attack"));
}

void ABossEnemy::ExecuteStealthFinishRaycast()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;

	FVector StartLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector Direction = (PlayerLocation - StartLocation).GetSafeNormal();
	FVector EndLocation = StartLocation + (Direction * 1000.0f); // ���� ��Ÿ�

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Pawn,
		CollisionParams
	);

	if (bHit && HitResult.GetActor() == PlayerPawn)
	{
		// ���� ������ ���� (30 ������)
		UGameplayStatics::ApplyPointDamage(
			PlayerPawn, 30.0f, StartLocation, HitResult, nullptr, this, nullptr
		);

		UE_LOG(LogTemp, Warning, TEXT("Stealth Cannon Hit! Dealing 30 damage"));
	}
}

void ABossEnemy::OnStealthFinishMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndStealthAttack();
}

void ABossEnemy::EndStealthAttack()
{
	// ��� ���� �ʱ�ȭ
	CurrentStealthPhase = 0;
	bIsStealthStarting = false;
	bIsStealthDiving = false;
	bIsStealthInvisible = false;
	bIsStealthKicking = false;
	bIsStealthFinishing = false;

	bIsInvincible = false;

	SetActorHiddenInGame(false);

	// ��Ÿ�� ����
	bCanUseStealthAttack = false;
	GetWorld()->GetTimerManager().SetTimer(
		StealthCooldownTimer,
		this,
		&ABossEnemy::OnStealthCooldownEnd,
		StealthCooldown,
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("Stealth Attack Completed - Cooldown Started"));

	ABossEnemyAIController* BossAI = Cast<ABossEnemyAIController>(GetController());
	if (BossAI)
	{
		BossAI->HandleStealthPhaseTransition(0);
	}
}

void ABossEnemy::OnStealthCooldownEnd()
{
	bCanUseStealthAttack = true;
	UE_LOG(LogTemp, Warning, TEXT("Stealth Attack Ready"));
}


float ABossEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsBossDead) return 0.0f; // �̹� ����ߴٸ� ������ ����
	float DamageApplied = FMath::Min(BossHealth, DamageAmount); // ������ ü�°� �������� �ҷ���
	BossHealth -= DamageApplied; // ü�¿��� ��������ŭ ����
	UE_LOG(LogTemp, Warning, TEXT("Boss took %f damage, Health remaining: %f"), DamageAmount, BossHealth);

	// ���� ���� üũ �߰�
	if (bIsInvincible)
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss is invulnerable - damage ignored"));
		return 0.0f;
	}
	if (bIsPlayingBossIntro)
	{
		return 0.0f; // ���� �߿��� ������ ��ȿȭ
	}

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
