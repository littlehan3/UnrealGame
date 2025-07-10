#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "BossEnemyAIController.generated.h"

class ABossEnemy; // 보스 포인터 사용으로 전방선언
class ABossEnemyAnimInstance; // 애님 인스턴스 사용으로 전방선언 

UENUM(BlueprintType)
enum class EBossEnemyAIState : uint8 // 보스 상태 열거형 선언
{
	Idle,
	MoveAroundPlayer,
	MoveToPlayer,
	NormalAttack,
	//JumpAttack,
	//SpecialAttack,
	//Dash,
	//ProjectileAttack,
	//TeleportToPlayersBack
};

UCLASS()
class LOCOMOTION_API ABossEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABossEnemyAIController();

	void StopBossAI(); // AI 동작 중지 함수

protected:
	virtual void BeginPlay() override; // Beginplay 오버라이드로 초기화
	virtual void Tick(float DeltaTime) override; // Tick 오버라이드로 프레임별 갱신

private:
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossMoveRadius = 600.0f; // 원 반지름

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossAttackRange = 250.0f; // 공격 범위

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossMoveSpeed = 400.0f; // 이동 속도

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossDetectRadius = 2000.0f; // 보스가 플레이어를 인식하는 거리

	EBossEnemyAIState CurrentState = EBossEnemyAIState::Idle;
	float CurrentAngle = 0.0f; // 원 위 위치 각도

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossAttackCooldown = 1.0f; // 공격 쿨타임 1초

	APawn* PlayerPawn = nullptr; // 플레이어 참조
	bool bCanBossAttack = true; // 공격 가능여부
	bool bIsBossAttacking = false; // 공격중 여부
	FTimerHandle BossNormalAttackTimerHandle; // 일반공격 쿨타임 타이머

	void BossMoveAroundPlayer(float DeltaTime); // 플레이어 주위를 도는 함수
	void BossMoveToPlayer(); // 플레이어에게 이동하는 함수
	void BossNormalAttack(); // 일반공격 함수
    bool BossChasePlayer(); // MoveActor 상태에서 공격 가능하면 공격 상태로 전환하는 함수
	void UpdateBossAIState(float DistanceToPlayer); // 상태 업데이트 함수
	void SetBossAIState(EBossEnemyAIState NewState); // 상태 전환 함수
	void DrawDebugInfo(); // 상태별 디버그 시각화 함수

};
