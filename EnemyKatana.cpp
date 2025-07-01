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
    KatanaMesh->SetSimulatePhysics(false); // ������ �� ���� �ùķ��̼� ��Ȱ��ȭ
    KatanaMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // �浹 Ȱ��ȭ

    HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
    HitBox->SetupAttachment(KatanaMesh);
    HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �⺻������ ��Ȱ��ȭ
    HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    HitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // �÷��̾ ����

    HitBox->OnComponentBeginOverlap.AddDynamic(this, &AEnemyKatana::OnHitBoxOverlap);
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
        UE_LOG(LogTemp, Warning, TEXT("Performing Raycast Attack")); // ���� ���� Ȯ��
        PerformRaycastAttack();
    }
}

void AEnemyKatana::StartAttack()
{
    OverlapHitActors.Empty();
    RaycastHitActors.Empty();
    DamagedActors.Empty();
    bIsAttacking = true;
}

void AEnemyKatana::EndAttack()
{
    bIsAttacking = false;
    bIsStrongAttack = false;
    OverlapHitActors.Empty();
    RaycastHitActors.Empty();
    DamagedActors.Empty();
    HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemyKatana::EnableAttackHitDetection(EAttackType AttackType)
{
    bIsAttacking = true; // ���� ���� Ȱ��ȭ �߰�
    CurrentAttackType = AttackType;
    OverlapHitActors.Empty();
    RaycastHitActors.Empty();
    DamagedActors.Empty();

    HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    UE_LOG(LogTemp, Warning, TEXT("StrongAttack Active: %d"), bIsStrongAttack); // �α� �߰�
}

void AEnemyKatana::DisableAttackHitDetection()
{
    bIsAttacking = false;
    HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OverlapHitActors.Empty();
    RaycastHitActors.Empty();
    DamagedActors.Empty();
}

void AEnemyKatana::OnHitBoxOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherActor) return;
    OverlapHitActors.Add(OtherActor);
    //UE_LOG(LogTemp, Warning, TEXT("īŸ�� ������ ����: %s"), *OtherActor->GetName());
    TryApplyDamage(OtherActor);
}

void AEnemyKatana::PerformRaycastAttack()
{
    FVector Start = KatanaMesh->GetComponentLocation();
    FVector End = Start + KatanaMesh->GetForwardVector() * 120.0f;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    if (GetOwner())
    {
        Params.AddIgnoredActor(GetOwner()); // ������ ����

        // �����ڰ� ���� �� �׷� ��ü ����
        TArray<AActor*> EnemyActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), EnemyActors);
        for (AActor* Enemy : EnemyActorsCache)
        {
            Params.AddIgnoredActor(Enemy);
        }
    }

    TArray<FHitResult> OutHits;
    bool bHit = GetWorld()->SweepMultiByChannel(
        OutHits,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(30.0f),
        Params
    );

    // ����� ���� �� ��ü �ð�ȭ
    // �⺻������ �Ķ���
     DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false, 5.0f, SDPG_Foreground, 3.0f);
     DrawDebugSphere(GetWorld(), Start, 30.0f, 12, FColor::Blue, false, 5.0f, SDPG_Foreground, 3.0f);
     DrawDebugSphere(GetWorld(), End, 30.0f, 12, FColor::Blue, false, 5.0f, SDPG_Foreground, 3.0f);

    if (bHit)
    {
        for (const FHitResult& Hit : OutHits)
        {
            if (Hit.GetActor())
            {
                RaycastHitActors.Add(Hit.GetActor());
                //UE_LOG(LogTemp, Warning, TEXT("īŸ�� ����ĳ��Ʈ ����: %s"), *Hit.GetActor()->GetName());
                TryApplyDamage(Hit.GetActor());

                // ����ĳ���Ϳ� �浹���� ���� ������ ��ü ����� ǥ��
                if (Hit.GetActor()->IsA(AMainCharacter::StaticClass()))
                {
                    DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 30.0f, 12, FColor::Red, false, 0.3f, SDPG_Foreground, 4.0f);
                }
            }
        }
    }
}

void AEnemyKatana::TryApplyDamage(AActor* OtherActor)
{
    if (!OtherActor || DamagedActors.Contains(OtherActor)) return;

    AActor* KatanaOwner = GetOwner();
    if (OtherActor == KatanaOwner)
        return;

    // �ٸ� �� ĳ�������� Ȯ��
    if (OtherActor->IsA(AEnemy::StaticClass()))
        return;

    // �÷��̾ ������ ����
    if (RaycastHitActors.Contains(OtherActor) || OverlapHitActors.Contains(OtherActor))
    {
        float DamageAmount = 0.0f;
        FString AttackTypeStr;

        switch (CurrentAttackType)
        {
        case EAttackType::Normal:
            DamageAmount = 20.0f;
            AttackTypeStr = TEXT("NormalAttack");
            break;
        case EAttackType::Strong:
            DamageAmount = 50.0f;
            AttackTypeStr = TEXT("StrongAttack");
            break;
        case EAttackType::Jump: 
            DamageAmount = 30.0f;
            AttackTypeStr = TEXT("JumpAttack");
            break;
        }

        UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, nullptr, this, nullptr);
        UE_LOG(LogTemp, Warning, TEXT("AttackType: %s, Damage: %f"), *AttackTypeStr, DamageAmount);

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
