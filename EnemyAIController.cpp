#include "EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Enemy.h"
#include "EnemyAnimInstance.h"
#include "BrainComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    APawn* ControlledPawn = GetPawn();

    if (ControlledPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("AIController is Possessing: %s"), *ControlledPawn->GetName());

        // AI가 NavMesh 위에 있는지 확인
        UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
        if (NavSys)
        {
            FNavLocation OutLocation;
            if (NavSys->ProjectPointToNavigation(ControlledPawn->GetActorLocation(), OutLocation))
            {
                UE_LOG(LogTemp, Warning, TEXT("AI is on a valid NavMesh!"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("AI is NOT on a valid NavMesh! AI cannot move."));
            }
        }
    }
}

void AEnemyAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!PlayerPawn) // 플레이어 NULL 체크
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        if (!PlayerPawn) return;
    }

    if (!GetPawn()) return; // AI 캐릭터가 NULL이면 실행 중지
    if (bIsDodging) return;

    // 플레이어와 AI의 거리 계산
    float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());

    // AI가 항상 플레이어를 바라보도록 설정
    FRotator LookAtRotation = (PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation()).Rotation();
    LookAtRotation.Pitch = 0.0f;
    LookAtRotation.Roll = 0.0f;
    GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAtRotation, DeltaTime, 5.0f));

    if (DistanceToPlayer <= DetectionRadius)
    {
        if (!bIsJumpAttacking)
        {
            JumpAttack();
        }
        else if (DistanceToPlayer <= AttackRange && bCanAttack)
        {
            if (NormalAttackCount == 3)
            {
                StrongAttack();
            }
            else
            {
                if (FMath::FRand() <= DodgeChance && bCanDodge)
                {
                    TryDodge();
                }
                else
                {
                    NormalAttack();
                }
            }
        }
        else
        {
            MoveToActor(PlayerPawn, 5.0f);
        }
    }
    else if (DistanceToPlayer > StopChasingRadius)
    {
        StopMovement();
        bIsJumpAttacking = false;
        NormalAttackCount = 0;
    }
}


AEnemyAIController::AEnemyAIController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AEnemyAIController::NormalAttack()
{
    AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
    if (EnemyCharacter)
    {
        bIsAttacking = true; // 공격 진행중
        EnemyCharacter->PlayNormalAttackAnimation();
        NormalAttackCount++; // 일반 공격 횟수 카운트 증가
        bCanAttack = false; // 공격 후 쿨다운 적용
        GetWorld()->GetTimerManager().SetTimer(NormalAttackTimerHandle, this, &AEnemyAIController::ResetAttack, AttackCooldown, false);
    }
}

void AEnemyAIController::ResetAttack()
{
    bCanAttack = true; // 공격 쿨다운 코기화
    bIsStrongAttacking = false; // 강 공격 종료
    bIsAttacking = false; // 공격 상태 종료
}

void AEnemyAIController::StrongAttack()
{
    AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
    if (EnemyCharacter)
    {
        bIsAttacking = true; // 공격 진행중
        bIsStrongAttacking = true; // 강 공격 시작
        EnemyCharacter->PlayStrongAttackAnimation();
        NormalAttackCount = 0; // 일반 공격 카운트 초기화
        bCanAttack = false; // 공격후 쿨다운 적용
        GetWorld()->GetTimerManager().SetTimer(NormalAttackTimerHandle, this, &AEnemyAIController::ResetAttack, AttackCooldown, false);
    }
}

void AEnemyAIController::TryDodge()
{
    if (bIsDodging || !bCanDodge || !bCanAttack || bIsAttacking) return; // 공격 진행 중엔 닷지 불가

    AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
    if (!EnemyCharacter) return;

    bIsDodging = true;
    bCanDodge = false; //연속 닷지방지

    bool bDodgeLeft = FMath::RandBool(); // 좌/우 랜덤 회피 결정
    EnemyCharacter->PlayDodgeAnimation(bDodgeLeft);

    // 닷지 몽타주의 정확한 길이를 가져와서 타이머 설정
    float DodgeDuration = (bDodgeLeft ? EnemyCharacter->GetDodgeLeftDuration() : EnemyCharacter->GetDodgeRightDuration());
    GetWorld()->GetTimerManager().SetTimer(DodgeTimerHandle, this, &AEnemyAIController::ResetDodge, DodgeDuration, false);
    // 회피 후 일정 시간 동안 회피 불가상태 유지
    GetWorld()->GetTimerManager().SetTimer(DodgeCooldownTimerHandle, this, &AEnemyAIController::ResetDodgeCoolDown, DodgeCooldown, false);
}

void AEnemyAIController::ResetDodge()
{
    bIsDodging = false; // 닷지 초기화
}

void AEnemyAIController::ResetDodgeCoolDown()
{
    bCanDodge = true; // 지정된시간이 지나면 닷지 가능
}

void AEnemyAIController::JumpAttack()
{
    AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
    if (!EnemyCharacter) return;

    bIsJumpAttacking = true; // 점프 공격 사용 상태 설정
    bCanAttack = false; // 점프 공격 중 다른 공격 불가

    EnemyCharacter->PlayJumpAttackAnimation(); // 점프 공격 애니메이션 실행

    StopMovement();

    // 점프 공격 타이머 전용 핸들 사용
    float JumpAttackDuration = EnemyCharacter->GetJumpAttackDuration();
    GetWorld()->GetTimerManager().SetTimer(JumpAttackTimerHandle, this, &AEnemyAIController::ResetAttack, JumpAttackDuration, false);
}

void AEnemyAIController::StopAI()
{
    AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
    if (!EnemyCharacter) return;

    // 먼저 모든 이동 중지
    StopMovement();
    UE_LOG(LogTemp, Warning, TEXT("%s AI Movement Stopped"), *GetPawn()->GetName());

    // BrainComponent가 존재하는지 확인 후 사용
    if (BrainComponent)
    {
        if (BrainComponent->IsRunning())
        {
            BrainComponent->StopLogic(TEXT("Enemy Died"));
            UE_LOG(LogTemp, Warning, TEXT("%s AI BrainLogic Stopped"), *GetPawn()->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s AI BrainLogic Already Stopped"), *GetPawn()->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s BrainComponent is NULL, Skip Logic Stop"), *GetPawn()->GetName());
    }

    // AI를 완전히 분리하기 위해 폰 UnPossess
    APawn* ControlledPawn = GetPawn();
    if (ControlledPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI Controller is UnPossessing %s"), *ControlledPawn->GetName());
        UnPossess();

        // 추가 안전 조치: 캐릭터의 충돌 및 틱 비활성화
        ControlledPawn->SetActorEnableCollision(false);
        ControlledPawn->SetActorTickEnabled(false);
    }
}