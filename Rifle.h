#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"  
#include "Rifle.generated.h"

class USceneComponent;
class AMainCharacter;

UCLASS()
class LOCOMOTION_API ARifle : public AActor
{
    GENERATED_BODY()

public:
    ARifle();
    void Reload();
    void ResetFire();
    void Fire(float CrosshairSpreadAngle = 0.0f); // 크로스헤어 연동 발사 함수

    bool IsReloading() const { return bIsReloading; } // 재장전 상태 getter
    int32 GetCurrentAmmo() const { return CurrentAmmo; } // 현재 총알 수 getter

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accuracy")
    float MovementSpreadMultiplier = 1.5f; // 이동 중 분산 배율

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accuracy")
    float AimSpreadReduction = 0.3f; // 에임 모드 시 분산 감소 (더 정확하게)

private:
    void ProcessHit(const FHitResult& HitResult, FVector ShotDirection);
    void FinishReload(); // 재장전 완료 함수 추가

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* RifleMesh;

    // 머즐 소켓 컴포넌트 추가
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MuzzleSocket;

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* FireSound;

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* ReloadSound;  // 재장전 사운드 추가

    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* MuzzleFlash;  // 파티클 -> 나이아가라

    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* ImpactEffect;  // 파티클 -> 나이아가라

    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* BulletTrail;   // 파티클 -> 나이아가라

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    int32 CurrentAmmo = 30;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    int32 MaxAmmo = 30;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    int32 TotalAmmo = 300;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float FireRate = 0.15f;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float ReloadTime = 2.0f; // 재장전 시간 설정

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float Damage = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float Range = 5000.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weapon Stats")
    bool bCanFire = true;

    UPROPERTY(VisibleAnywhere, Category = "Weapon Stats")
    bool bIsReloading = false; // 재장전 중인지 여부 확인

    FTimerHandle FireRateTimerHandle;
    FTimerHandle ReloadTimerHandle; // 재장전 타이머 추가

    // 머즐 플래시 설정 변수들 추가
    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float MuzzleFlashDuration = 0.15f; // 머즐 플래시 지속 시간

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float MuzzleFlashScale = 0.1f; // 머즐 플래시 크기 조절

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.5", ClampMax = "3.0"))
    float MuzzleFlashPlayRate = 2.0f; // 머즐 플래시 재생 속도

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float ImpactEffectDuration = 0.3f; // 히트 이펙트 재생 시간

    FTimerHandle MuzzleFlashTimerHandle; // 머즐 플래시 타이머
    FTimerHandle ImpactEffectTimerHandle; // 히트 이팩트 타이머

    UPROPERTY()
    class UNiagaraComponent* CurrentMuzzleFlashComponent; // 머즐 플래시 컴포넌트 참조

    UPROPERTY()
    class UNiagaraComponent* CurrentImpactEffectComponent; // 히트 이펙트 컴포넌트 참조

    void StopMuzzleFlash(); // 머즐 플래시 정지 함수
    void StopImpactEffect(); // 히트 이펙트 정지 함수 추가

};
