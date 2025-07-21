#include "BossProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"

ABossProjectile::ABossProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    CollisionComponent->InitSphereRadius(CollisionRadius); 
    CollisionComponent->SetCollisionProfileName(TEXT("EnemyProjectile")); 
    CollisionComponent->OnComponentHit.AddDynamic(this, &ABossProjectile::OnHit);
    RootComponent = CollisionComponent;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    ProjectileMovement->InitialSpeed = 1200.f;
    ProjectileMovement->MaxSpeed = 1200.f;
    ProjectileMovement->ProjectileGravityScale = 0.f;
    ProjectileMovement->bRotationFollowsVelocity = true;

    InitialLifeSpan = 4.f;
}

void ABossProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (Shooter)
        CollisionComponent->IgnoreActorWhenMoving(Shooter, true);

    if (LoopingFlightSound)
        FlightAudioComponent = UGameplayStatics::SpawnSoundAttached(
            LoopingFlightSound, RootComponent, NAME_None,
            FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true);
}

void ABossProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // �ʴ� 720�� ȸ�� (Yaw ����)
    FRotator RotationDelta(0.0f, 720.0f * DeltaTime, 0.0f);
    MeshComponent->AddRelativeRotation(RotationDelta);
}

void ABossProjectile::FireInDirection(const FVector& ShootDir)
{
    if (ProjectileMovement)
        ProjectileMovement->Velocity = ShootDir.GetSafeNormal() * ProjectileMovement->InitialSpeed;
}

void ABossProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector Impulse,
    const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == this || OtherActor == Shooter)
        return;

    if (FlightAudioComponent) FlightAudioComponent->Stop();

    if (ExplosionEffect)
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), ExplosionEffect, GetActorLocation(), FRotator::ZeroRotator);

    if (ExplosionSound)
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());

    ApplyAreaDamage();
    Destroy();
}

void ABossProjectile::ApplyAreaDamage()
{
    const FVector Center = GetActorLocation();
    DrawDebugSphere(GetWorld(), Center, DamageRadius, 32, FColor::Red, false, 3.0f);

    // �ݰ� �� ���� ���� ã��
    TArray<FOverlapResult> Overlaps;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(DamageRadius);

    bool bHasOverlaps = GetWorld()->OverlapMultiByObjectType(
        Overlaps,
        Center,
        FQuat::Identity,
        FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
        Sphere
    );

    if (bHasOverlaps)
    {
        // �� TSet���� �ڵ� �ߺ� ����
        TSet<AActor*> UniqueActors;

        for (const FOverlapResult& Overlap : Overlaps)
        {
            AActor* HitActor = Overlap.GetActor();
            if (HitActor && HitActor != Shooter)
            {
                UniqueActors.Add(HitActor);  // TSet�� �ڵ����� �ߺ� ����
            }
        }

        // �ߺ� ���ŵ� ���͵鿡�� ������ ����
        for (AActor* HitActor : UniqueActors)
        {
            UE_LOG(LogTemp, Warning, TEXT("Applying damage to: %s"), *HitActor->GetName());

            FDamageEvent DamageEvent;
            HitActor->TakeDamage(Damage, DamageEvent, nullptr, Shooter);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No overlapping actors found in damage radius!"));
    }
}

