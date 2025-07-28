#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "CrossHairComponent.h"
#include "CrossHairWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class LOCOMOTION_API UCrossHairWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // 블루프린트에서 바인딩할 Border 컴포넌트들
    UPROPERTY(BlueprintReadOnly, Category = "Crosshair", meta = (BindWidget))
    UBorder* TopLine;

    UPROPERTY(BlueprintReadOnly, Category = "Crosshair", meta = (BindWidget))
    UBorder* BottomLine;

    UPROPERTY(BlueprintReadOnly, Category = "Crosshair", meta = (BindWidget))
    UBorder* LeftLine;

    UPROPERTY(BlueprintReadOnly, Category = "Crosshair", meta = (BindWidget))
    UBorder* RightLine;

    // C++ 컴포넌트 참조
    UPROPERTY()
    UCrossHairComponent* CrossHairCompRef;

    // 초기화 및 업데이트 함수
    void InitializeCrossHairReference();
    void UpdateCrossHairVisuals();
    void UpdateCrossHairPositions();
    void UpdateCrossHairColors();

public:
    // 블루프린트에서 호출 가능한 함수들
    UFUNCTION(BlueprintCallable, Category = "Crosshair")
    void SetCrossHairComponentReference(UCrossHairComponent* Component);

    UFUNCTION(BlueprintPure, Category = "Crosshair")
    bool IsComponentValid() const { return IsValid(CrossHairCompRef); }
};
