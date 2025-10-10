#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossEnemyAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraSystem.h"
#include "BossEnemy.generated.h"

class ABossProjectile;
class AEnemyBossKatana;

UCLASS(Blueprintable)
class LOCOMOTION_API ABossEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	ABossEnemy();

	virtual void PostInitializeComponents() override; // AI 컨트롤러 확인 함수

	void PlayBossNormalAttackAnimation(); // 일반공격 애니메이션 실행 함수
	void PlayBossUpperBodyAttackAnimation(); // 상체 전용 애니메이션 실행 함수
	void PlayBossTeleportAnimation(); // 텔레포트 함수
	bool bIsBossTeleporting = false; // 텔레포트 중 여부
	bool bCanTeleport = true; // 텔레포트 가능 여부
	void PlayBossSpawnIntroAnimation(); // 보스 등장 애니메이션 재생 함수
	bool bIsPlayingBossIntro = false; // 보스 등장 애니메이션 재생 중 여부
	void PlayBossAttackTeleportAnimation(); // 공격용 텔레포트 애니메이션 재생 함수
	void PlayBossRangedAttackAnimation(); // 원거리 공격 애니메이션 재생 함수
	bool bIsBossAttackTeleporting = false; // 공격용 텔레포트 중 여부
	bool bIsBossRangedAttacking = false; // 원거리 공격 중 여부
	bool bShouldUseRangedAfterTeleport = false; // 텔레포트 후 원거리 공격 사용 여부 
	bool bIsInvincible = false; // 무적상태 여부

	void PlayBossStealthAttackAnimation(); // 1단계 스텔스 시작 에니메이션 실행 함수
	void StartStealthDivePhase(); // 2단계 스텔스 진입 함수
	void StartStealthInvisiblePhase(); // 3단계 스텔스 함수
	FVector CalculateRandomTeleportLocation(); // 4단계 스텔스 텔레포트 위치 계산 함수
	void ExecuteStealthKick(); // 5단계 스텔스 킥 함수
	void ExecuteStealthKickRaycast(); // 스텔스 킥 레이캐스트 함수
	void LaunchPlayerIntoAir(APawn* PlayerPawn, float LaunchHeight); // 스텔스 킥 에어본 함수
	void ExecuteStealthFinish(); // 6단계 스텔스 피니쉬 함수
	void ExecuteStealthFinishRaycast(); // 스텔스 피니쉬 레이캐스트 함수
	void EndStealthAttack(); // 스텔스 공격 종료 함수
	bool bCanUseStealthAttack = true; // 스텔스 공격 사용 가능 여부
	bool bIsStealthStarting = false;  // 스텔스 시작 중
	bool bIsStealthDiving = false;    // 뛰어드는 중
	bool bIsStealthInvisible = false; // 완전 투명 상태
	bool bIsStealthKicking = false;   // 킥 공격 중
	bool bIsStealthFinishing = false; // 피니쉬 공격 중
	void UpdateStealthTeleportLocation(); // 스텔스 텔레포트 위치 업데이트 함수

	UFUNCTION()
	void OnStealthCooldownEnd(); // 스텔스 쿨타임 종료

	
	virtual float TakeDamage( // 데미지를 입었을때 호출되는 함수 (AActor의 TakeDamage 오버라이드)
		float DamageAmount, // 입은 데미지 양
		struct FDamageEvent const& DamageEvent, // 데미지 이벤트 정보
		class AController* EventInstigator, // 데미지를 가한 컨트롤러
		AActor* DamageCauser // 데미지를 유발한 엑터
	) override;

	bool bIsBossDead = false; // 사망 여부
	bool bIsBossStrongAttacking = false; // 강공격중 여부
	bool bCanBossAttack = false; // 공격 가능 여부
	bool bIsBossHit = false; // 피격중 여부
	bool bIsFullBodyAttacking = false;// 전신공격중 여부
	bool bHasExecutedKickRaycast = false; // 킥 레이캐스트 여부

	float BossHealth = 200.0f; // 체력

	UFUNCTION() // 애님 노티파이에서 호출할 함수
	void SpawnBossProjectile();

	// 보스 전용 카타나 참조
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	AEnemyBossKatana* EquippedBossKatana;

	void StartAttack();
	void EndAttack();
	void BossDie(); // 사망 함수

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossHitSound; // 피격 사운드

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossDieSound; // 사망 사운드

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossNormalAttackSound; // 일반공격 사운드

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossStrongAttackSound; // 강공격 사운드

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UBossEnemyAnimInstance* BossEnemyAnimInstance; // 애니메이션 인스턴스 추가

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossNormalAttackMontages; // 일반 공격 몽타주 배열

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossUpperBodyMontages; // 상체 전용 몽타주 배열

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossHitReactionMontages; // 피격 몽타주 배열

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossDeadMontages; // 사망 몽타주 배열

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* StealthStartMontage; // 1단계: 스텔스 시작 몽타주

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* StealthDiveMontage; // 2단계: 뛰어드는 몽타주

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* StealthKickMontage; // 5단계: 스텔스 킥 몽타주

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* StealthFinishMontage; // 6단계: 스텔스 피니쉬 몽타주

	void SetUpBossAI(); // AI가 네브매쉬에서 이동할수있게 설정하는 함수
	void StopBossActions(); // 모든 동작 정지 함수
	void HideBossEnemy(); // 사망시 보스를 숨기는 함수
	FTimerHandle BossDeathHideTimerHandle; // 사망 몽타주 타이머
	void DisableBossMovement();
	void EnableBossMovement();

	UFUNCTION()
	void OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 일반공격 몽타주 델리게이트

	UFUNCTION()
	void OnUpperBodyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 상체공격 몽타주 델리게이트

	UFUNCTION()
	void OnHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 히트 몽타주 델리게이트

	UFUNCTION()
	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 사망 몽타주 델리게이트

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float TeleportDistance = 500.0f; // 텔레포트 거리

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float TeleportCooldown = 10.0f; // 텔레포트 쿨타임

	FTimerHandle TeleportExecutionTimer; // 텔레포트 이동 타이머
	FTimerHandle AttackTeleportExecutionTimer; // 공격 텔레포트 이동 타이머
	FTimerHandle TeleportCooldownTimer; // 텔레포트 쿨타임 타이머

	void OnTeleportCooldownEnd(); // 텔레포트 쿨타임 종료 함수

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossTeleportMontages; // 텔레포트 몽타주 배열

	UFUNCTION()
	void OnTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 텔레포트 몽타주 델리게이트

	void ExecuteTeleport(); // 실제 텔레포트 실행 함수 (몽타주 중간에 호출)
	FVector CalculateTeleportLocation(); // 텔레포트 위치 계산 함수
	FVector AdjustHeightForCharacter(const FVector& TargetLocation); // 캐릭터 높이 보정 함수

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossSpawnIntroMontages; // 보스 등장 몽타주 배열

	UFUNCTION()
	void OnBossIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 보스 등장 몽타주 종료 델리게이트

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossAttackTeleportMontages; // 공격용 텔레포트 몽타주 배열

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossRangedAttackMontages; // 원거리 공격 몽타주 배열

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackTeleportRange = 150.0f; // 공격용 텔레포트 거리 (플레이어 근처로)

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float PostTeleportPauseTime = 0.5f; // 텔레포트 후 정지 시간

	UFUNCTION()
	void OnAttackTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 공격용 텔레포트 몽타주 델리게이트

	UFUNCTION()
	void OnRangedAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 원거리 공격 몽타주 델리게이트

	void ExecuteAttackTeleport(); // 실제 공격용 텔레포트 실행 함수
	FVector CalculateAttackTeleportLocation(); // 공격용 텔레포트 위치 계산 함수
	void OnPostTeleportPauseEnd(); // 텔레포트 후 정지 종료 함수

	FTimerHandle PostTeleportPauseTimer; // 텔레포트 후 정지 타이머

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class ABossProjectile> BossProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	FVector MuzzleOffset = FVector(100.f, 0.f, 80.f); // 보스 발사 위치 오프셋

	UFUNCTION()
	void OnStealthStartMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 스텔스 1단계 몽타주 델리게이트
	UFUNCTION()
	void OnStealthDiveMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 스텔스 2단계 몽타주 델리게이트
	UFUNCTION()
	void OnStealthFinishMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 스텔스 6단계 몽타주 델리게이트

	// 스텔스 단계 관리
	UPROPERTY(VisibleAnywhere, Category = "Stealth")
	int32 CurrentStealthPhase = 0;  // 현재 스텔스 단계

	// 타이머들
	FTimerHandle StealthWaitTimer;  // 스텔스 대기 타이머
	FTimerHandle PlayerAirborneTimer; // 플레이어 공중 체류 타이머

	// 계산된 텔레포트 위치
	FVector CalculatedTeleportLocation;

	// 스텔스 관련 추가 변수들
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float StealthCooldown = 30.0f; // 스텔스 쿨타임

	FTimerHandle StealthCooldownTimer; // 스텔스 쿨타임 타이머
	FTimerHandle StealthDiveTransitionTimer;// 스텔스 다이브 퍼센트 전환 타이머

	//BP에서 Katana Blueprint 지정
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyBossKatana> BossKatanaClass;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* StealthFinishEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	USoundBase* StealthFinishSound;
};
