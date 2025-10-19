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

    // ���� ���� �� �� ��, �׸��� AllyCacheUpdateInterval(2��)���� �ֱ������� UpdateAllyCaches �Լ��� ȣ���ϵ��� Ÿ�̸� ����
    GetWorld()->GetTimerManager().SetTimer(
        AllyCacheUpdateTimerHandle,
        this,
        &AEnemyGuardianAIController::UpdateAllyCaches,
        AllyCacheUpdateInterval,
        true, // �ݺ� ����
        0.1f  // �ʱ� ���� (��� ���Ͱ� ������ �ð��� ��)
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

// �ֱ������� ȣ��Ǿ� �Ʊ� ����� �����ϰ� ĳ�ÿ� �����ϴ� �Լ�
void AEnemyGuardianAIController::UpdateAllyCaches()
{
    // ���� ĳ�� �ʱ�ȭ
    CachedShooters.Empty();
    CachedGuardians.Empty();

    // ���忡 �ִ� ��� ���͸� ã�� ��ȿ�� ��� ĳ�ÿ� �߰�
    for (TActorIterator<AEnemyShooter> It(GetWorld()); It; ++It)
    {
        AEnemyShooter* Shooter = *It;
        if (Shooter && !Shooter->bIsDead && !Shooter->bIsPlayingIntro)
        {
            CachedShooters.Add(Shooter);
        }
    }

    // ���忡 �ִ� ��� ������� ã�� ��ȿ�� ��� ĳ�ÿ� �߰�
    for (TActorIterator<AEnemyGuardian> It(GetWorld()); It; ++It)
    {
        AEnemyGuardian* Guardian = *It;
        if (Guardian && !Guardian->bIsDead && !Guardian->bIsPlayingIntro && !Guardian->bIsStunned)
        {
            CachedGuardians.Add(Guardian);
        }
    }
}

// �Ʊ� ���͸� ��ȣ�ϴ� AI �ൿ ����
void AEnemyGuardianAIController::PerformShooterProtection()
{
    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn());
    if (!Guardian) return;

    // 1. ĳ�õ� ���� ����� ��������� ��ȣ �ൿ �ߴ�
    if (CachedShooters.Num() == 0)
    {
        FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
        FRotator LookRotation = ToPlayer.Rotation();
        Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
        return;
    }

    // 2. �ڽſ��� ���� ����� ���͸� ĳ�õ� ��Ͽ��� ã��
    AEnemyShooter* NearestShooter = nullptr;
    float MinDistance = FLT_MAX;
    for (const auto& WeakShooter : CachedShooters)
    {
        if (WeakShooter.IsValid()) // ���� ������ ��ȿ���� Ȯ��
        {
            AEnemyShooter* Shooter = WeakShooter.Get();
            float Distance = FVector::Dist(Guardian->GetActorLocation(), Shooter->GetActorLocation());
            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                NearestShooter = Shooter;
            }
        }
    }
    if (!NearestShooter) return; // ��ȿ�� ���͸� ã�� ���ߴٸ� �ߴ�

    // 3. �ڽŰ� '���� ����'�� ��ȣ�Ϸ��� �ٸ� ��� ������� ĳ�õ� ��Ͽ��� ã��
    TArray<AEnemyGuardian*> GuardiansProtectingSameShooter;
    for (const auto& WeakGuardian : CachedGuardians)
    {
        if (WeakGuardian.IsValid())
        {
            AEnemyGuardian* OtherGuardian = WeakGuardian.Get();
            // ���а� �ı��� ������� ��ȣ �ӹ����� ����
            if (OtherGuardian->bIsShieldDestroyed) continue;

            // �ٸ� ������� ���� ����� ���͵� '���� ��ǥ ����'�� ������ Ȯ��
            AEnemyShooter* OtherNearestShooter = nullptr;
            float OtherMinDistance = FLT_MAX;
            for (const auto& WeakShooter : CachedShooters)
            {
                if (WeakShooter.IsValid())
                {
                    AEnemyShooter* Shooter = WeakShooter.Get();
                    float Distance = FVector::Dist(OtherGuardian->GetActorLocation(), Shooter->GetActorLocation());
                    if (Distance < OtherMinDistance)
                    {
                        OtherMinDistance = Distance;
                        OtherNearestShooter = Shooter;
                    }
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

    // 5. ���� ���� �ִ� �������� �����Ͽ� ������ �ε��� �ο�
    GuardiansProtectingSameShooter.Sort([](const AEnemyGuardian& A, const AEnemyGuardian& B) {
        return A.GetUniqueID() < B.GetUniqueID();
        });
    int32 MyIndex = GuardiansProtectingSameShooter.IndexOfByPredicate([Guardian](const AEnemyGuardian* G) {
        return G == Guardian;
        });

    // 6. �ε����� ������� ���� ��ǥ ��ġ ���
    FVector FinalTargetLocation = BaseProtectionLocation;
    if (GuardiansProtectingSameShooter.Num() > 1 && MyIndex != INDEX_NONE)
    {
        float SpreadDistance = 80.0f; // ����� ������ ����
        int32 TotalGuardians = GuardiansProtectingSameShooter.Num();
        float OffsetFromCenter = (MyIndex - (TotalGuardians - 1) * 0.5f) * SpreadDistance;
        FinalTargetLocation += NearestShooter->GetActorRightVector() * OffsetFromCenter;
    }

    // 7. �׻� �÷��̾ �ٶ󺸵��� ȸ��
    FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
    FRotator LookRotation = ToPlayer.Rotation();
    Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));

    // 8. ���� ��ǥ ��ġ�� �̵�
    float DistanceToTarget = FVector::Dist(Guardian->GetActorLocation(), FinalTargetLocation);
    if (DistanceToTarget > MinDistanceToTarget)
    {
        FVector DirectionToTarget = (FinalTargetLocation - Guardian->GetActorLocation()).GetSafeNormal();
        Guardian->AddMovementInput(DirectionToTarget, 1.0f);
    }
}

// �÷��̾ �����ϴ� AI �ൿ ���� (���� �ı� ��)
void AEnemyGuardianAIController::PerformSurroundMovement()
{
    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn());
    if (!Guardian) return;

    // 1. Ȱ�� ������(�װų� ������ �ƴ�) ����� ĳ�ÿ��� ���͸�
    TArray<AEnemyGuardian*> ActiveGuardians;
    for (const auto& WeakGuardian : CachedGuardians)
    {
        if (WeakGuardian.IsValid())
        {
            // ���а� �ı��� ����� ���� �⵿�� ����
            if (WeakGuardian.Get()->bIsShieldDestroyed)
            {
                ActiveGuardians.Add(WeakGuardian.Get());
            }
        }
    }
    // �ڱ� �ڽ� �߰� (ĳ�ÿ��� ���� �ı� ���� �ڽ��� ���Ե��� ���� �� �����Ƿ�)
    if (!ActiveGuardians.Contains(Guardian))
    {
        ActiveGuardians.Add(Guardian);
    }


    // 2. Ȱ�� ������ ������� 1�� ���̸� �ܼ��ϰ� �÷��̾�� ����
    if (ActiveGuardians.Num() <= 1)
    {
        MoveToActor(PlayerPawn, 50.0f);
        return;
    }

    // 3. ��ü ����� ��Ͽ��� �ڽ��� ������ �ε����� ã��
    int32 MyIndex = ActiveGuardians.IndexOfByKey(Guardian);
    if (MyIndex == INDEX_NONE)
    {
        MoveToActor(PlayerPawn, 50.0f);
        return;
    }

    // 4. ���� ��ġ ���
    float AngleDeg = 360.0f / ActiveGuardians.Num();
    float MyAngleDeg = AngleDeg * MyIndex;
    float MyAngleRad = FMath::DegreesToRadians(MyAngleDeg);
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

void AEnemyGuardianAIController::StopAI() 
{
    // AI ���� ���� ����
    GetWorld()->GetTimerManager().ClearTimer(AllyCacheUpdateTimerHandle); // Ÿ�̸� ����
    StopMovement();
}