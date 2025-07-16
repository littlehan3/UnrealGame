#include "ObjectPoolManager.h"

AObjectPoolManager::AObjectPoolManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AObjectPoolManager::BeginPlay()
{
    Super::BeginPlay();

    // 게임 시작 시 실행될 코드
}

void AObjectPoolManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 매 프레임마다 실행될 코드
}

void AObjectPoolManager::InitializePool()
{
    // 풀 초기화 코드 (현재는 빈 함수)
    UE_LOG(LogTemp, Log, TEXT("Pool initialized"));
}

void AObjectPoolManager::CleanupPool()
{
    // 풀 정리 코드 (현재는 빈 함수)
    UE_LOG(LogTemp, Log, TEXT("Pool cleaned up"));
}
