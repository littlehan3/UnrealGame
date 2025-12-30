#include "MainGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "Async/Async.h"
#include "WaveRecordSaveGame.h"

AMainGameModeBase::AMainGameModeBase()
{
    PrimaryActorTick.bCanEverTick = false;

    // 웨이브 시스템 기본값
    bWaveSystemEnabled = true;
    DefaultWaveClearDelay = 3.0f;
    CurrentWaveIndex = 0;
    bWaveInProgress = false;
    bWaveSystemActive = false;
    bEnableAsyncCleanup = true;
    PreCalculatedLocationCount = 200;
    CurrentLocationIndex = 0;
    bLocationCalculationComplete = false;
    DroneSpawnHeightOffset = 200.0f;

    // 적 관리 초기화
    TotalEnemiesInWave = 0;
    EnemiesKilledInWave = 0;
    CurrentSpawnEntryIndex = 0;
    CurrentSpawnCount = 0;
}

void AMainGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    // AI 클래스 등록 상태 확인
    UE_LOG(LogTemp, Error, TEXT("=== AI CLASSES REGISTRATION CHECK ==="));
    UE_LOG(LogTemp, Error, TEXT("EnemyType: %s"), EnemyType ? *EnemyType->GetName() : TEXT("NOT SET"));
    UE_LOG(LogTemp, Error, TEXT("BossEnemyType: %s"), BossEnemyType ? *BossEnemyType->GetName() : TEXT("NOT SET"));
    UE_LOG(LogTemp, Error, TEXT("EnemyDogType: %s"), EnemyDogType ? *EnemyDogType->GetName() : TEXT("NOT SET"));
    UE_LOG(LogTemp, Error, TEXT("EnemyDroneType: %s"), EnemyDroneType ? *EnemyDroneType->GetName() : TEXT("NOT SET"));
    UE_LOG(LogTemp, Error, TEXT("EnemyGuardianType: %s"), EnemyGuardianType ? *EnemyGuardianType->GetName() : TEXT("NOT SET"));
    UE_LOG(LogTemp, Error, TEXT("EnemyShooterType: %s"), EnemyShooterType ? *EnemyShooterType->GetName() : TEXT("NOT SET"));

    // 스폰 위치 사전 계산 및 웨이브 시스템 시작
    GetWorldTimerManager().SetTimerForNextTick([this]()
        {
            PreCalculateSpawnLocations();

            GetWorldTimerManager().SetTimer(
                LocationRefreshTimer,
                this,
                &AMainGameModeBase::RefreshSpawnLocations,
                300.0f,
                true
            );

            if (bWaveSystemEnabled)
            {
                UE_LOG(LogTemp, Warning, TEXT("Wave system starting in 3.0 seconds..."));
                GetWorldTimerManager().SetTimer(
                    WaveSystemStartDelayTimer, // 새로 사용할 TimerHandle
                    this,
                    &AMainGameModeBase::StartWaveSystem,
                    3.0f, // 3초 지연
                    false
                );
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Wave system is disabled - No enemies will spawn"));
            }
        });
}


// ========================================
// 웨이브 시스템
// ========================================

void AMainGameModeBase::StartWaveSystem()
{
    if (WaveConfigurations.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No wave configurations found! Please set up waves in Blueprint."));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Wave System Started - Total Waves: %d"), WaveConfigurations.Num());

    bWaveSystemActive = true;
    CurrentWaveIndex = 0;
    StartWave(0);
}

void AMainGameModeBase::StartWave(int32 WaveIndex)
{
    if (WaveIndex >= WaveConfigurations.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("All waves completed! Wave system finished."));
        bWaveSystemActive = false;
        return;
    }

    const FWaveConfiguration& WaveConfig = WaveConfigurations[WaveIndex];
    CurrentWaveIndex = WaveIndex;

    UE_LOG(LogTemp, Error, TEXT("=== WAVE %d CONFIGURATION DEBUG ==="), WaveIndex + 1);
    UE_LOG(LogTemp, Error, TEXT("Wave Name: %s"), *WaveConfig.WaveName);
    UE_LOG(LogTemp, Error, TEXT("Spawn Entries Count: %d"), WaveConfig.SpawnEntries.Num());

    // 각 스폰 엔트리 상세 출력
    for (int32 i = 0; i < WaveConfig.SpawnEntries.Num(); i++)
    {
        const FWaveSpawnEntry& Entry = WaveConfig.SpawnEntries[i];
        UE_LOG(LogTemp, Error, TEXT("Entry [%d]:"), i);
        UE_LOG(LogTemp, Error, TEXT("  - EnemyClass: %s"), Entry.EnemyClass ? *Entry.EnemyClass->GetName() : TEXT("NULL"));
        UE_LOG(LogTemp, Error, TEXT("  - SpawnCount: %d"), Entry.SpawnCount);
        UE_LOG(LogTemp, Error, TEXT("  - SpawnDelay: %f"), Entry.SpawnDelay);
        UE_LOG(LogTemp, Error, TEXT("  - SpawnInterval: %f"), Entry.SpawnInterval);
    }

    // 웨이브 시작 전 준비
    bWaveInProgress = true;
    CurrentSpawnEntryIndex = 0;
    CurrentSpawnCount = 0;
    TotalEnemiesInWave = 0;
    EnemiesKilledInWave = 0;

    // 총 스폰될 적 수 계산
    for (const FWaveSpawnEntry& Entry : WaveConfig.SpawnEntries)
    {
        if (Entry.EnemyClass)
        {
            TotalEnemiesInWave += Entry.SpawnCount;
        }
    }

    UE_LOG(LogTemp, Error, TEXT("Total enemies to spawn in wave: %d"), TotalEnemiesInWave);
    UE_LOG(LogTemp, Error, TEXT("Prepare time: %f seconds"), WaveConfig.PrepareTime);

    // 메인 캐릭터를 가져와서 사운드 재생 준비
    ACharacter * PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    AMainCharacter* MainCharacter = Cast<AMainCharacter>(PlayerCharacter);

    // 준비 시간 후 스폰 시작
    if (WaveConfig.PrepareTime > 0.0f)
    {
        if (MainCharacter)
        {
            MainCharacter->PlayWavePrepareSound();
        }

        UE_LOG(LogTemp, Error, TEXT("Starting spawn timer with %f seconds delay"), WaveConfig.PrepareTime);
        GetWorldTimerManager().SetTimer(
            WaveStartTimer,
            this,
            &AMainGameModeBase::ProcessWaveSpawn,
            WaveConfig.PrepareTime,
            false
        );
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Starting spawn immediately"));
        ProcessWaveSpawn();
    }
}


void AMainGameModeBase::ProcessWaveSpawn()
{
    UE_LOG(LogTemp, Error, TEXT("=== ProcessWaveSpawn DEBUG ==="));
    UE_LOG(LogTemp, Error, TEXT("CurrentSpawnEntryIndex: %d / %d"), CurrentSpawnEntryIndex, WaveConfigurations[CurrentWaveIndex].SpawnEntries.Num());
    UE_LOG(LogTemp, Error, TEXT("CurrentSpawnCount: %d"), CurrentSpawnCount);

    if (!bWaveSystemActive || CurrentWaveIndex >= WaveConfigurations.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("Wave system inactive or invalid wave index"));
        return;
    }

    const FWaveConfiguration& WaveConfig = WaveConfigurations[CurrentWaveIndex];

    if (CurrentSpawnEntryIndex < WaveConfig.SpawnEntries.Num())
    {
        const FWaveSpawnEntry& CurrentEntry = WaveConfig.SpawnEntries[CurrentSpawnEntryIndex];

        UE_LOG(LogTemp, Error, TEXT("Processing entry %d: %s (Count: %d/%d)"),
            CurrentSpawnEntryIndex,
            CurrentEntry.EnemyClass ? *CurrentEntry.EnemyClass->GetName() : TEXT("NULL"),
            CurrentSpawnCount,
            CurrentEntry.SpawnCount);

        if (CurrentEntry.EnemyClass && CurrentSpawnCount < CurrentEntry.SpawnCount)
        {
            // 현재 엔트리의 적 스폰
            SpawnEnemyFromEntry(CurrentEntry);
            CurrentSpawnCount++;

            UE_LOG(LogTemp, Error, TEXT("Spawned %d/%d of current entry"), CurrentSpawnCount, CurrentEntry.SpawnCount);

            // 같은 타입을 더 스폰해야 하는지 확인
            if (CurrentSpawnCount < CurrentEntry.SpawnCount)
            {
                UE_LOG(LogTemp, Error, TEXT("Scheduling next spawn of same type in %f seconds"), CurrentEntry.SpawnInterval);
                GetWorldTimerManager().SetTimer(
                    SpawnTimer,
                    this,
                    &AMainGameModeBase::ProcessWaveSpawn,
                    CurrentEntry.SpawnInterval,
                    false
                );
                return;
            }
            else
            {
                // 현재 엔트리 완료, 다음 엔트리로 이동
                UE_LOG(LogTemp, Error, TEXT("Current entry completed, moving to next entry"));
                CurrentSpawnEntryIndex++;
                CurrentSpawnCount = 0;

                // 다음 엔트리가 있는지 확인
                if (CurrentSpawnEntryIndex < WaveConfig.SpawnEntries.Num())
                {
                    const FWaveSpawnEntry& NextEntry = WaveConfig.SpawnEntries[CurrentSpawnEntryIndex];
                    UE_LOG(LogTemp, Error, TEXT("Starting next entry %d: %s after %f seconds delay"),
                        CurrentSpawnEntryIndex,
                        NextEntry.EnemyClass ? *NextEntry.EnemyClass->GetName() : TEXT("NULL"),
                        NextEntry.SpawnDelay);

                    // ⭐ 핵심 수정: 지연시간이 0이면 즉시 처리, 아니면 타이머 설정
                    if (NextEntry.SpawnDelay <= 0.0f)
                    {
                        UE_LOG(LogTemp, Error, TEXT("No delay, processing next entry immediately"));
                        ProcessWaveSpawn(); // 즉시 재귀 호출
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Setting timer for next entry"));
                        GetWorldTimerManager().SetTimer(
                            SpawnTimer,
                            this,
                            &AMainGameModeBase::ProcessWaveSpawn,
                            NextEntry.SpawnDelay,
                            false
                        );
                    }
                    return;
                }
            }
        }
        else
        {
            // 유효하지 않은 엔트리이거나 스폰 완료됨, 다음 엔트리로
            UE_LOG(LogTemp, Error, TEXT("Invalid entry or completed, moving to next"));
            CurrentSpawnEntryIndex++;
            CurrentSpawnCount = 0;

            // 즉시 다음 엔트리 처리
            ProcessWaveSpawn();
            return;
        }
    }

    // 모든 엔트리 완료
    UE_LOG(LogTemp, Error, TEXT("All spawn entries completed! Starting wave completion check."));
    CheckWaveCompletion();
}

void AMainGameModeBase::SpawnEnemyFromEntry(const FWaveSpawnEntry& Entry)
{
    UE_LOG(LogTemp, Error, TEXT("=== SpawnEnemyFromEntry DEBUG START ==="));
    UE_LOG(LogTemp, Error, TEXT("Entry.EnemyClass is valid: %s"), Entry.EnemyClass ? TEXT("TRUE") : TEXT("FALSE"));

    if (Entry.EnemyClass)
    {
        UE_LOG(LogTemp, Error, TEXT("EnemyClass Name: %s"), *Entry.EnemyClass->GetName());
        UE_LOG(LogTemp, Error, TEXT("EnemyClass Path: %s"), *Entry.EnemyClass->GetPathName());

        // 클래스가 각각 무엇인지 체크
        if (Entry.EnemyClass->IsChildOf<AEnemy>())
        {
            UE_LOG(LogTemp, Error, TEXT("This is AEnemy class"));
        }
        else if (Entry.EnemyClass->IsChildOf<AEnemyDog>())
        {
            UE_LOG(LogTemp, Error, TEXT("This is AEnemyDog class"));
        }
        else if (Entry.EnemyClass->IsChildOf<AEnemyDrone>())
        {
            UE_LOG(LogTemp, Error, TEXT("This is AEnemyDrone class"));
        }
        else if (Entry.EnemyClass->IsChildOf<AEnemyGuardian>())
        {
            UE_LOG(LogTemp, Error, TEXT("This is AEnemyGuardian class"));
        }
        else if (Entry.EnemyClass->IsChildOf<AEnemyShooter>())
        {
            UE_LOG(LogTemp, Error, TEXT("This is AEnemyShooter class"));
        }
        else if (Entry.EnemyClass->IsChildOf<ABossEnemy>())
        {
            UE_LOG(LogTemp, Error, TEXT("This is ABossEnemy class"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Unknown class type - IsChildOf<APawn>: %s"),
                Entry.EnemyClass->IsChildOf<APawn>() ? TEXT("TRUE") : TEXT("FALSE"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("EnemyClass is NULL! Cannot spawn."));
        return;
    }

    FVector SpawnLocation = GetNextSpawnLocation();
    UE_LOG(LogTemp, Error, TEXT("Spawn Location: %s"), *SpawnLocation.ToString());

    SpawnEnemyAtLocation(Entry.EnemyClass, SpawnLocation);
    UE_LOG(LogTemp, Error, TEXT("=== SpawnEnemyFromEntry DEBUG END ==="));
}


void AMainGameModeBase::OnWaveCompleted()
{
    if (!bWaveInProgress) return;

    bWaveInProgress = false;

    UE_LOG(LogTemp, Warning, TEXT("Wave %d completed! Enemies killed: %d/%d"),
        CurrentWaveIndex + 1, EnemiesKilledInWave, TotalEnemiesInWave);

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    AMainCharacter* MainCharacter = Cast<AMainCharacter>(PlayerCharacter);

    if (MainCharacter)
    {
        MainCharacter->GiveReward(HealthRewardOnClear, AmmoRewardOnClear);
        UE_LOG(LogTemp, Warning, TEXT("Giving wave clear reward: Health=%f, Ammo=%d"),
            HealthRewardOnClear, AmmoRewardOnClear);
    }

    // 비동기 메모리 정리 시작
    if (bEnableAsyncCleanup)
    {
        PerformAsyncCleanup();
    }

    PrepareNextWave();
}

void AMainGameModeBase::PrepareNextWave()
{
    // 다음 웨이브 준비
    GetWorldTimerManager().SetTimer(
        WaveStartTimer,
        this,
        &AMainGameModeBase::StartNextWave,
        DefaultWaveClearDelay,
        false
    );
}

void AMainGameModeBase::StartNextWave()
{
    int32 NextWaveIndex = CurrentWaveIndex + 1;

    if (NextWaveIndex < WaveConfigurations.Num())
    {
        StartWave(NextWaveIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("All waves completed! Game finished."));
        bWaveSystemActive = false;
    }
}

// ========================================
// 스폰 함수들
// ========================================

void AMainGameModeBase::SpawnEnemyAtLocation(TSubclassOf<class APawn> EnemyClass, const FVector& Location)
{
    UE_LOG(LogTemp, Error, TEXT("=== SpawnEnemyAtLocation DEBUG START ==="));
    UE_LOG(LogTemp, Error, TEXT("EnemyClass is valid: %s"), EnemyClass ? TEXT("TRUE") : TEXT("FALSE"));

    if (!EnemyClass)
    {
        UE_LOG(LogTemp, Error, TEXT("EnemyClass is NULL! Aborting spawn."));
        return;
    }

    UE_LOG(LogTemp, Error, TEXT("Attempting to spawn: %s"), *EnemyClass->GetName());

    // 드론의 경우 스폰 위치 높이 조정
    FVector AdjustedLocation = Location;
    if (EnemyClass->IsChildOf<AEnemyDrone>())
    {
        AdjustedLocation.Z += DroneSpawnHeightOffset;
        UE_LOG(LogTemp, Error, TEXT("Drone detected - Adjusted spawn height by +%f"), DroneSpawnHeightOffset);
    }

    UE_LOG(LogTemp, Error, TEXT("At location: %s"), *Location.ToString());

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnParams.Name = NAME_None; // 언리얼이 자동으로 고유한 이름 생성

    UE_LOG(LogTemp, Error, TEXT("World is valid: %s"), GetWorld() ? TEXT("TRUE") : TEXT("FALSE"));

    // 스폰 시도 전
    UE_LOG(LogTemp, Error, TEXT("About to call SpawnActor..."));

    APawn* SpawnedEnemy = GetWorld()->SpawnActor<APawn>(
        EnemyClass,
        AdjustedLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );

    // 스폰 결과 확인
    if (SpawnedEnemy)
    {
        UE_LOG(LogTemp, Error, TEXT("SUCCESS: Spawned % s"), *SpawnedEnemy->GetClass()->GetName());
        UE_LOG(LogTemp, Error, TEXT("Spawned Actor Name: %s"), *SpawnedEnemy->GetName());
        UE_LOG(LogTemp, Error, TEXT("Spawned Actor Location: %s"), *SpawnedEnemy->GetActorLocation().ToString());

        SpawnedEnemies.Add(SpawnedEnemy);
        UE_LOG(LogTemp, Error, TEXT("Added to SpawnedEnemies array. Total count: %d"), SpawnedEnemies.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FAILED: Could not spawn %s"), *EnemyClass->GetName());
        UE_LOG(LogTemp, Error, TEXT("Possible reasons:"));
        UE_LOG(LogTemp, Error, TEXT("- Invalid spawn location"));
        UE_LOG(LogTemp, Error, TEXT("- Blueprint compilation issues"));
        UE_LOG(LogTemp, Error, TEXT("- Missing default constructor"));
        UE_LOG(LogTemp, Error, TEXT("- Class registration issues"));
    }

    UE_LOG(LogTemp, Error, TEXT("=== SpawnEnemyAtLocation DEBUG END ==="));
}



// ========================================
// 적 관리
// ========================================

void AMainGameModeBase::OnEnemyDestroyed(APawn* DestroyedEnemy)
{
    if (!DestroyedEnemy) return;

    SpawnedEnemies.RemoveAll([DestroyedEnemy](const TWeakObjectPtr<APawn>& EnemyPtr)
        {
            return EnemyPtr.Get() == DestroyedEnemy;
        });

    if (bWaveSystemEnabled && bWaveInProgress)
    {
        EnemiesKilledInWave++;
        UE_LOG(LogTemp, Log, TEXT("Enemy killed. Progress: %d/%d"),
            EnemiesKilledInWave, TotalEnemiesInWave);

        CheckWaveCompletion();
    }
}

// ========================================
// 성능 최적화
// ========================================

void AMainGameModeBase::PerformAsyncCleanup()
{
    UE_LOG(LogTemp, Log, TEXT("Starting async cleanup for Wave %d"), CurrentWaveIndex + 1);

    AsyncTask(ENamedThreads::BackgroundThreadPriority, [this]()
        {
            FPlatformProcess::Sleep(0.1f);

            AsyncTask(ENamedThreads::GameThread, [this]()
                {
                    // 무효한 참조 정리
                    SpawnedEnemies.RemoveAll([](const TWeakObjectPtr<APawn>& EnemyPtr)  // AEnemy → APawn
                        {
                            return !EnemyPtr.IsValid();
                        });

                    SpawnedEnemies.Shrink();

                    UE_LOG(LogTemp, Log, TEXT("Async cleanup completed. Active enemies: %d"),
                        SpawnedEnemies.Num());
                });
        });
}


void AMainGameModeBase::ForceCompleteMemoryCleanup()
{
    UE_LOG(LogTemp, Warning, TEXT("Performing complete memory cleanup"));

    // 모든 적 강제 제거
    KillAllEnemies();

    // 배열 완전 초기화
    SpawnedEnemies.Empty();
    SpawnedEnemies.Shrink();

    // 타이머 정리
    GetWorldTimerManager().ClearTimer(WaveStartTimer);
    GetWorldTimerManager().ClearTimer(SpawnTimer);
    GetWorldTimerManager().ClearTimer(CleanupTimer);

    // 강제 가비지 컬렉션
    if (GEngine)
    {
        GEngine->ForceGarbageCollection(true);
        UE_LOG(LogTemp, Warning, TEXT("Forced garbage collection completed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Complete memory cleanup finished"));
}

// ========================================
// 공개 인터페이스
// ========================================

void AMainGameModeBase::SetWaveSystemEnabled(bool bEnabled)
{
    if (bWaveSystemEnabled == bEnabled) return;

    bWaveSystemEnabled = bEnabled;
    StopAllSystems();

    if (bWaveSystemEnabled)
    {
        StartWaveSystem();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Wave system disabled"));
    }
}

FString AMainGameModeBase::GetCurrentWaveName() const
{
    if (CurrentWaveIndex < WaveConfigurations.Num())
    {
        return WaveConfigurations[CurrentWaveIndex].WaveName;
    }
    return TEXT("Unknown Wave");
}

int32 AMainGameModeBase::GetActiveEnemyCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<APawn>& EnemyPtr : SpawnedEnemies)  // AEnemy → APawn
    {
        if (EnemyPtr.IsValid())
        {
            Count++;
        }
    }

    return Count;
}

void AMainGameModeBase::CheckAndSaveBestWaveRecord(int32 LastClearedWaveIndex)
{
    // 1. 저장 객체 로드 또는 생성
    UWaveRecordSaveGame* SaveGameInstance = nullptr;

    if (UGameplayStatics::DoesSaveGameExist(UWaveRecordSaveGame::SaveSlotName, UWaveRecordSaveGame::UserIndex))
    {
        SaveGameInstance = Cast<UWaveRecordSaveGame>(UGameplayStatics::LoadGameFromSlot(UWaveRecordSaveGame::SaveSlotName, UWaveRecordSaveGame::UserIndex));
    }

    if (!SaveGameInstance)
    {
        // 기록이 없다면 새로 생성
        SaveGameInstance = Cast<UWaveRecordSaveGame>(UGameplayStatics::CreateSaveGameObject(UWaveRecordSaveGame::StaticClass()));
        if (!SaveGameInstance) return;
    }

    // 2. 최고 기록 비교
    if (LastClearedWaveIndex > SaveGameInstance->BestClearedWaveIndex)
    {
        // 새로운 최고 기록 업데이트
        SaveGameInstance->BestClearedWaveIndex = LastClearedWaveIndex;

        // 웨이브 이름 가져오기 (인덱스가 유효할 경우)
        if (LastClearedWaveIndex >= 0 && LastClearedWaveIndex < WaveConfigurations.Num())
        {
            SaveGameInstance->BestClearedWaveName = WaveConfigurations[LastClearedWaveIndex].WaveName;
            UE_LOG(LogTemp, Warning, TEXT("New Best Record: Wave %d (%s) cleared!"), LastClearedWaveIndex + 1, *SaveGameInstance->BestClearedWaveName);
        }
        else
        {
            SaveGameInstance->BestClearedWaveName = FString::Printf(TEXT("Wave %d"), LastClearedWaveIndex + 1);
            UE_LOG(LogTemp, Warning, TEXT("New Best Record: Wave %d cleared! (Name not found)"), LastClearedWaveIndex + 1);
        }

        // 3. 디스크에 저장
        UGameplayStatics::SaveGameToSlot(SaveGameInstance, UWaveRecordSaveGame::SaveSlotName, UWaveRecordSaveGame::UserIndex);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Current score (Wave %d) is not a new best record (Best is Wave %d)."),
            LastClearedWaveIndex + 1, SaveGameInstance->BestClearedWaveIndex + 1);
    }
}

void AMainGameModeBase::RestartWaveSystem()
{
    StopAllSystems();

    if (bWaveSystemEnabled)
    {
        StartWaveSystem();
    }
}

void AMainGameModeBase::SkipToWave(int32 WaveIndex)
{
    if (WaveIndex >= 0 && WaveIndex < WaveConfigurations.Num())
    {
        StopAllSystems();
        KillAllEnemies();
        StartWave(WaveIndex);
    }
}

void AMainGameModeBase::StopAllSystems()
{
    // 웨이브 시스템 정지
    bWaveSystemActive = false;
    bWaveInProgress = false;
    GetWorldTimerManager().ClearTimer(WaveStartTimer);
    GetWorldTimerManager().ClearTimer(SpawnTimer);
    GetWorldTimerManager().ClearTimer(CleanupTimer);
}

// ========================================
// 유틸리티 함수들
// ========================================

void AMainGameModeBase::PreCalculateSpawnLocations()
{
    UE_LOG(LogTemp, Warning, TEXT("Pre-calculating %d spawn locations..."), PreCalculatedLocationCount);

    PreCalculatedSpawnLocations.Empty();
    PreCalculatedSpawnLocations.Reserve(PreCalculatedLocationCount);

    FVector SpawnCenter = bSpawnAroundPlayer ? GetPlayerLocation() : SpawnCenterLocation;

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

    while (PreCalculatedSpawnLocations.Num() < PreCalculatedLocationCount)
    {
        FVector SpawnLocation;
        if (FindRandomLocationOnNavMesh(SpawnCenter, DefaultSpawnRadius, SpawnLocation))
        {
            PreCalculatedSpawnLocations.Add(SpawnLocation);
        }
        else
        {
            break;
        }
    }

    bLocationCalculationComplete = true;
    UE_LOG(LogTemp, Warning, TEXT("Pre-calculated %d spawn locations completed"), PreCalculatedSpawnLocations.Num());
}

void AMainGameModeBase::RefreshSpawnLocations()
{
    UE_LOG(LogTemp, Warning, TEXT("Refreshing spawn locations..."));
    RefillSpawnLocationsAsync();
}

void AMainGameModeBase::RefillSpawnLocationsAsync()
{
    if (!bLocationCalculationComplete) return;

    AsyncTask(ENamedThreads::BackgroundThreadPriority, [this]()
        {
            TArray<FVector> NewLocations;
            FVector SpawnCenter = bSpawnAroundPlayer ? GetPlayerLocation() : SpawnCenterLocation;

            for (int32 i = 0; i < 50; i++)
            {
                FVector SpawnLocation;
                if (FindRandomLocationOnNavMesh(SpawnCenter, DefaultSpawnRadius, SpawnLocation))
                {
                    NewLocations.Add(SpawnLocation);
                }
            }

            AsyncTask(ENamedThreads::GameThread, [this, NewLocations]()
                {
                    if (NewLocations.Num() > 0)
                    {
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

void AMainGameModeBase::KillAllEnemies()
{
    int32 KilledCount = 0;

    // 배열 복사 후 순회 (안전한 실행을 위해)
    TArray<TWeakObjectPtr<APawn>> EnemiesToKill = SpawnedEnemies;

    for (const auto& EnemyPtr : EnemiesToKill)
    {
        if (EnemyPtr.IsValid())
        {
            APawn* EnemyPawn = EnemyPtr.Get();

            // 1. AEnemy 타입인지 확인
            if (AEnemy* SpecificEnemy = Cast<AEnemy>(EnemyPawn))
            {
                SpecificEnemy->Die();
                KilledCount++;
                continue; // 다음 적으로 넘어감
            }

            // 2. AEnemyDog 타입인지 확인
            if (AEnemyDog* SpecificEnemy = Cast<AEnemyDog>(EnemyPawn))
            {
                SpecificEnemy->Die();
                KilledCount++;
                continue; // 다음 적으로 넘어감
            }

            // 3. AEnemyDrone 타입인지 확인
            if (AEnemyDrone* SpecificEnemy = Cast<AEnemyDrone>(EnemyPawn))
            {
                SpecificEnemy->Die();
                KilledCount++;
                continue;
            }

            // 4. AEnemyGuardian 타입인지 확인
            if (AEnemyGuardian* SpecificEnemy = Cast<AEnemyGuardian>(EnemyPawn))
            {
                SpecificEnemy->Die();
                KilledCount++;
                continue;
            }

            // 5. AEnemyShooter 타입인지 확인
            if (AEnemyShooter* SpecificEnemy = Cast<AEnemyShooter>(EnemyPawn))
            {
                SpecificEnemy->Die();
                KilledCount++;
                continue;
            }

            // 6. BossEnemy 타입인지 확인
            if (ABossEnemy* SpecificEnemy = Cast<ABossEnemy>(EnemyPawn))
            {
                SpecificEnemy->BossDie();
                KilledCount++;
                continue;
            }
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("Called Die() on %d enemies"), KilledCount);
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
        return PlayerCharacter->GetActorLocation();
    }
    return FVector::ZeroVector;
}

void AMainGameModeBase::CheckWaveCompletion()
{
    if (!bWaveInProgress) return;

    int32 ActiveEnemyCount = GetActiveEnemyCount();
    UE_LOG(LogTemp, Warning, TEXT("Check Wave Completion - Active enemies: %d"), ActiveEnemyCount);

    if (ActiveEnemyCount == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("All enemies defeated! Wave completed."));
        OnWaveCompleted();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Wave still in progress - %d enemies remaining"), ActiveEnemyCount);
    }
}
