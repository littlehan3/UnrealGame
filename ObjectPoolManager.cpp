#include "ObjectPoolManager.h"

AObjectPoolManager::AObjectPoolManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AObjectPoolManager::BeginPlay()
{
    Super::BeginPlay();

    // ���� ���� �� ����� �ڵ�
}

void AObjectPoolManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // �� �����Ӹ��� ����� �ڵ�
}

void AObjectPoolManager::InitializePool()
{
    // Ǯ �ʱ�ȭ �ڵ� (����� �� �Լ�)
    UE_LOG(LogTemp, Log, TEXT("Pool initialized"));
}

void AObjectPoolManager::CleanupPool()
{
    // Ǯ ���� �ڵ� (����� �� �Լ�)
    UE_LOG(LogTemp, Log, TEXT("Pool cleaned up"));
}
