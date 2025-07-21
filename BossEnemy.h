#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossEnemyAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "BossEnemy.generated.h"

class ABossProjectile;

UCLASS()
class LOCOMOTION_API ABossEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	ABossEnemy();

	virtual void PostInitializeComponents() override; // AI 이동

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

	float BossHealth = 200.0f; // 체력

	UFUNCTION() // 애님 노티파이에서 호출할 함수
	void SpawnBossProjectile();

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

	void BossDie(); // 사망 함수
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
	float TeleportCooldown = 3.0f; // 텔레포트 쿨타임

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

};
