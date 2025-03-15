#include "EnemyKatana.h"

// Sets default values
AEnemyKatana::AEnemyKatana()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // 카타나 메시 초기화 및 RootComponent로 설정
    KatanaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaMesh"));
    RootComponent = KatanaMesh;

    KatanaMesh->SetSimulatePhysics(false); // 시작할 때 물리 시뮬레이션 비활성화
    KatanaMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 충돌 활성화
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
    SetActorHiddenInGame(true);  // 렌더링 숨김
    SetActorEnableCollision(false); // 충돌 제거
    SetActorTickEnabled(false); // Tick 비활성화

    // 2초 후 삭제 (메모리에서 완적히 삭제하기 위한 보완) 자동 가비지 컬렉션 유도
    SetLifeSpan(2.0f);

    UE_LOG(LogTemp, Warning, TEXT("Katana %s is now hidden and will be destroyed soon!"), *GetName());
}