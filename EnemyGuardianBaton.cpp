#include "EnemyGuardianBaton.h"
#include "Components/StaticMeshComponent.h" // ����ƽ �޽� ������Ʈ ���
#include "Components/BoxComponent.h" // �ڽ� ������Ʈ ��� (���� �ڵ忡���� �̻��)
#include "Kismet/GameplayStatics.h" // �����÷��� ��ƿ��Ƽ �Լ� ���
#include "MainCharacter.h" // �÷��̾� ĳ���� Ŭ���� ����
#include "Engine/World.h" // UWorld ���
#include "DrawDebugHelpers.h" // ����� �ð�ȭ ��� ���
#include "EnemyGuardian.h" // ������ Ŭ���� ����

AEnemyGuardianBaton::AEnemyGuardianBaton()
{
    PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ� ȣ�� Ȱ��ȭ

    BatonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatonMesh"));
    RootComponent = BatonMesh; // ��Ʈ ������Ʈ�� ����

    BatonChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatonChildMesh"));
    BatonChildMesh->SetupAttachment(BatonMesh); // �θ� �޽��� ����
}

void AEnemyGuardianBaton::BeginPlay()
{
    Super::BeginPlay(); // �θ� Ŭ���� BeginPlay ȣ��
    // ���� ���� �� ���忡 �ִ� ��� ����� ���͸� ã�� ĳ�ÿ� ���� (����� PerformRaycastAttack���� �Ź� �ٽ� ã��)
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyGuardian::StaticClass(), EnemyActorsCache);
}

void AEnemyGuardianBaton::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // �θ� Ŭ���� Tick ȣ��
    if (bIsAttacking) // ���� ������ Ȱ��ȭ�� ������ ����
    {
        PerformRaycastAttack(); // �� ƽ���� ���� ���� �Լ� ȣ��
    }
}

void AEnemyGuardianBaton::StartAttack()
{
    bIsAttacking = true; // ���� ���� Ȱ��ȭ
    RaycastHitActors.Empty(); // ���� ���� ��� �ʱ�ȭ
    DamagedActors.Empty();
}

void AEnemyGuardianBaton::EndAttack()
{
    bIsAttacking = false; // ���� ���� ��Ȱ��ȭ
    RaycastHitActors.Empty(); // ���� ��� �ʱ�ȭ
    DamagedActors.Empty();
}

void AEnemyGuardianBaton::EnableAttackHitDetection()
{
    bIsAttacking = true; // ���� ���� Ȱ��ȭ
    RaycastHitActors.Empty(); // ���� ���� ��� �ʱ�ȭ
    DamagedActors.Empty();
}

void AEnemyGuardianBaton::DisableAttackHitDetection()
{
    bIsAttacking = false; // ���� ���� ��Ȱ��ȭ
    RaycastHitActors.Empty(); // ���� ��� �ʱ�ȭ
    DamagedActors.Empty();
}

void AEnemyGuardianBaton::PerformRaycastAttack()
{
    FVector Start = BatonChildMesh->GetComponentLocation(); // �ڽ� �޽� ��ġ�� ���� ���������� ���
    FVector Forward = BatonChildMesh->GetForwardVector(); // �ڽ� �޽��� ���� ����
    float TotalDistance = 120.0f; // ��ü ���� ���� �Ÿ�
    int NumSteps = 5; // ������ ���� Ƚ�� (��Ȯ��)
    float StepLength = TotalDistance / NumSteps; // �� ���ܴ� �Ÿ�

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // �ڱ� �ڽ� ����
    if (GetOwner())
    {
        Params.AddIgnoredActor(GetOwner()); // ������(�����) ����

        // ������ ��� �Ʊ� ����� ����
        TArray<AActor*> EnemyActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyGuardian::StaticClass(), EnemyActors);
        for (AActor* Enemy : EnemyActors)
        {
            Params.AddIgnoredActor(Enemy);
        }
    }

    // ����(Sweep)�� ���� �ƴ� Ư�� ����(��ü)�� �̵���Ű�� �浹�� �˻��ϴ� ���
    for (int i = 0; i < NumSteps; ++i)
    {
        FVector SweepStart = Start + Forward * StepLength * i;
        FVector SweepEnd = Start + Forward * StepLength * (i + 1);

        TArray<FHitResult> OutHits;
        // �ݰ� 30�� ��ü�� �̵���Ű�� Pawn ä�ΰ� �浹�ϴ� ��� ����� ã��
        bool bHit = GetWorld()->SweepMultiByChannel(
            OutHits, SweepStart, SweepEnd, FQuat::Identity,
            ECC_Pawn, FCollisionShape::MakeSphere(30.0f), Params
        );

        if (bHit)
        {
            for (const FHitResult& Hit : OutHits)
            {
                AActor* HitActor = Hit.GetActor();
                if (!HitActor) continue;

                if (HitActor->IsA(AMainCharacter::StaticClass())) // ���� ����� �÷��̾���
                {
                    RaycastHitActors.Add(HitActor); // ���� ��Ͽ� �߰�
                    ApplyDamage(HitActor); // ������ ����
                    return; // �÷��̾ �� �� �������� �̹� ƽ�� ������ ����
                }
            }
        }
    }
}

void AEnemyGuardianBaton::ApplyDamage(AActor* OtherActor)
{
    // ����� ���ų�, �̹� �������� �����ų�, ������ �ڽ��̸� ������ ���� ����
    if (!OtherActor || DamagedActors.Contains(OtherActor) || OtherActor == GetOwner()) return;

    if (RaycastHitActors.Contains(OtherActor)) // ���� ��Ͽ� �ִ� ����̶��
    {
        UGameplayStatics::ApplyDamage(OtherActor, BatonDamage, nullptr, this, nullptr); // ������ ����
        DamagedActors.Add(OtherActor); // ������ ���� ��Ͽ� �߰� (�ߺ� ������ ����)
    }
}

void AEnemyGuardianBaton::HideBaton()
{
    // Ÿ�̸� �� ���� ����
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    bIsAttacking = false;

    // ������ ����
    RaycastHitActors.Empty();
    DamagedActors.Empty();
    EnemyActorsCache.Empty();
    SetOwner(nullptr);

    // ������Ʈ ����
    if (BatonMesh && IsValid(BatonMesh) && !BatonMesh->IsBeingDestroyed())
    {
        BatonMesh->SetVisibility(false);
        BatonMesh->SetHiddenInGame(true);
        BatonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BatonMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
        BatonMesh->SetComponentTickEnabled(false);
        BatonMesh->SetStaticMesh(nullptr);
        BatonMesh->DestroyComponent();
    }

    // ���� ����
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);

    // ���� �����ӿ� �����ϰ� ���� ����
    GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyGuardianBaton>(this)]()
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
            {
                WeakThis->Destroy();
            }
        });
}