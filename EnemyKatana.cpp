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
        //UE_LOG(LogTemp, Warning, TEXT("Performing Raycast Attack")); // 공격 실행 확인
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
    bIsAttacking = true; // 공격 상태 활성화 추가
    CurrentAttackType = AttackType;
    RaycastHitActors.Empty();
    DamagedActors.Empty();

    UE_LOG(LogTemp, Warning, TEXT("StrongAttack Active: %d"), bIsStrongAttack); // 로그 추가
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

        // 소유자가 속한 적 그룹 전체 무시
        TArray<AActor*> EnemyActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), EnemyActors);
        for (AActor* Enemy : EnemyActors)
        {
            Params.AddIgnoredActor(Enemy);
        }
    }

    // Sweep 전체 경로에 파란색 선만 한 번만 그림
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

                    // 맞았을 때만 빨간색 구 표시
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

    // 다른 적 캐릭터인지 확인
    if (OtherActor->IsA(AEnemy::StaticClass()))
        return;

    // 플레이어만 데미지 적용
    if (RaycastHitActors.Contains(OtherActor))
    {
        float DamageAmount = 0.0f;
        FString AttackTypeStr;

        AEnemy* EnemyOwner = Cast<AEnemy>(KatanaOwner); // Enemy 클래스에만 bIsEliteEnemy가 있으니까 카타나 오너를 Enemy로 캐스팅
        bool bIsElite = (EnemyOwner && EnemyOwner->bIsEliteEnemy); // 앨리트 적 여부

        switch (CurrentAttackType)
        {
        case EAttackType::Normal:
            DamageAmount = bIsElite ? 30.0f : 20.0f; // 앨리트와 일반 데미지 구분
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
    SetActorHiddenInGame(true);  // 렌더링 숨김
    SetActorEnableCollision(false); // 충돌 제거
    //SetActorTickEnabled(false); // Tick 비활성화

    // 2초 후 삭제 (메모리에서 완적히 삭제하기 위한 보완) 자동 가비지 컬렉션 유도
    SetLifeSpan(2.0f);
}
