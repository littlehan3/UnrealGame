#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HealthInterface.h"
#include "BossEnemy.generated.h"

class ABossProjectile;
class AEnemyBossKatana;
class UNiagaraSystem;
class ABossEnemyAIController;
class UBossEnemyAnimInstance;

// 보스 전체 상태 Enum
UENUM(BlueprintType)
enum class EBossState : uint8
{
	Idle,           // 대기
	Intro,          // 등장 애니메이션
	Combat,         // 전투 가능 상태
	Teleporting,    // 후퇴 텔레포트
	AttackTeleport, // 공격 텔레포트
	RangedAttack,   // 원거리 공격
	NormalAttack,   // 근접 공격
	UpperBodyAttack,// 상체 공격
	StealthAttack,  // 스텔스 공격
	HitReaction,    // 피격 반응
	Dead            // 사망
};

// 스텔스 세부 단계 Enum
UENUM(BlueprintType)
enum class EStealthPhase : uint8
{
	None = 0,	// 0단계: 없음 (스텔스 종료,  AI 복구 시점)
	Starting,   // 1단계: 스텔스 시작 애니메이션
	Diving,     // 2단계: 스텔스 다이빙
	Invisible,  // 3단계: 스텔스 시 투명화
	Kicking,    // 4단계: 스텔스 종료 후 킥 공격
	Finishing   // 6단계: 스텔스 킥 적중시 피니시 공격
};

UCLASS(Blueprintable)
class LOCOMOTION_API ABossEnemy : public ACharacter, public IHealthInterface
{
	GENERATED_BODY()

public:
	ABossEnemy();

	void PlayBossNormalAttackAnimation(); // 근접 공격 애니메이션 재생
	void PlayBossUpperBodyAttackAnimation(); // 상체 공격 애니메이션 재생
	void PlayBossTeleportAnimation(); // 후퇴 텔레포트 애니메이션 재생
	void PlayBossSpawnIntroAnimation(); // 등장 애니메이션 재생
	void PlayBossAttackTeleportAnimation(); // 공격 텔레포트 애니메이션 재생
	void PlayBossRangedAttackAnimation(); // 원거리 공격 애니메이션 재생
	void PlayBossStealthAttackAnimation(); // 스텔스 공격 애니메이션 재생

	// 스텔스 공격 단계 함수
	void StartStealthDivePhase(); // 스텔스 다이빙
	void StartStealthInvisiblePhase(); // 스텔스 투명화
	FVector CalculateRandomTeleportLocation(); // 랜덤 위치 계산
	void ExecuteStealthKick(); // 킥 공격
	void ExecuteStealthKickRaycast(); // 킥 공격 레이캐스트
	void LaunchPlayerIntoAir(APawn* PlayerPawn, float LaunchHeight); // 플레이어 공중 발사
	void ExecuteStealthFinish(); // 피니시 공격
	void ExecuteStealthFinishRaycast(); // 피니시 공격 레이캐스트
	void EndStealthAttack(); // 스텔스 공격 종료
	void UpdateStealthTeleportLocation(); // 투명화 중 위치 업데이트

	UFUNCTION()
	void OnStealthCooldownEnd(); // 스텔스 쿨다운 종료

	// 데미지 처리 오버라이드
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	// 상태 쿼리 함수 AIController용
	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	EBossState GetCurrentState() const { return CurrentState; } // 현재 보스 상태 반환

	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	EStealthPhase GetStealthPhase() const { return CurrentStealthPhase; } // 현재 스텔스 단계 반환

	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	bool IsInState(EBossState State) const { return CurrentState == State; } // 특정 상태인지 확인

	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	bool IsDead() const { return CurrentState == EBossState::Dead; } // 사망 상태인지 확인

	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	bool IsInCombatState() const { return CurrentState == EBossState::Combat; } // 전투 상태인지 확인

	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	bool IsPerformingAction() const; // 액션 수행 중인지 확인

	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	bool IsExecutingStealthAttack() const; // 스텔스 공격 실행 중인지 확인

	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	bool CanPerformAttack() const { return bCanBossAttack && CurrentState == EBossState::Combat; } // 공격 가능 여부 확인

	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	bool CanTeleport() const { return bCanTeleport; } // 텔레포트 가능 여부 확인

	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	bool CanUseStealthAttack() const { return bCanUseStealthAttack; } // 스텔스 공격 가능 여부 확인

	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	bool IsInvincible() const { return bIsInvincible; } // 무적 상태인지 확인

	void SetCanBossAttack(bool bValue) { bCanBossAttack = bValue; } // AI 컨트롤러용 공격 가능 플래그 설정

	void StartAttack(); // 공격 시작
	void EndAttack(); // 공격 종료
	void BossDie(); // 보스 사망 처리

	UFUNCTION()
	void SpawnBossProjectile(); // 보스 투사체 생성

	// IHealthInterface 연동 함수
	virtual float GetHealthPercent_Implementation() const override; // 체력 백분율 반환
	virtual bool IsEnemyDead_Implementation() const override; // 적이 사망했는지 반환

	void PlayWeaponHitSound(); // 무기 히트 사운드 재생

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossHitReactionMontages; // 히트 몽타주 배열

	FORCEINLINE class UNiagaraSystem* GetWeaponHitNiagaraEffect() const { return WeaponHitNiagaraEffect; } // 무기 히트 이펙트 반환

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxBossHealth = 2000.0f; // 최대 체력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float BossHealth = 2000.0f; // 현재 체력

	bool bShouldUseRangedAfterTeleport = false; // 텔레포트 후 원거리 공격 여부

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	AEnemyBossKatana* EquippedBossKatana; // 카타나 참조

	UPROPERTY()
	class ABossEnemyAIController* AICon; // AI 컨트롤러 캐싱을 위한 참조
	UPROPERTY()
	class UBossEnemyAnimInstance* AnimInstance; // 애니메이션 인스턴스 캐싱을 위한 참조
	UPROPERTY()
	class UCharacterMovementComponent* MoveComp; // 무브 컴포넌트 캐싱을 위한 참조

	UPROPERTY(VisibleAnywhere, Category = "Boss|State")
	EBossState CurrentState = EBossState::Idle; // 현재 보스 상태

	UPROPERTY(VisibleAnywhere, Category = "Boss|State")
	EStealthPhase CurrentStealthPhase = EStealthPhase::None; // 현재 스텔스 단계

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanTeleport = true; // 텔레포트 가능 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanUseStealthAttack = true; // 스텔스 공격 가능 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanBossAttack = false; // 공격 가능 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsInvincible = false; // 무적 상태 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bHasExecutedKickRaycast = false; // 킥 레이캐스트 실행 여부

	UPROPERTY()
	USoundBase* BossHitSound; // 피격 사운드
	UPROPERTY()
	USoundBase* BossDieSound; // 사망 사운드
	UPROPERTY()
	USoundBase* BossNormalAttackSound; // 근접 공격 사운드
	UPROPERTY()
	USoundBase* BossStrongAttackSound; // 강력한 공격 사운드
	UPROPERTY()
	USoundBase* BossWeaponHitSound; // 무기 히트 사운드

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> BossNormalAttackMontages; // 근접 공격 몽타주 배열
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> BossUpperBodyMontages; // 상체 공격 몽타주 배열
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> BossDeadMontages; // 사망 몽타주 배열
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* StealthStartMontage; // 스텔스 시작 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* StealthDiveMontage; // 스텔스 다이빙 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* StealthKickMontage; // 스텔스 킥 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* StealthFinishMontage; //	스텔스 피니시 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> BossTeleportMontages; // 텔레포트 몽타주 배열
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> BossSpawnIntroMontages; // 등장 몽타주 배열
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> BossAttackTeleportMontages; // 공격 텔레포트 몽타주 배열
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> BossRangedAttackMontages; // 원거리 공격 몽타주 배열

	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float TeleportDistance = 500.0f; // 텔레포트 거리
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float TeleportCooldown = 10.0f; // 텔레포트 쿨다운
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float AttackTeleportRange = 150.0f; // 공격 텔레포트 범위
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float PostTeleportPauseTime = 0.5f; // 텔레포트 후 일시정지 시간
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float StealthCooldown = 30.0f; // 스텔스 공격 쿨다운

	FTimerHandle TeleportExecutionTimer; // 텔레포트 실행 타이머
	FTimerHandle AttackTeleportExecutionTimer; // 공격 텔레포트 실행 타이머
	FTimerHandle TeleportCooldownTimer; // 텔레포트 쿨다운 타이머
	FTimerHandle BossDeathHideTimerHandle; // 보스 사망 숨김 타이머
	FTimerHandle PostTeleportPauseTimer; // 텔레포트 후 일시정지 타이머
	FTimerHandle StealthWaitTimer; // 스텔스 대기 타이머
	FTimerHandle PlayerAirborneTimer; // 플레이어 공중 체류 타이머
	FTimerHandle StealthCooldownTimer; // 스텔스 쿨다운 타이머
	FTimerHandle StealthDiveTransitionTimer; // 스텔스 다이빙 전환 타이머
	FTimerHandle StealthKickExecutionTimer; // 스텔스 킥 실행 타이머

	FVector CalculatedTeleportLocation; // 계산된 텔레포트 위치

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEnemyBossKatana> BossKatanaClass; // 보스 카타나 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class ABossProjectile> BossProjectileClass; // 보스 투사체 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
	FVector MuzzleOffset = FVector(100.f, 0.f, 80.f); // 투사체 발사 위치 오프셋
	UPROPERTY(EditDefaultsOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	UNiagaraSystem* StealthFinishEffect; // 스텔스 피니시 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	USoundBase* StealthFinishSound; // 스텔스 피니시 사운드
	UPROPERTY(EditDefaultsOnly, Category = "FX")
	class UNiagaraSystem* WeaponHitNiagaraEffect = nullptr; // 무기 히트 이펙트

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Movement", meta = (AllowPrivateAccess = "true"))
	float BaseMaxWalkSpeed = 300.0f; // 기본 이동 속도
	UPROPERTY(EditDefaultsOnly, Category = "Stats | Movement", meta = (AllowPrivateAccess = "true"))
	float BaseMaxAcceleration = 5000.0f; // 최대 가속도
	UPROPERTY(EditDefaultsOnly, Category = "Stats | Movement", meta = (AllowPrivateAccess = "true"))
	float BaseBrakingFrictionFactor = 10.0f; // 제동 마찰력

	// 상태 전환 함수
	void SetState(EBossState NewState); // 보스 상태 설정
	void SetStealthPhase(EStealthPhase NewPhase); // 스텔스 단계 설정

	// 몽타주 재생 헬퍼 (배열에서 랜덤 선택)
	bool PlayRandomMontage(
		const TArray<UAnimMontage*>& Montages,
		void(ABossEnemy::* EndCallback)(UAnimMontage*, bool),
		bool bUseUpperBodyBlend = false,
		bool bLookAtPlayer = false,
		float LookAtSpeed = 8.0f);

	// 단일 몽타주 재생 헬퍼
	bool PlaySingleMontage(
		UAnimMontage* Montage,
		void(ABossEnemy::* EndCallback)(UAnimMontage*, bool),
		bool bUseUpperBodyBlend = false,
		bool bLookAtPlayer = false,
		float LookAtSpeed = 8.0f);
	// 텔레포트 몽타주 재생 헬퍼
	bool PlayTeleportMontage(
		const TArray<UAnimMontage*>& Montages,
		float TeleportTimingRatio,
		FTimerHandle& ExecutionTimer,
		void(ABossEnemy::* EndCallback)(UAnimMontage*, bool),
		TFunction<void()> TeleportAction);

	// 이동 제어
	void ApplyBaseWalkSpeed(); // 기본 걷기 속도 적용
	void DisableBossMovement(); // 이동 비활성화
	void EnableBossMovement(); // 이동 활성화
    void StopAIMovementAndDisable(); // AI 이동 정지 + 이동 비활성화
    void ResetUpperBodyBlend(); // 상체 블렌드 해제
    void ResetLookAt(float LookAtSpeed = 5.0f); // 바라보기 해제

	// AI 셋업
	void SetUpBossAI(); // 보스 AI 설정
	void StopBossActions(); // 보스 액션 중지
	void HideBossEnemy(); // 보스 숨기기

	// 안전한 타이머 설정 헬퍼
	void SetSafeTimer(FTimerHandle& Handle, float Time, TFunction<void()> Callback, bool bLoop = false); // 타이머 설정
	void SetSafeTimerForNextTick(TFunction<void()> Callback); // 다음 틱에 타이머 설정

	// 텔레포트
	void ExecuteTeleport(); // 텔레포트 실행
	FVector CalculateTeleportLocation(); // 텔레포트 위치 계산
	FVector AdjustHeightForCharacter(const FVector& TargetLocation); // 캐릭터 높이 조정
	void OnTeleportCooldownEnd(); // 텔레포트 쿨다운 종료
	void ExecuteAttackTeleport(); // 공격 텔레포트 실행
	FVector CalculateAttackTeleportLocation(); // 공격 텔레포트 위치 계산
	void OnPostTeleportPauseEnd(); // 텔레포트 후 일시정지 종료

	UFUNCTION()
	void OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 근접 공격 몽타주 종료 콜백
	UFUNCTION()
	void OnUpperBodyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 상체 공격 몽타주 종료 콜백
	UFUNCTION()
	void OnHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 히트 몽타주 종료 콜백
	UFUNCTION()
	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 사망 몽타주 종료 콜백
	UFUNCTION()
	void OnTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 텔레포트 몽타주 종료 콜백
	UFUNCTION()
	void OnBossIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 등장 몽타주 종료 콜백
	UFUNCTION()
	void OnAttackTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 공격 텔레포트 몽타주 종료 콜백
	UFUNCTION()
	void OnRangedAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 원거리 공격 몽타주 종료 콜백
	UFUNCTION()
	void OnStealthStartMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 스텔스 시작 몽타주 종료 콜백
	UFUNCTION()
	void OnStealthDiveMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 스텔스 다이빙 몽타주 종료 콜백
	UFUNCTION()
	void OnStealthFinishMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 스텔스 피니시 몽타주 종료 콜백
	UFUNCTION()
	void OnStealthKickMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 스텔스 킥 몽타주 종료 콜백
};