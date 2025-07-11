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
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	// 원 위치 주기적 갱신 타이머 (0.7초마다)
	GetWorld()->GetTimerManager().SetTimer(CirclePositionTimerHandle, this, &AEnemyAIController::OnCirclePositionTimer, 0.7f, true);
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//APawn* ControlledPawn = GetPawn(); // 폰이 존재할때마다 디버그를 그림
	//if (!ControlledPawn) return;

	//FVector PawnLocation = ControlledPawn->GetActorLocation();

	// 빨간색: 공격 범위
	//DrawDebugSphere(GetWorld(), PawnLocation, AttackRange, 32, FColor::Red, false, -1.0f, 0, 2.0f);

	// 파란색: 원 위치 도달 거리
	//DrawDebugSphere(GetWorld(), PawnLocation, CircleArriveThreshold, 32, FColor::Blue, false, -1.0f, 0, 2.0f);

	// 초록색: 추적 시작 거리
	//DrawDebugSphere(GetWorld(), PawnLocation, ChaseStartDistance, 32, FColor::Green, false, -1.0f, 0, 2.0f);

	// 하늘색: 플레이어 기준으로 하늘색 원 반지름
	if (PlayerPawn)
	{
		DrawDebugSphere(GetWorld(), PlayerPawn->GetActorLocation(), CircleRadius, 32, FColor::Cyan, false, -1.0f, 0, 2.0f);
	}

	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (!EnemyCharacter || EnemyCharacter->bIsDead)
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

	float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
	UpdateAIState(DistanceToPlayer);

	switch (CurrentState)
	{
	case EEnemyAIState::MoveToCircle:
		if (!bHasCachedTarget)
			CalculateCirclePosition();

		if (FVector::Dist(GetPawn()->GetActorLocation(), CachedTargetLocation) > CircleArriveThreshold)
		{
			MoveToLocation(CachedTargetLocation, 5.0f);
		}
		else
		{
			StopMovement();
			SetAIState(EEnemyAIState::Idle);
		}
		// 필요시 MoveToCircle 상태에서도 공격,회피 조건
		break;

	case EEnemyAIState::ChasePlayer:
	{
		MoveToActor(PlayerPawn, 5.0f);

		// 공격/회피 조건 체크
		float DistToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());

		if (!bIsJumpAttacking)
			JumpAttack();

		if (DistToPlayer <= AttackRange && bCanAttack)
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

	// 항상 플레이어 바라보기
	FRotator LookAtRotation = (PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation()).Rotation();
	LookAtRotation.Pitch = 0.0f;
	LookAtRotation.Roll = 0.0f;
	GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAtRotation, DeltaTime, 5.0f));
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

	TArray<AActor*> AllEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), AllEnemies);

	int32 MyIndex = AllEnemies.IndexOfByKey(EnemyCharacter);
	int32 TotalEnemies = AllEnemies.Num();

	float Angle = 0.0f;
	if (TotalEnemies > 0)
	{
		Angle = 2 * PI * (MyIndex / (float)TotalEnemies);
	}
	float Radius = 200.0f;
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

	//UE_LOG(LogTemp, Warning, TEXT("NormalAttack() called. Current NormalAttackCount: %d"), NormalAttackCount);

	// 강공격 체크: 일반 공격 3회 후 강공격 실행
	if (NormalAttackCount >= 3)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Triggering StrongAttack. Resetting NormalAttackCount."));
		StrongAttack();
		return;
	}

	if (EnemyCharacter)
	{
		UAnimInstance* AnimInstance = EnemyCharacter->GetMesh()->GetAnimInstance();

		// 현재 애니메이션이 진행 중이라면 새로운 공격 차단
		if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
		{
			//UE_LOG(LogTemp, Warning, TEXT("NormalAttack() blocked: Animation still playing."));
			return;
		}

		bIsAttacking = true; // 공격 진행중
		EnemyCharacter->PlayNormalAttackAnimation();
		NormalAttackCount++; // 일반 공격 횟수 카운트 증가

		//UE_LOG(LogTemp, Warning, TEXT("Normal Attack performed. Updated NormalAttackCount: %d"), NormalAttackCount);

		bCanAttack = false; // 공격 후 쿨다운 적용
		GetWorld()->GetTimerManager().SetTimer(NormalAttackTimerHandle, this, &AEnemyAIController::ResetAttack, AttackCooldown, false);
	}
}

void AEnemyAIController::ResetAttack()
{
	//UE_LOG(LogTemp, Warning, TEXT("ResetAttack() called. Before Reset: NormalAttackCount = %d"), NormalAttackCount);

	bCanAttack = true; // 공격 쿨다운 코기화
	bIsStrongAttacking = false; // 강 공격 종료
	bIsAttacking = false; // 공격 상태 종료

	// 강공격 이후 일반 공격 초기화
	if (bIsStrongAttacking)
	{
		NormalAttackCount = 0;
		bCanStrongAttack = false;
	}

	//UE_LOG(LogTemp, Warning, TEXT("ResetAttack() completed. After Reset: NormalAttackCount = %d"), NormalAttackCount);
}

void AEnemyAIController::StrongAttack()
{
	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (EnemyCharacter->bIsInAirStun) return; // 스턴 상태면 공격 금지

	if (bIsAttacking || bIsStrongAttacking) return; // 현재 공격 중이면 실행 금지

	//UE_LOG(LogTemp, Warning, TEXT("StrongAttack() called. Resetting NormalAttackCount to 0"));

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

	//UE_LOG(LogTemp, Warning, TEXT("TryDodge() called. NormalAttackCount before dodge: %d"), NormalAttackCount);

	bIsDodging = true;
	bCanDodge = false; //연속 닷지방지

	bool bDodgeLeft = FMath::RandBool(); // 좌/우 랜덤 회피 결정
	EnemyCharacter->PlayDodgeAnimation(bDodgeLeft);

	// 닷지 몽타주의 정확한 길이를 가져와서 타이머 설정
	float DodgeDuration = (bDodgeLeft ? EnemyCharacter->GetDodgeLeftDuration() : EnemyCharacter->GetDodgeRightDuration());
	GetWorld()->GetTimerManager().SetTimer(DodgeTimerHandle, this, &AEnemyAIController::ResetDodge, DodgeDuration, false);
	// 회피 후 일정 시간 동안 회피 불가상태 유지
	GetWorld()->GetTimerManager().SetTimer(DodgeCooldownTimerHandle, this, &AEnemyAIController::ResetDodgeCoolDown, DodgeCooldown, false);

	//UE_LOG(LogTemp, Warning, TEXT("Dodge executed. NormalAttackCount after dodge: %d"), NormalAttackCount);
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

	// 1. 월드 내 모든 적 찾기
	TArray<AActor*> AllEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), AllEnemies);

	// 2. 내 인덱스 찾기
	int32 MyIndex = AllEnemies.IndexOfByKey(EnemyCharacter);
	int32 TotalEnemies = AllEnemies.Num();

	// 3. 목표 위치 각도 계산 (랜덤성 부여)
	float BaseAngle = 0.0f;
	float AngleRandomOffset = FMath::FRandRange(-0.2f, 0.2f); // -0.2~0.2 라디안 랜덤 오프셋
	if (TotalEnemies > 0)
	{
		BaseAngle = 2 * PI * (MyIndex / (float)TotalEnemies) + AngleRandomOffset;
	}

	float Radius = 200.0f;
	FVector Offset = FVector(FMath::Cos(BaseAngle), FMath::Sin(BaseAngle), 0) * Radius;
	FVector TargetLocation = PlayerPawn->GetActorLocation() + Offset;

	// 4. 충돌 회피: 주변 적들과 너무 가까우면 반지름을 늘림
	float MinDistanceToOther = 99999.0f;
	for (AActor* OtherEnemy : AllEnemies)
	{
		if (OtherEnemy != EnemyCharacter)
		{
			float Dist = FVector::Dist(TargetLocation, OtherEnemy->GetActorLocation());
			if (Dist < MinDistanceToOther)
				MinDistanceToOther = Dist;
		}
	}
	if (MinDistanceToOther < 120.0f)
	{
		float ExtraRadius = 50.0f + FMath::FRandRange(0, 50.0f);
		Offset = FVector(FMath::Cos(BaseAngle), FMath::Sin(BaseAngle), 0) * (Radius + ExtraRadius);
		TargetLocation = PlayerPawn->GetActorLocation() + Offset;
	}

	// 5. 네비게이션 메시 위로 위치 보정
	FNavLocation NavLoc;
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSys && NavSys->ProjectPointToNavigation(TargetLocation, NavLoc, FVector(50, 50, 100)))
	{
		TargetLocation = NavLoc.Location;
	}

	// 6. 이동 명령
	MoveToLocation(TargetLocation, 5.0f);
}

void AEnemyAIController::StopAI()
{
	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (!EnemyCharacter) return;

	// 먼저 모든 이동 중지
	StopMovement();
	UE_LOG(LogTemp, Warning, TEXT("%s AI Movement Stopped"), *GetPawn()->GetName());

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