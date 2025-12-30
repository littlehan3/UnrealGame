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
	//MoveAroundPlayer,
	//ChasePlayer,
	MoveToPlayer,
	NormalAttack,
	//BackDash,
	//ProjectileAttack,
	//SpecialAttack,

	//JumpAttack,
	//Dash
};

UCLASS()
class LOCOMOTION_API ABossEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABossEnemyAIController();
	void StopBossAI(); // AI 동작 중지 함수
	void OnBossNormalAttackMontageEnded();
	void SetBossAIState(EBossEnemyAIState NewState); // 상태 전환 함수
	void OnBossAttackTeleportEnded(); // 공격용 텔레포트 종료 알림
	void OnBossRangedAttackEnded(); // 원거리 공격 종료 알림

	// 스텔스 공격 관련 함수들
	UFUNCTION()
	bool CanExecuteStealthAttack() const;

	UFUNCTION()
	bool IsInOptimalStealthRange() const;

	UFUNCTION()
	bool IsExecutingStealthAttack() const;

	// 스텔스 단계별 AI 제어 함수들
	UFUNCTION()
	void HandleStealthPhaseTransition(int32 NewPhase);

protected:
	virtual void BeginPlay() override; // Beginplay 오버라이드로 초기화
	virtual void Tick(float DeltaTime) override; // Tick 오버라이드로 프레임별 갱신

private:
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossMoveRadius = 600.0f; // 원 반지름

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossStandingAttackRange = 200.0f; // 전신 공격 범위

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossMovingAttackRange = 250.0f; // 상체분리 공격 범위

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossDetectRadius = 3000.0f; // 보스가 플레이어를 인식하는 거리

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossAttackCooldown = 1.0f; // 공격 쿨타임 1초

	EBossEnemyAIState CurrentState = EBossEnemyAIState::Idle;


	APawn* PlayerPawn = nullptr; // 플레이어 참조
	bool bCanBossAttack = true; // 공격 가능여부
	bool bIsBossAttacking = false; // 공격중 여부
	FTimerHandle BossNormalAttackTimerHandle; // 일반공격 쿨타임 타이머

	void BossMoveToPlayer(); // 플레이어에게 이동하는 함수
	void BossNormalAttack(); // 일반공격 함수

	void UpdateBossAIState(float DistanceToPlayer); // 상태 업데이트 함수
	//void DrawDebugInfo(); // 디버그 시각화 함수

	// 스텔스 공격 설정값들
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float StealthAttackOptimalRange = 300.0f; // 스텔스 공격 최적 거리

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float StealthAttackMinRange = 200.0f; // 스텔스 공격 최소 거리

	// AI 상태 관리
	bool bIsAIDisabledForStealth = false; // 스텔스 중 AI 비활성화 여부

	float RangedAttackCooldown = 10.0f; // 투사체 공격 쿨타임
	float LastRangedAttackTime = -FLT_MAX; // 마지막 원거리 공격 시간
	bool bIgnoreRangedCooldownOnce = false; // 뒷텔레포트 직후 1회 무조건 허용하는 플래그 

};