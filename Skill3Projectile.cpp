#include "Skill3Projectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Enemy.h"

ASkill3Projectile::ASkill3Projectile()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->InitSphereRadius(CollisionRadius);
    CollisionComponent->SetCollisionProfileName("BlockAllDynamic");
    CollisionComponent->OnComponentHit.AddDynamic(this, &ASkill3Projectile::OnHit);
    RootComponent = CollisionComponent;

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 1500.0f;
    ProjectileMovement->MaxSpeed = 1500.0f;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bRotationFollowsVelocity = true;

    InitialLifeSpan = 3.0f;
}

void ASkill3Projectile::BeginPlay()
{
    Super::BeginPlay();

    if (Shooter)
    {
        CollisionComponent->IgnoreActorWhenMoving(Shooter, true);
    }

    // ���� ���� ���
    if (LoopingFlightSound)
    {
        FlightAudioComponent = UGameplayStatics::SpawnSoundAttached(
            LoopingFlightSound,
            RootComponent,
            NAME_None,
            FVector::ZeroVector,
            EAttachLocation::KeepRelativeOffset,
            true  // ���� ���
        );
    }
}

void ASkill3Projectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == this || OtherActor == Shooter)
        return;

    // ���� ���� ����
    if (FlightAudioComponent)
    {
        FlightAudioComponent->Stop();
    }

    // ���� ����Ʈ ����
    if (ExplosionEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            ExplosionEffect,
            GetActorLocation(),
            GetActorRotation(),
            FVector(1.0f),
            true,
            true
        );
    }

    // ���� ���� ���
    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
    }

    // ���� ���� ����
    ApplyAreaDamage();

    Destroy(); // ���� �浹 �� �ı�
}

void ASkill3Projectile::ApplyAreaDamage()
{
    FVector ExplosionCenter = GetActorLocation();

    // ����׿� ���� ���� �ð�ȭ (������ ��)
    DrawDebugSphere(
        GetWorld(),
        ExplosionCenter,
        DamageRadius,
        32,
        FColor::Red,
        false,
        1.5f,     // ���� �ð�
        0,
        2.0f      // �� �β�
    );

    TArray<FHitResult> HitResults;
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    TArray<AActor*> IgnoredActors;
    IgnoredActors.Add(Shooter); // ������ ����

    bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
        GetWorld(),
        ExplosionCenter,
        ExplosionCenter,
        DamageRadius,
        ObjectTypes,
        false,
        IgnoredActors,
        EDrawDebugTrace::None,
        HitResults,
        true
    );

    TSet<AActor*> DamagedActors;

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && !DamagedActors.Contains(HitActor))
        {
            UGameplayStatics::ApplyDamage(HitActor, Damage, nullptr, Shooter, nullptr);
            DamagedActors.Add(HitActor);
        }
    }
}

void ASkill3Projectile::FireInDirection(const FVector& ShootDirection)
{
    if (ProjectileMovement)
    {
        ProjectileMovement->Velocity = ShootDirection * ProjectileMovement->InitialSpeed;
    }
}