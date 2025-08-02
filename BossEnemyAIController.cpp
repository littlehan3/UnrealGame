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

    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn());
    if (!Boss) return;

    // ���� �ִϸ��̼� ��� ���̸� AI �ൿ ����
    if (Boss->bIsPlayingBossIntro)
    {
        // ���� �߿��� NormalAttack ���¸� �����ϵ� �ൿ�� ���� ����
        if (CurrentState != EBossEnemyAIState::NormalAttack)
        {
            SetBossAIState(EBossEnemyAIState::NormalAttack);
        }
        DrawDebugInfo();
        return;
    }

    // ���ڽ� ���� ���̸� AI �ൿ ���� ����
    if (IsExecutingStealthAttack() || bIsAIDisabledForStealth)
    {
        if (CurrentState != EBossEnemyAIState::NormalAttack)
        {
            SetBossAIState(EBossEnemyAIState::NormalAttack);
        }
        DrawDebugInfo();
        return; // �ٸ� �ൿ�� �������� ����
    }

    // **���ڽ� ���� �� Idle ���¿� ���� ��� ������ MoveToPlayer ���·� ��ȯ**
    if (CurrentState == EBossEnemyAIState::Idle && !Boss->bIsPlayingBossIntro)
    {
        float DistToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
        if (DistToPlayer <= BossDetectRadius)
        {
            UE_LOG(LogTemp, Warning, TEXT("Force transitioning from Idle to MoveToPlayer"));
            SetBossAIState(EBossEnemyAIState::MoveToPlayer);
        }
    }

    // **���ڽ� ���� �� AI ���� ���� ������ġ**
    if (!bIsAIDisabledForStealth && !IsExecutingStealthAttack() &&
        !Boss->bIsFullBodyAttacking && !Boss->bIsPlayingBossIntro)
    {
        // ���ڽ��� ������ �����µ� Idle ���¿� ���� ���
        if (CurrentState == EBossEnemyAIState::Idle)
        {
            float DistToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
            if (DistToPlayer <= BossDetectRadius)
            {
                UE_LOG(LogTemp, Warning, TEXT("Force recovering from post-stealth Idle state"));
                SetBossAIState(EBossEnemyAIState::MoveToPlayer);
                SetFocus(PlayerPawn); // �÷��̾� ��Ŀ�� �缳��
            }
        }
    }

    // ���Ű��� ���̰ų� ��� ������ �ڷ���Ʈ ���� ���� ���� ������Ʈ�� ����
    if (Boss->bIsFullBodyAttacking || Boss->bIsBossTeleporting ||
        Boss->bIsBossAttackTeleporting || Boss->bIsBossRangedAttacking)
    {
        // �ش� ���µ� �߿��� NormalAttack ���¸� ����
        if (CurrentState != EBossEnemyAIState::NormalAttack)
        {
            SetBossAIState(EBossEnemyAIState::NormalAttack);
        }
        DrawDebugInfo();
        return; // �ٸ� �ൿ�� �������� ����
    }

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

    // ���Ű��� ���� ���� �̵����� ����
    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn());
    if (Boss && Boss->bIsFullBodyAttacking)
    {
        StopMovement();
        UE_LOG(LogTemp, Warning, TEXT("Cannot move during full body attack"));
        return;
    }

    MoveToActor(PlayerPawn, 5.0f);
}

void ABossEnemyAIController::BossNormalAttack()
{
    if (!bCanBossAttack) return; // ���� ���� ���°� �ƴϸ� ����

    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn()); // ���� ��ü�� ĳ��Ʈ�ؼ� ������
    if (!Boss) return; // ������ ������ ����

    // �ǰ� ���̰ų� ����� ��� �Ǵ� ���Ű��� ���� ��� �Ǵ� �ڷ���Ʈ ���� ��� �������� ����
    if (Boss->bIsBossHit || Boss->bIsBossDead || Boss->bIsFullBodyAttacking ||
        Boss->bIsBossTeleporting || Boss->bIsBossAttackTeleporting || Boss->bIsBossRangedAttacking) return;

    float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation()); // �÷��̾�� ���� ���� �Ÿ� ���
    float CurrentTime = GetWorld()->GetTimeSeconds();

    UE_LOG(LogTemp, Warning, TEXT("Boss choosing attack - Distance: %f"), DistanceToPlayer); // ����� �α�: ���� �Ÿ� ���

    // �ֿ켱����: ���ڽ� ���� üũ�� ��� �Ÿ� ���� ������ ����
    if (CanExecuteStealthAttack()) // ���ڽ� ���� ������ �����ϴ��� üũ
    {
        UE_LOG(LogTemp, Warning, TEXT("Stealth conditions met - rolling dice")); // ����� �α�: ���ڽ� ���� ������
        float StealthChance = FMath::RandRange(0.0f, 1.0f); // 0.0~1.0 ������ ���� �� ����
        if (StealthChance <= 0.5f) // 50% Ȯ���� ���ڽ� ���� ����
        {
            Boss->PlayBossStealthAttackAnimation(); // ���ڽ� ���� �ִϸ��̼� ���
            UE_LOG(LogTemp, Warning, TEXT("Boss executing STEALTH ATTACK!")); // ����� �α�: ���ڽ� ���� �����
            return; // ���ڽ� ������ ���õǸ� �ٸ� ������ �������� ����
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Stealth dice roll failed - continuing with normal attacks")); // ����� �α�: ���ڽ� Ȯ�� ����, �Ϲ� �������� ����
        }
    }

    // �� ������ ���� �Ÿ��� ���� ���� ����
    if (Boss && Boss->bCanBossAttack) // ������ �����ϰ� ���� ���� �������� üũ
    {
        if (DistanceToPlayer <= BossStandingAttackRange) // 0-200 ����: �ٰŸ� ���� ����
        {
            bool bShouldTeleport = FMath::RandBool(); // 50% Ȯ���� true/false ����
            if (bShouldTeleport && Boss->bCanTeleport) // �ڷ���Ʈ�� ���õǰ� �ڷ���Ʈ ���� ������ ���
            {
                // 50% Ȯ�� - �־����� �ڷ���Ʈ ����
                Boss->PlayBossTeleportAnimation(); // ���� �ڷ���Ʈ �ִϸ��̼� ���
                UE_LOG(LogTemp, Warning, TEXT("Boss chose retreat teleport - Distance: %f"), DistanceToPlayer); // ����� �α�: ���� �ڷ���Ʈ ���õ�
            }
            else
            {
                // 50% Ȯ�� - ���� ���� ����
                Boss->PlayBossNormalAttackAnimation(); // �Ϲ� ���� �ִϸ��̼� ���
                StopMovement(); // AI �̵� ���� (���� �߿��� �������� ����)
                UE_LOG(LogTemp, Warning, TEXT("Boss chose standing attack - Distance: %f"), DistanceToPlayer); // ����� �α�: �ٰŸ� ���� ���õ�
            }
        }
        else if (DistanceToPlayer > BossStandingAttackRange && DistanceToPlayer <= BossMovingAttackRange) // 201-250 ����: �߰Ÿ� ���� ����
        {
            bool bDoRangedAttack = false;
            // 1. ���Ÿ� ��Ÿ�� �� ����(���ڷ���Ʈ �� 1ȸ ���) üũ
            if (bIgnoreRangedCooldownOnce ||
                ((CurrentTime - LastRangedAttackTime) >= RangedAttackCooldown))
            {
                bDoRangedAttack = FMath::RandBool(); // 50% Ȯ�� (���� ����)
            }

            // 2. ���� ���� ����
            if (bDoRangedAttack)
            {
                Boss->PlayBossRangedAttackAnimation();
                LastRangedAttackTime = CurrentTime;
                if (bIgnoreRangedCooldownOnce) bIgnoreRangedCooldownOnce = false; // �� ���� ����
                UE_LOG(LogTemp, Warning, TEXT("Moving Ranged Attack - CD OK - Distance: %f"), DistanceToPlayer);
            }
            else
            {
                Boss->PlayBossUpperBodyAttackAnimation();
                UE_LOG(LogTemp, Warning, TEXT("Moving UpperBody Attack - or RangedAttack on CD - Distance: %f"), DistanceToPlayer);
            }
        }
        else
        {
            // ���� ������ ��� ��� (251 �̻�) �������� ����
            UE_LOG(LogTemp, Warning, TEXT("Out of attack range - Distance: %f"), DistanceToPlayer); // ����� �α�: ���� ���� ���
            return; // �������� �����Ƿ� bCanBossAttack�� false�� �������� �ʰ� ����
        }

        // ���ڽ� ������ �ƴ� �Ϲ� ������ ����� ��쿡�� ���� �Ұ� ���·� ����
        if (!Boss->bIsBossTeleporting && !Boss->bIsBossAttackTeleporting && !IsExecutingStealthAttack())
        {
            bCanBossAttack = false; // ���� �� ��Ÿ���� ���� ���� �Ұ� ���·� ����
        }
    }
}


void ABossEnemyAIController::OnBossNormalAttackMontageEnded()
{
    bCanBossAttack = true;
}

void ABossEnemyAIController::OnBossAttackTeleportEnded()
{
    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn());
    if (!Boss)
    {
        bCanBossAttack = true;
        return;
    }

    if (Boss->bShouldUseRangedAfterTeleport)
    {
        bIgnoreRangedCooldownOnce = true; // ���� ����ü ���ݸ� ��Ÿ�� ����
        Boss->PlayBossRangedAttackAnimation();
        LastRangedAttackTime = GetWorld()->GetTimeSeconds(); // CD ����
        Boss->bShouldUseRangedAfterTeleport = false;
    }
    else 
    {
        bCanBossAttack = true;
    }
}

void ABossEnemyAIController::OnBossRangedAttackEnded()
{
    UE_LOG(LogTemp, Warning, TEXT("Ranged attack ended - resuming chase logic"));
    bCanBossAttack = true; // ��� �߰� �簳
}

bool ABossEnemyAIController::CanExecuteStealthAttack() const
{
    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn());
    if (!Boss) return false;

    // ���ڽ� ���� ���� ���� üũ (BossEnemy�� �ִ� ����)
    if (!Boss->bCanUseStealthAttack) return false;

    // �ٸ� Ư�� ������ ���� ������ üũ (BossEnemy�� �ִ� ������)
    if (Boss->bIsBossRangedAttacking || Boss->bIsBossTeleporting ||
        Boss->bIsBossAttackTeleporting) return false;

    // �Ÿ� ���� üũ
    if (!IsInOptimalStealthRange()) return false;

    return true;
}

bool ABossEnemyAIController::IsInOptimalStealthRange() const
{
    APawn* TargetPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);  // ������ ����
    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn());

    if (!TargetPlayer || !Boss) return false;

    float Distance = FVector::Dist(Boss->GetActorLocation(), TargetPlayer->GetActorLocation());

    // ���ڽ� ���� ���� �Ÿ�: 300-600
    return (Distance >= StealthAttackMinRange && Distance <= StealthAttackOptimalRange);
}


bool ABossEnemyAIController::IsExecutingStealthAttack() const
{
    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn());
    if (!Boss) return false;

    // BossEnemy�� ������ ����� ���ڽ� �����鸸 üũ
    return (Boss->bIsStealthStarting ||
        Boss->bIsStealthDiving ||
        Boss->bIsStealthInvisible ||
        Boss->bIsStealthKicking ||
        Boss->bIsStealthFinishing);
}

void ABossEnemyAIController::HandleStealthPhaseTransition(int32 NewPhase)
{
    switch (NewPhase)
    {
    case 1: // ���ڽ� ����
        bIsAIDisabledForStealth = true;
        bCanBossAttack = false;  // **AI ���ݵ� ��Ȱ��ȭ**
        StopMovement();
        UE_LOG(LogTemp, Warning, TEXT("AI Disabled for Stealth Attack"));
        break;

    case 3: // ����ȭ �ܰ�
        SetFocus(nullptr);
        StopMovement();
        UE_LOG(LogTemp, Warning, TEXT("Stealth invisible - AI tracking disabled"));
        break;

    case 5: // ű ����
        if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
        {
            SetFocus(Player);
        }
        UE_LOG(LogTemp, Warning, TEXT("Stealth kick phase - refocusing player"));
        break;

    case 6: // �ǴϽ� ���� �ܰ�
        StopMovement();
        UE_LOG(LogTemp, Warning, TEXT("Stealth finish phase - AI disabled"));
        break;

    case 0: // ���ڽ� ���� AI ���� ����
        bIsAIDisabledForStealth = false;
        bCanBossAttack = true;  // AI ��Ʈ�ѷ� ���� ���� ����

        // �÷��̾� �ٽ� Ÿ����
        if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
        {
            SetFocus(Player);
        }

        // AI ���¸� ������ MoveToPlayer�� ����
        SetBossAIState(EBossEnemyAIState::MoveToPlayer);

        UE_LOG(LogTemp, Warning, TEXT("AI Fully Enabled after Stealth Attack - Attack capability restored"));
        break;
    }
}

void ABossEnemyAIController::DrawDebugInfo()
{
    if (!PlayerPawn || !GetPawn()) return;

    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    FVector BossLoc = GetPawn()->GetActorLocation();

    // �÷��̾� ���� �� ������ ǥ��
    DrawDebugCircle(GetWorld(), PlayerLoc, BossMoveRadius, 64, FColor::Cyan, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);

    // ���� ���� ���� ǥ��
 
    DrawDebugCircle(GetWorld(), BossLoc, BossMovingAttackRange, 64, FColor::Orange, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);

    DrawDebugCircle(GetWorld(), BossLoc, BossStandingAttackRange, 64, FColor::Yellow, false, -1, 0, 1, FVector(1, 0, 0), FVector(0, 1, 0), false);

    DrawDebugCircle(GetWorld(), BossLoc, StealthAttackMinRange, 64, FColor::Green, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);

    DrawDebugCircle(GetWorld(), BossLoc, StealthAttackOptimalRange, 64, FColor::Blue, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);
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