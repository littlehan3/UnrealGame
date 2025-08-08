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
	virtual float TakeDamage( // �������� �Ծ����� ȣ��Ǵ� �Լ� (AActor�� TakeDamage �������̵�)
		float DamageAmount, // ���� ������ ��
		struct FDamageEvent const& DamageEvent, // ������ �̺�Ʈ ���� 
		class AController* EventInstigator, // �������� ���� ��Ʈ�ѷ�
		AActor* DamageCauser // �������� ������ ����
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
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ��� ���¿��� ��Ʈ�� ���¸� �����ϱ� ���� �Լ�

	FTimerHandle DeathTimerHandle;
	FTimerHandle StunTimerHandle;
};
