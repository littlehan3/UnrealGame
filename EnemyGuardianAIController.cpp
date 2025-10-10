#include "EnemyGuardianAIController.h"
#include "EnemyGuardian.h" // ������ ���
#include "EnemyShooter.h" // ��ȣ�� ���
#include "Kismet/GameplayStatics.h" // �����÷��� ��ƿ��Ƽ �Լ�
#include "TimerManager.h" // FTimerManager ���
#include "GameFramework/Character.h" // ACharacter Ŭ���� ����
#include "GameFramework/CharacterMovementComponent.h" // ĳ���� �����Ʈ ������Ʈ

AEnemyGuardianAIController::AEnemyGuardianAIController()
{
    PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ� ȣ�� Ȱ��ȭ
}

void AEnemyGuardianAIController::BeginPlay()
{
    Super::BeginPlay(); // �θ� Ŭ���� BeginPlay ȣ��
    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // �÷��̾� �� ã�� ����

    // 4���� �̵� ������ ���� Ÿ�̸� ���� (����� ������ ����)
    GetWorld()->GetTimerManager().SetTimer(
        MoveTimerHandle, this, &AEnemyGuardianAIController::MoveInDirection, MoveDuration, true
    );
}

void AEnemyGuardianAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // �θ� Ŭ���� Tick ȣ��

    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn()); // ���� ���� �� ��������
    // ���� ���ų�, �׾��ų�, ���� ���̰ų�, ���� ������ ���� �ƹ��� ������ �������� ����
    if (!Guardian || Guardian->bIsDead || Guardian->bIsPlayingIntro || Guardian->bIsStunned)
        return;

    if (!PlayerPawn) // �÷��̾� �� ������ ���ٸ�
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // �ٽ� ã�ƺ�
        if (!PlayerPawn) return; // �׷��� ������ ���� �ߴ�
    }

    // ���� ���� ���� �ٸ� ��� �ൿ�� �ߴ��ϰ� �̵��� ����
    if (Guardian->bIsShieldAttacking || Guardian->bIsBatonAttacking)
    {
        StopMovement();
        return;
    }

    // ���а� �ı����� �ʾ��� ��: �÷��̾ ���� ������ ������ ���� ���� �õ�
    if (!Guardian->bIsShieldDestroyed && Guardian->EquippedShield)
    {
        float DistanceToPlayer = FVector::Dist(Guardian->GetActorLocation(), PlayerPawn->GetActorLocation());
        if (DistanceToPlayer <= Guardian->ShieldAttackRadius)
        {
            if (!Guardian->bIsShieldAttacking) // ���� ���� ���� �ƴ϶��
            {
                StopMovement(); // �̵��� ���߰�
                // �÷��̾ �ٶ� ��
                FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
                FRotator LookRotation = ToPlayer.Rotation();
                Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
                // ���� ���� �ִϸ��̼��� ���
                Guardian->PlayShieldAttackAnimation();
            }
            return; // ���� ���� �������� �ٸ� �̵� ������ �������� ����
        }
    }

    // ���а� �ı��� ��: �÷��̾ ���� ������ ������ ���к� ���� �õ�
    if (Guardian->bIsShieldDestroyed && Guardian->EquippedBaton)
    {
        float DistanceToPlayer = FVector::Dist(Guardian->GetActorLocation(), PlayerPawn->GetActorLocation());
        if (DistanceToPlayer <= Guardian->BatonAttackRadius)
        {
            if (!Guardian->bIsBatonAttacking) // ���� ���� ���� �ƴ϶��
            {
                StopMovement(); // �̵��� ���߰�
                // �÷��̾ �ٶ� ��
                FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
                FRotator LookRotation = ToPlayer.Rotation();
                Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
                // ���к� ���� �ִϸ��̼��� ���
                Guardian->PlayNormalAttackAnimation();
            }
            return; // ���� ���� �������� �ٸ� �̵� ������ �������� ����
        }
    }

    // ���� ���� �ƴ� ��, ������� ���¿� ���� �̵� ���� ����
    if (!Guardian->bIsShieldDestroyed && Guardian->EquippedShield)
    {
        PerformShooterProtection(); // ���а� ������ ���� ��ȣ
    }
    else
    {
        PerformSurroundMovement(); // ���а� ������ �÷��̾� ����
    }
}

// �Ʊ� ���͸� ��ȣ�ϴ� AI �ൿ ����
void AEnemyGuardianAIController::PerformShooterProtection()
{
    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn());
    if (!Guardian) return;

    // 1. ���忡 �ִ� ��� ����ִ� ���͸� ����
    TArray<AEnemyShooter*> AllShooters;
    for (TActorIterator<AEnemyShooter> It(GetWorld()); It; ++It)
    {
        AEnemyShooter* Shooter = *It;
        if (Shooter && !Shooter->bIsDead && !Shooter->bIsPlayingIntro)
        {
            AllShooters.Add(Shooter);
        }
    }

    if (AllShooters.Num() == 0) // ��ȣ�� ���Ͱ� ������
    {
        // �÷��̾ �ٶ󺸰� ���ڸ��� �� �ֵ��� ��
        FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
        FRotator LookRotation = ToPlayer.Rotation();
        Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
        return;
    }

    // 2. �ڽſ��� ���� ����� ���͸� ã��
    AEnemyShooter* NearestShooter = nullptr;
    float MinDistance = FLT_MAX;
    for (AEnemyShooter* Shooter : AllShooters)
    {
        float Distance = FVector::Dist(Guardian->GetActorLocation(), Shooter->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            NearestShooter = Shooter;
        }
    }
    if (!NearestShooter) return;

    // 3. �ڽŰ� '���� ����'�� ��ȣ�Ϸ��� �ٸ� ��� ������� ã��
    TArray<AEnemyGuardian*> GuardiansProtectingSameShooter;
    for (TActorIterator<AEnemyGuardian> It(GetWorld()); It; ++It)
    {
        AEnemyGuardian* OtherGuardian = *It;
        if (OtherGuardian && !OtherGuardian->bIsDead && !OtherGuardian->bIsPlayingIntro &&
            !OtherGuardian->bIsStunned && !OtherGuardian->bIsShieldDestroyed)
        {
            // �ٸ� ������� ���� ����� ���͵� '���� ��ǥ ����'�� ������ Ȯ��
            AEnemyShooter* OtherNearestShooter = nullptr;
            float OtherMinDistance = FLT_MAX;
            for (AEnemyShooter* Shooter : AllShooters)
            {
                float Distance = FVector::Dist(OtherGuardian->GetActorLocation(), Shooter->GetActorLocation());
                if (Distance < OtherMinDistance)
                {
                    OtherMinDistance = Distance;
                    OtherNearestShooter = Shooter;
                }
            }

            if (OtherNearestShooter == NearestShooter) // ��ǥ�� ���ٸ�
            {
                GuardiansProtectingSameShooter.Add(OtherGuardian); // ���� ���� ��
            }
        }
    }

    // 4. �⺻ ��ȣ ��ġ ��� (���� ����� ������ ����)
    FVector ShooterForward = NearestShooter->GetActorForwardVector();
    FVector BaseProtectionLocation = NearestShooter->GetActorLocation() + ShooterForward * ProtectionDistance;

    // 5. ���� ���� �ִ� �������� �����Ͽ� ������ �ε��� �ο� (��ħ ����)
    GuardiansProtectingSameShooter.Sort([](const AEnemyGuardian& A, const AEnemyGuardian& B) {
        return A.GetUniqueID() < B.GetUniqueID();
        });
    int32 MyIndex = GuardiansProtectingSameShooter.IndexOfByPredicate([Guardian](const AEnemyGuardian* G) {
        return G == Guardian;
        });

    // 6. �ε����� ������� �⺻ ��ġ���� ��/��� �л�� ���� ��ǥ ��ġ ���
    FVector FinalTargetLocation = BaseProtectionLocation;
    if (GuardiansProtectingSameShooter.Num() > 1 && MyIndex != INDEX_NONE)
    {
        float SpreadDistance = 80.0f; // ����� ������ ����
        int32 TotalGuardians = GuardiansProtectingSameShooter.Num();
        // (�ڽ��� �ε���) - (��ü �ο��� �߰���) �� ���� �߾��� �������� �� �ڽ��� ��� ��ġ�� ����
        float OffsetFromCenter = (MyIndex - (TotalGuardians - 1) * 0.5f) * SpreadDistance;
        FinalTargetLocation += NearestShooter->GetActorRightVector() * OffsetFromCenter;
    }

    // 7. �׻� �÷��̾ �ٶ󺸵��� ȸ��
    FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
    FRotator LookRotation = ToPlayer.Rotation();
    Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));

    // 8. ���� ��ǥ ��ġ�� �̵�
    float DistanceToTarget = FVector::Dist(Guardian->GetActorLocation(), FinalTargetLocation);
    if (DistanceToTarget > MinDistanceToTarget) // ���� ��ǥ ��ġ�� �������� ���ߴٸ�
    {
        FVector DirectionToTarget = (FinalTargetLocation - Guardian->GetActorLocation()).GetSafeNormal();
        Guardian->AddMovementInput(DirectionToTarget, 1.0f); // ��ǥ�� ���� �̵�
    }
}

// �÷��̾ �����ϴ� AI �ൿ ���� (���� �ı� ��)
void AEnemyGuardianAIController::PerformSurroundMovement()
{
    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn());
    if (!Guardian) return;

    // 1. ���忡 �ִ� ��� ����ִ� ����� ����
    TArray<AActor*> AllGuardians;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyGuardian::StaticClass(), AllGuardians);

    // �� �� ������ Ȱ�� ������(�װų� ������ �ƴ�) ����� ���͸�
    TArray<AEnemyGuardian*> ActiveGuardians;
    for (AActor* Actor : AllGuardians)
    {
        AEnemyGuardian* OtherGuardian = Cast<AEnemyGuardian>(Actor);
        if (OtherGuardian && !OtherGuardian->bIsDead && !OtherGuardian->bIsPlayingIntro && !OtherGuardian->bIsStunned)
        {
            ActiveGuardians.Add(OtherGuardian);
        }
    }

    // Ȱ�� ������ ������� 1�� ���̸� �ܼ��ϰ� �÷��̾�� ����
    if (ActiveGuardians.Num() <= 1)
    {
        FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
        FRotator LookRotation = ToPlayer.Rotation();
        Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
        MoveToActor(PlayerPawn, 50.0f);
        return;
    }

    // 2. ��ü ����� ��Ͽ��� �ڽ��� ������ �ε����� ã��
    int32 MyIndex = ActiveGuardians.IndexOfByKey(Guardian);
    if (MyIndex == INDEX_NONE)
    {
        MoveToActor(PlayerPawn, 50.0f); // ���� ��ã���� �׳� �÷��̾�� ����
        return;
    }

    // 3. ���� ��ġ ���
    float AngleDeg = 360.0f / ActiveGuardians.Num(); // 360���� ����� ���� ������ ���� ���� ���
    float MyAngleDeg = AngleDeg * MyIndex; // �ڽ��� �ε����� ���� ��ǥ ����
    float MyAngleRad = FMath::DegreesToRadians(MyAngleDeg); // �������� ��ȯ

    // 4. �÷��̾ �߽����� �� ���� ���� ��ġ ���
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    FVector Offset = FVector(FMath::Cos(MyAngleRad), FMath::Sin(MyAngleRad), 0) * SurroundRadius;
    FVector TargetLocation = PlayerLocation + Offset;

    // 5. �׻� �÷��̾ �ٶ󺸵��� ȸ��
    FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
    FRotator LookRotation = ToPlayer.Rotation();
    Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));

    // 6. ���� ���� ��ġ�� �̵�
    MoveToLocation(TargetLocation, 50.0f);
}

void AEnemyGuardianAIController::MoveInDirection()
{
    // ���� ������ �ʴ� 4���� �̵� ���� ����
    DirectionIndex = (DirectionIndex + 1) % 4;
}

void AEnemyGuardianAIController::StopAI() {}