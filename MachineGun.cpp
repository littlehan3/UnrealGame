#include "MachineGun.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h" // UGameplaystatic 사용을 위한 헤더 추가
#include "DrawDebugHelpers.h" // 디버그를 그리기 위한 헤더 추가
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

    // [신규 추가] AEnemyShooterGun과 동일하게 MuzzleSocket 생성 및 설정
    MuzzleSocket = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MuzzleSocket"));
    MuzzleSocket->SetupAttachment(GunMesh); // GunMesh에 부착
    MuzzleSocket->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 없음
    MuzzleSocket->SetGenerateOverlapEvents(false);
    MuzzleSocket->SetVisibility(false); // 게임에서는 보이지 않음 (BP 뷰포트에서는 보임)

    // 포인터 멤버 변수 초기화
    CurrentMuzzleFlashComponent = nullptr;
}

void AMachineGun::BeginPlay()
{
    Super::BeginPlay();
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
    // --- [신규] 발사 시작 사운드 재생 ---
    if (StartFireSound)
    {
        // 총의 현재 위치에서 사운드를 재생합니다.
        UGameplayStatics::PlaySoundAtLocation(this, StartFireSound, GetActorLocation());
    }
    // --- [신규] 종료 --

    GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AMachineGun::Fire, FireRate, true);
}

void AMachineGun::StopFire() // 발사 중지
{
    // --- [신규] 발사 종료 사운드 재생 ---
    if (StopFireSound)
    {
        // 총의 현재 위치에서 사운드를 재생합니다.
        UGameplayStatics::PlaySoundAtLocation(this, StopFireSound, GetActorLocation());
    }
    // --- [신규] 종료 ---

    GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

// 탄 퍼짐 계산 함수 (SpreadAngle 기준으로 랜덤한 회전값 생성)
FVector AMachineGun::GetFireDirectionWithSpread()
{
    FVector Forward = GetActorForwardVector();
    FRotator SpreadRot = FRotator(
        FMath::FRandRange(-SpreadAngle, SpreadAngle),
        FMath::FRandRange(-SpreadAngle, SpreadAngle),
        0.f
    );

    return SpreadRot.RotateVector(Forward); // 회전 적용된 방향 반환
}

void AMachineGun::Fire()
{
    AController* OwnerController = GetOwner() ? GetOwner()->GetInstigatorController() : nullptr;
    if (!OwnerController)
    {
        UE_LOG(LogTemp, Error, TEXT("MachineGun: No OwnerController!"));
        return;
    }
    // 카메라 방향 사용을 위한 플레이어 컨트롤러 캐스팅
    APlayerController* PlayerController = Cast<APlayerController>(OwnerController);
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("MachineGun: Owner is not a PlayerController!"));
        return;
    }

    // 카메라 위치와 방향 가져옴
    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

    // 카메라 기준으로 탄퍼짐 적용
    FRotator SpreadRot = FRotator(
        FMath::FRandRange(-SpreadAngle, SpreadAngle),
        FMath::FRandRange(-SpreadAngle, SpreadAngle),
        0.f
    );
    FVector ShotDirection = SpreadRot.RotateVector(CameraRotation.Vector());

    FVector Start = CameraLocation;
    FVector End = Start + (ShotDirection * 10000.0f); // 쏘는 거리

    // 레이캐스트 설정
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 머신건 자신 제외
    Params.AddIgnoredActor(GetOwner()); // 캐릭터 제외

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params); // 레이캐스트 충돌 설정

    if (bHit)
    {
        AActor* HitActor = Hit.GetActor();

        // 데미지 적용
        UGameplayStatics::ApplyPointDamage(HitActor, BulletDamage, ShotDirection, Hit, GetInstigatorController(), this, nullptr);

         // [신규 추가] 넉백 적용 로직
        ACharacter* HitCharacter = Cast<ACharacter>(HitActor);
        if (HitCharacter && HitCharacter->GetCharacterMovement())
        {
            // 발사 방향으로 수평 넉백을 적용합니다.
            FVector LaunchDir = ShotDirection;
            LaunchDir.Z = 0; // 수평으로만 밀어냅니다.
            LaunchDir.Normalize();

            HitCharacter->LaunchCharacter(LaunchDir * KnockbackStrength, true, false); 
            // 두 번째 매개변수를 'true'로 변경하여 기존 수평 속도를 무시하고 덮어씌움 (bXYOverride)
        }

        // 2. 피격 사운드 재생 (HitSound)
        if (HitSound)
        {
            // 피격 위치(ImpactPoint) 근처에서 사운드 재생
            UGameplayStatics::PlaySoundAtLocation(this, HitSound, Hit.ImpactPoint);
        }

        // 3. 피격 이펙트 재생 (ImpactEffect)
        if (ImpactEffect)
        {
            StopImpactEffect(); // 이전 이펙트 정지

            CurrentImpactEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(),
                ImpactEffect,
                Hit.ImpactPoint,
                (-ShotDirection).Rotation() // 충돌 방향으로 회전
            );

            if (CurrentImpactEffectComponent)
            {
                GetWorldTimerManager().SetTimer(
                    ImpactEffectTimerHandle,
                    this,
                    &AMachineGun::StopImpactEffect,
                    ImpactEffectDuration,
                    false
                );
            }
        }
    }

    // 매 발사(Fire)마다 오너(AMainCharacter)를 캐스팅하여 쉐이크 함수를 호출합니다.
    AMainCharacter* OwnerCharacter = Cast<AMainCharacter>(GetOwner());
    if (OwnerCharacter)
    {
        OwnerCharacter->ApplyCameraShake();

        // [신규 추가] 라이플과 동일하게, 매 발사마다 크로스헤어 확장을 트리거합니다.
        UCrossHairComponent* CrosshairComp = OwnerCharacter->GetCrosshairComponent(); // [cite: 23]
        if (CrosshairComp)
        {
            CrosshairComp->StartExpansion(1.0f); // 
        }
    }

    // MuzzleSocket 컴포넌트의 위치를 가져와서 사용
    FVector MuzzleLocation = MuzzleSocket ? MuzzleSocket->GetComponentLocation() : GunMesh->GetComponentLocation();
    PlayMuzzleFlash(MuzzleLocation);

    if (FireSound)
    {
        // 총구 위치(MuzzleLocation)에서 사운드를 재생합니다.
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, MuzzleLocation);
    }
}

void AMachineGun::PlayMuzzleFlash(FVector Location)
{
    // [개선] 연사 시 이전에 재생 중이던 이펙트가 있다면 즉시 정지시킵니다.
    // 이렇게 하면 타이머가 만료되기 전에 다음 발사가 일어나도 이펙트가 중첩되거나 누락되지 않습니다.
    StopMuzzleFlash();

    // 총구 섬광 이펙트 재생 로직
    if (MuzzleFlash)
    {
        CurrentMuzzleFlashComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), MuzzleFlash, Location, GetActorRotation(), FVector(MuzzleFlashScale)
        );
        if (CurrentMuzzleFlashComponent)
        {
            CurrentMuzzleFlashComponent->SetFloatParameter(TEXT("PlayRate"), MuzzleFlashPlayRate);
            // MuzzleFlashDuration 후에 StopMuzzleFlash 함수를 호출하도록 타이머 설정
            GetWorld()->GetTimerManager().SetTimer(
                MuzzleFlashTimerHandle, this, &AMachineGun::StopMuzzleFlash, MuzzleFlashDuration, false
            );
        }
    }
}

void AMachineGun::StopMuzzleFlash()
{
    // [개선] 타이머가 만료되기 전에 PlayMuzzleFlash에 의해 수동으로 호출될 수 있으므로,
    // 타이머 핸들을 항상 클리어하여 중복 실행을 방지합니다.
    GetWorld()->GetTimerManager().ClearTimer(MuzzleFlashTimerHandle);

    if (CurrentMuzzleFlashComponent && IsValid(CurrentMuzzleFlashComponent))
    {
        CurrentMuzzleFlashComponent->DestroyComponent();
        CurrentMuzzleFlashComponent = nullptr;
    }
}

void AMachineGun::StopImpactEffect()
{
    if (CurrentImpactEffectComponent && IsValid(CurrentImpactEffectComponent))
    {
        CurrentImpactEffectComponent->Deactivate();
        CurrentImpactEffectComponent->DestroyComponent();
        CurrentImpactEffectComponent = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("Niagara Impact Effect stopped (MachineGun)"));
    }
}
