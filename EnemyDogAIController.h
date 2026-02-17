#pragma once

#include "CoreMinimal.h"
#include "AIController.h" // AIController 클래스를 상속받기 위해 포함
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
	
	UFUNCTION()
	void StopAI(); // AI의 모든 동작을 중지시키는 함수

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출되는 함수
	virtual void Tick(float DeltaTime) override; // 매 프레임 호출되는 함수

private:
	APawn* PlayerPawn; // 플레이어 폰에 대한 참조

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float AttackRange = 130.0f; // 공격을 시작하는 범위

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float AttackCooldown = 1.0f; // 공격 후 다음 공격까지의 대기 시간

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float ChaseStartDistance = 3000.0f; // 플레이어 추적을 시작하는 거리

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float SurroundRadius = 100.0f; // 플레이어를 포위할 때 유지하는 거리

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float AIUpdateInterval = 0.05f; // 틱 주기 (최적화용)

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float RotationInterpSpeed = 10.0f; // 회전 부드러움 정도

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float MoveAcceptanceRadius = 10.0f; // 이동 목표 도착 인정 범위

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float AttackStartDelay = 0.1f; // 공격 애니메이션 전 지연 시간

	EEnemyDogAIState CurrentState = EEnemyDogAIState::Idle; // 현재 AI의 상태

	UPROPERTY(VisibleInstanceOnly, Category = "State")
	bool bCanAttack = true; // 현재 공격이 가능한지 여부
	UPROPERTY(VisibleInstanceOnly, Category = "State")
	bool bIsAttacking = false; // 현재 공격 애니메이션이 진행 중인지 여부
	UPROPERTY(VisibleInstanceOnly, Category = "State")
	bool bHasCachedTarget = false; // 목표 위치 캐싱 여부

	void NormalAttack(); // 일반 공격을 수행하는 함수
	void ResetAttack(); // 공격 쿨타임이 끝나고 다시 공격 가능 상태로 만드는 함수

	FTimerHandle NormalAttackTimerHandle; // 공격 쿨타임을 제어하는 타이머
	FTimerHandle AttackDelayTimer; // 공격 지연 타이머

	void UpdateAIState(float DistanceToPlayer); // 플레이어와의 거리에 따라 AI 상태를 갱신하는 함수
	void SetAIState(EEnemyDogAIState NewState); // 새로운 AI 상태로 변경하는 함수
	void ChasePlayer(); // 플레이어를 추적하는 로직을 처리하는 함수

	FVector CachedTargetLocation; // 목표 위치 캐싱

	float RotationUpdateTimer = 0.0f; // 회전 업데이트 타이머
	int32 StaticAngleOffset = 0; // 고정 각도 오프셋
};