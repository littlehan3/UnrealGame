#include "LockOnComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"  // 필수 추가
#include "Enemy.h" 
#include "DrawDebugHelpers.h"
#include "MainCharacter.h"
#include "DrawDebugHelpers.h" //래이케스트 디버그 시각화를 위한 디버깅헤더

ULockOnSystem::ULockOnSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void ULockOnSystem::BeginPlay()
{
    Super::BeginPlay();
    OwnerCharacter = Cast<AMainCharacter>(GetOwner());
}

void ULockOnSystem::FindAndLockTarget()
{
    if (!OwnerCharacter) return;

    FVector PlayerLocation = OwnerCharacter->GetActorLocation();
    AEnemy* BestTarget = nullptr;
    float BestDistance = LockOnRadius;

    // 락온 가능 범위를 디버그 스피어로 시각화
    DrawDebugSphere(GetWorld(), PlayerLocation, LockOnRadius, 12, FColor::Green, false, 2.0f, 0, 2.0f);

    for (TActorIterator<AEnemy> It(GetWorld()); It; ++It)
    {
        AEnemy* Candidate = *It;
        if (Candidate == Cast<ACharacter>(OwnerCharacter)) continue;
        if (!Candidate->CanBeLockedOn()) continue;

        FVector TargetLocation = Candidate->GetActorLocation();

        // 레이캐스트(LineTrace) 실행
        FHitResult HitResult;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(OwnerCharacter);
        Params.AddIgnoredActor(Candidate);

        bool bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult, PlayerLocation, TargetLocation, ECC_Visibility, Params
        );

        // 레이캐스트 결과 시각화 (적이 보이면 파란색, 장애물에 막히면 빨간색)
        FColor LineColor = bHit ? FColor::Red : FColor::Blue;
        DrawDebugLine(GetWorld(), PlayerLocation, TargetLocation, LineColor, false, 2.0f, 0, 2.0f);

        if (bHit && HitResult.GetActor() != Candidate)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Lock-On Blocked"), *Candidate->GetName());
            continue;
        }

        float Distance = FVector::Dist(PlayerLocation, TargetLocation);
        if (Distance < BestDistance)
        {
            BestTarget = Candidate;
            BestDistance = Distance;
        }
    }

    if (BestTarget)
    {
        LockedTarget = BestTarget;
        UE_LOG(LogTemp, Warning, TEXT("Locked On Target: %s"), *LockedTarget->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid target found for Lock-On."));
    }
}

void ULockOnSystem::UnlockTarget()
{
    if (LockedTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("Lock-On Released: %s"), *LockedTarget->GetName());
        LockedTarget = nullptr;
    }
}

bool ULockOnSystem::IsLockedOn() const
{
    return LockedTarget != nullptr;
}

AActor* ULockOnSystem::GetLockedTarget() const
{
    return LockedTarget;
}

// 락온된 상태에서 캐릭터의 방향을 강제로 조정
void ULockOnSystem::UpdateLockOnRotation(float DeltaTime)
{
    if (!OwnerCharacter || !LockedTarget) return;

    FVector TargetLocation = LockedTarget->GetActorLocation();
    FVector DirectionToTarget = (TargetLocation - OwnerCharacter->GetActorLocation()).GetSafeNormal();

    FRotator TargetRotation = DirectionToTarget.Rotation();
    TargetRotation.Pitch = 0.0f; // 캐릭터의 피치 값 고정 (상하 움직임 방지)

    FRotator NewRotation = FMath::RInterpTo(OwnerCharacter->GetActorRotation(), TargetRotation, DeltaTime, RotationSpeed);
    OwnerCharacter->SetActorRotation(NewRotation);

}

void ULockOnSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerCharacter || !LockedTarget) return;

    float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), LockedTarget->GetActorLocation());
	float LockOnReleaseRadius = LockOnRadius * 1.2f; // 락온 사거리의 1.2배만큼 멀어질 시 락온 해제

    if (Distance > LockOnReleaseRadius)
    {
        UE_LOG(LogTemp, Warning, TEXT("Lock-On Target Too Far! Lock-On Released."));
        UnlockTarget();
        return;
    }

    // 락온 상태일 때 카메라가 자동으로 타겟을 바라보도록 설정
    UpdateLockOnCameraRotation(DeltaTime);
}

void ULockOnSystem::UpdateLockOnCameraRotation(float DeltaTime)
{
    if (!OwnerCharacter || !LockedTarget) return;

    APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PlayerController) return;

    FVector TargetLocation = LockedTarget->GetActorLocation();
    FVector PlayerLocation = OwnerCharacter->GetActorLocation();

    // 카메라가 바라볼 방향 계산 (플레이어 → 락온 대상)
    FVector DirectionToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

    // 타겟을 바라보는 회전 값 계산
    FRotator TargetRotation = DirectionToTarget.Rotation();

    // 피치값을 조정하여 카메라가 위아래로 크게 움직이지 않도록 제한
    TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch, -30.0f, 30.0f);

    // 부드러운 회전 적용
    FRotator NewControlRotation = FMath::RInterpTo(PlayerController->GetControlRotation(), TargetRotation, DeltaTime, RotationSpeed);

    // 컨트롤러 회전 적용
    PlayerController->SetControlRotation(NewControlRotation);
}



