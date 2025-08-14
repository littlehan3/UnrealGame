#include "EnemyDroneMissile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "EnemyDrone.h"
#include "Engine/OverlapResult.h" // FOverlapResult 구조체 사용

AEnemyDroneMissile::AEnemyDroneMissile()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->InitSphereRadius(14.f);
    CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    CollisionComponent->OnComponentHit.AddDynamic(this, &AEnemyDroneMissile::OnHit);
    RootComponent = CollisionComponent;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(CollisionComponent);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 600.f;
    ProjectileMovement->MaxSpeed = 600.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->ProjectileGravityScale = 0.0f;

    TargetActor = nullptr;
    Health;
    bExploded = false;
    ExplosionRadius;
    LastMoveDirection = FVector::ZeroVector;

    SetActorTickInterval(0.05f);
}

void AEnemyDroneMissile::SetTarget(AActor* Target)
{
    TargetActor = Target;
}

void AEnemyDroneMissile::BeginPlay()
{
    Super::BeginPlay();
}

void AEnemyDroneMissile::ResetMissile(FVector SpawnLocation, AActor* NewTarget)
{
    // 이동 컴포넌트 완전 리셋
    ProjectileMovement->StopMovementImmediately();
    ProjectileMovement->SetUpdatedComponent(CollisionComponent);
    ProjectileMovement->Activate(true); // ProjectileMovement 재활성화

    // 상태 초기화
    SetActorLocation(SpawnLocation);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    SetActorEnableCollision(true);
    SetActorHiddenInGame(false);
    MeshComp->SetHiddenInGame(false);

    Health;
    bExploded = false;
    TargetActor = NewTarget;
    LastMoveDirection = FVector::ZeroVector;

    // 충돌 무시 목록 초기화 후 다시 세팅 (풀링 시 중복 방지)
    CollisionComponent->MoveIgnoreActors.Empty();

    TArray<AActor*> AllDrones;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDrone::StaticClass(), AllDrones);
    for (AActor* Drone : AllDrones)
        CollisionComponent->IgnoreActorWhenMoving(Drone, true);

    TArray<AActor*> AllMissiles;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDroneMissile::StaticClass(), AllMissiles);
    for (AActor* Missile : AllMissiles)
    {
        if (Missile != this)
            CollisionComponent->IgnoreActorWhenMoving(Missile, true);
    }

    if (AActor* OwnerActor = GetOwner())
        CollisionComponent->IgnoreActorWhenMoving(OwnerActor, true);
    if (APawn* InstigatorPawn = GetInstigator())
        CollisionComponent->IgnoreActorWhenMoving(InstigatorPawn, true);

    FVector Dir = FVector::ZeroVector;
    if (IsValid(TargetActor))
    {
        Dir = (TargetActor->GetActorLocation() - SpawnLocation).GetSafeNormal();
    }
    if (Dir.IsNearlyZero())
    {
        Dir = GetActorForwardVector();
        if (Dir.IsNearlyZero())
            Dir = FVector::ForwardVector;
    }

    SetActorRotation(Dir.Rotation());
    ProjectileMovement->Velocity = Dir * ProjectileMovement->InitialSpeed;
    LastMoveDirection = Dir;
}

void AEnemyDroneMissile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 0.15초 간격으로만 움직임 갱신 (프레임 방어)
    static float AccumulatedTime = 0.f;
    AccumulatedTime += DeltaTime;
    if (AccumulatedTime < 0.15f)
        return;
    AccumulatedTime = 0.f;
    if (bExploded) return;

    FVector Dir = FVector::ZeroVector;
    if (IsValid(TargetActor))
    {
        Dir = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    }

    if (!Dir.IsNearlyZero())
    {
        FRotator NewRot = Dir.Rotation();
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), NewRot, DeltaTime, 5.f));
        ProjectileMovement->Velocity = Dir * ProjectileMovement->InitialSpeed;
        LastMoveDirection = Dir;
    }
    else
    {
        if (LastMoveDirection.IsNearlyZero())
            LastMoveDirection = GetActorForwardVector();
        if (LastMoveDirection.IsNearlyZero())
            LastMoveDirection = FVector::ForwardVector;

        ProjectileMovement->Velocity = LastMoveDirection * ProjectileMovement->InitialSpeed;
    }

    DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 24,
        FColor::Red.WithAlpha(64), false, -1.f, 0, 1.f);
}

void AEnemyDroneMissile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (bExploded) return;
    if (!OtherActor || OtherActor == this) return;

    //AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;
    //UGameplayStatics::ApplyDamage(OtherActor, Damage, InstigatorController, this, nullptr);
    Explode();
}

void AEnemyDroneMissile::Explode()
{
    if (bExploded) return;
    bExploded = true;

    if (ExplosionEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), ExplosionEffect, GetActorLocation(), FRotator::ZeroRotator);
    }

    // 폭발 반경 내 플레이어만 피해 적용
    FVector ExplosionCenter = GetActorLocation();
    float ExplosionRadiusTemp = ExplosionRadius; // 변수 그대로 사용

    // Pawn 채널로 플레이어 감지
    TArray<FOverlapResult> Overlaps;
    FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ExplosionRadiusTemp);

    bool bHasOverlaps = GetWorld()->OverlapMultiByChannel(
        Overlaps,
        ExplosionCenter,
        FQuat::Identity,
        ECC_Pawn,
        CollisionShape
    );

    if (bHasOverlaps)
    {
        ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
        for (auto& Overlap : Overlaps)
        {
            if (Overlap.GetActor() == PlayerCharacter)
            {
                UGameplayStatics::ApplyDamage(
                    PlayerCharacter,
                    Damage,
                    GetInstigator() ? GetInstigator()->GetController() : nullptr,
                    this,
                    nullptr
                );
                break; // 플레이어 1명만 처리하고 끝
            }
        }
    }

    // 폭발 범위 시각화
    DrawDebugSphere(GetWorld(), ExplosionCenter, ExplosionRadius, 32, FColor::Red, false, 1.0f, 0, 2.0f);

    // 폭발 후 미사일 숨김/정지
    MeshComp->SetHiddenInGame(true);
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    ProjectileMovement->StopMovementImmediately();
}

float AEnemyDroneMissile::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    Health -= DamageAmount;
    if (Health <= 0.f && !bExploded)
        Explode();
    return DamageAmount;
}
