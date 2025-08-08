#include "EnemyDogAIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFrameWork/Character.h"
#include "EnemyDog.h"
#include "EnemyDogAnimInstance.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	if (!PlayerPawn)
	{
		PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (!PlayerPawn) return;
	}

	float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
	UpdateAIState(DistanceToPlayer);

	switch (CurrentState)
	{
	case EEnemyDogAIState::Idle:
		// ĳ���� ����
		StopMovement();
		break;

	case EEnemyDogAIState::ChasePlayer:
		ChasePlayer();
		break;
	}
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
	if (!PlayerPawn || !GetPawn()) return;

	// �÷��̾� �̵� ��ǥ
	MoveToActor(PlayerPawn, 50.0f); // ��ǥ ��ġ ��ó���� �̵�
}

void AEnemyDogAIController::NormalAttack()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	AEnemyDog* EnemyDog = Cast<AEnemyDog>(ControlledPawn);
	if (EnemyDog && EnemyDog->bCanAttack)
	{
		bIsAttacking = true;
		bCanAttack = false;

		// EnemyDog�� ���� �Լ� ȣ��
		EnemyDog->StartAttack();

		// ���� ��ٿ� ����
		GetWorld()->GetTimerManager().SetTimer(NormalAttackTimerHandle, this, &AEnemyDogAIController::ResetAttack, AttackCooldown, false);
	}
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