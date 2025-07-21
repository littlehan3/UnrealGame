#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_BossFireProjectile.generated.h"

UCLASS()
class LOCOMOTION_API UAnimNotify_BossFireProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	// �ִϸ��̼��� �ش� �����ӿ� �������� �� ȣ��
	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation) override;
	
};
