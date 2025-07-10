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

    DrawDebugInfo(); // 디버그 시각화

    // 상태 업데이트 및 동작 분리
    UpdateBossAIState(DistToPlayer);

    // 상태별 행동
    switch (CurrentState)
    {
    case EBossEnemyAIState::Idle:
        // Idle 상태에서는 별도 동작 없음
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
        // 플레이어가 인식 범위에 들어오면 MoveAroundPlayer로 전환
        if (DistanceToPlayer < BossDetectRadius)
            SetBossAIState(EBossEnemyAIState::MoveAroundPlayer);
        break;

    case EBossEnemyAIState::MoveAroundPlayer:
        // MoveAroundPlayer 상태에서 2초 후 MoveToPlayer로 전환(간보기)
        if (BossChasePlayer())
            SetBossAIState(EBossEnemyAIState::MoveToPlayer);
        break;

    case EBossEnemyAIState::MoveToPlayer:
        // 플레이어가 공격 범위에 들어오면 NormalAttack으로 전환
        if (DistanceToPlayer <= BossAttackRange)
            SetBossAIState(EBossEnemyAIState::NormalAttack);
        break;

    case EBossEnemyAIState::NormalAttack:
        // 공격 후 BossNormalAttack에서 상태 전환 처리
        break;
    }
}

void ABossEnemyAIController::SetBossAIState(EBossEnemyAIState NewState)
{
    if (CurrentState != NewState)
    {
        StopMovement();
        CurrentState = NewState;

        // 상태 전환시 초기화
        if (CurrentState == EBossEnemyAIState::MoveAroundPlayer)
        {
            // 각도 랜덤 시작
            CurrentAngle = FMath::FRandRange(0, 2 * PI);
        }
    }
}

void ABossEnemyAIController::BossMoveAroundPlayer(float DeltaTime)
{
    if (!PlayerPawn || !GetPawn()) return;

    // 각도 증가 시계방향으로 이동
    CurrentAngle += DeltaTime * 0.7f;
    if (CurrentAngle > 2 * PI) CurrentAngle -= 2 * PI;

    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    FVector Offset = FVector(FMath::Cos(CurrentAngle), FMath::Sin(CurrentAngle), 0) * BossMoveRadius;
    FVector TargetLoc = PlayerLoc + Offset;

    // 네비 메시 위로 위치 보정
    FNavLocation NavLoc;
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (NavSys && NavSys->ProjectPointToNavigation(TargetLoc, NavLoc, FVector(50, 50, 100)))
    {
        TargetLoc = NavLoc.Location;
    }

    MoveToLocation(TargetLoc, 5.0f);

    // 항상 플레이어 바라보기
    FRotator LookAt = (PlayerLoc - GetPawn()->GetActorLocation()).Rotation();
    LookAt.Pitch = 0;
    LookAt.Roll = 0;
    GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAt, DeltaTime, 5.0f));
}

void ABossEnemyAIController::BossMoveToPlayer()
{
    if (!PlayerPawn || !GetPawn()) return;

    MoveToActor(PlayerPawn, 5.0f);

    // 항상 플레이어 바라보기
    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    FRotator LookAt = (PlayerLoc - GetPawn()->GetActorLocation()).Rotation();
    LookAt.Pitch = 0;
    LookAt.Roll = 0;
    GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAt, GetWorld()->GetDeltaSeconds(), 5.0f));
}

bool ABossEnemyAIController::BossChasePlayer()
{
    // MoveAroundPlayer 상태에서 2초 이상 지나면 MoveToPlayer로 전환
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

    // 공격 애니메이션 실행 (실제 구현시 보스 캐릭터의 함수 호출)
    UE_LOG(LogTemp, Warning, TEXT("Boss Normal Attacking!"));

    bCanBossAttack = false;

    // 공격 후 일정 시간 대기 후 다시 MoveAroundPlayer로 전환
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

    // 플레이어 기준 원 반지름 표시
    DrawDebugCircle(GetWorld(), PlayerLoc, BossMoveRadius, 64, FColor::Cyan, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);

    // 보스 공격 범위 표시
    DrawDebugSphere(GetWorld(), BossLoc, BossAttackRange, 32, FColor::Red, false, -1, 0, 2);

    // 보스 인식 범위(Idle -> MoveAroundPlayer 전환)
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