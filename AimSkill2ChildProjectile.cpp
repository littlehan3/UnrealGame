#include "AimSkill2ChildProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Enemy.h"

AAimSkill2ChildProjectile::AAimSkill2ChildProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->InitSphereRadius(20.f);
    CollisionComponent->SetCollisionProfileName("BlockAllDynamic");
    CollisionComponent->OnComponentHit.AddDynamic(this, &AAimSkill2ChildProjectile::OnHit);
    RootComponent = CollisionComponent;

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = FlySpeed;
    ProjectileMovement->MaxSpeed = FlySpeed;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bRotationFollowsVelocity = true;

    InitialLifeSpan = 10.0f;
}

void AAimSkill2ChildProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (Shooter)
    {
        CollisionComponent->IgnoreActorWhenMoving(Shooter, true);
    }
}

void AAimSkill2ChildProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (Target && IsValid(Target))
    {
        FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        ProjectileMovement->Velocity = ToTarget * FlySpeed;

        float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
        if (Distance <= 100.0f)
        {
            Explode();
        }
    }
    else
    {
        Explode();
    }
}

void AAimSkill2ChildProjectile::SetTarget(AActor* InTarget)
{
    Target = InTarget;
}

void AAimSkill2ChildProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor == Shooter || OtherActor == this)
        return;

    Explode();
}

void AAimSkill2ChildProjectile::Explode()
{
    if (ExplosionEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), ExplosionEffect, GetActorLocation(), FRotator::ZeroRotator);
    }

    UGameplayStatics::ApplyRadialDamage(
        GetWorld(),
        Damage,
        GetActorLocation(),
        DamageRadius,
        nullptr,
        { Shooter },
        this,
        nullptr,
        true
    );

    Destroy();
}
