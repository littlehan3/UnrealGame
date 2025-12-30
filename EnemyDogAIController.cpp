#include "EnemyDogAIController.h"
#include "Kismet/GameplayStatics.h" // UGameplayStatics 함수 사용
#include "NavigationSystem.h" // 네비게이션 시스템 사용
#include "GameFrameWork/Character.h" // ACharacter 클래스 참조
#include "EnemyDog.h" // 제어할 EnemyDog 클래스 참조
#include "EnemyDogAnimInstance.h" // 애님 인스턴스 클래스 참조
#include "TimerManager.h" // FTimerManager 사용
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트 참조
#include "DrawDebugHelpers.h" // 디버그 시각화 기능 사용

AEnemyDogAIController::AEnemyDogAIController()
{
	PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 설정
	SetActorTickInterval(0.05f); // Tick 주기 0.05초로 최적화

	RotationUpdateTimer = 0.0f; // 변수 초기화
	StaticAngleOffset = 0; // 변수 초기화
}

void AEnemyDogAIController::BeginPlay()
{
	Super::BeginPlay(); // 부모 클래스 BeginPlay 호출
	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // 게임 시작 시 플레이어 폰을 찾아 저장
}

void AEnemyDogAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // 부모 클래스 Tick 호출

	AEnemyDog* EnemyDogCharacter = Cast<AEnemyDog>(GetPawn()); // 현재 컨트롤러가 빙의한 폰을 EnemyDog으로 캐스팅
	if (!EnemyDogCharacter || EnemyDogCharacter->bIsDead) // 폰이 없거나 죽었다면
	{
		StopMovement(); // 이동 중지
		return; // Tick 함수 종료
	}

	if (EnemyDogCharacter->bIsPlayingIntro) // 등장 애니메이션 중이라면
	{
		StopMovement(); // 이동 중지
		return;
	}

	if (EnemyDogCharacter->bIsInAirStun) // 공중 스턴 상태라면
	{
		StopMovement(); // 이동 중지
		return;
	}

	if (EnemyDogCharacter->bIsTrappedInGravityField)
	{
		StopMovement();
		return;
	}

	if (!PlayerPawn) // 플레이어 폰 참조가 없다면
	{
		PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // 다시 찾아봄
		if (!PlayerPawn) return; // 그래도 없으면 Tick 함수 종료
	}

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(EnemyDogCharacter->GetMesh()->GetAnimInstance());
	if (AnimInstance && EnemyDogCharacter->HitMontages.Num() > 0)
	{
		for (UAnimMontage* Montage : EnemyDogCharacter->HitMontages)
		{
			if (Montage && AnimInstance->Montage_IsPlaying(Montage))
			{
				StopMovement();
				return; // 피격 몽타주 재생 중이면 즉시 Tick 종료
			}
		}
	}

	// AI가 항상 플레이어를 바라보도록 설정
	FVector ToTarget = PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation(); // 자신에게서 플레이어로 향하는 방향 벡터
	ToTarget.Z = 0; // 위아래(Z축)는 무시하여 수평으로만 바라보게 함
	if (!ToTarget.IsNearlyZero())
	{
		FRotator TargetRot = ToTarget.Rotation(); // 방향 벡터를 회전값(Rotator)으로 변환
		GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), TargetRot, DeltaTime, 10.0f)); // 부드럽게 회전
	}

	float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation()); // 플레이어와의 거리 계산
	UpdateAIState(DistanceToPlayer); // 거리에 따라 AI 상태 갱신

	// 현재 AI 상태에 따라 행동 결정
	switch (CurrentState)
	{
	case EEnemyDogAIState::Idle: // 대기 상태라면
		StopMovement(); // 이동 중지
		break;

	case EEnemyDogAIState::ChasePlayer: // 추적 상태라면
		ChasePlayer(); // 추적 함수 호출
		break;
	}

	// 디버그용: 공격 범위 및 추적 시작 거리 시각화
	//DrawDebugSphere(GetWorld(), GetPawn()->GetActorLocation(), AttackRange, 32, FColor::Red, false, -1.0f, 0, 2.0f);
	//DrawDebugSphere(GetWorld(), GetPawn()->GetActorLocation(), ChaseStartDistance, 32, FColor::Blue, false, -1.0f, 0, 1.0f);
}

void AEnemyDogAIController::UpdateAIState(float DistanceToPlayer)
{
	if (DistanceToPlayer <= AttackRange && bCanAttack) // 공격 범위 내에 있고 공격이 가능하다면
	{
		SetAIState(EEnemyDogAIState::Idle); // 대기 상태로 변경
		NormalAttack(); // 공격 실행
	}
	else if (DistanceToPlayer <= ChaseStartDistance) // 추적 시작 거리 내에 있다면
	{
		SetAIState(EEnemyDogAIState::ChasePlayer); // 추적 상태로 변경
	}
	else // 너무 멀리 떨어져 있다면
	{
		SetAIState(EEnemyDogAIState::Idle); // 대기 상태로 변경
	}
}

void AEnemyDogAIController::SetAIState(EEnemyDogAIState NewState)
{
	if (CurrentState == NewState) return; // 현재 상태와 같다면 변경하지 않음
	CurrentState = NewState; // 상태 변경
}

void AEnemyDogAIController::ChasePlayer()
{
	if (!PlayerPawn || !GetPawn()) return; // 플레이어나 자신이 없다면 함수 종료

	// 현재 월드에 있는 모든 EnemyDog 액터를 배열로 가져옴
	TArray<AActor*> AllEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDog::StaticClass(), AllEnemies);

	if (AllEnemies.Num() <= 1) // 맵에 자신 혼자 있다면
	{
		MoveToActor(PlayerPawn, 10.0f); // 단순하게 플레이어를 향해 직진
		return;
	}

	int32 MyIndex = AllEnemies.IndexOfByKey(GetPawn()); // 전체 적 목록에서 자신의 인덱스(순번)를 찾음
	if (MyIndex == INDEX_NONE) // 만약 자신을 찾지 못했다면 (방어 코드)
	{
		MoveToActor(PlayerPawn, 10.0f); // 단순 직진
		return;
	}

	// 포위 대형을 만들기 위한 각도 계산
	float AngleDeg = 360.0f / AllEnemies.Num(); // 적 전체 수로 360도를 나눔 (적 하나당 차지할 각도)
	float MyAngleDeg = AngleDeg * MyIndex; // 자신의 인덱스를 곱해 고유한 목표 각도를 설정
	float MyAngleRad = FMath::DegreesToRadians(MyAngleDeg); // 각도를 라디안으로 변환

	// 플레이어 위치를 기준으로 포위 목표 지점 계산
	FVector PlayerLocation = PlayerPawn->GetActorLocation(); // 플레이어 위치
	FVector Offset = FVector(FMath::Cos(MyAngleRad), FMath::Sin(MyAngleRad), 0) * SurroundRadius; // 플레이어로부터 SurroundRadius만큼 떨어진 원형 좌표 계산
	FVector TargetLocation = PlayerLocation + Offset; // 플레이어 위치에 오프셋을 더해 최종 목표 지점 설정

	MoveToLocation(TargetLocation, 10.0f); // 계산된 목표 지점으로 이동

	// 디버그용: 목표 지점을 작은 구체로 시각화
	/*DrawDebugSphere(GetWorld(), TargetLocation, 25.0f, 8, FColor::Red, false, 0.05f);*/
}

void AEnemyDogAIController::NormalAttack()
{
	AEnemyDog* EnemyDog = Cast<AEnemyDog>(GetPawn()); // 제어 중인 폰 가져오기
	if (!EnemyDog || EnemyDog->bIsInAirStun) return; // 폰이 없거나 스턴 상태면 공격 불가
	if (bIsAttacking) return; // 이미 공격 중이면 중복 실행 방지

	StopMovement(); // 공격 직전 이동을 멈춤

	if (PlayerPawn) // 플레이어가 유효하다면
	{
		// 플레이어를 정확히 바라보도록 부드럽게 회전
		FRotator CurrentRotation = GetPawn()->GetActorRotation();
		FRotator TargetRotation = (PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation()).Rotation();
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;

		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), 10.0f);
		GetPawn()->SetActorRotation(NewRotation);
	}

	bIsAttacking = true; // 공격 중 상태로 설정
	bCanAttack = false; // 공격 쿨타임 시작

	FTimerHandle AttackDelayTimer;
	GetWorld()->GetTimerManager().SetTimer( // 0.1초 지연 후 공격 애니메이션 재생 (회전할 시간 확보)
		AttackDelayTimer,
		[this, EnemyDog]()
		{
			EnemyDog->PlayNormalAttackAnimation(); // EnemyDog의 공격 애니메이션 재생 함수 호출
		},
		0.1f,
		false
	);

	// 설정된 쿨타임 이후에 ResetAttack 함수를 호출하여 다시 공격 가능 상태로 만듦
	GetWorld()->GetTimerManager().SetTimer(NormalAttackTimerHandle, this, &AEnemyDogAIController::ResetAttack, AttackCooldown, false);
}

void AEnemyDogAIController::ResetAttack()
{
	bCanAttack = true; // 공격 가능 상태로 복귀
	bIsAttacking = false; // 공격 중 상태 해제
}

void AEnemyDogAIController::StopAI()
{
	AEnemyDog* EnemyDogCharacter = Cast<AEnemyDog>(GetPawn());
	if (!EnemyDogCharacter) return; // 폰이 없으면 리턴

	bIsAttacking = false; // 모든 상태 변수 초기화
	bCanAttack = false;
	StopMovement(); // 이동 중지
}