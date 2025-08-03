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

    // 체력 및 상태
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    float Health;

    UPROPERTY(BlueprintReadOnly, Category = "Enemy")
    bool bIsDead;

    // 피격 처리
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

    // 사망 처리
    virtual void Die();

protected:
    virtual void BeginPlay() override;
};
