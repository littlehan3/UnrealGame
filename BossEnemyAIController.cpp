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

    if (!PlayerPawn)
        PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn || !GetPawn()) return;

    FVector BossLoc = GetPawn()->GetActorLocation();
    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    float DistToPlayer = FVector::Dist(BossLoc, PlayerLoc);

    DrawDebugInfo(); // ����� �ð�ȭ

    // ���� ������Ʈ �� ���� �и�
    UpdateBossAIState(DistToPlayer);

    // ���º� �ൿ
    switch (CurrentState)
    {
    case EBossEnemyAIState::Idle:
        // Idle ���¿����� ���� ���� ����
        break;

    case EBossEnemyAIState::MoveAroundPlayer:
        BossMoveAroundPlayer(DeltaTime);
        break;

    case EBossEnemyAIState::MoveToPlayer:
        BossMoveToPlayer();
        break;

    case EBossEnemyAIState::NormalAttack:
        BossNormalAttack();
        break;
    }
}

void ABossEnemyAIController::UpdateBossAIState(float DistanceToPlayer)
{
    switch (CurrentState)
    {
    case EBossEnemyAIState::Idle:
        // �÷��̾ �ν� ������ ������ MoveAroundPlayer�� ��ȯ
        if (DistanceToPlayer < BossDetectRadius)
            SetBossAIState(EBossEnemyAIState::MoveAroundPlayer);
        break;

    case EBossEnemyAIState::MoveAroundPlayer:
        // MoveAroundPlayer ���¿��� 2�� �� MoveToPlayer�� ��ȯ(������)
        if (BossChasePlayer())
            SetBossAIState(EBossEnemyAIState::MoveToPlayer);
        break;

    case EBossEnemyAIState::MoveToPlayer:
        // �÷��̾ ���� ������ ������ NormalAttack���� ��ȯ
        if (DistanceToPlayer <= BossAttackRange)
            SetBossAIState(EBossEnemyAIState::NormalAttack);
        break;

    case EBossEnemyAIState::NormalAttack:
        // ���� �� BossNormalAttack���� ���� ��ȯ ó��
        break;
    }
}

void ABossEnemyAIController::SetBossAIState(EBossEnemyAIState NewState)
{
    if (CurrentState != NewState)
    {
        StopMovement();
        CurrentState = NewState;

        // ���� ��ȯ�� �ʱ�ȭ
        if (CurrentState == EBossEnemyAIState::MoveAroundPlayer)
        {
            // ���� ���� ����
            CurrentAngle = FMath::FRandRange(0, 2 * PI);
        }
    }
}

void ABossEnemyAIController::BossMoveAroundPlayer(float DeltaTime)
{
    if (!PlayerPawn || !GetPawn()) return;

    // ���� ���� �ð�������� �̵�
    CurrentAngle += DeltaTime * 0.7f;
    if (CurrentAngle > 2 * PI) CurrentAngle -= 2 * PI;

    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    FVector Offset = FVector(FMath::Cos(CurrentAngle), FMath::Sin(CurrentAngle), 0) * BossMoveRadius;
    FVector TargetLoc = PlayerLoc + Offset;

    // �׺� �޽� ���� ��ġ ����
    FNavLocation NavLoc;
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (NavSys && NavSys->ProjectPointToNavigation(TargetLoc, NavLoc, FVector(50, 50, 100)))
    {
        TargetLoc = NavLoc.Location;
    }

    MoveToLocation(TargetLoc, 5.0f);

    // �׻� �÷��̾� �ٶ󺸱�
    FRotator LookAt = (PlayerLoc - GetPawn()->GetActorLocation()).Rotation();
    LookAt.Pitch = 0;
    LookAt.Roll = 0;
    GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAt, DeltaTime, 5.0f));
}

void ABossEnemyAIController::BossMoveToPlayer()
{
    if (!PlayerPawn || !GetPawn()) return;

    MoveToActor(PlayerPawn, 5.0f);

    // �׻� �÷��̾� �ٶ󺸱�
    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    FRotator LookAt = (PlayerLoc - GetPawn()->GetActorLocation()).Rotation();
    LookAt.Pitch = 0;
    LookAt.Roll = 0;
    GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAt, GetWorld()->GetDeltaSeconds(), 5.0f));
}

bool ABossEnemyAIController::BossChasePlayer()
{
    // MoveAroundPlayer ���¿��� 2�� �̻� ������ MoveToPlayer�� ��ȯ
    static float Elapsed = 0.0f;
    Elapsed += GetWorld()->GetDeltaSeconds();
    if (Elapsed > 2.0f)
    {
        Elapsed = 0.0f;
        return true;
    }
    return false;
}

void ABossEnemyAIController::BossNormalAttack()
{
    if (!bCanBossAttack) return;

    // ���� �ִϸ��̼� ���� (���� ������ ���� ĳ������ �Լ� ȣ��)
    UE_LOG(LogTemp, Warning, TEXT("Boss Normal Attacking!"));

    bCanBossAttack = false;

    // ���� �� ���� �ð� ��� �� �ٽ� MoveAroundPlayer�� ��ȯ
    GetWorld()->GetTimerManager().SetTimer(
        BossNormalAttackTimerHandle,
        [this]()
        {
            bCanBossAttack = true;
            SetBossAIState(EBossEnemyAIState::MoveAroundPlayer);
        },
        BossAttackCooldown,
        false
    );
}

void ABossEnemyAIController::DrawDebugInfo()
{
    if (!PlayerPawn || !GetPawn()) return;

    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    FVector BossLoc = GetPawn()->GetActorLocation();

    // �÷��̾� ���� �� ������ ǥ��
    DrawDebugCircle(GetWorld(), PlayerLoc, BossMoveRadius, 64, FColor::Cyan, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);

    // ���� ���� ���� ǥ��
    DrawDebugSphere(GetWorld(), BossLoc, BossAttackRange, 32, FColor::Red, false, -1, 0, 2);

    // ���� �ν� ����(Idle -> MoveAroundPlayer ��ȯ)
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