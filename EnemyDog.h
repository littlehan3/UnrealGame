#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" // ACharacter Ŭ������ ��ӹޱ� ���� ����
#include "EnemyDogAnimInstance.h" // �� �ִϸ��̼� �ν��Ͻ� Ŭ������ ����ϱ� ���� ����
#include "Animation/AnimInstance.h" // UAnimInstance Ŭ������ ����ϱ� ���� ����
#include "NiagaraSystem.h" // ���̾ư��� ����Ʈ �ý����� ����ϱ� ���� ����
#include "EnemyDog.generated.h"

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyDog : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyDog(); // ������

	virtual void PostInitializeComponents() override; // ������Ʈ �ʱ�ȭ ���� ȣ��Ǵ� �Լ�

	void PlaySpawnIntroAnimation(); // ���� �� ���� �ִϸ��̼��� ����ϴ� �Լ�
	bool bIsPlayingIntro = false; // ���� �ִϸ��̼� ��� ����

	void SetUpAI(); // AI �ʱ� ������ �����ϴ� �Լ�
	void PlayNormalAttackAnimation(); // �Ϲ� ���� �ִϸ��̼��� ����ϴ� �Լ�
	virtual float TakeDamage( // �������� �Ծ����� ȣ��Ǵ� �Լ� (AActor�� TakeDamage �������̵�)
		float DamageAmount, // ���� ������ ��
		struct FDamageEvent const& DamageEvent, // ������ �̺�Ʈ ���� 
		class AController* EventInstigator, // �������� ���� ��Ʈ�ѷ�
		AActor* DamageCauser // �������� ������ ����
	) override;

	void Die(); // ��� ó�� �Լ�
	void Explode(); // ���� ȿ�� �� ������ ó�� �Լ�
	void ApplyBaseWalkSpeed(); // �⺻ �̵� �ӵ��� �����ϴ� �Լ�
	void HideEnemy(); // �� ĳ���͸� ����� �޸𸮿��� �����ϴ� �Լ�
	void EnterInAirStunState(float Duration); // ���� ����(���) ���·� ��ȯ�ϴ� �Լ�
	void ExitInAirStunState(); // ���� ���� ���¸� �����ϴ� �Լ�
	void ApplyGravityPull(FVector ExplosionCenter, float PullStrength); // Ư�� ��ġ�� �������� ���� ȿ���� �����ϴ� �Լ�
	void RaycastAttack(); // ����ĳ��Ʈ�� �̿��� ���� ������ �ϴ� �Լ�
	void StartAttack(); // ���� ���� �Լ�
	void EndAttack(); // ���� ���� �Լ�
	void PlayStunMontageLoop(); // ���� ��Ÿ�ָ� �ݺ� ����ϴ� �Լ�

	// ���� ������
	TSet<AActor*> RaycastHitActors; // ����ĳ��Ʈ ���ݿ� ���� ���͵��� �����ϴ� Set
	TSet<AActor*> DamagedActors; // �̹� �������� ���� ���͵��� �����ϴ� Set

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Health")
	float Health = 50.0f; // ���� ü��

	bool bIsDead = false; // ��� ���� ����
	bool bIsInAirStun = false; // ���� ���� ���� ����
	bool bCanAttack = false; // ���� ���� ����
	bool bIsAttacking = false; // ���� ���� ������ ����
	bool bHasExecutedRaycast = false; // �̹� ���ݿ��� ����ĳ��Ʈ ������ �����ߴ��� ����
	float Damage = 10.0f; // �Ϲ� ���� ������
	float ExplosionDamage = 40.0f; // ���� ������

protected:
	virtual void BeginPlay() override; // ���� ���� �� ȣ��Ǵ� �Լ�

private:
	// �ִϸ��̼� ��Ÿ��
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* SpawnIntroMontage; // ���� �ִϸ��̼� ��Ÿ��

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> NormalAttackMontages; // �Ϲ� ���� ��Ÿ�� �迭 (���� �����)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> HitMontages; // �ǰ� ��Ÿ�� �迭 (���� �����)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> DeadMontages; // ��� ��Ÿ�� �迭 (���� �����)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* InAirStunMontage; // ���� ���� ���� ��Ÿ��

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* InAirStunDeathMontage; // ���� ���� ���¿��� ��� �� ����� ��Ÿ��

	// ����Ʈ �� ����
	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* ExplosionEffect; // ���� �� ����� ���̾ư��� ����Ʈ

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* ExplosionSound; // ���� �� ����� ����

	// ��Ÿ�� ���� �� ȣ��� �Լ��� (��������Ʈ ���ε���)
	UFUNCTION()
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ���� ��Ÿ�� ���� ��

	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted); // �ǰ� ��Ÿ�� ���� �� (��� ���� ������)
	void OnDeadMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ��� ��Ÿ�� ���� ��
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ���� ��Ÿ�� ���� ��

	// Ÿ�̸� �ڵ�
	FTimerHandle DeathTimerHandle; // ��� �� ���߱����� ������ ���� Ÿ�̸�
	FTimerHandle StunTimerHandle; // ���� ���� ���ӽð��� �����ϴ� Ÿ�̸�
	FTimerHandle StunAnimRepeatTimerHandle; // ���� ���� �ִϸ��̼� �ݺ��� ���� Ÿ�̸�

	// ���� �Ӽ�
	UPROPERTY(EditAnywhere, Category = "Missile")
	float ExplosionRadius = 100.f; // ���� �ݰ�

};