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

// 웨이브별 스폰 규칙 구조체
USTRUCT(BlueprintType)
struct FWaveSpawnEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Classes")
    TSubclassOf<class APawn> EnemyClass; // 스폰할 적 클래스

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Spawn")
    int32 SpawnCount = 1; // 스폰 개수

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Spawn")
    float SpawnDelay = 0.0f; // 그룹 시작 지연 시간

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Spawn")
    float SpawnInterval = 1.0f; // 개체별 스폰 주기

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
    float PrepareTime = 7.0f; // 웨이브 시작 전 준비시간

    FWaveConfiguration() : PrepareTime(7.0f) {}
};

UCLASS()
class LOCOMOTION_API AMainGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMainGameModeBase();

    // AI에서 접근할 수 있도록 살아있는 적 목록을 반환하는 getter 함수
    const TArray<TWeakObjectPtr<class APawn>>& GetSpawnedEnemies() const { return SpawnedEnemies; }

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void OnEnemyDestroyed(APawn* DestroyedEnemy); // 적 파괴시 호출되어 웨이브 진행 상태 갱신

    // 웨이브 시스템 제어
    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void SetWaveSystemEnabled(bool bEnabled); // 웨이브 시스템 활성화 여부 설정 함수

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
    void RestartWaveSystem(); // 웨이브 시스템을 초기상태로 되돌려 재시작 하는 함수

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void SkipToWave(int32 WaveIndex); // 특정 웨이브 번호로 바로 건너뛰는 함수

    // 적 정보
    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetActiveEnemyCount() const; // 현재 필드에 살아있는 적의 수 반환

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetEnemiesKilledInWave() const { return EnemiesKilledInWave; } // 현재 웨이브에서 처치한 적의 수 반환

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetTotalEnemiesInWave() const { return TotalEnemiesInWave; } // 현재 웨이브에서 스폰되어야 할 적의 수 반환

    UFUNCTION(BlueprintCallable, Category = "Enemy Info")
    int32 GetPreCalculatedLocationCount() const { return PreCalculatedSpawnLocations.Num(); } // 사전에 계산되어 저장된 스폰 위치들의 수 반환

    // 최고 기록을 체크하고 저장하는 함수
    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void CheckAndSaveBestWaveRecord(int32 LastClearedWaveIndex); // 최고 기록을 확인하고 세이브 데이터에 저장하는 함수

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    bool bWaveSystemEnabled = true; // 웨이브 시스템 활성화 여부

    // 6종 AI 클래스 등록
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
    float DefaultWaveClearDelay = 5.0f; // 기본 웨이브 클리어 대기시간

    // 스폰 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    bool bSpawnAroundPlayer = true; // 플레이어 주변 스폰 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    FVector SpawnCenterLocation = FVector::ZeroVector; // 고정 스폰 중심점
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    float DefaultSpawnRadius = 1000.0f; // 스폰 반경
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    float DroneSpawnHeightOffset = 200.0f; // 드론 스폰 높이 오프셋

    // 성능 최적화
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 PreCalculatedLocationCount = 200; // 사전계산 스폰 위치 수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableDeferredCleanup = true; // 지연 메모리 정리 활성화 여부

    // 보상 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System|Reward")
    float HealthRewardOnClear = 200.0f; // 체력 보상
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System|Reward")
    int32 AmmoRewardOnClear = 60; // 탄약 보상

private:
    // 웨이브 상태 모니터링
    UPROPERTY(VisibleInstanceOnly, Category = "State|Wave", meta = (AllowPrivateAccess = "true"))
    int32 CurrentWaveIndex = 0; // 현재 웨이브
    UPROPERTY(VisibleInstanceOnly, Category = "State|Wave", meta = (AllowPrivateAccess = "true"))
    bool bWaveInProgress = false; // 웨이브 진행 중 여부
    UPROPERTY(VisibleInstanceOnly, Category = "State|Wave", meta = (AllowPrivateAccess = "true"))
    bool bWaveSystemActive = false; // 웨이브 시스템 가동 여부
    UPROPERTY(VisibleInstanceOnly, Category = "State|Wave", meta = (AllowPrivateAccess = "true"))
    float SpawnLocationRefreshTime = 300.0f; // 스폰 위치 갱신 주기

    // 타이머
    FTimerHandle WaveStartTimer;
    FTimerHandle SpawnTimer;
    FTimerHandle CleanupTimer;
    FTimerHandle LocationRefreshTimer;
    FTimerHandle WaveSystemStartDelayTimer;

    // 현재 웨이브 스폰 진행 상황
    int32 CurrentSpawnEntryIndex = 0; // 현재 엔트리 인덱스
    int32 CurrentSpawnCount = 0; // 현재 스폰 수

    // 살아있는 적을 약참조로 관리
    UPROPERTY()
    TArray<TWeakObjectPtr<class APawn>> SpawnedEnemies; // 스폰된 적 배열

    int32 TotalEnemiesInWave = 0; // 웨이브의 총 적 수
    int32 EnemiesKilledInWave = 0; // 웨이브에서 처치한 적 수

    // 스폰 위치 캐싱
    TArray<FVector> PreCalculatedSpawnLocations; // 사전계산된 스폰 위치 배열
    int32 CurrentLocationIndex = 0; // 현재 위치 인덱스
    bool bLocationCalculationComplete = false; // 사전계산 완료 여부

    // 웨이브 시스템 함수
    void StartWaveSystem(); // 첫 웨이브 시작 함수
    void StartWave(int32 WaveIndex); // 몇 번째 웨이브를 시작할지 정하는 함수
    void ProcessWaveSpawn(); // 웨이브 스폰 함수
    void SpawnEnemyFromEntry(const FWaveSpawnEntry& Entry); // BP에 설정된 엔트리별 적 소환 함수
    void CheckWaveCompletion(); // 웨이브 클리어 조건 체크 함수
    void OnWaveCompleted(); // 웨이브 클리어시 함수
    void PrepareNextWave(); // 다음 웨이브 준비 함수
    UFUNCTION()
    void StartNextWave(); // 다음 웨이브 시작 함수
    void SpawnEnemyAtLocation(TSubclassOf<class APawn> EnemyClass, const FVector& Location); // 적 스폰 함수

    // 성능 최적화 함수
    void PreCalculateSpawnLocations(); // 스폰위치 사전 계산 함수
    void RefreshSpawnLocations(); // 스폰위치 갱신 함수
    void RefillSpawnLocationsPerFrame(); // 프레임당 1개씩 스폰위치를 채우는 함수
    FVector GetNextSpawnLocation(); // 다음 스폰 위치를 가져오는 함수
    void PerformDeferredCleanup(); // 지연 메모리 정리 함수
    void ForceCompleteMemoryCleanup(); // 강제 메모리 정리 함수

    // 스폰 위치 분할 갱신 (게임스레드)
	FTimerHandle RefillTimer; // 스폰 위치 분할 갱신 타이머 핸들
	int32 RefillCount = 0; // 현재까지 채운 위치 수
	int32 RefillTargetCount = 50; // 한 번에 채울 위치 수
	FVector RefillCenter; // 스폰 위치를 채울 중심점
	float RefillRadius = 0.0f; // 스폰 위치를 채울 반경
    void RefillOneSpawnLocation(); // 프레임당 1개씩 NavMesh 쿼리 실행

    void KillAllEnemies(); // 모든 적을 죽이는 함수
    bool FindRandomLocationOnNavMesh(FVector CenterLocation, float Radius, FVector& OutLocation); // 내브 메시 위의 랜덤 위치를 찾는 함수
    FVector GetPlayerLocation(); // 플레이어 위치를 가져오는 함수
    void StopAllSystems(); // 웨이브 시스템 정지 함수

};
