#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class AEnemy;
class AMaincharacter; 

// 적 AI 상태 열거형
UENUM(BlueprintType)
enum class EEnemyAIState : uint8
{
	Idle, // 아이들 상태
	MoveToCircle, // 포위 상태
	ChasePlayer // 추적 상태
};

UCLASS()
class LOCOMOTION_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	UFUNCTION()
	void StopAI(); // AI 동작중지

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	APawn* PlayerPawn; // 플레이어 참조

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float StopChasingRadius = 6000.0f; // 플레이어를 쫓다가 멈추는 범위

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float AttackRange = 200.0f; // 공격 범위

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float AttackCooldown = 2.0f; // 공격 쿨타임

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float DodgeChance = 0.3f; // 닷지 확률

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float DodgeCooldown = 5.0f; // 연속 닷지 방지 쿨타임

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float CircleRadius = 500.0f; // 원 반지름

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float CircleArriveThreshold = 60.0f; // 원 위치 도달 거리

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float ChaseStartDistance = 3000.0f; // 추적 시작 거리

	// 현재 AI 상태 변수
	EEnemyAIState CurrentState = EEnemyAIState::Idle; 

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
	bool bCanAttack = true; // 일반공격 가능 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
	bool bIsDodging = false; // 닷지 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
	bool bCanDodge = true; // 닷기 가능 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
	bool bCanStrongAttack = true; // 강공격 가능 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
	bool bIsStrongAttacking = false; // 강공격 진행 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
	bool bIsJumpAttacking = false; // 점프 공격 진행 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
	bool bIsAttacking = false; // 공격 진행 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
	bool bHasCachedTarget = false; // 캐싱된 타겟 위치 여부

	int32 NormalAttackCount = 0; // 일반 공격 횟수 카운트

	void NormalAttack(); // 일반공격 함수
	void ResetAttack(); // 공격 쿨다운 초기화
	void TryDodge(); // 회피 시도
	void ResetDodge();// 회피 후 초기화
	void ResetDodgeCoolDown(); // 지속 회피를 방지하기 위한 쿨다운 초기화
	void StrongAttack(); // 강공격 함수
	void JumpAttack(); // 점프공격 함수

	FTimerHandle NormalAttackTimerHandle; // 일반공격 쿨타임 타이머
	FTimerHandle DodgeTimerHandle; // 닷지 쿨타임 타이머
	FTimerHandle DodgeCooldownTimerHandle; // 닷지 쿨다운 타이머
	FTimerHandle JumpAttackTimerHandle; // 점프공격 타이머
	FTimerHandle CirclePositionTimerHandle; // 원 위치 재계산 타이머

	void MoveToDistributedLocation(); // 분산 위치로 이동
	void CalculateCirclePosition(); // 원 위치 계산
	void UpdateAIState(float DistanceToPlayer); // AI 상태 업데이트
	void SetAIState(EEnemyAIState NewState); // AI 상태 설정

	FVector CachedTargetLocation; // 캐싱된 타겟 위치

	void OnCirclePositionTimer(); // 원 위치 재계산 타이머 콜백
	float RotationUpdateTimer = 0.0f; // 회전 업데이트 타이머
	int32 StaticAngleOffset = 0; // 원 위치 계산을 위한 고정 각도 오프셋
};
