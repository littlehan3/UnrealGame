#include "EnemyShooterGun.h"
#include "EnemyShooter.h" // 소유자 클래스 참조
#include "Engine/World.h" // UWorld 사용
#include "GameFramework/DamageType.h" // UDamageType 사용
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수 사용
#include "DrawDebugHelpers.h" // 디버그 시각화 기능 사용
#include "Sound/SoundBase.h" // USoundBase 사용
#include "NiagaraSystem.h" // 나이아가라 시스템 사용
#include "NiagaraComponent.h" // 나이아가라 컴포넌트 사용
#include "NiagaraFunctionLibrary.h" // 나이아가라 이펙트 스폰 함수 사용
#include "GameFramework/PlayerController.h" // 플레이어 컨트롤러 참조
#include "GameFramework/Pawn.h" // APawn 참조
#include "Materials/MaterialInterface.h" // 머티리얼 인터페이스 사용

AEnemyShooterGun::AEnemyShooterGun()
{
	PrimaryActorTick.bCanEverTick = false; // 매 프레임 Tick 불필요

	// 기본 컴포넌트 생성 및 설정
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
	MuzzleSocket->SetVisibility(false); // 게임에서는 보이지 않음

	// 레이저 조준선 컴포넌트 생성
	AimingLaserMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AimingLaserMesh"));
	AimingLaserMesh->SetupAttachment(RootComponent);
	AimingLaserMesh->SetVisibility(false); // 기본적으로 숨김
	AimingLaserMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 포인터 멤버 변수 초기화
	CurrentMuzzleFlashComponent = nullptr;
	CurrentImpactEffectComponent = nullptr;
}

void AEnemyShooterGun::BeginPlay()
{
	Super::BeginPlay();

	// 레이저 메쉬 에셋 및 머티리얼 설정
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
	FVector PredictedPlayerLocation = CalculatePredictedPlayerPosition(PlayerPawn); // 플레이어의 미래 위치 예측

	// 지연 발사를 위해 현재 총구 위치와 목표 위치를 저장
	StoredMuzzleLocation = MuzzleLocation;
	StoredTargetLocation = PredictedPlayerLocation;

	if (bShowAimWarning) // 조준 경고 옵션이 켜져 있다면
	{
		ShowAimingLaserMesh(MuzzleLocation, PredictedPlayerLocation); // 레이저 조준선 표시
		if (AimWarningSound) // 경고 사운드 재생
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), AimWarningSound, MuzzleLocation, 0.7f);
		}

		// AimWarningTime 후에 ExecuteDelayedShot 함수를 호출하도록 타이머 설정
		GetWorld()->GetTimerManager().SetTimer(
			DelayedShotTimerHandle, this, &AEnemyShooterGun::ExecuteDelayedShot, AimWarningTime, false
		);
	}
	else // 조준 경고 옵션이 꺼져 있다면
	{
		ExecuteDelayedShot(); // 즉시 발사
	}
}

void AEnemyShooterGun::ExecuteDelayedShot()
{
	HideAimingLaserMesh(); // 레이저 조준선 숨김

	// 실제 발사 시점에 맞춰 소유자(Shooter)의 사격 애니메이션을 재생
	if (AEnemyShooter* OwnerShooter = Cast<AEnemyShooter>(GetOwner()))
	{
		OwnerShooter->PlayShootingAnimation();
	}

	// 명중률을 적용하여 최종 목표 위치 결정
	FVector FinalTargetLocation = ApplyAccuracySpread(StoredTargetLocation);
	FVector ForwardDirection = (FinalTargetLocation - StoredMuzzleLocation).GetSafeNormal();
	FVector EndLocation = StoredMuzzleLocation + (ForwardDirection * FireRange); // 최대 사정거리까지의 끝점

	// 히트스캔: 총구에서 최종 목표 방향으로 라인 트레이스 실행
	FHitResult HitResult;
	bool bHit = PerformLineTrace(StoredMuzzleLocation, EndLocation, HitResult);

	PlayMuzzleFlash(StoredMuzzleLocation); // 총구 섬광 이펙트 재생

	if (bHit) // 라인 트레이스에 무언가 맞았다면
	{
		PlayImpactEffect(HitResult.Location, HitResult.Normal); // 피격 지점에 이펙트 재생

		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (PlayerPawn && HitResult.GetActor() == PlayerPawn) // 맞은 대상이 플레이어라면
		{
			UGameplayStatics::ApplyDamage( // 데미지 적용
				HitResult.GetActor(), Damage, GetOwner()->GetInstigatorController(), this, UDamageType::StaticClass()
			);
		}
	}

	if (FireSound) // 격발 사운드 재생
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, StoredMuzzleLocation);
	}
}

void AEnemyShooterGun::ShowAimingLaserMesh(FVector StartLocation, FVector EndLocation)
{
	if (AimingLaserMesh && AimingLaserMeshAsset)
	{
		SetupLaserTransform(StartLocation, EndLocation); // 레이저의 위치, 회전, 크기 설정
		AimingLaserMesh->SetVisibility(true); // 보이게 설정

		// AimWarningTime 후에 자동으로 레이저를 숨기도록 타이머 설정
		GetWorld()->GetTimerManager().SetTimer(
			AimingLaserTimerHandle, this, &AEnemyShooterGun::HideAimingLaserMesh, AimWarningTime, false
		);
	}
}

void AEnemyShooterGun::SetupLaserTransform(FVector StartLocation, FVector EndLocation)
{
	if (!AimingLaserMesh) return;

	float Distance = FVector::Dist(StartLocation, EndLocation); // 시작점과 끝점 사이의 거리
	FVector Direction = (EndLocation - StartLocation).GetSafeNormal(); // 방향 벡터
	FVector MidPoint = StartLocation + (Direction * Distance * 0.5f); // 두 점의 중간 지점
	FRotator LookRotation = Direction.Rotation(); // 방향 벡터를 회전값으로 변환

	FVector FinalLocation = MidPoint + AimingLaserOffset; // 오프셋 적용된 최종 위치
	FRotator FinalRotation = LookRotation + AimingLaserRotationOffset; // 오프셋 적용된 최종 회전
	FVector FinalScale = FVector(Distance / 100.0f, AimingLaserScale, AimingLaserScale); // X축을 거리에 맞게 조절

	// 계산된 Transform 적용
	AimingLaserMesh->SetWorldLocation(FinalLocation);
	AimingLaserMesh->SetWorldRotation(FinalRotation);
	AimingLaserMesh->SetWorldScale3D(FinalScale);
}

void AEnemyShooterGun::HideAimingLaserMesh()
{
	if (AimingLaserMesh)
	{
		AimingLaserMesh->SetVisibility(false); // 보이지 않게 설정
	}
}

bool AEnemyShooterGun::PerformLineTrace(FVector StartLocation, FVector EndLocation, FHitResult& OutHitResult)
{
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	CollisionParams.AddIgnoredActor(GetOwner());
	CollisionParams.bTraceComplex = true;

	// 여러 채널에 대해 순서대로 검사하여 하나라도 맞으면 true 반환
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
	// 총구 섬광 이펙트 재생 로직
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
	// 피격 이펙트 재생 로직
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
	// (미래 위치) = (현재 위치) + (현재 속도) * (예측 시간)
	return CurrentLocation + (CurrentVelocity * PredictionTime);
}

FVector AEnemyShooterGun::ApplyAccuracySpread(FVector TargetLocation)
{
	float AccuracyRoll = FMath::RandRange(0.0f, 1.0f); // 0과 1 사이의 랜덤 값 생성

	if (AccuracyRoll <= Accuracy) // 명중 판정
	{
		float MinSpread = MaxSpreadRadius * 0.1f;
		FVector RandomOffset = FMath::VRand() * FMath::RandRange(0.0f, MinSpread);
		return TargetLocation + RandomOffset;
	}
	else // 빗나감 판정
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

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 정리
	HideAimingLaserMesh(); // 레이저 숨김
	StopMuzzleFlash(); // 이펙트 정지
	StopImpactEffect();
	SetOwner(nullptr); // 소유자 참조 해제

	// 모든 컴포넌트 파괴
	if (GunMesh && IsValid(GunMesh) && !GunMesh->IsBeingDestroyed()) GunMesh->DestroyComponent();
	if (AimingLaserMesh && IsValid(AimingLaserMesh) && !AimingLaserMesh->IsBeingDestroyed()) AimingLaserMesh->DestroyComponent();
	if (MuzzleSocket && IsValid(MuzzleSocket) && !MuzzleSocket->IsBeingDestroyed()) MuzzleSocket->DestroyComponent();

	// 액터 자체 비활성화 및 제거
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