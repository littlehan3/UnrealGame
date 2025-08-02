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

    // 기본값 설정
    SpawnInterval = 10.0f;
    bSpawnEnabled = true;
    bEnableWaveSystem = true;
    PreCalculatedLocationCount = 200;

    // 웨이브 레벨 시스템 초기화
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
                // 스폰 위치 사전 계산
                PreCalculateSpawnLocations();

                // 5분마다 스폰 위치 갱신
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

    // 여러 반지름으로 다양한 위치 계산
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

    // 부족한 위치들 추가 계산
    while (PreCalculatedSpawnLocations.Num() < PreCalculatedLocationCount)
    {
        FVector SpawnLocation;
        if (FindRandomLocationOnNavMesh(SpawnCenter, DefaultSpawnRadius, SpawnLocation))
        {
            PreCalculatedSpawnLocations.Add(SpawnLocation);
        }
        else
        {
            break; // NavMesh에서 더 이상 위치를 찾을 수 없음
        }
    }

    bLocationCalculationComplete = true;
    UE_LOG(LogTemp, Warning, TEXT("Pre-calculated %d spawn locations completed"), PreCalculatedSpawnLocations.Num());
}

void AMainGameModeBase::RefreshSpawnLocations()
{
    UE_LOG(LogTemp, Warning, TEXT("Refreshing spawn locations..."));

    // 비동기로 새 위치들 계산
    RefillSpawnLocationsAsync();
}

void AMainGameModeBase::RefillSpawnLocationsAsync()
{
    if (!bLocationCalculationComplete) return;

    // 백그라운드에서 새 위치들 계산 (메인 스레드 블로킹 없음)
    AsyncTask(ENamedThreads::BackgroundThreadPriority, [this]()
        {
            TArray<FVector> NewLocations;
            FVector SpawnCenter = bSpawnAroundPlayer ? GetPlayerLocation() : SpawnCenterLocation;

            // 50개의 새 위치 계산
            for (int32 i = 0; i < 50; i++)
            {
                FVector SpawnLocation;
                if (FindRandomLocationOnNavMesh(SpawnCenter, DefaultSpawnRadius, SpawnLocation))
                {
                    NewLocations.Add(SpawnLocation);
                }
            }

            // 메인 스레드에서 배열 업데이트
            AsyncTask(ENamedThreads::GameThread, [this, NewLocations]()
                {
                    if (NewLocations.Num() > 0)
                    {
                        // 기존 위치 일부를 새 위치로 교체
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

    // 웨이브 레벨 시작 시 초기화
    SpawnStepCount = 0;
    bIsBossAlive = false;
    bWaitingForBossTransition = false;

    // 첫 번째 스폰을 10초 후에 시작
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

    // 보스 사망 후 전환 대기 중이면 스폰 중단
    if (bWaitingForBossTransition)
    {
        UE_LOG(LogTemp, Warning, TEXT("Spawn skipped - Waiting for boss transition"));
        return;
    }

    SpawnStepCount++;

    UE_LOG(LogTemp, Warning, TEXT("Processing spawn step %d (Wave Level %d)"), SpawnStepCount, CurrentWaveLevel);

    // 스폰 패턴 결정
    if (SpawnStepCount <= 5)
    {
        // 10초(1명), 20초(2명), 30초(3명), 40초(4명), 50초(5명)
        SpawnEnemies(SpawnStepCount);
    }
    else if (SpawnStepCount == 6)
    {
        // 60초: 보스 스폰
        SpawnBoss();
    }
    else
    {
        // 70초 이후는 다음 웨이브 레벨에서 처리
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

    // 보스 스폰 전 모든 적 제거
    KillAllEnemies();

    // 웨이브 레벨의 모든 메모리 정리
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

    // 메인 스폰 타이머 중지
    GetWorldTimerManager().ClearTimer(MainSpawnTimer);

    // 10초 후 다음 웨이브 레벨 시작
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

    // 웨이브 레벨 증가
    CurrentWaveLevel++;

    // 새로운 웨이브 레벨 시작
    StartWaveLevel();
}

void AMainGameModeBase::ForceCompleteMemoryCleanup()
{
    UE_LOG(LogTemp, Warning, TEXT("Performing complete memory cleanup for Wave Level %d"), CurrentWaveLevel);

    // 1. 모든 적 강제 제거
    KillAllEnemies();

    // 2. 배열 완전 초기화
    SpawnedEnemies.Empty();
    SpawnedEnemies.Shrink();

    // 3. 보스 참조 정리
    CurrentBoss.Reset();

    // 4. 타이머 정리
    GetWorldTimerManager().ClearTimer(MainSpawnTimer);
    GetWorldTimerManager().ClearTimer(PostBossTransitionTimer);

    // 5. 강제 가비지 컬렉션
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
        // 체력 증가 (레벨마다 100% 증가)
        float HealthMultiplier = 1.0f + (WaveLevel - 1) * 1.0f;
        Enemy->Health = Enemy->Health * HealthMultiplier;

        // 이동속도 증가 (레벨마다 30% 증가, 최대 3배)
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
                // 기존 InstantDeath() 함수 사용 (애니메이션 재생)
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

    // 레거시 타이머들도 정리
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

// 레거시 함수들
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
