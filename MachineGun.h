#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/StaticMeshComponent.h"
#include "MachineGun.generated.h"

class USoundBase;

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

    // 총알에 맞은 적에게 가할 넉백 강도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    float KnockbackStrength = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Audio")
    USoundBase* FireSound;

    /** [신규] 발사 시작 시 재생할 사운드 (예: 준비, 촤르륵) */
    UPROPERTY(EditAnywhere, Category = "Audio")
    USoundBase* StartFireSound;

    /** [신규] 발사 종료 시 재생할 사운드 (예: 약실 냉각, 딸칵) */
    UPROPERTY(EditAnywhere, Category = "Audio")
    USoundBase* StopFireSound;

private:
    void Fire(); // 1발 발사
    FVector GetFireDirectionWithSpread(); // 탄퍼짐 적용 방향

    // 총기 설정값
    float FireRate = 0.05f;    // 초당 발사 간격 (ex. 0.1f = 초당 10발)
    float BulletDamage = 10.0f; // 데미지
    float SpreadAngle = 2.0f; // 퍼짐 정도 (degree)

    FTimerHandle FireTimerHandle;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* GunMesh;

    /** [신규 추가] 총구 위치를 나타내는 소켓용 메쉬 (BP에서 위치 조절 가능) */
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MuzzleSocket;

    /** [신규 추가] 총구 섬광 이펙트 (나이아가라) */
    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* MuzzleFlash;

    /** 총구 섬광 지속 시간 */
    UPROPERTY(EditAnywhere, Category = "Effects")
    float MuzzleFlashDuration = 0.15f;

    /** 총구 섬광 크기 */
    UPROPERTY(EditAnywhere, Category = "Effects")
    float MuzzleFlashScale = 1.0f;

    /** 총구 섬광 재생 속도 */
    UPROPERTY(EditAnywhere, Category = "Effects")
    float MuzzleFlashPlayRate = 2.0f;

    /** 머즐 플래시 정지용 타이머 핸들 */
    FTimerHandle MuzzleFlashTimerHandle;

    /** 현재 재생 중인 이펙트 컴포넌트 참조 */
    UPROPERTY()
    class UNiagaraComponent* CurrentMuzzleFlashComponent;

    /** 총구 섬광 재생 */
    void PlayMuzzleFlash(FVector Location);
    /** 총구 섬광 정지 */
    void StopMuzzleFlash();

    /** [신규 추가] 피격 시 재생할 사운드 */
    UPROPERTY(EditAnywhere, Category = "Effects")
    class USoundBase* HitSound;

    /** [신규 추가] 피격 시 재생할 이펙트 */
    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* ImpactEffect;

    /** 히트 임팩트 재생 시간 */
    UPROPERTY(EditAnywhere, Category = "Effects")
    float ImpactEffectDuration = 0.2f; // ARifle과 동일하게 설정

    /** 히트 이펙트 정지용 타이머 핸들 */
    FTimerHandle ImpactEffectTimerHandle;

    /** 현재 재생 중인 히트 이펙트 컴포넌트 참조 */
    UPROPERTY()
    class UNiagaraComponent* CurrentImpactEffectComponent;

    void StopImpactEffect();
};
