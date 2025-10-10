#include "EnemyShooterGun.h"
#include "EnemyShooter.h" // ������ Ŭ���� ����
#include "Engine/World.h" // UWorld ���
#include "GameFramework/DamageType.h" // UDamageType ���
#include "Kismet/GameplayStatics.h" // �����÷��� ��ƿ��Ƽ �Լ� ���
#include "DrawDebugHelpers.h" // ����� �ð�ȭ ��� ���
#include "Sound/SoundBase.h" // USoundBase ���
#include "NiagaraSystem.h" // ���̾ư��� �ý��� ���
#include "NiagaraComponent.h" // ���̾ư��� ������Ʈ ���
#include "NiagaraFunctionLibrary.h" // ���̾ư��� ����Ʈ ���� �Լ� ���
#include "GameFramework/PlayerController.h" // �÷��̾� ��Ʈ�ѷ� ����
#include "GameFramework/Pawn.h" // APawn ����
#include "Materials/MaterialInterface.h" // ��Ƽ���� �������̽� ���

AEnemyShooterGun::AEnemyShooterGun()
{
	PrimaryActorTick.bCanEverTick = false; // �� ������ Tick ���ʿ�

	// �⺻ ������Ʈ ���� �� ����
	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	RootComponent = GunMesh;
	GunMesh->SetSimulatePhysics(false);
	GunMesh->SetEnableGravity(false);
	GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MuzzleSocket = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MuzzleSocket"));
	MuzzleSocket->SetupAttachment(GunMesh);
	MuzzleSocket->SetSimulatePhysics(false);
	MuzzleSocket->SetEnableGravity(false);
	MuzzleSocket->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MuzzleSocket->SetVisibility(false); // ���ӿ����� ������ ����

	// ������ ���ؼ� ������Ʈ ����
	AimingLaserMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AimingLaserMesh"));
	AimingLaserMesh->SetupAttachment(RootComponent);
	AimingLaserMesh->SetVisibility(false); // �⺻������ ����
	AimingLaserMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ������ ��� ���� �ʱ�ȭ
	CurrentMuzzleFlashComponent = nullptr;
	CurrentImpactEffectComponent = nullptr;
}

void AEnemyShooterGun::BeginPlay()
{
	Super::BeginPlay();

	// ������ �޽� ���� �� ��Ƽ���� ����
	if (AimingLaserMeshAsset && AimingLaserMesh)
	{
		AimingLaserMesh->SetStaticMesh(AimingLaserMeshAsset);
		if (AimingLaserMaterial)
		{
			AimingLaserMesh->SetMaterial(0, AimingLaserMaterial);
		}
	}

	if (GetOwner())
	{
		UE_LOG(LogTemp, Warning, TEXT("Gun Owner: %s"), *GetOwner()->GetName());
	}
}

void AEnemyShooterGun::FireGun()
{
	if (!GetOwner()) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;

	FVector MuzzleLocation = MuzzleSocket ? MuzzleSocket->GetComponentLocation() : GunMesh->GetComponentLocation();
	FVector PredictedPlayerLocation = CalculatePredictedPlayerPosition(PlayerPawn); // �÷��̾��� �̷� ��ġ ����

	// ���� �߻縦 ���� ���� �ѱ� ��ġ�� ��ǥ ��ġ�� ����
	StoredMuzzleLocation = MuzzleLocation;
	StoredTargetLocation = PredictedPlayerLocation;

	if (bShowAimWarning) // ���� ��� �ɼ��� ���� �ִٸ�
	{
		ShowAimingLaserMesh(MuzzleLocation, PredictedPlayerLocation); // ������ ���ؼ� ǥ��
		if (AimWarningSound) // ��� ���� ���
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), AimWarningSound, MuzzleLocation, 0.7f);
		}

		// AimWarningTime �Ŀ� ExecuteDelayedShot �Լ��� ȣ���ϵ��� Ÿ�̸� ����
		GetWorld()->GetTimerManager().SetTimer(
			DelayedShotTimerHandle, this, &AEnemyShooterGun::ExecuteDelayedShot, AimWarningTime, false
		);
	}
	else // ���� ��� �ɼ��� ���� �ִٸ�
	{
		ExecuteDelayedShot(); // ��� �߻�
	}
}

void AEnemyShooterGun::ExecuteDelayedShot()
{
	HideAimingLaserMesh(); // ������ ���ؼ� ����

	// ���� �߻� ������ ���� ������(Shooter)�� ��� �ִϸ��̼��� ���
	if (AEnemyShooter* OwnerShooter = Cast<AEnemyShooter>(GetOwner()))
	{
		OwnerShooter->PlayShootingAnimation();
	}

	// ���߷��� �����Ͽ� ���� ��ǥ ��ġ ����
	FVector FinalTargetLocation = ApplyAccuracySpread(StoredTargetLocation);
	FVector ForwardDirection = (FinalTargetLocation - StoredMuzzleLocation).GetSafeNormal();
	FVector EndLocation = StoredMuzzleLocation + (ForwardDirection * FireRange); // �ִ� �����Ÿ������� ����

	// ��Ʈ��ĵ: �ѱ����� ���� ��ǥ �������� ���� Ʈ���̽� ����
	FHitResult HitResult;
	bool bHit = PerformLineTrace(StoredMuzzleLocation, EndLocation, HitResult);

	PlayMuzzleFlash(StoredMuzzleLocation); // �ѱ� ���� ����Ʈ ���

	if (bHit) // ���� Ʈ���̽��� ���� �¾Ҵٸ�
	{
		PlayImpactEffect(HitResult.Location, HitResult.Normal); // �ǰ� ������ ����Ʈ ���

		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (PlayerPawn && HitResult.GetActor() == PlayerPawn) // ���� ����� �÷��̾���
		{
			UGameplayStatics::ApplyDamage( // ������ ����
				HitResult.GetActor(), Damage, GetOwner()->GetInstigatorController(), this, UDamageType::StaticClass()
			);
		}
	}

	if (FireSound) // �ݹ� ���� ���
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, StoredMuzzleLocation);
	}
}

void AEnemyShooterGun::ShowAimingLaserMesh(FVector StartLocation, FVector EndLocation)
{
	if (AimingLaserMesh && AimingLaserMeshAsset)
	{
		SetupLaserTransform(StartLocation, EndLocation); // �������� ��ġ, ȸ��, ũ�� ����
		AimingLaserMesh->SetVisibility(true); // ���̰� ����

		// AimWarningTime �Ŀ� �ڵ����� �������� ���⵵�� Ÿ�̸� ����
		GetWorld()->GetTimerManager().SetTimer(
			AimingLaserTimerHandle, this, &AEnemyShooterGun::HideAimingLaserMesh, AimWarningTime, false
		);
	}
}

void AEnemyShooterGun::SetupLaserTransform(FVector StartLocation, FVector EndLocation)
{
	if (!AimingLaserMesh) return;

	float Distance = FVector::Dist(StartLocation, EndLocation); // �������� ���� ������ �Ÿ�
	FVector Direction = (EndLocation - StartLocation).GetSafeNormal(); // ���� ����
	FVector MidPoint = StartLocation + (Direction * Distance * 0.5f); // �� ���� �߰� ����
	FRotator LookRotation = Direction.Rotation(); // ���� ���͸� ȸ�������� ��ȯ

	FVector FinalLocation = MidPoint + AimingLaserOffset; // ������ ����� ���� ��ġ
	FRotator FinalRotation = LookRotation + AimingLaserRotationOffset; // ������ ����� ���� ȸ��
	FVector FinalScale = FVector(Distance / 100.0f, AimingLaserScale, AimingLaserScale); // X���� �Ÿ��� �°� ����

	// ���� Transform ����
	AimingLaserMesh->SetWorldLocation(FinalLocation);
	AimingLaserMesh->SetWorldRotation(FinalRotation);
	AimingLaserMesh->SetWorldScale3D(FinalScale);
}

void AEnemyShooterGun::HideAimingLaserMesh()
{
	if (AimingLaserMesh)
	{
		AimingLaserMesh->SetVisibility(false); // ������ �ʰ� ����
	}
}

bool AEnemyShooterGun::PerformLineTrace(FVector StartLocation, FVector EndLocation, FHitResult& OutHitResult)
{
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	CollisionParams.AddIgnoredActor(GetOwner());
	CollisionParams.bTraceComplex = true;

	// ���� ä�ο� ���� ������� �˻��Ͽ� �ϳ��� ������ true ��ȯ
	return GetWorld()->LineTraceSingleByChannel(
		OutHitResult, StartLocation, EndLocation, ECollisionChannel::ECC_WorldStatic, CollisionParams
	) || GetWorld()->LineTraceSingleByChannel(
		OutHitResult, StartLocation, EndLocation, ECollisionChannel::ECC_WorldDynamic, CollisionParams
	) || GetWorld()->LineTraceSingleByChannel(
		OutHitResult, StartLocation, EndLocation, ECollisionChannel::ECC_Pawn, CollisionParams
	);
}

void AEnemyShooterGun::PlayMuzzleFlash(FVector Location)
{
	// �ѱ� ���� ����Ʈ ��� ����
	if (MuzzleFlash)
	{
		CurrentMuzzleFlashComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), MuzzleFlash, Location, GetActorRotation(), FVector(MuzzleFlashScale)
		);
		if (CurrentMuzzleFlashComponent)
		{
			CurrentMuzzleFlashComponent->SetFloatParameter(TEXT("PlayRate"), MuzzleFlashPlayRate);
			GetWorld()->GetTimerManager().SetTimer(
				MuzzleFlashTimerHandle, this, &AEnemyShooterGun::StopMuzzleFlash, MuzzleFlashDuration, false
			);
		}
	}
}

void AEnemyShooterGun::PlayImpactEffect(FVector Location, FVector ImpactNormal)
{
	// �ǰ� ����Ʈ ��� ����
	if (ImpactEffect)
	{
		CurrentImpactEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), ImpactEffect, Location, ImpactNormal.Rotation()
		);
		if (CurrentImpactEffectComponent)
		{
			GetWorld()->GetTimerManager().SetTimer(
				ImpactEffectTimerHandle, this, &AEnemyShooterGun::StopImpactEffect, ImpactEffectDuration, false
			);
		}
	}
}

FVector AEnemyShooterGun::CalculatePredictedPlayerPosition(APawn* Player)
{
	if (!Player) return FVector::ZeroVector;
	FVector CurrentLocation = Player->GetActorLocation();
	FVector CurrentVelocity = Player->GetVelocity();
	// (�̷� ��ġ) = (���� ��ġ) + (���� �ӵ�) * (���� �ð�)
	return CurrentLocation + (CurrentVelocity * PredictionTime);
}

FVector AEnemyShooterGun::ApplyAccuracySpread(FVector TargetLocation)
{
	float AccuracyRoll = FMath::RandRange(0.0f, 1.0f); // 0�� 1 ������ ���� �� ����

	if (AccuracyRoll <= Accuracy) // ���� ����
	{
		float MinSpread = MaxSpreadRadius * 0.1f;
		FVector RandomOffset = FMath::VRand() * FMath::RandRange(0.0f, MinSpread);
		return TargetLocation + RandomOffset;
	}
	else // ������ ����
	{
		FVector RandomOffset = FMath::VRand() * FMath::RandRange(MaxSpreadRadius * 0.3f, MaxSpreadRadius);
		return TargetLocation + RandomOffset;
	}
}

void AEnemyShooterGun::StopMuzzleFlash()
{
	if (CurrentMuzzleFlashComponent && IsValid(CurrentMuzzleFlashComponent))
	{
		CurrentMuzzleFlashComponent->DestroyComponent();
		CurrentMuzzleFlashComponent = nullptr;
	}
}

void AEnemyShooterGun::StopImpactEffect()
{
	if (CurrentImpactEffectComponent && IsValid(CurrentImpactEffectComponent))
	{
		CurrentImpactEffectComponent->DestroyComponent();
		CurrentImpactEffectComponent = nullptr;
	}
}

void AEnemyShooterGun::HideGun()
{
	UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyShooterGun Memory Cleanup"));

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // ��� Ÿ�̸� ����
	HideAimingLaserMesh(); // ������ ����
	StopMuzzleFlash(); // ����Ʈ ����
	StopImpactEffect();
	SetOwner(nullptr); // ������ ���� ����

	// ��� ������Ʈ �ı�
	if (GunMesh && IsValid(GunMesh) && !GunMesh->IsBeingDestroyed()) GunMesh->DestroyComponent();
	if (AimingLaserMesh && IsValid(AimingLaserMesh) && !AimingLaserMesh->IsBeingDestroyed()) AimingLaserMesh->DestroyComponent();
	if (MuzzleSocket && IsValid(MuzzleSocket) && !MuzzleSocket->IsBeingDestroyed()) MuzzleSocket->DestroyComponent();

	// ���� ��ü ��Ȱ��ȭ �� ����
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyShooterGun>(this)]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
			{
				WeakThis->Destroy();
			}
		});
}