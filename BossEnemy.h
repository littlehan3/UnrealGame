#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossEnemyAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraSystem.h"
#include "BossEnemy.generated.h"

class ABossProjectile;
class AEnemyBossKatana;

UCLASS(Blueprintable)
class LOCOMOTION_API ABossEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	ABossEnemy();

	virtual void PostInitializeComponents() override; // AI ��Ʈ�ѷ� Ȯ�� �Լ�

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

	void PlayBossStealthAttackAnimation(); // 1�ܰ� ���ڽ� ���� ���ϸ��̼� ���� �Լ�
	void StartStealthDivePhase(); // 2�ܰ� ���ڽ� ���� �Լ�
	void StartStealthInvisiblePhase(); // 3�ܰ� ���ڽ� �Լ�
	FVector CalculateRandomTeleportLocation(); // 4�ܰ� ���ڽ� �ڷ���Ʈ ��ġ ��� �Լ�
	void ExecuteStealthKick(); // 5�ܰ� ���ڽ� ű �Լ�
	void ExecuteStealthKickRaycast(); // ���ڽ� ű ����ĳ��Ʈ �Լ�
	void LaunchPlayerIntoAir(APawn* PlayerPawn, float LaunchHeight); // ���ڽ� ű ��� �Լ�
	void ExecuteStealthFinish(); // 6�ܰ� ���ڽ� �ǴϽ� �Լ�
	void ExecuteStealthFinishRaycast(); // ���ڽ� �ǴϽ� ����ĳ��Ʈ �Լ�
	void EndStealthAttack(); // ���ڽ� ���� ���� �Լ�
	bool bCanUseStealthAttack = true; // ���ڽ� ���� ��� ���� ����
	bool bIsStealthStarting = false;  // ���ڽ� ���� ��
	bool bIsStealthDiving = false;    // �پ��� ��
	bool bIsStealthInvisible = false; // ���� ���� ����
	bool bIsStealthKicking = false;   // ű ���� ��
	bool bIsStealthFinishing = false; // �ǴϽ� ���� ��
	void UpdateStealthTeleportLocation(); // ���ڽ� �ڷ���Ʈ ��ġ ������Ʈ �Լ�

	UFUNCTION()
	void OnStealthCooldownEnd(); // ���ڽ� ��Ÿ�� ����

	
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
	bool bHasExecutedKickRaycast = false; // ű ����ĳ��Ʈ ����

	float BossHealth = 200.0f; // ü��

	UFUNCTION() // �ִ� ��Ƽ���̿��� ȣ���� �Լ�
	void SpawnBossProjectile();

	// ���� ���� īŸ�� ����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	AEnemyBossKatana* EquippedBossKatana;

	void StartAttack();
	void EndAttack();
	void BossDie(); // ��� �Լ�

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

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* StealthStartMontage; // 1�ܰ�: ���ڽ� ���� ��Ÿ��

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* StealthDiveMontage; // 2�ܰ�: �پ��� ��Ÿ��

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* StealthKickMontage; // 5�ܰ�: ���ڽ� ű ��Ÿ��

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* StealthFinishMontage; // 6�ܰ�: ���ڽ� �ǴϽ� ��Ÿ��

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
	float TeleportCooldown = 10.0f; // �ڷ���Ʈ ��Ÿ��

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

	UFUNCTION()
	void OnStealthStartMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ���ڽ� 1�ܰ� ��Ÿ�� ��������Ʈ
	UFUNCTION()
	void OnStealthDiveMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ���ڽ� 2�ܰ� ��Ÿ�� ��������Ʈ
	UFUNCTION()
	void OnStealthFinishMontageEnded(UAnimMontage* Montage, bool bInterrupted); // ���ڽ� 6�ܰ� ��Ÿ�� ��������Ʈ

	// ���ڽ� �ܰ� ����
	UPROPERTY(VisibleAnywhere, Category = "Stealth")
	int32 CurrentStealthPhase = 0;  // ���� ���ڽ� �ܰ�

	// Ÿ�̸ӵ�
	FTimerHandle StealthWaitTimer;  // ���ڽ� ��� Ÿ�̸�
	FTimerHandle PlayerAirborneTimer; // �÷��̾� ���� ü�� Ÿ�̸�

	// ���� �ڷ���Ʈ ��ġ
	FVector CalculatedTeleportLocation;

	// ���ڽ� ���� �߰� ������
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float StealthCooldown = 30.0f; // ���ڽ� ��Ÿ��

	FTimerHandle StealthCooldownTimer; // ���ڽ� ��Ÿ�� Ÿ�̸�
	FTimerHandle StealthDiveTransitionTimer;// ���ڽ� ���̺� �ۼ�Ʈ ��ȯ Ÿ�̸�

	//BP���� Katana Blueprint ����
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyBossKatana> BossKatanaClass;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* StealthFinishEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	USoundBase* StealthFinishSound;
};
