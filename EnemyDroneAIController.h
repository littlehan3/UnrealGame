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
    float OrbitRadius = 500.f; // �÷��̾� �ֺ� ȸ�� �ݰ�

    UPROPERTY(EditAnywhere, Category = "AI")
    float OrbitSpeed = 45.f;  // ȸ�� �ӵ�(��/��)

    UPROPERTY(EditAnywhere, Category = "AI")
    float HeightOffset = 300.f; // �÷��̾� ���� ���� ������

private:
    AActor* PlayerActor;
    float CurrentAngle = 0.f;
    bool bClockwise = false;

    float TimeStuck = 0.f;
    float MaxStuckTime = 1.5f;
    float BaseHeight = 300.f;
    bool bTriedReverse = false;

    float TimeOutOfRadius = 0.f;
    float OutOfRadiusLimit = 1.5f; // 1.5��
    float RadiusTolerance = 100.f; // �˵� �ݰ� ���� ��밪

    float TargetHeight = 0.f;
    bool bRising = false;
};
