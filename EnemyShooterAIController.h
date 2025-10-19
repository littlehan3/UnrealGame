#pragma once

#include "CoreMinimal.h"
#include "AIController.h" // AIController Ŭ���� ���
#include "EnemyShooterAIController.generated.h"

// ���� ����
class AEnemyShooter;
class AEnemyGuardian;
class AEnemyGrenade;

// AI�� �ൿ ���¸� �����ϴ� ������
UENUM(BlueprintType)
enum class EEnemyShooterAIState : uint8
{
    Idle,       // ��� (�÷��̾� �̹߰�)
    Detecting,  // Ž�� (�÷��̾� �߰�, �ൿ ���� ��)
    Moving,     // �̵� (��� ��ġ �Ǵ� ���� ��ġ�� �̵�)
    Shooting,   // ��� (��� ���� �Ÿ����� ����)
    Retreating  // ���� (�÷��̾ �ʹ� ����� ��)
};

UCLASS()
class LOCOMOTION_API AEnemyShooterAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyShooterAIController(); // ������

protected:
    virtual void BeginPlay() override; // ���� ���� �� ȣ��
    virtual void Tick(float DeltaTime) override; // �� ������ ȣ��

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI State")
    EEnemyShooterAIState CurrentState = EEnemyShooterAIState::Idle; // ���� AI ����

    // AI �⺻ ���� (�Ÿ�)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Range")
    float DetectionRadius = 3000.0f; // �÷��̾� Ž�� �ִ� �ݰ�

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Range")
    float ShootRadius = 700.0f; // ����� �����ϴ� �ִ� �Ÿ�

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Range")
    float MinShootDistance = 350.0f; // �� �Ÿ����� ��������� ���� ����

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Range")
    float RetreatBuffer = 100.0f; // ���� �� �ٽ� ��� ���·� ��ȯ�ϱ� ���� �Ÿ� ����

    // �����̼�(����) ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Formation")
    float FormationRadius = 600.0f; // �÷��̾ �߽����� ������ ������ �ݰ�

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Formation")
    float MinAllyDistance = 180.0f; // �ٸ� �Ʊ��� �����Ϸ��� �ּ� �Ÿ� (��ħ ����)

    // ȸ�� ���� ���� (���� �þ߰� ����)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Rotation", meta = (ClampMin = "-89.0", ClampMax = "0.0"))
    float MaxLookDownAngle = -45.0f; // �Ʒ������� �� �� �ִ� �ִ� ����

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Rotation", meta = (ClampMin = "0.0", ClampMax = "89.0"))
    float MaxLookUpAngle = 15.0f; // �������� �� �� �ִ� �ִ� ����

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Rotation", meta = (ClampMin = "1.0", ClampMax = "20.0"))
    float RotationInterpSpeed = 8.0f; // �÷��̾ ���� ȸ���ϴ� �ӵ�

    // ���� ����ȭ ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Performance")
    float AILogicUpdateInterval = 0.1f; // AI ���� ������Ʈ �ֱ� (������ ����)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Performance")
    float AlliesSearchInterval = 1.0f; // �ֺ� �Ʊ��� �˻��ϴ� �ֱ�

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Performance")
    float ClearShotCheckInterval = 0.3f; // �缱 Ȯ���� Ȯ���ϴ� �ֱ�

private:
    APawn* PlayerPawn = nullptr; // �÷��̾� ���� ���� ����
    float LastShootTime = 0.0f; // ������ ��� �ð� (��Ÿ�� ����)
    float ShootCooldown = 2.0f; // ��� ��Ÿ��

    // �����̼� ���� ����
    FVector AssignedPosition; // �ڽ��� �̵��ؾ� �� ���� �� ��ǥ ��ġ
    int32 FormationIndex = -1; // ���� �� �ڽ��� ���� (������ ����)
    bool bHasAssignedPosition = false; // ��ǥ ��ġ�� �Ҵ�޾Ҵ��� ����
    float LastPositionUpdate = 0.0f; // ���������� ���� ��ġ�� ������ �ð�
    float PositionUpdateInterval = 1.0f; // ���� ��ġ ���� �ֱ�

    // ���� ����ȭ�� ���� Ÿ�̸� ����
    float LastAILogicUpdate = 0.0f; // ������ AI ���� ������Ʈ �ð� (������ ����)
    float LastAlliesSearch = 0.0f; // ������ �Ʊ� �˻� �ð�
    float LastClearShotCheck = 0.0f; // ������ �缱 Ȯ�� �ð�

    // ĳ�õ� ������ (�Ź� �˻��ϴ� ����� ���̱� ����)
    TArray<TWeakObjectPtr<AEnemyShooter>> CachedAllies; // ĳ�õ� �ֺ� �Ʊ� ���
    bool bCachedHasClearShot = true; // ĳ�õ� �缱 Ȯ�� ����

    // ���� ����ȭ�� �Լ�
    void UpdateCachedAllies(); // �ֺ� �Ʊ� ����� �˻��Ͽ� ĳ�ø� ����
    void UpdateCachedClearShot(); // �÷��̾������ �缱�� Ȯ���Ǿ����� Ȯ���Ͽ� ĳ�ø� ����

    // ���� �ӽ� ���� �Լ�
    void UpdateAIState(); // ���� ���¸� Ȯ���ϰ� �ٸ� ���·� ��ȯ���� ����
    void HandleIdleState(); // ��� ������ ���� ���� ó��
    void HandleDetectingState(); // Ž�� ������ ���� ���� ó��
    void HandleMovingState(); // �̵� ������ ���� ���� ó��
    void HandleShootingState(); // ��� ������ ���� ���� ó��
    void HandleRetreatingState(); // ���� ������ ���� ���� ó��

    // ���� Ȯ�ο� ��ƿ��Ƽ �Լ�
    float GetDistanceToPlayer() const; // �÷��̾������ �Ÿ� ��ȯ
    bool IsPlayerInDetectionRange() const; // �÷��̾ Ž�� ���� �ȿ� �ִ��� Ȯ��
    bool IsPlayerInShootRange() const; // �÷��̾ ��� ���� �ȿ� �ִ��� Ȯ��
    bool IsPlayerTooClose() const; // �÷��̾ �ʹ� ������ �ִ��� Ȯ��
    bool HasClearShotToPlayer() const; // �缱�� Ȯ���Ǿ����� (ĳ�õ� ��) Ȯ��

    // ������ ȸ�� �ý���
    void LookAtPlayerWithConstraints(float DeltaTime); // ���ѵ� ���� ������ �÷��̾ �ε巴�� �ٶ�
    FRotator CalculateConstrainedLookRotation(FVector TargetLocation) const; // ���� ������ ������ ���� ȸ���� ���

    // �����̼�(����) �ý���
    void UpdateFormationPosition(); // �ڽ��� ���� ��ġ�� ����
    FVector CalculateFormationPosition(); // �÷��̾�� �Ʊ� ��ġ�� ������� �ڽ��� ��ǥ ��ġ ���
    bool IsPositionOccupied(FVector Position, float CheckRadius = 150.0f); // Ư�� ��ġ�� �ٸ� �Ʊ��� ���� �����Ǿ����� Ȯ��
    bool ShouldMoveToFormation() const; // ���� ���� ��ġ�� �̵��ؾ� �ϴ��� �Ǵ�
    TArray<AActor*> GetAllAllies() const; // ��ȿ�� ��� �Ʊ� ���� ��� ��ȯ (ĳ�� ���)

    // �⺻ �׼� �Լ�
    void MoveTowardsTarget(FVector Target); // Ư�� ��ǥ �������� �̵�
    void MoveTowardsPlayer(); // �÷��̾ ���� �̵�
    //void RetreatFromPlayer(); // �÷��̾�κ��� ���� (HandleRetreatingState���� ���� ó��)
    void StopMovementInput(); // �̵� ����
    void PerformShooting(); // ��� ����
    bool CanShoot() const; // ���� ��� ��Ÿ���� �������� Ȯ��

    // ����ź ���� ����
    bool bIsBlockedByGuardian = false; // �Ʊ� ����� ���� �þ߰� �������� ����
    float LastGrenadeThrowTime = 0.0f; // ������ ����ź ��ô �ð�
    UPROPERTY(EditAnywhere, Category = "AI Combat")
    float GrenadeCooldown = 3.0f; // ����ź ��Ÿ��

    UPROPERTY(EditAnywhere, Category = "AI Combat")
    float GrenadeLaunchSpeed = 700.0f; // ����ź �߻� �ӵ�

    UPROPERTY(EditAnywhere, Category = "AI Combat")
    float GrenadeTargetOffset = 250.0f; // ����ź ��ǥ ���� ������ (�÷��̾� �ణ ��)
};