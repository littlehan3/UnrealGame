#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/TimerHandle.h"
#include "NavigationSystem.h"
#include "Enemy.h"
#include "BossEnemy.h"
#include "EnemyDrone.h"
#include "EnemyDog.h"
#include "EnemyGuardian.h"
#include "EnemyShooter.h"
#include "MainGameModeBase.generated.h"

// 웨이브별 스폰 정보 구조체
USTRUCT(BlueprintType)
struct FWaveSpawnEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Classes")
    TSubclassOf<class APawn> EnemyClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Spawn")
    int32 SpawnCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Spawn")
    float SpawnDelay = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Spawn")
    float SpawnInterval = 1.0f;

    FWaveSpawnEntry() : SpawnCount(1), SpawnDelay(0.0f), SpawnInterval(1.0f) {}
};

// 웨이브 설정 구조체
USTRUCT(BlueprintType)
struct FWaveConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Config")
    FString WaveName = TEXT("Wave");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Config")
    TArray<FWaveSpawnEntry> SpawnEntries;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Config")
    float PrepareTime = 5.0f;

    FWaveConfiguration() : PrepareTime(5.0f) {}
};

UCLASS()
class LOCOMOTION_API AMainGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMainGameModeBase();

    UFUNCTION(BlueprintCallable)
    void OnEnemyDestroyed(APawn* DestroyedEnemy);

protected:
    virtual void BeginPlay() override;

    // === 웨이브 시스템 메인 컨트롤 ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    bool bWaveSystemEnabled = true;

    // === 6개 AI 클래스 등록 ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Classes")
    TSubclassOf<class AEnemy> EnemyType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Classes")
    TSubclassOf<class ABossEnemy> BossEnemyType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Classes")
    TSubclassOf<class AEnemyDog> EnemyDogType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Classes")
    TSubclassOf<class AEnemyDrone> EnemyDroneType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Classes")
    TSubclassOf<class AEnemyGuardian> EnemyGuardianType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Classes")
    TSubclassOf<class AEnemyShooter> EnemyShooterType;

    // === 웨이브 시스템 ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    TArray<FWaveConfiguration> WaveConfigurations;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    float DefaultWaveClearDelay = 3.0f;

    // === 스폰 설정 ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    bool bSpawnAroundPlayer = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    FVector SpawnCenterLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    float DefaultSpawnRadius = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    float DroneSpawnHeightOffset = 200.0f;

    // === 성능 최적화 ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 PreCalculatedLocationCount = 200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableAsyncCleanup = true;

private:
    // === 웨이브 시스템 변수 ===
    int32 CurrentWaveIndex = 0;
    bool bWaveInProgress = false;
    bool bWaveSystemActive = false;
    FTimerHandle WaveStartTimer;
    FTimerHandle SpawnTimer;
    FTimerHandle CleanupTimer;
    FTimerHandle LocationRefreshTimer;

    // 현재 웨이브 스폰 진행 상황
    int32 CurrentSpawnEntryIndex = 0;
    int32 CurrentSpawnCount = 0;

    // === 적 관리 ===
    TArray<TWeakObjectPtr<class APawn>> SpawnedEnemies;
    int32 TotalEnemiesInWave = 0;
    int32 EnemiesKilledInWave = 0;

    // === 스폰 위치 관리 ===
    TArray<FVector> PreCalculatedSpawnLocations;
    int32 CurrentLocationIndex = 0;
    bool bLocationCalculationComplete = false;

    // === 웨이브 시스템 함수 ===
    void StartWaveSystem();
    void StartWave(int32 WaveIndex);
    void ProcessWaveSpawn();
    void SpawnEnemyFromEntry(const FWaveSpawnEntry& Entry);
    void CheckWaveCompletion();
    void OnWaveCompleted();
    void PrepareNextWave();

    UFUNCTION()
    void StartNextWave();

    // === 스폰 함수 ===
    void SpawnEnemyAtLocation(TSubclassOf<class APawn> EnemyClass, const FVector& Location);

    // === 성능 최적화 함수 ===
    void PreCalculateSpawnLocations();
    void RefreshSpawnLocations();
    void RefillSpawnLocationsAsync();
    FVector GetNextSpawnLocation();
    void PerformAsyncCleanup();
    void ForceCompleteMemoryCleanup();

    // === 유틸리티 함수 ===
    void KillAllEnemies();
    bool FindRandomLocationOnNavMesh(FVector CenterLocation, float Radius, FVector& OutLocation);
    FVector GetPlayerLocation();
    void StopAllSystems();

public:
    // === 웨이브 시스템 제어 ===
    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void SetWaveSystemEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    bool IsWaveSystemEnabled() const { return bWaveSystemEnabled; }

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; }

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    bool IsWaveInProgress() const { return bWaveInProgress; }

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    int32 GetTotalWaveCount() const { return WaveConfigurations.Num(); }

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    FString GetCurrentWaveName() const;

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void RestartWaveSystem();

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void SkipToWave(int32 WaveIndex);

    // === 적 정보 ===
    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetActiveEnemyCount() const;

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetEnemiesKilledInWave() const { return EnemiesKilledInWave; }

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetTotalEnemiesInWave() const { return TotalEnemiesInWave; }

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetPreCalculatedLocationCount() const { return PreCalculatedSpawnLocations.Num(); }
};
