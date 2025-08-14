#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyDroneAIController.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyDroneAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyDroneAIController();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, Category = "AI")
    float OrbitRadius = 500.f; // 플레이어 주변 회전 반경

    UPROPERTY(EditAnywhere, Category = "AI")
    float OrbitSpeed = 45.f;  // 회전 속도(도/초)

    UPROPERTY(EditAnywhere, Category = "AI")
    float HeightOffset = 300.f; // 플레이어 기준 높이 유지값

private:
    AActor* PlayerActor;
    float CurrentAngle = 0.f;
    bool bClockwise = false;

    float TimeStuck = 0.f;
    float MaxStuckTime = 1.5f;
    float BaseHeight = 300.f;
    bool bTriedReverse = false;

    float TimeOutOfRadius = 0.f;
    float OutOfRadiusLimit = 1.5f; // 1.5초
    float RadiusTolerance = 100.f; // 궤도 반경 오차 허용값

    float TargetHeight = 0.f;
    bool bRising = false;
};
