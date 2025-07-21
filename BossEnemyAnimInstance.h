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

	// Getter �Լ� ���� (�ִϸ��̼� ABP���� ���� ������ �� �ֵ���)
	UFUNCTION(BlueprintCallable, Category = "Animation")
	float GetSpeed() const { return Speed; }

	UFUNCTION(BlueprintCallable, Category = "Animation")
	float GetDirection() const { return Direction; }

	UFUNCTION(BlueprintCallable, Category = "Animation")
	bool GetIsDead() const { return bIsDead; }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bUseUpperBodyBlend = false; // ����ü �и� ��� ����

	// �÷��̾� �ٶ󺸱� ���� ������
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bShouldLookAtPlayer = false; // �÷��̾� �ٶ󺸱� Ȱ��ȭ ����

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	FRotator LookAtRotation = FRotator::ZeroRotator; // ��ǥ ȸ����

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float LookAtSpeed = 5.0f; // ȸ�� �ӵ�

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

	void UpdateLookAtRotation(float DeltaTime); // �÷��̾� �ٶ󺸱� ������Ʈ �Լ�

};