#include "EnemyKatana.h"

// Sets default values
AEnemyKatana::AEnemyKatana()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // īŸ�� �޽� �ʱ�ȭ �� RootComponent�� ����
    KatanaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaMesh"));
    RootComponent = KatanaMesh;

    KatanaMesh->SetSimulatePhysics(false); // ������ �� ���� �ùķ��̼� ��Ȱ��ȭ
    KatanaMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // �浹 Ȱ��ȭ
}

// Called when the game starts or when spawned
void AEnemyKatana::BeginPlay()
{
    Super::BeginPlay();
}

// Called every frame
void AEnemyKatana::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AEnemyKatana::HideKatana()
{
    SetActorHiddenInGame(true);  // ������ ����
    SetActorEnableCollision(false); // �浹 ����
    SetActorTickEnabled(false); // Tick ��Ȱ��ȭ

    // 2�� �� ���� (�޸𸮿��� ������ �����ϱ� ���� ����) �ڵ� ������ �÷��� ����
    SetLifeSpan(2.0f);

    UE_LOG(LogTemp, Warning, TEXT("Katana %s is now hidden and will be destroyed soon!"), *GetName());
}