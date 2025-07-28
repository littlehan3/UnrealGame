#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "CrossHairComponent.generated.h"

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOCOMOTION_API UCrossHairComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCrossHairComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // �������Ʈ���� ȣ�� ������ �Լ���
    UFUNCTION(BlueprintCallable, Category = "Crosshair")
    void StartExpansion(float SpreadMultiplier = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Crosshair")
    void SetMovementSpread(float NewMovementSpread);

    UFUNCTION(BlueprintCallable, Category = "Crosshair")
    void SetCrosshairActive(bool bActive);

    // �������Ʈ���� �б� �������� ���� ������ �Լ���
    UFUNCTION(BlueprintPure, Category = "Crosshair")
    float GetCurrentSpread() const { return CurrentSpread; }

    UFUNCTION(BlueprintPure, Category = "Crosshair")
    FVector2D GetTopLinePosition() const;

    UFUNCTION(BlueprintPure, Category = "Crosshair")
    FVector2D GetBottomLinePosition() const;

    UFUNCTION(BlueprintPure, Category = "Crosshair")
    FVector2D GetLeftLinePosition() const;

    UFUNCTION(BlueprintPure, Category = "Crosshair")
    FVector2D GetRightLinePosition() const;

    UFUNCTION(BlueprintPure, Category = "Crosshair")
    FLinearColor GetCrosshairColor() const;

    // ź�� �л� ���� �Լ� �߰�
    UFUNCTION(BlueprintPure, Category = "Crosshair")
    float GetBulletSpreadAngle() const;

    UFUNCTION(BlueprintPure, Category = "Crosshair")
    float GetNormalizedSpread() const; // 0.0 ~ 1.0 ������ ����ȭ�� �л� ��

protected:
    // �⺻ ũ�ν���� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair Settings")
    float DefaultSpread = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair Settings")
    float MaxSpread = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair Settings")
    float MinSpread = 5.0f;

    // �ִϸ��̼� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
    float SpreadSpeed = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
    float RecoverySpeed = 4.0f;

    // ���� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Settings")
    FLinearColor DefaultColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Settings")
    FLinearColor ExpandedColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);

    // ź�� �л� ���� ���� �߰�
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Spread Settings")
    float BaseBulletSpread = 1.0f; // �⺻ ź�� �л� ���� (�� ����)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Spread Settings")
    float MaxBulletSpread = 5.0f; // �ִ� ź�� �л� ���� (�� ����)

private:
    // ���� ���� ����
    float CurrentSpread;
    float TargetSpread;
    float MovementSpread;
    bool bIsExpanding;
    bool bIsActive;

    // ���� ��� ����
    int32 ConsecutiveShots;
    float LastExpansionTime;
    float ConsecutiveShotWindow = 0.5f;

    void UpdateSpread(float DeltaTime);
    void CalculateTargetSpread();

};
