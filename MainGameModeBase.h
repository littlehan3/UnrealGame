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

    // 적 스폰 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
    TArray<FEnemySpawnInfo> EnemySpawnList;

    // 보스 스폰 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Spawn")
    TArray<FBossSpawnInfo> BossSpawnList;

    // 플레이어 주변 스폰 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    bool bSpawnAroundPlayer = true;

    // 스폰 중심점
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    FVector SpawnCenterLocation = FVector::ZeroVector;

    // 웨이브 시스템 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    bool bEnableWaveSystem = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    float SpawnInterval = 10.0f;  // 10초마다 스폰

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    bool bSpawnEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    TSubclassOf<class AEnemy> DefaultEnemyClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    TSubclassOf<class ABossEnemy> DefaultBossClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    float DefaultSpawnRadius = 1000.0f;

    // 성능 최적화 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 PreCalculatedLocationCount = 200;  // 사전 계산할 위치 수

private:
    // 레거시 스폰 시스템
    TArray<FTimerHandle> EnemySpawnTimers;
    TArray<FTimerHandle> BossSpawnTimers;

    // 새로운 웨이브 레벨 시스템
    FTimerHandle MainSpawnTimer;
    FTimerHandle PostBossTransitionTimer;
    FTimerHandle LocationRefreshTimer;

    // 웨이브 레벨 관리
    int32 CurrentWaveLevel = 1;
    int32 SpawnStepCount = 0;       // 전체 스폰 단계 카운트 (0부터 시작)
    bool bIsBossAlive = false;
    bool bWaitingForBossTransition = false;  // 보스 사망 후 전환 대기 중

    // 성능 최적화 - 사전 계산된 스폰 위치
    TArray<FVector> PreCalculatedSpawnLocations;
    int32 CurrentLocationIndex = 0;
    bool bLocationCalculationComplete = false;

    // 메모리 관리
    TArray<TWeakObjectPtr<class AEnemy>> SpawnedEnemies;
    TWeakObjectPtr<class ABossEnemy> CurrentBoss;

    // 웨이브 레벨 시스템 함수들
    UFUNCTION()
    void ProcessSpawnStep();

    UFUNCTION()
    void StartNextWaveLevel();

    UFUNCTION()
    void RefreshSpawnLocations();

    void StartWaveLevel();
    void ForceCompleteMemoryCleanup();

    // 성능 최적화 함수들
    void PreCalculateSpawnLocations();
    void RefillSpawnLocationsAsync();
    FVector GetNextSpawnLocation();

    // 시스템 제어
    void StartWaveSystem();
    void StopWaveSystem();

    // 스폰 함수들
    void SpawnEnemies(int32 Count);
    void SpawnBoss();
    void SpawnActorAtLocation(const FVector& Location);

    // 메모리 관리
    void KillAllEnemies();

    // 유틸리티 함수들
    bool SpawnActorOnNavMesh(TSubclassOf<class APawn> ActorClass, FVector CenterLocation,
        float Radius, float ZOffset = 0.0f);
    bool FindRandomLocationOnNavMesh(FVector CenterLocation, float Radius, FVector& OutLocation);
    FVector GetPlayerLocation();
    void ApplyEnemyStatsForWaveLevel(AEnemy* Enemy, int32 WaveLevel);

    // 레거시 함수들
    UFUNCTION()
    void SpawnEnemyWave(int32 EnemyWaveIndex);

    UFUNCTION()
    void SpawnBossWave(int32 BossWaveIndex);

public:
    // 보스 사망 알림
    UFUNCTION(BlueprintCallable)
    void OnBossDead();

    // 시스템 제어
    UFUNCTION(BlueprintCallable)
    void SetSpawnEnabled(bool bEnabled) { bSpawnEnabled = bEnabled; }

    UFUNCTION(BlueprintCallable)
    void SetWaveSystemEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable)
    bool IsWaveSystemEnabled() const { return bEnableWaveSystem; }

    // 웨이브 레벨 정보
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
