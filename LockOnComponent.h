#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOCOMOTION_API ULockOnSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    ULockOnSystem();

    void UpdateLockOnCameraRotation(float DeltaTime); // 카메라 회전 보정 함수 추가

protected:
    virtual void BeginPlay() override;
    
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; //TickComponent 함수 추가

public:
    void FindAndLockTarget();
    void UnlockTarget();
    bool IsLockedOn() const;
    bool IsTargetValid(AActor* Target) const; // 락온 대상이 유효한지 확인

    AActor* GetLockedTarget() const;

    void UpdateLockOnRotation(float DeltaTime);  // 락온 유지용 회전 업데이트 추가

private:
    UPROPERTY()
    AActor* LockedTarget;

    UPROPERTY()
    class AMainCharacter* OwnerCharacter;

    UPROPERTY(EditDefaultsOnly, Category = "Lock-On")
    float LockOnRadius = 1000.0f;  // 락온 가능 거리

    UPROPERTY(EditDefaultsOnly, Category = "Lock-On")
    float RotationSpeed = 5.0f;  // 락온 중 회전 속도
};
