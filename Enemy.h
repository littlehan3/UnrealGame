#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Enemy.generated.h"

UCLASS()
class LOCOMOTION_API AEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemy();

protected:
    virtual void BeginPlay() override;

public:
    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator,
        AActor* DamageCauser
    ) override;

private:
    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* HitSound;

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* DieSound;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float Health = 100.0f;

	bool bIsDead = false;

    void Die();
};
