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

private:
    UPROPERTY()
    AActor* PlayerActor = nullptr; // 플레이어 액터 참조

    // 플레이어 주변을 공전하는 궤도의 반경
    UPROPERTY(EditAnywhere, Category = "AI", meta = (AllowPrivateAccess = "true"))
    float OrbitRadius = 500.0f;

    // 궤도를 따라 회전하는 속도 (초당 각도)
    UPROPERTY(EditAnywhere, Category = "AI", meta = (AllowPrivateAccess = "true"))
    float OrbitSpeed = 45.0f;

    // 플레이어로부터 유지하려는 기본 고도
    UPROPERTY(EditAnywhere, Category = "AI", meta = (AllowPrivateAccess = "true"))
    float HeightOffset = 300.0f;

    UPROPERTY(VisibleInstanceOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    float CurrentAngle = 0.0f; // 현재 궤도 상의 각도

    UPROPERTY(VisibleInstanceOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    bool bClockwise = false; // 현재 회전 방향 (true: 시계, false: 반시계)

    UPROPERTY(VisibleInstanceOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    // 장애물에 끼었는지 판단하기 위한 변수들
    float TimeStuck = 0.0f; // 한자리에 머무른 시간

    UPROPERTY(EditAnywhere, Category = "AI", meta = (AllowPrivateAccess = "true"))
    float MaxStuckTime = 1.5f; // 최대 허용 시간

    UPROPERTY(VisibleInstanceOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    bool bTriedReverse = false; // 방향을 한 번 반전했는지 여부

    UPROPERTY(VisibleInstanceOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    // 기본 고도를 저장해두는 변수 (고도 회피 후 복귀용)
    float BaseHeight = 300.f;

    // 궤도 이탈 감지용 변수들
    UPROPERTY(VisibleInstanceOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    float TimeOutOfRadius = 0.0f; // 궤도 반경을 벗어난 시간
	UPROPERTY(EditAnywhere, Category = "AI", meta = (AllowPrivateAccess = "true"))
    float OutOfRadiusLimit = 1.5f; // 궤도 이탈 허용 시간
	UPROPERTY(EditAnywhere, Category = "AI", meta = (AllowPrivateAccess = "true"))
    float RadiusTolerance = 100.0f; // 궤도 반경의 오차 허용 범위
    // 목표 고도 도달을 인정하는 허용 오차 범위
    UPROPERTY(EditAnywhere, Category = "AI | State", meta = (AllowPrivateAccess = "true"))
    float HeightArrivalTolerance = 5.0f;

    // 긴급 고도 상승 로직용 변수들
    UPROPERTY(VisibleInstanceOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    float TargetHeight = 0.0f; // 긴급 회피 시 목표 고도
	UPROPERTY(VisibleInstanceOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    bool bRising = false; // 현재 긴급 상승 중인지 여부
	UPROPERTY(EditAnywhere, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float EscapeHight = 500.0f; // 끼임 탈출 시 상승할 고도

    // 보간 속도 설정
    UPROPERTY(EditAnywhere, Category = "AI | InterpSpeed", meta = (AllowPrivateAccess = "true"))
    float MoveInterpSpeed = 2.0f;
    UPROPERTY(EditAnywhere, Category = "AI | InterpSpeed", meta = (AllowPrivateAccess = "true"))
    float DefaultMoveInterpSpeed = 1.0f;
    UPROPERTY(EditAnywhere, Category = "AI | InterpSpeed", meta = (AllowPrivateAccess = "true"))
    float RotationInterpSpeed = 5.0f;
    UPROPERTY(EditAnywhere, Category = "AI | InterpSpeed", meta = (AllowPrivateAccess = "true"))
    float HeightReturnInterpSpeed = 0.5f; // 고도 복귀 속도

    // 드론끼리 만났을 때 고도를 변경할 단위 거리 (기존 150.f)
    UPROPERTY(EditAnywhere, Category = "AI | Avoidance", meta = (AllowPrivateAccess = "true"))
    float DroneAvoidanceStep = 150.0f;

    // 드론이 유지할 수 있는 최소 고도 (기존 150.f)
    UPROPERTY(EditAnywhere, Category = "AI | Avoidance", meta = (AllowPrivateAccess = "true"))
    float MinFlightHeight = 150.0f;

    // 드론이 올라갈 수 있는 최대 고도 (기존 1000.f)
    UPROPERTY(EditAnywhere, Category = "AI | Avoidance", meta = (AllowPrivateAccess = "true"))
    float MaxFlightHeight = 1000.0f;
};