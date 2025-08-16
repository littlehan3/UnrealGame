#include "EnemyGuardianShield.h"
#include "EnemyGuardian.h"

AEnemyGuardianShield::AEnemyGuardianShield()
{
	PrimaryActorTick.bCanEverTick = true;

	// ���� �޽� �ʱ�ȭ �� RootComponent�� ����
	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
	RootComponent = ShieldMesh;

	ShieldChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldChildMesh"));
	ShieldChildMesh->SetupAttachment(ShieldMesh);  // ��Ʈ ������Ʈ�� �ڽ����� ���̰� �ʱ�ȭ

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

