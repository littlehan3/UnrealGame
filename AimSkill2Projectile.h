#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "AimSkill2Projectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class LOCOMOTION_API AAimSkill2Projectile : public AActor
{
    GENERATED_BODY()

public:
    AAimSkill2Projectile();

    void SetDamage(float InDamage) { Damage = InDamage; }
    void SetShooter(AActor* InShooter) { Shooter = InShooter; }
    virtual void Tick(float DeltaTime) override;
    void FireInDirection(const FVector& ShootDirection);

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    void ApplyAreaDamage(); // 광역 피해 함수 및 끌어당김 효과

private:
    UPROPERTY(VisibleAnywhere)
    USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(EditDefaultsOnly)
    UNiagaraSystem* ExplosionEffect;

    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    USoundBase* ExplosionSound;

    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    USoundBase* LoopingFlightSound;

    UPROPERTY()
    UAudioComponent* FlightAudioComponent; // 비행 사운드용 컴포넌트

    float Damage = 10.0f;
    float DamageRadius = 150.0f; // 광역 피해 반경

    AActor* Shooter;

    float PullStrength = 1000.0f; // 기본값 설정
};
