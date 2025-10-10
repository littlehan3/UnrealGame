#include "EnemyGuardianShield.h"
#include "EnemyGuardian.h" // ������ Ŭ���� ����
#include "MainCharacter.h" // �÷��̾� ĳ���� Ŭ���� ����
#include "Kismet/GameplayStatics.h" // �����÷��� ��ƿ��Ƽ �Լ� ���
#include "Engine/World.h" // UWorld ���

AEnemyGuardianShield::AEnemyGuardianShield()
{
    PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ� ȣ�� Ȱ��ȭ

    ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
    RootComponent = ShieldMesh; // ��Ʈ ������Ʈ�� ����

    ShieldChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldChildMesh"));
    ShieldChildMesh->SetupAttachment(ShieldMesh); // �θ� �޽��� ����
}

void AEnemyGuardianShield::BeginPlay()
{
    Super::BeginPlay(); // �θ� Ŭ���� BeginPlay ȣ��
}

void AEnemyGuardianShield::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // �θ� Ŭ���� Tick ȣ��
    if (bIsAttacking) // ���� ������ Ȱ��ȭ�� ������ ����
    {
        PerformRaycastAttack(); // �� ƽ���� ���� ���� �Լ� ȣ��
    }
}

void AEnemyGuardianShield::HideShield()
{
    // Ÿ�̸� �� ���� ����
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

    // ������ ���� ����
    SetOwner(nullptr);

    // ������Ʈ ����
    if (ShieldMesh && IsValid(ShieldMesh) && !ShieldMesh->IsBeingDestroyed())
    {
        ShieldMesh->SetVisibility(false);
        ShieldMesh->SetHiddenInGame(true);
        ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ShieldMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
        ShieldMesh->SetComponentTickEnabled(false);
        ShieldMesh->SetStaticMesh(nullptr);
        ShieldMesh->DestroyComponent();
    }

    // ���� ����
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);

    // ���� �����ӿ� �����ϰ� ���� ����
    GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyGuardianShield>(this)]()
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
            {
                WeakThis->Destroy();
            }
        });
}

void AEnemyGuardianShield::StartShieldAttack()
{
    bIsAttacking = true; // ���� ���� Ȱ��ȭ
    RaycastHitActors.Empty(); // ���� ���� ��� �ʱ�ȭ
    DamagedActors.Empty();
}

void AEnemyGuardianShield::EndShieldAttack()
{
    bIsAttacking = false; // ���� ���� ��Ȱ��ȭ
    RaycastHitActors.Empty(); // ���� ��� �ʱ�ȭ
    DamagedActors.Empty();
}

void AEnemyGuardianShield::PerformRaycastAttack()
{
    if (!bIsAttacking || !GetOwner()) return;

    FVector Start = GetOwner()->GetActorLocation(); // ���� ������ (������� ��ġ)
    FVector Forward = GetOwner()->GetActorForwardVector(); // ������� ���� ����
    float AttackDistance = 200.0f; // ���� ���� ���� �Ÿ�
    FVector End = Start + (Forward * AttackDistance); // ���� ����

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // �ڱ� �ڽ� ����
    Params.AddIgnoredActor(GetOwner()); // ������(�����) ����

    // ������ ��� �Ʊ� ����� ����
    TArray<AActor*> EnemyActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyGuardian::StaticClass(), EnemyActors);
    for (AActor* Enemy : EnemyActors)
    {
        Params.AddIgnoredActor(Enemy);
    }

    FHitResult HitResult;
    // Pawn ä�ο� ���� ���� Ʈ���̽�(����ĳ��Ʈ) ����
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult, Start, End, ECC_Pawn, Params
    );

    if (bHit) // ���𰡿� �¾Ҵٸ�
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor && HitActor->IsA(AMainCharacter::StaticClass())) // ���� ����� �÷��̾���
        {
            RaycastHitActors.Add(HitActor); // ���� ��Ͽ� �߰�
            ApplyDamage(HitActor); // ������ ����
        }
    }
}

void AEnemyGuardianShield::ApplyDamage(AActor* OtherActor)
{
    // ����� ���ų�, �̹� �������� �����ų�, ������ �ڽ��̸� ������ ���� ����
    if (!OtherActor || DamagedActors.Contains(OtherActor) || OtherActor == GetOwner()) return;

    if (RaycastHitActors.Contains(OtherActor)) // ���� ��Ͽ� �ִ� ����̶��
    {
        UGameplayStatics::ApplyDamage(OtherActor, ShieldDamage, nullptr, this, nullptr); // ������ ����
        DamagedActors.Add(OtherActor); // ������ ���� ��Ͽ� �߰� (�ߺ� ������ ����)
    }
}