#include "Rifle.h"
#include "Enemy.h"
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
    if (bIsReloading)  // 재장전 중이면 사격 불가
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot Fire! Reloading..."));
        return;
    }

    if (!bCanFire)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot Fire Yet! FireRate Cooldown Active."));
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
                UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BulletTrail, HitResult.ImpactPoint);
            }

            UE_LOG(LogTemp, Warning, TEXT("Hit detected! Target: %s, Location: %s"),
                *HitResult.GetActor()->GetName(), *HitResult.ImpactPoint.ToString());

            DrawDebugLine(GetWorld(), CameraLocation, HitResult.ImpactPoint, FColor::Green, false, 2.0f, 0, 2.0f);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("HitResult.GetActor() is NULL!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No hit detected!"));
        DrawDebugLine(GetWorld(), CameraLocation, End, FColor::Red, false, 2.0f, 0, 2.0f);
    }

    if (MuzzleFlash)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, RifleMesh->GetComponentLocation(), CameraRotation);
    }

    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, RifleMesh->GetComponentLocation());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FireSound is NULL! Check if it's set in BP."));
    }

    CurrentAmmo--;
    bCanFire = false;
    GetWorldTimerManager().SetTimer(FireRateTimerHandle, this, &ARifle::ResetFire, FireRate, false);
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

    bIsReloading = true;  // 재장전 시작

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

    bIsReloading = false;  // 재장전 완료
}

void ARifle::ResetFire()
{
    bCanFire = true;
}


void ARifle::ProcessHit(const FHitResult& HitResult, FVector ShotDirection)
{
    AEnemy* Enemy = Cast<AEnemy>(HitResult.GetActor());  // 맞은 액터가 적인지 확인
    if (Enemy)
    {
        float AppliedDamage = (HitResult.BoneName == "head") ? Damage * 2.0f : Damage;

        UE_LOG(LogTemp, Warning, TEXT("ProcessHit: %s took %f damage"), *Enemy->GetName(), AppliedDamage);

        UGameplayStatics::ApplyPointDamage(Enemy, AppliedDamage, ShotDirection, HitResult, GetOwner()->GetInstigatorController(), this, UDamageType::StaticClass());

        if (ImpactEffect)
        {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, HitResult.ImpactPoint);
        }
    }
}