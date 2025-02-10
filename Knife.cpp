#include "Knife.h"
#include "Components/StaticMeshComponent.h"

AKnife::AKnife()
{
    PrimaryActorTick.bCanEverTick = true;

    // StaticMeshComponent 생성 및 RootComponent로 설정
    KnifeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KnifeMesh"));
    RootComponent = KnifeMesh;

    // 기본적으로 왼손 나이프로 설정
    KnifeType = EKnifeType::Left;
}

void AKnife::BeginPlay()
{
    Super::BeginPlay();

    if (KnifeType == EKnifeType::Left)
    {
        UE_LOG(LogTemp, Warning, TEXT("Left Knife spawned"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Right Knife spawned"));
    }
}

void AKnife::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// 나이프 초기화 함수
void AKnife::InitializeKnife(EKnifeType NewType)
{
    KnifeType = NewType;

    if (KnifeType == EKnifeType::Left)
    {
        UE_LOG(LogTemp, Warning, TEXT("Left Knife Initialized!"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Right Knife Initialized!"));
    }
}
