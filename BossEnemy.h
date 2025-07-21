#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossEnemyAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "BossEnemy.generated.h"

class ABossProjectile;

UCLASS()
class LOCOMOTION_API ABossEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	ABossEnemy();

	virtual void PostInitializeComponents() override; // AI �̵�

	void PlayBossNormalAttackAnimation(); // �Ϲݰ��� �ִϸ��̼� ���� �Լ�
	void PlayBossUpperBodyAttackAnimation(); // ��ü ���� �ִϸ��̼� ���� �Լ�
	void PlayBossTeleportAnimation(); // �ڷ���Ʈ �Լ�
	bool bIsBossTeleporting = false; // �ڷ���Ʈ �� ����
	bool bCanTeleport = true; // �ڷ���Ʈ ���� ����
	void PlayBossSpawnIntroAnimation(); // ���� ���� �ִϸ��̼� ��� �Լ�
	bool bIsPlayingBossIntro = false; // ���� ���� �ִϸ��̼� ��� �� ����
	void PlayBossAttackTeleportAnimation(); // ���ݿ� �ڷ���Ʈ �ִϸ��̼� ��� �Լ�
	void PlayBossRangedAttackAnimation(); // ���Ÿ� ���� �ִϸ��̼� ��� �Լ�
	bool bIsBossAttackTeleporting = false; // ���ݿ� �ڷ���Ʈ �� ����
	bool bIsBossRangedAttacking = false; // ���Ÿ� ���� �� ����
	bool bShouldUseRangedAfterTeleport = false; // �ڷ���Ʈ �� ���Ÿ� ���� ��� ���� 
	bool bIsInvincible = false; // �������� ����

	
	virtual float TakeDamage( // �������� �Ծ����� ȣ��Ǵ� �Լ� (AActor�� TakeDamage �������̵�)
		float DamageAmount, // ���� ������ ��
		struct FDamageEvent const& DamageEvent, // ������ �̺�Ʈ ����
		class AController* EventInstigator, // �������� ���� ��Ʈ�ѷ�
		AActor* DamageCauser // �������� ������ ����
	) override;

	bool bIsBossDead = false; // ��� ����
	bool bIsBossStrongAttacking = false; // �������� ����
	bool bCanBossAttack = false; // ���� ���� ����
	bool bIsBossHit = false; // �ǰ��� ����
	bool bIsFullBodyAttacking = false;// ���Ű����� ����

	float BossHealth = 200.0f; // ü��

	UFUNCTION() // �ִ� ��Ƽ���̿��� ȣ���� �Լ�
	void SpawnBossProjectile();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossHitSound; // �ǰ� ����

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossDieSound; // ��� ����

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossNormalAttackSound; // �Ϲݰ��� ����

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossStrongAttackSound; // ������ ����

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UBossEnemyAnimInstance* BossEnemyAnimInstance; // �ִϸ��̼� �ν��Ͻ� �߰�

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossNormalAttackMontages; // �Ϲ� ���� ��Ÿ�� �迭

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossUpperBodyMontages; // ��ü ���� ��Ÿ�� �迭

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossHitReactionMontages; // �ǰ� ��Ÿ�� �迭

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossDeadMontages; // ��� ��Ÿ�� �迭

	void BossDie(); // ��� �Լ�
	void SetUpBossAI(); // AI�� �׺�Ž����� �̵��Ҽ��ְ� �����ϴ� �Լ�
	void StopBossActions(); // ��� ���� ���� �Լ�
	void HideBossEnemy(); // ����� ������ ����� �Լ�
	FTimerHandle BossDeathHideTimerHandle; // ��� ��Ÿ�� Ÿ�̸�
	void DisableBossMovement();
	void EnableBossMovement();

	UFUNCTION()
	void OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // �Ϲݰ��� ��Ÿ�� ��������Ʈ

	UFUNCTION()
	void OnUpperBodyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ��ü���� ��Ÿ�� ��������Ʈ

	UFUNCTION()
	void OnHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ��Ʈ ��Ÿ�� ��������Ʈ

	UFUNCTION()
	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ��� ��Ÿ�� ��������Ʈ

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float TeleportDistance = 500.0f; // �ڷ���Ʈ �Ÿ�

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float TeleportCooldown = 3.0f; // �ڷ���Ʈ ��Ÿ��

	FTimerHandle TeleportExecutionTimer; // �ڷ���Ʈ �̵� Ÿ�̸�
	FTimerHandle AttackTeleportExecutionTimer; // ���� �ڷ���Ʈ �̵� Ÿ�̸�
	FTimerHandle TeleportCooldownTimer; // �ڷ���Ʈ ��Ÿ�� Ÿ�̸�

	void OnTeleportCooldownEnd(); // �ڷ���Ʈ ��Ÿ�� ���� �Լ�

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossTeleportMontages; // �ڷ���Ʈ ��Ÿ�� �迭

	UFUNCTION()
	void OnTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted); // �ڷ���Ʈ ��Ÿ�� ��������Ʈ

	void ExecuteTeleport(); // ���� �ڷ���Ʈ ���� �Լ� (��Ÿ�� �߰��� ȣ��)
	FVector CalculateTeleportLocation(); // �ڷ���Ʈ ��ġ ��� �Լ�
	FVector AdjustHeightForCharacter(const FVector& TargetLocation); // ĳ���� ���� ���� �Լ�

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossSpawnIntroMontages; // ���� ���� ��Ÿ�� �迭

	UFUNCTION()
	void OnBossIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ���� ���� ��Ÿ�� ���� ��������Ʈ

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossAttackTeleportMontages; // ���ݿ� �ڷ���Ʈ ��Ÿ�� �迭

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossRangedAttackMontages; // ���Ÿ� ���� ��Ÿ�� �迭

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackTeleportRange = 150.0f; // ���ݿ� �ڷ���Ʈ �Ÿ� (�÷��̾� ��ó��)

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float PostTeleportPauseTime = 0.5f; // �ڷ���Ʈ �� ���� �ð�

	UFUNCTION()
	void OnAttackTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ���ݿ� �ڷ���Ʈ ��Ÿ�� ��������Ʈ

	UFUNCTION()
	void OnRangedAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ���Ÿ� ���� ��Ÿ�� ��������Ʈ

	void ExecuteAttackTeleport(); // ���� ���ݿ� �ڷ���Ʈ ���� �Լ�
	FVector CalculateAttackTeleportLocation(); // ���ݿ� �ڷ���Ʈ ��ġ ��� �Լ�
	void OnPostTeleportPauseEnd(); // �ڷ���Ʈ �� ���� ���� �Լ�

	FTimerHandle PostTeleportPauseTimer; // �ڷ���Ʈ �� ���� Ÿ�̸�

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class ABossProjectile> BossProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	FVector MuzzleOffset = FVector(100.f, 0.f, 80.f); // ���� �߻� ��ġ ������

};
