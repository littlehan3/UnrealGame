#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rifle.generated.h"

UCLASS()
class LOCOMOTION_API ARifle : public AActor
{
    GENERATED_BODY()

public:
    ARifle();
    void Fire();
    void Reload();
    void ResetFire();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* RifleMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MuzzleSocket; 

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* FireSound;

    UPROPERTY(EditAnywhere, Category = "Effects")
    UParticleSystem* MuzzleFlash;

    UPROPERTY(EditAnywhere, Category = "Effects")
    UParticleSystem* ImpactEffect;

    UPROPERTY(EditAnywhere, Category = "Effects")
    UParticleSystem* BulletTrail;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    int32 CurrentAmmo = 30;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    int32 MaxAmmo = 30;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    int32 TotalAmmo = 60;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float FireRate = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float Damage = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float Range = 5000.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weapon Stats")
    bool bCanFire = true;

    FTimerHandle FireRateTimerHandle;

    void ProcessHit(const FHitResult& HitResult, FVector ShotDirection);
};
