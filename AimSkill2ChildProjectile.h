#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "AimSkill2ChildProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class LOCOMOTION_API AAimSkill2ChildProjectile : public AActor
{
    GENERATED_BODY()

public:
    AAimSkill2ChildProjectile();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    void SetTarget(AActor* InTarget);
    void SetShooter(AActor* InShooter) { Shooter = InShooter; }

protected:
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    void Explode();

private:
    UPROPERTY(VisibleAnywhere)
    USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(EditDefaultsOnly)
    UNiagaraSystem* ExplosionEffect;

    UPROPERTY(EditAnywhere)
    float Damage = 10.0f;

    UPROPERTY(EditAnywhere)
    float DamageRadius = 150.0f;

    UPROPERTY(EditAnywhere)
    float FlySpeed = 1500.0f;

    AActor* Target = nullptr;
    AActor* Shooter = nullptr;
};
