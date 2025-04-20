#include "AimSkill2Projectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Enemy.h"

AAimSkill2Projectile::AAimSkill2Projectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

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

	InitialLifeSpan = 10.0f;
}

void AAimSkill2Projectile::BeginPlay()
{
	Super::BeginPlay();

	if (Shooter)
	{
		CollisionComponent->IgnoreActorWhenMoving(Shooter, true);
	}

	if (LoopingFlightSound)
	{
		FlightAudioComponent = UGameplayStatics::SpawnSoundAttached(
			LoopingFlightSound, RootComponent, NAME_None, FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset, true);
	}
}

void AAimSkill2Projectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bHasReachedApex && GetActorLocation().Z >= ApexHeight)
	{
		bHasReachedApex = true;
		ProjectileMovement->StopMovementImmediately();
		GetWorldTimerManager().SetTimer(DelayTimerHandle, this, &AAimSkill2Projectile::EvaluateTargetAfterApex, DelayBeforeTracking, false);
	}

	FRotator RotationDelta(0.0f, 720.0f * DeltaTime, 0.0f);
	MeshComponent->AddRelativeRotation(RotationDelta);
}

void AAimSkill2Projectile::FireInDirection(const FVector& ShootDirection)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = ShootDirection * ProjectileMovement->InitialSpeed;
	}
}

void AAimSkill2Projectile::EvaluateTargetAfterApex()
{
	AActor* Target = FindClosestEnemy();

	if (Target)
	{
		FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		ProjectileMovement->Velocity = ToTarget * 1500.0f;
	}
	else
	{
		GetWorldTimerManager().SetTimer(DelayTimerHandle, this, &AAimSkill2Projectile::AutoExplodeIfNoTarget, 0.3f, false);
	}
}

void AAimSkill2Projectile::AutoExplodeIfNoTarget()
{
	UE_LOG(LogTemp, Warning, TEXT("AutoExplode called at %s"), *GetActorLocation().ToString());
	OnHit(nullptr, nullptr, nullptr, FVector::ZeroVector, FHitResult());
}

void AAimSkill2Projectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && (OtherActor == this || OtherActor == Shooter)) return;

	if (FlightAudioComponent) FlightAudioComponent->Stop();

	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), ExplosionEffect, GetActorLocation(), GetActorRotation(), FVector(1.0f), true);
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}

	ApplyAreaDamage();
	Destroy();
}

void AAimSkill2Projectile::ApplyAreaDamage()
{
	FVector ExplosionCenter = GetActorLocation();

	DrawDebugSphere(GetWorld(), ExplosionCenter, DamageRadius, 32, FColor::Red, false, 1.5f, 0, 2.0f);

	TArray<FHitResult> HitResults;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(Shooter);

	UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(), ExplosionCenter, ExplosionCenter, DamageRadius,
		ObjectTypes, false, IgnoredActors, EDrawDebugTrace::None, HitResults, true);

	TSet<AActor*> DamagedActors;

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && !DamagedActors.Contains(HitActor))
		{
			UGameplayStatics::ApplyDamage(HitActor, Damage, nullptr, Shooter, nullptr);
			DamagedActors.Add(HitActor);

			AEnemy* Enemy = Cast<AEnemy>(HitActor);
			if (Enemy)
			{
				Enemy->ApplyGravityPull(ExplosionCenter, PullStrength);
			}
			else
			{
				UPrimitiveComponent* PhysComp = HitActor->FindComponentByClass<UPrimitiveComponent>();
				if (PhysComp && PhysComp->IsSimulatingPhysics())
				{
					FVector Direction = (ExplosionCenter - HitActor->GetActorLocation()).GetSafeNormal();
					PhysComp->AddForce(Direction * PullStrength);
				}
			}
		}
	}
}

AActor* AAimSkill2Projectile::FindClosestEnemy()
{
	DrawDebugSphere(GetWorld(), GetActorLocation(), DetectionRadius, 32, FColor::Green, false, 1.0f, 0, 2.0f);

	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> Ignored;
	Ignored.Add(Shooter);

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		GetActorLocation(),
		DetectionRadius,
		ObjectTypes,
		AEnemy::StaticClass(),
		Ignored,
		OverlappedActors);

	AActor* ClosestEnemy = nullptr;
	float MinDistSquared = FLT_MAX;

	for (AActor* Actor : OverlappedActors)
	{
		float DistSq = FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation());
		if (DistSq < MinDistSquared)
		{
			MinDistSquared = DistSq;
			ClosestEnemy = Actor;
		}
	}

	return ClosestEnemy;
}