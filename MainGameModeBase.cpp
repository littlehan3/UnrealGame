#include "MainGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "MainCharacter.h"
#include "Enemy.h"
#include "BossEnemy.h"
#include "EnemyDog.h"
#include "EnemyDrone.h"
#include "EnemyGuardian.h"
#include "EnemyShooter.h"
#include "WaveRecordSaveGame.h"

AMainGameModeBase::AMainGameModeBase()
{
    PrimaryActorTick.bCanEverTick = false; // Tick 비활성화

    // 웨이브 시스템 기본값 초기화
    bWaveSystemEnabled = true;
    DefaultWaveClearDelay = 5.0f; 
    CurrentWaveIndex = 0; 
    bWaveInProgress = false;
    bWaveSystemActive = false;
    bEnableDeferredCleanup = true;
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

    UWorld* World = GetWorld(); // 월드 가져옴
    if (!World) return; // 유효성 검사

    // AI 클래스 등록 상태 확인
    UE_LOG(LogTemp, Error, TEXT("=== AI CLASSES REGISTRATION CHECK ==="));
    UE_LOG(LogTemp, Error, TEXT("EnemyType: %s"), EnemyType ? *EnemyType->GetName() : TEXT("NOT SET"));
    UE_LOG(LogTemp, Error, TEXT("BossEnemyType: %s"), BossEnemyType ? *BossEnemyType->GetName() : TEXT("NOT SET"));
    UE_LOG(LogTemp, Error, TEXT("EnemyDogType: %s"), EnemyDogType ? *EnemyDogType->GetName() : TEXT("NOT SET"));
    UE_LOG(LogTemp, Error, TEXT("EnemyDroneType: %s"), EnemyDroneType ? *EnemyDroneType->GetName() : TEXT("NOT SET"));
    UE_LOG(LogTemp, Error, TEXT("EnemyGuardianType: %s"), EnemyGuardianType ? *EnemyGuardianType->GetName() : TEXT("NOT SET"));
    UE_LOG(LogTemp, Error, TEXT("EnemyShooterType: %s"), EnemyShooterType ? *EnemyShooterType->GetName() : TEXT("NOT SET"));

    // SetTimerForNextTick으로 엔진 초기화 직후가 아닌 다음 틱에 동작
    World->GetTimerManager().SetTimerForNextTick([this]() 
        {
            TWeakObjectPtr<AMainGameModeBase> WeakThis(this); // 모드베이스 약참조
            if (!WeakThis.IsValid()) return; // 유효성 검사

            WeakThis->PreCalculateSpawnLocations(); // 초기 위치 계산 함수를 미리 호출

            // 주기적 위치 갱신 타이머
            GetWorldTimerManager().SetTimer(LocationRefreshTimer, [WeakThis]()
                {
                    if (WeakThis.IsValid())  // 유효성 검사
                    {
                        WeakThis->RefreshSpawnLocations(); // 스폰 위치 초기화
                    }
                }, SpawnLocationRefreshTime, true);  // 스폰 위치 초기화 주기로, 반복

            if (bWaveSystemEnabled) // 웨이브 시스템이 활성화 상태라면
            {
                // TWeakObjectPtr(약참조)를 사용하여 타이머 실행 시점의 객체 유효성 보장
                GetWorldTimerManager().SetTimer(WaveSystemStartDelayTimer, [WeakThis]()
                    {
                        if (WeakThis.IsValid())  // 유효성 검사
                        {
                            WeakThis->StartWaveSystem(); // 웨이브 시스템 시작
                        }
                    }, DefaultWaveClearDelay, false); // 일정시간 지연 후 시작, 단발성
            }
        }
    );
}

// 웨이브 시스템
void AMainGameModeBase::StartWaveSystem()
{ 
    if (WaveConfigurations.Num() == 0) // 설정된 웨이브가 하나도 없다면
    {
        UE_LOG(LogTemp, Error, TEXT("No wave configurations found! Please set up waves in Blueprint."));
        return; // 리턴
    }

    UE_LOG(LogTemp, Warning, TEXT("Wave System Started - Total Waves: %d"), WaveConfigurations.Num());

    bWaveSystemActive = true; // 웨이브 시스템 활성화 
    CurrentWaveIndex = 0; // 현재 웨이브 초기화
    StartWave(0); // 첫번째 웨이브 부터 순차적으로 시작
}

void AMainGameModeBase::StartWave(int32 WaveIndex)
{
    UWorld* World = GetWorld(); // 월드 가져옴
    if (!World) return; // 유효성 검사
    if (!WaveConfigurations.IsValidIndex(WaveIndex)) return; // 웨이브 인덱스 유효성 검사

    if (WaveIndex >= WaveConfigurations.Num()) // 모든 웨이브가 끝났다면
    {
        UE_LOG(LogTemp, Warning, TEXT("All waves completed! Wave system finished."));
        bWaveSystemActive = false; // 웨이브 시스템 비활성화
        return; // 리턴
    }
    // BP에서 설정한 전체 웨이브 배열 중 지금 시작할 순서에 해당하는 데이터를 &참조 후 가져옴
    // 데이터를 통째로 복사하는 오버헤드를 줄이기 위해 &참조 사용
    const FWaveConfiguration& WaveConfig = WaveConfigurations[WaveIndex];
    CurrentWaveIndex = WaveIndex; // 현재웨이브 번호를 시작하는 번호로 갱신

    // 웨이브 시작 디버깅
    UE_LOG(LogTemp, Error, TEXT("=== WAVE %d CONFIGURATION DEBUG ==="), WaveIndex + 1); // 현재 웨이브
    UE_LOG(LogTemp, Error, TEXT("Wave Name: %s"), *WaveConfig.WaveName); // 웨이브 명
    UE_LOG(LogTemp, Error, TEXT("Spawn Entries Count: %d"), WaveConfig.SpawnEntries.Num()); // 스폰 규칙 (엔트리) 갯수

    // 웨이브에 포함된 각 스폰 엔트리의 세부정보를 순회 하여 출력
    for (int32 i = 0; i < WaveConfig.SpawnEntries.Num(); i++)
    {
        const FWaveSpawnEntry& Entry = WaveConfig.SpawnEntries[i]; // i 번째 스폰 항목의 참조를 가져옴
        UE_LOG(LogTemp, Error, TEXT("Entry [%d]:"), i); // 몇 번째 스폰 규칙인지
        UE_LOG(LogTemp, Error, TEXT("EnemyClass: %s"), Entry.EnemyClass ? *Entry.EnemyClass->GetName() : TEXT("NULL")); // 스폰할 적의 클래스가 할당되어 있는지
        UE_LOG(LogTemp, Error, TEXT("SpawnCount: %d"), Entry.SpawnCount); // 스폰 될 적의 수
        UE_LOG(LogTemp, Error, TEXT("SpawnDelay: %f"), Entry.SpawnDelay); // 스폰 지연 시간
        UE_LOG(LogTemp, Error, TEXT("SpawnInterval: %f"), Entry.SpawnInterval); // 스폰 간격
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
        if (Entry.EnemyClass) // 엔트리에 적 클래스가 있다면
        {
            TotalEnemiesInWave += Entry.SpawnCount; // 총 스폰량을 계산
        }
    }
    UE_LOG(LogTemp, Error, TEXT("Total enemies to spawn in wave: %d"), TotalEnemiesInWave);
    UE_LOG(LogTemp, Error, TEXT("Prepare time: %f seconds"), WaveConfig.PrepareTime);

    // 메인캐릭터 선언 후 캐스팅
    if (AMainCharacter* MainChar = Cast<AMainCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
    {
        MainChar->PlayWavePrepareSound(); // 웨이브 준비 사운드 재생
    }

    TWeakObjectPtr<AMainGameModeBase> WeakThis(this); // 약참조 선언
    if (WaveConfig.PrepareTime > 0.0f) // 준비 시간 후
    {
        World->GetTimerManager().SetTimer(WaveStartTimer, [WeakThis]()
            { 
                if (WeakThis.IsValid()) // 유효성 검사
                {
                    WeakThis->ProcessWaveSpawn(); // 스폰 시작
                }
            }, WaveConfig.PrepareTime, false);
    }
    else // 준비시간이 없다면
    {
        ProcessWaveSpawn(); // 스폰 시작
    }
}

void AMainGameModeBase::ProcessWaveSpawn()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UE_LOG(LogTemp, Error, TEXT("=== ProcessWaveSpawn DEBUG ==="));
    UE_LOG(LogTemp, Error, TEXT("CurrentSpawnEntryIndex: %d / %d"), CurrentSpawnEntryIndex, WaveConfigurations[CurrentWaveIndex].SpawnEntries.Num());
    UE_LOG(LogTemp, Error, TEXT("CurrentSpawnCount: %d"), CurrentSpawnCount);

    if (!bWaveSystemActive || CurrentWaveIndex >= WaveConfigurations.Num()) // 시스템이 비활성화거나 인덱스가 비정상적이라면 
    {
        UE_LOG(LogTemp, Error, TEXT("Wave system inactive or invalid wave index"));
        return; // 중단
    }

    const FWaveConfiguration& WaveConfig = WaveConfigurations[CurrentWaveIndex];

    // 현재 웨이브의 모든 엔트리를 순회했는지 확인
    if (CurrentSpawnEntryIndex < WaveConfig.SpawnEntries.Num())
    {
        const FWaveSpawnEntry& CurrentEntry = WaveConfig.SpawnEntries[CurrentSpawnEntryIndex];

        UE_LOG(LogTemp, Error, TEXT("Processing entry %d: %s (Count: %d/%d)"),
            CurrentSpawnEntryIndex,
            CurrentEntry.EnemyClass ? *CurrentEntry.EnemyClass->GetName() : TEXT("NULL"),
            CurrentSpawnCount,
            CurrentEntry.SpawnCount);

        // 현재 항목의 스폰 개수가 남았는지 확인
        if (CurrentEntry.EnemyClass && CurrentSpawnCount < CurrentEntry.SpawnCount)
        {
            SpawnEnemyFromEntry(CurrentEntry); // 현재 엔트리의 적 스폰
            CurrentSpawnCount++; // 스폰 카운트 증가

            UE_LOG(LogTemp, Error, TEXT("Spawned %d/%d of current entry"), CurrentSpawnCount, CurrentEntry.SpawnCount);

            // 약참조 선언: 타이머 실행 시점에 GameMode가 유효한지 체크하기 위함
            TWeakObjectPtr<AMainGameModeBase> WeakThis(this);

            // 같은 종류의 적을 더 스폰해야 하는 경우
            if (CurrentSpawnCount < CurrentEntry.SpawnCount)
            {
                GetWorldTimerManager().SetTimer(SpawnTimer, [WeakThis]()
                    {
                        if (WeakThis.IsValid()) // 타이머 실행 시점에 유효성 검사
                        {
                            WeakThis->ProcessWaveSpawn(); // 스폰
                        }
                    }, CurrentEntry.SpawnInterval, false); // 엔트리에 설정된 스폰 간격만큼, 단발성
                return;
            }
            // 현재 항목 스폰이 끝나고 다음 항목으로 넘어가는 경우
            else
            {
                CurrentSpawnEntryIndex++; // 스폰 엔트리 증가
                CurrentSpawnCount = 0; // 현재 스폰 수 초기화

                if (CurrentSpawnEntryIndex < WaveConfig.SpawnEntries.Num()) 
                {
                    const FWaveSpawnEntry& NextEntry = WaveConfig.SpawnEntries[CurrentSpawnEntryIndex];

                    if (NextEntry.SpawnDelay <= 0.0f)
                    {
                        ProcessWaveSpawn(); // 지연 시간이 없으면 즉시 다음 항목 처리
                    }
                    else
                    {
                        // 다음 항목 시작 전 대기 시간을 타이머로 처리 (약참조 적용)
                        World->GetTimerManager().SetTimer(SpawnTimer, [WeakThis]()
                            {
                                if (WeakThis.IsValid())
                                {
                                    WeakThis->ProcessWaveSpawn();
                                }
                            }, NextEntry.SpawnDelay, false);
                    }
                    return;
                }
            }
        }
        else
        {
            CurrentSpawnEntryIndex++; // 다음 엔트리
            CurrentSpawnCount = 0; // 현재 스폰 수 초기화
            ProcessWaveSpawn(); // 스폰
            return;
        }
    }
    CheckWaveCompletion(); // 모든 스폰이 끝나면 웨이브 클리어 체크 함수 호출
}

void AMainGameModeBase::SpawnEnemyFromEntry(const FWaveSpawnEntry& Entry)
{
    if (!Entry.EnemyClass) return;

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

    FVector SpawnLocation = GetNextSpawnLocation(); // 사전계산된 좌표목록을 불러오고
    SpawnEnemyAtLocation(Entry.EnemyClass, SpawnLocation); // 불러온 위치에 스폰 함수 호출
}

void AMainGameModeBase::OnWaveCompleted()
{
    if (!bWaveInProgress) return;
    bWaveInProgress = false;

    UE_LOG(LogTemp, Warning, TEXT("Wave %d completed! Enemies killed: %d/%d"),
        CurrentWaveIndex + 1, EnemiesKilledInWave, TotalEnemiesInWave);

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    AMainCharacter* MainCharacter = Cast<AMainCharacter>(PlayerCharacter);

    if (MainCharacter) // 메인캐릭터가 존재한다면
    {
        MainCharacter->GiveReward(HealthRewardOnClear, AmmoRewardOnClear); // 메인 캐릭터의 보상지급 함수 호출
        UE_LOG(LogTemp, Warning, TEXT("Giving wave clear reward: Health=%f, Ammo=%d"), HealthRewardOnClear, AmmoRewardOnClear);
    }

    PrepareNextWave(); // 다음 웨이브 준비 함수 호출
}

void AMainGameModeBase::PrepareNextWave()
{
    UWorld* World = GetWorld();
    if (!World) return;

    TWeakObjectPtr<AMainGameModeBase> WeakThis(this); // 약참조 선언

    World->GetTimerManager().SetTimer(
        WaveStartTimer,
        [WeakThis]()
        {
            if (WeakThis.IsValid()) // 유효성검사
            {
                WeakThis->StartNextWave(); // 다음 웨이브 시작
            }
        },
        DefaultWaveClearDelay, false // 웨이브 클리어 딜레이 시간 후에, 단발성
    );
}

void AMainGameModeBase::StartNextWave()
{
    int32 NextWaveIndex = CurrentWaveIndex + 1; // 현재 웨이브 인덱스 에 1 추가

    if (NextWaveIndex < WaveConfigurations.Num()) // 다음 웨이브가 총 설정된 웨이브 수 보다 적다면
    {
        StartWave(NextWaveIndex); // 다음 웨이브 실행
    }
    else // 아니라면
    {
        UE_LOG(LogTemp, Warning, TEXT("All waves completed! Game finished."));
        bWaveSystemActive = false; // 웨이브 비활성화
    }
}

void AMainGameModeBase::SpawnEnemyAtLocation(TSubclassOf<class APawn> EnemyClass, const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (!EnemyClass)
    {
        UE_LOG(LogTemp, Error, TEXT("EnemyClass is NULL! Aborting spawn."));
        return;
    }

    // 드론의 경우 스폰 위치 높이 조정
    FVector AdjustedLocation = Location; // 조정될 위치
    if (EnemyClass->IsChildOf<AEnemyDrone>()) // 드론 타입 클래스 일 경우
    {
        AdjustedLocation.Z += DroneSpawnHeightOffset; // 드론스폰오프셋값 만큼 높게 소환
        UE_LOG(LogTemp, Error, TEXT("Drone detected - Adjusted spawn height by +%f"), DroneSpawnHeightOffset);
    }

    FActorSpawnParameters SpawnParams;
    // 스폰위치에 장애물이 있어도 AdjustIfPossibleButAlwaysSpawn 으로 최대한 위치 조정해서 스폰 보장
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnParams.Name = NAME_None; // 언리얼이 자동으로 고유한 이름 생성

    if (APawn* SpawnedEnemy = World->SpawnActor<APawn>(EnemyClass, AdjustedLocation, FRotator::ZeroRotator, SpawnParams))
    {
        SpawnedEnemies.Add(SpawnedEnemy); // 생성된 적의 참조를 배열에 추가
    }
}

void AMainGameModeBase::OnEnemyDestroyed(APawn* DestroyedEnemy)
{
    if (!DestroyedEnemy) return;

    // 사망한 적의 참조를 배열에서 찾아
    SpawnedEnemies.RemoveAll([DestroyedEnemy](const TWeakObjectPtr<APawn>& EnemyPtr)
        {
            return EnemyPtr.Get() == DestroyedEnemy; // 즉시 제거
        });

    if (bWaveSystemEnabled && bWaveInProgress) // 웨이브가 진행중이라면
    {
        EnemiesKilledInWave++; // 처치 수 갱신
        CheckWaveCompletion(); // 웨이브 검사 함수 호출
    }
}

void AMainGameModeBase::PerformDeferredCleanup()
{
    UE_LOG(LogTemp, Log, TEXT("Starting deferred cleanup for Wave %d"), CurrentWaveIndex + 1);

    // 0.1초 후 게임스레드에서 정리 실행 (기존 AsyncTask + Sleep(0.1f) + GameThread 패턴을 타이머로 대체)
    TWeakObjectPtr<AMainGameModeBase> WeakThis(this);
    GetWorldTimerManager().SetTimer(CleanupTimer, [WeakThis]()
        {
            if (WeakThis.IsValid())
            {
                // 더이상 유효하지 않은 적(이미 파괴된 적)의 참조를 배열에서 모두 제거
                WeakThis->SpawnedEnemies.RemoveAll([](const TWeakObjectPtr<APawn>& EnemyPtr)
                    {
                        return !EnemyPtr.IsValid();
                    });

                // 배열의 실제 메모리 할당 크기를 현재 데이터 개수에 딱 맞게 줄여 메모리 낭비 방지
                WeakThis->SpawnedEnemies.Shrink();

                UE_LOG(LogTemp, Warning, TEXT("Deferred cleanup completed. Active enemies: %d"), WeakThis->SpawnedEnemies.Num());
            }
        }, 0.1f, false); // 0.1초 후 1회 실행
}

void AMainGameModeBase::ForceCompleteMemoryCleanup()
{
    UE_LOG(LogTemp, Warning, TEXT("Performing complete memory cleanup"));

    // 모든 적 강제 제거
    KillAllEnemies();

    // 배열 초기화
    SpawnedEnemies.Empty();
    SpawnedEnemies.Shrink();

    // 타이머 정리
    GetWorldTimerManager().ClearTimer(WaveStartTimer);
    GetWorldTimerManager().ClearTimer(SpawnTimer);
    GetWorldTimerManager().ClearTimer(CleanupTimer);
    
    // 잔여 메모리 정리
    if (GEngine)
    {
        GEngine->ForceGarbageCollection(true); // 강제 GC 수행
        UE_LOG(LogTemp, Warning, TEXT("Forced garbage collection completed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Complete memory cleanup finished"));
}

void AMainGameModeBase::SetWaveSystemEnabled(bool bEnabled)
{
    if (bWaveSystemEnabled == bEnabled) return;

    bWaveSystemEnabled = bEnabled;
    StopAllSystems(); // 상태 변경 시 기존 타이머 초기화

    if (bWaveSystemEnabled)
    {
        StartWaveSystem(); // 웨이브 시작
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Wave system disabled"));
    }
}

FString AMainGameModeBase::GetCurrentWaveName() const
{
    //UI 표시를 위한 현재 진행중인 웨이브 고유 이름 반환
    if (CurrentWaveIndex < WaveConfigurations.Num())
    {
        return WaveConfigurations[CurrentWaveIndex].WaveName;
    }
    return TEXT("Unknown Wave");
}

int32 AMainGameModeBase::GetActiveEnemyCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<APawn>& EnemyPtr : SpawnedEnemies)  // 월드에 살아있는 적들
    {
        if (EnemyPtr.IsValid()) // 유효하다면
        {
            Count++; // 카운트
        }
    }

    return Count;
}

void AMainGameModeBase::CheckAndSaveBestWaveRecord(int32 LastClearedWaveIndex)
{
    // 클리어한 웨이브가 기존 최고 기록보다 높을 경우 세이브 파일에 기록을 갱신
    // 1. 저장 객체 로드 또는 생성
    UWaveRecordSaveGame* SaveGameInstance = nullptr;

    // 기존 데이터 로드
    if (UGameplayStatics::DoesSaveGameExist(UWaveRecordSaveGame::SaveSlotName, UWaveRecordSaveGame::UserIndex))
    {
        SaveGameInstance = Cast<UWaveRecordSaveGame>(UGameplayStatics::LoadGameFromSlot(UWaveRecordSaveGame::SaveSlotName, UWaveRecordSaveGame::UserIndex));
    }

    if (!SaveGameInstance) // 없다면
    {
        SaveGameInstance = Cast<UWaveRecordSaveGame>(UGameplayStatics::CreateSaveGameObject(UWaveRecordSaveGame::StaticClass())); // 생성
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
        UE_LOG(LogTemp, Warning, TEXT("Current score (Wave %d) is not a new best record (Best is Wave %d)."),
            LastClearedWaveIndex + 1, SaveGameInstance->BestClearedWaveIndex + 1);
    }
}

void AMainGameModeBase::RestartWaveSystem()
{
    StopAllSystems(); // 시스템 정지

    if (bWaveSystemEnabled) // 웨이브가 활성화 되어있다면
    {
        StartWaveSystem(); // 다시 0번 웨이브 부터 시작
    }
}

void AMainGameModeBase::SkipToWave(int32 WaveIndex)
{
    // 테스트용 함수
    // 특정 웨이브로 점프하기 위해 현재 상태를 강제 종료하고 지정된 웨이브 시작
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
    GetWorldTimerManager().ClearTimer(RefillTimer);
}

void AMainGameModeBase::PreCalculateSpawnLocations()
{
    // NavMesh의 랜덤 좌표 탐색은 무겁기에 게임 시작 시 대량으로 수행해서 런타임 중에는 연산 없이 배열에서 꺼내 쓰도록 함
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

    bLocationCalculationComplete = true;
    UE_LOG(LogTemp, Warning, TEXT("Pre-calculated %d spawn locations completed"), PreCalculatedSpawnLocations.Num());
}

void AMainGameModeBase::RefreshSpawnLocations()
{
    UE_LOG(LogTemp, Warning, TEXT("Refreshing spawn locations..."));
    RefillSpawnLocationsPerFrame();
}

void AMainGameModeBase::RefillSpawnLocationsPerFrame()
{
    if (!bLocationCalculationComplete) return;

    // 게임스레드에서 안전하게 값 캐싱
    RefillCenter = bSpawnAroundPlayer ? GetPlayerLocation() : SpawnCenterLocation;
    RefillRadius = DefaultSpawnRadius;
    RefillCount = 0;

    // 기존 타이머가 있으면 정리
    GetWorldTimerManager().ClearTimer(RefillTimer);

    // 프레임당 1개씩 NavMesh 쿼리 실행 (게임스레드에서 안전)
    TWeakObjectPtr<AMainGameModeBase> WeakThis(this);
    GetWorldTimerManager().SetTimer(RefillTimer, [WeakThis]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->RefillOneSpawnLocation();
            }
        }, 0.0f, true); // 0초 간격 = 매 프레임, 반복
}

void AMainGameModeBase::RefillOneSpawnLocation()
{
    if (RefillCount >= RefillTargetCount)
    {
        GetWorldTimerManager().ClearTimer(RefillTimer);
        UE_LOG(LogTemp, Warning, TEXT("Spawn location refill completed: %d locations updated"), RefillCount);
        return;
    }

    FVector SpawnLocation;
    if (FindRandomLocationOnNavMesh(RefillCenter, RefillRadius, SpawnLocation))
    {
        if (PreCalculatedSpawnLocations.Num() > 0)
        {
            int32 ReplaceIndex = FMath::RandRange(0, PreCalculatedSpawnLocations.Num() - 1);
            PreCalculatedSpawnLocations[ReplaceIndex] = SpawnLocation;
        }
    }

    RefillCount++;
}

FVector AMainGameModeBase::GetNextSpawnLocation()
{
    // 미리 계산된 배열에서 인덱스를 순환하며
    if (PreCalculatedSpawnLocations.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No pre-calculated spawn locations available!"));
        return GetPlayerLocation(); // 좌표를 가져옴
    }

    FVector SpawnLocation = PreCalculatedSpawnLocations[CurrentLocationIndex];
    CurrentLocationIndex = (CurrentLocationIndex + 1) % PreCalculatedSpawnLocations.Num();

    return SpawnLocation;
}

void AMainGameModeBase::KillAllEnemies()
{
    int32 KilledCount = 0;

    // 순회 중 배열 변형 방지를 위해 약참조로 복사본 생성
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
    SpawnedEnemies.Empty(); // 배열을 비워 GC를 도움
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
    if (IsValid(PlayerCharacter))
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
        UE_LOG(LogTemp, Warning, TEXT("Wave still in progress - %d enemies remaining"), ActiveEnemyCount);
    }
}
