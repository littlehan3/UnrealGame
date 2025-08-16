#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
//#include "EnemyGuardianShield.h"
#include "EnemyGuardian.generated.h"

class AEnemyGuardianShield;

UCLASS()
class LOCOMOTION_API AEnemyGuardian : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyGuardian();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	AEnemyGuardianShield* EquippedShield; // 무기 참조 

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyGuardianShield> ShieldClass;

};
