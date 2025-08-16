#include "EnemyGuardian.h"
#include "EnemyGuardianShield.h"

AEnemyGuardian::AEnemyGuardian()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AEnemyGuardian::BeginPlay()
{
	Super::BeginPlay();
	
    UE_LOG(LogTemp, Warning, TEXT("=== EnemyGuardian BeginPlay Started ==="));

    // ���� Ŭ������ �����Ǿ� �ִٸ� ���� ���� �� ����
    if (ShieldClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShieldClass is valid"));

        EquippedShield = GetWorld()->SpawnActor<AEnemyGuardianShield>(ShieldClass);
        if (EquippedShield)
        {
            EquippedShield->SetOwner(this);
            USkeletalMeshComponent* MeshComp = GetMesh();
            if (MeshComp)
            {
                FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
                EquippedShield->AttachToComponent(MeshComp, AttachmentRules, FName("EnemyGuardianShieldSocket"));
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("Shield Attached"));
    }
}

void AEnemyGuardian::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemyGuardian::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

