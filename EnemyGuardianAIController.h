#pragma once

#include "CoreMinimal.h"
#include "AIController.h" // AIController Ŭ���� ���
#include "Engine/World.h" // UWorld Ŭ���� ���
#include "EngineUtils.h" // TActorIterator ���
#include "EnemyGuardianAIController.generated.h"

// ���� ����
class AEnemyShooter;
class AEnemyGuardian;

UCLASS()
class LOCOMOTION_API AEnemyGuardianAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyGuardianAIController(); // ������
    void StopAI(); // AI�� ��� ������ ������Ű�� �Լ� (���� �̻��)

protected:
    virtual void BeginPlay() override; // ���� ���� �� ȣ��
    virtual void Tick(float DeltaTime) override; // �� ������ ȣ��

    // AI �ൿ ���� ������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior")
    float ProtectionDistance = 150.0f; // ��ȣ�� ���ͷκ��� ������ ���� �Ÿ�

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior")
    float MinDistanceToTarget = 50.0f; // ��ǥ ���� ���޷� ������ �ּ� �Ÿ�

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior")
    float SurroundRadius = 150.0f; // �÷��̾ ������ �� ������ �Ÿ�

    // ���� ����ȭ ����
    UPROPERTY(EditAnywhere, Category = "AI Performance")
    float AllyCacheUpdateInterval = 2.0f; // �ֺ� �Ʊ� ����� �����ϴ� �ֱ�

private:
    APawn* PlayerPawn; // �÷��̾� ���� ���� ����

    // �ٽ� �ൿ ���� �Լ�
    void PerformShooterProtection(); // �Ʊ� ���͸� ��ȣ�ϴ� ����
    void PerformSurroundMovement(); // �÷��̾ �����ϴ� ����

    // ����ȭ�� ���� ĳ�� �ý���
    void UpdateAllyCaches(); // �ֱ������� ȣ��Ǿ� �Ʊ� ����� �����ϴ� �Լ�

    FTimerHandle AllyCacheUpdateTimerHandle; // �Ʊ� ĳ�� ������ ���� Ÿ�̸�

    // ĳ�õ� �Ʊ� ��� (TWeakObjectPtr�� ����Ͽ� �Ʊ��� �׾ �޸� ���� ����)
    TArray<TWeakObjectPtr<AEnemyShooter>> CachedShooters;
    TArray<TWeakObjectPtr<AEnemyGuardian>> CachedGuardians;
};