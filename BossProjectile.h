#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UAudioComponent;

UCLASS()
class LOCOMOTION_API ABossProjectile : public AActor
{
	GENERATED_BODY()
	
public:	

	ABossProjectile();

	void SetDamage(float InDamage) { Damage = InDamage; } // 데미지 설정
	void SetShooter(AActor* InShooter) { Shooter = InShooter; } // 발사자 설정
	void FireInDirection(const FVector& ShootDir); // 발사 방향 설정

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit); // 충돌 처리
	void ApplyAreaDamage(); // 광역 데미지 적용

private:	

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement = nullptr;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true")) 
	TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> ExplosionEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "Projectile|Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> ExplosionSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Projectile|Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> LoopingFlightSound = nullptr;

	UPROPERTY()
	TObjectPtr<UAudioComponent> FlightAudioComponent = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> Shooter = nullptr;

	UPROPERTY(EditAnywhere, Category = "Projectile|Collision", meta = (AllowPrivateAccess = "true"))
	float CollisionRadius = 18.f;

	UPROPERTY(EditAnywhere, Category = "Projectile|Stat", meta = (AllowPrivateAccess = "true"))
	float Damage = 20.f;   // 데미지
	UPROPERTY(EditAnywhere, Category = "Projectile|Stat", meta = (AllowPrivateAccess = "true"))
	float DamageRadius = 150.f;  // 광역 반경

};
