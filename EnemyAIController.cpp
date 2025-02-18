#include "EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"

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

        // AI 이동 명령 실행
        if (PlayerPawn)
        {
            MoveToActor(PlayerPawn, 5.0f);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AIController has no Pawn! Waiting and retrying..."));

        // AIController가 할당될 때까지 다시 Possess 시도
        GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
            {
                Possess(GetPawn());
                if (GetPawn())
                {
                    UE_LOG(LogTemp, Warning, TEXT("AIController successfully Possessed: %s"), *GetPawn()->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("AIController still has no Pawn!"));
                }
            });
    }

}

void AEnemyAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!PlayerPawn) return;

    float DistanceToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());

    // AI가 이동할 수 있는지 확인
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (NavSys && GetPawn())
    {
        if (!NavSys->GetNavDataForProps(GetPawn()->GetNavAgentPropertiesRef()))
        {
            UE_LOG(LogTemp, Error, TEXT("AIController: No valid NavMesh found! AI cannot move."));
            return;
        }
    }

    if (DistanceToPlayer <= DetectionRadius)
    {
        UE_LOG(LogTemp, Warning, TEXT("Chasing Player..."));
        MoveToActor(PlayerPawn, 5.0f);
    }
    else if (DistanceToPlayer > StopChasingRadius)
    {
        UE_LOG(LogTemp, Warning, TEXT("Stop Chasing"));
        StopMovement();
    }
}

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true; // Tick 활성화
}

