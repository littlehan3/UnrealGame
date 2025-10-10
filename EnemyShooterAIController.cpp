#include "EnemyShooterAIController.h"
#include "EnemyShooter.h" // ������ ���
#include "EnemyGuardian.h" // ��ȣ�ۿ��� �Ʊ� ���к�
#include "Kismet/GameplayStatics.h" // �����÷��� ��ƿ��Ƽ �Լ�
#include "GameFramework/CharacterMovementComponent.h" // ĳ���� �����Ʈ ������Ʈ
#include "Engine/World.h" // UWorld ����
#include "DrawDebugHelpers.h" // ����� �ð�ȭ ���
#include "EngineUtils.h" // TActorIterator ���
#include "EnemyShooterGrenade.h" // ��ô�� ����ź

AEnemyShooterAIController::AEnemyShooterAIController()
{
    PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ� ȣ�� Ȱ��ȭ
}

void AEnemyShooterAIController::BeginPlay()
{
    Super::BeginPlay(); // �θ� Ŭ���� BeginPlay ȣ��
    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // �÷��̾� �� ã�� ����
}

void AEnemyShooterAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // �θ� Ŭ���� Tick ȣ��
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn()); // ���� ���� �� ��������
    // ���� ���ų�, �׾��ų�, ���� ���� ���� �ƹ��� ������ �������� ����
    if (!Shooter || Shooter->bIsDead || Shooter->bIsPlayingIntro)
        return;

    if (!PlayerPawn) // �÷��̾� �� ������ ���ٸ� (�÷��̾ �׾��� ��Ȱ�ϴ� ��)
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // �ٽ� ã�ƺ�
        if (!PlayerPawn) return; // �׷��� ������ ���� �ߴ�
    }

    LookAtPlayerWithConstraints(DeltaTime); // �� ƽ���� �÷��̾ �ε巴�� �ٶ�

    float CurrentTime = GetWorld()->GetTimeSeconds(); // ���� ���� �ð�

    // --- ���� ����ȭ: Ÿ�̸Ӹ� �̿��� ��� ������ ������ ���� ---
    // 1�ʸ���: �ֺ� ���� �˻�
    if (CurrentTime - LastAlliesSearch >= AlliesSearchInterval)
    {
        UpdateCachedAllies(); // ���� ��� ĳ�� ����
        LastAlliesSearch = CurrentTime; // ������ �˻� �ð� ���
    }

    // 0.3�ʸ���: �缱 Ȯ��(LOS) �˻�
    if (CurrentTime - LastClearShotCheck >= ClearShotCheckInterval)
    {
        UpdateCachedClearShot(); // �缱 Ȯ�� ���� ĳ�� ����
        LastClearShotCheck = CurrentTime; // ������ �˻� �ð� ���
    }

    // 1�ʸ���: �ڽ��� �����̼� ��ġ ����
    if (CurrentTime - LastPositionUpdate >= PositionUpdateInterval)
    {
        UpdateFormationPosition();
        LastPositionUpdate = CurrentTime;
    }

    // ���� �ӽ� ������Ʈ: ���� ���¿� ���� �ൿ ����
    UpdateAIState();
}

// �ֺ� �Ʊ� ����� �˻��Ͽ� ĳ�ÿ� �����ϴ� �Լ� (1�ʿ� �� �� ȣ��)
void AEnemyShooterAIController::UpdateCachedAllies()
{
    CachedAllies.Empty(); // ���� ĳ�� �ʱ�ȭ
    if (!GetPawn()) return;

    // ���忡 �ִ� ��� AEnemyShooter ���͸� ��ȸ
    for (TActorIterator<AEnemyShooter> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
    {
        AEnemyShooter* OtherShooter = *ActorIterator;
        if (OtherShooter == GetPawn()) continue; // �ڱ� �ڽ��� ����
        if (OtherShooter->bIsDead) continue; // ���� ����� ����

        CachedAllies.Add(TWeakObjectPtr<AEnemyShooter>(OtherShooter)); // ���� ����(Weak Ptr)�� ĳ�ÿ� �߰�
    }
}

// �÷��̾������ �缱�� Ȯ���Ǿ����� Ȯ���Ͽ� ĳ�ÿ� �����ϴ� �Լ� (0.3�ʿ� �� �� ȣ��)
void AEnemyShooterAIController::UpdateCachedClearShot()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter || !PlayerPawn)
    {
        bCachedHasClearShot = false; // �˻� �Ұ� �� false�� ����
        return;
    }

    FVector StartLocation = Shooter->GetActorLocation(); // ���� Ʈ���̽� ������ (�ڽ� ��ġ)
    FVector EndLocation = PlayerPawn->GetActorLocation(); // ���� Ʈ���̽� ���� (�÷��̾� ��ġ)

    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(Shooter); // �ڱ� �ڽ��� ����
    CollisionParams.AddIgnoredActor(PlayerPawn); // �÷��̾ ���� (�߰��� �ٸ� ���Ͱ� �ִ��� Ȯ���ϱ� ����)
    CollisionParams.bTraceComplex = false; // ������ �浹 �˻� ��Ȱ��ȭ (����)

    // Pawn ä�ο� ���� ���� Ʈ���̽� ����
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult, StartLocation, EndLocation, ECollisionChannel::ECC_Pawn, CollisionParams
    );

    bIsBlockedByGuardian = false; // ����� ���� ���� �ʱ�ȭ

    if (bHit) // ���𰡿� �¾Ҵٸ�
    {
        if (Cast<AEnemyShooter>(HitResult.GetActor())) // �ٸ� ���Ϳ� ���� �����ٸ�
        {
            bCachedHasClearShot = false; // �缱 ��Ȯ��
            return;
        }

        if (AEnemyGuardian* HitGuardian = Cast<AEnemyGuardian>(HitResult.GetActor())) // ����� ���� �����ٸ�
        {
            if (!HitGuardian->bIsDead) // ����ִ� ������̶��
            {
                bCachedHasClearShot = false; // �缱 ��Ȯ��
                bIsBlockedByGuardian = true; // ����� ���� �����ٰ� ��� (����ź ��ô ����)
                UE_LOG(LogTemp, Warning, TEXT("shooter stopped shooting because guardian is blocking forward. now start grenade attack"));
                return;
            }
        }
    }

    // �ƹ��Ϳ��� ������ �ʾҴٸ� �缱 Ȯ��
    bCachedHasClearShot = true;
    bIsBlockedByGuardian = false;
}

// ���ѵ� ���� ������ �÷��̾ �ε巴�� �ٶ󺸴� �Լ�
void AEnemyShooterAIController::LookAtPlayerWithConstraints(float DeltaTime)
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter || !PlayerPawn) return;

    // ��ǥ ȸ���� ��� (���� ���� ���� ����)
    FRotator TargetRotation = CalculateConstrainedLookRotation(PlayerPawn->GetActorLocation());
    FRotator CurrentRotation = Shooter->GetActorRotation(); // ���� ȸ����

    // ���� ȸ�������� ��ǥ ȸ�������� �ε巴�� ����
    FRotator NewRotation = FMath::RInterpTo(
        CurrentRotation, TargetRotation, DeltaTime, RotationInterpSpeed
    );
    Shooter->SetActorRotation(NewRotation); // ���� ȸ���� ����
}

// ���� ���� ������ ������ ���� ȸ������ ����ϴ� �Լ�
FRotator AEnemyShooterAIController::CalculateConstrainedLookRotation(FVector TargetLocation) const
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter) return FRotator::ZeroRotator;

    FVector ShooterLocation = Shooter->GetActorLocation();
    FVector ToTarget = TargetLocation - ShooterLocation; // �ڽſ��� Ÿ���� ���ϴ� ���� ����

    FRotator LookRotation = ToTarget.Rotation(); // �⺻ ȸ���� ���

    // Pitch(���� ȸ��) ���� ������ �ִ�/�ּҰ� ���̷� ����
    float ClampedPitch = FMath::Clamp(LookRotation.Pitch, MaxLookDownAngle, MaxLookUpAngle);

    // Yaw(�¿� ȸ��)�� �״��, Roll(����)�� 0���� ������ ���� ȸ���� ��ȯ
    FRotator ConstrainedRotation = FRotator(ClampedPitch, LookRotation.Yaw, 0.0f);

    return ConstrainedRotation;
}

// AI ���� �ӽ� ������Ʈ �Լ�
void AEnemyShooterAIController::UpdateAIState()
{
    // ���� ���¿� ���� ������ �ڵ鷯 �Լ� ȣ��
    switch (CurrentState)
    {
    case EEnemyShooterAIState::Idle:         HandleIdleState();       break;
    case EEnemyShooterAIState::Detecting:    HandleDetectingState();  break;
    case EEnemyShooterAIState::Moving:       HandleMovingState();     break;
    case EEnemyShooterAIState::Shooting:     HandleShootingState();   break;
    case EEnemyShooterAIState::Retreating:   HandleRetreatingState(); break;
    }
}

// ��� ����: �÷��̾ Ž�� ������ ������ Detecting ���·� ��ȯ
void AEnemyShooterAIController::HandleIdleState()
{
    if (IsPlayerInDetectionRange())
    {
        CurrentState = EEnemyShooterAIState::Detecting;
    }
}

// Ž�� ����: �÷��̾���� �Ÿ��� ������� ���� �ൿ(����, ���, �̵�)�� ����
void AEnemyShooterAIController::HandleDetectingState()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter) return;

    if (IsPlayerTooClose()) // �ʹ� ������
    {
        CurrentState = EEnemyShooterAIState::Retreating; // ����
    }
    else if (IsPlayerInShootRange() && HasClearShotToPlayer()) // ��� ���� �Ÿ��̰� �缱�� Ȯ���Ǹ�
    {
        CurrentState = EEnemyShooterAIState::Shooting; // ���
        StopMovementInput();
    }
    else if (IsPlayerInDetectionRange()) // Ž�� �Ÿ� �ȿ� �ִٸ�
    {
        CurrentState = EEnemyShooterAIState::Moving; // �̵�
    }
    else // Ž�� �Ÿ��� ����ٸ�
    {
        CurrentState = EEnemyShooterAIState::Idle; // �ٽ� ��� ���·�
        StopMovementInput();
    }
}

// �̵� ����: ��� ��ġ�� ���� ��ġ�� �̵�. �̵� �߿��� ��� ��� ���� ���θ� Ȯ��.
void AEnemyShooterAIController::HandleMovingState()
{
    // �ֿ켱 ����: �̵� ���̶� ����� ���������� ��� ��� ���·� ��ȯ
    if (IsPlayerInShootRange() && HasClearShotToPlayer())
    {
        CurrentState = EEnemyShooterAIState::Shooting;
        StopMovementInput();
        return;
    }

    // ������: �ٸ� ���� ��ȯ ���� Ȯ��
    if (IsPlayerTooClose())
    {
        CurrentState = EEnemyShooterAIState::Retreating;
        return;
    }
    if (!IsPlayerInDetectionRange())
    {
        CurrentState = EEnemyShooterAIState::Idle;
        StopMovementInput();
        return;
    }
    if (bIsBlockedByGuardian) // ����� �����ٸ� ���ʿ��� �̵� ���� (����ź �� ���)
    {
        StopMovementInput();
        return;
    }

    // ���ļ���: ���� ��� ���ǿ� �ش����� ���� ���� �̵� ���� ����
    if (ShouldMoveToFormation()) // ���� ��ġ���� ����ٸ�
    {
        MoveTowardsTarget(AssignedPosition); // ���� ��ġ�� �̵�
    }
    else // ���� ��ġ�� �ִٸ�
    {
        MoveTowardsPlayer(); // �÷��̾ ���� ����
    }
}

// ��� ����: ����� �����ϰ�, ��Ȳ ��ȭ�� ���� �ٸ� ���·� ��ȯ
void AEnemyShooterAIController::HandleShootingState()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter || !PlayerPawn) return;

    // �缱�� Ȯ������ �ʾ��� ��� (�ٽ� ���� �Ǵ�)
    if (!HasClearShotToPlayer())
    {
        // ���� '�����'�� ���� �����ٸ�, ����ź ��ô �õ�
        if (bIsBlockedByGuardian)
        {
            if (GetWorld()->GetTimeSeconds() - LastGrenadeThrowTime < GrenadeCooldown) // ��Ÿ�� Ȯ��
            {
                StopMovementInput(); // ��Ÿ�� �߿��� ���
                return;
            }

            TSubclassOf<AEnemyShooterGrenade> GrenadeToSpawn = Shooter->GrenadeClass; // ���� ����ź Ŭ���� ��������
            if (!GrenadeToSpawn) return;

            FVector StartLocation = Shooter->GetActorLocation() + FVector(0, 0, 100.0f); // �ణ ������ �߻�

            // ����ź ��ǥ ���� ���: �÷��̾� ��ġ���� �ణ '��'�� ���� (�÷��̾ ���� ���� ����)
            FVector PlayerLocation = PlayerPawn->GetActorLocation();
            FVector DirectionToPlayer = PlayerLocation - StartLocation;
            DirectionToPlayer.Z = 0; // ���� ���⸸ ���
            FVector EndLocation = PlayerLocation - (DirectionToPlayer.GetSafeNormal() * GrenadeTargetOffset);
            EndLocation.Z += 60.0f; // �ణ�� ���� ����

            // ���� ������ �˵��� ���� �߻� �ӵ� ���� �ޱ�
            FVector LaunchVelocity;
            bool bHaveAimSolution = UGameplayStatics::SuggestProjectileVelocity(
                this, LaunchVelocity, StartLocation, EndLocation, GrenadeLaunchSpeed, false, 0.0f, GetWorld()->GetGravityZ()
            );

            if (bHaveAimSolution) // �߻� ������ ���Դٸ�
            {
                FActorSpawnParameters SpawnParams;
                SpawnParams.Owner = GetPawn();
                SpawnParams.Instigator = GetPawn();

                // ����ź ���� �� �߻�
                AEnemyShooterGrenade* Grenade = GetWorld()->SpawnActor<AEnemyShooterGrenade>(
                    GrenadeToSpawn, StartLocation, FRotator::ZeroRotator, SpawnParams
                );
                if (Grenade)
                {
                    Grenade->LaunchGrenade(LaunchVelocity);
                    LastGrenadeThrowTime = GetWorld()->GetTimeSeconds(); // ��Ÿ�� Ÿ�̸� ����
                    Shooter->PlayThrowingGrenadeAnimation(); // ĳ������ ��ô �ִϸ��̼� ���
                }
            }
            StopMovementInput();
            return;
        }

        // ������� �ƴ� �ٸ� ������ �缱�� �����ٸ� �ٽ� �̵� ���·� ��ȯ
        CurrentState = EEnemyShooterAIState::Moving;
        return;
    }

    // �缱�� Ȯ���� ���: �Ÿ� ��Ȯ��
    if (IsPlayerTooClose())
    {
        CurrentState = EEnemyShooterAIState::Retreating;
        return;
    }
    else if (!IsPlayerInShootRange())
    {
        CurrentState = IsPlayerInDetectionRange() ? EEnemyShooterAIState::Moving : EEnemyShooterAIState::Idle;
        StopMovementInput();
        return;
    }

    // ��� ������ �����ϸ� ���
    if (CanShoot() && !Shooter->bIsAttacking && !Shooter->bIsDead)
    {
        PerformShooting();
    }
}

// ���� ����: �÷��̾�κ��� �־����鼭 �Ÿ��� ����
void AEnemyShooterAIController::HandleRetreatingState()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter || !PlayerPawn) return;

    // �÷��̾� �ݴ� �������� �̵�
    FVector ToPlayer = PlayerPawn->GetActorLocation() - Shooter->GetActorLocation();
    FVector RetreatDir = -ToPlayer.GetSafeNormal(); // ���� ���͸� �ݴ��
    Shooter->AddMovementInput(RetreatDir, 1.0f);

    float Distance = GetDistanceToPlayer();
    // ����� �Ÿ��� ���Ȱ� �缱�� Ȯ���Ǹ� �ٽ� ��� ���·�
    if (Distance >= (MinShootDistance + RetreatBuffer) && Distance <= ShootRadius && HasClearShotToPlayer())
    {
        CurrentState = EEnemyShooterAIState::Shooting;
        StopMovementInput();
    }
    else if (Distance > ShootRadius && IsPlayerInDetectionRange()) // �ʹ� �־������� �ٽ� �̵� ���·�
    {
        CurrentState = EEnemyShooterAIState::Moving;
    }
    else if (!IsPlayerInDetectionRange()) // Ž�� ������ ����� ��� ���·�
    {
        CurrentState = EEnemyShooterAIState::Idle;
        StopMovementInput();
    }
}

// �ڽ��� ���� ��ġ�� ����ϰ� ����
void AEnemyShooterAIController::UpdateFormationPosition()
{
    AssignedPosition = CalculateFormationPosition();
    bHasAssignedPosition = true;
}

// �÷��̾�� �Ʊ� ��ġ�� ������� �ڽ��� �̻����� ���� ��ġ�� ���
FVector AEnemyShooterAIController::CalculateFormationPosition()
{
    if (!PlayerPawn || !GetPawn()) return GetPawn()->GetActorLocation();

    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    TArray<AActor*> AllAllies = GetAllAllies(); // ĳ�õ� �Ʊ� ��� ��������

    // �ڽ��� ������ ��ü ���� �� ���
    int32 TotalShooters = AllAllies.Num() + 1;
    // ������ �ε��� �ο� (�޸� �ּ� �񱳷� ���� ����)
    int32 MyIndex = 0;
    for (int32 i = 0; i < AllAllies.Num(); i++)
    {
        if (GetPawn() > AllAllies[i])
        {
            MyIndex++;
        }
    }

    // ���� ���� ���
    float AngleStep = 360.0f / TotalShooters; // �� ���Ͱ� ������ ����
    float MyAngle = MyIndex * AngleStep; // �ڽ��� ��ǥ ����

    // �÷��̾� ��ġ�� �߽����� �� ���� ��ǥ ���
    FVector ToFormationPos = FVector(
        FMath::Cos(FMath::DegreesToRadians(MyAngle)) * FormationRadius,
        FMath::Sin(FMath::DegreesToRadians(MyAngle)) * FormationRadius,
        0.0f
    );
    FVector IdealPosition = PlayerLocation + ToFormationPos;

    // ���� ���� ��ġ�� �̹� �ٸ� �Ʊ��� �ִٸ�, �ڸ��� ã�� ������ 45���� ȸ���ϸ� ��Ž��
    int32 Attempts = 0;
    while (IsPositionOccupied(IdealPosition) && Attempts < 8)
    {
        MyAngle += 45.0f;
        ToFormationPos = FVector(
            FMath::Cos(FMath::DegreesToRadians(MyAngle)) * FormationRadius,
            FMath::Sin(FMath::DegreesToRadians(MyAngle)) * FormationRadius,
            0.0f
        );
        IdealPosition = PlayerLocation + ToFormationPos;
        Attempts++;
    }

    return IdealPosition;
}

// Ư�� ��ġ�� �ٸ� �Ʊ��� ���� �����Ǿ����� Ȯ��
bool AEnemyShooterAIController::IsPositionOccupied(FVector Position, float CheckRadius)
{
    TArray<AActor*> AllAllies = GetAllAllies();
    for (AActor* Ally : AllAllies)
    {
        if (FVector::Dist(Position, Ally->GetActorLocation()) < CheckRadius)
        {
            return true; // ������
        }
    }
    return false; // �������
}

// �Ҵ�� ���� ��ġ�� �̵��ؾ� �ϴ��� �Ǵ�
bool AEnemyShooterAIController::ShouldMoveToFormation() const
{
    if (!bHasAssignedPosition) return false;
    // ���� ��ġ�� ��ǥ ��ġ���� 200 ���� �̻� ������ �ִٸ� �̵� �ʿ�
    return FVector::Dist(GetPawn()->GetActorLocation(), AssignedPosition) > 200.0f;
}

// ĳ�õ� �缱 Ȯ�� ���� ��ȯ
bool AEnemyShooterAIController::HasClearShotToPlayer() const
{
    return bCachedHasClearShot;
}

// ĳ�õ� �Ʊ� ����� ��ȿ�� AActor �迭�� ��ȯ�Ͽ� ��ȯ
TArray<AActor*> AEnemyShooterAIController::GetAllAllies() const
{
    TArray<AActor*> ValidAllies;
    for (const auto& WeakAlly : CachedAllies)
    {
        if (WeakAlly.IsValid()) // ���� ������ ��ȿ����(���Ͱ� �ı����� �ʾҴ���) Ȯ��
        {
            ValidAllies.Add(WeakAlly.Get());
        }
    }
    return ValidAllies;
}

// ��ǥ �������� �̵� (�̵��� ���, ȸ���� Tick���� ó��)
void AEnemyShooterAIController::MoveTowardsTarget(FVector Target)
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter) return;

    FVector ToTarget = Target - Shooter->GetActorLocation();
    FVector MoveDirection = ToTarget.GetSafeNormal();
    Shooter->AddMovementInput(MoveDirection, 1.0f);
}

// ��ƿ��Ƽ �Լ���
float AEnemyShooterAIController::GetDistanceToPlayer() const
{
    if (!PlayerPawn || !GetPawn()) return FLT_MAX;
    return FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
}

bool AEnemyShooterAIController::IsPlayerInDetectionRange() const
{
    return GetDistanceToPlayer() <= DetectionRadius;
}

bool AEnemyShooterAIController::IsPlayerInShootRange() const
{
    float Distance = GetDistanceToPlayer();
    return (Distance >= MinShootDistance && Distance <= ShootRadius);
}

bool AEnemyShooterAIController::IsPlayerTooClose() const
{
    return GetDistanceToPlayer() < MinShootDistance;
}

void AEnemyShooterAIController::MoveTowardsPlayer()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter || !PlayerPawn) return;

    FVector ToPlayer = PlayerPawn->GetActorLocation() - Shooter->GetActorLocation();
    FVector MoveDirection = ToPlayer.GetSafeNormal();
    Shooter->AddMovementInput(MoveDirection, 1.0f);
}

//void AEnemyShooterAIController::RetreatFromPlayer()
//{
//    // ���� �̵� ������ HandleRetreatingState���� ���� ó����
//}

void AEnemyShooterAIController::StopMovementInput()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (Shooter)
    {
        Shooter->GetCharacterMovement()->StopMovementImmediately();
    }
}

void AEnemyShooterAIController::PerformShooting()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter) return;

    // Shooter->PlayShootingAnimation(); // �ִϸ��̼� ��� ��� ��� ���� �� �߻�

    Shooter->Shoot(); // ĳ������ Shoot �Լ� ȣ��

    LastShootTime = GetWorld()->GetTimeSeconds(); // ������ ��� �ð� ���
}

bool AEnemyShooterAIController::CanShoot() const
{
    return GetWorld()->GetTimeSeconds() - LastShootTime >= ShootCooldown; // ��Ÿ���� �������� Ȯ��
}