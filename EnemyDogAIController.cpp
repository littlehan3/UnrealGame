#include "EnemyDogAIController.h"
#include "Kismet/GameplayStatics.h" // UGameplayStatics �Լ� ���
#include "NavigationSystem.h" // �׺���̼� �ý��� ���
#include "GameFrameWork/Character.h" // ACharacter Ŭ���� ����
#include "EnemyDog.h" // ������ EnemyDog Ŭ���� ����
#include "EnemyDogAnimInstance.h" // �ִ� �ν��Ͻ� Ŭ���� ����
#include "TimerManager.h" // FTimerManager ���
#include "GameFramework/CharacterMovementComponent.h" // ĳ���� �����Ʈ ������Ʈ ����
#include "DrawDebugHelpers.h" // ����� �ð�ȭ ��� ���

AEnemyDogAIController::AEnemyDogAIController()
{
	PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ� ȣ�� ����
	SetActorTickInterval(0.05f); // Tick �ֱ� 0.05�ʷ� ����ȭ

	RotationUpdateTimer = 0.0f; // ���� �ʱ�ȭ
	StaticAngleOffset = 0; // ���� �ʱ�ȭ
}

void AEnemyDogAIController::BeginPlay()
{
	Super::BeginPlay(); // �θ� Ŭ���� BeginPlay ȣ��
	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // ���� ���� �� �÷��̾� ���� ã�� ����
}

void AEnemyDogAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // �θ� Ŭ���� Tick ȣ��

	AEnemyDog* EnemyDogCharacter = Cast<AEnemyDog>(GetPawn()); // ���� ��Ʈ�ѷ��� ������ ���� EnemyDog���� ĳ����
	if (!EnemyDogCharacter || EnemyDogCharacter->bIsDead) // ���� ���ų� �׾��ٸ�
	{
		StopMovement(); // �̵� ����
		return; // Tick �Լ� ����
	}

	if (EnemyDogCharacter->bIsPlayingIntro) // ���� �ִϸ��̼� ���̶��
	{
		StopMovement(); // �̵� ����
		return;
	}

	if (EnemyDogCharacter->bIsInAirStun) // ���� ���� ���¶��
	{
		StopMovement(); // �̵� ����
		return;
	}

	if (!PlayerPawn) // �÷��̾� �� ������ ���ٸ�
	{
		PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // �ٽ� ã�ƺ�
		if (!PlayerPawn) return; // �׷��� ������ Tick �Լ� ����
	}

	// AI�� �׻� �÷��̾ �ٶ󺸵��� ����
	FVector ToTarget = PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation(); // �ڽſ��Լ� �÷��̾�� ���ϴ� ���� ����
	ToTarget.Z = 0; // ���Ʒ�(Z��)�� �����Ͽ� �������θ� �ٶ󺸰� ��
	if (!ToTarget.IsNearlyZero())
	{
		FRotator TargetRot = ToTarget.Rotation(); // ���� ���͸� ȸ����(Rotator)���� ��ȯ
		GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), TargetRot, DeltaTime, 10.0f)); // �ε巴�� ȸ��
	}

	float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation()); // �÷��̾���� �Ÿ� ���
	UpdateAIState(DistanceToPlayer); // �Ÿ��� ���� AI ���� ����

	// ���� AI ���¿� ���� �ൿ ����
	switch (CurrentState)
	{
	case EEnemyDogAIState::Idle: // ��� ���¶��
		StopMovement(); // �̵� ����
		break;

	case EEnemyDogAIState::ChasePlayer: // ���� ���¶��
		ChasePlayer(); // ���� �Լ� ȣ��
		break;
	}

	// ����׿�: ���� ���� �� ���� ���� �Ÿ� �ð�ȭ
	//DrawDebugSphere(GetWorld(), GetPawn()->GetActorLocation(), AttackRange, 32, FColor::Red, false, -1.0f, 0, 2.0f);
	//DrawDebugSphere(GetWorld(), GetPawn()->GetActorLocation(), ChaseStartDistance, 32, FColor::Blue, false, -1.0f, 0, 1.0f);
}

void AEnemyDogAIController::UpdateAIState(float DistanceToPlayer)
{
	if (DistanceToPlayer <= AttackRange && bCanAttack) // ���� ���� ���� �ְ� ������ �����ϴٸ�
	{
		SetAIState(EEnemyDogAIState::Idle); // ��� ���·� ����
		NormalAttack(); // ���� ����
	}
	else if (DistanceToPlayer <= ChaseStartDistance) // ���� ���� �Ÿ� ���� �ִٸ�
	{
		SetAIState(EEnemyDogAIState::ChasePlayer); // ���� ���·� ����
	}
	else // �ʹ� �ָ� ������ �ִٸ�
	{
		SetAIState(EEnemyDogAIState::Idle); // ��� ���·� ����
	}
}

void AEnemyDogAIController::SetAIState(EEnemyDogAIState NewState)
{
	if (CurrentState == NewState) return; // ���� ���¿� ���ٸ� �������� ����
	CurrentState = NewState; // ���� ����
}

void AEnemyDogAIController::ChasePlayer()
{
	if (!PlayerPawn || !GetPawn()) return; // �÷��̾ �ڽ��� ���ٸ� �Լ� ����

	// ���� ���忡 �ִ� ��� EnemyDog ���͸� �迭�� ������
	TArray<AActor*> AllEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDog::StaticClass(), AllEnemies);

	if (AllEnemies.Num() <= 1) // �ʿ� �ڽ� ȥ�� �ִٸ�
	{
		MoveToActor(PlayerPawn, 10.0f); // �ܼ��ϰ� �÷��̾ ���� ����
		return;
	}

	int32 MyIndex = AllEnemies.IndexOfByKey(GetPawn()); // ��ü �� ��Ͽ��� �ڽ��� �ε���(����)�� ã��
	if (MyIndex == INDEX_NONE) // ���� �ڽ��� ã�� ���ߴٸ� (��� �ڵ�)
	{
		MoveToActor(PlayerPawn, 10.0f); // �ܼ� ����
		return;
	}

	// ���� ������ ����� ���� ���� ���
	float AngleDeg = 360.0f / AllEnemies.Num(); // �� ��ü ���� 360���� ���� (�� �ϳ��� ������ ����)
	float MyAngleDeg = AngleDeg * MyIndex; // �ڽ��� �ε����� ���� ������ ��ǥ ������ ����
	float MyAngleRad = FMath::DegreesToRadians(MyAngleDeg); // ������ �������� ��ȯ

	// �÷��̾� ��ġ�� �������� ���� ��ǥ ���� ���
	FVector PlayerLocation = PlayerPawn->GetActorLocation(); // �÷��̾� ��ġ
	FVector Offset = FVector(FMath::Cos(MyAngleRad), FMath::Sin(MyAngleRad), 0) * SurroundRadius; // �÷��̾�κ��� SurroundRadius��ŭ ������ ���� ��ǥ ���
	FVector TargetLocation = PlayerLocation + Offset; // �÷��̾� ��ġ�� �������� ���� ���� ��ǥ ���� ����

	MoveToLocation(TargetLocation, 10.0f); // ���� ��ǥ �������� �̵�

	// ����׿�: ��ǥ ������ ���� ��ü�� �ð�ȭ
	DrawDebugSphere(GetWorld(), TargetLocation, 25.0f, 8, FColor::Red, false, 0.05f);
}

void AEnemyDogAIController::NormalAttack()
{
	AEnemyDog* EnemyDog = Cast<AEnemyDog>(GetPawn()); // ���� ���� �� ��������
	if (!EnemyDog || EnemyDog->bIsInAirStun) return; // ���� ���ų� ���� ���¸� ���� �Ұ�
	if (bIsAttacking) return; // �̹� ���� ���̸� �ߺ� ���� ����

	StopMovement(); // ���� ���� �̵��� ����

	if (PlayerPawn) // �÷��̾ ��ȿ�ϴٸ�
	{
		// �÷��̾ ��Ȯ�� �ٶ󺸵��� �ε巴�� ȸ��
		FRotator CurrentRotation = GetPawn()->GetActorRotation();
		FRotator TargetRotation = (PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation()).Rotation();
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;

		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), 10.0f);
		GetPawn()->SetActorRotation(NewRotation);
	}

	bIsAttacking = true; // ���� �� ���·� ����
	bCanAttack = false; // ���� ��Ÿ�� ����

	FTimerHandle AttackDelayTimer;
	GetWorld()->GetTimerManager().SetTimer( // 0.1�� ���� �� ���� �ִϸ��̼� ��� (ȸ���� �ð� Ȯ��)
		AttackDelayTimer,
		[this, EnemyDog]()
		{
			EnemyDog->PlayNormalAttackAnimation(); // EnemyDog�� ���� �ִϸ��̼� ��� �Լ� ȣ��
		},
		0.1f,
		false
	);

	// ������ ��Ÿ�� ���Ŀ� ResetAttack �Լ��� ȣ���Ͽ� �ٽ� ���� ���� ���·� ����
	GetWorld()->GetTimerManager().SetTimer(NormalAttackTimerHandle, this, &AEnemyDogAIController::ResetAttack, AttackCooldown, false);
}

void AEnemyDogAIController::ResetAttack()
{
	bCanAttack = true; // ���� ���� ���·� ����
	bIsAttacking = false; // ���� �� ���� ����
}

void AEnemyDogAIController::StopAI()
{
	AEnemyDog* EnemyDogCharacter = Cast<AEnemyDog>(GetPawn());
	if (!EnemyDogCharacter) return; // ���� ������ ����

	bIsAttacking = false; // ��� ���� ���� �ʱ�ȭ
	bCanAttack = false;
	StopMovement(); // �̵� ����
}