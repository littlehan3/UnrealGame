#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" // ACharacter 클래스를 상속받기 위해 포함
#include "EnemyDogAnimInstance.h" // 적 애니메이션 인스턴스 클래스를 사용하기 위해 포함
#include "Animation/AnimInstance.h" // UAnimInstance 클래스를 사용하기 위해 포함
#include "NiagaraSystem.h" // 나이아가라 이펙트 시스템을 사용하기 위해 포함
#include "HealthInterface.h"
#include "EnemyDog.generated.h"

class UNiagaraSystem;

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyDog : public ACharacter, public IHealthInterface
{
	GENERATED_BODY()

public:
	AEnemyDog(); // 생성자

	virtual void PostInitializeComponents() override; // 컴포넌트 초기화 이후 호출되는 함수

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
	// [수정] AEnemy와 동일하게 함수 이름 변경 및 Disable 함수 추가
	void EnableGravityPull(FVector ExplosionCenter, float PullStrength); //
	void DisableGravityPull();
	void RaycastAttack(); // 레이캐스트를 이용해 공격 판정을 하는 함수
	void StartAttack(); // 공격 시작 함수
	void EndAttack(); // 공격 종료 함수
	void PlayStunMontageLoop(); // 스턴 몽타주를 반복 재생하는 함수

	// 판정 데이터
	TSet<AActor*> RaycastHitActors; // 레이캐스트 공격에 맞은 액터들을 저장하는 Set
	TSet<AActor*> DamagedActors; // 이미 데미지를 입은 액터들을 저장하는 Set

	// HealthBar 연동을 위한 MaxHealth 추가
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 50.0f; // 최대 체력

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Health")
	float Health = 50.0f;

	bool bIsDead = false; // 사망 상태 여부
	bool bIsInAirStun = false; // 공중 스턴 상태 여부
	bool bCanAttack = false; // 공격 가능 여부
	bool bIsAttacking = false; // 현재 공격 중인지 여부
	bool bHasExecutedRaycast = false; // 이번 공격에서 레이캐스트 판정을 실행했는지 여부
	float Damage = 10.0f; // 일반 공격 데미지
	float ExplosionDamage = 40.0f; // 폭발 데미지

	/** IHealthInterface 구현 함수 */
	virtual float GetHealthPercent_Implementation() const override;
	virtual bool IsEnemyDead_Implementation() const override;

	// [신규 추가] 중력장(블랙홀) 상태 변수 (AEnemy와 동일)
	bool bIsTrappedInGravityField = false;
	FVector GravityFieldCenter = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> HitMontages; // 피격 몽타주 배열 (랜덤 재생용)

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출되는 함수

private:
	// 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* SpawnIntroMontage; // 등장 애니메이션 몽타주

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> NormalAttackMontages; // 일반 공격 몽타주 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> DeadMontages; // 사망 몽타주 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* InAirStunMontage; // 공중 스턴 상태 몽타주

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* InAirStunDeathMontage; // 공중 스턴 상태에서 사망 시 재생될 몽타주

	// 이펙트 및 사운드
	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* ExplosionEffect; // 폭발 시 재생될 나이아가라 이펙트

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* ExplosionSound; // 폭발 시 재생될 사운드

	// 몽타주 종료 시 호출될 함수들 (델리게이트 바인딩용)
	UFUNCTION()
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 등장 몽타주 종료 시

	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 피격 몽타주 종료 시 (에어본 상태 관리용)
	void OnDeadMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 사망 몽타주 종료 시
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 공격 몽타주 종료 시

	// 타이머 핸들
	FTimerHandle DeathTimerHandle; // 사망 후 폭발까지의 지연을 위한 타이머
	FTimerHandle StunTimerHandle; // 공중 스턴 지속시간을 제어하는 타이머
	FTimerHandle StunAnimRepeatTimerHandle; // 공중 스턴 애니메이션 반복을 위한 타이머

	// 공격 속성
	UPROPERTY(EditAnywhere, Category = "Missile")
	float ExplosionRadius = 100.f; // 폭발 반경

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	class UNiagaraSystem* WeaponHitNiagaraEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* AttackHitSound; // 공격이 성공했을 때 재생될 사운드

};