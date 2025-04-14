#include "AimSkill2Projectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"

AAimSkill2Projectile::AAimSkill2Projectile()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
    CollisionComp->InitSphereRadius(15.f);
    CollisionComp->SetCollisionProfileName("OverlapAllDynamic");
    CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AAimSkill2Projectile::OnOverlap);
    RootComponent = CollisionComp;

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = CollisionComp;
    ProjectileMovement->InitialSpeed = 1000.f;
    ProjectileMovement->MaxSpeed = 1000.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;

    InitialLifeSpan = LifeSpanSeconds;
}

void AAimSkill2Projectile::BeginPlay()
{
    Super::BeginPlay();
}

void AAimSkill2Projectile::SetShooter(AActor* InShooter)
{
    Shooter = InShooter;
}

void AAimSkill2Projectile::FireInDirection(const FVector& ShootDirection)
{
    if (ProjectileMovement)
    {
        ProjectileMovement->Velocity = ShootDirection * ProjectileMovement->InitialSpeed;
    }
}

void AAimSkill2Projectile::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this || OtherActor == Shooter)
        return;

    if (ACharacter* Target = Cast<ACharacter>(OtherActor))
    {
        FVector PullDir = GetActorLocation() - Target->GetActorLocation();
        PullDir.Normalize();
        Target->LaunchCharacter(PullDir * PullStrength, true, true);
    }
}
