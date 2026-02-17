#include "BossEnemy.h"
#include "BossEnemyAnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "BossEnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MainGameModeBase.h"
#include "BossProjectile.h"
#include "EnemyBossKatana.h"
#include "MainCharacter.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"

ABossEnemy::ABossEnemy()
{
	PrimaryActorTick.bCanEverTick = true; // Tick 활성화
	AICon = nullptr;
	AnimInstance = nullptr;
	MoveComp = GetCharacterMovement();
	EquippedBossKatana = nullptr;

	ApplyBaseWalkSpeed(); // 기본 이동 속도 적용 함수 호출

	AIControllerClass = ABossEnemyAIController::StaticClass(); // AI 컨트롤러 클래스 설정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; // AI 자동 소유 설정
}

void ABossEnemy::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	if (!World) return;

	SetCanBeDamaged(true); // 데미지 허용
	BossHealth = MaxBossHealth; // 보스 체력 초기화

	AICon = Cast<ABossEnemyAIController>(GetController()); // AI 컨트롤러 가져옴
	if (!AICon) // AI 컨트롤러가 없다면
	{
		SpawnDefaultController(); // AI 컨트롤러 스폰
		AICon = Cast<ABossEnemyAIController>(GetController());  // AI 컨트롤러 다시 가져옴
	}

	if (GetMesh())
	{
		AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	}

	MoveComp = GetCharacterMovement();

	SetUpBossAI(); // AI 설정 함수 호출

	// 카타나 스폰 및 장착
	if (IsValid(BossKatanaClass))
	{
		EquippedBossKatana = World->SpawnActor<AEnemyBossKatana>(BossKatanaClass);
		if (EquippedBossKatana)
		{
			EquippedBossKatana->SetOwner(this);
			USkeletalMeshComponent* MeshComp = GetMesh();
			if (MeshComp)
			{
				FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
				EquippedBossKatana->AttachToComponent(
					MeshComp,
					AttachmentRules,
					FName("BossEnemyKatanaSocket")
				);
			}
		}
	}

	PlayBossSpawnIntroAnimation();
}

// ========== 헬퍼 함수 구현 ==========

//UBossEnemyAnimInstance* ABossEnemy::GetBossAnimInstance() const
//{
//	USkeletalMeshComponent* MeshComp = GetMesh();
//	if (!IsValid(MeshComp)) return nullptr;
//	return Cast<UBossEnemyAnimInstance>(MeshComp->GetAnimInstance());
//}

//ABossEnemyAIController* ABossEnemy::GetBossAIController() const
//{
//	return Cast<ABossEnemyAIController>(GetController());
//}

//APawn* ABossEnemy::GetPlayerPawn() const
//{
//	UWorld* World = GetWorld();
//	if (!IsValid(World)) return nullptr;
//	return UGameplayStatics::GetPlayerPawn(World, 0);
//}

void ABossEnemy::SetState(EBossState NewState)
{
	if (CurrentState == NewState) return;
	CurrentState = NewState;
}

void ABossEnemy::SetStealthPhase(EStealthPhase NewPhase)
{
	CurrentStealthPhase = NewPhase;

	// AI 컨트롤러에 스텔스 단계 변경 알림
	if (AICon)
	{
		AICon->HandleStealthPhaseTransition(static_cast<int32>(NewPhase));
	}
}

void ABossEnemy::SetSafeTimer(FTimerHandle& Handle, float Time, TFunction<void()> Callback, bool bLoop)
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	TWeakObjectPtr<ABossEnemy> WeakThis(this);
	World->GetTimerManager().SetTimer(
		Handle,
		FTimerDelegate::CreateLambda([WeakThis, Callback]()
			{
				if (WeakThis.IsValid())
				{
					Callback();
				}
			}),
		Time,
		bLoop
	);
}

void ABossEnemy::SetSafeTimerForNextTick(TFunction<void()> Callback)
{
	UWorld* World = GetWorld();
	if (!World) return;

	TWeakObjectPtr<ABossEnemy> WeakThis(this);
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateLambda([WeakThis, Callback]()
			{
				if (WeakThis.IsValid())
				{
					Callback();
				}
			})
	);
}

bool ABossEnemy::PlayRandomMontage(
	const TArray<UAnimMontage*>& Montages,
	void(ABossEnemy::* EndCallback)(UAnimMontage*, bool),
	bool bUseUpperBodyBlend,
	bool bLookAtPlayer,
	float LookAtSpeed)
{
	if (Montages.Num() == 0) return false;

	//UBossEnemyAnimInstance* AnimInstance = GetBossAnimInstance();
	if (AnimInstance)
	{
		// 다른 몽타주 정지
		AnimInstance->bUseUpperBodyBlend = bUseUpperBodyBlend;
		AnimInstance->Montage_Stop(0.1f, nullptr);
	}

	// 플레이어 바라보기 설정
	if (bLookAtPlayer)
	{
		AnimInstance->bShouldLookAtPlayer = true;
		AnimInstance->LookAtSpeed = LookAtSpeed;
	}

	// 랜덤 몽타주 선택 및 재생
	int32 RandomIndex = FMath::RandRange(0, Montages.Num() - 1);
	UAnimMontage* SelectedMontage = Montages[RandomIndex];

	if (!SelectedMontage) return false;

	float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);
	if (PlayResult <= 0.0f)
	{
		// 재생 실패 시 상태 복원
		if (bLookAtPlayer)
		{
			AnimInstance->bShouldLookAtPlayer = false;
		}
		return false;
	}

	// 종료 델리게이트 바인딩
	if (EndCallback)
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, EndCallback);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	return true;
}

bool ABossEnemy::PlaySingleMontage(
	UAnimMontage* Montage,
	void(ABossEnemy::* EndCallback)(UAnimMontage*, bool),
	bool bUseUpperBodyBlend,
	bool bLookAtPlayer,
	float LookAtSpeed)
{
	if (!Montage) return false;

	//UBossEnemyAnimInstance* AnimInstance = GetBossAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = bUseUpperBodyBlend;
		AnimInstance->Montage_Stop(0.1f, nullptr);
	}

	if (bLookAtPlayer)
	{
		AnimInstance->bShouldLookAtPlayer = true;
		AnimInstance->LookAtSpeed = LookAtSpeed;
	}

	float PlayResult = AnimInstance->Montage_Play(Montage, 1.0f);
	if (PlayResult <= 0.0f)
	{
		if (bLookAtPlayer)
		{
			AnimInstance->bShouldLookAtPlayer = false;
		}
		return false;
	}

	if (EndCallback)
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, EndCallback);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	}

	return true;
}

bool ABossEnemy::PlayTeleportMontage(
	const TArray<UAnimMontage*>& Montages,
	float TeleportTimingRatio,
	FTimerHandle& ExecutionTimer,
	void(ABossEnemy::* EndCallback)(UAnimMontage*, bool),
	TFunction<void()> TeleportAction)
{
	if (Montages.Num() == 0) return false;

	//UBossEnemyAnimInstance* AnimInstance = GetBossAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false;
		AnimInstance->Montage_Stop(0.1f, nullptr);
		AnimInstance->bShouldLookAtPlayer = true;
		AnimInstance->LookAtSpeed = 8.0f;
	}

	int32 RandomIndex = FMath::RandRange(0, Montages.Num() - 1);
	UAnimMontage* SelectedMontage = Montages[RandomIndex];
	if (!SelectedMontage) return false;

	float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);
	if (PlayResult <= 0.0f) return false;

	if (EndCallback)
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, EndCallback);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	float MontageLength = SelectedMontage->GetPlayLength();
	float TeleportTiming = MontageLength * TeleportTimingRatio;

	SetSafeTimer(ExecutionTimer, TeleportTiming, TeleportAction);

	return true;
}

bool ABossEnemy::IsPerformingAction() const
{
	return CurrentState != EBossState::Idle &&
		CurrentState != EBossState::Combat &&
		CurrentState != EBossState::Dead;
}

bool ABossEnemy::IsExecutingStealthAttack() const
{
	return CurrentState == EBossState::StealthAttack &&
		CurrentStealthPhase != EStealthPhase::None;
}

void ABossEnemy::ApplyBaseWalkSpeed()
{
	if (MoveComp)
	{
		MoveComp->MaxWalkSpeed = BaseMaxWalkSpeed; // 기본 이동 속도로 설정
		MoveComp->MaxAcceleration = BaseMaxAcceleration; // 가속도를 높여 즉시 최대 속도에 도달하게 설정
		MoveComp->BrakingFrictionFactor = BaseBrakingFrictionFactor; // 제동 마찰력을 높여 즉시 멈추도록 설정
	}
}

void ABossEnemy::DisableBossMovement()
{
	if (MoveComp)
	{
		MoveComp->DisableMovement();
		MoveComp->StopMovementImmediately();
	}
}

void ABossEnemy::EnableBossMovement()
{
	if (IsDead()) return;

	if (MoveComp)
	{
		MoveComp->SetMovementMode(EMovementMode::MOVE_NavWalking);
	}
}

void ABossEnemy::StopAIMovementAndDisable()
{
	if (AICon)
	{
		AICon->StopMovement();
	}
	DisableBossMovement();
}

void ABossEnemy::ResetUpperBodyBlend()
{
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false;
	}
}

void ABossEnemy::ResetLookAt(float LookAtSpeed) 
{
	if(AnimInstance)
	{
		AnimInstance->bShouldLookAtPlayer = false;
		AnimInstance->LookAtSpeed = LookAtSpeed;
	}
}

void ABossEnemy::SetUpBossAI()
{
	if (MoveComp)
	{
		MoveComp->SetMovementMode(EMovementMode::MOVE_NavWalking);
	}
}

void ABossEnemy::PlayBossSpawnIntroAnimation()
{
	if (BossSpawnIntroMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No boss intro montages found - skipping intro animation"));
		SetState(EBossState::Combat);
		bCanBossAttack = true;
		return;
	}

	SetState(EBossState::Intro);
	bCanBossAttack = false;
	bIsInvincible = true;

	StopAIMovementAndDisable();

	if (!PlayRandomMontage(BossSpawnIntroMontages, &ABossEnemy::OnBossIntroMontageEnded, false, false))
	{
		UE_LOG(LogTemp, Error, TEXT("Boss intro montage failed to play"));
		SetState(EBossState::Combat);
		bCanBossAttack = true;
		bIsInvincible = false;
		EnableBossMovement();
	}
}

void ABossEnemy::OnBossIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ResetUpperBodyBlend();

	SetState(EBossState::Combat);
	bIsInvincible = false;
	EnableBossMovement();

	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true;
}

void ABossEnemy::PlayBossNormalAttackAnimation()
{
	if (IsDead()) return;

	if (!AnimInstance || BossNormalAttackMontages.Num() == 0) return;
	if (AnimInstance->IsAnyMontagePlaying()) return;

	SetState(EBossState::NormalAttack);
	bCanBossAttack = false;

	StopAIMovementAndDisable();

	if (!PlayRandomMontage(BossNormalAttackMontages, &ABossEnemy::OnNormalAttackMontageEnded, false, true, 8.0f))
	{
		SetState(EBossState::Combat);
		bCanBossAttack = true;
		EnableBossMovement();
		return;
	}

	if (BossNormalAttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossNormalAttackSound, GetActorLocation());
	}
}

void ABossEnemy::OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (IsDead()) return;
	ResetUpperBodyBlend();
	ResetLookAt(5.0f);

	SetState(EBossState::Combat);
	EnableBossMovement();

	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true;
}

// ========== 상체 공격 ==========
void ABossEnemy::PlayBossUpperBodyAttackAnimation()
{
	if (IsDead()) return;
	if (!AnimInstance || BossUpperBodyMontages.Num() == 0) return;

	SetState(EBossState::UpperBodyAttack);
	bCanBossAttack = false;

	if (!PlayRandomMontage(BossUpperBodyMontages, &ABossEnemy::OnUpperBodyAttackMontageEnded, true, true, 6.0f))
	{
		SetState(EBossState::Combat);
		bCanBossAttack = true;
	}
}

void ABossEnemy::OnUpperBodyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (IsDead()) return;
	ResetUpperBodyBlend();
	ResetLookAt(5.0f);

	SetState(EBossState::Combat);

	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true;
}

// ========== 후퇴 텔레포트 ==========

void ABossEnemy::PlayBossTeleportAnimation()
{
	if (!bCanTeleport || IsDead()) return;
	if (CurrentState == EBossState::Teleporting) return;
	UWorld* World = GetWorld();
	if (!World) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(PlayerPawn)) return;

	if (BossTeleportMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No teleport montages available - teleport cancelled"));
		return;
	}

	SetState(EBossState::Teleporting);
	bCanBossAttack = false;
	bIsInvincible = true;

	StopAIMovementAndDisable();

	if (!PlayTeleportMontage(
		BossTeleportMontages,
		0.5f,
		TeleportExecutionTimer,
		&ABossEnemy::OnTeleportMontageEnded,
		[this]()
		{
			ExecuteTeleport();
		}))
	{
		UE_LOG(LogTemp, Error, TEXT("Teleport montage failed to play"));
		SetState(EBossState::Combat);
		bCanBossAttack = true;
		bIsInvincible = false;
		EnableBossMovement();
	}
}

FVector ABossEnemy::CalculateTeleportLocation()
{
	UWorld* World = GetWorld();
	if (!World) return GetActorLocation();

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(PlayerPawn)) return GetActorLocation();

	FVector BossLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector DirectionAwayFromPlayer = (BossLocation - PlayerLocation).GetSafeNormal();
	FVector TeleportLocation = BossLocation + (DirectionAwayFromPlayer * TeleportDistance);

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World);
	if (NavSystem)
	{
		FNavLocation ValidLocation;
		if (NavSystem->ProjectPointToNavigation(TeleportLocation, ValidLocation, FVector(200.0f, 200.0f, 500.0f)))
		{
			return AdjustHeightForCharacter(ValidLocation.Location);
		}
		else
		{
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
					return AdjustHeightForCharacter(ValidLocation.Location);
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("Could not find valid teleport location - staying in place"));
			return BossLocation;
		}
	}

	return AdjustHeightForCharacter(TeleportLocation);
}

FVector ABossEnemy::AdjustHeightForCharacter(const FVector& TargetLocation)
{
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	if (!CapsuleComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("No capsule component found - using default height adjustment"));
		return TargetLocation + FVector(0, 0, 90.0f);
	}

	float CapsuleHalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();

	UWorld* World = GetWorld();
	if (!World) return TargetLocation + FVector(0, 0, CapsuleHalfHeight + 5.0f);
	FVector StartLocation = TargetLocation + FVector(0, 0, 500.0f);
	FVector EndLocation = TargetLocation + FVector(0, 0, -500.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_WorldStatic,
		QueryParams
	);

	if (bHit)
	{
		FVector AdjustedLocation = HitResult.Location + FVector(0, 0, CapsuleHalfHeight + 5.0f);
		return AdjustedLocation;
	}
	else
	{
		return TargetLocation + FVector(0, 0, CapsuleHalfHeight + 5.0f);
	}
}

void ABossEnemy::ExecuteTeleport()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector TeleportLocation = CalculateTeleportLocation();
	SetActorLocation(TeleportLocation);
}

void ABossEnemy::OnTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UWorld* World = GetWorld();
	if (!World) return;
	ResetUpperBodyBlend();

	bIsInvincible = false;

	// 텔레포트 후 잠시 대기
	SetSafeTimer(PostTeleportPauseTimer, PostTeleportPauseTime, [this]()
		{
			OnPostTeleportPauseEnd();
		});

	// 텔레포트 쿨다운 시작
	bCanTeleport = false;
	SetSafeTimer(TeleportCooldownTimer, TeleportCooldown, [this]()
		{
			OnTeleportCooldownEnd();
		});
}

void ABossEnemy::OnTeleportCooldownEnd()
{
	bCanTeleport = true;
}

void ABossEnemy::OnPostTeleportPauseEnd()
{
	if (IsDead()) return;

	bool bActionStarted = false;
	if (AICon)
	{
		bActionStarted = AICon->HandlePostTeleportPause();
	}

	if (bActionStarted)
	{
		return;
	}

	SetState(EBossState::Combat);
	bIsInvincible = false;
	EnableBossMovement();
	bCanBossAttack = true;
}

// ========== 공격 텔레포트 ==========

void ABossEnemy::PlayBossAttackTeleportAnimation()
{
	if (IsDead()) return;
	if (CurrentState == EBossState::AttackTeleport) return;
	UWorld* World = GetWorld();
	if (!World) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(PlayerPawn)) return;

	if (BossAttackTeleportMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No attack teleport montages available - teleport cancelled"));
		return;
	}

	SetState(EBossState::AttackTeleport);
	bCanBossAttack = false;
	bIsInvincible = true;

	StopAIMovementAndDisable();

	if (!PlayTeleportMontage(
		BossAttackTeleportMontages,
		0.4f,
		AttackTeleportExecutionTimer,
		&ABossEnemy::OnAttackTeleportMontageEnded,
		[this]()
		{
			ExecuteAttackTeleport();
		}))
	{
		UE_LOG(LogTemp, Error, TEXT("Attack teleport montage failed to play"));
		SetState(EBossState::Combat);
		bCanBossAttack = true;
		bIsInvincible = false;
		EnableBossMovement();
	}
}

FVector ABossEnemy::CalculateAttackTeleportLocation()
{
	UWorld* World = GetWorld();
	if (!World) return GetActorLocation();

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(PlayerPawn)) return GetActorLocation();

	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector BossLocation = GetActorLocation();
	FVector PlayerForward = PlayerPawn->GetActorForwardVector();
	FVector BehindPlayerDirection = -PlayerForward;

	float RandomOffset = FMath::RandRange(-45.0f, 45.0f);
	FVector RotatedDirection = BehindPlayerDirection.RotateAngleAxis(RandomOffset, FVector(0, 0, 1));
	FVector TeleportLocation = PlayerLocation + (RotatedDirection * AttackTeleportRange);

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World);
	if (NavSystem)
	{
		FNavLocation ValidLocation;
		if (NavSystem->ProjectPointToNavigation(TeleportLocation, ValidLocation, FVector(200.0f, 200.0f, 500.0f)))
		{
			return AdjustHeightForCharacter(ValidLocation.Location);
		}

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
	UWorld* World = GetWorld();
	if (!World) return;

	if (!IsValid(this)) return;

	FVector TeleportLocation = CalculateAttackTeleportLocation();
	SetActorLocation(TeleportLocation);
}

void ABossEnemy::OnAttackTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ResetUpperBodyBlend();

	SetState(EBossState::Combat);
	bIsInvincible = false;
	EnableBossMovement();

	if (AICon)
	{
		AICon->OnBossAttackTeleportEnded();
	}

	bCanBossAttack = true;
}

// ========== 원거리 공격 ==========
void ABossEnemy::PlayBossRangedAttackAnimation()
{
	if (IsDead()) return;
	if (CurrentState == EBossState::RangedAttack) return;

	if (BossRangedAttackMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No ranged attack montages available - attack cancelled"));
		return;
	}

	SetState(EBossState::RangedAttack);
	bCanBossAttack = false;

	StopAIMovementAndDisable();

	if (!PlayRandomMontage(BossRangedAttackMontages, &ABossEnemy::OnRangedAttackMontageEnded, false, true, 8.0f))
	{
		UE_LOG(LogTemp, Error, TEXT("Ranged attack montage failed to play"));
		SetState(EBossState::Combat);
		bCanBossAttack = true;
		EnableBossMovement();
		return;
	}
}

void ABossEnemy::SpawnBossProjectile()
{
	if (!BossProjectileClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(PlayerPawn)) return;

	const FVector SpawnLocation = GetActorLocation()
		+ GetActorForwardVector() * MuzzleOffset.X
		+ GetActorRightVector() * MuzzleOffset.Y
		+ FVector(0, 0, MuzzleOffset.Z);

	const FVector TargetLocation = PlayerPawn->GetActorLocation();
	const FVector ShootDirection = (TargetLocation - SpawnLocation).GetSafeNormal();

	FTransform SpawnTM(ShootDirection.Rotation(), SpawnLocation);

	ABossProjectile* Projectile = World->SpawnActorDeferred<ABossProjectile>(
		BossProjectileClass,
		SpawnTM,
		this,
		this,
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
	ResetUpperBodyBlend();

	SetState(EBossState::Combat);
	EnableBossMovement();

	if (AICon)
	{
		AICon->OnBossRangedAttackEnded();
	}

	bCanBossAttack = true;
}

// ========== 스텔스 공격 ==========
void ABossEnemy::PlayBossStealthAttackAnimation()
{
	if (!StealthStartMontage) return;
	if (!bCanUseStealthAttack)
	{
		UE_LOG(LogTemp, Warning, TEXT("Stealth Attack is on cooldown"));
		return;
	}
	if (IsDead()) return;

	SetState(EBossState::StealthAttack);
	SetStealthPhase(EStealthPhase::Starting);
	bIsInvincible = true;
	bCanBossAttack = false;

	PlayAnimMontage(StealthStartMontage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ABossEnemy::OnStealthStartMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, StealthStartMontage);
}

void ABossEnemy::OnStealthStartMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted) return;

	StartStealthDivePhase();
}

void ABossEnemy::StartStealthDivePhase()
{
	if (!StealthDiveMontage) return;

	SetStealthPhase(EStealthPhase::Diving);
	bIsInvincible = true;

	PlayAnimMontage(StealthDiveMontage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ABossEnemy::OnStealthDiveMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, StealthDiveMontage);

	float MontageLength = StealthDiveMontage->GetPlayLength();
	float TransitionTiming = MontageLength * 0.8f;

	SetSafeTimer(StealthDiveTransitionTimer, TransitionTiming, [this]()
		{
			StartStealthInvisiblePhase();
		});

	if (EquippedBossKatana)
	{
		EquippedBossKatana->SetActorHiddenInGame(true);
	}
}

void ABossEnemy::OnStealthDiveMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted) return;

	// 이미 StartStealthInvisiblePhase가 타이머로 호출되었을 수 있음
	if (CurrentStealthPhase != EStealthPhase::Invisible)
	{
		StartStealthInvisiblePhase();
	}
}

void ABossEnemy::StartStealthInvisiblePhase()
{
	SetStealthPhase(EStealthPhase::Invisible);
	bIsInvincible = true;

	SetActorHiddenInGame(true);

	// 실시간 위치 추적 타이머
	SetSafeTimer(StealthWaitTimer, 0.01f, [this]()
		{
			UpdateStealthTeleportLocation();
		}, true);

	// 2초 후 킥 공격 실행
	SetSafeTimer(StealthKickExecutionTimer, 2.0f, [this]()
		{
			ExecuteStealthKick();
		});
}

void ABossEnemy::UpdateStealthTeleportLocation()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (CurrentStealthPhase != EStealthPhase::Invisible)
	{
		World->GetTimerManager().ClearTimer(StealthWaitTimer);
		return;
	}

	CalculatedTeleportLocation = CalculateRandomTeleportLocation();
}

FVector ABossEnemy::CalculateRandomTeleportLocation()
{
	UWorld* World = GetWorld();
	if (!World) return GetActorLocation();
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(PlayerPawn)) return GetActorLocation();

	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector PlayerForward = PlayerPawn->GetActorForwardVector();
	FVector PlayerRight = PlayerPawn->GetActorRightVector();

	TArray<FVector> Directions;
	Directions.Add(PlayerForward);
	Directions.Add(-PlayerForward);
	Directions.Add(PlayerRight);
	Directions.Add(-PlayerRight);

	int32 RandomIndex = FMath::RandRange(0, Directions.Num() - 1);
	FVector ChosenDirection = Directions[RandomIndex];

	float DistanceFromPlayer = 100.0f;
	FVector TeleportLocation = PlayerLocation + (ChosenDirection * DistanceFromPlayer);

	return AdjustHeightForCharacter(TeleportLocation);
}

void ABossEnemy::ExecuteStealthKick()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (EquippedBossKatana)
	{
		EquippedBossKatana->SetActorHiddenInGame(false);
	}

	SetStealthPhase(EStealthPhase::Kicking);
	bIsInvincible = true;
	bHasExecutedKickRaycast = false;

	DisableBossMovement();
	bCanBossAttack = false;

	World->GetTimerManager().ClearTimer(StealthWaitTimer);
	SetActorHiddenInGame(false);
	SetActorLocation(CalculatedTeleportLocation);

	// 플레이어 방향으로 회전
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (IsValid(PlayerPawn))
	{
		FVector Direction = PlayerPawn->GetActorLocation() - GetActorLocation();
		Direction.Z = 0;
		SetActorRotation(Direction.Rotation());
	}

	if (StealthKickMontage)
	{
		float PlayResult = PlayAnimMontage(StealthKickMontage);
		if (PlayResult > 0.f)
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ABossEnemy::OnStealthKickMontageEnded);
			GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, StealthKickMontage);

			float KickTiming = StealthKickMontage->GetPlayLength() * 0.5f;
			FTimerHandle KickRaycastTimer;
			SetSafeTimer(KickRaycastTimer, KickTiming, [this]()
				{
					ExecuteStealthKickRaycast();
				});
		}
	}
}

void ABossEnemy::ExecuteStealthKickRaycast()
{
	if (bHasExecutedKickRaycast)
	{
		return;
	}
	bHasExecutedKickRaycast = true;

	UWorld* World = GetWorld();
	if (!World) return;
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(PlayerPawn)) return;

	FVector StartLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector Direction = (PlayerLocation - StartLocation).GetSafeNormal();

	float KickRange = 120.0f;
	float KickRadius = 50.0f;
	FVector EndLocation = StartLocation + (Direction * KickRange);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	FCollisionShape SphereShape = FCollisionShape::MakeSphere(KickRadius);
	bool bHit = World->SweepSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		ECollisionChannel::ECC_Pawn,
		SphereShape,
		CollisionParams
	);

	if (bHit && HitResult.GetActor() == PlayerPawn)
	{
		AMainCharacter* MainCharacter = Cast<AMainCharacter>(PlayerPawn);
		if (MainCharacter)
		{
			MainCharacter->PlayBigHitReaction();
		}

		UGameplayStatics::ApplyPointDamage(
			PlayerPawn, 20.0f, StartLocation, HitResult, nullptr, this, nullptr
		);

		LaunchPlayerIntoAir(PlayerPawn, 300.0f);

		SetSafeTimer(PlayerAirborneTimer, 0.1f, [this]()
			{
				ExecuteStealthFinish();
			});

	}
	else
	{
		EndStealthAttack();
	}
}

void ABossEnemy::LaunchPlayerIntoAir(APawn* PlayerPawn, float LaunchHeight)
{
	if (!IsValid(PlayerPawn)) return;

	ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn);
	if (PlayerCharacter)
	{
		FVector LaunchVelocity(0, 0, LaunchHeight);
		PlayerCharacter->LaunchCharacter(LaunchVelocity, false, true);

		UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement();
		if (Movement)
		{
			Movement->GravityScale = 0.0f;
			Movement->Velocity = FVector::ZeroVector;
		}
	}
}

void ABossEnemy::ExecuteStealthFinish()
{

	SetStealthPhase(EStealthPhase::Finishing);
	bIsInvincible = true;
	bCanBossAttack = false;

	StopAIMovementAndDisable();

	if (StealthFinishMontage)
	{
		PlayAnimMontage(StealthFinishMontage);

		float CannonTiming = StealthFinishMontage->GetPlayLength() * 0.7f;

		FTimerHandle CannonTimer;
		SetSafeTimer(CannonTimer, CannonTiming, [this]()
			{
				ExecuteStealthFinishRaycast();
			});

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &ABossEnemy::OnStealthFinishMontageEnded);
		GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, StealthFinishMontage);
	}

}

void ABossEnemy::ExecuteStealthFinishRaycast()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(PlayerPawn)) return;

	FVector StartLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector Direction = (PlayerLocation - StartLocation).GetSafeNormal();
	FVector EndLocation = StartLocation + (Direction * 1000.0f);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Pawn,
		CollisionParams
	);

	ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn);
	auto RestorePlayerGravity = [PlayerCharacter]()
	{
		if (!PlayerCharacter) return;
		UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement();
		if (Movement)
		{
			Movement->GravityScale = 1.0f;
		}
	};

	if (bHit && HitResult.GetActor() == PlayerPawn)
	{
		if (StealthFinishEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World, StealthFinishEffect, HitResult.Location,
				FRotator::ZeroRotator, FVector(1.0f), true
			);
		}
		if (StealthFinishSound)
		{
			UGameplayStatics::PlaySoundAtLocation(World, StealthFinishSound, HitResult.Location);
		}

		UGameplayStatics::ApplyPointDamage(
			PlayerPawn, 30.0f, StartLocation, HitResult, nullptr, this, nullptr
		);
	}

	RestorePlayerGravity();
}

void ABossEnemy::OnStealthFinishMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ResetUpperBodyBlend();
	ResetLookAt(5.0f);

	bCanBossAttack = true;
	EnableBossMovement();

	EndStealthAttack();
}

void ABossEnemy::OnStealthKickMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (IsDead()) return;

	SetStealthPhase(EStealthPhase::None);
	EnableBossMovement();
	bCanBossAttack = true;
}

void ABossEnemy::EndStealthAttack()
{
	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().ClearTimer(StealthWaitTimer);

	SetState(EBossState::Combat);
	SetStealthPhase(EStealthPhase::None);
	bIsInvincible = false;
	SetActorHiddenInGame(false);
	bCanBossAttack = true;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (IsValid(PlayerPawn))
	{
		ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn);
		if (PlayerCharacter)
		{
			UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement();
			if (Movement && Movement->GravityScale <= 0.0f)
			{
				Movement->GravityScale = 1.0f;
			}
		}
	}

	bCanUseStealthAttack = false;
	SetSafeTimer(StealthCooldownTimer, StealthCooldown, [this]()
		{
			OnStealthCooldownEnd();
		});

	if (AICon)
	{
		SetSafeTimerForNextTick([this]()
			{
				if (IsValid(this) && IsValid(AICon))
				{
					AICon->HandlePostStealthRecovery();
				}
			});
	}
}

void ABossEnemy::OnStealthCooldownEnd()
{
	bCanUseStealthAttack = true;
}

void ABossEnemy::StartAttack()
{
	if (EquippedBossKatana)
	{
		EquippedBossKatana->EnableAttackHitDetection();
	}
}

void ABossEnemy::EndAttack()
{
	if (EquippedBossKatana)
	{
		EquippedBossKatana->DisableAttackHitDetection();
	}
}

float ABossEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (IsDead()) return 0.0f;

	if (bIsInvincible || CurrentState == EBossState::Intro)
	{
		return 0.0f;
	}

	float DamageApplied = FMath::Min(BossHealth, DamageAmount);
	BossHealth -= DamageApplied;

	if (BossHitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossHitSound, GetActorLocation());
	}

	// 슈퍼아머 체크: 특정 액션 중에는 피격 모션 생략
	bool bIsUninterruptible = IsPerformingAction() && CurrentState != EBossState::Combat;

	// 피격 애니메이션 (슈퍼아머가 아닌 경우만)
	if (BossHitReactionMontages.Num() > 0 && !bIsUninterruptible)
	{
		EBossState PreviousState = CurrentState;
		SetState(EBossState::HitReaction);
		bCanBossAttack = false;

		if (AICon)
		{
			AICon->StopMovement();
		}

		if (AnimInstance)
		{
			AnimInstance->bUseUpperBodyBlend = false;
			AnimInstance->Montage_Stop(0.5f);
		}

		int32 RandomIndex = FMath::RandRange(0, BossHitReactionMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossHitReactionMontages[RandomIndex];
		if (SelectedMontage && AnimInstance)
		{
			float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

			if (PlayResult > 0.0f)
			{
				FOnMontageEnded HitEndDelegate;
				HitEndDelegate.BindUObject(this, &ABossEnemy::OnHitReactionMontageEnded);
				AnimInstance->Montage_SetEndDelegate(HitEndDelegate, SelectedMontage);
			}
			else
			{
				SetState(EBossState::Combat);
				bCanBossAttack = true;
			}
		}
	}

	if (BossHealth <= 0.0f)
	{
		BossDie();
	}

	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	return DamageApplied;
}

void ABossEnemy::OnHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (IsDead()) return;

	ResetUpperBodyBlend();

	SetState(EBossState::Combat);

	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true;
}

float ABossEnemy::GetHealthPercent_Implementation() const
{
	if (IsDead() || BossHealth <= 0.0f)
	{
		return 0.0f;
	}
	if (MaxBossHealth <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(BossHealth / MaxBossHealth, 0.0f, 1.0f);
}

bool ABossEnemy::IsEnemyDead_Implementation() const
{
	return IsDead();
}

void ABossEnemy::PlayWeaponHitSound()
{
	if (BossWeaponHitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossWeaponHitSound, GetActorLocation());
	}
}

void ABossEnemy::BossDie()
{
	if (IsDead()) return;
	SetState(EBossState::Dead);

	if (BossDieSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossDieSound, GetActorLocation());
	}

	StopBossActions();

	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false;
		AnimInstance->Montage_Stop(0.0f);
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
					float MontageLength = SelectedMontage->GetPlayLength();
					float HideTime = MontageLength * 0.9f;

					SetSafeTimer(BossDeathHideTimerHandle, HideTime, [this]()
						{
							HideBossEnemy();
						});
					return;
				}
			}
		}
	}

	HideBossEnemy();
}

void ABossEnemy::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	HideBossEnemy();
}

void ABossEnemy::StopBossActions()
{
	if (MoveComp)
	{
		MoveComp->DisableMovement();
		MoveComp->StopMovementImmediately();
		MoveComp->SetMovementMode(EMovementMode::MOVE_None);
	}

	if (AICon)
	{
		AICon->StopBossAI();
	}

	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(0.0f);
	}

	if (IsDead())
	{
		SetActorTickEnabled(false);

		UAnimInstance* BaseAnimInstance = GetMesh()->GetAnimInstance();
		if (BaseAnimInstance)
		{
			BaseAnimInstance->OnMontageEnded.RemoveAll(this);
		}
	}
}

void ABossEnemy::HideBossEnemy()
{
	if (!IsDead()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// GameMode에 파괴 알림
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(World->GetAuthGameMode()))
	{
		GameMode->OnEnemyDestroyed(this);
	}

	// 모든 타이머 정리
	World->GetTimerManager().ClearAllTimersForObject(this);

	// 애니메이션 델리게이트 해제
	UAnimInstance* BossAnimInstance = GetMesh()->GetAnimInstance();
	if (BossAnimInstance && IsValid(BossAnimInstance))
	{
		BossAnimInstance->OnMontageEnded.RemoveAll(this);
		BossAnimInstance->OnMontageBlendingOut.RemoveAll(this);
		BossAnimInstance->OnMontageStarted.RemoveAll(this);
	}

	// AI 시스템 정리
	ABossEnemyAIController* BossAICon = Cast<ABossEnemyAIController>(GetController());
	if (BossAICon && IsValid(BossAICon))
	{
		BossAICon->StopBossAI();
		BossAICon->UnPossess();
		BossAICon->Destroy();
	}

	// 무기 정리
	if (EquippedBossKatana && IsValid(EquippedBossKatana))
	{
		EquippedBossKatana->HideKatana();
		EquippedBossKatana = nullptr;
	}

	// 무브먼트 정리
	UCharacterMovementComponent* BossMoveComp = GetCharacterMovement();
	if (BossMoveComp && IsValid(BossMoveComp))
	{
		BossMoveComp->DisableMovement();
		BossMoveComp->StopMovementImmediately();
		BossMoveComp->SetMovementMode(EMovementMode::MOVE_None);
		BossMoveComp->SetComponentTickEnabled(false);
	}

	// 메시 정리
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp && IsValid(MeshComp))
	{
		MeshComp->SetVisibility(false);
		MeshComp->SetHiddenInGame(true);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
		MeshComp->SetComponentTickEnabled(false);
		MeshComp->SetAnimInstanceClass(nullptr);
		MeshComp->SetSkeletalMesh(nullptr);
	}

	// 액터 정리
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	SetCanBeDamaged(false);

	// 다음 프레임에 안전하게 액터 파괴
	TWeakObjectPtr<ABossEnemy> WeakThis(this);
	World->GetTimerManager().SetTimerForNextTick([WeakThis]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
			{
				WeakThis->Destroy();
			}
		});
}
