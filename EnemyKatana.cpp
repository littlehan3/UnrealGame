#include "EnemyKatana.h"

// Sets default values
AEnemyKatana::AEnemyKatana()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
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
