#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" // ACharacter 클래스 상속
#include "HealthInterface.h"
#include "EnemyShooter.generated.h"

class AEnemyShooterGun; // 총 클래스 전방 선언
class AEnemyShooterGrenade; // 수류탄 클래스 전방 선언

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyShooter : public ACharacter, public IHealthInterface
{
	GENERATED_BODY()

public:
	AEnemyShooter(); // 생성자

	void PlaySpawnIntroAnimation(); // 스폰 시 등장 애니메이션을 재생하는 함수
	void PlayShootingAnimation(); // 사격 애니메이션을 재생하는 함수
	void PlayThrowingGrenadeAnimation(); // 수류탄 투척 애니메이션을 재생하는 함수

	// 데미지를 입었을때 호출되는 함수
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	void Die(); // 사망 처리 함수
	void ApplyBaseWalkSpeed(); // 기본 이동 속도를 적용하는 함수
	void HideEnemy(); // 사망 후 액터 정리 및 숨김 처리 함수
	void EnterInAirStunState(float Duration); // 공중 스턴 상태 진입 함수 
	void ExitInAirStunState(); // 공중 스턴 상태 해제 함수 
	void EnableGravityPull(FVector ExplosionCenter, float PullStrength); // 중력장으로 적을 잡아두는 함수
	void DisableGravityPull(); // 중력장 효과 해제 함수
	void Shoot(); // 장착된 총을 발사하는 함수

	// MaxHealth 추가 및 기존 Health 변수 수정
	UPROPERTY(EditAnywhere, Category = "Health")
	float MaxHealth = 200.0f; // 최대 체력

	UPROPERTY(EditAnywhere, Category = "Health")
	float Health = 200.0f; // 현재 체력

	/** IHealthInterface 구현 함수 */
	virtual float GetHealthPercent_Implementation() const override;
	virtual bool IsEnemyDead_Implementation() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsTrappedInGravityField = false; // 중력장 상태 여부

	FVector GravityFieldCenter = FVector::ZeroVector; // 중력장 중심 좌표

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsDead = false; // 사망 상태 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsInAirStun = false; // 공중 스턴 상태 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsPlayingIntro = false; // 등장 애니메이션 재생 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsAttacking = false; // 현재 공격 애니메이션 재생 여부

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyShooterGrenade> GrenadeClass; // 수류탄 클래스

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출

private:
	UPROPERTY()
	TObjectPtr<class AEnemyShooterAIController> AICon = nullptr; // AI 컨트롤러 캐싱을 위한 참조

	UPROPERTY()
	TObjectPtr<class UEnemyShooterAnimInstance> AnimInstance = nullptr; // 애니메이션 인스턴스 캐싱을 위한 참조

	UPROPERTY()
	TObjectPtr<class UCharacterMovementComponent> MoveComp = nullptr; // 무브 컴포넌트 캐싱을 위한 참조

	UPROPERTY()
	TObjectPtr<AEnemyShooterGun> EquippedGun = nullptr; // 현재 장착된 총에 대한 참조

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEnemyShooterGun> GunClass; // 총 클래스

	// 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> SpawnIntroMontage = nullptr; // 등장 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> ShootingMontage = nullptr; // 사격 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UAnimMontage>> HitMontages; // 피격 애니메이션 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UAnimMontage>> DeadMontages; // 사망 애니메이션 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> InAirStunMontage = nullptr; // 공중 스턴 상태 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> InAirStunDeathMontage = nullptr; // 공중 스턴 중 사망 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> ThrowingGrenadeMontage = nullptr; // 수류탄 투척 애니메이션

	// 몽타주 종료 시 호출될 함수들 (델리게이트 바인딩용)
	UFUNCTION()
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION()
	void OnShootingMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION()
	void OnThrowingGrenadeMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION()
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 타이머 핸들
	FTimerHandle StunTimerHandle; // 스턴 지속시간 제어용 타이머
	FTimerHandle DeathTimerHandle; // 사망 후 정리까지의 지연용 타이머
	FTimerHandle GravityDisableHandle; // 중력장 효과 해제 타이머

	void StopActions(); // 모든 행동을 즉시 중지시키는 함수

	UPROPERTY(VisibleAnywhere, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanAttack = false; // 공격 가능 여부 (쿨타임 등)
	UPROPERTY(VisibleAnywhere, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsShooting = false; // 현재 사격 중인지 여부
};