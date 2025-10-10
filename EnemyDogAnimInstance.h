#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h" // UAnimInstance Ŭ������ ��ӹޱ� ���� ����
#include "EnemyDogAnimInstance.generated.h"

UCLASS()
class LOCOMOTION_API UEnemyDogAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UEnemyDogAnimInstance(); // ������

    // �ִϸ��̼� �������Ʈ���� ȣ���Ͽ� �ӵ� ���� ������ �Լ�
    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetSpeed() const { return Speed; }

    // �ִϸ��̼� �������Ʈ���� ȣ���Ͽ� ���� ���� ������ �Լ�
    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetDirection() const { return Direction; }

    // �ִϸ��̼� �������Ʈ���� ȣ���Ͽ� ��� ���¸� ������ �Լ�
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool GetIsDead() const { return bIsDead; }

    // �ִϸ��̼� �������Ʈ�� ������ ����
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bIsDead; // ��� ����

protected:
    virtual void NativeInitializeAnimation() override; // �ִϸ��̼� �ν��Ͻ� �ʱ�ȭ �� ȣ��
    virtual void NativeUpdateAnimation(float DeltaTime) override; // �� ������ �ִϸ��̼� ���� �� ȣ��

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    class AEnemyDog* EnemyDogCharacter; // �� �ִ� �ν��Ͻ��� �������� EnemyDog ĳ���Ϳ� ���� ����

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    float Speed; // ĳ������ ���� �ӵ�

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    float Direction; // ĳ������ ���� �̵� ���� (���� ���� -180 ~ 180��)

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    bool bIsInAirStun; // ���� ���� ���� ����

    // �ӵ��� ȸ������ ������� �̵� ���� ������ ����ϴ� �Լ�
    float CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation);
};