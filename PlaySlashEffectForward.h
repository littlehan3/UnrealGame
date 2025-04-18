#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "NiagaraSystem.h"
#include "PlaySlashEffectForward.generated.h"

UCLASS()
class LOCOMOTION_API UAnimNotify_PlaySlashEffectForward : public UAnimNotify
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Effect")
    UNiagaraSystem* SlashEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Effect")
    FVector LocationOffset = FVector::ZeroVector; // 위치 오프셋 (로컬기준)

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
