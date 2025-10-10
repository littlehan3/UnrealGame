#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" // ACharacter Ŭ���� ���
#include "EnemyDroneMissile.h" // ����� �߻��� �̻��� Ŭ������ �˾ƾ� �ϹǷ� ����
#include "NiagaraSystem.h" // ���̾ư��� ����Ʈ �ý��� ���
#include "EnemyDrone.generated.h"

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyDrone : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemyDrone(); // ������

    // ������Ʈ Ǯ���� ���� �̻��� �迭
    UPROPERTY()
    TArray<AEnemyDroneMissile*> MissilePool; // �̸� ������ �� �̻��ϵ��� ��Ƶδ� �迭

    // �̻��� Ǯ���� ���� ��� ������(��Ȱ��ȭ��) �̻����� �������� �Լ�
    AEnemyDroneMissile* GetAvailableMissileFromPool();

    // �������� �Ծ��� �� ȣ��Ǵ� �Լ�
    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator,
        AActor* DamageCauser
    ) override;

    void Die(); // ��� ó�� �Լ�
    void HideEnemy(); // ��� �� ���� ���� �� ���� ó�� �Լ�
    float Health = 40.0f; // ���� ü��
    bool bIsDead = false; // ��� ���� ����

protected:
    virtual void BeginPlay() override; // ���� ���� �� ȣ��Ǵ� �Լ�
    virtual void Tick(float DeltaTime) override; // �� ������ ȣ��Ǵ� �Լ�

    // �������Ʈ���� ������ �̻��� Ŭ����
    UPROPERTY(EditDefaultsOnly, Category = "Pawn")
    TSubclassOf<AEnemyDroneMissile> MissileClass; // Ǯ���� �̻����� ���� Ŭ����

    float MissileCooldown = 3.0f; // �̻��� �߻� �� ���� �߻������ ��� �ð�
    float MissileTimer = 0.0f; // �̻��� ��Ÿ���� ����ϱ� ���� Ÿ�̸� ����

    // ��� �� ����� ����Ʈ �� ����
    UPROPERTY(EditAnywhere, Category = "Effects")
    UNiagaraSystem* DeathEffect; // ��� �� ���̾ư��� ����Ʈ

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* DeathSound; // ��� �� ����

    AActor* PlayerActor; // �÷��̾� ���Ϳ� ���� ����
    void ShootMissile(); // �̻����� �߻��ϴ� �Լ�
};