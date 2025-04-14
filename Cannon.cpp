#include "Cannon.h"
#include "Components/StaticMeshComponent.h"
#include "AimSkill2Projectile.h"
#include "Kismet/GameplayStatics.h"

ACannon::ACannon()
{
	PrimaryActorTick.bCanEverTick = false;

	CannonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CannonMesh"));
	RootComponent = CannonMesh;

	CannonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CannonMesh->SetGenerateOverlapEvents(false);
}

void ACannon::BeginPlay()
{
	Super::BeginPlay();
}

void ACannon::SetShooter(AActor* InShooter)
{
	Shooter = InShooter;
}

void ACannon::SetProjectileClass(TSubclassOf<AAimSkill2Projectile> InClass)
{
	ProjectileClass = InClass;
}

void ACannon::FireProjectile()
{
    UE_LOG(LogTemp, Warning, TEXT("FireProjectile CALLED"));

    if (!ProjectileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("ProjectileClass is NULL!"));
        return;
    }

    if (!Shooter)
    {
        UE_LOG(LogTemp, Error, TEXT("Shooter is NULL!"));
        return;
    }

    // ĳ�� ���� �������� ������ �������� �߻�
    FVector ShootDirection = -CannonMesh->GetForwardVector();
    FVector SpawnLocation = GetActorLocation() + ShootDirection * 80.f; // Cannon ���� ��ü ��ġ
    FRotator SpawnRotation = ShootDirection.Rotation();

    // ����� ǥ��
    DrawDebugSphere(GetWorld(), SpawnLocation, 15.f, 12, FColor::Green, false, 2.0f);
    DrawDebugDirectionalArrow(GetWorld(), SpawnLocation, SpawnLocation + ShootDirection * 200.f,
        120.f, FColor::Red, false, 2.0f, 0, 3.0f);

    FActorSpawnParameters Params;
    Params.Owner = Shooter;
    Params.Instigator = Cast<APawn>(Shooter);

    AAimSkill2Projectile* Projectile = GetWorld()->SpawnActor<AAimSkill2Projectile>(
        ProjectileClass, SpawnLocation, SpawnRotation, Params);

    if (Projectile)
    {
        UE_LOG(LogTemp, Warning, TEXT("Projectile Spawned at %s"), *SpawnLocation.ToString());
        Projectile->SetShooter(Shooter);
        Projectile->FireInDirection(ShootDirection);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn projectile!"));
    }
}