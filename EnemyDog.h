#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyDogAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "NiagaraSystem.h"
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
	virtual float TakeDamage( // �������� �Ծ����� ȣ��Ǵ� �Լ� (AActor�� TakeDamage �������̵�)
		float DamageAmount, // ���� ������ ��
		struct FDamageEvent const& DamageEvent, // ������ �̺�Ʈ ���� 
		class AController* EventInstigator, // �������� ���� ��Ʈ�ѷ�
		AActor* DamageCauser // �������� ������ ����
	) override;
	void Die();
	void Explode();
	void ApplyBaseWalkSpeed();
	void HideEnemy();
	void EnterInAirStunState(float Duration);
	void ExitInAirStunState();
	void ApplyGravityPull(FVector ExplosionCenter, float PullStrength);
	void RaycastAttack();
	void StartAttack();
	void EndAttack();
	void PlayStunMontageLoop();

	// ���� ������
	TSet<AActor*> RaycastHitActors;
	TSet<AActor*> DamagedActors;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Health")
	float Health = 50.0f;
	bool bIsDead = false;
	bool bIsInAirStun = false;
	bool bCanAttack = false;
	bool bIsAttacking = false;
	bool bHasExecutedRaycast = false;
	float Damage = 10.0f;
	float ExplosionDamage = 40.0f;

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

	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* ExplosionEffect;

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* ExplosionSound;

	UFUNCTION()
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ��� ���¿��� ��Ʈ�� ���¸� �����ϱ� ���� �Լ�
	void OnDeadMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	FTimerHandle DeathTimerHandle;
	FTimerHandle StunTimerHandle;
	FTimerHandle StunAnimRepeatTimerHandle;

	UPROPERTY(EditAnywhere, Category = "Missile")
	float ExplosionRadius = 100.f;

};
