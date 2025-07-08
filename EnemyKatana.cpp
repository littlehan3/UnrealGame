#include "EnemyKatana.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MainCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Enemy.h"

// Sets default values
AEnemyKatana::AEnemyKatana()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // īŸ�� �޽� �ʱ�ȭ �� RootComponent�� ����
    KatanaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaMesh"));
    RootComponent = KatanaMesh;
}

// Called when the game starts or when spawned
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
    FVector Start = KatanaMesh->GetComponentLocation();
    FVector Forward = KatanaMesh->GetForwardVector();
    float TotalDistance = 120.0f;
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

    // Sweep ��ü ��ο� �Ķ��� ���� �� ���� �׸�
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
                if (Hit.GetActor())
                {
                    RaycastHitActors.Add(Hit.GetActor());
                    ApplyDamage(Hit.GetActor());

                    // �¾��� ���� ������ �� ǥ��
                    if (Hit.GetActor()->IsA(AMainCharacter::StaticClass()))
                    {
                        DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 30.0f, 12, FColor::Red, false, 0.3f, SDPG_Foreground, 4.0f);
                        return;
                    }
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
    SetActorHiddenInGame(true);  // ������ ����
    SetActorEnableCollision(false); // �浹 ����
    //SetActorTickEnabled(false); // Tick ��Ȱ��ȭ

    // 2�� �� ���� (�޸𸮿��� ������ �����ϱ� ���� ����) �ڵ� ������ �÷��� ����
    SetLifeSpan(2.0f);
}
