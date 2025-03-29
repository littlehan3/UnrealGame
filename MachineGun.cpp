#include "MachineGun.h"
#include "Components/StaticMeshComponent.h"

AMachineGun::AMachineGun()
{
    PrimaryActorTick.bCanEverTick = true;

    GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
    RootComponent = GunMesh; // �޽� ���� �� ��Ʈ������Ʈ ����

    GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // �浹 ��Ȱ��ȭ BP�� �����ϰ� ����
    GunMesh->SetGenerateOverlapEvents(false); // ������ �̺�Ʈ ��Ȱ��ȭ
}

void AMachineGun::BeginPlay()
{
    Super::BeginPlay();
}

void AMachineGun::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
