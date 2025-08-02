#include "MainGameModeBase.h"
#include "Enemy.h"
#include "BossEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Async/Async.h"

AMainGameModeBase::AMainGameModeBase()
{
    PrimaryActorTick.bCanEverTick = false;

    // �⺻�� ����
    SpawnInterval = 10.0f;
    bSpawnEnabled = true;
    bEnableWaveSystem = true;
    PreCalculatedLocationCount = 200;

    // ���̺� ���� �ý��� �ʱ�ȭ
    CurrentWaveLevel = 1;
    SpawnStepCount = 0;
    bIsBossAlive = false;
    bWaitingForBossTransition = false;
    CurrentLocationIndex = 0;
    bLocationCalculationComplete = false;
}

void AMainGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    if (bEnableWaveSystem)
    {
        GetWorldTimerManager().SetTimerForNextTick([this]()
            {
                // ���� ��ġ ���� ���
                PreCalculateSpawnLocations();

                // 5�и��� ���� ��ġ ����
                GetWorldTimerManager().SetTimer(
                    LocationRefreshTimer,
                    this,
                    &AMainGameModeBase::RefreshSpawnLocations,
                    300.0f,
                    true
                );

                StartWaveSystem();
            });
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Wave System is DISABLED - No enemies will spawn"));
    }
}

void AMainGameModeBase::PreCalculateSpawnLocations()
{
    UE_LOG(LogTemp, Warning, TEXT("Pre-calculating %d spawn locations..."), PreCalculatedLocationCount);

    PreCalculatedSpawnLocations.Empty();
    PreCalculatedSpawnLocations.Reserve(PreCalculatedLocationCount);

    FVector SpawnCenter = bSpawnAroundPlayer ? GetPlayerLocation() : SpawnCenterLocation;

    // ���� ���������� �پ��� ��ġ ���
    TArray<float> RadiusValues = { 500.0f, 750.0f, 1000.0f, 1250.0f, 1500.0f };
    int32 LocationsPerRadius = PreCalculatedLocationCount / RadiusValues.Num();

    for (float Radius : RadiusValues)
    {
        for (int32 i = 0; i < LocationsPerRadius; i++)
        {
            FVector SpawnLocation;
            if (FindRandomLocationOnNavMesh(SpawnCenter, Radius, SpawnLocation))
            {
                PreCalculatedSpawnLocations.Add(SpawnLocation);
            }
        }
    }

    // ������ ��ġ�� �߰� ���
    while (PreCalculatedSpawnLocations.Num() < PreCalculatedLocationCount)
    {
        FVector SpawnLocation;
        if (FindRandomLocationOnNavMesh(SpawnCenter, DefaultSpawnRadius, SpawnLocation))
        {
            PreCalculatedSpawnLocations.Add(SpawnLocation);
        }
        else
        {
            break; // NavMesh���� �� �̻� ��ġ�� ã�� �� ����
        }
    }

    bLocationCalculationComplete = true;
    UE_LOG(LogTemp, Warning, TEXT("Pre-calculated %d spawn locations completed"), PreCalculatedSpawnLocations.Num());
}

void AMainGameModeBase::RefreshSpawnLocations()
{
    UE_LOG(LogTemp, Warning, TEXT("Refreshing spawn locations..."));

    // �񵿱�� �� ��ġ�� ���
    RefillSpawnLocationsAsync();
}

void AMainGameModeBase::RefillSpawnLocationsAsync()
{
    if (!bLocationCalculationComplete) return;

    // ��׶��忡�� �� ��ġ�� ��� (���� ������ ���ŷ ����)
    AsyncTask(ENamedThreads::BackgroundThreadPriority, [this]()
        {
            TArray<FVector> NewLocations;
            FVector SpawnCenter = bSpawnAroundPlayer ? GetPlayerLocation() : SpawnCenterLocation;

            // 50���� �� ��ġ ���
            for (int32 i = 0; i < 50; i++)
            {
                FVector SpawnLocation;
                if (FindRandomLocationOnNavMesh(SpawnCenter, DefaultSpawnRadius, SpawnLocation))
                {
                    NewLocations.Add(SpawnLocation);
                }
            }

            // ���� �����忡�� �迭 ������Ʈ
            AsyncTask(ENamedThreads::GameThread, [this, NewLocations]()
                {
                    if (NewLocations.Num() > 0)
                    {
                        // ���� ��ġ �Ϻθ� �� ��ġ�� ��ü
                        int32 StartIndex = FMath::RandRange(0, FMath::Max(0, PreCalculatedSpawnLocations.Num() - NewLocations.Num()));
                        for (int32 i = 0; i < NewLocations.Num() && StartIndex + i < PreCalculatedSpawnLocations.Num(); i++)
                        {
                            PreCalculatedSpawnLocations[StartIndex + i] = NewLocations[i];
                        }

                        UE_LOG(LogTemp, Warning, TEXT("Refreshed %d spawn locations"), NewLocations.Num());
                    }
                });
        });
}

FVector AMainGameModeBase::GetNextSpawnLocation()
{
    if (PreCalculatedSpawnLocations.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No pre-calculated spawn locations available!"));
        return GetPlayerLocation();
    }

    FVector SpawnLocation = PreCalculatedSpawnLocations[CurrentLocationIndex];
    CurrentLocationIndex = (CurrentLocationIndex + 1) % PreCalculatedSpawnLocations.Num();

    return SpawnLocation;
}

void AMainGameModeBase::StartWaveSystem()
{
    UE_LOG(LogTemp, Warning, TEXT("Wave Level System Started - Level %d"), CurrentWaveLevel);
    StartWaveLevel();
}

void AMainGameModeBase::StartWaveLevel()
{
    UE_LOG(LogTemp, Warning, TEXT("Starting Wave Level %d"), CurrentWaveLevel);

    // ���̺� ���� ���� �� �ʱ�ȭ
    SpawnStepCount = 0;
    bIsBossAlive = false;
    bWaitingForBossTransition = false;

    // ù ��° ������ 10�� �Ŀ� ����
    GetWorldTimerManager().SetTimer(
        MainSpawnTimer,
        this,
        &AMainGameModeBase::ProcessSpawnStep,
        SpawnInterval,
        true
    );
}

void AMainGameModeBase::ProcessSpawnStep()
{
    if (!bEnableWaveSystem || !bSpawnEnabled)
    {
        UE_LOG(LogTemp, Warning, TEXT("Spawn skipped - System disabled"));
        return;
    }

    // ���� ��� �� ��ȯ ��� ���̸� ���� �ߴ�
    if (bWaitingForBossTransition)
    {
        UE_LOG(LogTemp, Warning, TEXT("Spawn skipped - Waiting for boss transition"));
        return;
    }

    SpawnStepCount++;

    UE_LOG(LogTemp, Warning, TEXT("Processing spawn step %d (Wave Level %d)"), SpawnStepCount, CurrentWaveLevel);

    // ���� ���� ����
    if (SpawnStepCount <= 5)
    {
        // 10��(1��), 20��(2��), 30��(3��), 40��(4��), 50��(5��)
        SpawnEnemies(SpawnStepCount);
    }
    else if (SpawnStepCount == 6)
    {
        // 60��: ���� ����
        SpawnBoss();
    }
    else
    {
        // 70�� ���Ĵ� ���� ���̺� �������� ó��
        UE_LOG(LogTemp, Warning, TEXT("Unexpected spawn step: %d"), SpawnStepCount);
    }
}

void AMainGameModeBase::SpawnEnemies(int32 Count)
{
    if (!DefaultEnemyClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Default Enemy Class is null"));
        return;
    }

    if (!bLocationCalculationComplete)
    {
        UE_LOG(LogTemp, Warning, TEXT("Location calculation not complete yet"));
        return;
    }

    for (int32 i = 0; i < Count; i++)
    {
        FVector SpawnLocation = GetNextSpawnLocation();
        SpawnActorAtLocation(SpawnLocation);
    }

    UE_LOG(LogTemp, Warning, TEXT("Spawned %d enemies at step %d (Wave Level %d) - NavMesh searches: 0"),
        Count, SpawnStepCount, CurrentWaveLevel);
}

void AMainGameModeBase::SpawnActorAtLocation(const FVector& Location)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AEnemy* SpawnedEnemy = GetWorld()->SpawnActor<AEnemy>(
        DefaultEnemyClass,
        Location,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (SpawnedEnemy)
    {
        ApplyEnemyStatsForWaveLevel(SpawnedEnemy, CurrentWaveLevel);
        SpawnedEnemies.Add(SpawnedEnemy);
    }
}

void AMainGameModeBase::SpawnBoss()
{
    if (bIsBossAlive)
    {
        UE_LOG(LogTemp, Warning, TEXT("Boss spawn skipped - Boss already alive"));
        return;
    }

    if (!DefaultBossClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Default Boss Class is null"));
        return;
    }

    // ���� ���� �� ��� �� ����
    KillAllEnemies();

    // ���̺� ������ ��� �޸� ����
    ForceCompleteMemoryCleanup();

    FVector SpawnLocation = GetNextSpawnLocation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ABossEnemy* SpawnedBoss = GetWorld()->SpawnActor<ABossEnemy>(
        DefaultBossClass,
        SpawnLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (SpawnedBoss)
    {
        CurrentBoss = SpawnedBoss;
        bIsBossAlive = true;
        UE_LOG(LogTemp, Warning, TEXT("Boss spawned at step %d (Wave Level %d) - NavMesh searches: 0"),
            SpawnStepCount, CurrentWaveLevel);
    }
}

void AMainGameModeBase::OnBossDead()
{
    if (!bIsBossAlive) return;

    bIsBossAlive = false;
    CurrentBoss.Reset();
    bWaitingForBossTransition = true;

    UE_LOG(LogTemp, Warning, TEXT("Boss died at step %d (Wave Level %d) - Starting transition"),
        SpawnStepCount, CurrentWaveLevel);

    // ���� ���� Ÿ�̸� ����
    GetWorldTimerManager().ClearTimer(MainSpawnTimer);

    // 10�� �� ���� ���̺� ���� ����
    GetWorldTimerManager().SetTimer(
        PostBossTransitionTimer,
        this,
        &AMainGameModeBase::StartNextWaveLevel,
        10.0f,
        false
    );
}

void AMainGameModeBase::StartNextWaveLevel()
{
    UE_LOG(LogTemp, Warning, TEXT("Starting next wave level transition"));

    // ���̺� ���� ����
    CurrentWaveLevel++;

    // ���ο� ���̺� ���� ����
    StartWaveLevel();
}

void AMainGameModeBase::ForceCompleteMemoryCleanup()
{
    UE_LOG(LogTemp, Warning, TEXT("Performing complete memory cleanup for Wave Level %d"), CurrentWaveLevel);

    // 1. ��� �� ���� ����
    KillAllEnemies();

    // 2. �迭 ���� �ʱ�ȭ
    SpawnedEnemies.Empty();
    SpawnedEnemies.Shrink();

    // 3. ���� ���� ����
    CurrentBoss.Reset();

    // 4. Ÿ�̸� ����
    GetWorldTimerManager().ClearTimer(MainSpawnTimer);
    GetWorldTimerManager().ClearTimer(PostBossTransitionTimer);

    // 5. ���� ������ �÷���
    if (GEngine)
    {
        GEngine->ForceGarbageCollection(true);
        UE_LOG(LogTemp, Warning, TEXT("Forced garbage collection completed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Memory cleanup completed. SpawnedEnemies array size: %d"),
        SpawnedEnemies.Num());
}

void AMainGameModeBase::ApplyEnemyStatsForWaveLevel(AEnemy* Enemy, int32 WaveLevel)
{
    if (!Enemy) return;

    if (WaveLevel > 1)
    {
        // ü�� ���� (�������� 100% ����)
        float HealthMultiplier = 1.0f + (WaveLevel - 1) * 1.0f;
        Enemy->Health = Enemy->Health * HealthMultiplier;

        // �̵��ӵ� ���� (�������� 30% ����, �ִ� 3��)
        float SpeedMultiplier = FMath::Min(1.0f + (WaveLevel - 1) * 0.3f, 3.0f);
        if (Enemy->GetCharacterMovement())
        {
            Enemy->GetCharacterMovement()->MaxWalkSpeed *= SpeedMultiplier;
        }

        UE_LOG(LogTemp, Warning, TEXT("Enemy enhanced for Wave Level %d - Health: %.1f, Speed multiplier: %.1f"),
            WaveLevel, Enemy->Health, SpeedMultiplier);
    }
}

void AMainGameModeBase::KillAllEnemies()
{
    int32 KilledCount = 0;

    for (int32 i = SpawnedEnemies.Num() - 1; i >= 0; i--)
    {
        if (SpawnedEnemies[i].IsValid())
        {
            AEnemy* Enemy = SpawnedEnemies[i].Get();
            if (Enemy && !Enemy->bIsDead)
            {
                // ���� InstantDeath() �Լ� ��� (�ִϸ��̼� ���)
                Enemy->InstantDeath();
                KilledCount++;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Killed %d enemies"), KilledCount);
}

void AMainGameModeBase::OnEnemyDestroyed(AEnemy* DestroyedEnemy)
{
    if (!DestroyedEnemy) return;

    SpawnedEnemies.RemoveAll([DestroyedEnemy](const TWeakObjectPtr<AEnemy>& EnemyPtr)
        {
            return EnemyPtr.Get() == DestroyedEnemy;
        });
}

void AMainGameModeBase::StopWaveSystem()
{
    UE_LOG(LogTemp, Warning, TEXT("Wave System STOPPED"));

    GetWorldTimerManager().ClearTimer(MainSpawnTimer);
    GetWorldTimerManager().ClearTimer(PostBossTransitionTimer);
    GetWorldTimerManager().ClearTimer(LocationRefreshTimer);

    // ���Ž� Ÿ�̸ӵ鵵 ����
    for (FTimerHandle& Timer : EnemySpawnTimers)
    {
        GetWorldTimerManager().ClearTimer(Timer);
    }
    for (FTimerHandle& Timer : BossSpawnTimers)
    {
        GetWorldTimerManager().ClearTimer(Timer);
    }

    EnemySpawnTimers.Empty();
    BossSpawnTimers.Empty();
}

void AMainGameModeBase::SetWaveSystemEnabled(bool bEnabled)
{
    bEnableWaveSystem = bEnabled;

    if (bEnableWaveSystem)
    {
        StartWaveSystem();
    }
    else
    {
        StopWaveSystem();
    }
}

// ���Ž� �Լ���
void AMainGameModeBase::SpawnEnemyWave(int32 EnemyWaveIndex)
{
    if (!bEnableWaveSystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("Legacy enemy spawn blocked - Wave System is disabled"));
        return;
    }
}

void AMainGameModeBase::SpawnBossWave(int32 BossWaveIndex)
{
    if (!bEnableWaveSystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("Legacy boss spawn blocked - Wave System is disabled"));
        return;
    }
}

bool AMainGameModeBase::SpawnActorOnNavMesh(TSubclassOf<class APawn> ActorClass, FVector CenterLocation,
    float Radius, float ZOffset)
{
    if (!ActorClass) return false;

    FVector SpawnLocation;
    if (FindRandomLocationOnNavMesh(CenterLocation, Radius, SpawnLocation))
    {
        SpawnLocation.Z += ZOffset;

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        APawn* SpawnedActor = GetWorld()->SpawnActor<APawn>(
            ActorClass,
            SpawnLocation,
            FRotator::ZeroRotator,
            SpawnParams
        );

        return SpawnedActor != nullptr;
    }
    return false;
}

bool AMainGameModeBase::FindRandomLocationOnNavMesh(FVector CenterLocation, float Radius, FVector& OutLocation)
{
    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSystem) return false;

    FNavLocation RandomLocation;
    bool bFound = NavSystem->GetRandomPointInNavigableRadius(CenterLocation, Radius, RandomLocation);

    if (bFound)
    {
        OutLocation = RandomLocation.Location;
        return true;
    }
    return false;
}

FVector AMainGameModeBase::GetPlayerLocation()
{
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (PlayerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player location obtained: %s"), *PlayerCharacter->GetActorLocation().ToString());
        return PlayerCharacter->GetActorLocation();
    }
    UE_LOG(LogTemp, Warning, TEXT("Player character not found, returning ZeroVector"));
    return FVector::ZeroVector;
}
