#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyBase : public AActor
{
    GENERATED_BODY()

public:
    AEnemyBase();

    // ü�� �� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    float Health;

    UPROPERTY(BlueprintReadOnly, Category = "Enemy")
    bool bIsDead;

    // �ǰ� ó��
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

    // ��� ó��
    virtual void Die();

protected:
    virtual void BeginPlay() override;
};
