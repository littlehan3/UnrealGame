#include "EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Enemy.h"

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

    // 플레이어와 AI의 거리 계산
    float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());

    if (!PlayerPawn || !GetPawn() || bIsDodging) return;

    // AI가 항상 플레이어를 바라보도록 설정
    FRotator LookAtRotation = (PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation()).Rotation();
    LookAtRotation.Pitch = 0.0f;
    LookAtRotation.Roll = 0.0f;
    GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAtRotation, DeltaTime, 5.0f));


    if (DistanceToPlayer <= DetectionRadius)
    {
        if (!bIsJumpAttacking) // 공격 범위 내에 플레이어가 진입시
        {
            JumpAttack(); // 점프공격
        }
        else if (DistanceToPlayer <= AttackRange && bCanAttack) // 공격 범위 내에 플레이어가 있고 공격 가능상태라면
        {
            if (NormalAttackCount == 3) // 일반 공격을 3번 했다면
            {
                StrongAttack(); // 강 공격
            }
            else
            {
                if (FMath::FRand() <= DodgeChance && bCanDodge) // 지정된 닷지 확률과 닷지가능 상태라면 
                {
                    TryDodge(); // 닷지
                }
                else
                {
                    NormalAttack(); // 일반 공격
                }
            }
        }
        else
        {
            MoveToActor(PlayerPawn, 5.0f); // 공격 범위 박이면 플레이어를 향해 이동
        }
    }
    else if (DistanceToPlayer > StopChasingRadius)
    {
        StopMovement(); // 감지 반경을 벗어나면 이동 중지
        bIsJumpAttacking = false; // 감지범위 밖으로 나가면 점프공격 다시 사용 가능
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
}

void AEnemyAIController::StrongAttack()
{
    AEnemy* EnemyCharacter = Cast<AEnemy>(GetPawn());
    if (EnemyCharacter)
    {
        bIsStrongAttacking = true; // 강 공격 시작
        EnemyCharacter->PlayStrongAttackAnimation();
        NormalAttackCount = 0; // 일반 공격 카운트 초기화
        bCanAttack = false; // 공격후 쿨다운 적용
        GetWorld()->GetTimerManager().SetTimer(NormalAttackTimerHandle, this, &AEnemyAIController::ResetAttack, AttackCooldown, false);
    }
}

void AEnemyAIController::TryDodge()
{
    if (bIsDodging || !bCanDodge || !bCanAttack || !bIsStrongAttacking) return; // 닷지 중,닷지 불가상태, 공격상태면 실행되지 않도록 설정

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

    // 루트모션이 적용된 애니메이션
    float JumpAttackDuration = EnemyCharacter->GetJumpAttackDuration();
    GetWorld()->GetTimerManager().SetTimer(NormalAttackTimerHandle, this, &AEnemyAIController::ResetAttack, JumpAttackDuration, false);
}