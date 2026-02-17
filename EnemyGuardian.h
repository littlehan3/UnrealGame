#pragma once

#include "CoreMinimal.h" 
#include "GameFramework/Character.h" // ACharacter 클래스 상속
//#include "EnemyGuardianAnimInstance.h" // 사용할 애님 인스턴스 클래스 포함
//#include "EnemyGuardianShield.h" // 장착할 방패 클래스 포함
#include "HealthInterface.h"
#include "EnemyGuardian.generated.h"

// 전방 선언
class AEnemyGuardianBaton;
class AEnemyGuardianShield;
class UNiagaraSystem;
class UAnimMontage;
class USoundBase;
class EnemyGuardianAIController;


UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyGuardian : public ACharacter, public IHealthInterface
{
	GENERATED_BODY()

public:
	AEnemyGuardian(); // 생성자

	void PlaySpawnIntroAnimation(); // 스폰 시 등장 애니메이션을 재생하는 함수
	//void SetUpAI(); // AI 초기 설정을 수행하는 함수
	void PlayShieldAttackAnimation(); // 방패 공격 애니메이션을 재생하는 함수
	void PlayNormalAttackAnimation(); // 일반 공격(진압봉) 애니메이션을 재생하는 함수
	void PlayBlockAnimation(); // 방어 애니메이션을 재생하는 함수
	void Stun(); // 방패 파괴 시 호출되는 스턴 함수

	// 데미지를 입었을 때 호출되는 함수 (AActor의 TakeDamage 오버라이드)
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,

		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	void Die(); // 사망 처리 함수
	//void HideEnemy(); // 사망 후 액터 정리 및 숨김 처리 함수
	void StartAttack(); // 공격 판정 시작 함수 (애니메이션 노티파이용)
	void EndAttack(); // 공격 판정 종료 함수 (애니메이션 노티파이용)

	void PlayWeaponHitSound();
	void PlayShieldHitSound();

	// MaxHealth 추가
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 600.0f; // 최대 체력

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float Health = 600.0f; // 현재 체력

	virtual float GetHealthPercent_Implementation() const override;
	virtual bool IsEnemyDead_Implementation() const override;

	FORCEINLINE class UNiagaraSystem* GetWeaponHitNiagaraEffect() const { return WeaponHitNiagaraEffect; }

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* HitMontage; // 피격 애니메이션

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsShieldDestroyed = false; // 방패 파괴 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsPlayingIntro = false; // 등장 애니메이션 재생 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsDead = false; // 사망 상태 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsStunned = false; // 현재 스턴 상태인지 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsShieldAttacking = false; // 현재 방패 공격 중인지 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsBatonAttacking = false; // 현재 진압봉 공격 중인지 여부

	// 공격 속성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ShieldAttackRadius = 150.0f; // AI가 방패 공격을 시작하는 거리

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BatonAttackRadius = 200.0f; // AI가 진압봉 공격을 시작하는 거리

	// 장착된 무기 참조
	UPROPERTY()
	AEnemyGuardianShield* EquippedShield; // 현재 장착된 방패 액터

	UPROPERTY()
	AEnemyGuardianBaton* EquippedBaton; // 현재 장착된 진압봉 액터

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출

private:

	UPROPERTY()
	class AEnemyGuardianAIController* AICon; // AI 컨트롤러 참조
	UPROPERTY()
	class UEnemyGuardianAnimInstance* AnimInstance; // 애니메이션 인스턴스 참조
	UPROPERTY()
	class UCharacterMovementComponent* MoveComp; // 이동 컴포넌트 참조

	void ApplyBaseWalkSpeed(); // 기본 이동 속도를 적용하는 함수
	void HideEnemy(); // 사망 후 액터 정리 및 숨김 처리 함수

	// 몽타주 종료 시 호출될 함수들 (델리게이트 바인딩용)
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnShieldAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnStunMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 블루프린트에서 설정할 원본 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEnemyGuardianShield> ShieldClass; // 생성할 방패 클래스

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEnemyGuardianBaton> BatonClass; // 생성할 진압봉 클래스

	// 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* SpawnIntroMontage; // 등장 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> ShieldAttackMontages; // 방패 공격 애니메이션 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> NormalAttackMontages; // 진압봉 공격 애니메이션 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* BlockMontage; // 방어 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> DeadMontages; // 사망 애니메이션 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* StunMontage; // 스턴 애니메이션

	// 타이머 핸들
	FTimerHandle StunTransitionTimer; // 스턴 상태 전환용 타이머 (현재 사용되지 않음)
	FTimerHandle DeathTimerHandle; // 사망 후 정리까지의 지연용 타이머

	UPROPERTY(EditDefaultsOnly, Category = "Effects | Niagara", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* WeaponHitNiagaraEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects | Sound", meta = (AllowPrivateAccess = "true"))
	class USoundBase* WeaponHitSound; // 무기 피격 사운드

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects | Sound", meta = (AllowPrivateAccess = "true"))
	class USoundBase* ShieldHitSound; // 무기 피격 사운드
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanAttack = false; // 공격 가능 여부 (쿨타임 등)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsStunRecovering = false; // 스턴에서 회복 중인지 여부
};