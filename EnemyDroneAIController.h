#pragma once

#include "CoreMinimal.h"
#include "AIController.h" // AIController 클래스 상속
#include "EnemyDroneAIController.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyDroneAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyDroneAIController(); // 생성자

protected:
    virtual void BeginPlay() override; // 게임 시작 시 호출
    virtual void Tick(float DeltaTime) override; // 매 프레임 호출

    // 플레이어 주변을 공전하는 궤도의 반경
    UPROPERTY(EditAnywhere, Category = "AI")
    float OrbitRadius = 500.f;

    // 궤도를 따라 회전하는 속도 (초당 각도)
    UPROPERTY(EditAnywhere, Category = "AI")
    float OrbitSpeed = 45.f;

    // 플레이어로부터 유지하려는 기본 고도
    UPROPERTY(EditAnywhere, Category = "AI")
    float HeightOffset = 300.f;

private:
    AActor* PlayerActor; // 플레이어 액터 참조
    float CurrentAngle = 0.f; // 현재 궤도 상의 각도
    bool bClockwise = false; // 현재 회전 방향 (true: 시계, false: 반시계)

    // 장애물에 끼었는지 판단하기 위한 변수들
    float TimeStuck = 0.f; // 한자리에 머무른 시간
    float MaxStuckTime = 1.5f; // 최대 허용 시간
    bool bTriedReverse = false; // 방향을 한 번 반전했는지 여부

    // 기본 고도를 저장해두는 변수 (고도 회피 후 복귀용)
    float BaseHeight = 300.f;

    // 궤도 이탈 감지용 변수들
    float TimeOutOfRadius = 0.f; // 궤도 반경을 벗어난 시간
    float OutOfRadiusLimit = 1.5f; // 궤도 이탈 허용 시간
    float RadiusTolerance = 100.f; // 궤도 반경의 오차 허용 범위

    // 긴급 고도 상승 로직용 변수들
    float TargetHeight = 0.f; // 긴급 회피 시 목표 고도
    bool bRising = false; // 현재 긴급 상승 중인지 여부
};