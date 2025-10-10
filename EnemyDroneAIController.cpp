#include "EnemyDroneAIController.h"
#include "Kismet/GameplayStatics.h" // �����÷��� ��ƿ��Ƽ �Լ� ���
#include "GameFramework/Character.h" // ACharacter Ŭ���� ����
#include "EnemyDrone.h" // ������ ����� EnemyDrone Ŭ���� ����

AEnemyDroneAIController::AEnemyDroneAIController()
{
    PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ� ȣ�� Ȱ��ȭ
}

void AEnemyDroneAIController::BeginPlay()
{
    Super::BeginPlay(); // �θ� Ŭ���� BeginPlay ȣ��
    PlayerActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0); // �÷��̾� ���� ã�� ����
}

void AEnemyDroneAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // �θ� Ŭ���� Tick ȣ��
    if (!PlayerActor || !GetPawn()) return; // �÷��̾ ������ ���� ������ ���� �ߴ�

    FVector DroneLocation = GetPawn()->GetActorLocation(); // ���� ��� ��ġ
    FVector PlayerLocation = PlayerActor->GetActorLocation(); // ���� �÷��̾� ��ġ

    // �˵� ��Ż ���� �� ȸ�� ����
    float DistFromPlayer = FVector::Dist2D(DroneLocation, PlayerLocation); // �÷��̾���� ���� �Ÿ� ���
    // ���� �Ÿ��� ��ǥ �˵� �ݰ濡�� ��� ���� �̻����� ������� Ȯ��
    if (DistFromPlayer > OrbitRadius + RadiusTolerance || DistFromPlayer < OrbitRadius - RadiusTolerance)
    {
        TimeOutOfRadius += DeltaTime; // ��� �ð� ����
        if (TimeOutOfRadius >= OutOfRadiusLimit) // ���� �ð� �̻� ��� �־��ٸ�
        {
            TargetHeight = DroneLocation.Z + 1000.f; // ���� ������ 1000��ŭ ���� ���� ��ǥ�� ����
            bRising = true; // ��� ��� ���·� ��ȯ
            TimeOutOfRadius = 0.f; // Ÿ�̸� �ʱ�ȭ
        }
    }
    else
    {
        TimeOutOfRadius = 0.f; // �˵� �ȿ� �ִٸ� Ÿ�̸� �ʱ�ȭ
    }

    // ��� ��� ����
    if (bRising) // ��� ��� ������ ���
    {
        FVector MoveUpLocation = DroneLocation; // ���� ��ġ���� Z���� ����
        MoveUpLocation.Z = FMath::FInterpTo(DroneLocation.Z, TargetHeight, DeltaTime, 1.0f); // ��ǥ ���̱��� �ε巴�� ���
        GetPawn()->SetActorLocation(MoveUpLocation, true); // ��ġ ������Ʈ

        if (FMath::IsNearlyEqual(MoveUpLocation.Z, TargetHeight, 5.f)) // ��ǥ ���̿� ���� �����ߴٸ�
        {
            bRising = false; // ��� ���� ����
        }
    }

    // �˵� ���� ����
    float Direction = bClockwise ? 1.0f : -1.0f; // ȸ�� ���� ����
    CurrentAngle += OrbitSpeed * DeltaTime * Direction; // �ð��� ���� ���� ���� ������Ʈ
    if (CurrentAngle >= 360.f) CurrentAngle -= 360.f; // 360�� �Ѿ�� 0����
    if (CurrentAngle < 0.f) CurrentAngle += 360.f; // 0�� �̸��̸� 360����

    float Radian = FMath::DegreesToRadians(CurrentAngle); // ������ �������� ��ȯ
    // �� ���� ���: �÷��̾� ��ġ�� �߽����� (cos, sin)�� �̿��� �� ���� ��ǥ�� ����
    FVector Offset = FVector(FMath::Cos(Radian) * OrbitRadius, FMath::Sin(Radian) * OrbitRadius, HeightOffset);
    FVector TargetLocation = PlayerLocation + Offset; // ���� �������� ��ǥ ��ġ

    // ��ֹ� ���� ���� (���� Ʈ���̽�)
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetPawn()); // �ڱ� �ڽ��� ����
    FHitResult HitResult;
    // ���� ��ġ���� ��ǥ ��ġ���� ���� Ʈ���̽��� ���� ��ֹ��� �ִ��� Ȯ��
    if (GetWorld()->LineTraceSingleByChannel(HitResult, DroneLocation, TargetLocation, ECC_Visibility, QueryParams))
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor && HitActor != GetPawn())
        {
            if (HitActor->IsA(AEnemyDrone::StaticClass())) // ���� �ٸ� ��п� �ε����ٸ�
            {
                float RandomSign = (FMath::RandBool()) ? 1.f : -1.f; // �� �Ǵ� �Ʒ� ����
                HeightOffset = FMath::Clamp(HeightOffset + 150.f * RandomSign, 150.f, 1000.f); // ���� �����ϰ� �����Ͽ� ȸ��
            }
            else // ���̳� �ǹ� �� �ٸ� ��ֹ��� �ε����ٸ�
            {
                bClockwise = !bClockwise; // ȸ�� ������ �ݴ�� ��ȯ
                if (bTriedReverse) // �̹� �� �� ������ �ٲ�µ� �� �ε����ٸ� (���� ���·� ����)
                {
                    TimeStuck += DeltaTime; // ���� �ð� ����
                    if (TimeStuck >= MaxStuckTime) // ���� �ð� �̻� �����ִٸ�
                    {
                        HeightOffset = BaseHeight + 1000.f; // ���� ũ�� ������ Ż�� �õ�
                        TimeStuck = 0.f;
                        bTriedReverse = false;
                    }
                }
                else // ó�� �ε����ٸ�
                {
                    bTriedReverse = true; // ������ �ٲ�ٰ� ���
                    TimeStuck = 0.f; // ���� �ð� �ʱ�ȭ
                }
            }
            DrawDebugLine(GetWorld(), DroneLocation, HitResult.ImpactPoint, FColor::Green, false, 0.1f, 0, 1.f);
        }
    }
    else // ��ֹ��� ���ٸ�
    {
        // ������ �⺻ ���� ����
        HeightOffset = FMath::FInterpTo(HeightOffset, BaseHeight, DeltaTime, 0.5f);
        // ���� ���� ���� ������ �ʱ�ȭ
        bTriedReverse = false;
        TimeStuck = 0.f;
    }

    // ���� �̵� �� ȸ��
    FVector NewLocation = FMath::VInterpTo(DroneLocation, TargetLocation, DeltaTime, 2.0f); // ���� ��ǥ ��ġ�� �ε巴�� �̵�
    GetPawn()->SetActorLocation(NewLocation, true);

    // �׻� �÷��̾ �ٶ󺸵��� ȸ��
    FVector ToPlayer = PlayerLocation - NewLocation;
    FRotator LookAtRotation = FRotationMatrix::MakeFromX(ToPlayer).Rotator();
    GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAtRotation, DeltaTime, 5.0f));
}