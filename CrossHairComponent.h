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

    // 블루프린트에서 호출 가능한 함수들
    UFUNCTION(BlueprintCallable, Category = "Crosshair")
    void StartExpansion(float SpreadMultiplier = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Crosshair")
    void SetMovementSpread(float NewMovementSpread);

    UFUNCTION(BlueprintCallable, Category = "Crosshair")
    void SetCrosshairActive(bool bActive);

    // 블루프린트에서 읽기 전용으로 접근 가능한 함수들
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

    // 탄도 분산 계산용 함수 추가
    UFUNCTION(BlueprintPure, Category = "Crosshair")
    float GetBulletSpreadAngle() const;

    UFUNCTION(BlueprintPure, Category = "Crosshair")
    float GetNormalizedSpread() const; // 0.0 ~ 1.0 사이의 정규화된 분산 값

protected:
    // 기본 크로스헤어 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair Settings")
    float DefaultSpread = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair Settings")
    float MaxSpread = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair Settings")
    float MinSpread = 5.0f;

    // 애니메이션 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
    float SpreadSpeed = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
    float RecoverySpeed = 4.0f;

    // 색상 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Settings")
    FLinearColor DefaultColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Settings")
    FLinearColor ExpandedColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);

    // 탄도 분산 관련 설정 추가
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Spread Settings")
    float BaseBulletSpread = 1.0f; // 기본 탄도 분산 각도 (도 단위)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Spread Settings")
    float MaxBulletSpread = 5.0f; // 최대 탄도 분산 각도 (도 단위)

private:
    // 내부 상태 변수
    float CurrentSpread;
    float TargetSpread;
    float MovementSpread;
    bool bIsExpanding;
    bool bIsActive;

    // 연속 사격 관리
    int32 ConsecutiveShots;
    float LastExpansionTime;
    float ConsecutiveShotWindow = 0.5f;

    void UpdateSpread(float DeltaTime);
    void CalculateTargetSpread();

};
