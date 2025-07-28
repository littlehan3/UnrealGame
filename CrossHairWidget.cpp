#include "CrossHairWidget.h"
#include "MainCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

void UCrossHairWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 자동으로 크로스헤어 컴포넌트 참조 획득
    InitializeCrossHairReference();

    // 초기 설정
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

    // 컴포넌트가 유효할 때만 업데이트
    if (IsValid(CrossHairCompRef))
    {
        UpdateCrossHairVisuals();
    }
}

void UCrossHairWidget::InitializeCrossHairReference()
{
    // 플레이어 캐릭터에서 크로스헤어 컴포넌트 자동 획득
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

    // 위치 업데이트
    UpdateCrossHairPositions();

    // 색상 업데이트
    UpdateCrossHairColors();
}

void UCrossHairWidget::UpdateCrossHairPositions()
{
    if (!CrossHairCompRef) return;

    // C++ 함수에서 직접 위치 값 가져오기
    FVector2D TopPos = CrossHairCompRef->GetTopLinePosition();
    FVector2D BottomPos = CrossHairCompRef->GetBottomLinePosition();
    FVector2D LeftPos = CrossHairCompRef->GetLeftLinePosition();
    FVector2D RightPos = CrossHairCompRef->GetRightLinePosition();

    // Border 컴포넌트들의 위치 업데이트
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

    // 모든 라인의 색상 업데이트
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
