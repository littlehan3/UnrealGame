#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainGameModeBase.generated.h"

class APawn;
class AEnemy;
class ABossEnemy;
class AEnemyDrone;
class AEnemyDog;
class AEnemyShooter;
class AEnemyGuardian;
class UNavigationSystemV1;
class UWaveRecordSaveGame;

// 웨이브별 스폰 정보 구조체
USTRUCT(BlueprintType)
struct FWaveSpawnEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Classes")
    TSubclassOf<class APawn> EnemyClass; // 스폰할 적 클래스

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Spawn")
    int32 SpawnCount = 1; // 스폰 갯수

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Spawn")
    float SpawnDelay = 0.0f; // 항목별 스폰 지연 시간

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Spawn")
    float SpawnInterval = 1.0f; // 객체별 스폰 주기

    FWaveSpawnEntry() : SpawnCount(1), SpawnDelay(0.0f), SpawnInterval(1.0f) {}
};

// 웨이브 설정 구조체
USTRUCT(BlueprintType)
struct FWaveConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Config")
    FString WaveName = TEXT("Wave"); // 웨이브 이름

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Config")
    TArray<FWaveSpawnEntry> SpawnEntries; // 해당 웨이브의 적 스폰 목록 배열

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Config")
    float PrepareTime = 7.0f; // 웨이브 시작 전 대기시간

    FWaveConfiguration() : PrepareTime(7.0f) {}
};

UCLASS()
class LOCOMOTION_API AMainGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMainGameModeBase();

    // AI들이 참고할 수 있도록 현재 스폰된 적들의 명단을 반환하는 getter 함수
    const TArray<TWeakObjectPtr<class APawn>>& GetSpawnedEnemies() const { return SpawnedEnemies; }

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void OnEnemyDestroyed(APawn* DestroyedEnemy); // 적 파괴시 호출되어 웨이브 진행 상태 갱신

    // 웨이브 시스템 제어
    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void SetWaveSystemEnabled(bool bEnabled); // 웨이브 시스템 활성화 상태 설정 함수

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    bool IsWaveSystemEnabled() const { return bWaveSystemEnabled; } // 현재 웨이브 시스템 활성화 여부 반환 함수

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    FORCEINLINE int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; } // 현재 진행중인 웨이브 번호 반환 함수

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    FORCEINLINE bool IsWaveInProgress() const { return bWaveInProgress; } // 현재 웨이브가 진행중인지 여부 반환 함수

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    int32 GetTotalWaveCount() const { return WaveConfigurations.Num(); } // 전체 설정된 웨이브의 총 개수 반환 함수

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    FString GetCurrentWaveName() const; // 현재 웨이브의 이름을 가져오는 함수

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void RestartWaveSystem(); // 웨이브 시스템을 초기상태로 돌리고 재시작 하는 함수

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void SkipToWave(int32 WaveIndex); // 특정 웨이브 번호를 즉시 건너뛰는 함수

    // 적 정보
    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetActiveEnemyCount() const; // 현재 맵에 살아있는 적들의 수 반환

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetEnemiesKilledInWave() const { return EnemiesKilledInWave; } // 현재 웨이브에서 처치한 적들의 수 반환

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetTotalEnemiesInWave() const { return TotalEnemiesInWave; } // 현재 웨이브에서 스폰되어야 할 적들의 수 반환

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetPreCalculatedLocationCount() const { return PreCalculatedSpawnLocations.Num(); } // 사전에 계산되어 저장딘 스폰 가능 위치들의 수 반환

    // 최고 기록을 체크하고 저장하는 함수
    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void CheckAndSaveBestWaveRecord(int32 LastClearedWaveIndex); // 최고 기록을 확인하고 세이브 데이터에 저장하는 함수

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    bool bWaveSystemEnabled = true; // 웨이브 시스템 활성화 여부

    // 6개 AI 클래스 등록
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

    // 웨이브 시스템
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    TArray<FWaveConfiguration> WaveConfigurations; // 웨이브 설정 배열

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    float DefaultWaveClearDelay = 5.0f; // 기본 웨이브 클리어 지연시간

    // 스폰 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    bool bSpawnAroundPlayer = true; // 플레이 주변 스폰 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    FVector SpawnCenterLocation = FVector::ZeroVector; // 고정 스폰 중심점
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    float DefaultSpawnRadius = 1000.0f; // 스폰 반경
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    float DroneSpawnHeightOffset = 200.0f; // 드론 전용 스폰 오프셋

    // 성능 최적화
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 PreCalculatedLocationCount = 200; // 사전에 계산할 위치 수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableAsyncCleanup = true; // 비동기 메모리 정리 활성화여부

    // 보상 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System|Reward")
    float HealthRewardOnClear = 200.0f; // 체력 보상
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System|Reward")
    int32 AmmoRewardOnClear = 60; // 총알 보상

private:
    // 웨이브 상태 모니터링
    UPROPERTY(VisibleInstanceOnly, Category = "State|Wave", meta = (AllowPrivateAccess = "true"))
    int32 CurrentWaveIndex = 0; // 현재 웨이브 
    UPROPERTY(VisibleInstanceOnly, Category = "State|Wave", meta = (AllowPrivateAccess = "true"))
    bool bWaveInProgress = false; // 웨이브 진행 중 여부
    UPROPERTY(VisibleInstanceOnly, Category = "State|Wave", meta = (AllowPrivateAccess = "true"))
    bool bWaveSystemActive = false; // 웨이브 시스템 동작 여부
    UPROPERTY(VisibleInstanceOnly, Category = "State|Wave", meta = (AllowPrivateAccess = "true"))
    float SpawnLocationRefreshTime = 300.0f; // 스폰 위치 갱신 주기

    // 타이머
    FTimerHandle WaveStartTimer;
    FTimerHandle SpawnTimer;
    FTimerHandle CleanupTimer;
    FTimerHandle LocationRefreshTimer;
    FTimerHandle WaveSystemStartDelayTimer;

    // 현재 웨이브 스폰 진행 상황
    int32 CurrentSpawnEntryIndex = 0; // 현재 웨이브
    int32 CurrentSpawnCount = 0; // 현재스폰수

    // 스폰 된 적 관리 약참조로 안전하게 유지
    UPROPERTY()
    TArray<TWeakObjectPtr<class APawn>> SpawnedEnemies; // 스폰된 적들 배열

    int32 TotalEnemiesInWave = 0; // 웨이브의 총 적 수
    int32 EnemiesKilledInWave = 0; // 웨이브에서 사망 한 적 수

    // 스폰 위치 관리
    TArray<FVector> PreCalculatedSpawnLocations; // 사전에 계산할 스폰 위치 배열
    int32 CurrentLocationIndex = 0; // 현재 위치 목록
    bool bLocationCalculationComplete = false; // 사전계산 성공 여부

    // 웨이브 시스템 함수
    void StartWaveSystem(); // 첫 웨이브 시작 함수
    void StartWave(int32 WaveIndex); // 몇 번째 웨이브를 시작할지 정하는 함수
    void ProcessWaveSpawn(); // 웨이브 스폰 함수
    void SpawnEnemyFromEntry(const FWaveSpawnEntry& Entry); // BP로 설정한 엔트리의 적 소환 함수
    void CheckWaveCompletion(); // 웨이브 클리어 여부 체크 함수 
    void OnWaveCompleted(); // 웨이브 클리어시 함수
    void PrepareNextWave(); // 다음 웨이브 준비 함수
    UFUNCTION()
    void StartNextWave(); // 다음 웨이브 시작함수
    void SpawnEnemyAtLocation(TSubclassOf<class APawn> EnemyClass, const FVector& Location); // 실제 스폰 함수

    // 성능 최적화 함수
    void PreCalculateSpawnLocations(); // 스폰위치 사전 계산 함수
    void RefreshSpawnLocations(); // 스폰위치 초기화 함수
    void RefillSpawnLocationsAsync(); // 비동기로 스폰위치를 채우는 함수
    FVector GetNextSpawnLocation(); // 다음 스폰 위치를 가져오는 함수
    void PerformAsyncCleanup(); // 비동기 스폰위치 정리 함수
    void ForceCompleteMemoryCleanup(); // 강제 메모리 정리 함수

    // 유틸리티 함수
    void KillAllEnemies(); // 모든 적을 죽이는 함수
    bool FindRandomLocationOnNavMesh(FVector CenterLocation, float Radius, FVector& OutLocation); // 네브 메쉬 위의 핸덤 위치를 찾는 함수
    FVector GetPlayerLocation(); // 플레이어 위치를 가져오는 함수
    void StopAllSystems(); // 웨이브 시스템 정지 함수

};
