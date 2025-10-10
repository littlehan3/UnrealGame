#include "EnemyDrone.h"
#include "Kismet/GameplayStatics.h" // �����÷��� ��ƿ��Ƽ �Լ� ���
#include "GameFramework/CharacterMovementComponent.h" // ĳ���� �����Ʈ ������Ʈ ����
#include "NiagaraFunctionLibrary.h" // ���̾ư��� ����Ʈ ���� �Լ� ���
#include "GameFramework/ProjectileMovementComponent.h" // �̻����� ����ü �����Ʈ ����
#include "EnemyDroneAIController.h" // �� ����� ����� AI ��Ʈ�ѷ�
#include "MainGameModeBase.h" // ���Ӹ�忡 �� ����� �˸��� ���� ����

AEnemyDrone::AEnemyDrone()
{
    PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ� ȣ�� Ȱ��ȭ
    GetCharacterMovement()->SetMovementMode(MOVE_Flying); // �̵� ��带 '����'���� ����
    GetCharacterMovement()->GravityScale = 0.0f; // ���� ĳ�����̹Ƿ� �߷� ��Ȱ��ȭ
    AIControllerClass = AEnemyDroneAIController::StaticClass(); // �� ĳ���Ͱ� ����� AI ��Ʈ�ѷ� Ŭ���� ����
}

void AEnemyDrone::BeginPlay()
{
    Super::BeginPlay(); // �θ� Ŭ���� BeginPlay ȣ��

    if (!GetController()) // AI ��Ʈ�ѷ��� �Ҵ���� �ʾҴٸ�
    {
        UE_LOG(LogTemp, Warning, TEXT("No AI Controller found, spawning one"));
        AEnemyDroneAIController* NewController = GetWorld()->SpawnActor<AEnemyDroneAIController>(); // �� ��Ʈ�ѷ� ����
        if (NewController)
        {
            NewController->Possess(this); // ������ ��Ʈ�ѷ��� �� ��п� �����ϵ��� ����
        }
    }

    PlayerActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0); // �÷��̾� ĳ���� ã�� ����

    // ������Ʈ Ǯ��: ���� ���� �� �̻��� 10���� �̸� �����Ͽ� Ǯ�� �־��
    for (int32 i = 0; i < 10; ++i)
    {
        if (MissileClass) // �̻��� Ŭ������ ��ȿ�ϴٸ�
        {
            // �̻����� ���忡 ����������, �ٷ� ��������� ����
            AEnemyDroneMissile* Missile = GetWorld()->SpawnActor<AEnemyDroneMissile>(MissileClass, FVector::ZeroVector, FRotator::ZeroRotator);
            if (Missile)
            {
                Missile->SetActorHiddenInGame(true); // ������ �ʰ� ����
                Missile->SetActorEnableCollision(false); // �浹 ��Ȱ��ȭ
                MissilePool.Add(Missile); // �迭(Ǯ)�� �߰��Ͽ� ����
            }
        }
    }
}

void AEnemyDrone::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // �θ� Ŭ���� Tick ȣ��
    MissileTimer += DeltaTime; // �� ƽ���� ��Ÿ�� Ÿ�̸Ӹ� ������Ŵ

    if (MissileTimer >= MissileCooldown) // Ÿ�̸Ӱ� ��Ÿ���� �ʰ��ϸ�
    {
        ShootMissile(); // �̻��� �߻�
        MissileTimer = 0.0f; // Ÿ�̸� �ʱ�ȭ
    }
}

void AEnemyDrone::ShootMissile()
{
    AEnemyDroneMissile* Missile = GetAvailableMissileFromPool(); // Ǯ���� ��� ������ �̻����� ������
    if (Missile && PlayerActor) // �̻��ϰ� �÷��̾ ��� ��ȿ�ϴٸ�
    {
        FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 100.f; // ����� �ణ �տ��� �̻��� ����
        FRotator SpawnRot = (PlayerActor->GetActorLocation() - SpawnLoc).Rotation(); // ���� �� �÷��̾ �ٶ󺸴� ���� ����
        Missile->ResetMissile(SpawnLoc, PlayerActor); // ������ �̻����� �ʱ�ȭ�ϰ� �߻� �غ�
        Missile->SetActorRotation(SpawnRot); // �̻��� ���� ����
    }
}

AEnemyDroneMissile* AEnemyDrone::GetAvailableMissileFromPool()
{
    for (AEnemyDroneMissile* Missile : MissilePool) // Ǯ�� �ִ� ��� �̻����� ��ȸ
    {
        if (Missile && Missile->IsHidden()) // �̻����� ��ȿ�ϰ�, ���� ������ �ִٸ�(��� ���� �ƴ϶��)
        {
            return Missile; // �ش� �̻��� ��ȯ
        }
    }
    return nullptr; // ��� ������ �̻����� ������ null ��ȯ
}

float AEnemyDrone::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f; // �̹� �׾��ٸ� ������ ���� ����

    float DamageApplied = FMath::Min(Health, DamageAmount); // ���� ����� ������ ��� (ü�� �̻����� ������ �ʵ���)
    Health -= DamageApplied; // ü�� ����

    UE_LOG(LogTemp, Warning, TEXT("Drone took damage: %f, Health: %f"), DamageApplied, Health);

    if (Health <= 0.0f) // ü���� 0 ���϶��
    {
        Die(); // ��� ó�� �Լ� ȣ��
    }
    return DamageApplied; // ���� ����� ������ �� ��ȯ
}

void AEnemyDrone::Die()
{
    if (bIsDead) return; // �ߺ� ��� ó�� ����
    bIsDead = true; // ��� ���·� ��ȯ

    // ��� ����Ʈ ���
    if (DeathEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathEffect, GetActorLocation(), GetActorRotation());
    }

    // ��� ���� ���
    if (DeathSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
    }

    // ���� Ȱ��ȭ�� ��� �̻����� ������ ���߽�Ŵ
    for (AEnemyDroneMissile* Missile : MissilePool)
    {
        if (Missile && !Missile->IsHidden()) // Ǯ���� Ȱ��ȭ�� �̻����� ã��
        {
            Missile->Explode(); // ��� ���߽�Ŵ
        }
    }

    HideEnemy(); // ���� ���� �Լ� ȣ��
}

void AEnemyDrone::HideEnemy()
{
    UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyDrone - Cleanup"));
    // ���Ӹ�忡 ���� �ı��Ǿ����� �˸�
    if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
    {
        GameMode->OnEnemyDestroyed(this);
    }

    GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // ��� Ÿ�̸� ����

    // AI ��Ʈ�ѷ� ����
    AController* AICon = GetController();
    if (AICon && IsValid(AICon))
    {
        AICon->UnPossess(); // ���� ����
        AICon->Destroy(); // ��Ʈ�ѷ� �ı�
    }

    // �����Ʈ ������Ʈ ����
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp && IsValid(MoveComp))
    {
        MoveComp->DisableMovement(); // �̵� ��Ȱ��ȭ
        MoveComp->StopMovementImmediately(); // ��� ����
        MoveComp->SetComponentTickEnabled(false); // ������Ʈ ƽ ��Ȱ��ȭ
    }

    // �޽� ����
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetVisibility(false); // ������ �ʰ� ����
        MeshComp->SetHiddenInGame(true); // ���� ������ ���� ó��
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �浹 ��Ȱ��ȭ
        MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // ��� �浹 ���� ����
        MeshComp->SetComponentTickEnabled(false); // ������Ʈ ƽ ��Ȱ��ȭ
    }

    // �̻��� Ǯ ���� (�ı��� �ƴ� ��Ȱ��ȭ)
    for (AEnemyDroneMissile* Missile : MissilePool)
    {
        if (Missile && !Missile->IsHidden()) // ���� Ȱ��ȭ�� �̻����� �����ִٸ�
        {
            // ������Ʈ Ǯ�� ����̹Ƿ� Destroy ��� ���� ó���Ͽ� ���� �����ϰ� ���ܵ�
            Missile->SetActorHiddenInGame(true);
            Missile->SetActorEnableCollision(false);
            Missile->ProjectileMovement->StopMovementImmediately();
        }
    }

    // ���� ��ü ��Ȱ��ȭ
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    SetCanBeDamaged(false);

    // ���� �����ӿ� �����ϰ� ���� ����
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