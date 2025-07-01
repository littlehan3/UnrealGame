#pragma once

#include "CoreMinimal.h"
#include "EnemyKatana.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnemyStartAttack.generated.h"

UCLASS()
class LOCOMOTION_API UAnimNotify_EnemyStartAttack : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	EAttackType AttackType = EAttackType::Normal;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
