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
	MoveAroundPlayer,
	MoveToPlayer,
	NormalAttack,
	//JumpAttack,
	//SpecialAttack,
	//Dash,
	//ProjectileAttack,
	//TeleportToPlayersBack
};

UCLASS()
class LOCOMOTION_API ABossEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABossEnemyAIController();

	void StopBossAI(); // AI ���� ���� �Լ�

protected:
	virtual void BeginPlay() override; // Beginplay �������̵�� �ʱ�ȭ
	virtual void Tick(float DeltaTime) override; // Tick �������̵�� �����Ӻ� ����

private:
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossMoveRadius = 600.0f; // �� ������

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossAttackRange = 250.0f; // ���� ����

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossMoveSpeed = 400.0f; // �̵� �ӵ�

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossDetectRadius = 2000.0f; // ������ �÷��̾ �ν��ϴ� �Ÿ�

	EBossEnemyAIState CurrentState = EBossEnemyAIState::Idle;
	float CurrentAngle = 0.0f; // �� �� ��ġ ����

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BossAttackCooldown = 1.0f; // ���� ��Ÿ�� 1��

	APawn* PlayerPawn = nullptr; // �÷��̾� ����
	bool bCanBossAttack = true; // ���� ���ɿ���
	bool bIsBossAttacking = false; // ������ ����
	FTimerHandle BossNormalAttackTimerHandle; // �Ϲݰ��� ��Ÿ�� Ÿ�̸�

	void BossMoveAroundPlayer(float DeltaTime); // �÷��̾� ������ ���� �Լ�
	void BossMoveToPlayer(); // �÷��̾�� �̵��ϴ� �Լ�
	void BossNormalAttack(); // �Ϲݰ��� �Լ�
    bool BossChasePlayer(); // MoveActor ���¿��� ���� �����ϸ� ���� ���·� ��ȯ�ϴ� �Լ�
	void UpdateBossAIState(float DistanceToPlayer); // ���� ������Ʈ �Լ�
	void SetBossAIState(EBossEnemyAIState NewState); // ���� ��ȯ �Լ�
	void DrawDebugInfo(); // ���º� ����� �ð�ȭ �Լ�

};
