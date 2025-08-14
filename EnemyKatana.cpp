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

    // 카타나 메시 초기화 및 RootComponent로 설정
    KatanaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaMesh"));
    RootComponent = KatanaMesh;

    KatanaChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaChildMesh"));
    KatanaChildMesh->SetupAttachment(KatanaMesh);  // 루트 컴포넌트의 자식으로 붙이고 초기화
}

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
    FVector Start = KatanaChildMesh->GetComponentLocation(); // 자식 메쉬 기준 start
    FVector Forward = KatanaChildMesh->GetForwardVector();  // 자식 메쉬 기준 end
    float TotalDistance = 150.0f;
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

    // Sweep 전체 경로에 초록색 선만 한 번만 그림
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

                // 메인 캐릭터만 피해 처리
                if (HitActor->IsA(AMainCharacter::StaticClass()))
                {
                    RaycastHitActors.Add(HitActor);
                    ApplyDamage(HitActor);

                    // 맞은 위치에 빨간 구 표시
                    DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 30.0f, 12, FColor::Red, false, 0.3f, SDPG_Foreground, 4.0f);

                    return;  // 첫 번째 맞은 메인 캐릭터에서 함수 종료
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
    UE_LOG(LogTemp, Warning, TEXT("Hiding Katana Memory Cleanup"));

    // 1. 이벤트 및 타이머 정리 (최우선)
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 해제로 콜백 함수 호출 차단

    // 2. 상태 플래그 정리
    bIsAttacking = false; // 공격 상태 초기화
    bIsStrongAttack = false; // 강공격 상태 초기화

    // 3. 배열 데이터 정리
    RaycastHitActors.Empty(); // 히트 액터 목록 완전 삭제
    DamagedActors.Empty(); // 데미지 액터 목록 완전 삭제
    EnemyActorsCache.Empty(); // 캐시된 적 목록 완전 삭제

    // 4. 소유자 관계 해제 (컴포넌트 정리 전)
    SetOwner(nullptr); // 순환 참조 방지를 위해 소유자 관계 먼저 해제

    // 5. 컴포넌트 시스템 정리
    if (KatanaMesh && IsValid(KatanaMesh) && !KatanaMesh->IsBeingDestroyed())
    {
        // 렌더링 시스템 비활성화
        KatanaMesh->SetVisibility(false);
        KatanaMesh->SetHiddenInGame(true);

        // 물리 시스템 비활성화
        KatanaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        KatanaMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

        // 업데이트 시스템 비활성화
        KatanaMesh->SetComponentTickEnabled(false);

        // 참조 해제
        KatanaMesh->SetStaticMesh(nullptr);

        // 컴포넌트 완전 제거 (무기는 별도 컴포넌트이므로 안전)
        KatanaMesh->DestroyComponent();
    }

    // 6. 액터 레벨 시스템 정리
    SetActorHiddenInGame(true); // 액터 렌더링 비활성화
    SetActorEnableCollision(false); // 액터 충돌 비활성화
    SetActorTickEnabled(false); // 액터 틱 비활성화

    // 7. 현재 프레임 처리 완료 후 다음 프레임에 안전하게 엑터 제거 (크래쉬 방지)
    GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyKatana>(this)]() // 스마트 포인터 WeakObjectPtr로 약한 참조를 사용하여 안전하게 지연 실행
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // 약한 참조한 엑터가 유효하고 파괴되지 않았다면
            {
                WeakThis->Destroy(); // 액터 완전 제거
                UE_LOG(LogTemp, Warning, TEXT("EnemyKatana Successfully Destroyed."));
            }
        });
}
