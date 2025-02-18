#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "EnemyAIController.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyAIController : public AAIController
{
    GENERATED_BODY()

public:
	AEnemyAIController();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    APawn* PlayerPawn;  // 플레이어 참조
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float DetectionRadius = 800.0f;  // 플레이어 감지 범위

    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float StopChasingRadius = 1200.0f;  // 플레이어를 쫓다가 멈추는 범위
};
