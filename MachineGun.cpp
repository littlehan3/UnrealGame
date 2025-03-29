#include "MachineGun.h"
#include "Components/StaticMeshComponent.h"

AMachineGun::AMachineGun()
{
    PrimaryActorTick.bCanEverTick = true;

    GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
    RootComponent = GunMesh; // 메쉬 생성 및 루트컴포넌트 설정

    GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // 충돌 비활성화 BP도 동일하게 설정
    GunMesh->SetGenerateOverlapEvents(false); // 오버렙 이벤트 비활성화
}

void AMachineGun::BeginPlay()
{
    Super::BeginPlay();
}

void AMachineGun::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
