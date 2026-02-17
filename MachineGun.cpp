#include "MachineGun.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "MainCharacter.h"
#include "CrossHairComponent.h"

AMachineGun::AMachineGun()
{
    PrimaryActorTick.bCanEverTick = true;

    GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
    RootComponent = GunMesh; // 메쉬 생성 및 루트컴포넌트 설정
    GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // 충돌 비활성화 BP도 동일하게 설정
    GunMesh->SetGenerateOverlapEvents(false); // 오버렙 이벤트 비활성화
	MuzzleSocket = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MuzzleSocket")); // 총구 소켓 메쉬 생성
    MuzzleSocket->SetupAttachment(GunMesh); // GunMesh에 부착
    MuzzleSocket->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 없음
	MuzzleSocket->SetGenerateOverlapEvents(false); // 오버랩 이벤트 비활성화
    MuzzleSocket->SetVisibility(false); // 위치 정보만 사용하므로 렌더링 제외

    CurrentMuzzleFlashComponent = nullptr; // 포인터 멤버 변수 초기화
	CurrentImpactEffectComponent = nullptr; // 포인터 멤버 변수 초기화
}

// 총알 발사 관련 파라미터 설정 함수
void AMachineGun::SetFireParams(float InFireRate, float InDamage, float InSpreadAngle)
{
    FireRate = InFireRate; // 발사 속도
    BulletDamage = InDamage; // 데미지
    SpreadAngle = InSpreadAngle; // 탄퍼짐 각도
}

void AMachineGun::StartFire() // 발사 시작
{
	UWorld* World = GetWorld(); // 월드 가져오기
	if (!World) return; // 유효성 검사

	if (StartFireSound) // 발사 시작 사운드가 설정되어 있으면
    {
		UGameplayStatics::PlaySoundAtLocation(this, StartFireSound, GetActorLocation()); // 발사 시작 사운드 재생
    }

	TWeakObjectPtr<AMachineGun> WeakThis(this); // 약참조 생성
	World->GetTimerManager().SetTimer(FireTimerHandle, [WeakThis]() // 발사 람다
        {
			if (WeakThis.IsValid()) // 유효성 검사
            {
				WeakThis->Fire(); // 발사 함수 호출
            }
		}, FireRate, true); // 발사속도만큼 반복 설정
}

void AMachineGun::StopFire() // 발사 중지
{
	UWorld* World = GetWorld(); // 월드 가져오기
	if (!World) return; // 유효성 검사

	if (StopFireSound) // 발사 종료 사운드가 설정되어 있으면
    {
		UGameplayStatics::PlaySoundAtLocation(this, StopFireSound, GetActorLocation()); // 발사 종료 사운드 재생
    }
    
    World->GetTimerManager().ClearTimer(FireTimerHandle); // 발사 타이머 클리어
}

FVector AMachineGun::GetFireDirectionWithSpread(FVector BaseDirection)
{
	FRotator SpreadRot = FRotator(
		FMath::FRandRange(-SpreadAngle, SpreadAngle), // 랜덤 퍼짐 각도 계산
		FMath::FRandRange(-SpreadAngle, SpreadAngle), // 랜덤 퍼짐 각도 계산
		0.f // Z축 회전은 없음
	);

	return SpreadRot.RotateVector(BaseDirection);
}

void AMachineGun::Fire()
{ 
	UWorld* World = GetWorld(); // 월드 가져오기
	if (!World) return; // 유효성 검사
	AActor* WeaponOwner = GetOwner(); // 소유자 메인캐릭터 가져오기
	if (!IsValid(WeaponOwner)) return; // 유효성 검사
	AController* OwnerController = WeaponOwner->GetInstigatorController(); // 오너의 컨트롤러 가져오기
	if (!IsValid(OwnerController)) return; // 유효성 검사
	APlayerController* PC = Cast<APlayerController>(OwnerController); // 플레이어 컨트롤러 캐스팅
	if (!IsValid(PC)) return; // 유효성 검사

	FVector CameraLocation; // 카메라 위치
	FRotator CameraRotation; // 카메라 회전
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation); // 카메라 뷰포인트 가져오기

	//FVector ShotDirection = GetFireDirectionWithSpread(); // 탄 퍼짐 적용 방향 함수를 호출하여 발사 방향 계산
	FVector ShotDirection = GetFireDirectionWithSpread(CameraRotation.Vector()); // 탄 퍼짐 적용 방향 함수를 호출하여 발사 방향 계산
	FVector Start = CameraLocation; // 발사 시작 위치
	FVector End = Start + (ShotDirection * MaxRange); // 발사 끝 위치

    // 레이캐스트 설정
	FHitResult Hit; // 히트 결과 저장용
	FCollisionQueryParams Params; // 레이캐스트 파라미터 설정
    Params.AddIgnoredActor(this); // 머신건 자신 제외
    Params.AddIgnoredActor(GetOwner()); // 캐릭터 제외
    bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params); // 레이캐스트 충돌 설정

	if (bHit) // 히트했을 때
    {
		AActor* HitActor = Hit.GetActor(); // 히트한 액터 가져오기
		if (IsValid(HitActor)) // 유효성 검사
		{
            // 데미지 적용
            UGameplayStatics::ApplyPointDamage(HitActor, BulletDamage, ShotDirection, Hit, GetInstigatorController(), this, nullptr);

			if (ACharacter* HitChar = Cast<ACharacter>(HitActor)) // 캐릭터일 경우 넉백 적용
			{
				FVector LaunchDir = ShotDirection; // 넉백 방향
				LaunchDir.Z = 0; // 수평면으로 제한
				LaunchDir.Normalize(); // 정규화
				HitChar->LaunchCharacter(LaunchDir.GetSafeNormal() * KnockbackStrength, true, false); // 넉백 적용
			}
		}
		if (HitSound) // 피격 사운드 재생
        {
			UGameplayStatics::PlaySoundAtLocation(this, HitSound, Hit.ImpactPoint); // 피격 위치에서 사운드 재생
        }
		if (ImpactEffect) // 피격 이펙트 재생
        {
            StopImpactEffect(); // 이전 이펙트 정지

            CurrentImpactEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation( 
                World, // 월드
				ImpactEffect, // 이펙트 시스템
				Hit.ImpactPoint, // 피격 위치
                (-ShotDirection).Rotation() // 충돌 방향으로 회전
            );
			if (CurrentImpactEffectComponent) // 이펙트 컴포넌트가 유효하면
            {
				TWeakObjectPtr<AMachineGun> WeakThis(this); // 약참조 생성
				World->GetTimerManager().SetTimer(ImpactEffectTimerHandle, [WeakThis]() // 람다
                    {
						if (WeakThis.IsValid()) // 유효성 검사
                        {
							WeakThis->StopImpactEffect(); // 이펙트 정지
                        }
					}, ImpactEffectDuration, false); // 타이머 설정
            }
        }
    }
	AMainCharacter* OwnerCharacter = Cast<AMainCharacter>(GetOwner()); // 오너 캐릭터 캐스팅
	if (OwnerCharacter) // 유효성 검사
    {
        OwnerCharacter->ApplyCameraShake(); // 카메라 쉐이크 호출 

        // 라이플과 동일하게, 매 발사마다 크로스헤어 확장을 트리거
		UCrossHairComponent* CrosshairComp = OwnerCharacter->GetCrosshairComponent(); // 크로스헤어 컴포넌트 가져오기
        if (CrosshairComp)
        {
			CrosshairComp->StartExpansion(1.0f); // 크로스헤어 확장 시작
        }
    }

	FVector MuzzleLocation; // 머즐 위치 변수를 불러옴
	if (IsValid(MuzzleSocket)) // 소켓이 유효하면
    { 
		MuzzleLocation = MuzzleSocket->GetComponentLocation(); // 소켓 위치 사용
    }
	else if (IsValid(GunMesh)) // 총기 메쉬가 유효하면
    {
        // 소켓이 없다면 총기 메쉬의 위치를 사용
        MuzzleLocation = GunMesh->GetComponentLocation();
    }
	else // 두 컴포넌트 모두 없다면
    {
        MuzzleLocation = GetActorLocation(); //액터의 루트 위치를 사용
    }
	PlayMuzzleFlash(MuzzleLocation); // 머즐 플래쉬 재생

	if (FireSound) // 발사 사운드 재생
    {
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, MuzzleLocation); // 발사 사운드 재생
    }
}

void AMachineGun::PlayMuzzleFlash(FVector Location)
{
	UWorld* World = GetWorld(); // 월드 가져오기
	if (!World) return; // 유효성 검사
	StopMuzzleFlash(); // 이전 이펙트 정지

	if (MuzzleFlash) // 이펙트가 설정되어 있으면
    {
		// 머즐 플래시 이펙트 스폰
		CurrentMuzzleFlashComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, MuzzleFlash, Location, GetActorRotation(), FVector(MuzzleFlashScale)
        );
		if (CurrentMuzzleFlashComponent) // 이펙트 컴포넌트가 유효하면
        {
			CurrentMuzzleFlashComponent->SetFloatParameter(TEXT("PlayRate"), MuzzleFlashPlayRate); // 재생 속도 설정
            // MuzzleFlashDuration 후에 StopMuzzleFlash 함수를 호출하도록 타이머 설정
            World->GetTimerManager().SetTimer(
				MuzzleFlashTimerHandle, this, &AMachineGun::StopMuzzleFlash, MuzzleFlashDuration, false // 한 번만 실행
            );
        }
    }
}

void AMachineGun::StopMuzzleFlash()
{
	UWorld* World = GetWorld(); // 월드 가져오기
	if (!World) return; // 유효성 검사
    
	World->GetTimerManager().ClearTimer(MuzzleFlashTimerHandle); // 머즐 플래시 타이머 클리어

	if (IsValid(CurrentMuzzleFlashComponent)) // 이펙트 컴포넌트가 유효하면
    {
		CurrentMuzzleFlashComponent->DestroyComponent(); // 이펙트 컴포넌트 파괴
		CurrentMuzzleFlashComponent = nullptr; // 포인터 초기화
    }
}

void AMachineGun::StopImpactEffect()
{
	if (IsValid(CurrentImpactEffectComponent)) // 이펙트 컴포넌트가 유효하면
    {
		CurrentImpactEffectComponent->Deactivate(); // 이펙트 비활성화
		CurrentImpactEffectComponent->DestroyComponent(); // 이펙트 컴포넌트 파괴
		CurrentImpactEffectComponent = nullptr; // 포인터 초기화
    }
}
