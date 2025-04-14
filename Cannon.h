#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cannon.generated.h"

class AAimSkill2Projectile;

UCLASS()
class LOCOMOTION_API ACannon : public AActor
{
	GENERATED_BODY()

public:
	ACannon();

	void FireProjectile();
	void SetShooter(AActor* InShooter);
	void SetProjectileClass(TSubclassOf<AAimSkill2Projectile> InClass);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* CannonMesh;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAimSkill2Projectile> ProjectileClass;

	UPROPERTY()
	AActor* Shooter;
};