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
	StaticAngleOffset = 0.0f;
}

void AEnemyAIController::BeginPlay()
{
	UWorld* World = GetWorld();
	if (!World) return;

	Super::BeginPlay();

	PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0); // 플레이어 참조 초기화

	// 원 위치 주기적 갱신 타이머
	TWeakObjectPtr<AEnemyAIController> WeakThis(this); // 약참조 생성
	World->GetTimerManager().SetTimer(CirclePositionTimerHandle, 
		[WeakThis]()
		{
			if (WeakThis.IsValid()) // 유효성 검사
			{
				WeakThis->OnCirclePositionTimer(); // 원 위치 갱신 함수 호출
			}
		}, 2.0f, true); // 2초 마다 반복
}

void AEnemyAIController::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World) return;
	Super::Tick(DeltaTime);

	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (!EnemyCharacter || EnemyCharacter->bIsDead)
	{
		StopMovement();
		return;
	}

	// 상태 이상 체크 (등장, 스턴, 피격 중 이동 차단)
	UEnemyAnimInstance* AnimInstance = Cast<UEnemyAnimInstance>(EnemyCharacter->GetMesh()->GetAnimInstance());
	bool bIsReacting = AnimInstance && EnemyCharacter->HitReactionMontage && AnimInstance->Montage_IsPlaying(EnemyCharacter->HitReactionMontage);

	// 등장 애니메이션 재생 중이면 AI 행동 중지
	if (EnemyCharacter->bIsPlayingIntro || EnemyCharacter->bIsInAirStun || bIsReacting)
	{
		StopMovement();
		return;
	}

	if (!IsValid(PlayerPawn))
	{
		PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
		if (!IsValid(PlayerPawn)) return;
	}

	if (bIsDodging) return;


	//// [추가/수정] 피격 몽타주 재생 중인지 확인하여 이동 중지
	//// EnemyAnimInstance를 가져와 현재 HitReactionMontage를 재생 중인지 확인합니다.
	//UEnemyAnimInstance* AnimInstance = Cast<UEnemyAnimInstance>(EnemyCharacter->GetMesh()->GetAnimInstance());
	//if (AnimInstance && EnemyCharacter->HitReactionMontage && AnimInstance->Montage_IsPlaying(EnemyCharacter->HitReactionMontage))
	//{
	//	StopMovement();
	//	return;
	//}

	//if (!GetPawn() || bIsDodging) return;

	// 거리 계산 최적화 (제곱근 계산을 피하기 위해 DistSquared 사용)
	float DistanceToPlayerSquared = FVector::DistSquared(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation()); // 제곱 거리 계산
	float DistanceToPlayer = FMath::Sqrt(DistanceToPlayerSquared); // 실제 거리 계산이 필요한 경우에만 제곱근 계산

	UpdateAIState(DistanceToPlayer); // AI 상태 업데이트

	// 상태에 따른 행동 처리
	switch (CurrentState) 
	{
	case EEnemyAIState::MoveToCircle: // 원 위치로 이동
		if (!bHasCachedTarget) CalculateCirclePosition(); // 타겟 위치가 없으면 계산

		// 거리 계산 최적화 (제곱근 계산 피함)
		if (FVector::DistSquared(EnemyCharacter->GetActorLocation(), CachedTargetLocation) > FMath::Square(CircleArriveThreshold)) // 도달 거리 체크
		{
			MoveToLocation(CachedTargetLocation, 5.0f); // 원 위치로 이동
		}
		else // 도달했으면 대기 상태로 전환
		{
			StopMovement(); // 도착 시 이동 중지
			SetAIState(EEnemyAIState::Idle); // 대기 상태로 전환
		}
		break; // 이동 상태 끝

	case EEnemyAIState::ChasePlayer: // 플레이어 추적
	{
		MoveToActor(PlayerPawn, 5.0f); // 플레이어 근처로 이동

		if (!bIsJumpAttacking) JumpAttack(); // 점프 공격 시도

		if (DistanceToPlayerSquared <= FMath::Square(AttackRange) && bCanAttack) // 공격 범위 내 도달 및 공격 가능 시
		{
			if (NormalAttackCount == 3) // 일반공격을 3회 했으면
				StrongAttack(); // 강공격 실행
			else // 그 외의 경우
			{
				if (FMath::FRand() <= DodgeChance && bCanDodge) // 닷지 확률 체크
				{
					TryDodge(); // 닷지
				}
				else 
				{
					NormalAttack(); // 일반공격
				}
			}
		}
		break; // 추적 상태 끝
	}

	case EEnemyAIState::Idle: // 대기 상태
		StopMovement(); // 이동 중지
		break; // 대기 상태 끝
	}

	// 회전 보간 빈도 제한 (60fps → 10fps)
	RotationUpdateTimer += DeltaTime; // 시간 누적
	// if (RotationUpdateTimer >= 0.1f)
	if (RotationUpdateTimer >= 0.05f) // 20fps
	{
		FVector Direction = PlayerPawn->GetActorLocation() - EnemyCharacter->GetActorLocation(); // 플레이어 방향 계산
		FRotator LookAtRotation = Direction.Rotation(); // 회전 계산
		LookAtRotation.Pitch = 0.0f; // 피치값 0으로 고정
		LookAtRotation.Roll = 0.0f; // 롤값 0으로 고정
		EnemyCharacter->SetActorRotation(FMath::RInterpTo(EnemyCharacter->GetActorRotation(), LookAtRotation, RotationUpdateTimer, 5.0f)); // 보간 회전
		RotationUpdateTimer = 0.0f; // 타이머 초기화
	}
}

void AEnemyAIController::UpdateAIState(float DistanceToPlayer)
{
	if (DistanceToPlayer > StopChasingRadius) // 너무 멀리 떨어지면
	{
		SetAIState(EEnemyAIState::Idle); // 대기 상태로 전환
		bHasCachedTarget = false; // 타겟 위치 초기화
		bIsJumpAttacking = false; // 점프 공격 상태 초기화
		NormalAttackCount = 0; // 일반 공격 카운트 초기화
		return; // 상태 종료
	}

	if (DistanceToPlayer <= ChaseStartDistance) // 추적 시작 거리 이내면
	{ 
		SetAIState(EEnemyAIState::ChasePlayer); // 추적 상태로 전환
	}
	else // 그 외의 경우 원 위치로 이동 상태로 전환
	{
		SetAIState(EEnemyAIState::MoveToCircle); // 원 위치로 이동 상태로 전환
	}
}

void AEnemyAIController::SetAIState(EEnemyAIState NewState)
{
	if (CurrentState != NewState) // 상태가 변경될 때만 처리
	{
		StopMovement(); // 기존 이동 중지
		CurrentState = NewState; // 상태 갱신

		// 상태 전환 시마다 이속 재설정
		AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn()); // 캐릭터 참조
		if (EnemyCharacter)
		{
			EnemyCharacter->ApplyBaseWalkSpeed(); // 기본 이동속도 적용
		}

		if (CurrentState == EEnemyAIState::MoveToCircle) // 원 위치로 이동 상태일 때
		{
			CalculateCirclePosition(); // 타겟 위치 계산
		}
	}
}

void AEnemyAIController::OnCirclePositionTimer()
{
	if (CurrentState == EEnemyAIState::MoveToCircle) // 원 위치로 이동 상태일 때
	{
		CalculateCirclePosition(); // 타겟 위치 재계산
	}
}

void AEnemyAIController::CalculateCirclePosition()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn()); // 캐릭터 참조
	if (!EnemyCharacter || !PlayerPawn) return; // 유효성 검사

	StaticAngleOffset = (StaticAngleOffset + 1) % 360; // 각도 오프셋 갱신

	float Angle = FMath::DegreesToRadians(StaticAngleOffset + FMath::RandRange(-30, 30)); // 각도 계산
	float Radius = 200.0f + FMath::RandRange(-50.0f, 50.0f); // 반지름 계산

	FVector Offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius; // 오프셋 계산
	FVector TargetLocation = PlayerPawn->GetActorLocation() + Offset; // 목표 위치 계산

	// 네비게이션 메시 위로 보정
	FNavLocation NavLoc; // 네비게이션 위치 변수
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World); // 네비게이션 시스템 참조
	if (NavSystem && NavSystem->ProjectPointToNavigation(TargetLocation, NavLoc, FVector(50, 50, 100))) // 네비게이션에 위치 프로젝션
	{
		TargetLocation = NavLoc.Location; // 네비게이션 위치로 보정
	}

	CachedTargetLocation = TargetLocation; // 타겟 위치 캐싱
	bHasCachedTarget = true; // 타겟 위치 존재 플래그 설정
}

void AEnemyAIController::NormalAttack()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (!IsValid(EnemyCharacter) || EnemyCharacter->bIsInAirStun || bIsAttacking) return; // 스턴 상태면 공격 금지

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
		// 약참조 타이머 적용 - 공격 리셋
		TWeakObjectPtr<AEnemyAIController> WeakThis(this);
		World->GetTimerManager().SetTimer(NormalAttackTimerHandle, [WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->ResetAttack(); // 공격 리셋 함수 호출
				}
			}, AttackCooldown, false);
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
	UWorld* World = GetWorld();
	if (!World) return;

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

		// 약참조 타이머 적용 - 공격 리셋
		TWeakObjectPtr<AEnemyAIController> WeakThis(this);
		World->GetTimerManager().SetTimer(NormalAttackTimerHandle,
			[WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->ResetAttack(); // 공격 리셋 함수 호출
				}
			}, AttackCooldown, false);
	}
}

void AEnemyAIController::TryDodge()
{
	UWorld* World = GetWorld();
	if (!World) return;

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

	TWeakObjectPtr<AEnemyAIController> WeakThis(this);
	World->GetTimerManager().SetTimer(DodgeTimerHandle, 
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ResetDodge(); // 닷지 리셋 함수 호출
			}
		}, DodgeDuration, false);

	// 회피 후 일정 시간 동안 회피 불가상태 유지
	World->GetTimerManager().SetTimer(DodgeCooldownTimerHandle,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ResetDodgeCoolDown(); // 닷지 쿨다운 리셋 함수 호출
			}
		}, DodgeCooldown, false);
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
	UWorld* World = GetWorld();
	if (!World) return;

	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (!EnemyCharacter) return;

	if (bIsJumpAttacking) return;

	bIsJumpAttacking = true; // 점프 공격 사용 상태 설정
	bCanAttack = false; // 점프 공격 중 다른 공격 불가

	EnemyCharacter->PlayJumpAttackAnimation(); // 점프 공격 애니메이션 실행

	StopMovement();

	// 점프 공격 타이머 전용 핸들 사용
	float JumpAttackDuration = EnemyCharacter->GetJumpAttackDuration();
	TWeakObjectPtr<AEnemyAIController> WeakThis(this);
	World->GetTimerManager().SetTimer(JumpAttackTimerHandle,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ResetAttack(); // 공격 리셋 함수 호출
			}
		}, JumpAttackDuration, false);;
}

void AEnemyAIController::MoveToDistributedLocation()
{
	UWorld* World = GetWorld();
	if (!World) return;

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
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (NavSys && NavSys->ProjectPointToNavigation(TargetLocation, NavLoc, FVector(50, 50, 100)))
	{
		TargetLocation = NavLoc.Location;
	}

	// 이동 명령
	MoveToLocation(TargetLocation, 5.0f);
}

void AEnemyAIController::StopAI()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
	if (!EnemyCharacter) return;

	// 먼저 모든 이동 중지
	StopMovement();
	UE_LOG(LogTemp, Warning, TEXT("%s AI Movement Stopped"), *GetPawn()->GetName());

	// 모든 타이머 정리
	World->GetTimerManager().ClearTimer(NormalAttackTimerHandle);
	World->GetTimerManager().ClearTimer(DodgeTimerHandle);
	World->GetTimerManager().ClearTimer(DodgeCooldownTimerHandle);
	World->GetTimerManager().ClearTimer(JumpAttackTimerHandle);
	World->GetTimerManager().ClearTimer(CirclePositionTimerHandle);

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
