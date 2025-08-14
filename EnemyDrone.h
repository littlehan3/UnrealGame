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

    virtual float TakeDamage( // 데미지를 입었을때 호출되는 함수 (AActor의 TakeDamage 오버라이드)
        float DamageAmount, // 입은 데미지 양
        struct FDamageEvent const& DamageEvent, // 데미지 이벤트 정보 
        class AController* EventInstigator, // 데미지를 가한 컨트롤러
        AActor* DamageCauser // 데미지를 유발한 엑터
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
