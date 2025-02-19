#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "EnemyAIController.generated.h"

class AEnemy; //포인터만 사용하므로 전방선언

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

    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float AttackRange = 200.0f; // 공격 범위

    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float AttackCooldown = 2.0f; // 공격 쿨타임

    bool bCanAttack = true; // 공격 가능 여부
    bool bIsAttacking = true; 

    void AttackPlayer(); // 공격 함수
    void ResetAttack(); // 공격 쿨다운 초기화

    FTimerHandle AttackTimerHandle; // 공격 쿨타임 타이머
};