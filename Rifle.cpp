#include "Rifle.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
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
		MuzzleSocket->SetSimulatePhysics(false); // 물리 시뮬레이션 비활성화
		MuzzleSocket->SetEnableGravity(false); // 중력 영향 비활성화
		MuzzleSocket->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 비활성화
		MuzzleSocket->SetVisibility(false); // 렌더링 제외 (위치 정보용
	}

	CurrentMuzzleFlashComponent = nullptr; // 머즐 플래시 컴포넌트 초기화
	CurrentImpactEffectComponent = nullptr; // 히트 이펙트 컴포넌트 초기화
}

void ARifle::AddTotalAmmo(int32 AmmoToAdd)
{
	// 음수 값이 들어와도 처리 가능하도록 FMath::Max로 0 이하로 내려가지 않도록 합니다.
	TotalAmmo = FMath::Max(0, TotalAmmo + AmmoToAdd);

	// 로그 출력 (디버깅용)
	UE_LOG(LogTemp, Warning, TEXT("ARifle received %d ammo. New Total Ammo: %d"), AmmoToAdd, TotalAmmo);
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
	UWorld* World = GetWorld(); // 월드 가져오기
	if (!World || bIsReloading || !bCanFire) return; // 유효성 및 조건검사

	// 자동 장전
	if (CurrentAmmo <= 0) // 현재 총알 수 가 0 이라면
	{
		Reload(); // 재장전
		return;
	}

	AActor* WeaponOwner = GetOwner(); // 소유자 액터 가져오기
	if (!IsValid(WeaponOwner)) return; // 소유자 없으면 리턴
	AController* OwnerController = WeaponOwner->GetInstigatorController(); // 총의 소유자를 조종하는 컨트롤러를 가져옴
	if (!IsValid(OwnerController)) return; // 컨트롤러 없으면 리턴
	APlayerController* PlayerController = Cast<APlayerController>(OwnerController); // 가져온 컨트롤러를 플레이어 컨트롤러로 변환
	if (!PlayerController)return; // 플레이어 컨트롤러 없으면 리턴

	FVector CameraLocation; // 카메라의 월드 위치 저장
	FRotator CameraRotation; // 카메라의 월드 회전값 저장
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation); // 플레이어 카메라의 위치와 회전값을 가져옴

	FVector ShotDirection = CameraRotation.Vector(); // 카메라 회전값으로 부터 정면 방향 벡터를 계산
	FVector End = CameraLocation + (ShotDirection * Range); // 래이캐스트 끝점 = 발사 시작점 + 사정거리만큼의 방향 벡터

	FHitResult HitResult; // 래이캐스트의 충돌 결과를 저장
	FCollisionQueryParams QueryParams; // 래이캐스트 추가 옵션을 설정
	QueryParams.AddIgnoredActor(this); // 이 액터는 래이캐스트 히트를 무시 
	QueryParams.AddIgnoredActor(GetOwner()); // 소유자도 래이캐스트 히트를 무시
	bool bHit = World->LineTraceSingleByChannel(HitResult, CameraLocation, End, ECC_Visibility, QueryParams); // 월드에서 지정된 채널을 따라 래이캐스트를 수행하고 충돌 여부를 bHit에 저장

	if (bHit) // 래이캐스트에 뭔가 맞았다면
	{
		if (HitResult.GetActor()) // 래이캐스트 결과 유효한 대상이라면
		{
			ProcessHit(HitResult, ShotDirection); // 히트처리 함수 호출
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
			CurrentMuzzleFlashComponent->DestroyComponent(); // 파괴후 메모리에서 제거
			CurrentMuzzleFlashComponent = nullptr; // 포인터 초기화
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

			TWeakObjectPtr<ARifle> WeakThis(this); // 약참조 생성
			World->GetTimerManager().SetTimer(
				MuzzleFlashTimerHandle,
				[WeakThis]() // 람다 캡처
				{
					if (WeakThis.IsValid()) // 유효성 검사
					{
						WeakThis->StopMuzzleFlash(); // 머즐 플래시 정지 함수 호출
					}
				},
				MuzzleFlashDuration, false); // 한 번만 실행
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

	CurrentAmmo--; // 현재 총알 수 감소
	bCanFire = false; // 사격 불가로 설정

	// 발사 속도 리셋 타이머
	TWeakObjectPtr<ARifle> WeakThis(this); // 약참조 생성
	World->GetTimerManager().SetTimer(
		FireRateTimerHandle, 
		[WeakThis]() // 람다 캡처
		{
			if (WeakThis.IsValid()) // 유효성 검사
			{
				WeakThis->ResetFire(); // 사격 가능여부 리셋 함수 호출
			}
		}, FireRate, false); // 한 번만 실행
}

void ARifle::StopMuzzleFlash()
{
	UWorld* World = GetWorld();
	if (!World) return;
	World->GetTimerManager().ClearTimer(MuzzleFlashTimerHandle); // 머즐 플래시 타이머 클리어

	if (IsValid(CurrentMuzzleFlashComponent)) // 현재 머즐임팩트가 유효한지 확인
	{
		CurrentMuzzleFlashComponent->Deactivate(); // 비활성화
		CurrentMuzzleFlashComponent->DestroyComponent(); // 파괴후 메모리에서 제거
		CurrentMuzzleFlashComponent = nullptr; // 포인터 초기화
	}
}

void ARifle::StopImpactEffect()
{
	UWorld* World = GetWorld();
	if (!World) return;
	World->GetTimerManager().ClearTimer(ImpactEffectTimerHandle); // 히트 이펙트 타이머 클리어

	if (IsValid(CurrentImpactEffectComponent)) // 현재 피격임팩트가 유효한지 확인
	{
		CurrentImpactEffectComponent->Deactivate(); // 비활성화 
		CurrentImpactEffectComponent->DestroyComponent(); // 파괴 후 메모리에서 제거 
		CurrentImpactEffectComponent = nullptr; // 포인터 초기화
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

	// ReloadSound와 ReloadAnnouncementSound를 동시에 재생합니다.
	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}
	if (ReloadAnnouncementSound) // 두 번째 사운드 재생
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadAnnouncementSound, GetActorLocation());
	}

	if (UWorld* World = GetWorld())
	{
		GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ARifle::FinishReload, ReloadTime, false); // 재장전 시간만큼 기다린 후 FinishReload 함수를 호출핟록 타이머 설정
	}
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

//void ARifle::ProcessHit(const FHitResult& HitResult, FVector ShotDirection)
//{
//	UWorld* World = GetWorld(); // 월드 가져오기
//	if (!World) return; // 월드 유효성 검사
//	AActor* HitActor = HitResult.GetActor(); // 충돌 결과에서 맞은 액터를 가져옴
//	if (!IsValid(HitActor)) return; // 맞은 액터가 유효하지 않다면 리턴
//	AActor* WeaponOwner = GetOwner(); // 총의 소유자 액터 가져오기
//	if (!IsValid(WeaponOwner)) return; // 소유자 유효성 검사
//
//	if (IsValid(HitActor)) // 맞은 액터가 유효하다면
//	{
//		float AppliedDamage = Damage; // 기본 데미지 설정
//		if (HitResult.BoneName == "Head" || HitResult.BoneName == "CC_Base_Head") // 맞은 위의 본 이름이 Head 또는 CC_Base_Head라면
//		{
//			AppliedDamage *= HeadshotDamageMultiplier; // 헤드샷 배수만큼 데미지 적용
//		}
//		if (WeaponOwner)
//		{
//			AController* InstigatorController = WeaponOwner->GetInstigatorController(); // 소유자의 컨트롤러 가져오기
//
//			// 컨트롤러가 존재할 때만 데미지 적용
//			if (InstigatorController)
//			{
//				UGameplayStatics::ApplyPointDamage( 
//					HitActor, // 데미지를 적용할 액터
//					AppliedDamage, // 적용할 데미지
//					ShotDirection, // 발사 방향
//					HitResult, // 충돌 결과
//					InstigatorController, // 데미지를 준 컨트롤러
//					this, // 데미지를 준 액터
//					UDamageType::StaticClass() // 데미지 타입
//				);
//			}
//			if (ImpactEffect) // 피격 이펙트가 설정되어 있다면
//			{
//				// 이전 Impact Effect가 있다면 제거
//				if (IsValid(CurrentImpactEffectComponent))
//				{
//					CurrentImpactEffectComponent->DestroyComponent(); // 파괴후 메모리에서 제거
//					CurrentImpactEffectComponent = nullptr; // 포인터 초기화
//				}
//
//				FVector ImpactLocation = HitResult.ImpactPoint; // 피격 위치
//				FRotator ImpactRotation = (-ShotDirection).Rotation(); // 충돌 방향으로 회전
//
//				CurrentImpactEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
//					World, // 월드
//					ImpactEffect, // 이펙트 시스템
//					ImpactLocation, // 피격 위치
//					ImpactRotation // 충돌 방향으로 회전
//				);
//
//				if (CurrentImpactEffectComponent)
//				{
//					TWeakObjectPtr<ARifle> WeakThis(this); // 약참조 생성
//					World->GetTimerManager().SetTimer(
//							ImpactEffectTimerHandle, 
//						[WeakThis]() // 람다 캡처
//							{
//							if (WeakThis.IsValid()) // 유효성 검사
//							{
//								WeakThis->StopImpactEffect(); // 히트 이펙트 정지 함수 호출
//							}
//						},
//						ImpactEffectDuration, false); // 한 번만 실행
//				}
//			}
//
//			// 사운드 재생 로직 수정
//			USoundBase* SoundToPlay = HitSound; // 기본값은 일반 히트 사운드
//
//			// 헤드샷 여부를 다시 판단
//			if (HitResult.GetActor() && (HitResult.BoneName == "Head" || HitResult.BoneName == "CC_Base_Head"))
//			{
//				// 헤드샷이라면 HeadshotSound로 변경
//				if (HeadShotSound)
//				{
//					SoundToPlay = HeadShotSound;
//				}
//			}
//			// 최종적으로 결정된 사운드를 피격 위치에서 재생
//			if (SoundToPlay)
//			{
//				UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, HitResult.ImpactPoint); // 피격 위치에서 재생
//			}
//		}
//	}
//}

void ARifle::ProcessHit(const FHitResult& HitResult, FVector ShotDirection)
{
	UWorld* World = GetWorld(); // 월드 유효성 검사
	if (!World) return; // 월드 유효성 검사
	AActor* HitActor = HitResult.GetActor(); // 충돌 결과에서 맞은 액터를 가져옴
	if (!IsValid(HitActor)) return; // 맞은 액터가 유효하지 않다면 리턴
	AActor* WeaponOwner = GetOwner(); // 총의 소유자 액터 가져오기
	if (!IsValid(WeaponOwner)) return; // 소유자 유효성 검사
	AController* InstigatorController = WeaponOwner->GetInstigatorController(); // 소유자의 컨트롤러 가져오기
	if (!IsValid(InstigatorController)) return; // 컨트롤러 유효성 검사

	bool bIsHeadshot = false; // 헤드샷 여부 초기화
	if (HitResult.BoneName == "Head" || HitResult.BoneName == "CC_Base_Head") // 맞은 위의 본 이름이 Head 또는 CC_Base_Head라면
	{
		bIsHeadshot = true; // 헤드샷 플래그 설정
	}
	float AppliedDamage = Damage; // 기본 데미지 설정
	if (bIsHeadshot) // 헤드샷일 경우
	{
		AppliedDamage = Damage * HeadshotDamageMultiplier; // 헤드샷 배수만큼 데미지 적용
	}
	// 데미지 적용
	UGameplayStatics::ApplyPointDamage(
		HitActor, // 데미지를 적용할 액터
		AppliedDamage, // 적용할 데미지
		ShotDirection, // 발사 방향
		HitResult, // 충돌 결과
		InstigatorController, // 데미지를 준 컨트롤러
		this, // 데미지를 준 액터
		UDamageType::StaticClass() // 데미지 타입
	);
	if (ImpactEffect) // 피격 이펙트가 설정되어 있다면
	{
		StopImpactEffect(); // 이전 이펙트가 있다면 제거

		CurrentImpactEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, // 월드
			ImpactEffect, // 이펙트 시스템
			HitResult.ImpactPoint, // 피격 위치
			(-ShotDirection).Rotation() // 충돌 방향으로 회전
		);
		if (CurrentImpactEffectComponent) // 이펙트 컴포넌트가 유효하면
		{
			TWeakObjectPtr<ARifle> WeakThis(this); // 약참조 생성
			World->GetTimerManager().SetTimer( 
				ImpactEffectTimerHandle,
				[WeakThis]() // 람다 캡처
				{
					if (WeakThis.IsValid()) // 유효성 검사
					{
						WeakThis->StopImpactEffect(); // 히트 이펙트 정지 함수 호출
					}
				},
				ImpactEffectDuration, false); // 한 번만 실행
		}
	}

	// 사운드 결정
	USoundBase* SoundToPlay = HitSound; // 기본값은 일반 히트 사운드
	if (bIsHeadshot) // 헤드샷일 경우
	{
		if (HeadShotSound) // 헤드샷 사운드가 유효하면
		{
			SoundToPlay = HeadShotSound; // 헤드샷 사운드로 변경
		}
	}
	if (SoundToPlay) // 사운드가 유효하면
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, HitResult.ImpactPoint); // 피격 위치에서 재생
	}
}