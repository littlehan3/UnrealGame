#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h" // AActor 클래스 상속
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수 사용
#include "DrawDebugHelpers.h" // 디버그 시각화 기능 사용
#include "NiagaraComponent.h" // 나이아가라 컴포넌트 사용
#include "NiagaraSystem.h" // 나이아가라 시스템 사용
#include "Components/StaticMeshComponent.h" // 스태틱 메쉬 컴포넌트 사용
#include "EnemyShooterGun.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyShooterGun : public AActor
{
	GENERATED_BODY()

public:
	AEnemyShooterGun(); // 생성자

	void HideGun(); // 총을 숨기고 정리하는 함수
	void FireGun(); // 총 발사 로직 시작 함수

	UFUNCTION()
	void ExecuteDelayedShot(); // 조준 경고 시간 후 실제 발사를 실행하는 함수

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출

private:
	// 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* GunMesh; // 총의 외형을 나타내는 스태틱 메쉬

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MuzzleSocket; // 총구 위치를 나타내는 소켓용 메쉬

	UPROPERTY(VisibleAnywhere, Category = "Visual Effects")
	UStaticMeshComponent* AimingLaserMesh; // 조준 경고용 레이저를 표시할 스태틱 메쉬

	// 이펙트 및 사운드
	UPROPERTY(EditAnywhere, Category = "Effects")
	USoundBase* FireSound; // 격발 사운드

	UPROPERTY(EditAnywhere, Category = "Effects")
	USoundBase* AimWarningSound; // 조준 경고 사운드

	UPROPERTY(EditAnywhere, Category = "Effects")
	class UNiagaraSystem* MuzzleFlash; // 총구 섬광 이펙트

	UPROPERTY(EditAnywhere, Category = "Effects")
	class UNiagaraSystem* ImpactEffect; // 피격 지점 이펙트

	// 레이저 설정
	UPROPERTY(EditAnywhere, Category = "Laser Settings")
	UStaticMesh* AimingLaserMeshAsset; // 레이저로 사용할 스태틱 메쉬 에셋

	UPROPERTY(EditAnywhere, Category = "Laser Settings")
	UMaterialInterface* AimingLaserMaterial; // 레이저에 적용할 머티리얼

	// 무기 스탯
	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float Damage = 30.0f; // 데미지

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float FireRange = 1500.0f; // 최대 사정거리

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float Accuracy = 0.9f; // 명중률 (0~1)

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float MaxSpreadRadius = 100.0f; // 빗나갈 경우 최대 탄착군 반경

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float PredictionTime = 0.35f; // 플레이어의 미래 위치를 예측할 시간

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	bool bShowAimWarning = true; // 조준 경고 레이저 표시 여부

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float AimWarningTime = 0.3f; // 조준 경고 후 실제 발사까지 걸리는 시간

	// 레이저 시각 효과 설정
	UPROPERTY(EditAnywhere, Category = "Laser Settings")
	float AimingLaserScale = 1.0f; // 레이저 굵기

	UPROPERTY(EditAnywhere, Category = "Laser Settings")
	FVector AimingLaserOffset; // 레이저 위치 오프셋

	UPROPERTY(EditAnywhere, Category = "Laser Settings")
	FRotator AimingLaserRotationOffset; // 레이저 회전 오프셋

	// 파티클 이펙트 상세 설정
	UPROPERTY(EditAnywhere, Category = "Effects")
	float MuzzleFlashDuration = 0.15f; // 총구 섬광 지속 시간

	UPROPERTY(EditAnywhere, Category = "Effects")
	float MuzzleFlashScale = 1.0f; // 총구 섬광 크기

	UPROPERTY(EditAnywhere, Category = "Effects")
	float MuzzleFlashPlayRate = 2.0f; // 총구 섬광 재생 속도

	UPROPERTY(EditAnywhere, Category = "Effects")
	float ImpactEffectDuration = 0.3f; // 피격 이펙트 지속 시간

	// 지연 발사를 위해 임시로 데이터를 저장할 변수
	FVector StoredMuzzleLocation; // 발사 시점의 총구 위치
	FVector StoredTargetLocation; // 발사 시점의 목표 위치

	// 타이머 핸들
	FTimerHandle MuzzleFlashTimerHandle;
	FTimerHandle ImpactEffectTimerHandle;
	FTimerHandle DelayedShotTimerHandle;
	FTimerHandle AimingLaserTimerHandle;

	// 현재 재생 중인 이펙트 컴포넌트에 대한 참조
	UPROPERTY()
	class UNiagaraComponent* CurrentMuzzleFlashComponent;
	UPROPERTY()
	class UNiagaraComponent* CurrentImpactEffectComponent;

	// 내부 관리 함수
	FVector CalculatePredictedPlayerPosition(APawn* Player); // 플레이어의 미래 위치 예측
	FVector ApplyAccuracySpread(FVector TargetLocation); // 명중률에 따라 탄착군 형성
	void ShowAimingLaserMesh(FVector StartLocation, FVector EndLocation); // 레이저 표시
	void HideAimingLaserMesh(); // 레이저 숨김
	void SetupLaserTransform(FVector StartLocation, FVector EndLocation); // 레이저 메쉬의 Transform 조절
	void PlayMuzzleFlash(FVector Location); // 총구 섬광 재생
	void PlayImpactEffect(FVector Location, FVector ImpactNormal); // 피격 이펙트 재생
	void StopMuzzleFlash(); // 총구 섬광 정지
	void StopImpactEffect(); // 피격 이펙트 정지
	bool PerformLineTrace(FVector StartLocation, FVector EndLocation, FHitResult& OutHitResult); // 라인 트레이스 실행
};