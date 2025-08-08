#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "EnemyDog.h"
#include "EnemyDogAIController.generated.h"

class AEnemyDog;
class AEnemyDogAnimInstance;

UENUM(BlueprintType)
enum class EEnemyDogAIState : uint8
{
	Idle,
	ChasePlayer
};

UCLASS()
class LOCOMOTION_API AEnemyDogAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyDogAIController();
	void StopAI();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	APawn* PlayerPawn;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackRange = 200.0f; // 공격 범위

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackCooldown = 2.0f; // 공격 쿨타임

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float ChaseStartDistance = 1000.0f; // 추적 시작 거리

	EEnemyDogAIState CurrentState = EEnemyDogAIState::Idle;

	bool bCanAttack = true; // 일반공격 가능 여부
	bool bIsAttacking = false; // 공격 진행 여부
	int32 NormalAttackCount = 0; // 일반 공격 횟수 카운트

	void NormalAttack(); // 일반공격 함수
	void ResetAttack(); // 공격 쿨다운 초기화

	FTimerHandle NormalAttackTimerHandle; // 일반공격 쿨타임 타이머

	void UpdateAIState(float DistanceToPlayer);
	void SetAIState(EEnemyDogAIState NewState);
	void ChasePlayer();

	FVector CachedTargetLocation;
	bool bHasCachedTarget = false;

	float RotationUpdateTimer = 0.0f;
	int32 StaticAngleOffset = 0;
};
