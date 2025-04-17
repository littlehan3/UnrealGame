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

    // 초당 720도 회전 (Yaw 기준)
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

    // 비행 사운드 재생
    if (LoopingFlightSound)
    {
        FlightAudioComponent = UGameplayStatics::SpawnSoundAttached(
            LoopingFlightSound,
            RootComponent,
            NAME_None,
            FVector::ZeroVector,
            EAttachLocation::KeepRelativeOffset,
            true  // 루프 재생
        );
    }
}

void AAimSkill2Projectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == this || OtherActor == Shooter)
        return;

    // 비행 사운드 정지
    if (FlightAudioComponent)
    {
        FlightAudioComponent->Stop();
    }

    // 폭발 이펙트 생성
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

    // 폭발 사운드 재생
    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
    }

    // 광역 피해 및 끌어당기기 효과 적용
    ApplyAreaDamage();

    Destroy(); // 적과 충돌 시 파괴
}

void AAimSkill2Projectile::ApplyAreaDamage()
{
    FVector ExplosionCenter = GetActorLocation();

    // 디버그용 폭발 범위 시각화 (빨간색 원)
    DrawDebugSphere(
        GetWorld(),
        ExplosionCenter,
        DamageRadius,
        32,
        FColor::Red,
        false,
        1.5f,     // 지속 시간
        0,
        2.0f      // 선 두께
    );

    TArray<FHitResult> HitResults;
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));  // 파운 객체만 탐지

    TArray<AActor*> IgnoredActors;
    IgnoredActors.Add(Shooter); // 시전자 제외

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

            // 적 캐릭터에 중력 끌어당김 효과 적용
            AEnemy* Enemy = Cast<AEnemy>(HitActor);
            if (Enemy)
            {
                Enemy->ApplyGravityPull(ExplosionCenter, PullStrength);
                UE_LOG(LogTemp, Warning, TEXT("Applying gravity pull to enemy %s"), *Enemy->GetName());
            }
            else
            {
                // 물리 컴포넌트가 있는 다른 액터에도 힘 적용
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