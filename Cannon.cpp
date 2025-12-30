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

    if (!ProjectileClass || !Shooter)
    {
        UE_LOG(LogTemp, Error, TEXT("Missing projectile class or shooter."));
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    FVector ShootDirection = CameraRotation.Vector(); // 화면 중앙 방향
    FVector SpawnLocation = GetActorLocation() + ShootDirection * 200.f + FVector(0.f, 0.f, -150.f); // 캐논 위치 그대로
    FRotator SpawnRotation = ShootDirection.Rotation();

    //DrawDebugSphere(GetWorld(), SpawnLocation, 15.f, 12, FColor::Green, false, 2.0f);
    //DrawDebugDirectionalArrow(GetWorld(), SpawnLocation, SpawnLocation + ShootDirection * 200.f,
    //    120.f, FColor::Red, false, 2.0f, 0, 3.0f);

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
