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

	RifleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RifleMesh")); // 스태틱 메쉬 컴포넌트 생성
	RootComponent = RifleMesh; // 이 메쉬를 루트 컴포넌트로 설정

	if (RifleMesh)
	{
		RifleMesh->SetSimulatePhysics(false); // 물리 시뮬레이션 비활성화
		RifleMesh->SetEnableGravity(false); // 중력 영향 비활성화
		RifleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 비활성화
	}

	MuzzleSocket = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MuzzleSocket")); // 머즐 소켓 메쉬 컴포넌트 생성
	MuzzleSocket->SetupAttachment(RifleMesh); // 라이플 메쉬에 이 메쉬를 부착

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

	if (GetOwner()) // 소유자 확인
	{
		UE_LOG(LogTemp, Warning, TEXT("Rifle Owner: %s"), *GetOwner()->GetName()); // 소유자 이름 로그출력
	}
}

void ARifle::Fire(float CrosshairSpreadAngle)
{
	if (bIsReloading || !bCanFire) // 재장전 중이거나 격발 불가 상태이면
	{
		return; // 리턴
	}

	if (CurrentAmmo <= 0) // 현재 총알 수 가 0 이라면
	{
		UE_LOG(LogTemp, Warning, TEXT("No Ammo! Need to Reload."));
		Reload(); // 재장전
		return;
	}

	if (!GetOwner()) // 소유자가 없다면
	{
		UE_LOG(LogTemp, Error, TEXT("Rifle has NO OWNER!"));
		return; // 리턴
	}

	AController* OwnerController = GetOwner()->GetInstigatorController(); // 총의 소유자를 조종하는 컨트롤러를 가져옴
	
	if (!OwnerController) // 컨트롤러가 없다면
	{
		UE_LOG(LogTemp, Error, TEXT("No OwnerController found!"));
		return; // 리턴
	}

	APlayerController* PlayerController = Cast<APlayerController>(OwnerController); // 가져온 컨트롤러를 플레이어 컨트롤러로 변환
	if (!PlayerController) // 변환에 실패했다면
	{
		UE_LOG(LogTemp, Error, TEXT("Owner is not a PlayerController!"));
		return; // 리턴
	}

	FVector CameraLocation; // 카메라의 월드 위치 저장
	FRotator CameraRotation; // 카메라의 월드 회전값 저장
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation); // 플레이어 카메라의 위치와 회전값을 가져옴

	FVector ShotDirection = CameraRotation.Vector(); // 카메라 회전값으로 부터 정면 방향 벡터를 계산
	FVector End = CameraLocation + (ShotDirection * Range); // 래이캐스트 끝점 = 발사 시작점 + 사정거리만큼의 방향 벡터

	FHitResult HitResult; // 래이캐스트의 충돌 결과를 저장
	FCollisionQueryParams QueryParams; // 래이캐스트 추가 옵션을 설정
	QueryParams.AddIgnoredActor(this); // 이 액터는 래이캐스트 히트를 무시 
	QueryParams.AddIgnoredActor(GetOwner()); // 소유자도 래이캐스트 히트를 무시
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, End, ECC_Visibility, QueryParams); // 월드에서 지정된 채널을 따라 래이캐스트를 수행하고 충돌 여부를 bHit에 저장

	if (bHit) // 래이캐스트에 뭔가 맞았다면
	{
		if (HitResult.GetActor()) // 래이캐스트 결과 유효한 대상이라면
		{
			ProcessHit(HitResult, ShotDirection); // 히트처리 함수 호출

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
	if (CurrentMuzzleFlashComponent && IsValid(CurrentMuzzleFlashComponent)) // 현재 머즐임팩트가 유효한지 확인
	{
		CurrentMuzzleFlashComponent->Deactivate(); // 비활성화
		CurrentMuzzleFlashComponent->DestroyComponent(); // 파괴후 메모리에서 제거
		CurrentMuzzleFlashComponent = nullptr; // 포인터 초기화
		UE_LOG(LogTemp, Warning, TEXT("Muzzle Flash stopped and destroyed"));
	}
}

void ARifle::StopImpactEffect()
{
	if (CurrentImpactEffectComponent && IsValid(CurrentImpactEffectComponent)) // 현재 피격임팩트가 유효한지 확인
	{
		CurrentImpactEffectComponent->Deactivate(); // 비활성화 
		CurrentImpactEffectComponent->DestroyComponent(); // 파괴 후 메모리에서 제거 
		CurrentImpactEffectComponent = nullptr; // 포인터 초기화
		UE_LOG(LogTemp, Warning, TEXT("Niagara Impact Effect stopped"));
	}
}

void ARifle::Reload()
{
	if (bIsReloading || CurrentAmmo == MaxAmmo || TotalAmmo <= 0) // 재장전중이거나 총알이 가득 찼거나 전체총알이 없다면
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot Reload!"));
		return; // 리턴
	}

	UE_LOG(LogTemp, Warning, TEXT("Reloading..."));

	bIsReloading = true; // 재장전 시작

	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}

	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ARifle::FinishReload, ReloadTime, false); // 재장전 시간만큼 기다린 후 FinishReload 함수를 호출핟록 타이머 설정
}

void ARifle::FinishReload()
{
	int32 NeededAmmo = MaxAmmo - CurrentAmmo; // 필요한 총알 수계산
	int32 AmmoToReload = FMath::Min(TotalAmmo, NeededAmmo); // 보유한 총알과 필요한 총알중 더 작은 값으로 재장전할 수를 결정

	CurrentAmmo += AmmoToReload; // 현재 총알 수에 재장전할 총알 수를 더함
	TotalAmmo -= AmmoToReload; // 전체 총알 수에서 재장전할 만큼 뺌

	UE_LOG(LogTemp, Warning, TEXT("Reload Complete! Current Ammo: %d, Total Ammo: %d"), CurrentAmmo, TotalAmmo);

	bIsReloading = false; // 재장전 완료
}

void ARifle::ResetFire()
{
	bCanFire = true; // 사격 가능여부를 true로 전환
}

void ARifle::ProcessHit(const FHitResult& HitResult, FVector ShotDirection)
{
	AActor* HitActor = HitResult.GetActor(); // 충돌 결과에서 맞은 액터를 가져옴
	if (HitActor) // 맞은 액터가 유효하다면
	{
		/*if (HitResult.BoneName == "Head" || HitResult.BoneName == "CC_Base_Head")
		{
			float AppliedDamage = Damage * 100.0f;
		}
		else
		{
			float sAppliedDamage = Damage;
		}*/

		float AppliedDamage = (HitResult.BoneName == "Head" || HitResult.BoneName == "CC_Base_Head") ? Damage * 100.0f : Damage;
		// 맞은 부위의 본 이름이 Head 또는 CC_Base_Head라면 100배의 데미지를 아니면 기본 데미지를 적용 (인간형 적 ai 머리 본네임들)
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