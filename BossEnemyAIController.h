#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "BossEnemyAIController.generated.h"

class ABossEnemy; // ���� ������ ������� ���漱��
class ABossEnemyAnimInstance; // �ִ� �ν��Ͻ� ������� ���漱�� 

UENUM(BlueprintType)
enum class EBossEnemyAIState : uint8 // ���� ���� ������ ����
{
	Idle,
	//MoveAroundPlayer,
	//ChasePlayer,
	MoveToPlayer,
	NormalAttack,
	//BackDash,
	//ProjectileAttack,
	//SpecialAttack,

	//JumpAttack,
	//Dash
};

UCLASS()
class LOCOMOTION_API ABossEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABossEnemyAIController();
	void StopBossAI(); // AI ���� ���� �Լ�
	void OnBossNormalAttackMontageEnded();
	void SetBossAIState(EBossEnemyAIState NewState); // ���� ��ȯ �Լ�
	void OnBossAttackTeleportEnded(); // ���ݿ� �ڷ���Ʈ ���� �˸�
	void OnBossRangedAttackEnded(); // ���Ÿ� ���� ���� �˸�

protected:
	virtual void BeginPlay() override; // Beginplay �������̵�� �ʱ�ȭ
	virtual void Tick(float DeltaTime) override; // Tick �������̵�� �����Ӻ� ����

private:
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossMoveRadius = 600.0f; // �� ������

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossStandingAttackRange = 200.0f; // ���� ���� ����

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossMovingAttackRange = 250.0f; // ��ü�и� ���� ����

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossDetectRadius = 2000.0f; // ������ �÷��̾ �ν��ϴ� �Ÿ�

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossAttackCooldown = 1.0f; // ���� ��Ÿ�� 1��

	EBossEnemyAIState CurrentState = EBossEnemyAIState::Idle;


	APawn* PlayerPawn = nullptr; // �÷��̾� ����
	bool bCanBossAttack = true; // ���� ���ɿ���
	bool bIsBossAttacking = false; // ������ ����
	FTimerHandle BossNormalAttackTimerHandle; // �Ϲݰ��� ��Ÿ�� Ÿ�̸�

	void BossMoveToPlayer(); // �÷��̾�� �̵��ϴ� �Լ�
	void BossNormalAttack(); // �Ϲݰ��� �Լ�

	void UpdateBossAIState(float DistanceToPlayer); // ���� ������Ʈ �Լ�
	void DrawDebugInfo(); // ����� �ð�ȭ �Լ�

};
