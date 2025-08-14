#include "EnemyDogAIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFrameWork/Character.h"
#include "EnemyDog.h"
#include "EnemyDogAnimInstance.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

AEnemyDogAIController::AEnemyDogAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(0.05f);
	
	RotationUpdateTimer = 0.0f;
	StaticAngleOffset = 0;
}

void AEnemyDogAIController::BeginPlay()
{
	Super::BeginPlay();
	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
}

void AEnemyDogAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AEnemyDog* EnemyDogCharcter = Cast<AEnemyDog>(GetPawn());
	if (!EnemyDogCharcter || EnemyDogCharcter->bIsDead)
	{
		StopMovement();
		return;
	}

	if (EnemyDogCharcter->bIsPlayingIntro)
	{
		StopMovement();
		return;
	}

	if (EnemyDogCharcter->bIsInAirStun)
	{
		StopMovement();
		return;
	}

	if (!PlayerPawn)
	{
		PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (!PlayerPawn) return;
	}

	// 항상 플레이어 바라보기 (공격/포위 시 방향 유지)
	FVector ToTarget = PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation();
	ToTarget.Z = 0; // 상하 무시
	if (!ToTarget.IsNearlyZero())
	{
		FRotator TargetRot = ToTarget.Rotation();
		GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), TargetRot, DeltaTime, 10.0f));
	}

	float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
	UpdateAIState(DistanceToPlayer);

	switch (CurrentState)
	{
	case EEnemyDogAIState::Idle:
		// 캐릭터 정지
		StopMovement();
		break;

	case EEnemyDogAIState::ChasePlayer:
		ChasePlayer();
		break;
	}

	// 공격 범위 원 (빨간색, 반투명)
	//DrawDebugSphere(GetWorld(), GetPawn()->GetActorLocation(), AttackRange, 32, FColor::Red, false, -1.0f, 0, 2.0f);

	// 추적 시작 거리 원 (파란색, 반투명)
	//DrawDebugSphere(GetWorld(), GetPawn()->GetActorLocation(), ChaseStartDistance, 32, FColor::Blue, false, -1.0f, 0, 1.0f);
}

void AEnemyDogAIController::UpdateAIState(float DistanceToPlayer)
{
	if (DistanceToPlayer <= AttackRange && bCanAttack)
	{
		SetAIState(EEnemyDogAIState::Idle);
		NormalAttack();
	}
	else if (DistanceToPlayer <= ChaseStartDistance)
	{
		SetAIState(EEnemyDogAIState::ChasePlayer);
	}
	else
	{
		SetAIState(EEnemyDogAIState::Idle);
	}
}

void AEnemyDogAIController::SetAIState(EEnemyDogAIState NewState)
{
	if (CurrentState == NewState) return;
	CurrentState = NewState;
}

void AEnemyDogAIController::ChasePlayer()
{
	if (!PlayerPawn || !GetPawn()) return; // 플레이어 또는 내 폰이 널이면 함수 종료

	// 현재 맵에 존재하는 모든 EnemyDog 엑터들을 배열에 가져오기
	TArray<AActor*> AllEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDog::StaticClass(), AllEnemies); // 자기 자신과 다른 모든 개체가 포함

	if (AllEnemies.Num() <= 1) // 만약 적이 1마리라면
	{
		MoveToActor(PlayerPawn, 10.0f); // 플레이어에게 단순히 직진
		return;
	}

	int32 MyIndex = AllEnemies.IndexOfByKey(GetPawn()); // 전체 개 리스트에서 내 폰이 인덱스에서 몇 번째인지 찾기
	if (MyIndex == INDEX_NONE) // 이 값이 자신의 순번 MyIndex
	{
		MoveToActor(PlayerPawn, 10.0f); // 배열에 자신이 없다면 단순히 직진 (방어코드)
		return;
	}

	float AngleDeg = 360.0f / AllEnemies.Num(); // 적 전체의 수를 360도에 나눔
	float MyAngleDeg = AngleDeg * MyIndex; // 각 적마다 고유 각도를 부여 MyIndex 값에 따라 자신의 각도를 정함
	float MyAngleRad = FMath::DegreesToRadians(MyAngleDeg); // 라디안 단위로 변환

	// 포위 위치 계산
	FVector PlayerLocation = PlayerPawn->GetActorLocation(); // 플레이어 위치를 기준으로
	FVector Offset = FVector(FMath::Cos(MyAngleRad), FMath::Sin(MyAngleRad), 0) * SurroundRadius; // SurroundRadius 만큼 떨어진 원형 궤도의 좌표 생성 Cos, Sin 단위로 원의 X,Y 값을 구하고 반지름을 곱한 오프셋값
	FVector TargetLocation = PlayerLocation + Offset;  // 플레이어 위치에 오프셋값을 더해서 상대 좌표를 절대 좌표로 변환

	MoveToLocation(TargetLocation, 10.0f); // 포위 후 이동

	// 디버그 시각화 (원 주변 포지션 확인용)
	DrawDebugSphere(GetWorld(), TargetLocation, 25.0f, 8, FColor::Red, false, 0.05f); 
}

void AEnemyDogAIController::NormalAttack()
{
	AEnemyDog* EnemyDog = Cast<AEnemyDog>(GetPawn());
	if (!EnemyDog || EnemyDog->bIsInAirStun) return;
	if (bIsAttacking) return;

	// 공격 직전에 이동 멈추기
	StopMovement();

	if (PlayerPawn)
	{
		// 현재 회전값과 목표 회전값 구하기
		FRotator CurrentRotation = GetPawn()->GetActorRotation();
		FRotator TargetRotation = (PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation()).Rotation();
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;

		// 회전을 부드럽게 보간하기 위한 타이머 설정
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), 10.0f); // 10.0f는 회전 속도 조절 값
		GetPawn()->SetActorRotation(NewRotation);
	}

	bIsAttacking = true;
	bCanAttack = false;

	FTimerHandle AttackDelayTimer;
	GetWorld()->GetTimerManager().SetTimer(
		AttackDelayTimer,
		[this, EnemyDog]()
		{
			EnemyDog->PlayNormalAttackAnimation();
		},
		0.1f,  // 회전 후 약간 더 기다리기(예: 100밀리초)
		false
	);

	// 공격 쿨타임 타이머
	GetWorld()->GetTimerManager().SetTimer(NormalAttackTimerHandle, this, &AEnemyDogAIController::ResetAttack, AttackCooldown, false);
}

void AEnemyDogAIController::ResetAttack()
{
	bCanAttack = true;
	bIsAttacking = false;
}

void AEnemyDogAIController::StopAI()
{
	AEnemyDog* EnemyDogCharacter = Cast<AEnemyDog>(GetPawn());
	if (!EnemyDogCharacter) return;

	bIsAttacking = false;
	bCanAttack = false;
	StopMovement();
}