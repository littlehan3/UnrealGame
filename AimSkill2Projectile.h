#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AimSkill2Projectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class LOCOMOTION_API AAimSkill2Projectile : public AActor
{
    GENERATED_BODY()

public:
    AAimSkill2Projectile();

    void FireInDirection(const FVector& ShootDirection);
    void SetShooter(AActor* InShooter);

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

private:
    UPROPERTY(VisibleAnywhere)
    USphereComponent* CollisionComp;

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY()
    AActor* Shooter;

    UPROPERTY(EditDefaultsOnly)
    float PullStrength = 3000.0f;

    UPROPERTY(EditDefaultsOnly)
    float LifeSpanSeconds = 3.0f;
};
