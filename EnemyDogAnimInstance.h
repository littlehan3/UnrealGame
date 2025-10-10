#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h" // UAnimInstance 클래스를 상속받기 위해 포함
#include "EnemyDogAnimInstance.generated.h"

UCLASS()
class LOCOMOTION_API UEnemyDogAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UEnemyDogAnimInstance(); // 생성자

    // 애니메이션 블루프린트에서 호출하여 속도 값을 가져갈 함수
    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetSpeed() const { return Speed; }

    // 애니메이션 블루프린트에서 호출하여 방향 값을 가져갈 함수
    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetDirection() const { return Direction; }

    // 애니메이션 블루프린트에서 호출하여 사망 상태를 가져갈 함수
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool GetIsDead() const { return bIsDead; }

    // 애니메이션 블루프린트와 연동될 변수
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bIsDead; // 사망 여부

protected:
    virtual void NativeInitializeAnimation() override; // 애니메이션 인스턴스 초기화 시 호출
    virtual void NativeUpdateAnimation(float DeltaTime) override; // 매 프레임 애니메이션 갱신 시 호출

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    class AEnemyDog* EnemyDogCharacter; // 이 애님 인스턴스의 소유자인 EnemyDog 캐릭터에 대한 참조

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    float Speed; // 캐릭터의 현재 속도

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    float Direction; // 캐릭터의 현재 이동 방향 (전방 기준 -180 ~ 180도)

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    bool bIsInAirStun; // 공중 스턴 상태 여부

    // 속도와 회전값을 기반으로 이동 방향 각도를 계산하는 함수
    float CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation);
};