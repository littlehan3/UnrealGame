#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Skill3Projectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class LOCOMOTION_API ASkill3Projectile : public AActor
{
    GENERATED_BODY()

public:
    ASkill3Projectile();

    void SetDamage(float InDamage) { Damage = InDamage; }
    void SetShooter(AActor* InShooter) { Shooter = InShooter; }
    virtual void Tick(float DeltaTime) override;
    void FireInDirection(const FVector& ShootDirection);

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    void ApplyAreaDamage(); // ���� ���� �Լ�

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
    UAudioComponent* FlightAudioComponent; // ���� ����� ������Ʈ

    float Damage = 60.0f;
    float DamageRadius = 150.0f; // ���� ���� �ݰ�

    AActor* Shooter;
};
