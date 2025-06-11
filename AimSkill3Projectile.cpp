#include "AimSkill3Projectile.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

AAimSkill3Projectile::AAimSkill3Projectile()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    CollisionComponent->InitSphereRadius(50.f);
    CollisionComponent->SetCollisionProfileName("Projectile");
    CollisionComponent->OnComponentHit.AddDynamic(this, &AAimSkill3Projectile::OnHit);
    RootComponent = CollisionComponent;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    ProjectileMovement->InitialSpeed = 5000.f;
    ProjectileMovement->MaxSpeed = 5000.f;
    ProjectileMovement->ProjectileGravityScale = 3.0f;
    ProjectileMovement->bRotationFollowsVelocity = false;
}

void AAimSkill3Projectile::BeginPlay()
{
    Super::BeginPlay();
    SetLifeSpan(10.f);
}

void AAimSkill3Projectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bExploded)
    {
        DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(),
            GetActorLocation() + GetVelocity().GetSafeNormal() * 100.f,
            50.f, FColor::Red, false, -1.f, 0, 5.f);
    }
}

void AAimSkill3Projectile::FireInDirection(const FVector& ShootDirection)
{
    if (ProjectileMovement)
    {
        ProjectileMovement->Velocity = ShootDirection * ProjectileMovement->InitialSpeed;
    }
}

void AAimSkill3Projectile::SetExplosionParams(float InDamage, float InRadius)
{
    Damage = InDamage;
    ExplosionRadius = InRadius;
}

void AAimSkill3Projectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (bExploded) return;
    Explode();
}

void AAimSkill3Projectile::Explode()
{
    if (bExploded) return;
    bExploded = true;

    // ExplosionEffect 널 체크
    if (ExplosionEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            ExplosionEffect,
            GetActorLocation(),
            FRotator::ZeroRotator,
            FVector(ExplosionRadius / 100.f)
        );
    }

    // 데미지 적용 시 InstigatorController 널 체크
    AController* InstigatorCtrl = GetInstigatorController();
    UWorld* World = GetWorld();
    if (World)
    {
        TArray<AActor*> Ignored;
        UGameplayStatics::ApplyRadialDamage(
            World,
            Damage,
            GetActorLocation(),
            ExplosionRadius,
            nullptr,
            Ignored,
            this,
            InstigatorCtrl,
            true
        );
    }

    // 컴포넌트 널 체크
    if (CollisionComponent)
        CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (MeshComponent)
        MeshComponent->SetVisibility(false);

    SetActorHiddenInGame(true);
    SetLifeSpan(0.5f);
}