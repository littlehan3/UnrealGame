#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyDroneMissile.h"
#include "NiagaraSystem.h"
#include "EnemyDrone.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyDrone : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemyDrone();

    UPROPERTY()
    TArray<AEnemyDroneMissile*> MissilePool;
    AEnemyDroneMissile* GetAvailableMissileFromPool();

    virtual float TakeDamage( // �������� �Ծ����� ȣ��Ǵ� �Լ� (AActor�� TakeDamage �������̵�)
        float DamageAmount, // ���� ������ ��
        struct FDamageEvent const& DamageEvent, // ������ �̺�Ʈ ���� 
        class AController* EventInstigator, // �������� ���� ��Ʈ�ѷ�
        AActor* DamageCauser // �������� ������ ����
    ) override;

    void Die();
    void HideEnemy();
    float Health = 40.0f;
    bool bIsDead = false;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditDefaultsOnly, Category = "Pawn")
    TSubclassOf<AEnemyDroneMissile> MissileClass;

    float MissileCooldown = 3.0f;
    float MissileTimer = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Effects")
    UNiagaraSystem* DeathEffect;

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* DeathSound;

    AActor* PlayerActor;
    void ShootMissile();
};
