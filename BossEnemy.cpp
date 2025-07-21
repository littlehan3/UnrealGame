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
	PrimaryActorTick.bCanEverTick = true; // tick 활성화
	bCanBossAttack = true; // 공격 가능 여부 true
	AIControllerClass = ABossEnemyAIController::StaticClass(); // AI 컨트롤러 설정

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; //스폰 시에도 AI 컨트롤러 자동 할당

	GetMesh()->SetAnimInstanceClass(UBossEnemyAnimInstance::StaticClass()); // 애님 인스턴스 설정
	if (!AIControllerClass) // 컨트롤러가 없다면
	{
		UE_LOG(LogTemp, Error, TEXT("Boss AI Controller NULL"));
	}
}

void ABossEnemy::BeginPlay()
{
	Super::BeginPlay();
	SetCanBeDamaged(true); // 피해를 입을 수 있는지 여부 true

	// AI 컨트롤러 강제 할당
	if (!GetController())
	{
		SpawnDefaultController();
		UE_LOG(LogTemp, Warning, TEXT("Boss AI Controller manually spawned"));
	}

	AAIController* AICon = Cast<AAIController>(GetController()); // AI 컨트롤러를 불러와 캐스트
	if (AICon) // 컨트롤러가 존재한다면
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss AI Controller Assigend")); // 컨트롤러 할당됨
	}
	else // 아닌경우
	{
		UE_LOG(LogTemp, Error, TEXT("Boss AI Controller NULL")); // 컨트롤러 NULL
	}
	SetUpBossAI(); // AI 설정함수 호출

	PlayBossSpawnIntroAnimation(); // 보스 등장 애니메이션 재생
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
	bCanBossAttack = false; // 등장 중에는 공격 불가
	bIsFullBodyAttacking = true; // 전신 애니메이션으로 처리

	// AI 이동 중지
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement();
	}

	// 이동 차단
	DisableBossMovement();

	// 등장 몽타주 재생
	AnimInstance->bUseUpperBodyBlend = false; // 전신 애니메이션
	AnimInstance->Montage_Stop(0.1f, nullptr); // 다른 몽타주 중지

	int32 RandomIndex = FMath::RandRange(0, BossSpawnIntroMontages.Num() - 1);
	UAnimMontage* SelectedMontage = BossSpawnIntroMontages[RandomIndex];

	if (SelectedMontage)
	{
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

		if (PlayResult > 0.0f)
		{
			// 등장 몽타주 종료 델리게이트 바인딩
			FOnMontageEnded IntroEndDelegate;
			IntroEndDelegate.BindUObject(this, &ABossEnemy::OnBossIntroMontageEnded);
			AnimInstance->Montage_SetEndDelegate(IntroEndDelegate, SelectedMontage);

			UE_LOG(LogTemp, Warning, TEXT("Boss intro montage playing: %s"), *SelectedMontage->GetName());
		}
		else
		{
			// 몽타주 재생 실패 시 즉시 활성화
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

	// 애니메이션 상태 복원
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false;
	}

	// 전신공격 종료 및 이동 허용
	bIsFullBodyAttacking = false;
	bIsPlayingBossIntro = false;
	EnableBossMovement();

	// AI 컨트롤러에 등장 종료 알림
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
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking); // 캐릭터 무브먼트를 가져와 무브모드에 있는 네브 워킹모드로 설정
}

void ABossEnemy::PostInitializeComponents()
{
	Super::PostInitializeComponents(); // 컴포넌트 초기화 이후 추가 작업을 위해 부모함수 호출

	GetWorld()->GetTimerManager().SetTimerForNextTick([this]() // 다음 Tick에서 AI 컨트롤러 할당 여부를 확인하는 람다 등록
		{
			AAIController* AICon = Cast<AAIController>(GetController()); // 한번더 AI컨트롤러를 캐스팅해서 받아옴
			if (AICon) // 컨트롤러가 존재한다면
			{
				UE_LOG(LogTemp, Warning, TEXT("Boss AICon Assigned Later")); // 컨트롤러 할당됨
			}
			else // 아닌경우
			{
				UE_LOG(LogTemp, Error, TEXT("Boss AICon Still NULL")); // 컨트롤러 NULL
			}
		});
}

void ABossEnemy::PlayBossNormalAttackAnimation()
{
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance()); // 최신 애님인스턴스를 캐스팅해서 받아옴
	if (!AnimInstance || BossNormalAttackMontages.Num() == 0) return; // 애님인스턴스가 없거나 일반공격 몽타주가 없다면 리턴 

	AnimInstance->bUseUpperBodyBlend = false; // 상하체 분리 여부 false
	AnimInstance->Montage_Stop(0.1f, nullptr); // 다른슬롯 몽타주 중지
	if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return; // 애님인스턴스가 있고 애님인스턴스의 몽타주가 실행중이라면 리턴

	// ABP에서 플레이어 바라보기 활성화
	AnimInstance->bShouldLookAtPlayer = true;
	AnimInstance->LookAtSpeed = 8.0f; // 공격 시에는 더 빠른 회전

	int32 RandomIndex = FMath::RandRange(0, BossNormalAttackMontages.Num() - 1); // 일반공격 몽타주배열중에 랜덤으로 선택하는 랜덤인덱스 선언
	UAnimMontage* SelectedMontage = BossNormalAttackMontages[RandomIndex]; // 랜덤인텍스에서 선택한 몽타주를 가져옴
	if (SelectedMontage) // 몽타주가 선택 되었다면
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectedMontage: %s is playing"), *SelectedMontage->GetName());
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // 플레이결과는 애님인스터의 몽타주 플레이를 1.0배속으로 재생
		if (PlayResult == 0.0f) // 플레이 결과가 0이라면
		{
			UE_LOG(LogTemp, Warning, TEXT("Montage Play Failed")); // 몽타주 재생 실패
			// 몽타주 재생 실패 시 즉시 공격 가능 상태로 복원
			AnimInstance->bShouldLookAtPlayer = false; // 실패시 바라보기 비활성화
			bCanBossAttack = true;
			bIsFullBodyAttacking = false;
		}
		else
		{
			// 몽타주 종료 델리게이트 바인딩
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ABossEnemy::OnNormalAttackMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
			UE_LOG(LogTemp, Warning, TEXT("Montage Succesfully Playing")); // 몽타주 재생중
		}

		ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController()); // 컨트롤러를 캐스트해서 가져옴
		if (AICon) // 컨트롤러가 존재할 경우
		{
			AICon->StopMovement(); // 컨트롤러의 이동중지 함수를 호출
			UE_LOG(LogTemp, Warning, TEXT("Enemy Stopped Moving to Attack"));
		}

		bCanBossAttack = false; // 공격 중이므로 공격 불가 false
		bIsFullBodyAttacking = true; 
		DisableBossMovement();
	}
	if (BossNormalAttackSound) // 사운드가 있다면
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossNormalAttackSound, GetActorLocation()); // 해당 액터의 위치에서 소리 재생
	}
}

void ABossEnemy::OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Normal Attack Montage Ended"));

	// 공격 종료시
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // 기본값으로 복원
		// 플레이어 바라보기 비활성화
		AnimInstance->bShouldLookAtPlayer = false;
		AnimInstance->LookAtSpeed = 5.0f; // 기본 속도로 복원
	}

	// 전신공격 종료 및 이동 허용
	bIsFullBodyAttacking = false;
	EnableBossMovement(); // 이동 허용

	// AI 컨트롤러에 공격 종료 알림
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true; // 공격 가능 상태로 복원
}

// 이동 차단 함수
void ABossEnemy::DisableBossMovement()
{
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->DisableMovement(); // 이동 완전 차단
		MovementComp->StopMovementImmediately(); // 현재 이동 즉시 중지
	}

	UE_LOG(LogTemp, Warning, TEXT("Boss Movement Disabled"));
}

// 이동 허용 함수
void ABossEnemy::EnableBossMovement()
{
	if (bIsBossDead) return; // 사망한 경우 이동 허용하지 않음

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->SetMovementMode(EMovementMode::MOVE_NavWalking); // 네비게이션 워킹 모드로 복원
	}

	UE_LOG(LogTemp, Warning, TEXT("Boss Movement Enabled"));
}

void ABossEnemy::PlayBossUpperBodyAttackAnimation()
{
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance || BossUpperBodyMontages.Num() == 0) return;

	// 플레이어 바라보기 활성화
	AnimInstance->bShouldLookAtPlayer = true;
	AnimInstance->LookAtSpeed = 6.0f; // 상체 공격시 속도
	AnimInstance->bUseUpperBodyBlend = true; // 상하체분리 여부 true

	int32 RandomIndex = FMath::RandRange(0, BossUpperBodyMontages.Num() - 1);
	UAnimMontage* SelectedMontage = BossUpperBodyMontages[RandomIndex];

	if (SelectedMontage)
	{
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

		if (PlayResult > 0.0f)
		{
			// 상체 공격 종료 델리게이트 바인딩
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ABossEnemy::OnUpperBodyAttackMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);

			UE_LOG(LogTemp, Warning, TEXT("Upper Body Attack Playing"));
		}
		else
		{
			AnimInstance->bShouldLookAtPlayer = false; // 실패시 비활성화
			bCanBossAttack = true; // 재생 실패 시 즉시 복원
		}

		bCanBossAttack = false; // 공격 중이므로 공격 불가
	}
}

void ABossEnemy::OnUpperBodyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Upper Body Attack Montage Ended"));

	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // 기본값으로 복원
		// 플레이어 바라보기 비활성화
		AnimInstance->bShouldLookAtPlayer = false;
		AnimInstance->LookAtSpeed = 5.0f;
	}

	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true; // 공격 가능 상태로 복원
}

void ABossEnemy::PlayBossTeleportAnimation()
{
	if (!bCanTeleport || bIsBossTeleporting || bIsBossDead) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;

	// 텔레포트 몽타주가 없다면 텔레포트 취소
	if (BossTeleportMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No teleport montages available - teleport cancelled"));
		return;
	}

	bIsBossTeleporting = true;
	bCanBossAttack = false;
	bIsFullBodyAttacking = true; // 전신 애니메이션으로 처리
	bIsInvincible = true; // 무적 상태 활성화

	// AI 이동 중지
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement();
	}

	// 이동 차단
	DisableBossMovement();

	// 텔레포트 몽타주 재생
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // 전신 애니메이션
		AnimInstance->Montage_Stop(0.1f, nullptr); // 다른 몽타주 중지
		// ABP에서 플레이어 바라보기 활성화
		AnimInstance->bShouldLookAtPlayer = true;
		AnimInstance->LookAtSpeed = 8.0f; // 공격 시에는 더 빠른 회전

		int32 RandomIndex = FMath::RandRange(0, BossTeleportMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossTeleportMontages[RandomIndex];

		if (SelectedMontage)
		{
			float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

			if (PlayResult > 0.0f)
			{
				// 텔레포트 몽타주 종료 델리게이트 바인딩
				FOnMontageEnded TeleportEndDelegate;
				TeleportEndDelegate.BindUObject(this, &ABossEnemy::OnTeleportMontageEnded);
				AnimInstance->Montage_SetEndDelegate(TeleportEndDelegate, SelectedMontage);

				// 몽타주 중간 타이밍(50%)에서 실제 텔레포트 실행
				float MontageLength = SelectedMontage->GetPlayLength();
				float TeleportTiming = MontageLength * 0.5f; // 50% 지점에서 텔레포트

				GetWorld()->GetTimerManager().SetTimer(
					TeleportExecutionTimer, // 텔레포트 이동 타이머
					this,
					&ABossEnemy::ExecuteTeleport,
					TeleportTiming,
					false
				);

				UE_LOG(LogTemp, Warning, TEXT("Teleport montage playing - will teleport at 50%% (%.2f seconds)"), TeleportTiming);
			}
			else
			{
				// 몽타주 재생 실패시 텔레포트 취소
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
			// 선택된 몽타주가 없으면 텔레포트 취소
			UE_LOG(LogTemp, Error, TEXT("Selected teleport montage is null"));
			bIsBossTeleporting = false;
			bCanBossAttack = true;
			bIsFullBodyAttacking = false;
			EnableBossMovement();
		}
	}
	else
	{
		// 애니메이션 인스턴스가 없으면 텔레포트 취소
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

	// 네비게이션 시스템을 사용해 유효한 위치 찾기
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSystem)
	{
		FNavLocation ValidLocation;
		if (NavSystem->ProjectPointToNavigation(TeleportLocation, ValidLocation, FVector(200.0f, 200.0f, 500.0f)))
		{
			// 캐릭터 높이 보정 추가
			return AdjustHeightForCharacter(ValidLocation.Location);
		}
		else
		{
			// 유효한 위치를 찾지 못한 경우 다른 방향들 시도
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
					// 캐릭터 높이 보정 추가
					return AdjustHeightForCharacter(ValidLocation.Location);
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("Could not find valid teleport location - staying in place"));
			return BossLocation; // 유효한 위치를 찾지 못한 경우 현재 위치 반환
		}
	}

	// 네비게이션 시스템이 없는 경우에도 높이 보정 적용
	return AdjustHeightForCharacter(TeleportLocation);
}

FVector ABossEnemy::AdjustHeightForCharacter(const FVector& TargetLocation)
{
	// 캐릭터의 캡슐 컴포넌트에서 반쪽 높이 가져오기
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	if (!CapsuleComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("No capsule component found - using default height adjustment"));
		return TargetLocation + FVector(0, 0, 90.0f); // 기본값으로 90 단위 위로
	}

	float CapsuleHalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();

	// 라인 트레이스로 정확한 지면 높이 찾기
	FVector StartLocation = TargetLocation + FVector(0, 0, 500.0f); // 위에서부터 시작
	FVector EndLocation = TargetLocation + FVector(0, 0, -500.0f);  // 아래까지 검사

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // 자기 자신은 무시

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_WorldStatic, // 지형과의 충돌만 검사
		QueryParams
	);

	if (bHit)
	{
		// 지면에서 캐릭터 반쪽 높이만큼 위로 배치
		FVector AdjustedLocation = HitResult.Location + FVector(0, 0, CapsuleHalfHeight + 5.0f); // 5.0f는 여유 공간

		UE_LOG(LogTemp, Warning, TEXT("Height adjusted teleport - Ground: %s, Final: %s"),
			*HitResult.Location.ToString(), *AdjustedLocation.ToString());

		return AdjustedLocation;
	}
	else
	{
		// 라인 트레이스 실패시 기본 높이 보정
		FVector AdjustedLocation = TargetLocation + FVector(0, 0, CapsuleHalfHeight + 5.0f);

		UE_LOG(LogTemp, Warning, TEXT("Line trace failed - using default height adjustment: %s"),
			*AdjustedLocation.ToString());

		return AdjustedLocation;
	}
}

void ABossEnemy::ExecuteTeleport()
{
	FVector TeleportLocation = CalculateTeleportLocation();

	// 실제 텔레포트 실행
	SetActorLocation(TeleportLocation);
	UE_LOG(LogTemp, Warning, TEXT("Boss teleported during montage to: %s"), *TeleportLocation.ToString());
}

void ABossEnemy::OnTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Retreat Teleport Montage Ended - Starting pause before next action"));

	// 애니메이션 상태 복원
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false;
	}

	bIsInvincible = false;

	// 텔레포트 후 잠시 정지 (PostTeleportPauseTime만큼)
	// 정지 중에는 아직 이동을 허용하지 않음
	GetWorld()->GetTimerManager().SetTimer(
		PostTeleportPauseTimer,
		this,
		&ABossEnemy::OnPostTeleportPauseEnd,
		PostTeleportPauseTime,
		false
	);

	// 텔레포트 쿨타임 시작
	bCanTeleport = false;
	GetWorld()->GetTimerManager().SetTimer(
		TeleportCooldownTimer,
		this,
		&ABossEnemy::OnTeleportCooldownEnd,
		TeleportCooldown,
		false
	);

	// 아직 bCanBossAttack은 false로 유지 (정지 후 선택이 끝날 때까지)
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

	// 공격용 텔레포트 몽타주가 없다면 취소
	if (BossAttackTeleportMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No attack teleport montages available - teleport cancelled"));
		return;
	}

	bIsBossAttackTeleporting = true;
	bCanBossAttack = false;
	bIsFullBodyAttacking = true; // 전신 애니메이션으로 처리
	bIsInvincible = true;

	// AI 이동 중지
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement();
	}

	// 이동 차단
	DisableBossMovement();

	// 공격용 텔레포트 몽타주 재생
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // 전신 애니메이션
		AnimInstance->Montage_Stop(0.1f, nullptr); // 다른 몽타주 중지
		// ABP에서 플레이어 바라보기 활성화
		AnimInstance->bShouldLookAtPlayer = true;
		AnimInstance->LookAtSpeed = 8.0f; // 공격 시에는 더 빠른 회전

		int32 RandomIndex = FMath::RandRange(0, BossAttackTeleportMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossAttackTeleportMontages[RandomIndex];

		if (SelectedMontage)
		{
			float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

			if (PlayResult > 0.0f)
			{
				// 공격용 텔레포트 몽타주 종료 델리게이트 바인딩
				FOnMontageEnded AttackTeleportEndDelegate;
				AttackTeleportEndDelegate.BindUObject(this, &ABossEnemy::OnAttackTeleportMontageEnded);
				AnimInstance->Montage_SetEndDelegate(AttackTeleportEndDelegate, SelectedMontage);

				// 몽타주 중간 타이밍(40%)에서 실제 텔레포트 실행 (공격용이므로 조금 빠르게)
				float MontageLength = SelectedMontage->GetPlayLength();
				float TeleportTiming = MontageLength * 0.4f; // 40% 지점에서 텔레포트

				GetWorld()->GetTimerManager().SetTimer(
					AttackTeleportExecutionTimer, // 공격 텔레포트 이동 타이머
					this,
					&ABossEnemy::ExecuteAttackTeleport,
					TeleportTiming,
					false
				);

				UE_LOG(LogTemp, Warning, TEXT("Attack teleport montage playing - will teleport at 40%% (%.2f seconds)"), TeleportTiming);
			}
			else
			{
				// 몽타주 재생 실패시 취소
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

	// 플레이어가 바라보는 방향 (Forward Vector)
	FVector PlayerForward = PlayerPawn->GetActorForwardVector();

	// 플레이어의 뒤쪽 방향 계산
	FVector BehindPlayerDirection = -PlayerForward; // 반대 방향

	// 뒤쪽 방향에 약간의 랜덤성 추가 (좌우로 약간 변화)
	float RandomOffset = FMath::RandRange(-45.0f, 45.0f); // -45도 ~ +45도
	FVector RotatedDirection = BehindPlayerDirection.RotateAngleAxis(RandomOffset, FVector(0, 0, 1));

	// 우선적으로 뒤쪽으로 텔레포트 시도
	FVector TeleportLocation = PlayerLocation + (RotatedDirection * AttackTeleportRange);

	// 네비게이션 시스템으로 유효한 위치 확인
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSystem)
	{
		FNavLocation ValidLocation;
		if (NavSystem->ProjectPointToNavigation(TeleportLocation, ValidLocation, FVector(200.0f, 200.0f, 500.0f)))
		{
			UE_LOG(LogTemp, Warning, TEXT("Attack teleport to behind player successful"));
			return AdjustHeightForCharacter(ValidLocation.Location);
		}

		// 뒤쪽이 안되면 기존 랜덤 방향들 시도
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

	// 실제 텔레포트 실행
	SetActorLocation(TeleportLocation);
	UE_LOG(LogTemp, Warning, TEXT("Boss attack teleported to: %s"), *TeleportLocation.ToString());
}

void ABossEnemy::OnAttackTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Attack Teleport Montage Ended - Resuming combat immediately"));

	// 애니메이션 상태 복원
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false;
	}

	// 공격용 텔레포트 후에는 정지 없이 즉시 전투 재개
	bIsFullBodyAttacking = false;
	bIsBossAttackTeleporting = false;
	bIsInvincible = false;
	EnableBossMovement();

	// AI 컨트롤러에 공격용 텔레포트 종료 알림
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossAttackTeleportEnded(); // 즉시 전투 재개
	}

	bCanBossAttack = true;
}

void ABossEnemy::OnPostTeleportPauseEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("Post teleport pause ended - choosing next action"));

	// 정지 후 2가지 선택지: 즉시 캐릭터에게 텔레포트 OR 원거리 공격
	bool bShouldUseAttackTeleport = FMath::RandBool(); // 50% 확률

	if (bShouldUseAttackTeleport)
	{
		UE_LOG(LogTemp, Warning, TEXT("Chose attack teleport after retreat pause"));
		// 즉시 캐릭터에게 텔레포트 (공격 애니메이션 포함)
		PlayBossAttackTeleportAnimation();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Chose ranged attack after retreat pause"));
		// 원거리 공격 시전
		PlayBossRangedAttackAnimation();
	}

	// 전신공격 종료 및 이동 허용은 각 선택된 행동이 끝날 때 처리됨
	bIsFullBodyAttacking = false;
	bIsBossTeleporting = false;
	bIsInvincible = false;
	EnableBossMovement();
}

void ABossEnemy::PlayBossRangedAttackAnimation()
{
	if (bIsBossRangedAttacking || bIsBossDead) return;

	// 원거리 공격 몽타주가 없다면 취소
	if (BossRangedAttackMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No ranged attack montages available - attack cancelled"));
		return;
	}

	bIsBossRangedAttacking = true;
	bCanBossAttack = false;
	bIsFullBodyAttacking = true; // 전신 애니메이션으로 처리

	// AI 이동 중지
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement();
	}

	// 이동 차단
	DisableBossMovement();

	// 원거리 공격 몽타주 재생
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // 전신 애니메이션
		AnimInstance->Montage_Stop(0.1f, nullptr); // 다른 몽타주 중지
		// ABP에서 플레이어 바라보기 활성화
		AnimInstance->bShouldLookAtPlayer = true;
		AnimInstance->LookAtSpeed = 8.0f; // 공격 시에는 더 빠른 회전

		int32 RandomIndex = FMath::RandRange(0, BossRangedAttackMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossRangedAttackMontages[RandomIndex];

		if (SelectedMontage)
		{
			float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

			if (PlayResult > 0.0f)
			{
				// 원거리 공격 몽타주 종료 델리게이트 바인딩
				FOnMontageEnded RangedAttackEndDelegate;
				RangedAttackEndDelegate.BindUObject(this, &ABossEnemy::OnRangedAttackMontageEnded);
				AnimInstance->Montage_SetEndDelegate(RangedAttackEndDelegate, SelectedMontage);

				UE_LOG(LogTemp, Warning, TEXT("Ranged attack montage playing (DUMMY IMPLEMENTATION)"));

				// TODO: 몽타주 중간에 투사체 발사 로직 추가 예정
				// 몽타주 일정 퍼센트 지점에서 투사체 스폰
			}
			else
			{
				// 몽타주 재생 실패시 취소
				UE_LOG(LogTemp, Error, TEXT("Ranged attack montage failed to play"));
				bIsBossRangedAttacking = false;
				bCanBossAttack = true;
				bIsFullBodyAttacking = false;
				EnableBossMovement();
			}
		}
	}
}

#include "BossProjectile.h"   // 클래스 정의 포함

void ABossEnemy::SpawnBossProjectile()
{
	if (!BossProjectileClass) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;

	// 발사 위치와 방향
	const FVector SpawnLocation = GetActorLocation()
		+ GetActorForwardVector() * MuzzleOffset.X
		+ GetActorRightVector() * MuzzleOffset.Y
		+ FVector(0, 0, MuzzleOffset.Z);

	const FVector TargetLocation = PlayerPawn->GetActorLocation();
	const FVector ShootDirection = (TargetLocation - SpawnLocation).GetSafeNormal();

	FTransform SpawnTM(ShootDirection.Rotation(), SpawnLocation);

	// 템플릿 파라미터와 인자 타입 일치
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

	// 애니메이션 상태 복원
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false;
	}

	// 전신공격 종료 및 이동 허용
	bIsFullBodyAttacking = false;
	bIsBossRangedAttacking = false;
	EnableBossMovement();

	// AI 컨트롤러에 원거리 공격 종료 알림
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
	bIsInvincible = true; // 무적
    
    PlayAnimMontage(StealthStartMontage);
    
    // 몽타주 종료 델리게이트 바인딩
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &ABossEnemy::OnStealthStartMontageEnded);
    GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, StealthStartMontage);
    
    UE_LOG(LogTemp, Warning, TEXT("Stealth Phase 1: Start Animation"));

	// AI 컨트롤러에 스텔스 시작 알림
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
	bIsInvincible = true; // 무적 유지

	PlayAnimMontage(StealthDiveMontage); // 뛰어드는 몽타주 재생

	// 100% 완료 시 안전장치 델리게이트 (예외 상황 대비)
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ABossEnemy::OnStealthDiveMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, StealthDiveMontage);

	// **특정 지점에서 다음 단계로 넘어가는 타이머**
	float MontageLength = StealthDiveMontage->GetPlayLength(); // 몽타주 전체 길이 가져오기
	float TransitionTiming = MontageLength * 0.8f; // 지점 계산

	GetWorld()->GetTimerManager().SetTimer(
		StealthDiveTransitionTimer, // 클래스 멤버 타이머 핸들 사용
		this, // 호출할 객체
		&ABossEnemy::StartStealthInvisiblePhase, // 90% 지점에서 호출할 함수
		TransitionTiming, // 90% 타이밍에 실행
		false // 반복 안함
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

	// 완전 투명 처리
	SetActorHiddenInGame(true);      // 완전히 숨김
	// 또는 머티리얼 투명도를 0으로 설정

	// 텔레포트 위치 계산
	CalculatedTeleportLocation = CalculateRandomTeleportLocation();

	// 5초 대기 타이머 시작
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

	// 랜덤 방향 선택 (전방, 후방, 좌측, 우측)
	TArray<FVector> Directions;
	Directions.Add(PlayerForward);          // 전방
	Directions.Add(-PlayerForward);         // 후방
	Directions.Add(PlayerRight);            // 우측
	Directions.Add(-PlayerRight);           // 좌측

	int32 RandomIndex = FMath::RandRange(0, Directions.Num() - 1);
	FVector ChosenDirection = Directions[RandomIndex];

	// 코앞 거리 (100 단위)
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

	// 즉시 투명 해제
	SetActorHiddenInGame(false);

	// 계산된 위치로 텔레포트
	SetActorLocation(CalculatedTeleportLocation);

	// 플레이어 방향으로 회전
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (PlayerPawn)
	{
		FVector Direction = PlayerPawn->GetActorLocation() - GetActorLocation();
		Direction.Z = 0;
		SetActorRotation(Direction.Rotation());
	}

	// 킥 몽타주 재생
	if (StealthKickMontage)
	{
		PlayAnimMontage(StealthKickMontage);

		// 킥 타이밍에 맞춰 레이캐스트 (몽타주 50% 지점에서 실행)
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
	FVector EndLocation = StartLocation + (GetActorForwardVector() * 200.0f); // 킥 사거리

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
			// 플레이어에게 킥 데미지 (20)
			UGameplayStatics::ApplyPointDamage(
				HitPawn, 20.0f, StartLocation, HitResult, nullptr, this, nullptr
			);

			// 플레이어를 200만큼 위로 발사
			LaunchPlayerIntoAir(HitPawn, 200.0f);

			// 5초 후 피니쉬 공격 실행
			GetWorld()->GetTimerManager().SetTimer(
				PlayerAirborneTimer,
				this,
				&ABossEnemy::ExecuteStealthFinish,
				0.1f, // 즉시 피니쉬 시작
				false
			);

			UE_LOG(LogTemp, Warning, TEXT("Stealth Kick Hit! Launching player"));
		}
	}
	else
	{
		// 빗나감 - 스텔스 공격 종료
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
		// 위로 발사
		FVector LaunchVelocity(0, 0, LaunchHeight);
		PlayerCharacter->LaunchCharacter(LaunchVelocity, false, true);

		// 5초 동안 공중에 머물게 하기 (중력 무효화)
		UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement();
		if (Movement)
		{
			Movement->GravityScale = 0.0f; // 중력 제거

			// 5초 후 중력 복구
			FTimerHandle GravityRestoreTimer;
			GetWorld()->GetTimerManager().SetTimer(
				GravityRestoreTimer,
				[Movement]()
				{
					if (Movement)
					{
						Movement->GravityScale = 1.0f; // 중력 복구
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

		// 대포 발사 타이밍 (몽타주 70% 지점)
		float CannonTiming = StealthFinishMontage->GetPlayLength() * 0.7f;

		FTimerHandle CannonTimer;
		GetWorld()->GetTimerManager().SetTimer(
			CannonTimer,
			this,
			&ABossEnemy::ExecuteStealthFinishRaycast,
			CannonTiming,
			false
		);

		// 몽타주 종료 시 스텔스 공격 완전 종료
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
	FVector EndLocation = StartLocation + (Direction * 1000.0f); // 대포 사거리

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
		// 대포 데미지 적용 (30 데미지)
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
	// 모든 상태 초기화
	CurrentStealthPhase = 0;
	bIsStealthStarting = false;
	bIsStealthDiving = false;
	bIsStealthInvisible = false;
	bIsStealthKicking = false;
	bIsStealthFinishing = false;

	bIsInvincible = false;

	SetActorHiddenInGame(false);

	// 쿨타임 시작
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
	if (bIsBossDead) return 0.0f; // 이미 사망했다면 데미지 무시
	float DamageApplied = FMath::Min(BossHealth, DamageAmount); // 보스의 체력과 데미지를 불러옴
	BossHealth -= DamageApplied; // 체력에서 데미지만큼 차감
	UE_LOG(LogTemp, Warning, TEXT("Boss took %f damage, Health remaining: %f"), DamageAmount, BossHealth);

	// 무적 상태 체크 추가
	if (bIsInvincible)
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss is invulnerable - damage ignored"));
		return 0.0f;
	}
	if (bIsPlayingBossIntro)
	{
		return 0.0f; // 등장 중에는 데미지 무효화
	}

	if (BossHitSound) // 사운드가 있다면
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossHitSound, GetActorLocation()); // 해당 엑터의 위치에서 소리재생
	}

	if (BossHitReactionMontages.Num() > 0)
	{
		bIsBossHit = true; // 히트중 여부 true
		bCanBossAttack = false; // 공격가능 여부 false

		// AI 이동 중지
		ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
		if (AICon)
		{
			AICon->StopMovement();
		}

		UBossEnemyAnimInstance* BossAnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
		if (BossAnimInstance)
		{
			BossAnimInstance->bUseUpperBodyBlend = false;
			BossAnimInstance->Montage_Stop(0.5f); // 기존 몽타주 중지
		}

		int32 RandomIndex = FMath::RandRange(0, BossHitReactionMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossHitReactionMontages[RandomIndex];
		if (SelectedMontage)
		{
			UAnimInstance* HitAnimInstance = GetMesh()->GetAnimInstance(); // 다른 이름 사용
			if (HitAnimInstance)
			{
				UE_LOG(LogTemp, Warning, TEXT("AnimInstance is valid before playing hit montage"));
				float PlayResult = HitAnimInstance->Montage_Play(SelectedMontage, 1.0f);

				// 피격 몽타주 종료 델리게이트 추가
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
				bIsBossHit = false; // 히트중 여부 초기화
				bCanBossAttack = true; // 공격가능 여부 초기화
			}
		}
	}

	if (BossHealth <= 0.0f) // 체력이 0이하인 경우
	{
		BossDie(); // 사망함수 호출
	}
	
	return DamageApplied; // 받은 피해 초기화
}

void ABossEnemy::OnHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Hit Reaction Montage Ended"));

	bIsBossHit = false; // 히트중 상태 false

	// 피격 애니메이션 종료 후 상태 복원
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // 기본값으로 복원
	}

	// AI 컨트롤러에 히트 종료 알림
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded(); // 공격 가능 상태 복원
	}

	bCanBossAttack = true; // 공격가능 여부 초기화
}

void ABossEnemy::BossDie()
{
	if (bIsBossDead) return; // 이미 사망한 경우 리턴
	bIsBossDead = true; // 사망상태 트루

	// GameMode에 보스 사망 알림
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->OnBossDead();
	}

	// 사망 사운드 재생
	if (BossDieSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossDieSound, GetActorLocation());
	}

	StopBossActions();

	UBossEnemyAnimInstance* BossAnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (BossAnimInstance)
	{
		BossAnimInstance->bUseUpperBodyBlend = false;
		BossAnimInstance->Montage_Stop(0.0f); // 몽타주 즉시 중지
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
					// 몽타주 길이의 지정된 시점에 보스 숨김
					float MontageLength = SelectedMontage->GetPlayLength();
					float HideTime = MontageLength * 0.9f; // 90% 시점

					GetWorld()->GetTimerManager().SetTimer(
						BossDeathHideTimerHandle,
						this,
						&ABossEnemy::HideBossEnemy,
						HideTime, // 지정된 시점에 숨김
						false
					);

					UE_LOG(LogTemp, Warning, TEXT("Death montage playing - will hide at 99%% (%.2f seconds)"), HideTime);
					return; // 타이머가 실행될 때까지 대기
				}
			}
		}
	}
	// 몽타주가 없거나 재생 실패시
	HideBossEnemy(); // 보스를 숨기는 함수 호출
}

void ABossEnemy::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Death Montage Ended"));
	HideBossEnemy(); // 사망 몽타주 종료 후 보스 숨김
}

void ABossEnemy::StopBossActions()
{
	GetCharacterMovement()->DisableMovement(); // 모든 이동 차단
	GetCharacterMovement()->StopMovementImmediately(); // 모든 이동 즉시 차단
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None); // 이동 모드 비활성화

	// AI 컨트롤러 즉시 중지
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->StopBossAI(); // AI 중지
	}

	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance()); // 최신 애님인스턴스를 캐스팅해서 받아옴
	if (AnimInstance) // 애님 인스턴스의
	{
		AnimInstance->Montage_Stop(0.0f); // 모든 몽타주 중지
	}

	// 사망시
	if (bIsBossDead)
	{
		SetActorTickEnabled(false); // 틱 비활성화

		// 델리게이트 정리
		UAnimInstance* BaseAnimInstance = GetMesh()->GetAnimInstance();
		if (BaseAnimInstance)
		{
			BaseAnimInstance->OnMontageEnded.RemoveAll(this);
		}
	}
}

void ABossEnemy::HideBossEnemy()
{
	if (!bIsBossDead) return; // 이미 사망한 경우 리턴

	UE_LOG(LogTemp, Warning, TEXT("Hiding Boss Enemy - Memory Cleanup"));

	// 1. 이벤트 및 델리게이트 정리 (최우선)
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 해제

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 애님 인스턴스 참조 받아옴
	if (AnimInstance && IsValid(AnimInstance)) // 애님 인스턴스가 있고 유효하다면
	{
		// 애니메이션 이벤트 바인딩 완전 해제
		AnimInstance->OnMontageEnded.RemoveAll(this); // 몽타주 종료 이벤트 바인딩 해제
		AnimInstance->OnMontageBlendingOut.RemoveAll(this); // 몽타주 블랜드 아웃 바인딩 해제
		AnimInstance->OnMontageStarted.RemoveAll(this); // 몽타주 시작 이벤트 바인딩 해체
	}

	// 2. AI 시스템 완전 정리 
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController()); // AI 컨트롤러 참조 받아옴
	if (AICon && IsValid(AICon)) // AI 컨트롤러가 있고 유효하다면
	{
		AICon->StopBossAI(); // AI 로직 중단
		AICon->UnPossess(); // 컨트롤러-폰 관계 해제
		AICon->Destroy(); // AI 컨트롤러 완전 제거
	}

	// 3. 무브먼트 시스템 정리
	UCharacterMovementComponent* MovementComp = GetCharacterMovement(); // 캐릭터 무브먼트 컴포넌트 참조 받아옴
	if (MovementComp && IsValid(MovementComp)) // 무브먼트 컴포넌트가 있고 유효하다면
	{
		MovementComp->DisableMovement(); // 이동 비활성화
		MovementComp->StopMovementImmediately(); // 현재 이동 즉시 중단
		MovementComp->SetMovementMode(EMovementMode::MOVE_None); // Move모드 None 설정으로 네비게이션에서 제외
		MovementComp->SetComponentTickEnabled(false); // 무브먼트 컴포넌트 Tick 비활성화
	}

	// 4. 메쉬 컴포넌트 정리
	USkeletalMeshComponent* MeshComp = GetMesh(); // 스켈레탈 메쉬 컴포넌트 참조 받아옴
	if (MeshComp && IsValid(MeshComp)) // 메쉬 컴포넌트가 있고 유효하다면
	{
		// 렌더링 시스템 비활성화
		MeshComp->SetVisibility(false); // 메쉬 가시성 비활성화
		MeshComp->SetHiddenInGame(true); // 게임 내 숨김 처리
 
		// 물리 시스템 비활성화
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // NoCollision 설정으로 충돌검사 비활성화
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // ECR_Ignore 설정으로 충돌응답 무시

		// 업데이트 시스템 비활성화
		MeshComp->SetComponentTickEnabled(false); // 메쉬 컴포넌트 Tick 비활성화

		// 애니메이션 참조 해제
		MeshComp->SetAnimInstanceClass(nullptr); // ABP 참조 해제
		MeshComp->SetSkeletalMesh(nullptr); // 스켈레탈 메쉬 참조 해제

	}

	// 5. 액터 레벨 시스템 정리
	SetActorHiddenInGame(true); // 액터 렌더링 비활성화
	SetActorEnableCollision(false); // 액터 충돌 비활성화
	SetActorTickEnabled(false); // 액터 Tick 비활성화
	SetCanBeDamaged(false); // 데미지 처리 비활성화

	// 6. 현재 프레임 처리 완료 후 다음 프레임에 안전하게 엑터 제거 (크래쉬 방지)
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<ABossEnemy>(this)]() // 스마트 포인터 WeakObjectPtr로 약한 참조를 사용하여 안전하게 지연 실행
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // 약한 참조한 엑터가 유효하고 파괴되지 않았다면
			{
				WeakThis->Destroy(); // 액터 완전 제거
				UE_LOG(LogTemp, Warning, TEXT("EnemyBoss Successfully Destroyed."));
			}
		});
}
