#include "Rifle.h"
#include "Enemy.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h" // 나이아가라 함수 라이브러리 추가
#include "NiagaraComponent.h" // 나이아가라 컴포넌트 추가

ARifle::ARifle()
{
	PrimaryActorTick.bCanEverTick = false;

	RifleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RifleMesh"));
	RootComponent = RifleMesh;

	if (RifleMesh)
	{
		RifleMesh->SetSimulatePhysics(false);
		RifleMesh->SetEnableGravity(false);
		RifleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 머즐 소켓 컴포넌트 생성 및 설정
	MuzzleSocket = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MuzzleSocket"));
	MuzzleSocket->SetupAttachment(RifleMesh);

	if (MuzzleSocket)
	{
		MuzzleSocket->SetSimulatePhysics(false);
		MuzzleSocket->SetEnableGravity(false);
		MuzzleSocket->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MuzzleSocket->SetVisibility(false); // 에디터에서만 보이도록 설정
	}

	CurrentMuzzleFlashComponent = nullptr; // 머즐 플래시 컴포넌트 초기화
	CurrentImpactEffectComponent = nullptr; // 히트 이펙트 컴포넌트 초기화
}

void ARifle::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner())
	{
		UE_LOG(LogTemp, Warning, TEXT("Rifle Owner: %s"), *GetOwner()->GetName());
	}
}

void ARifle::Fire(float CrosshairSpreadAngle)
{
	if (bIsReloading) // 재장전 중이면 사격 불가
	{
		return;
	}

	if (!bCanFire)
	{
		return;
	}

	if (CurrentAmmo <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Ammo! Need to Reload."));
		Reload();
		return;
	}

	if (!GetOwner())
	{
		UE_LOG(LogTemp, Error, TEXT("Rifle has NO OWNER!"));
		return;
	}

	AController* OwnerController = GetOwner()->GetInstigatorController();
	if (!OwnerController)
	{
		UE_LOG(LogTemp, Error, TEXT("No OwnerController found!"));
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(OwnerController);
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("Owner is not a PlayerController!"));
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	FVector ShotDirection = CameraRotation.Vector();
	FVector End = CameraLocation + (ShotDirection * Range);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, End, ECC_Visibility, QueryParams);

	if (bHit)
	{
		if (HitResult.GetActor())
		{
			ProcessHit(HitResult, ShotDirection);

			if (BulletTrail)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					BulletTrail,
					HitResult.ImpactPoint
				);
			}

			UE_LOG(LogTemp, Warning, TEXT("Hit detected! Target: %s, Location: %s"),
				*HitResult.GetActor()->GetName(), *HitResult.ImpactPoint.ToString());

			//DrawDebugLine(GetWorld(), CameraLocation, HitResult.ImpactPoint, FColor::Green, false, 2.0f, 0, 2.0f);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("HitResult.GetActor() is NULL!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No hit detected!"));
		//DrawDebugLine(GetWorld(), CameraLocation, End, FColor::Red, false, 2.0f, 0, 2.0f);
	}

	// 부착된 위치에 새로운 이펙트 생성
	if (MuzzleFlash && MuzzleSocket)
	{
		// 기존 이펙트가 있다면 제거
		if (CurrentMuzzleFlashComponent && IsValid(CurrentMuzzleFlashComponent))
		{
			CurrentMuzzleFlashComponent->DestroyComponent();
			CurrentMuzzleFlashComponent = nullptr;
		}

		// 부착된 위치에 새로운 이펙트 생성
		FVector MuzzleLocation = MuzzleSocket->GetComponentLocation();
		FRotator MuzzleRotation = MuzzleSocket->GetComponentRotation();

		CurrentMuzzleFlashComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleFlash,
			MuzzleSocket, // 부모 컴포넌트 지정
			NAME_None, // 소켓 이름 (없음)
			FVector::ZeroVector, // 로컬 오프셋 (부모 기준 상대 위치)
			FRotator::ZeroRotator, // 로컬 회전 (부모 기준 상대 회전)
			EAttachLocation::SnapToTarget, // 부착 규칙
			true // 부모 소멸 시 자동 정리
		);

		if (CurrentMuzzleFlashComponent)
		{
			CurrentMuzzleFlashComponent->SetWorldScale3D(FVector(MuzzleFlashScale));
			CurrentMuzzleFlashComponent->SetFloatParameter(FName("PlayRate"), MuzzleFlashPlayRate);

			// 머즐 플래시 타이머 설정
			GetWorldTimerManager().SetTimer(
				MuzzleFlashTimerHandle,
				this,
				&ARifle::StopMuzzleFlash,
				MuzzleFlashDuration,
				false
			);

			UE_LOG(LogTemp, Warning, TEXT("Attached Muzzle Flash spawned and will stop in %f seconds"), MuzzleFlashDuration);
		}
	}

	// 사운드도 머즐 위치에서 재생
	if (FireSound && MuzzleSocket)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound,
			MuzzleSocket->GetComponentLocation());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FireSound is NULL! Check if it's set in BP."));
	}

	CurrentAmmo--;
	bCanFire = false;
	GetWorldTimerManager().SetTimer(FireRateTimerHandle, this, &ARifle::ResetFire, FireRate, false);
}

void ARifle::StopMuzzleFlash()
{
	if (CurrentMuzzleFlashComponent && IsValid(CurrentMuzzleFlashComponent))
	{
		CurrentMuzzleFlashComponent->Deactivate();
		CurrentMuzzleFlashComponent->DestroyComponent();
		CurrentMuzzleFlashComponent = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Muzzle Flash stopped and destroyed"));
	}
}

void ARifle::StopImpactEffect()
{
	if (CurrentImpactEffectComponent && IsValid(CurrentImpactEffectComponent))
	{
		CurrentImpactEffectComponent->Deactivate();
		CurrentImpactEffectComponent->DestroyComponent();
		CurrentImpactEffectComponent = nullptr;

		UE_LOG(LogTemp, Warning, TEXT("Niagara Impact Effect stopped"));
	}
}

// 재장전 기능 구현
void ARifle::Reload()
{
	if (bIsReloading || CurrentAmmo == MaxAmmo || TotalAmmo <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot Reload!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Reloading..."));

	bIsReloading = true; // 재장전 시작

	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}

	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ARifle::FinishReload, ReloadTime, false);
}

// 재장전 완료 처리
void ARifle::FinishReload()
{
	int32 NeededAmmo = MaxAmmo - CurrentAmmo;
	int32 AmmoToReload = FMath::Min(TotalAmmo, NeededAmmo);

	CurrentAmmo += AmmoToReload;
	TotalAmmo -= AmmoToReload;

	UE_LOG(LogTemp, Warning, TEXT("Reload Complete! Current Ammo: %d, Total Ammo: %d"), CurrentAmmo, TotalAmmo);

	bIsReloading = false; // 재장전 완료
}

void ARifle::ResetFire()
{
	bCanFire = true;
}

void ARifle::ProcessHit(const FHitResult& HitResult, FVector ShotDirection)
{
	AActor* HitActor = HitResult.GetActor();
	if (HitActor)
	{
		float AppliedDamage = (HitResult.BoneName == "Head") ? Damage * 100.0f : Damage;

		UGameplayStatics::ApplyPointDamage(HitActor, AppliedDamage, ShotDirection, HitResult, GetOwner()->GetInstigatorController(), this, UDamageType::StaticClass());

		// Impact Effect - 머즐 플래시처럼 조기 종료 기능 추가
		if (ImpactEffect)
		{
			// 이전 Impact Effect가 있다면 제거
			if (CurrentImpactEffectComponent && IsValid(CurrentImpactEffectComponent))
			{
				CurrentImpactEffectComponent->DestroyComponent();
				CurrentImpactEffectComponent = nullptr;
			}

			FVector ImpactLocation = HitResult.ImpactPoint;
			FRotator ImpactRotation = (-ShotDirection).Rotation(); // 충돌 방향으로 회전

			CurrentImpactEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				ImpactEffect,
				ImpactLocation,
				ImpactRotation
			);

			if (CurrentImpactEffectComponent)
			{
				// Impact Effect 타이머 설정 - 짧게 재생 후 종료
				GetWorldTimerManager().SetTimer(
					ImpactEffectTimerHandle,
					this,
					&ARifle::StopImpactEffect,
					ImpactEffectDuration,
					false
				);

				UE_LOG(LogTemp, Warning, TEXT("Niagara Impact Effect spawned at: %s, Duration: %f"),
					*ImpactLocation.ToString(), ImpactEffectDuration);
			}
		}
	}
}