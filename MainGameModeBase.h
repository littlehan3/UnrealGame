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

// ���̺꺰 ���� ���� ����ü
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

// ���̺� ���� ����ü
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

    // === ���̺� �ý��� ���� ��Ʈ�� ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    bool bWaveSystemEnabled = true;

    // === 6�� AI Ŭ���� ��� ===
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

    // === ���̺� �ý��� ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    TArray<FWaveConfiguration> WaveConfigurations;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    float DefaultWaveClearDelay = 3.0f;

    // === ���� ���� ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    bool bSpawnAroundPlayer = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    FVector SpawnCenterLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    float DefaultSpawnRadius = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    float DroneSpawnHeightOffset = 200.0f;

    // === ���� ����ȭ ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 PreCalculatedLocationCount = 200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableAsyncCleanup = true;

private:
    // === ���̺� �ý��� ���� ===
    int32 CurrentWaveIndex = 0;
    bool bWaveInProgress = false;
    bool bWaveSystemActive = false;
    FTimerHandle WaveStartTimer;
    FTimerHandle SpawnTimer;
    FTimerHandle CleanupTimer;
    FTimerHandle LocationRefreshTimer;

    // ���� ���̺� ���� ���� ��Ȳ
    int32 CurrentSpawnEntryIndex = 0;
    int32 CurrentSpawnCount = 0;

    // === �� ���� ===
    TArray<TWeakObjectPtr<class APawn>> SpawnedEnemies;
    int32 TotalEnemiesInWave = 0;
    int32 EnemiesKilledInWave = 0;

    // === ���� ��ġ ���� ===
    TArray<FVector> PreCalculatedSpawnLocations;
    int32 CurrentLocationIndex = 0;
    bool bLocationCalculationComplete = false;

    // === ���̺� �ý��� �Լ� ===
    void StartWaveSystem();
    void StartWave(int32 WaveIndex);
    void ProcessWaveSpawn();
    void SpawnEnemyFromEntry(const FWaveSpawnEntry& Entry);
    void CheckWaveCompletion();
    void OnWaveCompleted();
    void PrepareNextWave();

    UFUNCTION()
    void StartNextWave();

    // === ���� �Լ� ===
    void SpawnEnemyAtLocation(TSubclassOf<class APawn> EnemyClass, const FVector& Location);

    // === ���� ����ȭ �Լ� ===
    void PreCalculateSpawnLocations();
    void RefreshSpawnLocations();
    void RefillSpawnLocationsAsync();
    FVector GetNextSpawnLocation();
    void PerformAsyncCleanup();
    void ForceCompleteMemoryCleanup();

    // === ��ƿ��Ƽ �Լ� ===
    void KillAllEnemies();
    bool FindRandomLocationOnNavMesh(FVector CenterLocation, float Radius, FVector& OutLocation);
    FVector GetPlayerLocation();
    void StopAllSystems();

public:
    // === ���̺� �ý��� ���� ===
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

    // === �� ���� ===
    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetActiveEnemyCount() const;

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetEnemiesKilledInWave() const { return EnemiesKilledInWave; }

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetTotalEnemiesInWave() const { return TotalEnemiesInWave; }

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetPreCalculatedLocationCount() const { return PreCalculatedSpawnLocations.Num(); }
};
