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
    // �������Ʈ���� ���ε��� Border ������Ʈ��
    UPROPERTY(BlueprintReadOnly, Category = "Crosshair", meta = (BindWidget))
    UBorder* TopLine;

    UPROPERTY(BlueprintReadOnly, Category = "Crosshair", meta = (BindWidget))
    UBorder* BottomLine;

    UPROPERTY(BlueprintReadOnly, Category = "Crosshair", meta = (BindWidget))
    UBorder* LeftLine;

    UPROPERTY(BlueprintReadOnly, Category = "Crosshair", meta = (BindWidget))
    UBorder* RightLine;

    // C++ ������Ʈ ����
    UPROPERTY()
    UCrossHairComponent* CrossHairCompRef;

    // �ʱ�ȭ �� ������Ʈ �Լ�
    void InitializeCrossHairReference();
    void UpdateCrossHairVisuals();
    void UpdateCrossHairPositions();
    void UpdateCrossHairColors();

public:
    // �������Ʈ���� ȣ�� ������ �Լ���
    UFUNCTION(BlueprintCallable, Category = "Crosshair")
    void SetCrossHairComponentReference(UCrossHairComponent* Component);

    UFUNCTION(BlueprintPure, Category = "Crosshair")
    bool IsComponentValid() const { return IsValid(CrossHairCompRef); }
};
