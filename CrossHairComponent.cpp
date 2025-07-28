#include "CrossHairComponent.h"
#include "Kismet/KismetMathLibrary.h"

UCrossHairComponent::UCrossHairComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    CurrentSpread = DefaultSpread;
    TargetSpread = DefaultSpread;
    MovementSpread = 0.0f;
    bIsExpanding = false;
    bIsActive = false;
    ConsecutiveShots = 0;
    LastExpansionTime = 0.0f;
}

void UCrossHairComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentSpread = DefaultSpread;
    TargetSpread = DefaultSpread;
}

void UCrossHairComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsActive)
    {
        UpdateSpread(DeltaTime);
    }
}

void UCrossHairComponent::StartExpansion(float SpreadMultiplier)
{
    float CurrentTime = GetWorld()->GetTimeSeconds();

    // ���� ��� üũ
    if (CurrentTime - LastExpansionTime < ConsecutiveShotWindow)
    {
        ConsecutiveShots++;
    }
    else
    {
        ConsecutiveShots = 1;
    }

    LastExpansionTime = CurrentTime;

    // ���� ��ݿ� ���� �߰� Ȯ��
    float ConsecutiveMultiplier = 1.0f + (ConsecutiveShots - 1) * 0.3f;
    float FinalMultiplier = SpreadMultiplier * ConsecutiveMultiplier;

    TargetSpread = FMath::Clamp(DefaultSpread * FinalMultiplier, MinSpread, MaxSpread);
    bIsExpanding = true;
}

void UCrossHairComponent::SetMovementSpread(float NewMovementSpread)
{
    MovementSpread = FMath::Clamp(NewMovementSpread, 0.0f, MaxSpread * 0.5f);
    CalculateTargetSpread();
}

void UCrossHairComponent::SetCrosshairActive(bool bActive)
{
    bIsActive = bActive;
}

void UCrossHairComponent::UpdateSpread(float DeltaTime)
{
    CalculateTargetSpread();

    if (bIsExpanding)
    {
        CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetSpread, DeltaTime, SpreadSpeed);

        if (FMath::Abs(CurrentSpread - TargetSpread) < 0.5f)
        {
            bIsExpanding = false;
        }
    }
    else
    {
        CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetSpread, DeltaTime, RecoverySpeed);
    }
}

void UCrossHairComponent::CalculateTargetSpread()
{
    if (!bIsExpanding)
    {
        TargetSpread = DefaultSpread + MovementSpread;
        TargetSpread = FMath::Clamp(TargetSpread, MinSpread, MaxSpread);
    }
}

FVector2D UCrossHairComponent::GetTopLinePosition() const
{
    return FVector2D(0.0f, -CurrentSpread);
}

FVector2D UCrossHairComponent::GetBottomLinePosition() const
{
    return FVector2D(0.0f, CurrentSpread);
}

FVector2D UCrossHairComponent::GetLeftLinePosition() const
{
    return FVector2D(-CurrentSpread, 0.0f);
}

FVector2D UCrossHairComponent::GetRightLinePosition() const
{
    return FVector2D(CurrentSpread, 0.0f);
}

FLinearColor UCrossHairComponent::GetCrosshairColor() const
{
    float ExpandRatio = (CurrentSpread - DefaultSpread) / (MaxSpread - DefaultSpread);
    ExpandRatio = FMath::Clamp(ExpandRatio, 0.0f, 1.0f);

    return FMath::Lerp(DefaultColor, ExpandedColor, ExpandRatio);
}

float UCrossHairComponent::GetBulletSpreadAngle() const
{
    // ũ�ν���� �л��� ź�� �л� ������ ��ȯ
    float SpreadRatio = GetNormalizedSpread();
    return FMath::Lerp(BaseBulletSpread, MaxBulletSpread, SpreadRatio);
}

float UCrossHairComponent::GetNormalizedSpread() const
{
    // ���� �л��� 0.0 ~ 1.0 ���̷� ����ȭ
    float NormalizedSpread = (CurrentSpread - MinSpread) / (MaxSpread - MinSpread);
    return FMath::Clamp(NormalizedSpread, 0.0f, 1.0f);
}
