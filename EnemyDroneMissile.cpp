#include "EnemyDroneMissile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "EnemyDrone.h"
#include "Engine/OverlapResult.h"

AEnemyDroneMissile::AEnemyDroneMissile()
{
    PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ� ȣ�� Ȱ��ȭ

    // �浹 ������Ʈ ���� �� ����
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->InitSphereRadius(14.f);
    CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic")); // ������ ��� �Ͱ� �浹
    CollisionComponent->OnComponentHit.AddDynamic(this, &AEnemyDroneMissile::OnHit); // OnHit �Լ��� ��������Ʈ ���ε�
    RootComponent = CollisionComponent; // ��Ʈ ������Ʈ�� ����

    // �޽� ������Ʈ ���� �� ����
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(CollisionComponent); // �浹 ������Ʈ�� ����
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �޽� ��ü�� �浹�� ��� ����

    // ����ü �̵� ������Ʈ ���� �� ����
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 600.f; // �ʱ� �ӵ�
    ProjectileMovement->MaxSpeed = 600.f; // �ִ� �ӵ�
    ProjectileMovement->bRotationFollowsVelocity = true; // �̵� ���⿡ ���� �ڵ����� ȸ��
    ProjectileMovement->ProjectileGravityScale = 0.0f; // �߷� ���� ����

    SetActorTickInterval(0.05f); // Tick �ֱ� 0.05�ʷ� ����ȭ
}

void AEnemyDroneMissile::SetTarget(AActor* Target)
{
    TargetActor = Target; // Ÿ�� ����
}

void AEnemyDroneMissile::BeginPlay()
{
    Super::BeginPlay();
}

// ������Ʈ Ǯ���� ���� ��Ȱ�� �Լ�
void AEnemyDroneMissile::ResetMissile(FVector SpawnLocation, AActor* NewTarget)
{
    ProjectileMovement->StopMovementImmediately(); // ���� ������ ��� ����
    ProjectileMovement->SetUpdatedComponent(CollisionComponent); // �̵� ���� ������Ʈ �缳��
    ProjectileMovement->Activate(true); // �̵� ������Ʈ ��Ȱ��ȭ

    // ��ġ �� ���� �ʱ�ȭ
    SetActorLocation(SpawnLocation); // ���ο� ��ġ�� �̵�
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // �浹 ��Ȱ��ȭ
    SetActorEnableCollision(true); // ���� �浹 ��Ȱ��ȭ
    SetActorHiddenInGame(false); // �ٽ� ���̰� ����
    MeshComp->SetHiddenInGame(false);

    bExploded = false; // ���� ���� �ʱ�ȭ
    TargetActor = NewTarget; // ���ο� Ÿ�� ����
    LastMoveDirection = FVector::ZeroVector; // ������ �̵� ���� �ʱ�ȭ

    // Ǯ�� �� �߻��� �� �ִ� �浹 ������ �����ϱ� ���� ���� ���� ����� �ʱ�ȭ�ϰ� �ٽ� ����
    CollisionComponent->MoveIgnoreActors.Empty();
    // ��� �Ʊ� ����� �浹���� �����ϵ��� ����
    TArray<AActor*> AllDrones;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDrone::StaticClass(), AllDrones);
    for (AActor* Drone : AllDrones)
        CollisionComponent->IgnoreActorWhenMoving(Drone, true);
    // �ڱ� �ڽ��� ������ �ٸ� ��� �̻����� �浹���� �����ϵ��� ����
    TArray<AActor*> AllMissiles;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDroneMissile::StaticClass(), AllMissiles);
    for (AActor* Missile : AllMissiles)
    {
        if (Missile != this)
            CollisionComponent->IgnoreActorWhenMoving(Missile, true);
    }

    // �ʱ� �߻� ���� ���
    FVector Dir = FVector::ZeroVector;
    if (IsValid(TargetActor)) // Ÿ���� ��ȿ�ϴٸ�
    {
        Dir = (TargetActor->GetActorLocation() - SpawnLocation).GetSafeNormal(); // Ÿ���� ���ϴ� ���� ����
    }
    SetActorRotation(Dir.Rotation()); // ���⿡ �°� ȸ��
    ProjectileMovement->Velocity = Dir * ProjectileMovement->InitialSpeed; // �ش� �������� �ӵ� ����
    LastMoveDirection = Dir; // ������ �̵� ���� ����
}

void AEnemyDroneMissile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bExploded) return; // �����ߴٸ� ������Ʈ ����

    // Ÿ�� ���� (���� ���)
    FVector Dir = FVector::ZeroVector;
    if (IsValid(TargetActor)) // Ÿ���� ��ȿ�ϴٸ�
    {
        Dir = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal(); // ���� ��ġ���� Ÿ���� ���ϴ� ������ �ٽ� ���
    }

    if (!Dir.IsNearlyZero()) // ������ ��ȿ�ϴٸ�
    {
        FRotator NewRot = Dir.Rotation(); // ���ο� ���������� ȸ����
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), NewRot, DeltaTime, 5.f)); // �ε巴�� ���� ��ȯ
        ProjectileMovement->Velocity = Dir * ProjectileMovement->InitialSpeed; // ���ο� �������� �ӵ� ������Ʈ
        LastMoveDirection = Dir; // ������ �̵� ���� ����
    }
    else // Ÿ���� ���ƴٸ�
    {
        if (LastMoveDirection.IsNearlyZero())
            LastMoveDirection = GetActorForwardVector(); // ������ �̵� �������� ����
        ProjectileMovement->Velocity = LastMoveDirection * ProjectileMovement->InitialSpeed;
    }
}

void AEnemyDroneMissile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (bExploded) return; // �̹� �����ߴٸ� �ߺ� ���� ����
    if (!OtherActor || OtherActor == this) return; // �ε��� ����� ���ų� �ڱ� �ڽ��̸� ����

    Explode(); // ���𰡿� �ε����� ����
}

void AEnemyDroneMissile::Explode()
{
    if (bExploded) return; // �ߺ� ���� ����
    bExploded = true; // ���� ���·� ��ȯ

    // ���� ����Ʈ ����
    if (ExplosionEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), ExplosionEffect, GetActorLocation(), FRotator::ZeroRotator);
    }

    // ���� ���� ������ �÷��̾ ã�� ������ ���� (���� ����)
    FVector ExplosionCenter = GetActorLocation();
    float ExplosionRadiusTemp = ExplosionRadius;
    TArray<FOverlapResult> Overlaps;
    FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ExplosionRadiusTemp);
    // Pawn ä�ο� ���ؼ��� ������(����) �˻� ����
    bool bHasOverlaps = GetWorld()->OverlapMultiByChannel(Overlaps, ExplosionCenter, FQuat::Identity, ECC_Pawn, CollisionShape);
    if (bHasOverlaps) // ���� ���� Pawn�� �ִٸ�
    {
        ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
        for (auto& Overlap : Overlaps) // ��� �������� ���Ϳ� ����
        {
            if (Overlap.GetActor() == PlayerCharacter) // ���� �÷��̾���
            {
                UGameplayStatics::ApplyDamage(PlayerCharacter, Damage, GetInstigator() ? GetInstigator()->GetController() : nullptr, this, nullptr);
                break; // �÷��̾�� �� ���̹Ƿ� ã���� �ٷ� �ߴ�
            }
        }
    }

    // ���� �� �̻����� Ǯ�� ��ȯ�ϱ� ���� ��Ȱ��ȭ
    MeshComp->SetHiddenInGame(true);
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    ProjectileMovement->StopMovementImmediately();
}

float AEnemyDroneMissile::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    Health -= DamageAmount; // ü�� ����
    if (Health <= 0.f && !bExploded) // ü���� 0 ���ϰ� �ǰ� ���� �������� �ʾҴٸ�
        Explode(); // ���� (��� ����)
    return DamageAmount;
}