#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MachineGun.generated.h"

UCLASS()
class LOCOMOTION_API AMachineGun : public AActor
{
    GENERATED_BODY()

public:
    AMachineGun();

    void StartFire(); // 발사 시작
    void StopFire();  // 발사 종료
    void SetFireParams(float InFireRate, float InDamage, float InSpreadAngle);

protected:
    virtual void BeginPlay() override;

private:
    void Fire(); // 1발 발사
    FVector GetFireDirectionWithSpread(); // 탄퍼짐 적용 방향

    // 총기 설정값
    float FireRate = 0.1f;    // 초당 발사 간격 (ex. 0.1f = 초당 10발)
    float BulletDamage = 10.0f; // 데미지
    float SpreadAngle = 2.0f; // 퍼짐 정도 (degree)

    FTimerHandle FireTimerHandle;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* GunMesh;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UParticleSystem* MuzzleEffect;
};
