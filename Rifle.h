#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h" // 엑터 클래스 사용하기 위해 포함
#include "Rifle.generated.h"

class USoundBase; // 사운드 베이스 전방선언
class UNiagaraSystem; // 나이아가라 시스템 전방선언
class UNiagaraComponent; // 나이아가라 컴포넌트 전방선언
class UStaticMeshComponent; // 스태틱 메쉬 컴포넌트 전방선언

UCLASS()
class LOCOMOTION_API ARifle : public AActor
{
    GENERATED_BODY()

public:
    ARifle();
    void Reload(); // 재장전 함수
    void ResetFire(); // 발사 속도에 따라 다시 발사 할 수 있는 상태로 만드는 함수
    void Fire(float CrosshairSpreadAngle = 0.0f); // 크로스헤어 벌어짐 각도를 인자로 받는 발사 함수

    FORCEINLINE  bool IsReloading() const { return bIsReloading; } // 재장전 상태 반환 getter

    // UMG 바인딩을 위해 UFUNCTION으로 노출
    UFUNCTION(BlueprintPure, Category = "Weapon Stats")
    int32 GetCurrentAmmo() const { return CurrentAmmo; } // 현재 총알 수 반환 getter

    // UMG 바인딩을 위해 UFUNCTION으로 노출
    UFUNCTION(BlueprintPure, Category = "Weapon Stats")
    int32 GetMaxAmmo() const { return MaxAmmo; } // 최대 총알 수 반환 Getter

    UFUNCTION(BlueprintPure, Category = "Weapon Stats")
    int32 GetTotalAmmo() const { return TotalAmmo;  } // 전체 총알 수 반환 Getter

    // [신규] 전체 탄약 수를 추가하는 함수 선언
    void AddTotalAmmo(int32 AmmoToAdd);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accuracy")
    float MovementSpreadMultiplier = 1.5f; // 이동 시 총알의 분산도 증가 배율

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accuracy")
    float AimSpreadReduction = 0.3f; // 에임모드 시 총알의 분산도 감소 배율

private:
    void ProcessHit(const FHitResult& HitResult, FVector ShotDirection); // 피해처리 함수
    void FinishReload(); // 재장전 완료 함수

    UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* RifleMesh = nullptr; // 총 메쉬

    UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* MuzzleSocket = nullptr; // 머즐 소켓 메쉬

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* FireSound = nullptr; // 격발 사운드

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* ReloadSound = nullptr;  // 재장전 사운드

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    class UNiagaraSystem* MuzzleFlash = nullptr;  // 머즐 플래쉬

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    class UNiagaraSystem* ImpactEffect = nullptr;  // 히트 임팩트

    UPROPERTY(EditAnywhere, Category = "Weapon Stats", meta = (AllowPrivateAccess = "true"))
    int32 CurrentAmmo = 30; // 장전된 총알 수 

    UPROPERTY(EditAnywhere, Category = "Weapon Stats", meta = (AllowPrivateAccess = "true"))
    int32 MaxAmmo = 30; // 탄창 최대 총알 수

    UPROPERTY(EditAnywhere, Category = "Weapon Stats", meta = (AllowPrivateAccess = "true"))
    int32 TotalAmmo = 90; // 전체 총알 수 

    UPROPERTY(EditAnywhere, Category = "Weapon Stats", meta = (AllowPrivateAccess = "true"))
    float FireRate = 0.15f; // 연사속도

    UPROPERTY(EditAnywhere, Category = "Weapon Stats", meta = (AllowPrivateAccess = "true"))
    float ReloadTime = 2.0f; // 재장전 시간

    UPROPERTY(EditAnywhere, Category = "Weapon Stats", meta = (AllowPrivateAccess = "true"))
    float Damage = 15.0f; // 데미지

    UPROPERTY(EditAnywhere, Category = "Weapon Stats", meta = (AllowPrivateAccess = "true"))
    float HeadshotDamageMultiplier = 10.0f; // 헤드샷 데미지 배수

    UPROPERTY(EditAnywhere, Category = "Weapon Stats", meta = (AllowPrivateAccess = "true"))
    float Range = 5000.0f; // 사정거리

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon State", meta = (AllowPrivateAccess = "true"))
    bool bIsReloading = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon State", meta = (AllowPrivateAccess = "true"))
    bool bCanFire = true; // 격발 가능여부

    FTimerHandle FireRateTimerHandle; // 발사속도 제어 타이머
    FTimerHandle ReloadTimerHandle; // 재장전 타이머 추가

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.1", ClampMax = "5.0"), meta = (AllowPrivateAccess = "true"))
    float MuzzleFlashDuration = 0.15f; // 머즐 플래시 지속 시간

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.1", ClampMax = "10.0"), meta = (AllowPrivateAccess = "true"))
    float MuzzleFlashScale = 0.1f; // 머즐 플래시 크기 조절

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.5", ClampMax = "3.0"), meta = (AllowPrivateAccess = "true"))
    float MuzzleFlashPlayRate = 3.0f; // 머즐 플래시 재생 속도

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (ClampMin = "0.1", ClampMax = "5.0"), meta = (AllowPrivateAccess = "true"))
    float ImpactEffectDuration = 0.2f; // 히트 이펙트 재생 시간

    FTimerHandle MuzzleFlashTimerHandle; // 머즐 플래시 타이머
    FTimerHandle ImpactEffectTimerHandle; // 히트 이팩트 타이머

    UPROPERTY()
    class UNiagaraComponent* CurrentMuzzleFlashComponent = nullptr; // 머즐 플래시 컴포넌트 참조

    UPROPERTY()
    class UNiagaraComponent* CurrentImpactEffectComponent = nullptr; // 히트 이펙트 컴포넌트 참조

    void StopMuzzleFlash(); // 머즐 플래시 정지 함수
    void StopImpactEffect(); // 히트 이펙트 정지 함수 추가

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* ReloadAnnouncementSound = nullptr; // 재장전 알림 사운드

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* HitSound = nullptr; // 재장전 알림 사운드

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* HeadShotSound = nullptr; // 헤드샷 사운드


};
