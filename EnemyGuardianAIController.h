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

private:
    APawn* PlayerPawn; // �÷��̾� ���� ���� ����

    // 4���� �̵� ���� ���� ���� (����� ���� �������� ��ü��)
    FTimerHandle MoveTimerHandle; // �̵� ���� ��ȯ Ÿ�̸�
    float MoveDuration; // �̵� ���� �ð�
    int32 DirectionIndex; // ���� �̵� ���� �ε���

    // �ٽ� �ൿ ���� �Լ�
    void MoveInDirection(); // Ư�� �������� �̵� (���� �̻��)
    void PerformShooterProtection(); // �Ʊ� ���͸� ��ȣ�ϴ� ����
    void PerformSurroundMovement(); // �÷��̾ �����ϴ� ����
};