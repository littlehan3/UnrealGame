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

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* CannonMesh;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAimSkill2Projectile> ProjectileClass;

	UPROPERTY()
	AActor* Shooter;

	UPROPERTY(EditAnywhere, Category = "Projectile|Offset")
	float SpawnForwardOffset = 200.f; // 투사체 소환 시작 거리

	UPROPERTY(EditAnywhere, Category = "Projectile|Offset")
	float SpawnVerticalOffset = -150.f; // 투사체 소환 높이 보정
};