#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Kismet/KismetMathLibrary.h"
#include "BossEnemyAnimInstance.generated.h"

UCLASS()
class LOCOMOTION_API UBossEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UBossEnemyAnimInstance();

	// Getter 함수 선언 (애니메이션 ABP에서 변수 가져올 수 있도록)
	UFUNCTION(BlueprintCallable, Category = "Animation")
	float GetSpeed() const { return Speed; }

	UFUNCTION(BlueprintCallable, Category = "Animation")
	float GetDirection() const { return Direction; }

	UFUNCTION(BlueprintCallable, Category = "Animation")
	bool GetIsDead() const { return bIsDead; }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bUseUpperBodyBlend = false; // 상하체 분리 사용 여부

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class ABossEnemy* BossEnemyCharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	float Speed;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	float Direction;

	float CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation);

};
