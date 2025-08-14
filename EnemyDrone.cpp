#include "EnemyDrone.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"

AEnemyDrone::AEnemyDrone()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    GetCharacterMovement()->GravityScale = 0.0f;
}

void AEnemyDrone::BeginPlay()
{
    Super::BeginPlay();
    PlayerActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

    // �̸� 10���� �����Ͽ� Ǯ�� ����
    for (int32 i = 0; i < 10; ++i)
    {
        // �̻���
        if (MissileClass)
        {
            AEnemyDroneMissile* Missile = GetWorld()->SpawnActor<AEnemyDroneMissile>(MissileClass, FVector::ZeroVector, FRotator::ZeroRotator);
            if (Missile)
            {
                Missile->SetActorHiddenInGame(true);
                Missile->SetActorEnableCollision(false);
                MissilePool.Add(Missile);
            }
        }
    }
}

void AEnemyDrone::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    MissileTimer += DeltaTime;

    if (MissileTimer >= MissileCooldown)
    {
        ShootMissile();
        MissileTimer = 0.0f;
    }
}

void AEnemyDrone::ShootMissile()
{
    AEnemyDroneMissile* Missile = GetAvailableMissileFromPool();
    if (Missile && PlayerActor)
    {
        FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 100.f;
        FRotator SpawnRot = (PlayerActor->GetActorLocation() - SpawnLoc).Rotation();
        Missile->ResetMissile(SpawnLoc, PlayerActor);
        Missile->SetActorRotation(SpawnRot);
    }
}

AEnemyDroneMissile* AEnemyDrone::GetAvailableMissileFromPool()
{
    for (AEnemyDroneMissile* Missile : MissilePool)
    {
        if (Missile && Missile->IsHidden())
        {
            return Missile;
        }
    }
    return nullptr;
}

float AEnemyDrone::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f;

    float DamageApplied = FMath::Min(Health, DamageAmount);
    Health -= DamageApplied;

    UE_LOG(LogTemp, Warning, TEXT("Drone took damage: %f, Health: %f"), DamageApplied, Health);

    if (Health <= 0.0f)
    {
        Die();
    }
    return DamageApplied;
}

void AEnemyDrone::Die()
{
    if (bIsDead) return;
    bIsDead = true;

    // ���̾ư��� ����Ʈ ���
    if (DeathEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            DeathEffect,
            GetActorLocation(),
            GetActorRotation()
        );
    }

    // ���� ��� (����)
    if (DeathSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
    }

    // �̹� �߻�Ǿ� �÷��̾ ���� ���� �̻��� ���� ����
    for (AEnemyDroneMissile* Missile : MissilePool)
    {
        if (Missile && !Missile->IsHidden()) // ���� Ȱ�� ����
        {
            Missile->Explode();  // ü�� �����ϰ� �ٷ� �����ϰ� �Ϸ��� �� ���
        }
    }

    // ���� �Լ� ȣ��
    HideEnemy();
}

void AEnemyDrone::HideEnemy()
{
    UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyDrone - Cleanup"));

    // 1. Ÿ�̸� ����
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

    // 2. AI ��Ʈ�ѷ� ����
    AController* AICon = GetController();
    if (AICon && IsValid(AICon))
    {
        AICon->UnPossess();
        AICon->Destroy();
    }

    // 3. �����Ʈ ���� �� ��Ȱ��ȭ
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp && IsValid(MoveComp))
    {
        MoveComp->DisableMovement();
        MoveComp->StopMovementImmediately();
        MoveComp->SetComponentTickEnabled(false);
    }

    // 4. �޽� ���� & �ݸ��� ����
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetVisibility(false);
        MeshComp->SetHiddenInGame(true);
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
        MeshComp->SetComponentTickEnabled(false);
    }

    // �̻��� ���� (Ȥ�� ���� �ִ� ���)
    for (AEnemyDroneMissile* Missile : MissilePool)
    {
        if (Missile && !Missile->IsHidden())
        {
            // Ǯ���̱⿡ Destroy ��� ��ư, �ݸ��� ������ ���� �����ϰ� ����
            Missile->SetActorHiddenInGame(true); 
            Missile->SetActorEnableCollision(false);
            Missile->ProjectileMovement->StopMovementImmediately();
        }
    }

    // 5. ���� ��ü ��Ȱ��ȭ
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    SetCanBeDamaged(false);

    // 6. ���� �����ӿ� �����ϰ� ����
    GetWorld()->GetTimerManager().SetTimerForNextTick(
        [WeakThis = TWeakObjectPtr<AEnemyDrone>(this)]()
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
            {
                WeakThis->Destroy();
                UE_LOG(LogTemp, Warning, TEXT("EnemyDrone Successfully Destroyed"));
            }
        }
    );
}
