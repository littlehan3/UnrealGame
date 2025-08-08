#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "EnemyDog.h"
#include "EnemyDogAIController.generated.h"

class AEnemyDog;
class AEnemyDogAnimInstance;

UENUM(BlueprintType)
enum class EEnemyDogAIState : uint8
{
	Idle,
	ChasePlayer
};

UCLASS()
class LOCOMOTION_API AEnemyDogAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyDogAIController();
	void StopAI();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	APawn* PlayerPawn;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackRange = 200.0f; // ���� ����

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackCooldown = 2.0f; // ���� ��Ÿ��

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float ChaseStartDistance = 1000.0f; // ���� ���� �Ÿ�

	EEnemyDogAIState CurrentState = EEnemyDogAIState::Idle;

	bool bCanAttack = true; // �Ϲݰ��� ���� ����
	bool bIsAttacking = false; // ���� ���� ����
	int32 NormalAttackCount = 0; // �Ϲ� ���� Ƚ�� ī��Ʈ

	void NormalAttack(); // �Ϲݰ��� �Լ�
	void ResetAttack(); // ���� ��ٿ� �ʱ�ȭ

	FTimerHandle NormalAttackTimerHandle; // �Ϲݰ��� ��Ÿ�� Ÿ�̸�

	void UpdateAIState(float DistanceToPlayer);
	void SetAIState(EEnemyDogAIState NewState);
	void ChasePlayer();

	FVector CachedTargetLocation;
	bool bHasCachedTarget = false;

	float RotationUpdateTimer = 0.0f;
	int32 StaticAngleOffset = 0;
};
