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
}

void AEnemyShooterGun::BeginPlay()
{
	Super::BeginPlay();

	// 플레이어 폰 캐싱
	UWorld* World = GetWorld();
	if (World)
	{
		CachedPlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0); // 플레이어 폰 캐싱
		LastPlayerCacheTime = World->GetTimeSeconds(); // 캐싱 시간 초기화

		if (!CachedPlayerPawn) // 플레이어 폰이 유효하지 않다면
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemyShooterGun: Failed to cache PlayerPawn"));
		}
	}

	// 레이저 메쉬 에셋 및 머티리얼 설정
	if (AimingLaserMeshAsset && AimingLaserMesh) // 레이저 메쉬 컴포넌트와 에셋이 모두 유효하다면
	{
		AimingLaserMesh->SetStaticMesh(AimingLaserMeshAsset); // 레이저 메쉬 에셋 적용
		if (AimingLaserMaterial) // 레이저 머티리얼이 설정되어 있다면 적용
		{
			AimingLaserMesh->SetMaterial(0, AimingLaserMaterial); // 레이저 머티리얼 적용
		}
	}
}

void AEnemyShooterGun::FireGun()
{
	if (!IsValid(GetOwner())) return;

	// 플레이어 캐시 갱신 시도
	UpdateCachedPlayerPawn(); // 플레이어 캐시가 유효하지 않다면 갱신 시도
	if (!IsValid(CachedPlayerPawn)) return; // 여전히 없으면 중단

	UWorld* World = GetWorld();
	if (!World) return;

	FVector MuzzleLocation; // 총구 위치 계산
	if (MuzzleSocket)
	{
		// 소켓 컴포넌트가 있다면 그 위치를 사용
		MuzzleLocation = MuzzleSocket->GetComponentLocation();
	}
	else
	{
		// 없다면 총기 메쉬 자체의 중심 위치를 사용
		MuzzleLocation = GunMesh->GetComponentLocation();
	}

	FVector PredictedPlayerLocation = CalculatePredictedPlayerPosition(); // 플레이어의 미래 위치 예측

	// 지연 발사를 위해 현재 총구 위치와 목표 위치를 저장
	StoredMuzzleLocation = MuzzleLocation;
	StoredTargetLocation = PredictedPlayerLocation;

	if (bShowAimWarning) // 조준 경고 옵션이 켜져 있다면
	{
		ShowAimingLaserMesh(MuzzleLocation, PredictedPlayerLocation); // 레이저 조준선 표시
		if (AimWarningSound) // 경고 사운드 재생
		{
			UGameplayStatics::PlaySoundAtLocation(World, AimWarningSound, MuzzleLocation, 0.7f);
		}

		TWeakObjectPtr<AEnemyShooterGun> WeakThis(this); // 약참조 생성
		World->GetTimerManager().SetTimer( // AimWarningTime 후에 ExecuteDelayedShot 함수를 호출하는 타이머
			DelayedShotTimerHandle,
			[WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->ExecuteDelayedShot();
				}
			}, AimWarningTime, false); // 단발성
	}
	else // 조준 경고 옵션이 꺼져 있다면
	{
		ExecuteDelayedShot(); // 즉시 발사
	}
}

void AEnemyShooterGun::ExecuteDelayedShot()
{
	HideAimingLaserMesh(); // 레이저 조준선 숨김

	UWorld* World = GetWorld();
	if (!World) return;

	AEnemyShooter* OwnerShooter = Cast<AEnemyShooter>(GetOwner()); // 소유자 캐스팅

	if (OwnerShooter) // 소유자가 AEnemyShooter라면
	{
		OwnerShooter->PlayShootingAnimation(); // 소유자에게 발사 애니메이션 재생 요청
	}

	// 명중률을 적용하여 최종 목표 위치 결정
	FVector FinalTargetLocation = ApplyAccuracySpread(StoredTargetLocation); // 명중률에 따른 탄착군 형성
	FVector ForwardDirection = (FinalTargetLocation - StoredMuzzleLocation).GetSafeNormal(); // 총구에서 최종 목표 방향 계산
	FVector EndLocation = StoredMuzzleLocation + (ForwardDirection * FireRange); // 최대 사정거리까지의 끝점

	FHitResult HitResult; // 라인 트레이스 결과를 저장할 변수
	bool bHit = PerformLineTrace(StoredMuzzleLocation, EndLocation, HitResult); // 라인 트레이스 실행 및 결과 저장
	PlayMuzzleFlash(StoredMuzzleLocation); // 총구 섬광 이펙트 재생

	if (bHit) // 라인 트레이스에 무언가 맞았다면
	{
		PlayImpactEffect(HitResult.Location, HitResult.Normal); // 피격 지점에 이펙트 재생

		if (CachedPlayerPawn && HitResult.GetActor() == CachedPlayerPawn) // 맞은 대상이 플레이어라면
		{
			UGameplayStatics::ApplyDamage(
				HitResult.GetActor(), Damage, GetOwner()->GetInstigatorController(), this, UDamageType::StaticClass()); // 데미지 적용
		}
	}

	if (FireSound) // 격발 사운드 재생
	{
		UGameplayStatics::PlaySoundAtLocation(World, FireSound, StoredMuzzleLocation); // 사운드 재생
	}
}

void AEnemyShooterGun::ShowAimingLaserMesh(FVector StartLocation, FVector EndLocation)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (AimingLaserMesh && AimingLaserMeshAsset)
	{
		SetupLaserTransform(StartLocation, EndLocation); // 레이저의 위치, 회전, 크기 설정
		AimingLaserMesh->SetVisibility(true); // 보이게 설정

		// AimWarningTime 후에 자동으로 레이저를 숨기도록 타이머 설정
		TWeakObjectPtr<AEnemyShooterGun> WeakThis(this); // 약참조 생성
		World->GetTimerManager().SetTimer(
			AimingLaserTimerHandle,
			[WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HideAimingLaserMesh(); // 레이저 숨김
				}
			}, AimWarningTime, false); // 단발성
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
	if (AimingLaserMesh)
	{
		AimingLaserMesh->SetWorldLocation(FinalLocation); // 위치 설정
		AimingLaserMesh->SetWorldRotation(FinalRotation); // 회전 설정
		AimingLaserMesh->SetWorldScale3D(FinalScale); // 크기 설정
	}
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
	UWorld* World = GetWorld();
	if (!World) return false;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	CollisionParams.AddIgnoredActor(GetOwner());
	CollisionParams.bTraceComplex = true;

	//// 여러 채널에 대해 순서대로 검사하여 하나라도 맞으면 true 반환
	//return GetWorld()->LineTraceSingleByChannel(
	//	OutHitResult, StartLocation, EndLocation, ECollisionChannel::ECC_WorldStatic, CollisionParams
	//) || GetWorld()->LineTraceSingleByChannel(
	//	OutHitResult, StartLocation, EndLocation, ECollisionChannel::ECC_WorldDynamic, CollisionParams
	//) || GetWorld()->LineTraceSingleByChannel(
	//	OutHitResult, StartLocation, EndLocation, ECollisionChannel::ECC_Pawn, CollisionParams
	//);

	// ECC_Visibility는 WorldStatic, WorldDynamic, Pawn 모두 감지
	return World->LineTraceSingleByChannel(
		OutHitResult, StartLocation, EndLocation,
		ECC_Visibility, CollisionParams
	);
}

void AEnemyShooterGun::UpdateCachedPlayerPawn()
{
	UWorld* World = GetWorld();
	if (!World) return;

	float CurrentTime = World->GetTimeSeconds();

	// PlayerCahceInterval 초 마다 한 번만 플레이어 검색
	// 캐시된 플레이어 폰이 유효하고, 마지막 캐시 이후로 충분한 시간이 지나지 않았다면
	if (CurrentTime - LastPlayerCacheTime < PlayerCacheInterval && IsValid(CachedPlayerPawn))
	{
		return; // 아직 캐시 유효
	}

	CachedPlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0); // 플레이어 폰 검색 및 캐싱
	LastPlayerCacheTime = CurrentTime; // 캐시 시간 갱신

	if (!CachedPlayerPawn) // 플레이어 폰이 여전히 검색되지 않았다면
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyShooterGun: PlayerPawn not found")); 
	}
}

void AEnemyShooterGun::PlayMuzzleFlash(FVector Location)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 총구 섬광 이펙트 재생 로직
	if (MuzzleFlash)
	{
		CurrentMuzzleFlashComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation( 
			World, MuzzleFlash, Location, GetActorRotation(), FVector(MuzzleFlashScale)); // 이펙트 스폰 및 크기 조절
		if (CurrentMuzzleFlashComponent) // 이펙트가 유효하게 생성되었다면
		{
			CurrentMuzzleFlashComponent->SetFloatParameter(TEXT("PlayRate"), MuzzleFlashPlayRate); // 재생 속도 조절
			TWeakObjectPtr<AEnemyShooterGun> WeakThis(this); // 약참조 생성
			World->GetTimerManager().SetTimer( // MuzzleFlashDuration 후에 StopMuzzleFlash 함수를 호출하는 타이머
				MuzzleFlashTimerHandle,
				[WeakThis]()
				{
					if (WeakThis.IsValid()) // 유효성 검사
					{
						WeakThis->StopMuzzleFlash(); // 총구 섬광 정지
					}
				}, MuzzleFlashDuration, false); // 단발성
		}
	}
}

void AEnemyShooterGun::PlayImpactEffect(FVector Location, FVector ImpactNormal)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 피격 이펙트 재생 로직
	if (ImpactEffect) // 이펙트가 설정되어 있다면
	{
		// 피격 표면의 벡터를 회전값으로 사용하여 이펙트가 표면에 맞게 방향을 잡도록 함
		CurrentImpactEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, ImpactEffect, Location, ImpactNormal.Rotation());
		if (CurrentImpactEffectComponent) // 이펙트가 유효하게 생성되었다면
		{
			TWeakObjectPtr<AEnemyShooterGun> WeakThis(this); // 약참조 생성
			World->GetTimerManager().SetTimer(
				ImpactEffectTimerHandle,
				[WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->StopImpactEffect(); // 피격 이펙트 정지
					}
				}, ImpactEffectDuration, false); // 단발성
		}
	}
}

FVector AEnemyShooterGun::CalculatePredictedPlayerPosition()
{
	if (!CachedPlayerPawn) return FVector::ZeroVector; // 플레이어 폰이 유효하지 않으면 원점 반환
	FVector CurrentLocation = CachedPlayerPawn->GetActorLocation(); // 플레이어의 현재 위치 가져오기
	FVector CurrentVelocity = CachedPlayerPawn->GetVelocity(); // 플레이어의 현재 속도 가져오기
	// 등속 직선 운동 방정식
	// (미래 위치) = (현재 위치) + (현재 속도) * (예측 시간)
	return CurrentLocation + (CurrentVelocity * PredictionTime);
}

FVector AEnemyShooterGun::ApplyAccuracySpread(FVector TargetLocation)
{
	float AccuracyRoll = FMath::RandRange(0.0f, 1.0f); // 0과 1 사이의 랜덤 값 생성

	if (AccuracyRoll <= Accuracy) // 명중 판정
	{
		float MinSpread = MaxSpreadRadius * 0.1f; // 명중 시 최소 탄착군 반경 설정 (최대 반경의 10%로 설정)
		FVector RandomOffset = FMath::VRand() * FMath::RandRange(0.0f, MinSpread); // 명중 시에는 최대 반경의 10% 이내에서 랜덤 오프셋 생성
		return TargetLocation + RandomOffset; // 명중 시에는 목표 위치에 작은 랜덤 오프셋을 더하여 탄착군 형성
	}
	else // 빗나감 판정
	{
		FVector RandomOffset = FMath::VRand() * FMath::RandRange(MaxSpreadRadius * 0.3f, MaxSpreadRadius); // 빗나감 시에는 최대 반경의 30%에서 100% 사이에서 랜덤 오프셋 생성하여 더 넓은 탄착군 형성
		return TargetLocation + RandomOffset; // 빗나감 시에는 목표 위치에 더 큰 랜덤 오프셋을 더하여 탄착군 형성
	}
}

void AEnemyShooterGun::StopMuzzleFlash()
{
	if (CurrentMuzzleFlashComponent && IsValid(CurrentMuzzleFlashComponent)) // 현재 재생 중인 총구 섬광 컴포넌트가 유효하다면
	{
		CurrentMuzzleFlashComponent->DestroyComponent(); // 컴포넌트 파괴하여 이펙트 정지
		CurrentMuzzleFlashComponent = nullptr; // 참조 해제
	}
}

void AEnemyShooterGun::StopImpactEffect()
{
	if (CurrentImpactEffectComponent && IsValid(CurrentImpactEffectComponent)) // 현재 재생 중인 피격 이펙트 컴포넌트가 유효하다면
	{
		CurrentImpactEffectComponent->DestroyComponent(); // 컴포넌트 파괴하여 이펙트 정지
		CurrentImpactEffectComponent = nullptr; // 참조 해제
	}
}

void AEnemyShooterGun::HideGun()
{
	UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyShooterGun Memory Cleanup"));

	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 정리
	HideAimingLaserMesh(); // 레이저 숨김
	StopMuzzleFlash(); // 이펙트 정지
	StopImpactEffect();
	SetOwner(nullptr); // 소유자 참조 해제

	// 모든 컴포넌트 파괴
	if (IsValid(GunMesh) && !GunMesh->IsBeingDestroyed()) GunMesh->DestroyComponent();
	if (AimingLaserMesh && IsValid(AimingLaserMesh) && !AimingLaserMesh->IsBeingDestroyed()) AimingLaserMesh->DestroyComponent();
	if (MuzzleSocket && IsValid(MuzzleSocket) && !MuzzleSocket->IsBeingDestroyed()) MuzzleSocket->DestroyComponent();

	// 액터 자체 비활성화 및 제거
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	TWeakObjectPtr<AEnemyShooterGun> WeakThis(this); // 약참조 생성
	World->GetTimerManager().SetTimerForNextTick(
		[WeakThis]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
			{
				WeakThis->Destroy();
			}
		});
}