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

    // ���� ��� ��ġ�� ȸ��
    FVector DroneLocation = GetPawn()->GetActorLocation();
    FRotator DroneRotation = GetPawn()->GetActorRotation();
    FVector PlayerLocation = PlayerActor->GetActorLocation();

    // �÷��̾�κ��� �Ÿ�
    float DistFromPlayer = FVector::Dist2D(DroneLocation, PlayerLocation);

    // �˵� ��� üũ
    if (DistFromPlayer > OrbitRadius + RadiusTolerance || DistFromPlayer < OrbitRadius - RadiusTolerance)
    {
        TimeOutOfRadius += DeltaTime;

        if (TimeOutOfRadius >= OutOfRadiusLimit)
        {
            // �����̵� ��� ��ǥ ���̸� ������ ��
            TargetHeight = DroneLocation.Z + 1000.f;
            bRising = true; // ��� ����
            TimeOutOfRadius = 0.f;
        }
    }
    else
    {
        // �ݰ� ���̸� Ÿ�̸� ����
        TimeOutOfRadius = 0.f;
    }


    if (bRising)
    {
        FVector MoveUpLocation = DroneLocation;
        MoveUpLocation.Z = FMath::FInterpTo(DroneLocation.Z, TargetHeight, DeltaTime, 1.0f); // 2.0f �� �ӵ�

        GetPawn()->SetActorLocation(MoveUpLocation, true);

        // ��ǥ ���̿� ���� �����ϸ� ��� ����
        if (FMath::IsNearlyEqual(MoveUpLocation.Z, TargetHeight, 5.f))
        {
            bRising = false;
        }
    }

    // ȸ������ ���� (���ӵ� ����)
    float Direction = bClockwise ? 1.0f : -1.0f;
    CurrentAngle += OrbitSpeed * DeltaTime * Direction;
    if (CurrentAngle >= 360.f) CurrentAngle -= 360.f;
    if (CurrentAngle < 0.f) CurrentAngle += 360.f;

    // ���� ��ȯ
    float Radian = FMath::DegreesToRadians(CurrentAngle);
    // ��ǥ ��ġ ��� (X,Y�� �� ����, Z�� �÷��̾� ���� ���� ����)
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
                // ���� ��� �� ���� ��100
                float RandomSign = (FMath::RandBool()) ? 1.f : -1.f;
                HeightOffset = FMath::Clamp(HeightOffset + 150.f * RandomSign, 150.f, 1000.f);
            }
            else
            {
                // ��/�ǹ� �� ���� ����
                bClockwise = !bClockwise;

                // �̹� ���� �õ��� �� ������ ���� ó��
                if (bTriedReverse)
                {
                    TimeStuck += DeltaTime;
                    if (TimeStuck >= MaxStuckTime)
                    {
                        // ū �� ��� �� �ݰ� ����
                        HeightOffset = BaseHeight + 1000.f;
                        TimeStuck = 0.f;
                        bTriedReverse = false;
                    }
                }
                else
                {
                    bTriedReverse = true; // ù ���� ���
                    TimeStuck = 0.f;      // Ÿ�̸� �ʱ�ȭ
                }
            }

            DrawDebugLine(GetWorld(), DroneLocation, HitResult.ImpactPoint, FColor::Green, false, 0.1f, 0, 1.f);
        }
    }
    else
    {
        // �浹 ������ ������ �⺻ ���� ����
        HeightOffset = FMath::FInterpTo(HeightOffset, BaseHeight, DeltaTime, 0.5f);
        bTriedReverse = false;
        TimeStuck = 0.f;
    }


    // �ε巴�� �̵�
    //FVector CurrentLocation = DroneLocation;
    FVector NewLocation = FMath::VInterpTo(DroneLocation, TargetLocation, DeltaTime, 2.0f); // 2�� ���� �ӵ�
    GetPawn()->SetActorLocation(NewLocation, true);

    // �׻� �÷��̾ �ٶ󺸰� ȸ��
    FVector ToPlayer = PlayerLocation - NewLocation;
    FRotator LookAtRotation = FRotationMatrix::MakeFromX(ToPlayer).Rotator();
    GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAtRotation, DeltaTime, 5.0f));
}