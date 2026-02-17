#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BossEnemyAIController.generated.h"

class ABossEnemy;
class ABossEnemyAnimInstance;

// 보스 AI 상태 Enum
UENUM(BlueprintType)
enum class EBossEnemyAIState : uint8
{
	Idle, // 기본상태
	MoveToPlayer, // 플레이어에게 접근
	NormalAttack, // 근거리 공격
};

UCLASS()
class LOCOMOTION_API ABossEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABossEnemyAIController();

	void StopBossAI(); // AI 중지
	UFUNCTION()
	void OnBossNormalAttackMontageEnded(); // 근거리 공격 애니메이션 종료 처리
	UFUNCTION()
	void OnBossAttackTeleportEnded(); // 텔레포트 공격 종료 처리
	UFUNCTION()
	void OnBossRangedAttackEnded(); // 원거리 공격 종료 처리

	void SetBossAIState(EBossEnemyAIState NewState); // AI 상태 설정
	bool HandlePostTeleportPause(); // 텔레포트 후 일시정지 처리
	void HandlePostStealthRecovery(); // 스텔스 후 회복 처리

	UFUNCTION()
	bool CanExecuteStealthAttack() const; // 스텔스 공격 가능 여부 확인
	UFUNCTION()
	bool IsInOptimalStealthRange() const; // 스텔스 최적 거리 내에 있는지 확인
	UFUNCTION()
	bool IsExecutingStealthAttack() const; // 스텔스 공격 실행 중인지 확인
	UFUNCTION()
	void HandleStealthPhaseTransition(int32 NewPhase); // 스텔스 단계 전환 처리

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	//// 캐싱된 참조
	UPROPERTY()
	APawn* PlayerPawn = nullptr; // 플레이어 폰 캐싱을 위한 참조

	UPROPERTY()
	ABossEnemy* CachedBoss; // 보스 캐싱을 위한 참조 참조

	EBossEnemyAIState CurrentState = EBossEnemyAIState::Idle; // 기본적으로 Idle 상태로 시작

	UPROPERTY(EditDefaultsOnly, Category = "Stat", meta = (AllowPrivateAccess = "true"))
	float BossMoveRadius = 600.0f; // 보스가 플레이어에게 접근하는 반경
	UPROPERTY(EditDefaultsOnly, Category = "Stat", meta = (AllowPrivateAccess = "true"))
	float BossStandingAttackRange = 200.0f; // 보스가 서 있을 때의 공격 범위
	UPROPERTY(EditDefaultsOnly, Category = "Stat", meta = (AllowPrivateAccess = "true"))
	float BossMovingAttackRange = 250.0f; // 보스가 이동 중일 때의 공격 범위
	UPROPERTY(EditDefaultsOnly, Category = "Stat", meta = (AllowPrivateAccess = "true"))
	float BossDetectRadius = 3000.0f; // 보스가 플레이어를 감지하는 반경
	UPROPERTY(EditDefaultsOnly, Category = "Stat", meta = (AllowPrivateAccess = "true"))
	float BossAttackCooldown = 1.0f; // 보스 공격 쿨다운 시간
	UPROPERTY(EditDefaultsOnly, Category = "Stat", meta = (AllowPrivateAccess = "true"))
	float StealthAttackOptimalRange = 250.0f; // 스텔스 공격 최적 거리
	UPROPERTY(EditDefaultsOnly, Category = "Stat", meta = (AllowPrivateAccess = "true"))
	float StealthAttackMinRange = 200.0f; // 스텔스 공격 최소 거리
	UPROPERTY(EditDefaultsOnly, Category = "Stat", meta = (AllowPrivateAccess = "true"))
	float RangedAttackCooldown = 10.0f; // 원거리 공격 쿨다운 시간

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	float LastRangedAttackTime = -FLT_MAX; // 마지막 원거리 공격 시간 기록
	UPROPERTY(VisibleAnywhere, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIgnoreRangedCooldownOnce = false; // 원거리 공격 쿨다운 무시 플래그
	UPROPERTY(VisibleAnywhere, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanBossAttack = true; // 보스 공격 가능 플래그
	UPROPERTY(VisibleAnywhere, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsBossAttacking = false; // 보스가 공격 중인지 여부
	UPROPERTY(VisibleAnywhere, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsAIDisabledForStealth = false; // 스텔스 중 AI 비활성화 플래그

	FTimerHandle BossNormalAttackTimerHandle; // 근거리 공격 타이머 핸들

	void BossMoveToPlayer(); // 플레이어에게 이동
	void BossNormalAttack(); // 근거리 공격
	void UpdateBossAIState(float DistanceToPlayer); // AI 상태 업데이트

	bool IsBossBusy() const; // 보스가 바쁜지 확인
	bool ShouldHoldForIntroOrStealth(); // 인트로 또는 스텔스 중지 여부
	bool ShouldHoldForBossAction(); // 보스 액션 중지 여부
	void HandleIdleRecovery(float DistanceToPlayer); // 유휴 상태 복귀 처리
	bool IsBossInHitReaction() const; // 피격 리액션 상태 확인
	bool TryStartStealthAttack(); // 스텔스 공격 시도
	bool TryStartCloseRangeAttack(float DistanceToPlayer); // 근거리 공격 시도
	bool TryStartMidRangeAttack(float DistanceToPlayer, float CurrentTime); // 중거리 공격 시도
	bool CanDoRangedAttack(float CurrentTime) const; // 원거리 공격 가능 여부 확인
};
