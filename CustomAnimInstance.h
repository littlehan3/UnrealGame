#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CustomAnimInstance.generated.h"

UCLASS()
class LOCOMOTION_API UCustomAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float Speed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float Direction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float AimPitch;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bIsJumping;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bIsInDoubleJump;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bIsInAir;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bIsAiming;
};
