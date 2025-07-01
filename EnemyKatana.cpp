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

    // 카타나 메시 초기화 및 RootComponent로 설정
    KatanaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaMesh"));
    RootComponent = KatanaMesh;
    KatanaMesh->SetSimulatePhysics(false); // 시작할 때 물리 시뮬레이션 비활성화
    KatanaMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 충돌 활성화

    HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
    HitBox->SetupAttachment(KatanaMesh);
    HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 기본적으로 비활성화
    HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    HitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 플레이어만 감지

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
    //UE_LOG(LogTemp, Warning, TEXT("Katana Tick: %d"), bIsAttacking); // Tick 활성화 확인

    if (bIsAttacking)
    {
        UE_LOG(LogTemp, Warning, TEXT("Performing Raycast Attack")); // 공격 실행 확인
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
    bIsAttacking = true; // 공격 상태 활성화 추가
    CurrentAttackType = AttackType;
    OverlapHitActors.Empty();
    RaycastHitActors.Empty();
    DamagedActors.Empty();

    HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    UE_LOG(LogTemp, Warning, TEXT("StrongAttack Active: %d"), bIsStrongAttack); // 로그 추가
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
    //UE_LOG(LogTemp, Warning, TEXT("카타나 오버랩 감지: %s"), *OtherActor->GetName());
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
        Params.AddIgnoredActor(GetOwner()); // 소유자 무시

        // 소유자가 속한 적 그룹 전체 무시
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

    // 디버그 라인 및 구체 시각화
    // 기본적으로 파란색
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
                //UE_LOG(LogTemp, Warning, TEXT("카타나 레이캐스트 감지: %s"), *Hit.GetActor()->GetName());
                TryApplyDamage(Hit.GetActor());

                // 메인캐릭터와 충돌했을 때만 빨간색 구체 디버그 표시
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

    // 다른 적 캐릭터인지 확인
    if (OtherActor->IsA(AEnemy::StaticClass()))
        return;

    // 플레이어만 데미지 적용
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
    SetActorHiddenInGame(true);  // 렌더링 숨김
    SetActorEnableCollision(false); // 충돌 제거
    //SetActorTickEnabled(false); // Tick 비활성화

    // 2초 후 삭제 (메모리에서 완적히 삭제하기 위한 보완) 자동 가비지 컬렉션 유도
    SetLifeSpan(2.0f);
}
