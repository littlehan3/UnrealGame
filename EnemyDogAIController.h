#pragma once

#include "CoreMinimal.h"
#include "AIController.h" // AIController Ŭ������ ��ӹޱ� ���� ����
#include "NavigationSystem.h" // �׺���̼� �ý��� ����� ���� ����
#include "DrawDebugHelpers.h" // ����� �ð�ȭ ��� ����� ���� ����
#include "EnemyDog.h" // ������ ����� EnemyDog Ŭ������ �˾ƾ� �ϹǷ� ����
#include "EnemyDogAIController.generated.h"

class AEnemyDog; // EnemyDog Ŭ���� ���� ����
class AEnemyDogAnimInstance; // EnemyDog �ִ� �ν��Ͻ� Ŭ���� ���� ����

// AI�� �ൿ ���¸� �����ϴ� ������
UENUM(BlueprintType)
enum class EEnemyDogAIState : uint8
{
	Idle,        // ��� ����
	ChasePlayer  // �÷��̾� ���� ����
};

UCLASS()
class LOCOMOTION_API AEnemyDogAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyDogAIController(); // ������
	void StopAI(); // AI�� ��� ������ ������Ű�� �Լ�

protected:
	virtual void BeginPlay() override; // ���� ���� �� ȣ��Ǵ� �Լ�
	virtual void Tick(float DeltaTime) override; // �� ������ ȣ��Ǵ� �Լ�

private:
	APawn* PlayerPawn; // �÷��̾� ���� ���� ����

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackRange = 150.0f; // ������ �����ϴ� ����

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackCooldown = 1.0f; // ���� �� ���� ���ݱ����� ��� �ð�

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float ChaseStartDistance = 2000.0f; // �÷��̾� ������ �����ϴ� �Ÿ�

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float SurroundRadius = 100.0f; // �÷��̾ ������ �� �����ϴ� �Ÿ�

	EEnemyDogAIState CurrentState = EEnemyDogAIState::Idle; // ���� AI�� ����

	bool bCanAttack = true; // ���� ������ �������� ����
	bool bIsAttacking = false; // ���� ���� �ִϸ��̼��� ���� ������ ����
	//int32 NormalAttackCount = 0; // �Ϲ� ���� Ƚ�� ī��Ʈ

	void NormalAttack(); // �Ϲ� ������ �����ϴ� �Լ�
	void ResetAttack(); // ���� ��Ÿ���� ������ �ٽ� ���� ���� ���·� ����� �Լ�

	FTimerHandle NormalAttackTimerHandle; // ���� ��Ÿ���� �����ϴ� Ÿ�̸�

	void UpdateAIState(float DistanceToPlayer); // �÷��̾���� �Ÿ��� ���� AI ���¸� �����ϴ� �Լ�
	void SetAIState(EEnemyDogAIState NewState); // ���ο� AI ���·� �����ϴ� �Լ�
	void ChasePlayer(); // �÷��̾ �����ϴ� ������ ó���ϴ� �Լ�

	FVector CachedTargetLocation; // ��ǥ ��ġ ĳ��
	bool bHasCachedTarget = false; // ��ǥ ��ġ ĳ�� ����

	float RotationUpdateTimer = 0.0f; // ȸ�� ������Ʈ Ÿ�̸�
	int32 StaticAngleOffset = 0; // ���� ���� ������
};