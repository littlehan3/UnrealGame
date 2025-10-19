#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h" // 엑터 클래스 사용하기 위해 포함
#include "NiagaraComponent.h" // 나이아가라 시스템 사용하기 위해 포함
#include "NiagaraSystem.h" // 나이아가라 시스템 에셋 사용하기 위해 포함
#include "Rifle.generated.h"

class USceneComponent; // 씬컴포넌트클래스 전방선언
class AMainCharacter; // 메인캐릭터 전방선언

UCLASS()
class LOCOMOTION_API ARifle : public AActor
{
    GENERATED_BODY()

public:
    ARifle();
    void Reload(); // 재장전 함수
    void ResetFire(); // 발사 속도에 따라 다시 발사 할 수 있는 상태로 만드는 함수
    void Fire(float CrosshairSpreadAngle = 0.0f); // 크로스헤어 벌어짐 각도를 인자로 받는 발사 함수

    bool IsReloading() const { return bIsReloading; } // 재장전 상태 반환 getter
    int32 GetCurrentAmmo() const { return CurrentAmmo; } // 현재 총알 수 반환 getter

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accuracy")
    float MovementSpreadMultiplier = 1.5f; // 이동 시 총알의 분산도 증가 배율

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accuracy")
    float AimSpreadReduction = 0.3f; // 에임모드 시 총알의 분산도 감소 배율

private:
    void ProcessHit(const FHitResult& HitResult, FVector ShotDirection); // 피해처리 함수
    void FinishReload(); // 재장전 완료 함수

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* RifleMesh; // 총 메쉬

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MuzzleSocket; // 머즐 소켓 메쉬

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* FireSound; // 격발 사운드

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* ReloadSound;  // 재장전 사운드

    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* MuzzleFlash;  // 머즐 플래쉬

    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* ImpactEffect;  // 히트 임팩트

    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* BulletTrail;   // 궤적 임팩트

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    int32 CurrentAmmo = 30; // 장전된 총알 수 

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    int32 MaxAmmo = 30; // 탄창 최대 총알 수

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    int32 TotalAmmo = 300; // 전체 총알 수 

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float FireRate = 0.15f; // 연사속도

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float ReloadTime = 2.0f; // 재장전 시간

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float Damage = 30.0f; // 데미지

    UPROPERTY(EditAnywhere, Category = "Weapon Stats")
    float Range = 5000.0f; // 사정거리

    UPROPERTY(VisibleAnywhere, Category = "Weapon Stats")
    bool bCanFire = true; // 격발 가능여부

    UPROPERTY(VisibleAnywhere, Category = "Weapon Stats")
    bool bIsReloading = false; // 재장전 중 여부

    FTimerHandle FireRateTimerHandle; // 발사속도 제어 타이머
    FTimerHandle ReloadTimerHandle; // 재장전 타이머 추가

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float MuzzleFlashDuration = 0.15f; // 머즐 플래시 지속 시간

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float MuzzleFlashScale = 0.1f; // 머즐 플래시 크기 조절

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.5", ClampMax = "3.0"))
    float MuzzleFlashPlayRate = 3.0f; // 머즐 플래시 재생 속도

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float ImpactEffectDuration = 0.2f; // 히트 이펙트 재생 시간

    FTimerHandle MuzzleFlashTimerHandle; // 머즐 플래시 타이머
    FTimerHandle ImpactEffectTimerHandle; // 히트 이팩트 타이머

    UPROPERTY()
    class UNiagaraComponent* CurrentMuzzleFlashComponent; // 머즐 플래시 컴포넌트 참조

    UPROPERTY()
    class UNiagaraComponent* CurrentImpactEffectComponent; // 히트 이펙트 컴포넌트 참조

    void StopMuzzleFlash(); // 머즐 플래시 정지 함수
    void StopImpactEffect(); // 히트 이펙트 정지 함수 추가

};
