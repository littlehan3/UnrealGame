#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnemyDodgeLaunch.generated.h"

UCLASS()
class LOCOMOTION_API UAnimNotify_EnemyDodgeLaunch : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	bool bDodgeLeft = true; // ±âº»°ª: ¿ÞÂÊ ´åÁö

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
