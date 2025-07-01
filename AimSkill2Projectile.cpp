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
	PrimaryActorTick.bStartWithTickEnabled = true; // ���ۺ��� tick Ȱ��ȭ

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent")); // �浹 ������Ʈ ���� �� ����
	CollisionComponent->InitSphereRadius(20.f); // ��ü �ݰ�
	CollisionComponent->SetCollisionProfileName("BlockAllDynamic"); // �ݸ��� ����
	CollisionComponent->OnComponentHit.AddDynamic(this, &AAimSkill2Projectile::OnHit); // �浹 �̺�Ʈ �ڵ鷯 ���
	RootComponent = CollisionComponent; // ��Ʈ ������Ʈ ����

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent")); // ���� �޽� ������Ʈ ����
	MeshComponent->SetupAttachment(CollisionComponent); // �ݸ��� ������Ʈ�� ����
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �޽��� �ݸ��� ��Ȱ��ȭ

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement")); // ����ü �̵� ������Ʈ ����
	ProjectileMovement->InitialSpeed = 1500.0f; // ����ü �ʱ�ӵ� 
	ProjectileMovement->MaxSpeed = 1500.0f; // ����ü �ְ� �ӵ�
	ProjectileMovement->ProjectileGravityScale = 0.0f; // �߷� ���� ��Ȱ��ȭ
	ProjectileMovement->bRotationFollowsVelocity = true; // �̵� ���⿡ ���� ȸ��

	InitialLifeSpan = 10.0f; // ���� �ʱ���� ����
}

void AAimSkill2Projectile::BeginPlay()
{
	Super::BeginPlay();

	if (Shooter && CollisionComponent)
	{
		CollisionComponent->IgnoreActorWhenMoving(Shooter, true); // �߻��ڿ��� �浹 ����
	}

	if (LoopingFlightSound)
	{
		FlightAudioComponent = UGameplayStatics::SpawnSoundAttached( //���� ������Ʈ ���� �� ����
			LoopingFlightSound, RootComponent, NAME_None, FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset, true); // ��Ʈ ������Ʈ�� ���� ����
	}
}

void AAimSkill2Projectile::EndPlay(const EEndPlayReason::Type EndPlayReason) // ���Ͱ� ���ŵ� �� ȣ��Ǵ� �Լ�
{
	Super::EndPlay(EndPlayReason); // �θ� Ŭ������ endplay ȣ��

	// Ÿ�̸� ����
	GetWorldTimerManager().ClearTimer(PersistentEffectsTimerHandle);
	GetWorldTimerManager().ClearTimer(PeriodicDamageTimerHandle);
	GetWorldTimerManager().ClearTimer(ExplosionDurationTimerHandle);
	GetWorldTimerManager().ClearTimer(DelayTimerHandle);

	// ����� ������Ʈ ����
	if (PersistentAreaAudioComponent)
	{
		PersistentAreaAudioComponent->Stop();
	}
}

void AAimSkill2Projectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bHasReachedApex && GetActorLocation().Z >= ApexHeight) // ���� ������ �������� �ʾҰ� ��ġ�� �ְ��� �̻��̶��
	{
		bHasReachedApex = true; // �ְ��� ���� �÷��� ����
		ProjectileMovement->StopMovementImmediately(); // �̵� ��� ����
		GetWorldTimerManager().SetTimer(DelayTimerHandle, this, &AAimSkill2Projectile::EvaluateTargetAfterApex, DelayBeforeTracking, false); // ���� �� Ÿ�� �� �Լ� ȣ�� Ÿ�̸� ����
	}

	FRotator RotationDelta(0.0f, 720.0f * DeltaTime, 0.0f); // �ʴ� �޽� ȸ��
	MeshComponent->AddRelativeRotation(RotationDelta); // �޽��� ȸ�� ����

	// ���� ȿ���� Ȱ��ȭ�Ǿ� ������ �� ƽ���� DamagedActorsThisTick �ʱ�ȭ
	if (bExplosionActive)
	{
		DamagedActorsThisTick.Empty();
	}
}

void AAimSkill2Projectile::FireInDirection(const FVector& ShootDirection) 
{
	if (ProjectileMovement) // ����ü �̵� ������Ʈ�� �����ϸ�
	{
		ProjectileMovement->Velocity = ShootDirection * ProjectileMovement->InitialSpeed; // ������ �������� �ӵ� ����
	}
}

void AAimSkill2Projectile::EvaluateTargetAfterApex()
{
	AActor* Target = FindClosestEnemy(); // ���� ����� �� Ž��

	if (Target) // Ÿ���� �����ϸ�
	{
		FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal(); // Ÿ�� ���� ���� ���
		ProjectileMovement->Velocity = ToTarget * 1500.0f; // Ÿ�� �������� �ӵ� ����
	}
	else // ������
	{
		GetWorldTimerManager().SetTimer(DelayTimerHandle, this, &AAimSkill2Projectile::AutoExplodeIfNoTarget, 0.3f, false); // 0.3���� �ڵ� ����
	}
}

void AAimSkill2Projectile::AutoExplodeIfNoTarget()
{
	UE_LOG(LogTemp, Warning, TEXT("AutoExplode called at %s"), *GetActorLocation().ToString());
	OnHit(nullptr, nullptr, nullptr, FVector::ZeroVector, FHitResult()); // �浹 �̺�Ʈ ���� ȣ��
}

void AAimSkill2Projectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && (OtherActor == this || OtherActor == Shooter)) return; // �ڱ� �ڽ��̳� �߻��ڿ� �ѵ� �� ����

	if (FlightAudioComponent) FlightAudioComponent->Stop(); // ���� ���� ����

	// ���� ȿ�� ����
	if (ExplosionEffect) 
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), ExplosionEffect, GetActorLocation(), GetActorRotation(), FVector(1.0f), true);
	}

	// ���� ���� ����
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}

	ApplyAreaDamage(); // ���� ������ ����

	// �������� ���� ���� ����
	SpawnPersistentExplosionArea(GetActorLocation()); // ���� ��ġ�� ���� ���� ���� ����

	// �޽ÿ� �ݸ��� ������Ʈ �����
	SetActorHiddenInGame(true); // ���͸� ���ӿ��� ����
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �ݸ��� ��Ȱ��ȭ

	// ����ü�� ���� ���� �ð��� ���� �Ŀ� �ı���
	SetLifeSpan(ExplosionDuration);
}

void AAimSkill2Projectile::SpawnPersistentExplosionArea(const FVector& Location)
{
	ExplosionLocation = Location;
	bExplosionActive = true;

	// �������� ���� ȿ�� ����
	if (PersistentAreaEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), PersistentAreaEffect, Location, FRotator::ZeroRotator, FVector(1.0f), true, true, ENCPoolMethod::AutoRelease, true);
	}

	// �������� ���� ȿ�� ����
	if (PersistentAreaSound)
	{
		PersistentAreaAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
			this, PersistentAreaSound, Location, FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
	}

	// �������� ȿ�� Ÿ�̸� ����
	GetWorldTimerManager().SetTimer(
		PersistentEffectsTimerHandle,
		this,
		&AAimSkill2Projectile::ApplyPersistentEffects,
		0.1f, // 0.1�ʸ��� ������� ȿ�� ����
		true);

	// �ֱ����� ������ Ÿ�̸� ����
	GetWorldTimerManager().SetTimer(
		PeriodicDamageTimerHandle,
		this,
		&AAimSkill2Projectile::ApplyPeriodicDamage,
		DamageInterval,
		true);

	// ���� ���� �ð� Ÿ�̸� ����
	GetWorldTimerManager().SetTimer(
		ExplosionDurationTimerHandle,
		[this]()
		{
			bExplosionActive = false;
			GetWorldTimerManager().ClearTimer(PersistentEffectsTimerHandle);
			GetWorldTimerManager().ClearTimer(PeriodicDamageTimerHandle);

			if (PersistentAreaAudioComponent)
			{
				PersistentAreaAudioComponent->FadeOut(0.5f, 0.0f);
			}
		},
		ExplosionDuration,
		false);
}

void AAimSkill2Projectile::ApplyPersistentEffects()
{
	if (!bExplosionActive) return;

	// ����� �ð�ȭ
	DrawDebugSphere(GetWorld(), ExplosionLocation, DamageRadius, 16, FColor::Yellow, false, 0.12f, 0, 1.0f);

	// ���� �� ���� ã��
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(Shooter);

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		ExplosionLocation,
		DamageRadius,
		ObjectTypes,
		nullptr,
		IgnoredActors,
		OverlappedActors);

	// �� ���Ϳ� ������� ȿ�� ����
	for (AActor* Actor : OverlappedActors)
	{
		if (Actor && Actor != Shooter)
		{
			AEnemy* Enemy = Cast<AEnemy>(Actor);
			if (Enemy)
			{
				Enemy->ApplyGravityPull(ExplosionLocation, PullStrength);
			}
			else
			{
				UPrimitiveComponent* PhysComp = Actor->FindComponentByClass<UPrimitiveComponent>();
				if (PhysComp && PhysComp->IsSimulatingPhysics())
				{
					FVector Direction = (ExplosionLocation - Actor->GetActorLocation()).GetSafeNormal();
					PhysComp->AddForce(Direction * PullStrength);
				}
			}
		}
	}
}

void AAimSkill2Projectile::ApplyPeriodicDamage()
{
	if (!bExplosionActive) return;

	// ���� �� ���� ã��
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(Shooter);

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		ExplosionLocation,
		DamageRadius,
		ObjectTypes,
		nullptr,
		IgnoredActors,
		OverlappedActors);

	// �� ���Ϳ� ������ ����
	for (AActor* Actor : OverlappedActors)
	{
		if (Actor && Actor != Shooter)
		{
			UGameplayStatics::ApplyDamage(Actor, Damage, nullptr, Shooter, nullptr);

			// ������ ���� �ð�ȭ
			DrawDebugLine(
				GetWorld(),
				ExplosionLocation,
				Actor->GetActorLocation(),
				FColor::Red,
				false,
				0.5f,
				0,
				2.0f);
		}
	}
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

void AAimSkill2Projectile::SetShooter(AActor* InShooter)
{
	Shooter = InShooter;
	if (CollisionComponent && Shooter)
	{
		CollisionComponent->IgnoreActorWhenMoving(Shooter, true);
		TArray<UPrimitiveComponent*> Components;
		Shooter->GetComponents<UPrimitiveComponent>(Components);
		for (auto Comp : Components)
		{
			CollisionComponent->IgnoreComponentWhenMoving(Comp, true);
			//UE_LOG(LogTemp, Warning, TEXT("Setshooter ignorence called"));
		}
	}
}