#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_BossFireProjectile.generated.h"

UCLASS()
class LOCOMOTION_API UAnimNotify_BossFireProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 애니메이션이 해당 프레임에 도달했을 때 호출
	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation) override;
	
};
