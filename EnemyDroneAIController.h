#pragma once

#include "CoreMinimal.h"
#include "AIController.h" // AIController Ŭ���� ���
#include "EnemyDroneAIController.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyDroneAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyDroneAIController(); // ������

protected:
    virtual void BeginPlay() override; // ���� ���� �� ȣ��
    virtual void Tick(float DeltaTime) override; // �� ������ ȣ��

    // �÷��̾� �ֺ��� �����ϴ� �˵��� �ݰ�
    UPROPERTY(EditAnywhere, Category = "AI")
    float OrbitRadius = 500.f;

    // �˵��� ���� ȸ���ϴ� �ӵ� (�ʴ� ����)
    UPROPERTY(EditAnywhere, Category = "AI")
    float OrbitSpeed = 45.f;

    // �÷��̾�κ��� �����Ϸ��� �⺻ ��
    UPROPERTY(EditAnywhere, Category = "AI")
    float HeightOffset = 300.f;

private:
    AActor* PlayerActor; // �÷��̾� ���� ����
    float CurrentAngle = 0.f; // ���� �˵� ���� ����
    bool bClockwise = false; // ���� ȸ�� ���� (true: �ð�, false: �ݽð�)

    // ��ֹ��� �������� �Ǵ��ϱ� ���� ������
    float TimeStuck = 0.f; // ���ڸ��� �ӹ��� �ð�
    float MaxStuckTime = 1.5f; // �ִ� ��� �ð�
    bool bTriedReverse = false; // ������ �� �� �����ߴ��� ����

    // �⺻ ���� �����صδ� ���� (�� ȸ�� �� ���Ϳ�)
    float BaseHeight = 300.f;

    // �˵� ��Ż ������ ������
    float TimeOutOfRadius = 0.f; // �˵� �ݰ��� ��� �ð�
    float OutOfRadiusLimit = 1.5f; // �˵� ��Ż ��� �ð�
    float RadiusTolerance = 100.f; // �˵� �ݰ��� ���� ��� ����

    // ��� �� ��� ������ ������
    float TargetHeight = 0.f; // ��� ȸ�� �� ��ǥ ��
    bool bRising = false; // ���� ��� ��� ������ ����
};