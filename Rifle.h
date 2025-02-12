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
    void ProcessHit(const FHitResult& HitResult, FVector ShotDirection);
    void FinishReload(); // 재장전 완료 함수 추가

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* RifleMesh;

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* FireSound;

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* ReloadSound;  // 재장전 사운드 추가

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
    float ReloadTime = 2.0f; // 재장전 시간 설정

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float Damage = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float Range = 5000.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weapon Stats")
    bool bCanFire = true;

    UPROPERTY(VisibleAnywhere, Category = "Weapon Stats")
    bool bIsReloading = false; // 재장전 중인지 여부 확인

    FTimerHandle FireRateTimerHandle;
    FTimerHandle ReloadTimerHandle; // 재장전 타이머 추가
};
