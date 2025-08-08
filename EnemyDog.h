#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyDogAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "EnemyDog.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyDog : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyDog();

	virtual void PostInitializeComponents() override;

	void PlaySpawnIntroAnimation();
	bool bIsPlayingIntro = false;

	void SetUpAI();
	void PlayNormalAttackAnimation();
	virtual float TakeDamage( // 데미지를 입었을때 호출되는 함수 (AActor의 TakeDamage 오버라이드)
		float DamageAmount, // 입은 데미지 양
		struct FDamageEvent const& DamageEvent, // 데미지 이벤트 정보 
		class AController* EventInstigator, // 데미지를 가한 컨트롤러
		AActor* DamageCauser // 데미지를 유발한 엑터
	) override;
	void Die();
	void Explode();
	void StopActions();
	void ApplyBaseWalkSpeed();
	void HideEnemy();
	void EnterInAirStunState(float Duration);
	void ExitInAirStunState();
	void ApplyGravityPull(FVector ExplosionCenter, float PullStrength);
	void StartAttack();
	void EndAttack();

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Health")
	float Health = 50.0f;
	bool bIsDead = false;
	bool bIsInAirStun = false;
	bool bCanAttack = false;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* SpawnIntroMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> NormalAttackMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> HitMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> DeadMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* InAirStunMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* InAirStunDeathMontage;

	UFUNCTION()
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 에어본 상태에서 히트시 상태를 관리하기 위한 함수

	FTimerHandle DeathTimerHandle;
	FTimerHandle StunTimerHandle;
};
