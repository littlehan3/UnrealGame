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

    void StartFire(); // �߻� ����
    void StopFire();  // �߻� ����
    void SetFireParams(float InFireRate, float InDamage, float InSpreadAngle);

protected:
    virtual void BeginPlay() override;

private:
    void Fire(); // 1�� �߻�
    FVector GetFireDirectionWithSpread(); // ź���� ���� ����

    // �ѱ� ������
    float FireRate = 0.1f;    // �ʴ� �߻� ���� (ex. 0.1f = �ʴ� 10��)
    float BulletDamage = 10.0f; // ������
    float SpreadAngle = 2.0f; // ���� ���� (degree)

    FTimerHandle FireTimerHandle;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* GunMesh;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UParticleSystem* MuzzleEffect;
};
