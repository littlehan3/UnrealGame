#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "AimSkill3Projectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class LOCOMOTION_API AAimSkill3Projectile : public AActor
{
    GENERATED_BODY()

public:
    AAimSkill3Projectile();

    virtual void Tick(float DeltaTime) override;

    void FireInDirection(const FVector& ShootDirection);
    void SetExplosionParams(float InDamage, float InRadius);

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
    void Explode();

    UPROPERTY(VisibleAnywhere)
    USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(EditDefaultsOnly)
    UNiagaraSystem* ExplosionEffect;

    // 낙하 중 루핑 사운드 컴포넌트
    UPROPERTY(VisibleAnywhere, Category = "Audio")
    class UAudioComponent* LoopingSoundComponent;

    // 낙하 중 재생할 루핑 사운드 에셋
    UPROPERTY(EditAnywhere, Category = "Audio")
    class USoundBase* LoopingSound;

    // 폭발 시 재생할 사운드 에셋
    UPROPERTY(EditAnywhere, Category = "Audio")
    class USoundBase* ExplosionSound;

    UPROPERTY(EditAnywhere)
    float Damage = 500.f;

    UPROPERTY(EditAnywhere)
    float ExplosionRadius = 300.f;

    bool bExploded = false;
};