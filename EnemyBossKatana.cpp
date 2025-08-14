#include "EnemyBossKatana.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MainCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "BossEnemy.h"

AEnemyBossKatana::AEnemyBossKatana()
{
    PrimaryActorTick.bCanEverTick = true;

    KatanaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaMesh"));
    RootComponent = KatanaMesh; // ��Ʈ ������Ʈ �ʱ�ȭ

    KatanaChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaChildMesh"));
    KatanaChildMesh->SetupAttachment(KatanaMesh);  // ��Ʈ ������Ʈ�� �ڽ����� ���̰� �ʱ�ȭ
}

void AEnemyBossKatana::BeginPlay()
{
    Super::BeginPlay();
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABossEnemy::StaticClass(), EnemyActorsCache);
}

void AEnemyBossKatana::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsAttacking)
    {
        PerformRaycastAttack();
    }
}

void AEnemyBossKatana::StartAttack()
{
    bIsAttacking = true;
    RaycastHitActors.Empty();
    DamagedActors.Empty();
}

void AEnemyBossKatana::EndAttack()
{
    bIsAttacking = false;
    RaycastHitActors.Empty();
    DamagedActors.Empty();
}

void AEnemyBossKatana::EnableAttackHitDetection()
{
    bIsAttacking = true;
    RaycastHitActors.Empty();
    DamagedActors.Empty();
}

void AEnemyBossKatana::DisableAttackHitDetection()
{
    bIsAttacking = false;
    RaycastHitActors.Empty();
    DamagedActors.Empty();
}

void AEnemyBossKatana::PerformRaycastAttack()
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

        // ���� �ڽ��� �� ���� ����
        TArray<AActor*> EnemyActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABossEnemy::StaticClass(), EnemyActors);
        for (AActor* Enemy : EnemyActors)
        {
            Params.AddIgnoredActor(Enemy);
        }
    }

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

                // ���� ĳ���͸� ���͸�
                if (HitActor->IsA(AMainCharacter::StaticClass()))
                {
                    RaycastHitActors.Add(HitActor);
                    ApplyDamage(HitActor);

                    DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 30.0f, 12,
                        FColor::Red, false, 0.3f, SDPG_Foreground, 4.0f);

                    return; // ù ���� ���� ĳ���Ϳ��� ����
                }
            }
        }
    }
}

void AEnemyBossKatana::ApplyDamage(AActor* OtherActor)
{
    if (!OtherActor || DamagedActors.Contains(OtherActor)) return;
    if (OtherActor == GetOwner()) return;
    if (RaycastHitActors.Contains(OtherActor))
    {
        // ���� īŸ���� ������ ���� ������
        UGameplayStatics::ApplyDamage(OtherActor, BossDamage, nullptr, this, nullptr);
        DamagedActors.Add(OtherActor);
    }
}

void AEnemyBossKatana::HideKatana()
{
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    bIsAttacking = false;

    RaycastHitActors.Empty();
    DamagedActors.Empty();
    EnemyActorsCache.Empty();
    SetOwner(nullptr);

    if (KatanaMesh && IsValid(KatanaMesh) && !KatanaMesh->IsBeingDestroyed())
    {
        KatanaMesh->SetVisibility(false);
        KatanaMesh->SetHiddenInGame(true);
        KatanaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        KatanaMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
        KatanaMesh->SetComponentTickEnabled(false);
        KatanaMesh->SetStaticMesh(nullptr);
        KatanaMesh->DestroyComponent();
    }

    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);

    GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyBossKatana>(this)]()
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
            {
                WeakThis->Destroy();
            }
        });
}
