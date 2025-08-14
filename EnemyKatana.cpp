#include "EnemyKatana.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MainCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Enemy.h"

AEnemyKatana::AEnemyKatana()
{
    PrimaryActorTick.bCanEverTick = true;

    // īŸ�� �޽� �ʱ�ȭ �� RootComponent�� ����
    KatanaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaMesh"));
    RootComponent = KatanaMesh;

    KatanaChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaChildMesh"));
    KatanaChildMesh->SetupAttachment(KatanaMesh);  // ��Ʈ ������Ʈ�� �ڽ����� ���̰� �ʱ�ȭ
}

void AEnemyKatana::BeginPlay()
{
    Super::BeginPlay();
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), EnemyActorsCache);
}

void AEnemyKatana::Tick(float DeltaTime) 
{
    Super::Tick(DeltaTime);
    //UE_LOG(LogTemp, Warning, TEXT("Katana Tick: %d"), bIsAttacking); // Tick Ȱ��ȭ Ȯ��

    if (bIsAttacking)
    {
        //UE_LOG(LogTemp, Warning, TEXT("Performing Raycast Attack")); // ���� ���� Ȯ��
        PerformRaycastAttack();
    }
}

void AEnemyKatana::StartAttack()
{
    bIsAttacking = true;
    RaycastHitActors.Empty();
    DamagedActors.Empty();
}

void AEnemyKatana::EndAttack()
{
    bIsAttacking = false;
    bIsStrongAttack = false;
    RaycastHitActors.Empty();
    DamagedActors.Empty();
}

void AEnemyKatana::EnableAttackHitDetection(EAttackType AttackType)
{
    bIsAttacking = true; // ���� ���� Ȱ��ȭ �߰�
    CurrentAttackType = AttackType;
    RaycastHitActors.Empty();
    DamagedActors.Empty();

    UE_LOG(LogTemp, Warning, TEXT("StrongAttack Active: %d"), bIsStrongAttack); // �α� �߰�
}

void AEnemyKatana::DisableAttackHitDetection()
{
    bIsAttacking = false;
    RaycastHitActors.Empty();
    DamagedActors.Empty();
}

void AEnemyKatana::PerformRaycastAttack()
{
    FVector Start = KatanaChildMesh->GetComponentLocation(); // �ڽ� �޽� ���� start
    FVector Forward = KatanaChildMesh->GetForwardVector();  // �ڽ� �޽� ���� end
    float TotalDistance = 150.0f;
    int NumSteps = 5;
    float StepLength = TotalDistance / NumSteps;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    if (GetOwner())
    {
        Params.AddIgnoredActor(GetOwner());

        // �����ڰ� ���� �� �׷� ��ü ����
        TArray<AActor*> EnemyActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), EnemyActors);
        for (AActor* Enemy : EnemyActors)
        {
            Params.AddIgnoredActor(Enemy);
        }
    }

    // Sweep ��ü ��ο� �ʷϻ� ���� �� ���� �׸�
    FVector SweepLineEnd = Start + Forward * TotalDistance;
    DrawDebugLine(GetWorld(), Start, SweepLineEnd, FColor::Green, false, 5.0f, SDPG_Foreground, 3.0f);

    for (int i = 0; i < NumSteps; ++i)
    {
        FVector SweepStart = Start + Forward * StepLength * i;
        FVector SweepEnd = Start + Forward * StepLength * (i + 1);

        TArray<FHitResult> OutHits;
        bool bHit = GetWorld()->SweepMultiByChannel(
            OutHits,
            SweepStart,
            SweepEnd,
            FQuat::Identity,
            ECC_Pawn,
            FCollisionShape::MakeSphere(30.0f),
            Params
        );

        if (bHit)
        {
            for (const FHitResult& Hit : OutHits)
            {
                AActor* HitActor = Hit.GetActor();
                if (!HitActor) continue;

                // ���� ĳ���͸� ���� ó��
                if (HitActor->IsA(AMainCharacter::StaticClass()))
                {
                    RaycastHitActors.Add(HitActor);
                    ApplyDamage(HitActor);

                    // ���� ��ġ�� ���� �� ǥ��
                    DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 30.0f, 12, FColor::Red, false, 0.3f, SDPG_Foreground, 4.0f);

                    return;  // ù ��° ���� ���� ĳ���Ϳ��� �Լ� ����
                }
            }
        }
    }
}

void AEnemyKatana::ApplyDamage(AActor* OtherActor)
{
    if (!OtherActor || DamagedActors.Contains(OtherActor)) return;

    AActor* KatanaOwner = GetOwner();
    if (OtherActor == KatanaOwner)
        return;

    // �ٸ� �� ĳ�������� Ȯ��
    if (OtherActor->IsA(AEnemy::StaticClass()))
        return;

    // �÷��̾ ������ ����
    if (RaycastHitActors.Contains(OtherActor))
    {
        float DamageAmount = 0.0f;
        FString AttackTypeStr;

        AEnemy* EnemyOwner = Cast<AEnemy>(KatanaOwner); // Enemy Ŭ�������� bIsEliteEnemy�� �����ϱ� īŸ�� ���ʸ� Enemy�� ĳ����
        bool bIsElite = (EnemyOwner && EnemyOwner->bIsEliteEnemy); // �ٸ�Ʈ �� ����

        switch (CurrentAttackType)
        {
        case EAttackType::Normal:
            DamageAmount = bIsElite ? 30.0f : 20.0f; // �ٸ�Ʈ�� �Ϲ� ������ ����
            AttackTypeStr = TEXT("NormalAttack");
            break;
        case EAttackType::Strong:
            DamageAmount = bIsElite ? 60.0f : 50.0f; 
            AttackTypeStr = TEXT("StrongAttack");
            break;
        case EAttackType::Jump: 
            DamageAmount = bIsElite ? 40.0f : 30.0f;
            AttackTypeStr = TEXT("JumpAttack");
            break;
        }

        UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, nullptr, this, nullptr);
        UE_LOG(LogTemp, Warning, TEXT("AttackType: %s, Damage: %f IsElite: %d"), *AttackTypeStr, DamageAmount, bIsElite);

        DamagedActors.Add(OtherActor);
    }
}

void AEnemyKatana::HideKatana()
{
    UE_LOG(LogTemp, Warning, TEXT("Hiding Katana Memory Cleanup"));

    // 1. �̺�Ʈ �� Ÿ�̸� ���� (�ֿ켱)
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // ��� Ÿ�̸� ������ �ݹ� �Լ� ȣ�� ����

    // 2. ���� �÷��� ����
    bIsAttacking = false; // ���� ���� �ʱ�ȭ
    bIsStrongAttack = false; // ������ ���� �ʱ�ȭ

    // 3. �迭 ������ ����
    RaycastHitActors.Empty(); // ��Ʈ ���� ��� ���� ����
    DamagedActors.Empty(); // ������ ���� ��� ���� ����
    EnemyActorsCache.Empty(); // ĳ�õ� �� ��� ���� ����

    // 4. ������ ���� ���� (������Ʈ ���� ��)
    SetOwner(nullptr); // ��ȯ ���� ������ ���� ������ ���� ���� ����

    // 5. ������Ʈ �ý��� ����
    if (KatanaMesh && IsValid(KatanaMesh) && !KatanaMesh->IsBeingDestroyed())
    {
        // ������ �ý��� ��Ȱ��ȭ
        KatanaMesh->SetVisibility(false);
        KatanaMesh->SetHiddenInGame(true);

        // ���� �ý��� ��Ȱ��ȭ
        KatanaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        KatanaMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

        // ������Ʈ �ý��� ��Ȱ��ȭ
        KatanaMesh->SetComponentTickEnabled(false);

        // ���� ����
        KatanaMesh->SetStaticMesh(nullptr);

        // ������Ʈ ���� ���� (����� ���� ������Ʈ�̹Ƿ� ����)
        KatanaMesh->DestroyComponent();
    }

    // 6. ���� ���� �ý��� ����
    SetActorHiddenInGame(true); // ���� ������ ��Ȱ��ȭ
    SetActorEnableCollision(false); // ���� �浹 ��Ȱ��ȭ
    SetActorTickEnabled(false); // ���� ƽ ��Ȱ��ȭ

    // 7. ���� ������ ó�� �Ϸ� �� ���� �����ӿ� �����ϰ� ���� ���� (ũ���� ����)
    GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyKatana>(this)]() // ����Ʈ ������ WeakObjectPtr�� ���� ������ ����Ͽ� �����ϰ� ���� ����
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // ���� ������ ���Ͱ� ��ȿ�ϰ� �ı����� �ʾҴٸ�
            {
                WeakThis->Destroy(); // ���� ���� ����
                UE_LOG(LogTemp, Warning, TEXT("EnemyKatana Successfully Destroyed."));
            }
        });
}
