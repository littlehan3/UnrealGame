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
	PrimaryActorTick.bStartWithTickEnabled = true; // 시작부터 tick 활성화

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent")); // 충돌 컴포넌트 생성 및 설정
	CollisionComponent->InitSphereRadius(20.f); // 구체 반경
	CollisionComponent->SetCollisionProfileName("BlockAllDynamic"); // 콜리전 설정
	CollisionComponent->OnComponentHit.AddDynamic(this, &AAimSkill2Projectile::OnHit); // 충돌 이벤트 핸들러 등록
	RootComponent = CollisionComponent; // 루트 컴포넌트 설정

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent")); // 정적 메시 컴포넌트 생성
	MeshComponent->SetupAttachment(CollisionComponent); // 콜리전 컴포넌트에 부착
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 메쉬의 콜리전 비활성화

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement")); // 투사체 이동 컴포넌트 생성
	ProjectileMovement->InitialSpeed = 1500.0f; // 투사체 초기속도 
	ProjectileMovement->MaxSpeed = 1500.0f; // 투사체 최고 속도
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 중력 영향 비활성화
	ProjectileMovement->bRotationFollowsVelocity = true; // 이동 방향에 따라 회전

	InitialLifeSpan = 10.0f; // 엑터 초기수명 설정
}

void AAimSkill2Projectile::BeginPlay()
{
	Super::BeginPlay();

	if (Shooter && CollisionComponent)
	{
		CollisionComponent->IgnoreActorWhenMoving(Shooter, true); // 발사자와의 충돌 무시
	}

	if (LoopingFlightSound)
	{
		FlightAudioComponent = UGameplayStatics::SpawnSoundAttached( //사운드 컴포넌트 생성 및 부착
			LoopingFlightSound, RootComponent, NAME_None, FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset, true); // 루트 컴포넌트에 사운드 부착
	}
}

void AAimSkill2Projectile::EndPlay(const EEndPlayReason::Type EndPlayReason) // 엑터가 제거될 때 호출되는 함수
{
	Super::EndPlay(EndPlayReason); // 부모 클래스의 endplay 호출

	// 타이머 정리
	GetWorldTimerManager().ClearTimer(PersistentEffectsTimerHandle);
	GetWorldTimerManager().ClearTimer(PeriodicDamageTimerHandle);
	GetWorldTimerManager().ClearTimer(ExplosionDurationTimerHandle);
	GetWorldTimerManager().ClearTimer(DelayTimerHandle);

	// 오디오 컴포넌트 정리
	if (PersistentAreaAudioComponent)
	{
		PersistentAreaAudioComponent->Stop();
	}
}

void AAimSkill2Projectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bHasReachedApex && GetActorLocation().Z >= ApexHeight) // 아직 고점에 도달하지 않았고 위치가 최고점 이상이라면
	{
		bHasReachedApex = true; // 최고점 도달 플래그 설정
		ProjectileMovement->StopMovementImmediately(); // 이동 즉시 정지
		GetWorldTimerManager().SetTimer(DelayTimerHandle, this, &AAimSkill2Projectile::EvaluateTargetAfterApex, DelayBeforeTracking, false); // 지연 후 타겟 평가 함수 호출 타이머 설정
	}

	FRotator RotationDelta(0.0f, 720.0f * DeltaTime, 0.0f); // 초당 메쉬 회전
	MeshComponent->AddRelativeRotation(RotationDelta); // 메쉬에 회전 적용

	// 폭발 효과가 활성화되어 있으면 매 틱마다 DamagedActorsThisTick 초기화
	if (bExplosionActive)
	{
		DamagedActorsThisTick.Empty();
	}
}

void AAimSkill2Projectile::FireInDirection(const FVector& ShootDirection) 
{
	if (ProjectileMovement) // 투사체 이동 컴포넌트가 존재하면
	{
		ProjectileMovement->Velocity = ShootDirection * ProjectileMovement->InitialSpeed; // 지정된 방향으로 속도 설정
	}
}

void AAimSkill2Projectile::EvaluateTargetAfterApex()
{
	AActor* Target = FindClosestEnemy(); // 가장 가까운 적 탐색

	if (Target) // 타겟이 존재하면
	{
		FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal(); // 타겟 방향 벡터 계산
		ProjectileMovement->Velocity = ToTarget * 1500.0f; // 타겟 방향으로 속도 설정
	}
	else // 없으면
	{
		GetWorldTimerManager().SetTimer(DelayTimerHandle, this, &AAimSkill2Projectile::AutoExplodeIfNoTarget, 0.3f, false); // 0.3초후 자동 폭발
	}
}

void AAimSkill2Projectile::AutoExplodeIfNoTarget()
{
	UE_LOG(LogTemp, Warning, TEXT("AutoExplode called at %s"), *GetActorLocation().ToString());
	OnHit(nullptr, nullptr, nullptr, FVector::ZeroVector, FHitResult()); // 충돌 이벤트 수동 호출
}

void AAimSkill2Projectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && (OtherActor == this || OtherActor == Shooter)) return; // 자기 자신이나 발사자와 총돌 시 무시

	if (FlightAudioComponent) FlightAudioComponent->Stop(); // 비행 사운드 중지

	// 폭발 효과 생성
	if (ExplosionEffect) 
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), ExplosionEffect, GetActorLocation(), GetActorRotation(), FVector(1.0f), true);
	}

	// 폭발 사운드 생성
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}

	ApplyAreaDamage(); // 영역 데미지 적용

	// 지속적인 폭발 영역 생성
	SpawnPersistentExplosionArea(GetActorLocation()); // 현재 위치에 지속 폭발 영역 생성

	// 메시와 콜리전 컴포넌트 숨기기
	SetActorHiddenInGame(true); // 액터를 게임에서 숨김
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 콜리전 비활성화

	// 투사체는 폭발 지속 시간이 끝난 후에 파괴됨
	SetLifeSpan(ExplosionDuration);
}

void AAimSkill2Projectile::SpawnPersistentExplosionArea(const FVector& Location)
{
	ExplosionLocation = Location;
	bExplosionActive = true;

	// 지속적인 폭발 효과 생성
	if (PersistentAreaEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), PersistentAreaEffect, Location, FRotator::ZeroRotator, FVector(1.0f), true, true, ENCPoolMethod::AutoRelease, true);
	}

	// 지속적인 사운드 효과 생성
	if (PersistentAreaSound)
	{
		PersistentAreaAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
			this, PersistentAreaSound, Location, FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
	}

	// 지속적인 효과 타이머 설정
	GetWorldTimerManager().SetTimer(
		PersistentEffectsTimerHandle,
		this,
		&AAimSkill2Projectile::ApplyPersistentEffects,
		0.1f, // 0.1초마다 끌어당기는 효과 적용
		true);

	// 주기적인 데미지 타이머 설정
	GetWorldTimerManager().SetTimer(
		PeriodicDamageTimerHandle,
		this,
		&AAimSkill2Projectile::ApplyPeriodicDamage,
		DamageInterval,
		true);

	// 폭발 지속 시간 타이머 설정
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

	// 디버그 시각화
	DrawDebugSphere(GetWorld(), ExplosionLocation, DamageRadius, 16, FColor::Yellow, false, 0.12f, 0, 1.0f);

	// 범위 내 액터 찾기
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

	// 각 액터에 끌어당기는 효과 적용
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

	// 범위 내 액터 찾기
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

	// 각 액터에 데미지 적용
	for (AActor* Actor : OverlappedActors)
	{
		if (Actor && Actor != Shooter)
		{
			UGameplayStatics::ApplyDamage(Actor, Damage, nullptr, Shooter, nullptr);

			// 데미지 적용 시각화
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