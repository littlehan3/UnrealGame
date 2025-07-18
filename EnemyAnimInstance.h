#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnemyAnimInstance.generated.h"


UCLASS()
class LOCOMOTION_API UEnemyAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UEnemyAnimInstance();

    // Getter 함수 (애니메이션 블루프린트에서 변수 가져올 수 있도록)
    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetSpeed() const { return Speed; }

    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetDirection() const { return Direction; }

    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool GetIsInAir() const { return bIsInAir; }

    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool GetIsDead() const { return bIsDead; }

    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool GetIsEliteEnemy() const { return bIsEliteEnemy; }

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bIsDead;

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaTime) override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    class AEnemy* EnemyCharacter;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    float Speed;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    float Direction;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    bool bIsInAir;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    bool bIsEliteEnemy;

    // 방향 계산 함수 
    float CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation);

};