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

	// �׻� �÷��̾� �ٶ󺸱� (����/���� �� ���� ����)
	FVector ToTarget = PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation();
	ToTarget.Z = 0; // ���� ����
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
		// ĳ���� ����
		StopMovement();
		break;

	case EEnemyDogAIState::ChasePlayer:
		ChasePlayer();
		break;
	}

	// ���� ���� �� (������, ������)
	//DrawDebugSphere(GetWorld(), GetPawn()->GetActorLocation(), AttackRange, 32, FColor::Red, false, -1.0f, 0, 2.0f);

	// ���� ���� �Ÿ� �� (�Ķ���, ������)
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
	if (!PlayerPawn || !GetPawn()) return; // �÷��̾� �Ǵ� �� ���� ���̸� �Լ� ����

	// ���� �ʿ� �����ϴ� ��� EnemyDog ���͵��� �迭�� ��������
	TArray<AActor*> AllEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDog::StaticClass(), AllEnemies); // �ڱ� �ڽŰ� �ٸ� ��� ��ü�� ����

	if (AllEnemies.Num() <= 1) // ���� ���� 1�������
	{
		MoveToActor(PlayerPawn, 10.0f); // �÷��̾�� �ܼ��� ����
		return;
	}

	int32 MyIndex = AllEnemies.IndexOfByKey(GetPawn()); // ��ü �� ����Ʈ���� �� ���� �ε������� �� ��°���� ã��
	if (MyIndex == INDEX_NONE) // �� ���� �ڽ��� ���� MyIndex
	{
		MoveToActor(PlayerPawn, 10.0f); // �迭�� �ڽ��� ���ٸ� �ܼ��� ���� (����ڵ�)
		return;
	}

	float AngleDeg = 360.0f / AllEnemies.Num(); // �� ��ü�� ���� 360���� ����
	float MyAngleDeg = AngleDeg * MyIndex; // �� ������ ���� ������ �ο� MyIndex ���� ���� �ڽ��� ������ ����
	float MyAngleRad = FMath::DegreesToRadians(MyAngleDeg); // ���� ������ ��ȯ

	// ���� ��ġ ���
	FVector PlayerLocation = PlayerPawn->GetActorLocation(); // �÷��̾� ��ġ�� ��������
	FVector Offset = FVector(FMath::Cos(MyAngleRad), FMath::Sin(MyAngleRad), 0) * SurroundRadius; // SurroundRadius ��ŭ ������ ���� �˵��� ��ǥ ���� Cos, Sin ������ ���� X,Y ���� ���ϰ� �������� ���� �����°�
	FVector TargetLocation = PlayerLocation + Offset;  // �÷��̾� ��ġ�� �����°��� ���ؼ� ��� ��ǥ�� ���� ��ǥ�� ��ȯ

	MoveToLocation(TargetLocation, 10.0f); // ���� �� �̵�

	// ����� �ð�ȭ (�� �ֺ� ������ Ȯ�ο�)
	DrawDebugSphere(GetWorld(), TargetLocation, 25.0f, 8, FColor::Red, false, 0.05f); 
}

void AEnemyDogAIController::NormalAttack()
{
	AEnemyDog* EnemyDog = Cast<AEnemyDog>(GetPawn());
	if (!EnemyDog || EnemyDog->bIsInAirStun) return;
	if (bIsAttacking) return;

	// ���� ������ �̵� ���߱�
	StopMovement();

	if (PlayerPawn)
	{
		// ���� ȸ������ ��ǥ ȸ���� ���ϱ�
		FRotator CurrentRotation = GetPawn()->GetActorRotation();
		FRotator TargetRotation = (PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation()).Rotation();
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;

		// ȸ���� �ε巴�� �����ϱ� ���� Ÿ�̸� ����
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), 10.0f); // 10.0f�� ȸ�� �ӵ� ���� ��
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
		0.1f,  // ȸ�� �� �ణ �� ��ٸ���(��: 100�и���)
		false
	);

	// ���� ��Ÿ�� Ÿ�̸�
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