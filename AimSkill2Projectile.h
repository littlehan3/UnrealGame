#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
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
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	void FireInDirection(const FVector& ShootDirection);
	void SetDamage(float InDamage) { Damage = InDamage; }
	void SetShooter(AActor* InShooter) { Shooter = InShooter; }

protected:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void ApplyAreaDamage();
	AActor* FindClosestEnemy();
	void EvaluateTargetAfterApex();
	void AutoExplodeIfNoTarget();

private:
	UPROPERTY(VisibleAnywhere)
	USphereComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* ExplosionEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* CascadeEffect;

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* ExplosionSound;

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* LoopingFlightSound;

	UPROPERTY()
	UAudioComponent* FlightAudioComponent;

	AActor* Shooter;

	float Damage = 10.0f;
	float DamageRadius = 150.0f;
	float PullStrength = 1000.0f;

	bool bHasReachedApex = false;
	float ApexHeight = 500.0f;
	float DelayBeforeTracking = 0.5f;
	float DetectionRadius = 1200.0f;

	FTimerHandle DelayTimerHandle;
};