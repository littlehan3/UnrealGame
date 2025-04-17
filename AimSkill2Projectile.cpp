#include "AimSkill2Projectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Enemy.h"

AAimSkill2Projectile::AAimSkill2Projectile()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->InitSphereRadius(20.f);
    CollisionComponent->SetCollisionProfileName("BlockAllDynamic");
    CollisionComponent->OnComponentHit.AddDynamic(this, &AAimSkill2Projectile::OnHit);
    RootComponent = CollisionComponent;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(CollisionComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 1000.0f;
    ProjectileMovement->MaxSpeed = 1000.0f;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bRotationFollowsVelocity = true;

    InitialLifeSpan = 3.0f;
}

void AAimSkill2Projectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // �ʴ� 720�� ȸ�� (Yaw ����)
    FRotator RotationDelta(0.0f, 720.0f * DeltaTime, 0.0f);
    MeshComponent->AddRelativeRotation(RotationDelta);
}

void AAimSkill2Projectile::BeginPlay()
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

void AAimSkill2Projectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
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

    // ���� ���� �� ������� ȿ�� ����
    ApplyAreaDamage();

    Destroy(); // ���� �浹 �� �ı�
}

void AAimSkill2Projectile::ApplyAreaDamage()
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
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));  // �Ŀ� ��ü�� Ž��

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

            // �� ĳ���Ϳ� �߷� ������ ȿ�� ����
            AEnemy* Enemy = Cast<AEnemy>(HitActor);
            if (Enemy)
            {
                Enemy->ApplyGravityPull(ExplosionCenter, PullStrength);
                UE_LOG(LogTemp, Warning, TEXT("Applying gravity pull to enemy %s"), *Enemy->GetName());
            }
            else
            {
                // ���� ������Ʈ�� �ִ� �ٸ� ���Ϳ��� �� ����
                UPrimitiveComponent* HitComponent = HitActor->FindComponentByClass<UPrimitiveComponent>();
                if (HitComponent && HitComponent->IsSimulatingPhysics())
                {
                    FVector DirectionToCenter = (ExplosionCenter - HitActor->GetActorLocation()).GetSafeNormal();
                    FVector Force = DirectionToCenter * PullStrength;
                    HitComponent->AddForce(Force);
                }
            }
        }
    }
}

void AAimSkill2Projectile::FireInDirection(const FVector& ShootDirection)
{
    if (ProjectileMovement)
    {
        ProjectileMovement->Velocity = ShootDirection * ProjectileMovement->InitialSpeed;
    }
}