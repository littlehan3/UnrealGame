#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" // ACharacter Ŭ���� ���
#include "EnemyShooterGun.h" // ������ �� Ŭ������ �˾ƾ� �ϹǷ� ����
#include "EnemyShooterAIController.h" // ����� AI ��Ʈ�ѷ� Ŭ������ �˾ƾ� �ϹǷ� ����
#include "EnemyShooterAnimInstance.h" // ����� �ִ� �ν��Ͻ� Ŭ������ �˾ƾ� �ϹǷ� ����
#include "Animation/AnimInstance.h" // UAnimInstance Ŭ���� ���
#include "EnemyShooter.generated.h"

class AEnemyShooterGrenade; // ����ź Ŭ���� ���� ����

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyShooter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyShooter(); // ������

	virtual void PostInitializeComponents() override; // ������Ʈ �ʱ�ȭ ���� ȣ��Ǵ� �Լ�

	void PlaySpawnIntroAnimation(); // ���� �� ���� �ִϸ��̼��� ����ϴ� �Լ�
	void SetupAI(); // AI �ʱ� ������ �����ϴ� �Լ�
	void PlayShootingAnimation(); // ��� �ִϸ��̼��� ����ϴ� �Լ�
	void PlayThrowingGrenadeAnimation(); // ����ź ��ô �ִϸ��̼��� ����ϴ� �Լ�

	// �������� �Ծ����� ȣ��Ǵ� �Լ�
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	void Die(); // ��� ó�� �Լ�
	void ApplyBaseWalkSpeed(); // �⺻ �̵� �ӵ��� �����ϴ� �Լ�
	void HideEnemy(); // ��� �� ���� ���� �� ���� ó�� �Լ�
	void EnerInAirStunState(float Duration); // ���� ���� ���� ���� �Լ� (�̱���)
	void ExitInAirStunState(); // ���� ���� ���� ���� �Լ� (�̱���)
	void ApplyGravityPull(FVector ExlplosionCenter, float PullStrengh); // �ܺ� ���� ���� �������� ȿ�� �Լ� (�̱���)
	void Shoot(); // ������ ���� �߻��ϴ� �Լ�

	// ���� ������ (���� ������ ����)
	TSet<AActor*> RaycastHitActors;
	TSet<AActor*> DamagedActors;

	// ĳ���� ���� ����
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Health")
	float Health = 200.0f; // ���� ü��

	bool bIsPlayingIntro = false; // ���� �ִϸ��̼� ��� ����
	bool bIsDead = false; // ��� ���� ����
	bool bIsInAirStun = false; // ���� ���� ���� ����
	bool bCanAttack = false; // ���� ���� ���� (��Ÿ�� ��)
	bool bIsAttacking = false; // ���� ���� �ִϸ��̼� ��� ����
	bool bIsShooting = false; // ���� ��� ������ ����

	// ������ ���� �� ����ź Ŭ���� (�������Ʈ���� ����)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyShooterGun> GunClass; // ������ ���� ���� Ŭ����

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	AEnemyShooterGun* EquippedGun; // ���� ������ �ѿ� ���� ����

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyShooterGrenade> GrenadeClass; // ��ô�� ����ź�� ���� Ŭ����

protected:
	virtual void BeginPlay() override; // ���� ���� �� ȣ��

private:
	// �ִϸ��̼� ��Ÿ��
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* SpawnIntroMontage; // ���� �ִϸ��̼�

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ShootingMontage; // ��� �ִϸ��̼�

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> HitMontages; // �ǰ� �ִϸ��̼� �迭 (���� �����)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> DeadMontages; // ��� �ִϸ��̼� �迭 (���� �����)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* InAirStunMontage; // ���� ���� ���� �ִϸ��̼�

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* InAirStunDeathMontage; // ���� ���� �� ��� �ִϸ��̼�

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ThrowingGrenadeMontage; // ����ź ��ô �ִϸ��̼�

	// ��Ÿ�� ���� �� ȣ��� �Լ��� (��������Ʈ ���ε���)
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnShootingMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnThrowingGrenadeMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// Ÿ�̸� �ڵ�
	FTimerHandle StunTimerHandle; // ���� ���ӽð� ����� Ÿ�̸�
	FTimerHandle DeathTimerHandle; // ��� �� ���������� ������ Ÿ�̸�

	void StopActions(); // ��� �ൿ�� ��� ������Ű�� �Լ�
};