#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossEnemyAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "BossEnemy.generated.h"

UCLASS()
class LOCOMOTION_API ABossEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	ABossEnemy();

	virtual void PostInitializeComponents() override; // AI �̵�

	void PlayBossNormalAttackAnimation(); // �Ϲݰ��� �ִϸ��̼� ���� �Լ�
	void PlayBossUpperBodyAttack(); // ��ü ���� �ִϸ��̼� ���� �Լ�

	bool bIsBossDead = false; // ��� ����
	
	virtual float TakeDamage( // �������� �Ծ����� ȣ��Ǵ� �Լ� (AActor�� TakeDamage �������̵�)
		float DamageAmount, // ���� ������ ��
		struct FDamageEvent const& DamageEvent, // ������ �̺�Ʈ ����
		class AController* EventInstigator, // �������� ���� ��Ʈ�ѷ�
		AActor* DamageCauser // �������� ������ ����
	) override;

	bool bIsBossStrongAttacking = false; // �������� ����
	bool bCanBossAttack = false; // ���� ���� ����

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

	float BossHealth = 2000.0f; // ü��

	void BossDie(); // ��� �Լ�
	void SetUpBossAI(); // AI�� �׺�Ž����� �̵��Ҽ��ְ� �����ϴ� �Լ�
	void StopBossActions(); // ��� ���� ���� �Լ�
	void HideBossEnemy(); // ����� ������ ����� �Լ�
	FTimerHandle BossDeathHideTimerHandle; // ��� ��Ÿ�� Ÿ�̸�

	UFUNCTION()
	void OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnUpperBodyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
