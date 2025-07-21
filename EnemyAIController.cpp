#include "EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Enemy.h"
#include "EnemyAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	// AI 업데이트 빈도 제한 (60fps → 20fps)
	SetActorTickInterval(0.05f);

	// 성능 최적화를 위한 변수 초기화
	RotationUpdateTimer = 0.0f;
	StaticAngleOffset = 0;
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	// 원 위치 주기적 갱신 타이머 (0.7초 → 2초로 증가)
	GetWorld()->GetTimerManager().SetTimer(CirclePositionTimerHandle, this, &AEnemyAIController::OnCirclePositionTimer, 2.0f, true);
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (!EnemyCharacter || EnemyCharacter->bIsDead)
	{
		StopMovement();
		return;
	}

	// 등장 애니메이션 재생 중이면 AI 행동 중지
	if (EnemyCharacter->bIsPlayingIntro)
	{
		StopMovement();
		return;
	}

	if (EnemyCharacter->bIsInAirStun)
	{
		StopMovement();
		return;
	}

	if (!PlayerPawn)
	{
		PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (!PlayerPawn) return;
	}

	if (!GetPawn() || bIsDodging) return;

	// 거리 계산 최적화 (제곱근 계산을 피하기 위해 DistSquared 사용)
	float DistanceToPlayerSquared = FVector::DistSquared(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
	float DistanceToPlayer = FMath::Sqrt(DistanceToPlayerSquared);

	UpdateAIState(DistanceToPlayer);

	switch (CurrentState)
	{
	case EEnemyAIState::MoveToCircle:
		if (!bHasCachedTarget)
			CalculateCirclePosition();

		// 거리 계산 최적화 (제곱근 계산 피함)
		if (FVector::DistSquared(GetPawn()->GetActorLocation(), CachedTargetLocation) > FMath::Square(CircleArriveThreshold))
		{
			MoveToLocation(CachedTargetLocation, 5.0f);
		}
		else
		{
			StopMovement();
			SetAIState(EEnemyAIState::Idle);
		}
		break;

	case EEnemyAIState::ChasePlayer:
	{
		MoveToActor(PlayerPawn, 5.0f);

		// 공격/회피 조건 체크 (제곱근 계산 피함)
		if (!bIsJumpAttacking)
			JumpAttack();

		if (DistanceToPlayerSquared <= FMath::Square(AttackRange) && bCanAttack)
		{
			if (NormalAttackCount == 3)
				StrongAttack();
			else
			{
				if (FMath::FRand() <= DodgeChance && bCanDodge)
					TryDodge();
				else
					NormalAttack();
			}
		}
		break;
	}

	case EEnemyAIState::Idle:
		StopMovement();
		break;
	}

	// 회전 보간 빈도 제한 (60fps → 10fps)
	RotationUpdateTimer += DeltaTime;
	if (RotationUpdateTimer >= 0.1f)
	{
		FRotator LookAtRotation = (PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation()).Rotation();
		LookAtRotation.Pitch = 0.0f;
		LookAtRotation.Roll = 0.0f;
		GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAtRotation, RotationUpdateTimer, 5.0f));
		RotationUpdateTimer = 0.0f;
	}
}

void AEnemyAIController::UpdateAIState(float DistanceToPlayer)
{
	if (DistanceToPlayer > StopChasingRadius)
	{
		SetAIState(EEnemyAIState::Idle);
		bHasCachedTarget = false;
		bIsJumpAttacking = false;
		NormalAttackCount = 0;
		return;
	}

	if (DistanceToPlayer <= ChaseStartDistance)
	{
		SetAIState(EEnemyAIState::ChasePlayer);
	}
	else
	{
		SetAIState(EEnemyAIState::MoveToCircle);
	}
}

void AEnemyAIController::SetAIState(EEnemyAIState NewState)
{
	if (CurrentState != NewState)
	{
		StopMovement();
		CurrentState = NewState;

		// 상태 전환 시마다 이속 재설정
		AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
		if (EnemyCharacter)
		{
			EnemyCharacter->ApplyBaseWalkSpeed();
		}

		if (CurrentState == EEnemyAIState::MoveToCircle)
			CalculateCirclePosition();
	}
}

void AEnemyAIController::OnCirclePositionTimer()
{
	if (CurrentState == EEnemyAIState::MoveToCircle)
		CalculateCirclePosition();
}

void AEnemyAIController::CalculateCirclePosition()
{
	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (!EnemyCharacter || !PlayerPawn) return;

	StaticAngleOffset = (StaticAngleOffset + 1) % 360;

	float Angle = FMath::DegreesToRadians(StaticAngleOffset + FMath::RandRange(-30, 30));
	float Radius = 200.0f + FMath::RandRange(-50.0f, 50.0f);

	FVector Offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius;
	FVector TargetLocation = PlayerPawn->GetActorLocation() + Offset;

	// 네비게이션 메시 위로 보정
	FNavLocation NavLoc;
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSys && NavSys->ProjectPointToNavigation(TargetLocation, NavLoc, FVector(50, 50, 100)))
	{
		TargetLocation = NavLoc.Location;
	}

	CachedTargetLocation = TargetLocation;
	bHasCachedTarget = true;
}

void AEnemyAIController::NormalAttack()
{
	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (EnemyCharacter->bIsInAirStun) return; // 스턴 상태면 공격 금지

	if (bIsAttacking) return; // 현재 공격 중이면 중복 실행 방지

	// 강공격 체크: 일반 공격 3회 후 강공격 실행
	if (NormalAttackCount >= 3)
	{
		StrongAttack();
		return;
	}

	if (EnemyCharacter)
	{
		UAnimInstance* AnimInstance = EnemyCharacter->GetMesh()->GetAnimInstance();

		// 현재 애니메이션이 진행 중이라면 새로운 공격 차단
		if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
		{
			return;
		}

		bIsAttacking = true; // 공격 진행중
		EnemyCharacter->PlayNormalAttackAnimation();
		NormalAttackCount++; // 일반 공격 횟수 카운트 증가

		bCanAttack = false; // 공격 후 쿨다운 적용
		GetWorld()->GetTimerManager().SetTimer(NormalAttackTimerHandle, this, &AEnemyAIController::ResetAttack, AttackCooldown, false);
	}
}

void AEnemyAIController::ResetAttack()
{
	bCanAttack = true; // 공격 쿨다운 초기화
	bIsStrongAttacking = false; // 강 공격 종료
	bIsAttacking = false; // 공격 상태 종료

	// 강공격 이후 일반 공격 초기화
	if (bIsStrongAttacking)
	{
		NormalAttackCount = 0;
		bCanStrongAttack = false;
	}
}

void AEnemyAIController::StrongAttack()
{
	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (EnemyCharacter->bIsInAirStun) return; // 스턴 상태면 공격 금지

	if (bIsAttacking || bIsStrongAttacking) return; // 현재 공격 중이면 실행 금지

	if (EnemyCharacter)
	{
		bIsAttacking = true; // 공격 진행중
		bIsStrongAttacking = true; // 강 공격 시작
		EnemyCharacter->PlayStrongAttackAnimation();
		NormalAttackCount = 0; // 일반 공격 카운트 초기화
		bCanStrongAttack = false; // 강공격 후 쿨다운 적용
		bCanAttack = false; // 공격후 쿨다운 적용
		GetWorld()->GetTimerManager().SetTimer(NormalAttackTimerHandle, this, &AEnemyAIController::ResetAttack, AttackCooldown, false);
	}
}

void AEnemyAIController::TryDodge()
{
	if (bIsDodging || !bCanDodge || !bCanAttack || bIsAttacking || !bCanStrongAttack || bIsStrongAttacking) return; // 공격 진행 중엔 닷지 불가

	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());

	if (EnemyCharacter->bIsInAirStun) return; // 스턴 상태면 닷지 금지
	if (!EnemyCharacter) return;

	bIsDodging = true;
	bCanDodge = false; //연속 닷지방지

	bool bDodgeLeft = FMath::RandBool(); // 좌/우 랜덤 회피 결정
	EnemyCharacter->PlayDodgeAnimation(bDodgeLeft);

	// 닷지 몽타주의 정확한 길이를 가져와서 타이머 설정
	float DodgeDuration = (bDodgeLeft ? EnemyCharacter->GetDodgeLeftDuration() : EnemyCharacter->GetDodgeRightDuration());
	GetWorld()->GetTimerManager().SetTimer(DodgeTimerHandle, this, &AEnemyAIController::ResetDodge, DodgeDuration, false);
	// 회피 후 일정 시간 동안 회피 불가상태 유지
	GetWorld()->GetTimerManager().SetTimer(DodgeCooldownTimerHandle, this, &AEnemyAIController::ResetDodgeCoolDown, DodgeCooldown, false);
}

void AEnemyAIController::ResetDodge()
{
	bIsDodging = false; // 닷지 초기화
}

void AEnemyAIController::ResetDodgeCoolDown()
{
	bCanDodge = true; // 지정된시간이 지나면 닷지 가능
}

void AEnemyAIController::JumpAttack()
{
	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (!EnemyCharacter) return;

	if (bIsJumpAttacking) return;

	bIsJumpAttacking = true; // 점프 공격 사용 상태 설정
	bCanAttack = false; // 점프 공격 중 다른 공격 불가

	EnemyCharacter->PlayJumpAttackAnimation(); // 점프 공격 애니메이션 실행

	StopMovement();

	// 점프 공격 타이머 전용 핸들 사용
	float JumpAttackDuration = EnemyCharacter->GetJumpAttackDuration();
	GetWorld()->GetTimerManager().SetTimer(JumpAttackTimerHandle, this, &AEnemyAIController::ResetAttack, JumpAttackDuration, false);
}

void AEnemyAIController::MoveToDistributedLocation()
{
	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (!EnemyCharacter || !PlayerPawn) return;

	// 성능 최적화를 위해 복잡한 분산 로직 대신 단순한 랜덤 위치 생성
	float BaseAngle = FMath::DegreesToRadians(FMath::RandRange(0, 360));
	float AngleRandomOffset = FMath::FRandRange(-0.2f, 0.2f);
	float FinalAngle = BaseAngle + AngleRandomOffset;

	float Radius = 200.0f;
	FVector Offset = FVector(FMath::Cos(FinalAngle), FMath::Sin(FinalAngle), 0) * Radius;
	FVector TargetLocation = PlayerPawn->GetActorLocation() + Offset;

	// 충돌 회피를 위한 반지름 추가 (간단한 랜덤 방식)
	if (FMath::FRand() < 0.3f) // 30% 확률로 거리 증가
	{
		float ExtraRadius = 50.0f + FMath::FRandRange(0, 50.0f);
		Offset = FVector(FMath::Cos(FinalAngle), FMath::Sin(FinalAngle), 0) * (Radius + ExtraRadius);
		TargetLocation = PlayerPawn->GetActorLocation() + Offset;
	}

	// 네비게이션 메시 위로 위치 보정
	FNavLocation NavLoc;
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSys && NavSys->ProjectPointToNavigation(TargetLocation, NavLoc, FVector(50, 50, 100)))
	{
		TargetLocation = NavLoc.Location;
	}

	// 이동 명령
	MoveToLocation(TargetLocation, 5.0f);
}

void AEnemyAIController::StopAI()
{
	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (!EnemyCharacter) return;

	// 먼저 모든 이동 중지
	StopMovement();
	UE_LOG(LogTemp, Warning, TEXT("%s AI Movement Stopped"), *GetPawn()->GetName());

	// 모든 타이머 정리
	GetWorld()->GetTimerManager().ClearTimer(NormalAttackTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DodgeTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DodgeCooldownTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(JumpAttackTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(CirclePositionTimerHandle);

	// AI를 완전히 분리하기 위해 폰 UnPossess
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("AI Controller is UnPossessing %s"), *ControlledPawn->GetName());
		UnPossess();

		// 추가 안전 조치: 캐릭터의 충돌 및 틱 비활성화
		ControlledPawn->SetActorEnableCollision(false);
		ControlledPawn->SetActorTickEnabled(false);
	}
}
