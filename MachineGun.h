#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MachineGun.generated.h"

class USoundBase;
class UNiagaraSystem;
class UNiagaraComponent;
class UStaticMeshComponent;

UCLASS()
class LOCOMOTION_API AMachineGun : public AActor
{
    GENERATED_BODY()

public:
    AMachineGun();

    void StartFire(); // 발사 시작 함수
    void StopFire();  // 발사 종료 함수
	void SetFireParams(float InFireRate, float InDamage, float InSpreadAngle); // 총알 발사 관련 파라미터 설정 함수

protected:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float KnockbackStrength = 300.0f; // 넉백 강도

    UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* FireSound; // 발사 사운드

    UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* StartFireSound; // 발사 시작 사운드

    UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* StopFireSound; // 발사 종료 사운드

private:
    void Fire(); // 발사 함수
	FVector GetFireDirectionWithSpread(FVector BaseDirection); // 퍼짐이 적용된 발사 방향 계산 함수

    // 총기 설정값
	UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
    float FireRate = 0.05f; // 초당 발사 간격 (0.1f = 초당 10발 0.05 = 초당 20발)
    UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
    float BulletDamage = 10.0f; // 데미지
    UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float SpreadAngle = 2.0f; // 퍼짐 정도 (도 단위)
    UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
    float MaxRange = 10000.0f; // 최대 사거리

	FTimerHandle FireTimerHandle; // 발사 타이머 핸들

    UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* GunMesh; // 총기 메쉬
    UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MuzzleSocket; // 총구 소켓 메쉬

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* MuzzleFlash; // 머즐 플래시
    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	float MuzzleFlashDuration = 0.15f; // 머즐 플래시 지속시간
    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    float MuzzleFlashScale = 1.0f; // 머즐 플래시 크기
    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	float MuzzleFlashPlayRate = 2.0f; // 머즐 플래시 재생 속도

	FTimerHandle MuzzleFlashTimerHandle; // 머즐 플래시 정지용 타이머 핸들
    UPROPERTY()
	class UNiagaraComponent* CurrentMuzzleFlashComponent; // 머즐 플래시 컴포넌트 참조

	void PlayMuzzleFlash(FVector Location); // 총구 섬광 재생 함수
    void StopMuzzleFlash(); // 총구 섬광 정지 함수

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class USoundBase* HitSound; // 피격 사운드
    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    class UNiagaraSystem* ImpactEffect; // 피격 이펙트
    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    float ImpactEffectDuration = 0.2f; // 피격 이펙트 지속시간

	FTimerHandle ImpactEffectTimerHandle; // 피격 이펙트 정지용 타이머 핸들

    UPROPERTY()
	class UNiagaraComponent* CurrentImpactEffectComponent; // 피격 이펙트 컴포넌트 참조
	void StopImpactEffect(); // 피격 이펙트 정지 함수
};
