#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyKatana.h" 
#include "Enemy.generated.h"

class AEnemyKatana; 

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

    // 카타나 부착을 위한 변수 추가
    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    TSubclassOf<AEnemyKatana> KatanaClass;

    AEnemyKatana* EquippedKatana; 
};
