#pragma once

#include "CoreMinimal.h"
#include "AIController.h" // AIController 클래스 상속
#include "EnemyGuardianAIController.generated.h"

// 전방 선언
class AEnemyShooter;
class AEnemyGuardian;

UCLASS()
class LOCOMOTION_API AEnemyGuardianAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyGuardianAIController(); // 생성자
    void StopAI(); // AI의 모든 동작을 중지시키는 함수 (현재 미사용)

protected:
    virtual void BeginPlay() override; // 게임 시작 시 호출
    virtual void Tick(float DeltaTime) override; // 매 프레임 호출

private:
    UPROPERTY()
    APawn* PlayerPawn;

    // 핵심 행동 로직 함수
    void PerformShooterProtection(); // 아군 슈터를 보호하는 로직
    void PerformSurroundMovement(); // 플레이어를 포위하는 로직

    // 최적화를 위한 캐싱 시스템
    void UpdateAllyCaches(); // 주기적으로 호출되어 아군 목록을 갱신하는 함수

    FTimerHandle AllyCacheUpdateTimerHandle; // 아군 캐시 갱신을 위한 타이머

    // 캐시된 아군 목록 (TWeakObjectPtr로 사용하여 아군이 죽어도 메모리 누수 방지)
    TArray<TWeakObjectPtr<AEnemyShooter>> CachedShooters;
    TArray<TWeakObjectPtr<AEnemyGuardian>> CachedGuardians;

    // AI 행동 관련 설정값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (AllowPrivateAccess = "true"))
    float ProtectionDistance = 150.0f; // 보호할 슈터로부터 떨어져 있을 거리

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (AllowPrivateAccess = "true"))
    float MinDistanceToTarget = 50.0f; // 목표 지점 도달로 판정할 최소 거리

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (AllowPrivateAccess = "true"))
    float SurroundRadius = 150.0f; // 플레이어를 포위할 때 유지할 거리

    // 성능 최적화 설정
    UPROPERTY(EditAnywhere, Category = "AI Performance", meta = (AllowPrivateAccess = "true"))
    float AllyCacheUpdateInterval = 2.0f; // 주변 아군 목록을 갱신하는 주기
};