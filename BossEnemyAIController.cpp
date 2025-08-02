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

    // 스텔스 공격 중이면 AI 행동 완전 중지
    if (IsExecutingStealthAttack() || bIsAIDisabledForStealth)
    {
        if (CurrentState != EBossEnemyAIState::NormalAttack)
        {
            SetBossAIState(EBossEnemyAIState::NormalAttack);
        }
        DrawDebugInfo();
        return; // 다른 행동은 수행하지 않음
    }

    // **스텔스 종료 후 Idle 상태에 갇힌 경우 강제로 MoveToPlayer 상태로 전환**
    if (CurrentState == EBossEnemyAIState::Idle && !Boss->bIsPlayingBossIntro)
    {
        float DistToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
        if (DistToPlayer <= BossDetectRadius)
        {
            UE_LOG(LogTemp, Warning, TEXT("Force transitioning from Idle to MoveToPlayer"));
            SetBossAIState(EBossEnemyAIState::MoveToPlayer);
        }
    }

    // **스텔스 종료 후 AI 상태 복구 안전장치**
    if (!bIsAIDisabledForStealth && !IsExecutingStealthAttack() &&
        !Boss->bIsFullBodyAttacking && !Boss->bIsPlayingBossIntro)
    {
        // 스텔스가 완전히 끝났는데 Idle 상태에 갇힌 경우
        if (CurrentState == EBossEnemyAIState::Idle)
        {
            float DistToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
            if (DistToPlayer <= BossDetectRadius)
            {
                UE_LOG(LogTemp, Warning, TEXT("Force recovering from post-stealth Idle state"));
                SetBossAIState(EBossEnemyAIState::MoveToPlayer);
                SetFocus(PlayerPawn); // 플레이어 포커스 재설정
            }
        }
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
    if (!bCanBossAttack) return; // 공격 가능 상태가 아니면 리턴

    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn()); // 보스 객체를 캐스트해서 가져옴
    if (!Boss) return; // 보스가 없으면 리턴

    // 피격 중이거나 사망한 경우 또는 전신공격 중인 경우 또는 텔레포트 중인 경우 공격하지 않음
    if (Boss->bIsBossHit || Boss->bIsBossDead || Boss->bIsFullBodyAttacking ||
        Boss->bIsBossTeleporting || Boss->bIsBossAttackTeleporting || Boss->bIsBossRangedAttacking) return;

    float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation()); // 플레이어와 보스 간의 거리 계산
    float CurrentTime = GetWorld()->GetTimeSeconds();

    UE_LOG(LogTemp, Warning, TEXT("Boss choosing attack - Distance: %f"), DistanceToPlayer); // 디버그 로그: 현재 거리 출력

    // 최우선순위: 스텔스 공격 체크를 모든 거리 조건 이전에 실행
    if (CanExecuteStealthAttack()) // 스텔스 공격 조건을 만족하는지 체크
    {
        UE_LOG(LogTemp, Warning, TEXT("Stealth conditions met - rolling dice")); // 디버그 로그: 스텔스 조건 만족됨
        float StealthChance = FMath::RandRange(0.0f, 1.0f); // 0.0~1.0 사이의 랜덤 값 생성
        if (StealthChance <= 0.5f) // 50% 확률로 스텔스 공격 실행
        {
            Boss->PlayBossStealthAttackAnimation(); // 스텔스 공격 애니메이션 재생
            UE_LOG(LogTemp, Warning, TEXT("Boss executing STEALTH ATTACK!")); // 디버그 로그: 스텔스 공격 실행됨
            return; // 스텔스 공격이 선택되면 다른 공격은 실행하지 않음
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Stealth dice roll failed - continuing with normal attacks")); // 디버그 로그: 스텔스 확률 실패, 일반 공격으로 진행
        }
    }

    // 그 다음에 기존 거리별 공격 로직 실행
    if (Boss && Boss->bCanBossAttack) // 보스가 존재하고 공격 가능 상태인지 체크
    {
        if (DistanceToPlayer <= BossStandingAttackRange) // 0-200 범위: 근거리 공격 범위
        {
            bool bShouldTeleport = FMath::RandBool(); // 50% 확률로 true/false 결정
            if (bShouldTeleport && Boss->bCanTeleport) // 텔레포트가 선택되고 텔레포트 가능 상태인 경우
            {
                // 50% 확률 - 멀어지는 텔레포트 실행
                Boss->PlayBossTeleportAnimation(); // 후퇴 텔레포트 애니메이션 재생
                UE_LOG(LogTemp, Warning, TEXT("Boss chose retreat teleport - Distance: %f"), DistanceToPlayer); // 디버그 로그: 후퇴 텔레포트 선택됨
            }
            else
            {
                // 50% 확률 - 전신 공격 실행
                Boss->PlayBossNormalAttackAnimation(); // 일반 공격 애니메이션 재생
                StopMovement(); // AI 이동 중지 (공격 중에는 움직이지 않음)
                UE_LOG(LogTemp, Warning, TEXT("Boss chose standing attack - Distance: %f"), DistanceToPlayer); // 디버그 로그: 근거리 공격 선택됨
            }
        }
        else if (DistanceToPlayer > BossStandingAttackRange && DistanceToPlayer <= BossMovingAttackRange) // 201-250 범위: 중거리 공격 범위
        {
            bool bDoRangedAttack = false;
            // 1. 원거리 쿨타임 및 예외(뒷텔레포트 후 1회 허용) 체크
            if (bIgnoreRangedCooldownOnce ||
                ((CurrentTime - LastRangedAttackTime) >= RangedAttackCooldown))
            {
                bDoRangedAttack = FMath::RandBool(); // 50% 확률 (변경 가능)
            }

            // 2. 실제 공격 실행
            if (bDoRangedAttack)
            {
                Boss->PlayBossRangedAttackAnimation();
                LastRangedAttackTime = CurrentTime;
                if (bIgnoreRangedCooldownOnce) bIgnoreRangedCooldownOnce = false; // 한 번만 적용
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
            // 공격 범위를 벗어난 경우 (251 이상) 공격하지 않음
            UE_LOG(LogTemp, Warning, TEXT("Out of attack range - Distance: %f"), DistanceToPlayer); // 디버그 로그: 공격 범위 벗어남
            return; // 공격하지 않으므로 bCanBossAttack을 false로 설정하지 않고 리턴
        }

        // 스텔스 공격이 아닌 일반 공격이 실행된 경우에만 공격 불가 상태로 설정
        if (!Boss->bIsBossTeleporting && !Boss->bIsBossAttackTeleporting && !IsExecutingStealthAttack())
        {
            bCanBossAttack = false; // 공격 후 쿨타임을 위해 공격 불가 상태로 설정
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
        bIgnoreRangedCooldownOnce = true; // 다음 투사체 공격만 쿨타임 무시
        Boss->PlayBossRangedAttackAnimation();
        LastRangedAttackTime = GetWorld()->GetTimeSeconds(); // CD 갱신
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
    bCanBossAttack = true; // 즉시 추격 재개
}

bool ABossEnemyAIController::CanExecuteStealthAttack() const
{
    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn());
    if (!Boss) return false;

    // 스텔스 공격 가능 여부 체크 (BossEnemy에 있는 변수)
    if (!Boss->bCanUseStealthAttack) return false;

    // 다른 특수 공격이 진행 중인지 체크 (BossEnemy에 있는 변수들)
    if (Boss->bIsBossRangedAttacking || Boss->bIsBossTeleporting ||
        Boss->bIsBossAttackTeleporting) return false;

    // 거리 조건 체크
    if (!IsInOptimalStealthRange()) return false;

    return true;
}

bool ABossEnemyAIController::IsInOptimalStealthRange() const
{
    APawn* TargetPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);  // 변수명 변경
    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn());

    if (!TargetPlayer || !Boss) return false;

    float Distance = FVector::Dist(Boss->GetActorLocation(), TargetPlayer->GetActorLocation());

    // 스텔스 공격 최적 거리: 300-600
    return (Distance >= StealthAttackMinRange && Distance <= StealthAttackOptimalRange);
}


bool ABossEnemyAIController::IsExecutingStealthAttack() const
{
    ABossEnemy* Boss = Cast<ABossEnemy>(GetPawn());
    if (!Boss) return false;

    // BossEnemy에 실제로 선언된 스텔스 변수들만 체크
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
    case 1: // 스텔스 시작
        bIsAIDisabledForStealth = true;
        bCanBossAttack = false;  // **AI 공격도 비활성화**
        StopMovement();
        UE_LOG(LogTemp, Warning, TEXT("AI Disabled for Stealth Attack"));
        break;

    case 3: // 투명화 단계
        SetFocus(nullptr);
        StopMovement();
        UE_LOG(LogTemp, Warning, TEXT("Stealth invisible - AI tracking disabled"));
        break;

    case 5: // 킥 공격
        if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
        {
            SetFocus(Player);
        }
        UE_LOG(LogTemp, Warning, TEXT("Stealth kick phase - refocusing player"));
        break;

    case 6: // 피니쉬 공격 단계
        StopMovement();
        UE_LOG(LogTemp, Warning, TEXT("Stealth finish phase - AI disabled"));
        break;

    case 0: // 스텔스 종료 AI 상태 복구
        bIsAIDisabledForStealth = false;
        bCanBossAttack = true;  // AI 컨트롤러 공격 상태 복구

        // 플레이어 다시 타겟팅
        if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
        {
            SetFocus(Player);
        }

        // AI 상태를 강제로 MoveToPlayer로 설정
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

    // 플레이어 기준 원 반지름 표시
    DrawDebugCircle(GetWorld(), PlayerLoc, BossMoveRadius, 64, FColor::Cyan, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);

    // 보스 공격 범위 표시
 
    DrawDebugCircle(GetWorld(), BossLoc, BossMovingAttackRange, 64, FColor::Orange, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);

    DrawDebugCircle(GetWorld(), BossLoc, BossStandingAttackRange, 64, FColor::Yellow, false, -1, 0, 1, FVector(1, 0, 0), FVector(0, 1, 0), false);

    DrawDebugCircle(GetWorld(), BossLoc, StealthAttackMinRange, 64, FColor::Green, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);

    DrawDebugCircle(GetWorld(), BossLoc, StealthAttackOptimalRange, 64, FColor::Blue, false, -1, 0, 2, FVector(1, 0, 0), FVector(0, 1, 0), false);
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