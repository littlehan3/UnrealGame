#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" // ACharacter 클래스를 상속받기 위해 포함
#include "HealthInterface.h"
#include "EnemyDog.generated.h"

class UNiagaraSystem;
class USoundBase;

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyDog : public ACharacter, public IHealthInterface
{
	GENERATED_BODY()

public:
	AEnemyDog();

	void PlaySpawnIntroAnimation(); // 스폰 시 등장 애니메이션을 재생하는 함수
	bool bIsPlayingIntro = false; // 등장 애니메이션 재생 여부

	void SetUpAI(); // AI 초기 설정을 수행하는 함수
	void PlayNormalAttackAnimation(); // 일반 공격 애니메이션을 재생하는 함수
	virtual float TakeDamage( // 데미지를 입었을때 호출되는 함수 (AActor의 TakeDamage 오버라이드)
		float DamageAmount, // 입은 데미지 양
		struct FDamageEvent const& DamageEvent, // 데미지 이벤트 정보 
		class AController* EventInstigator, // 데미지를 가한 컨트롤러
		AActor* DamageCauser // 데미지를 유발한 엑터
	) override;

	void Die(); // 사망 처리 함수
	void Explode(); // 폭발 효과 및 데미지 처리 함수
	void ApplyBaseWalkSpeed(); // 기본 이동 속도를 적용하는 함수
	void HideEnemy(); // 적 캐릭터를 숨기고 메모리에서 제거하는 함수
	void EnterInAirStunState(float Duration); // 공중 스턴(에어본) 상태로 전환하는 함수
	void ExitInAirStunState(); // 공중 스턴 상태를 해제하는 함수
	void EnableGravityPull(FVector ExplosionCenter, float PullStrength); // 중력장 효과를 활성화하는 함수
	void DisableGravityPull();
	void RaycastAttack(); // 레이캐스트를 이용해 공격 판정을 하는 함수
	void StartAttack(); // 공격 시작 함수
	void EndAttack(); // 공격 종료 함수
	void PlayStunMontageLoop(); // 스턴 몽타주를 반복 재생하는 함수

	// 판정 데이터
	TSet<AActor*> RaycastHitActors; // 레이캐스트 공격에 맞은 액터들을 저장하는 Set
	TSet<AActor*> DamagedActors; // 이미 데미지를 입은 액터들을 저장하는 Set

	// HealthBar 연동을 위한 MaxHealth 추가
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 50.0f; // 최대 체력

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Stats")
	float Health = 50.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "State")
	bool bIsDead = false; // 사망 상태 여부
	UPROPERTY(VisibleInstanceOnly, Category = "State")
	bool bIsInAirStun = false; // 공중 스턴 상태 여부
	UPROPERTY(VisibleInstanceOnly, Category = "State")
	bool bCanAttack = false; // 공격 가능 여부
	UPROPERTY(VisibleInstanceOnly, Category = "State")
	bool bIsAttacking = false; // 현재 공격 중인지 여부
	UPROPERTY(VisibleInstanceOnly, Category = "State")
	bool bHasExecutedRaycast = false; // 이번 공격에서 레이캐스트 판정을 실행했는지 여부
	UPROPERTY(VisibleInstanceOnly, Category = "State")
	bool bIsTrappedInGravityField = false;

	virtual float GetHealthPercent_Implementation() const override; // 현재 체력 비율 반환
	virtual bool IsEnemyDead_Implementation() const override; // 적이 사망했는지 여부 반환
	FVector GravityFieldCenter = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> HitMontages; // 피격 몽타주 배열 (랜덤 재생용)

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출되는 함수

private:
	UPROPERTY()
	class AEnemyDogAIController* AICon; // AI 컨트롤러 캐싱을 위한 참조

	UPROPERTY()
	class UEnemyDogAnimInstance* AnimInstance; // 애니메이션 인스턴스 캐싱을 위한 참조

	UPROPERTY()
	class UCharacterMovementComponent* MoveComp; // 무브 컴포넌트 캐싱을 위한 참조

	// 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* SpawnIntroMontage; // 등장 애니메이션 몽타주

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> NormalAttackMontages; // 일반 공격 몽타주 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> DeadMontages; // 사망 몽타주 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* InAirStunMontage; // 공중 스턴 상태 몽타주

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* InAirStunDeathMontage; // 공중 스턴 상태에서 사망 시 재생될 몽타주

	// 이펙트 및 사운드
	UPROPERTY(EditAnywhere, Category = "SoundEffects", meta = (AllowPrivateAccess = "true"))
	UNiagaraSystem* ExplosionEffect; // 폭발 시 재생될 나이아가라 이펙트

	UPROPERTY(EditAnywhere, Category = "SoundEffects", meta = (AllowPrivateAccess = "true"))
	USoundBase* ExplosionSound; // 폭발 시 재생될 사운드

	UPROPERTY(EditDefaultsOnly, Category = "NiagaraEffects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* WeaponHitNiagaraEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "SoundEffects", meta = (AllowPrivateAccess = "true"))
	USoundBase* AttackHitSound; // 공격이 성공했을 때 재생될 사운드

	// 몽타주 종료 시 호출될 함수들 (델리게이트 바인딩용)
	UFUNCTION()
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 등장 몽타주 종료 시
	UFUNCTION()
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 피격 몽타주 종료 시 (에어본 상태 관리용)
	UFUNCTION()
	void OnDeadMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 사망 몽타주 종료 시
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 공격 몽타주 종료 시

	// 타이머 핸들
	FTimerHandle DeathTimerHandle; // 사망 후 폭발까지의 지연을 위한 타이머
	FTimerHandle StunTimerHandle; // 공중 스턴 지속시간을 제어하는 타이머
	FTimerHandle StunAnimRepeatTimerHandle; // 공중 스턴 애니메이션 반복을 위한 타이머
	FTimerHandle GravityDisableHandle; // 중력장 해제 타이머 핸들

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float ExplosionRadius = 100.f; // 폭발 반경
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float Damage = 10.0f; // 일반 공격 데미지
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float ExplosionDamage = 40.0f; // 폭발 데미지

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Config", meta = (AllowPrivateAccess = "true"))
	float AttackTraceDistance = 150.0f; // 레이캐스트 사거리

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Config", meta = (AllowPrivateAccess = "true"))
	float ExplosionDelay = 0.5f; // 사망 후 폭발까지 지연 시간

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Config", meta = (AllowPrivateAccess = "true"))
	float AIUpdateInterval = 0.05f; // 틱 주기 (최적화용)

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Config", meta = (AllowPrivateAccess = "true"))
	float AirborneLaunchStrength = 600.0f; // 에어본 띄우기 강도

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Config", meta = (AllowPrivateAccess = "true"))
	float GravityDisableDelay = 0.3f; // 에어본 후 중력 정지 지연 시간

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Config", meta = (AllowPrivateAccess = "true"))
	float StunRepeatInterval = 0.4f; // 스턴 애니메이션 반복 주기

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Movement", meta = (AllowPrivateAccess = "true"))
	float BaseMaxWalkSpeed = 600.0f; // 기본 이동 속도

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Movement", meta = (AllowPrivateAccess = "true"))
	float BaseMaxAcceleration = 5000.0f; // 최대 가속도

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Movement", meta = (AllowPrivateAccess = "true"))
	float BaseBrakingFrictionFactor = 10.0f; // 제동 마찰력

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Config", meta = (AllowPrivateAccess = "true"))
	float AttackAnimSpeed = 1.5f; // 공격 애니메이션 재생 속도

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Config", meta = (AllowPrivateAccess = "true"))
	float GravityPullTolerance = 50.0f; // 중앙 고정 판정 범위

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Config", meta = (AllowPrivateAccess = "true"))
	float RecoveryGravityScale = 1.5f; // 스턴 해제 후 낙하 중력

};