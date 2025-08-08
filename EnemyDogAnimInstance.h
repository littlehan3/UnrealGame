#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyDogAnimInstance.generated.h"

UCLASS()
class LOCOMOTION_API UEnemyDogAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
    UEnemyDogAnimInstance();

    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetSpeed() const { return Speed; }

    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetDirection() const { return Direction; }

    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool GetIsDead() const { return bIsDead; }

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bIsDead;

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaTime) override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    class AEnemyDog* EnemyDogCharacter;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    float Speed;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    float Direction;

    // 방향 계산 함수 
    float CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation);

};
