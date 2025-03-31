#include "MachineGun.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h" // UGameplaystatic 사용을 위한 헤더 추가
#include "DrawDebugHelpers.h" // 디버그를 그리기 위한 헤더 추가

AMachineGun::AMachineGun()
{
    PrimaryActorTick.bCanEverTick = true;

    GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
    RootComponent = GunMesh; // 메쉬 생성 및 루트컴포넌트 설정

    GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // 충돌 비활성화 BP도 동일하게 설정
    GunMesh->SetGenerateOverlapEvents(false); // 오버렙 이벤트 비활성화
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
    GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AMachineGun::Fire, FireRate, true);
}

void AMachineGun::StopFire() // 발사 중지
{
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
        // 데미지 적용
        UGameplayStatics::ApplyPointDamage(Hit.GetActor(), BulletDamage, ShotDirection, Hit, GetInstigatorController(), this, nullptr);
        DrawDebugLine(GetWorld(), Start, Hit.ImpactPoint, FColor::Green, false, 0.2f, 0, 1.f);
    }
    else
    {
        DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.2f, 0, 1.f);
    }

    // 총구 이펙트
    if (MuzzleEffect)
    {
        FVector MuzzleLocation = GunMesh->GetSocketLocation(FName("Muzzle"));
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleEffect, MuzzleLocation, GetActorRotation());
    }
}
