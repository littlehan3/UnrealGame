#pragma once

#include "CoreMinimal.h"
#include "AIController.h" // AIController 클래스를 상속받기 위해 포함
#include "NavigationSystem.h" // 네비게이션 시스템 사용을 위해 포함
#include "DrawDebugHelpers.h" // 디버그 시각화 기능 사용을 위해 포함
#include "EnemyDog.h" // 제어할 대상인 EnemyDog 클래스를 알아야 하므로 포함
#include "EnemyDogAIController.generated.h"

class AEnemyDog; // EnemyDog 클래스 전방 선언
class AEnemyDogAnimInstance; // EnemyDog 애님 인스턴스 클래스 전방 선언

// AI의 행동 상태를 정의하는 열거형
UENUM(BlueprintType)
enum class EEnemyDogAIState : uint8
{
	Idle,        // 대기 상태
	ChasePlayer  // 플레이어 추적 상태
};

UCLASS()
class LOCOMOTION_API AEnemyDogAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyDogAIController(); // 생성자
	void StopAI(); // AI의 모든 동작을 중지시키는 함수

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출되는 함수
	virtual void Tick(float DeltaTime) override; // 매 프레임 호출되는 함수

private:
	APawn* PlayerPawn; // 플레이어 폰에 대한 참조

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackRange = 150.0f; // 공격을 시작하는 범위

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackCooldown = 1.0f; // 공격 후 다음 공격까지의 대기 시간

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float ChaseStartDistance = 2000.0f; // 플레이어 추적을 시작하는 거리

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float SurroundRadius = 100.0f; // 플레이어를 포위할 때 유지하는 거리

	EEnemyDogAIState CurrentState = EEnemyDogAIState::Idle; // 현재 AI의 상태

	bool bCanAttack = true; // 현재 공격이 가능한지 여부
	bool bIsAttacking = false; // 현재 공격 애니메이션이 진행 중인지 여부
	//int32 NormalAttackCount = 0; // 일반 공격 횟수 카운트

	void NormalAttack(); // 일반 공격을 수행하는 함수
	void ResetAttack(); // 공격 쿨타임이 끝나고 다시 공격 가능 상태로 만드는 함수

	FTimerHandle NormalAttackTimerHandle; // 공격 쿨타임을 제어하는 타이머

	void UpdateAIState(float DistanceToPlayer); // 플레이어와의 거리에 따라 AI 상태를 갱신하는 함수
	void SetAIState(EEnemyDogAIState NewState); // 새로운 AI 상태로 변경하는 함수
	void ChasePlayer(); // 플레이어를 추적하는 로직을 처리하는 함수

	FVector CachedTargetLocation; // 목표 위치 캐싱
	bool bHasCachedTarget = false; // 목표 위치 캐싱 여부

	float RotationUpdateTimer = 0.0f; // 회전 업데이트 타이머
	int32 StaticAngleOffset = 0; // 고정 각도 오프셋
};