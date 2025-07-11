#include "BossEnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BossEnemy.h"
#include "BossEnemyAnimInstance.h"

ABossEnemyAIController::ABossEnemyAIController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ABossEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    CurrentState = EBossEnemyAIState::Idle;
}

void ABossEnemyAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!GetPawn() || !PlayerPawn) return;

    float DistToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
    UpdateBossAIState(DistToPlayer);

    DrawDebugInfo(); // ����� �ð�ȭ

    // ���º� �ൿ
    switch (CurrentState)
    {
    case EBossEnemyAIState::MoveToPlayer:
        BossMoveToPlayer();
        break;
    case EBossEnemyAIState::NormalAttack:
        if (bCanBossAttack)
        BossNormalAttack();
        break;
    }
}

void ABossEnemyAIController::SetBossAIState(EBossEnemyAIState NewState)
{
    if (CurrentState != NewState)
    {
        CurrentState = NewState;

        // ���º� ���� �α� ���
        FString LocalStateName;
        switch (CurrentState)
        {
        case EBossEnemyAIState::Idle: LocalStateName = TEXT("Idle"); break;
        case EBossEnemyAIState::MoveToPlayer: LocalStateName = TEXT("MoveToPlayer"); break;
        case EBossEnemyAIState::NormalAttack: LocalStateName = TEXT("NormalAttack"); break;
        }
        UE_LOG(LogTemp, Warning, TEXT("Boss State Changed: %s"), *LocalStateName);

    }
}

void ABossEnemyAIController::UpdateBossAIState(float DistanceToPlayer)
{
    switch (CurrentState)
    {
    case EBossEnemyAIState::Idle:
        if (DistanceToPlayer <= BossDetectRadius)
            SetBossAIState(EBossEnemyAIState::MoveToPlayer);
        break;
    case EBossEnemyAIState::MoveToPlayer:
        // ���� ���� ���� (0-200) �Ǵ� �̵� ���� ���� (201-250)�� ����
        if (DistanceToPlayer <= BossStandingAttackRange ||
            (DistanceToPlayer > BossStandingAttackRange && DistanceToPlayer <= BossMovingAttackRange))
        {
            SetBossAIState(EBossEnemyAIState::NormalAttack);
        }
        break;
    case EBossEnemyAIState::NormalAttack:
        if (DistanceToPlayer > BossMovingAttackRange)
            SetBossAIState(EBossEnemyAIState::MoveToPlayer);
        break;
    }
}


void ABossEnemyAIController::BossMoveToPlayer()
{
    if (!PlayerPawn || !GetPawn()) return;
    MoveToActor(PlayerPawn, 5.0f);
    LookAtPlayer(GetWorld()->GetDeltaSeconds());
}

void ABossEnemyAIController::BossNormalAttack()
{
    if (!bCanBossAttack) return;

    float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());

    LookAtPlayer(GetWorld()->GetDeltaSeconds());

    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn());
    if (Boss && Boss->bCanBossAttack)
    {
        if (DistanceToPlayer <= BossStandingAttackRange) // 0-200 ����
        {
            // ���� ���� - ���� �ִϸ��̼�
            Boss->PlayBossNormalAttackAnimation();
            StopMovement(); // �̵� ����
            UE_LOG(LogTemp, Warning, TEXT("Standing Attack - Distance: %f"), DistanceToPlayer);
        }
        else if (DistanceToPlayer > BossStandingAttackRange && DistanceToPlayer <= BossMovingAttackRange) // 201-250 ����
        {
            // �̵� ���� - ��ü�� �ִϸ��̼�
            Boss->PlayBossUpperBodyAttack();
            UE_LOG(LogTemp, Warning, TEXT("Moving Attack - Distance: %f"), DistanceToPlayer);
        }
        else
        {
            // ������ ��� ��� �������� ����
            UE_LOG(LogTemp, Warning, TEXT("Out of attack range - Distance: %f"), DistanceToPlayer);
            return; // �������� �����Ƿ� bCanBossAttack�� false�� �������� ����
        }
        bCanBossAttack = false;
    }
}

void ABossEnemyAIController::OnBossNormalAttackMontageEnded()
{
    bCanBossAttack = true;
}


void ABossEnemyAIController::LookAtPlayer(float DeltaTime)
{
    if (!PlayerPawn || !GetPawn()) return;
    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    FRotator LookAt = (PlayerLoc - GetPawn()->GetActorLocation()).Rotation();
    LookAt.Pitch = 0;
    LookAt.Roll = 0;
    GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAt, DeltaTime, 5.0f));
}


void ABossEnemyAIController::DrawDebugInfo()
{
    if (!PlayerPawn || !GetPawn()) return;

    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    FVector BossLoc = GetPawn()->GetActorLocation();

    // �÷��̾� ���� �� ������ ǥ��
    DrawDebugCircle(GetWorld(), PlayerLoc, BossMoveRadius, 64, FColor::Cyan, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);

    // ���� ���� ���� ǥ��
    // �ܰ� �� (250)
    DrawDebugCircle(GetWorld(), BossLoc, BossMovingAttackRange, 64, FColor::Orange, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);
    // ���� �� (200) 
    DrawDebugCircle(GetWorld(), BossLoc, BossStandingAttackRange, 64, FColor::Yellow, false, -1, 0, 1, FVector(1, 0, 0), FVector(0, 1, 0), false);

    // ���� �ν� ����
    DrawDebugSphere(GetWorld(), BossLoc, BossDetectRadius, 32, FColor::Green, false, -1, 0, 1);
}

void ABossEnemyAIController::StopBossAI()
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    // �̵� ����
    StopMovement();
    UE_LOG(LogTemp, Warning, TEXT("%s Boss AI Movement Stopped"), *ControlledPawn->GetName());

    // AI ��Ʈ�ѷ����� �� �и�
    UnPossess();

    // ĳ���� �浹 �� ƽ ��Ȱ��ȭ
    ControlledPawn->SetActorEnableCollision(false);
    ControlledPawn->SetActorTickEnabled(false);
}