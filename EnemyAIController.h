#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "EnemyAIController.generated.h"

class AEnemy; //포인터만 사용하므로 전방선언
class AEnemyAnimInstance;

UENUM(BlueprintType)
enum class EEnemyAIState : uint8
{
	Idle,
	MoveToCircle,
	ChasePlayer
};

UCLASS()
class LOCOMOTION_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	void StopAI(); // AI 동작중지

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	APawn* PlayerPawn; // 플레이어 참조

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float StopChasingRadius = 6000.0f; // 플레이어를 쫓다가 멈추는 범위

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackRange = 200.0f; // 공격 범위

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackCooldown = 2.0f; // 공격 쿨타임

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float DodgeChance = 0.3f; // 회피확률

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float DodgeCooldown = 5.0f; // 연속 닷지 방지 쿨타임

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float CircleRadius = 500.0f; // 원 반지름

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float CircleArriveThreshold = 60.0f; // 원 위치 도달 거리

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float ChaseStartDistance = 3000.0f; // 추적 시작 거리

	EEnemyAIState CurrentState = EEnemyAIState::Idle;

	bool bCanAttack = true; // 일반공격 가능 여부
	bool bIsDodging = false; // 닷지 여부
	bool bCanDodge = true; // 닷기 가능 여부
	bool bCanStrongAttack = true; // 강공격 가능 여부
	bool bIsStrongAttacking = false; // 강공격 진행 여부
	bool bIsJumpAttacking = false; // 점프 공격 진행 여부
	bool bIsAttacking = false; // 공격 진행 여부

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
	FTimerHandle CirclePositionTimerHandle;

	void MoveToDistributedLocation();
	void CalculateCirclePosition();
	void UpdateAIState(float DistanceToPlayer);
	void SetAIState(EEnemyAIState NewState);

	FVector CachedTargetLocation;
	bool bHasCachedTarget = false;

	void OnCirclePositionTimer();

	// 성능 최적화를 위한 새로운 변수들
	float RotationUpdateTimer = 0.0f;
	int32 StaticAngleOffset = 0;
};
