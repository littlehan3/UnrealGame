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
    FVector LocationOffset = FVector::ZeroVector; // ��ġ ������ (���ñ���)

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
