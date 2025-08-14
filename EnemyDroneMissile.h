#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyDroneMissile.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyDroneMissile : public AActor
{
    GENERATED_BODY()

public:
    AEnemyDroneMissile();

    void SetTarget(AActor* Target);
    void ResetMissile(FVector SpawnLocation, AActor* NewTarget);
    void Explode();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UProjectileMovementComponent* ProjectileMovement;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UStaticMeshComponent* MeshComp;

    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* ExplosionEffect;

    UPROPERTY(EditAnywhere, Category = "Missile")
    float Damage = 10.f;

    UPROPERTY(EditAnywhere, Category = "Missile")
    float Health = 10.f;

private:
    AActor* TargetActor;
    FVector LastMoveDirection;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

    bool bExploded = false;

    UPROPERTY(EditAnywhere, Category = "Missile")
    float ExplosionRadius = 50.f;
};
