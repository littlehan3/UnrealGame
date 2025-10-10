#pragma once

#include "CoreMinimal.h" 
#include "GameFramework/Character.h" // ACharacter Ŭ���� ���
#include "EnemyGuardianAnimInstance.h" // ����� �ִ� �ν��Ͻ� Ŭ���� ����
#include "Animation/AnimInstance.h" // UAnimInstance Ŭ���� ���
#include "EnemyGuardianShield.h" // ������ ���� Ŭ���� ����
#include "EnemyGuardian.generated.h"

// ���� ����
class AEnemyGuardianBaton;
class AEnemyGuardianShield;

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyGuardian : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyGuardian(); // ������
	virtual void Tick(float DeltaTime) override; // �� ������ ȣ��Ǵ� �Լ�

	void PlaySpawnIntroAnimation(); // ���� �� ���� �ִϸ��̼��� ����ϴ� �Լ�
	void SetUpAI(); // AI �ʱ� ������ �����ϴ� �Լ�
	void PlayShieldAttackAnimation(); // ���� ���� �ִϸ��̼��� ����ϴ� �Լ�
	void PlayNormalAttackAnimation(); // �Ϲ� ����(���к�) �ִϸ��̼��� ����ϴ� �Լ�
	void PlayBlockAnimation(); // ��� �ִϸ��̼��� ����ϴ� �Լ�
	void Stun(); // ���� �ı� �� ȣ��Ǵ� ���� �Լ�

	// �������� �Ծ��� �� ȣ��Ǵ� �Լ� (AActor�� TakeDamage �������̵�)
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,

		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	void Die(); // ��� ó�� �Լ�
	void ApplyBaseWalkSpeed(); // �⺻ �̵� �ӵ��� �����ϴ� �Լ�
	void HideEnemy(); // ��� �� ���� ���� �� ���� ó�� �Լ�
	void StartAttack(); // ���� ���� ���� �Լ� (�ִϸ��̼� ��Ƽ���̿�)
	void EndAttack(); // ���� ���� ���� �Լ� (�ִϸ��̼� ��Ƽ���̿�)

	// ĳ���� ���� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float Health = 300.0f; // ���� ü��

	bool bIsPlayingIntro = false; // ���� �ִϸ��̼� ��� ����
	bool bIsShieldDestroyed = false; // ���� �ı� ����
	bool bIsDead = false; // ��� ���� ����
	bool bCanAttack = false; // ���� ���� ���� (��Ÿ�� ��)
	bool bIsAttacking = false; // ���� ���� �ִϸ��̼� ��� ���� (������ ����)
	bool bHasExecutedRaycast = false; // �̹� ���ݿ��� ����ĳ��Ʈ ������ �����ߴ��� ����
	bool bIsStunned = false; // ���� ���� �������� ����
	bool bIsStunRecovering = false; // ���Ͽ��� ȸ�� ������ ����
	bool bIsShieldAttacking = false; // ���� ���� ���� ������ ����
	bool bIsBatonAttacking = false; // ���� ���к� ���� ������ ����
	bool bProtectedShooterSeekingSight = false; // ���� �þ� Ȯ�� ���� (���� ������ ����)

	float Damage = 50.0f; // �⺻ ������ (������ ����, �� ���Ⱑ ��ü ������ ����)

	// ������ ���� ����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	AEnemyGuardianShield* EquippedShield; // ���� ������ ���� ����

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	AEnemyGuardianBaton* EquippedBaton; // ���� ������ ���к� ����

	// ���� �Ӽ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ShieldAttackRadius = 150.0f; // AI�� ���� ������ �����ϴ� �Ÿ�

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BatonAttackRadius = 200.0f; // AI�� ���к� ������ �����ϴ� �Ÿ�

	// ���� ������ (���� ������ ����)
	TSet<AActor*> RaycastHitActors;
	TSet<AActor*> DamagedActors;

protected:
	virtual void BeginPlay() override; // ���� ���� �� ȣ��

private:
	// ��Ÿ�� ���� �� ȣ��� �Լ��� (��������Ʈ ���ε���)
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnShieldAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnStunMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// �������Ʈ���� ������ ���� Ŭ����
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyGuardianShield> ShieldClass; // ������ ���� Ŭ����

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyGuardianBaton> BatonClass; // ������ ���к� Ŭ����

	// �ִϸ��̼� ��Ÿ��
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* SpawnIntroMontage; // ���� �ִϸ��̼�

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> ShieldAttackMontages; // ���� ���� �ִϸ��̼� �迭 (���� �����)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> NormalAttackMontages; // ���к� ���� �ִϸ��̼� �迭 (���� �����)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* BlockMontage; // ��� �ִϸ��̼�

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* HitMontage; // �ǰ� �ִϸ��̼�

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> DeadMontages; // ��� �ִϸ��̼� �迭 (���� �����)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* StunMontage; // ���� �ִϸ��̼�

	// Ÿ�̸� �ڵ�
	FTimerHandle StunTransitionTimer; // ���� ���� ��ȯ�� Ÿ�̸� (���� ������ ����)
	FTimerHandle DeathTimerHandle; // ��� �� ���������� ������ Ÿ�̸�
};