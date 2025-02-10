#include "Rifle.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

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
}

void ARifle::BeginPlay()
{
    Super::BeginPlay();

    // ✅ "MuzzleSocket"이라는 이름의 자식 컴포넌트 가져오기
    TArray<USceneComponent*> ChildrenComponents;
    RifleMesh->GetChildrenComponents(true, ChildrenComponents);

    for (USceneComponent* Child : ChildrenComponents)
    {
        if (Child->GetName() == TEXT("MuzzleSocket"))
        {
            MuzzleSocket = Cast<UStaticMeshComponent>(Child);
            break;
        }
    }


    // 🔥 오너 확인 (디버깅용)
    if (GetOwner())
    {
        UE_LOG(LogTemp, Warning, TEXT("Rifle Owner: %s"), *GetOwner()->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Rifle has NO OWNER at BeginPlay! SetOwner() is missing."));
    }
}

void ARifle::Fire()
{
    if (!bCanFire)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot Fire Yet! FireRate Cooldown Active."));
        return;
    }

    if (CurrentAmmo <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No Ammo! Need to Reload."));
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        UE_LOG(LogTemp, Error, TEXT("Rifle has NO OWNER! SetOwner() might be missing."));
        return;
    }

    AController* OwnerController = OwnerActor->GetInstigatorController();
    if (!OwnerController)
    {
        UE_LOG(LogTemp, Error, TEXT("OwnerController is NULL! Check GetOwner()"));
        return;
    }

    CurrentAmmo--;

    if (MuzzleFlash && RifleMesh)
    {
        UGameplayStatics::SpawnEmitterAttached(MuzzleFlash, RifleMesh, TEXT("MuzzleSocket"));
    }

    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
    }

    // ✅ **총구 위치에서 직접 레이캐스트 발사**
    FVector MuzzleLocation = MuzzleSocket->GetComponentLocation();
    FRotator MuzzleRotation = MuzzleSocket->GetComponentRotation();
    FVector ShotDirection = MuzzleRotation.Vector();
    FVector End = MuzzleLocation + (ShotDirection * Range);


    // ✅ **총구에서 시작하는 레이캐스트 실행**
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(GetOwner());

    bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLocation, End, ECC_Visibility, QueryParams);

    if (bHit)
    {
        ProcessHit(HitResult, ShotDirection);

        if (BulletTrail)
        {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BulletTrail, HitResult.ImpactPoint);
        }

        UE_LOG(LogTemp, Warning, TEXT("Hit detected! Target: %s, Location: %s"),
            *HitResult.GetActor()->GetName(), *HitResult.ImpactPoint.ToString());

        // 🔴 **총구에서 타겟 지점까지 디버그 라인 표시**
        DrawDebugLine(GetWorld(), MuzzleLocation, HitResult.ImpactPoint, FColor::Green, false, 1.0f, 0, 1.0f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No hit detected!"));

        // ❌ 아무도 맞지 않았을 때 총구에서 직진 디버그 라인
        DrawDebugLine(GetWorld(), MuzzleLocation, End, FColor::Red, false, 1.0f, 0, 1.0f);
    }

    bCanFire = false;
    GetWorldTimerManager().SetTimer(FireRateTimerHandle, this, &ARifle::ResetFire, FireRate, false);
}



void ARifle::ProcessHit(const FHitResult& HitResult, FVector ShotDirection)
{
    if (HitResult.GetActor())
    {
        float AppliedDamage = (HitResult.BoneName == "head") ? Damage * 2.0f : Damage;
        UGameplayStatics::ApplyPointDamage(HitResult.GetActor(), AppliedDamage, ShotDirection, HitResult, GetOwner()->GetInstigatorController(), this, UDamageType::StaticClass());

        if (ImpactEffect)
        {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, HitResult.ImpactPoint);
        }
    }
}

void ARifle::ResetFire()
{
    bCanFire = true;
}

void ARifle::Reload()
{
    if (TotalAmmo > 0)
    {
        int32 NeededAmmo = MaxAmmo - CurrentAmmo;
        int32 AmmoToReload = FMath::Min(TotalAmmo, NeededAmmo);
        CurrentAmmo += AmmoToReload;
        TotalAmmo -= AmmoToReload;
    }
}
