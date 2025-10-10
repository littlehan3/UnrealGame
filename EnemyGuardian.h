#pragma once

#include "CoreMinimal.h" 
#include "GameFramework/Character.h" // ACharacter 클래스 상속
#include "EnemyGuardianAnimInstance.h" // 사용할 애님 인스턴스 클래스 포함
#include "Animation/AnimInstance.h" // UAnimInstance 클래스 사용
#include "EnemyGuardianShield.h" // 장착할 방패 클래스 포함
#include "EnemyGuardian.generated.h"

// 전방 선언
class AEnemyGuardianBaton;
class AEnemyGuardianShield;

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyGuardian : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyGuardian(); // 생성자
	virtual void Tick(float DeltaTime) override; // 매 프레임 호출되는 함수

	void PlaySpawnIntroAnimation(); // 스폰 시 등장 애니메이션을 재생하는 함수
	void SetUpAI(); // AI 초기 설정을 수행하는 함수
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
	void ApplyBaseWalkSpeed(); // 기본 이동 속도를 적용하는 함수
	void HideEnemy(); // 사망 후 액터 정리 및 숨김 처리 함수
	void StartAttack(); // 공격 판정 시작 함수 (애니메이션 노티파이용)
	void EndAttack(); // 공격 판정 종료 함수 (애니메이션 노티파이용)

	// 캐릭터 상태 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float Health = 300.0f; // 현재 체력

	bool bIsPlayingIntro = false; // 등장 애니메이션 재생 여부
	bool bIsShieldDestroyed = false; // 방패 파괴 여부
	bool bIsDead = false; // 사망 상태 여부
	bool bCanAttack = false; // 공격 가능 여부 (쿨타임 등)
	bool bIsAttacking = false; // 현재 공격 애니메이션 재생 여부 (사용되지 않음)
	bool bHasExecutedRaycast = false; // 이번 공격에서 레이캐스트 판정을 실행했는지 여부
	bool bIsStunned = false; // 현재 스턴 상태인지 여부
	bool bIsStunRecovering = false; // 스턴에서 회복 중인지 여부
	bool bIsShieldAttacking = false; // 현재 방패 공격 중인지 여부
	bool bIsBatonAttacking = false; // 현재 진압봉 공격 중인지 여부
	bool bProtectedShooterSeekingSight = false; // 슈터 시야 확보 관련 (현재 사용되지 않음)

	float Damage = 50.0f; // 기본 데미지 (사용되지 않음, 각 무기가 자체 데미지 소유)

	// 장착된 무기 참조
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	AEnemyGuardianShield* EquippedShield; // 현재 장착된 방패 액터

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	AEnemyGuardianBaton* EquippedBaton; // 현재 장착된 진압봉 액터

	// 공격 속성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ShieldAttackRadius = 150.0f; // AI가 방패 공격을 시작하는 거리

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BatonAttackRadius = 200.0f; // AI가 진압봉 공격을 시작하는 거리

	// 판정 데이터 (현재 사용되지 않음)
	TSet<AActor*> RaycastHitActors;
	TSet<AActor*> DamagedActors;

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출

private:
	// 몽타주 종료 시 호출될 함수들 (델리게이트 바인딩용)
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnShieldAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnStunMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 블루프린트에서 설정할 원본 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyGuardianShield> ShieldClass; // 생성할 방패 클래스

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyGuardianBaton> BatonClass; // 생성할 진압봉 클래스

	// 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* SpawnIntroMontage; // 등장 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> ShieldAttackMontages; // 방패 공격 애니메이션 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> NormalAttackMontages; // 진압봉 공격 애니메이션 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* BlockMontage; // 방어 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* HitMontage; // 피격 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> DeadMontages; // 사망 애니메이션 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* StunMontage; // 스턴 애니메이션

	// 타이머 핸들
	FTimerHandle StunTransitionTimer; // 스턴 상태 전환용 타이머 (현재 사용되지 않음)
	FTimerHandle DeathTimerHandle; // 사망 후 정리까지의 지연용 타이머
};