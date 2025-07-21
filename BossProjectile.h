#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "BossProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class LOCOMOTION_API ABossProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	virtual void Tick(float DeltaTime) override;

	ABossProjectile();

	void SetDamage(float InDamage) { Damage = InDamage; }
	void SetShooter(AActor* InShooter) { Shooter = InShooter; }

	void FireInDirection(const FVector& ShootDir);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit);

	void ApplyAreaDamage();

private:	

	UPROPERTY(VisibleAnywhere) 
	USphereComponent* CollisionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	float CollisionRadius = 18.f;

	UPROPERTY(VisibleAnywhere) 
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(VisibleAnywhere) 
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* ExplosionEffect;

	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* ExplosionSound;

	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* LoopingFlightSound;

	UPROPERTY()
	UAudioComponent* FlightAudioComponent = nullptr;

	float Damage = 40.f;   // 데미지
	float DamageRadius = 400.f;  // 광역 반경
	AActor* Shooter = nullptr;

};
