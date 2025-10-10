#pragma once

#include "CoreMinimal.h"
#include "AIController.h" // AIController 클래스 상속
#include "Engine/World.h" // UWorld 클래스 사용
#include "EngineUtils.h" // TActorIterator 사용
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

    // AI 행동 관련 설정값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior")
    float ProtectionDistance = 150.0f; // 보호할 슈터로부터 떨어져 있을 거리

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior")
    float MinDistanceToTarget = 50.0f; // 목표 지점 도달로 판정할 최소 거리

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior")
    float SurroundRadius = 150.0f; // 플레이어를 포위할 때 유지할 거리

private:
    APawn* PlayerPawn; // 플레이어 폰에 대한 참조

    // 4방향 이동 패턴 관련 변수 (현재는 포위 로직으로 대체됨)
    FTimerHandle MoveTimerHandle; // 이동 방향 전환 타이머
    float MoveDuration; // 이동 지속 시간
    int32 DirectionIndex; // 현재 이동 방향 인덱스

    // 핵심 행동 로직 함수
    void MoveInDirection(); // 특정 방향으로 이동 (현재 미사용)
    void PerformShooterProtection(); // 아군 슈터를 보호하는 로직
    void PerformSurroundMovement(); // 플레이어를 포위하는 로직
};