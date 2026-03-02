#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AimSkill3Projectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UAudioComponent;

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
    TObjectPtr<USphereComponent> CollisionComponent = nullptr;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement = nullptr;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;

    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<UNiagaraSystem> ExplosionEffect = nullptr;

    // 낙하 중 루핑 사운드 컴포넌트
    UPROPERTY(VisibleAnywhere, Category = "Audio")
    TObjectPtr<UAudioComponent> LoopingSoundComponent = nullptr;

    // 낙하 중 재생할 루핑 사운드 에셋
    UPROPERTY(EditAnywhere, Category = "Audio")
    TObjectPtr<USoundBase> LoopingSound = nullptr;

    // 폭발 시 재생할 사운드 에셋
    UPROPERTY(EditAnywhere, Category = "Audio", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USoundBase> ExplosionSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "Stat", meta = (AllowPrivateAccess = "true"))
    float Damage = 500.f;

    UPROPERTY(EditAnywhere, Category = "Stat", meta = (AllowPrivateAccess = "true"))
    float ExplosionRadius = 300.f;

	UPROPERTY(VisibleAnywhere, Category = "State", meta = (AllowPrivateAccess = "true"))
    bool bExploded = false;
};