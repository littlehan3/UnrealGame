#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h" // AActor Ŭ���� ���
#include "Kismet/GameplayStatics.h" // �����÷��� ��ƿ��Ƽ �Լ� ���
#include "DrawDebugHelpers.h" // ����� �ð�ȭ ��� ���
#include "NiagaraComponent.h" // ���̾ư��� ������Ʈ ���
#include "NiagaraSystem.h" // ���̾ư��� �ý��� ���
#include "Components/StaticMeshComponent.h" // ����ƽ �޽� ������Ʈ ���
#include "EnemyShooterGun.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyShooterGun : public AActor
{
	GENERATED_BODY()

public:
	AEnemyShooterGun(); // ������

	void HideGun(); // ���� ����� �����ϴ� �Լ�
	void FireGun(); // �� �߻� ���� ���� �Լ�

	UFUNCTION()
	void ExecuteDelayedShot(); // ���� ��� �ð� �� ���� �߻縦 �����ϴ� �Լ�

protected:
	virtual void BeginPlay() override; // ���� ���� �� ȣ��

private:
	// ������Ʈ
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* GunMesh; // ���� ������ ��Ÿ���� ����ƽ �޽�

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MuzzleSocket; // �ѱ� ��ġ�� ��Ÿ���� ���Ͽ� �޽�

	UPROPERTY(VisibleAnywhere, Category = "Visual Effects")
	UStaticMeshComponent* AimingLaserMesh; // ���� ���� �������� ǥ���� ����ƽ �޽�

	// ����Ʈ �� ����
	UPROPERTY(EditAnywhere, Category = "Effects")
	USoundBase* FireSound; // �ݹ� ����

	UPROPERTY(EditAnywhere, Category = "Effects")
	USoundBase* AimWarningSound; // ���� ��� ����

	UPROPERTY(EditAnywhere, Category = "Effects")
	class UNiagaraSystem* MuzzleFlash; // �ѱ� ���� ����Ʈ

	UPROPERTY(EditAnywhere, Category = "Effects")
	class UNiagaraSystem* ImpactEffect; // �ǰ� ���� ����Ʈ

	// ������ ����
	UPROPERTY(EditAnywhere, Category = "Laser Settings")
	UStaticMesh* AimingLaserMeshAsset; // �������� ����� ����ƽ �޽� ����

	UPROPERTY(EditAnywhere, Category = "Laser Settings")
	UMaterialInterface* AimingLaserMaterial; // �������� ������ ��Ƽ����

	// ���� ����
	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float Damage = 30.0f; // ������

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float FireRange = 1500.0f; // �ִ� �����Ÿ�

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float Accuracy = 0.9f; // ���߷� (0~1)

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float MaxSpreadRadius = 100.0f; // ������ ��� �ִ� ź���� �ݰ�

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float PredictionTime = 0.35f; // �÷��̾��� �̷� ��ġ�� ������ �ð�

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	bool bShowAimWarning = true; // ���� ��� ������ ǥ�� ����

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float AimWarningTime = 0.3f; // ���� ��� �� ���� �߻���� �ɸ��� �ð�

	// ������ �ð� ȿ�� ����
	UPROPERTY(EditAnywhere, Category = "Laser Settings")
	float AimingLaserScale = 1.0f; // ������ ����

	UPROPERTY(EditAnywhere, Category = "Laser Settings")
	FVector AimingLaserOffset; // ������ ��ġ ������

	UPROPERTY(EditAnywhere, Category = "Laser Settings")
	FRotator AimingLaserRotationOffset; // ������ ȸ�� ������

	// ��ƼŬ ����Ʈ �� ����
	UPROPERTY(EditAnywhere, Category = "Effects")
	float MuzzleFlashDuration = 0.15f; // �ѱ� ���� ���� �ð�

	UPROPERTY(EditAnywhere, Category = "Effects")
	float MuzzleFlashScale = 1.0f; // �ѱ� ���� ũ��

	UPROPERTY(EditAnywhere, Category = "Effects")
	float MuzzleFlashPlayRate = 2.0f; // �ѱ� ���� ��� �ӵ�

	UPROPERTY(EditAnywhere, Category = "Effects")
	float ImpactEffectDuration = 0.3f; // �ǰ� ����Ʈ ���� �ð�

	// ���� �߻縦 ���� �ӽ÷� �����͸� ������ ����
	FVector StoredMuzzleLocation; // �߻� ������ �ѱ� ��ġ
	FVector StoredTargetLocation; // �߻� ������ ��ǥ ��ġ

	// Ÿ�̸� �ڵ�
	FTimerHandle MuzzleFlashTimerHandle;
	FTimerHandle ImpactEffectTimerHandle;
	FTimerHandle DelayedShotTimerHandle;
	FTimerHandle AimingLaserTimerHandle;

	// ���� ��� ���� ����Ʈ ������Ʈ�� ���� ����
	UPROPERTY()
	class UNiagaraComponent* CurrentMuzzleFlashComponent;
	UPROPERTY()
	class UNiagaraComponent* CurrentImpactEffectComponent;

	// ���� ���� �Լ�
	FVector CalculatePredictedPlayerPosition(APawn* Player); // �÷��̾��� �̷� ��ġ ����
	FVector ApplyAccuracySpread(FVector TargetLocation); // ���߷��� ���� ź���� ����
	void ShowAimingLaserMesh(FVector StartLocation, FVector EndLocation); // ������ ǥ��
	void HideAimingLaserMesh(); // ������ ����
	void SetupLaserTransform(FVector StartLocation, FVector EndLocation); // ������ �޽��� Transform ����
	void PlayMuzzleFlash(FVector Location); // �ѱ� ���� ���
	void PlayImpactEffect(FVector Location, FVector ImpactNormal); // �ǰ� ����Ʈ ���
	void StopMuzzleFlash(); // �ѱ� ���� ����
	void StopImpactEffect(); // �ǰ� ����Ʈ ����
	bool PerformLineTrace(FVector StartLocation, FVector EndLocation, FHitResult& OutHitResult); // ���� Ʈ���̽� ����
};