#include "EnemyDroneAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "EnemyDrone.h"

AEnemyDroneAIController::AEnemyDroneAIController()
{
    PrimaryActorTick.bCanEverTick = true;

    OrbitRadius;
    OrbitSpeed;
    HeightOffset;
    BaseHeight;
    CurrentAngle;

    TimeStuck;
    MaxStuckTime;
    bTriedReverse;
}

void AEnemyDroneAIController::BeginPlay()
{
    Super::BeginPlay();
    PlayerActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

}

void AEnemyDroneAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!PlayerActor || !GetPawn()) return;

    // 현재 드론 위치와 회전
    FVector DroneLocation = GetPawn()->GetActorLocation();
    FRotator DroneRotation = GetPawn()->GetActorRotation();
    FVector PlayerLocation = PlayerActor->GetActorLocation();

    // 플레이어로부터 거리
    float DistFromPlayer = FVector::Dist2D(DroneLocation, PlayerLocation);

    // 궤도 벗어남 체크
    if (DistFromPlayer > OrbitRadius + RadiusTolerance || DistFromPlayer < OrbitRadius - RadiusTolerance)
    {
        TimeOutOfRadius += DeltaTime;

        if (TimeOutOfRadius >= OutOfRadiusLimit)
        {
            // 순간이동 대신 목표 높이를 설정만 함
            TargetHeight = DroneLocation.Z + 1000.f;
            bRising = true; // 상승 시작
            TimeOutOfRadius = 0.f;
        }
    }
    else
    {
        // 반경 안이면 타이머 리셋
        TimeOutOfRadius = 0.f;
    }


    if (bRising)
    {
        FVector MoveUpLocation = DroneLocation;
        MoveUpLocation.Z = FMath::FInterpTo(DroneLocation.Z, TargetHeight, DeltaTime, 1.0f); // 2.0f 는 속도

        GetPawn()->SetActorLocation(MoveUpLocation, true);

        // 목표 높이에 거의 도달하면 상승 종료
        if (FMath::IsNearlyEqual(MoveUpLocation.Z, TargetHeight, 5.f))
        {
            bRising = false;
        }
    }

    // 회전각도 증가 (각속도 적용)
    float Direction = bClockwise ? 1.0f : -1.0f;
    CurrentAngle += OrbitSpeed * DeltaTime * Direction;
    if (CurrentAngle >= 360.f) CurrentAngle -= 360.f;
    if (CurrentAngle < 0.f) CurrentAngle += 360.f;

    // 라디안 변환
    float Radian = FMath::DegreesToRadians(CurrentAngle);
    // 목표 위치 계산 (X,Y는 원 궤적, Z는 플레이어 기준 고정 높이)
    //FVector PlayerLocation = PlayerActor->GetActorLocation();
    FVector Offset = FVector(FMath::Cos(Radian) * OrbitRadius, FMath::Sin(Radian) * OrbitRadius, HeightOffset);
    FVector TargetLocation = PlayerLocation + Offset;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetPawn());

    FHitResult HitResult;
    if (GetWorld()->LineTraceSingleByChannel(HitResult, DroneLocation, TargetLocation, ECC_Visibility, QueryParams))
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor && HitActor != GetPawn())
        {
            if (HitActor->IsA(AEnemyDrone::StaticClass()))
            {
                // 같은 드론 → 고도만 ±100
                float RandomSign = (FMath::RandBool()) ? 1.f : -1.f;
                HeightOffset = FMath::Clamp(HeightOffset + 150.f * RandomSign, 150.f, 1000.f);
            }
            else
            {
                // 벽/건물 → 방향 반전
                bClockwise = !bClockwise;

                // 이미 반전 시도한 적 있으면 끼임 처리
                if (bTriedReverse)
                {
                    TimeStuck += DeltaTime;
                    if (TimeStuck >= MaxStuckTime)
                    {
                        // 큰 고도 상승 후 반경 복귀
                        HeightOffset = BaseHeight + 1000.f;
                        TimeStuck = 0.f;
                        bTriedReverse = false;
                    }
                }
                else
                {
                    bTriedReverse = true; // 첫 반전 기록
                    TimeStuck = 0.f;      // 타이머 초기화
                }
            }

            DrawDebugLine(GetWorld(), DroneLocation, HitResult.ImpactPoint, FColor::Green, false, 0.1f, 0, 1.f);
        }
    }
    else
    {
        // 충돌 없으면 서서히 기본 고도로 복귀
        HeightOffset = FMath::FInterpTo(HeightOffset, BaseHeight, DeltaTime, 0.5f);
        bTriedReverse = false;
        TimeStuck = 0.f;
    }


    // 부드럽게 이동
    //FVector CurrentLocation = DroneLocation;
    FVector NewLocation = FMath::VInterpTo(DroneLocation, TargetLocation, DeltaTime, 2.0f); // 2는 보간 속도
    GetPawn()->SetActorLocation(NewLocation, true);

    // 항상 플레이어를 바라보게 회전
    FVector ToPlayer = PlayerLocation - NewLocation;
    FRotator LookAtRotation = FRotationMatrix::MakeFromX(ToPlayer).Rotator();
    GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAtRotation, DeltaTime, 5.0f));
}