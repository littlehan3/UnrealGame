#include "EnemyGuardianShield.h"
#include "EnemyGuardian.h"

AEnemyGuardianShield::AEnemyGuardianShield()
{
	PrimaryActorTick.bCanEverTick = true;

	// 쉴드 메시 초기화 및 RootComponent로 설정
	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
	RootComponent = ShieldMesh;

	ShieldChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldChildMesh"));
	ShieldChildMesh->SetupAttachment(ShieldMesh);  // 루트 컴포넌트의 자식으로 붙이고 초기화

}

void AEnemyGuardianShield::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnemyGuardianShield::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

