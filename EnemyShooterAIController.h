#pragma once

#include "CoreMinimal.h"
#include "AIController.h" // AIController 클래스 상속
#include "EnemyShooterAIController.generated.h"

// 전방 선언
class AEnemyShooter;
class AEnemyGuardian;
class AEnemyGrenade;

// AI의 행동 상태를 정의하는 열거형
UENUM(BlueprintType)
enum class EEnemyShooterAIState : uint8
{
    Idle,       // 대기 (플레이어 미발견)
    Detecting,  // 탐지 (플레이어 발견, 행동 결정 중)
    Moving,     // 이동 (사격 위치 또는 진형 위치로 이동)
    Shooting,   // 사격 (사격 가능 거리에서 공격)
    Retreating  // 후퇴 (플레이어가 너무 가까울 때)
};

UCLASS()
class LOCOMOTION_API AEnemyShooterAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyShooterAIController(); // 생성자

protected:
    virtual void BeginPlay() override; // 게임 시작 시 호출
    virtual void Tick(float DeltaTime) override; // 매 프레임 호출

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI State")
    EEnemyShooterAIState CurrentState = EEnemyShooterAIState::Idle; // 현재 AI 상태

    // AI 기본 설정 (거리)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Range")
    float DetectionRadius = 3000.0f; // 플레이어 탐지 최대 반경

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Range")
    float ShootRadius = 700.0f; // 사격을 시작하는 최대 거리

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Range")
    float MinShootDistance = 350.0f; // 이 거리보다 가까워지면 후퇴 시작

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Range")
    float RetreatBuffer = 100.0f; // 후퇴 후 다시 사격 상태로 전환하기 위한 거리 여유

    // 포메이션(진형) 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Formation")
    float FormationRadius = 600.0f; // 플레이어를 중심으로 형성할 진형의 반경

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Formation")
    float MinAllyDistance = 180.0f; // 다른 아군과 유지하려는 최소 거리 (뭉침 방지)

    // 회전 제한 설정 (상하 시야각 제한)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Rotation", meta = (ClampMin = "-89.0", ClampMax = "0.0"))
    float MaxLookDownAngle = -45.0f; // 아래쪽으로 볼 수 있는 최대 각도

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Rotation", meta = (ClampMin = "0.0", ClampMax = "89.0"))
    float MaxLookUpAngle = 15.0f; // 위쪽으로 볼 수 있는 최대 각도

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Rotation", meta = (ClampMin = "1.0", ClampMax = "20.0"))
    float RotationInterpSpeed = 8.0f; // 플레이어를 향해 회전하는 속도

    // 성능 최적화 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Performance")
    float AILogicUpdateInterval = 0.1f; // AI 로직 업데이트 주기 (사용되지 않음)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Performance")
    float AlliesSearchInterval = 1.0f; // 주변 아군을 검색하는 주기

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Performance")
    float ClearShotCheckInterval = 0.3f; // 사선 확보를 확인하는 주기

private:
    APawn* PlayerPawn = nullptr; // 플레이어 폰에 대한 참조
    float LastShootTime = 0.0f; // 마지막 사격 시간 (쿨타임 계산용)
    float ShootCooldown = 2.0f; // 사격 쿨타임

    // 포메이션 관련 변수
    FVector AssignedPosition; // 자신이 이동해야 할 진형 내 목표 위치
    int32 FormationIndex = -1; // 진형 내 자신의 순번 (사용되지 않음)
    bool bHasAssignedPosition = false; // 목표 위치를 할당받았는지 여부
    float LastPositionUpdate = 0.0f; // 마지막으로 진형 위치를 갱신한 시간
    float PositionUpdateInterval = 1.0f; // 진형 위치 갱신 주기

    // 성능 최적화를 위한 타이머 변수
    float LastAILogicUpdate = 0.0f; // 마지막 AI 로직 업데이트 시간 (사용되지 않음)
    float LastAlliesSearch = 0.0f; // 마지막 아군 검색 시간
    float LastClearShotCheck = 0.0f; // 마지막 사선 확인 시간

    // 캐시된 데이터 (매번 검색하는 비용을 줄이기 위함)
    TArray<TWeakObjectPtr<AEnemyShooter>> CachedAllies; // 캐시된 주변 아군 목록
    bool bCachedHasClearShot = true; // 캐시된 사선 확보 여부

    // 성능 최적화용 함수
    void UpdateCachedAllies(); // 주변 아군 목록을 검색하여 캐시를 갱신
    void UpdateCachedClearShot(); // 플레이어까지의 사선이 확보되었는지 확인하여 캐시를 갱신

    // 상태 머신 관련 함수
    void UpdateAIState(); // 현재 상태를 확인하고 다른 상태로 전환할지 결정
    void HandleIdleState(); // 대기 상태일 때의 로직 처리
    void HandleDetectingState(); // 탐지 상태일 때의 로직 처리
    void HandleMovingState(); // 이동 상태일 때의 로직 처리
    void HandleShootingState(); // 사격 상태일 때의 로직 처리
    void HandleRetreatingState(); // 후퇴 상태일 때의 로직 처리

    // 상태 확인용 유틸리티 함수
    float GetDistanceToPlayer() const; // 플레이어까지의 거리 반환
    bool IsPlayerInDetectionRange() const; // 플레이어가 탐지 범위 안에 있는지 확인
    bool IsPlayerInShootRange() const; // 플레이어가 사격 범위 안에 있는지 확인
    bool IsPlayerTooClose() const; // 플레이어가 너무 가까이 있는지 확인
    bool HasClearShotToPlayer() const; // 사선이 확보되었는지 (캐시된 값) 확인

    // 개선된 회전 시스템
    void LookAtPlayerWithConstraints(float DeltaTime); // 제한된 각도 내에서 플레이어를 부드럽게 바라봄
    FRotator CalculateConstrainedLookRotation(FVector TargetLocation) const; // 제한 각도를 적용한 최종 회전값 계산

    // 포메이션(진형) 시스템
    void UpdateFormationPosition(); // 자신의 진형 위치를 갱신
    FVector CalculateFormationPosition(); // 플레이어와 아군 위치를 기반으로 자신의 목표 위치 계산
    bool IsPositionOccupied(FVector Position, float CheckRadius = 150.0f); // 특정 위치가 다른 아군에 의해 점유되었는지 확인
    bool ShouldMoveToFormation() const; // 현재 진형 위치로 이동해야 하는지 판단
    TArray<AActor*> GetAllAllies() const; // 유효한 모든 아군 액터 목록 반환 (캐시 사용)

    // 기본 액션 함수
    void MoveTowardsTarget(FVector Target); // 특정 목표 지점으로 이동
    void MoveTowardsPlayer(); // 플레이어를 향해 이동
    //void RetreatFromPlayer(); // 플레이어로부터 후퇴 (HandleRetreatingState에서 직접 처리)
    void StopMovementInput(); // 이동 중지
    void PerformShooting(); // 사격 실행
    bool CanShoot() const; // 현재 사격 쿨타임이 지났는지 확인

    // 수류탄 관련 변수
    bool bIsBlockedByGuardian = false; // 아군 가디언에 의해 시야가 막혔는지 여부
    float LastGrenadeThrowTime = 0.0f; // 마지막 수류탄 투척 시간
    UPROPERTY(EditAnywhere, Category = "AI Combat")
    float GrenadeCooldown = 3.0f; // 수류탄 쿨타임

    UPROPERTY(EditAnywhere, Category = "AI Combat")
    float GrenadeLaunchSpeed = 700.0f; // 수류탄 발사 속도

    UPROPERTY(EditAnywhere, Category = "AI Combat")
    float GrenadeTargetOffset = 250.0f; // 수류탄 목표 지점 보정값 (플레이어 약간 뒤)
};