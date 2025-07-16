#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/TimerHandle.h"
#include "NavigationSystem.h"
#include "MainGameModeBase.generated.h"

USTRUCT(BlueprintType)
struct FEnemySpawnInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<class AEnemy> EnemyClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SpawnCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnRadius;

    FEnemySpawnInfo() : SpawnTime(30.0f), SpawnCount(1), SpawnRadius(1000.0f) {}
};

USTRUCT(BlueprintType)
struct FBossSpawnInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<class ABossEnemy> BossClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnRadius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ZOffset;

    FBossSpawnInfo() : SpawnTime(60.0f), SpawnRadius(1000.0f), ZOffset(0.0f) {}
};

UCLASS()
class LOCOMOTION_API AMainGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMainGameModeBase();

    UFUNCTION(BlueprintCallable)
    void OnEnemyDestroyed(AEnemy* DestroyedEnemy);

protected:
    virtual void BeginPlay() override;

    // �� ���� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
    TArray<FEnemySpawnInfo> EnemySpawnList;

    // ���� ���� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Spawn")
    TArray<FBossSpawnInfo> BossSpawnList;

    // �÷��̾� �ֺ� ���� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    bool bSpawnAroundPlayer = true;

    // ���� �߽���
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    FVector SpawnCenterLocation = FVector::ZeroVector;

    // ���̺� �ý��� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    bool bEnableWaveSystem = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    float SpawnInterval = 10.0f;  // 10�ʸ��� ����

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    bool bSpawnEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    TSubclassOf<class AEnemy> DefaultEnemyClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    TSubclassOf<class ABossEnemy> DefaultBossClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    float DefaultSpawnRadius = 1000.0f;

    // ���� ����ȭ ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 PreCalculatedLocationCount = 200;  // ���� ����� ��ġ ��

private:
    // ���Ž� ���� �ý���
    TArray<FTimerHandle> EnemySpawnTimers;
    TArray<FTimerHandle> BossSpawnTimers;

    // ���ο� ���̺� ���� �ý���
    FTimerHandle MainSpawnTimer;
    FTimerHandle PostBossTransitionTimer;
    FTimerHandle LocationRefreshTimer;

    // ���̺� ���� ����
    int32 CurrentWaveLevel = 1;
    int32 SpawnStepCount = 0;       // ��ü ���� �ܰ� ī��Ʈ (0���� ����)
    bool bIsBossAlive = false;
    bool bWaitingForBossTransition = false;  // ���� ��� �� ��ȯ ��� ��

    // ���� ����ȭ - ���� ���� ���� ��ġ
    TArray<FVector> PreCalculatedSpawnLocations;
    int32 CurrentLocationIndex = 0;
    bool bLocationCalculationComplete = false;

    // �޸� ����
    TArray<TWeakObjectPtr<class AEnemy>> SpawnedEnemies;
    TWeakObjectPtr<class ABossEnemy> CurrentBoss;

    // ���̺� ���� �ý��� �Լ���
    UFUNCTION()
    void ProcessSpawnStep();

    UFUNCTION()
    void StartNextWaveLevel();

    UFUNCTION()
    void RefreshSpawnLocations();

    void StartWaveLevel();
    void ForceCompleteMemoryCleanup();

    // ���� ����ȭ �Լ���
    void PreCalculateSpawnLocations();
    void RefillSpawnLocationsAsync();
    FVector GetNextSpawnLocation();

    // �ý��� ����
    void StartWaveSystem();
    void StopWaveSystem();

    // ���� �Լ���
    void SpawnEnemies(int32 Count);
    void SpawnBoss();
    void SpawnActorAtLocation(const FVector& Location);

    // �޸� ����
    void KillAllEnemies();

    // ��ƿ��Ƽ �Լ���
    bool SpawnActorOnNavMesh(TSubclassOf<class APawn> ActorClass, FVector CenterLocation,
        float Radius, float ZOffset = 0.0f);
    bool FindRandomLocationOnNavMesh(FVector CenterLocation, float Radius, FVector& OutLocation);
    FVector GetPlayerLocation();
    void ApplyEnemyStatsForWaveLevel(AEnemy* Enemy, int32 WaveLevel);

    // ���Ž� �Լ���
    UFUNCTION()
    void SpawnEnemyWave(int32 EnemyWaveIndex);

    UFUNCTION()
    void SpawnBossWave(int32 BossWaveIndex);

public:
    // ���� ��� �˸�
    UFUNCTION(BlueprintCallable)
    void OnBossDead();

    // �ý��� ����
    UFUNCTION(BlueprintCallable)
    void SetSpawnEnabled(bool bEnabled) { bSpawnEnabled = bEnabled; }

    UFUNCTION(BlueprintCallable)
    void SetWaveSystemEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable)
    bool IsWaveSystemEnabled() const { return bEnableWaveSystem; }

    // ���̺� ���� ����
    UFUNCTION(BlueprintCallable)
    int32 GetCurrentWaveLevel() const { return CurrentWaveLevel; }

    UFUNCTION(BlueprintCallable)
    int32 GetSpawnStepCount() const { return SpawnStepCount; }

    UFUNCTION(BlueprintCallable)
    bool IsBossAlive() const { return bIsBossAlive; }

    UFUNCTION(BlueprintCallable)
    int32 GetActiveEnemyCount() const { return SpawnedEnemies.Num(); }

    UFUNCTION(BlueprintCallable)
    int32 GetPreCalculatedLocationCount() const { return PreCalculatedSpawnLocations.Num(); }
};
