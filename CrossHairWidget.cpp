#include "CrossHairWidget.h"
#include "MainCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

void UCrossHairWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // �ڵ����� ũ�ν���� ������Ʈ ���� ȹ��
    InitializeCrossHairReference();

    // �ʱ� ����
    if (IsValid(CrossHairCompRef))
    {
        UpdateCrossHairVisuals();
        UE_LOG(LogTemp, Warning, TEXT("CrossHair Widget Successfully Initialized"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to Initialize CrossHair Component Reference"));
    }
}

void UCrossHairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // ������Ʈ�� ��ȿ�� ���� ������Ʈ
    if (IsValid(CrossHairCompRef))
    {
        UpdateCrossHairVisuals();
    }
}

void UCrossHairWidget::InitializeCrossHairReference()
{
    // �÷��̾� ĳ���Ϳ��� ũ�ν���� ������Ʈ �ڵ� ȹ��
    if (AMainCharacter* MainChar = Cast<AMainCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
    {
        CrossHairCompRef = MainChar->GetCrosshairComponent();

        if (IsValid(CrossHairCompRef))
        {
            UE_LOG(LogTemp, Warning, TEXT("CrossHair Component Reference Acquired Successfully"));
        }
    }
}

void UCrossHairWidget::UpdateCrossHairVisuals()
{
    if (!IsValid(CrossHairCompRef)) return;

    // ��ġ ������Ʈ
    UpdateCrossHairPositions();

    // ���� ������Ʈ
    UpdateCrossHairColors();
}

void UCrossHairWidget::UpdateCrossHairPositions()
{
    if (!CrossHairCompRef) return;

    // C++ �Լ����� ���� ��ġ �� ��������
    FVector2D TopPos = CrossHairCompRef->GetTopLinePosition();
    FVector2D BottomPos = CrossHairCompRef->GetBottomLinePosition();
    FVector2D LeftPos = CrossHairCompRef->GetLeftLinePosition();
    FVector2D RightPos = CrossHairCompRef->GetRightLinePosition();

    // Border ������Ʈ���� ��ġ ������Ʈ
    if (TopLine)
        TopLine->SetRenderTranslation(TopPos);

    if (BottomLine)
        BottomLine->SetRenderTranslation(BottomPos);

    if (LeftLine)
        LeftLine->SetRenderTranslation(LeftPos);

    if (RightLine)
        RightLine->SetRenderTranslation(RightPos);
}

void UCrossHairWidget::UpdateCrossHairColors()
{
    if (!CrossHairCompRef) return;

    FLinearColor CurrentColor = CrossHairCompRef->GetCrosshairColor();

    // ��� ������ ���� ������Ʈ
    if (TopLine)
        TopLine->SetBrushColor(CurrentColor);

    if (BottomLine)
        BottomLine->SetBrushColor(CurrentColor);

    if (LeftLine)
        LeftLine->SetBrushColor(CurrentColor);

    if (RightLine)
        RightLine->SetBrushColor(CurrentColor);
}

void UCrossHairWidget::SetCrossHairComponentReference(UCrossHairComponent* Component)
{
    CrossHairCompRef = Component;

    if (IsValid(CrossHairCompRef))
    {
        UpdateCrossHairVisuals();
    }
}
