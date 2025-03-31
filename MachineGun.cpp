#include "MachineGun.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h" // UGameplaystatic ����� ���� ��� �߰�
#include "DrawDebugHelpers.h" // ����׸� �׸��� ���� ��� �߰�

AMachineGun::AMachineGun()
{
    PrimaryActorTick.bCanEverTick = true;

    GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
    RootComponent = GunMesh; // �޽� ���� �� ��Ʈ������Ʈ ����

    GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // �浹 ��Ȱ��ȭ BP�� �����ϰ� ����
    GunMesh->SetGenerateOverlapEvents(false); // ������ �̺�Ʈ ��Ȱ��ȭ
}

void AMachineGun::BeginPlay()
{
    Super::BeginPlay();
}

// �Ѿ� �߻� ���� �Ķ���� ���� �Լ�
void AMachineGun::SetFireParams(float InFireRate, float InDamage, float InSpreadAngle)
{
    FireRate = InFireRate; // �߻� �ӵ�
    BulletDamage = InDamage; // ������
    SpreadAngle = InSpreadAngle; // ź���� ����
}

void AMachineGun::StartFire() // �߻� ����
{
    GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AMachineGun::Fire, FireRate, true);
}

void AMachineGun::StopFire() // �߻� ����
{
    GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

// ź ���� ��� �Լ� (SpreadAngle �������� ������ ȸ���� ����)
FVector AMachineGun::GetFireDirectionWithSpread()
{
    FVector Forward = GetActorForwardVector();
    FRotator SpreadRot = FRotator(
        FMath::FRandRange(-SpreadAngle, SpreadAngle),
        FMath::FRandRange(-SpreadAngle, SpreadAngle),
        0.f
    );

    return SpreadRot.RotateVector(Forward); // ȸ�� ����� ���� ��ȯ
}

void AMachineGun::Fire()
{
    AController* OwnerController = GetOwner() ? GetOwner()->GetInstigatorController() : nullptr;
    if (!OwnerController)
    {
        UE_LOG(LogTemp, Error, TEXT("MachineGun: No OwnerController!"));
        return;
    }
    // ī�޶� ���� ����� ���� �÷��̾� ��Ʈ�ѷ� ĳ����
    APlayerController* PlayerController = Cast<APlayerController>(OwnerController);
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("MachineGun: Owner is not a PlayerController!"));
        return;
    }

    // ī�޶� ��ġ�� ���� ������
    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

    // ī�޶� �������� ź���� ����
    FRotator SpreadRot = FRotator(
        FMath::FRandRange(-SpreadAngle, SpreadAngle),
        FMath::FRandRange(-SpreadAngle, SpreadAngle),
        0.f
    );
    FVector ShotDirection = SpreadRot.RotateVector(CameraRotation.Vector());

    FVector Start = CameraLocation;
    FVector End = Start + (ShotDirection * 10000.0f); // ��� �Ÿ�

    // ����ĳ��Ʈ ����
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // �ӽŰ� �ڽ� ����
    Params.AddIgnoredActor(GetOwner()); // ĳ���� ����

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params); // ����ĳ��Ʈ �浹 ����

    if (bHit)
    {
        // ������ ����
        UGameplayStatics::ApplyPointDamage(Hit.GetActor(), BulletDamage, ShotDirection, Hit, GetInstigatorController(), this, nullptr);
        DrawDebugLine(GetWorld(), Start, Hit.ImpactPoint, FColor::Green, false, 0.2f, 0, 1.f);
    }
    else
    {
        DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.2f, 0, 1.f);
    }

    // �ѱ� ����Ʈ
    if (MuzzleEffect)
    {
        FVector MuzzleLocation = GunMesh->GetSocketLocation(FName("Muzzle"));
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleEffect, MuzzleLocation, GetActorRotation());
    }
}
