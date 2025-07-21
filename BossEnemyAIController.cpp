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

    // 등장 애니메이션 재생 중이면 AI 행동 중지
    if (Boss->bIsPlayingBossIntro)
    {
        // 등장 중에는 NormalAttack 상태를 유지하되 행동은 하지 않음
        if (CurrentState != EBossEnemyAIState::NormalAttack)
        {
            SetBossAIState(EBossEnemyAIState::NormalAttack);
        }
        DrawDebugInfo();
        return;
    }

    // 전신공격 중이거나 모든 종류의 텔레포트 중일 때는 상태 업데이트를 제한
    if (Boss->bIsFullBodyAttacking || Boss->bIsBossTeleporting ||
        Boss->bIsBossAttackTeleporting || Boss->bIsBossRangedAttacking)
    {
        // 해당 상태들 중에는 NormalAttack 상태를 유지
        if (CurrentState != EBossEnemyAIState::NormalAttack)
        {
            SetBossAIState(EBossEnemyAIState::NormalAttack);
        }
        DrawDebugInfo();
        return; // 다른 행동은 수행하지 않음
    }

    float DistToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
    UpdateBossAIState(DistToPlayer);

    DrawDebugInfo(); // 디버그 시각화

    // 상태별 행동
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

        // 상태별 진입 로그 출력
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
        // 정지 공격 범위 (0-200) 또는 이동 공격 범위 (201-250)에 진입
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

    // 전신공격 중일 때는 이동하지 않음
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
    if (!bCanBossAttack) return;

    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn());
    if (!Boss) return;

    // 피격 중이거나 사망한 경우 또는 전신공격 중인 경우 또는 텔레포트 중인 경우 공격하지 않음
    if (Boss->bIsBossHit || Boss->bIsBossDead || Boss->bIsFullBodyAttacking ||
        Boss->bIsBossTeleporting || Boss->bIsBossAttackTeleporting || Boss->bIsBossRangedAttacking) return;

    float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());

    if (Boss && Boss->bCanBossAttack)
    {
        if (DistanceToPlayer <= BossStandingAttackRange) // 0-200 범위
        {
            // **2가지 선택지만**: 전신공격 OR 멀어지는 텔레포트
            bool bShouldTeleport = FMath::RandBool(); // 50% 확률

            if (bShouldTeleport && Boss->bCanTeleport)
            {
                // 50% 확률 - 멀어지는 텔레포트 (텔레포트 후 정지하여 추가 선택 진행)
                Boss->PlayBossTeleportAnimation();
                UE_LOG(LogTemp, Warning, TEXT("Boss chose retreat teleport - will decide next action after pause - Distance: %f"), DistanceToPlayer);
            }
            else
            {
                // 50% 확률 - 전신 공격
                Boss->PlayBossNormalAttackAnimation();
                StopMovement(); // 이동 중지
                UE_LOG(LogTemp, Warning, TEXT("Boss chose standing attack - Distance: %f"), DistanceToPlayer);
            }
        }
        else if (DistanceToPlayer > BossStandingAttackRange && DistanceToPlayer <= BossMovingAttackRange) // 201-250 범위
        {
            // 이동 공격 - 상체만 애니메이션, 이동가능
            Boss->PlayBossUpperBodyAttackAnimation();
            UE_LOG(LogTemp, Warning, TEXT("Moving Attack - Distance: %f"), DistanceToPlayer);
        }
        else
        {
            // 범위를 벗어난 경우 공격하지 않음
            UE_LOG(LogTemp, Warning, TEXT("Out of attack range - Distance: %f"), DistanceToPlayer);
            return; // 공격하지 않으므로 bCanBossAttack을 false로 설정하지 않음
        }

        if (!Boss->bIsBossTeleporting && !Boss->bIsBossAttackTeleporting) // 텔레포트가 아닌 경우에만
        {
            bCanBossAttack = false;
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

    // 텔레포트 후 추가 행동 결정
    if (Boss->bShouldUseRangedAfterTeleport)
    {
        UE_LOG(LogTemp, Warning, TEXT("Executing ranged attack after teleport"));
        Boss->PlayBossRangedAttackAnimation();
        Boss->bShouldUseRangedAfterTeleport = false; // 플래그 리셋
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Resuming normal combat after attack teleport"));
        bCanBossAttack = true;
    }
}

void ABossEnemyAIController::OnBossRangedAttackEnded()
{
    UE_LOG(LogTemp, Warning, TEXT("Ranged attack ended - resuming chase logic"));
    bCanBossAttack = true; // 즉시 추격 재개
}

void ABossEnemyAIController::DrawDebugInfo()
{
    if (!PlayerPawn || !GetPawn()) return;

    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    FVector BossLoc = GetPawn()->GetActorLocation();

    // 플레이어 기준 원 반지름 표시
    DrawDebugCircle(GetWorld(), PlayerLoc, BossMoveRadius, 64, FColor::Cyan, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);

    // 보스 공격 범위 표시
    // 외곽 원 (250)
    DrawDebugCircle(GetWorld(), BossLoc, BossMovingAttackRange, 64, FColor::Orange, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);
    // 내부 원 (200) 
    DrawDebugCircle(GetWorld(), BossLoc, BossStandingAttackRange, 64, FColor::Yellow, false, -1, 0, 1, FVector(1, 0, 0), FVector(0, 1, 0), false);

    // 보스 인식 범위
    DrawDebugSphere(GetWorld(), BossLoc, BossDetectRadius, 32, FColor::Green, false, -1, 0, 1);
}

void ABossEnemyAIController::StopBossAI()
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    // 이동 중지
    StopMovement();
    UE_LOG(LogTemp, Warning, TEXT("%s Boss AI Movement Stopped"), *ControlledPawn->GetName());

    // AI 컨트롤러에서 폰 분리
    UnPossess();

    // 캐릭터 충돌 및 틱 비활성화
    ControlledPawn->SetActorEnableCollision(false);
    ControlledPawn->SetActorTickEnabled(false);
}
