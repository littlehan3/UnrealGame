#include "EnemyGuardianBaton.h"
#include "Components/StaticMeshComponent.h" // 스태틱 메쉬 컴포넌트 사용
#include "Components/BoxComponent.h" // 박스 컴포넌트 사용 (현재 코드에서는 미사용)
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수 사용
#include "MainCharacter.h" // 플레이어 캐릭터 클래스 참조
#include "Engine/World.h" // UWorld 사용
#include "DrawDebugHelpers.h" // 디버그 시각화 기능 사용
#include "EnemyGuardian.h" // 소유자 클래스 참조

AEnemyGuardianBaton::AEnemyGuardianBaton()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화

    BatonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatonMesh"));
    RootComponent = BatonMesh; // 루트 컴포넌트로 설정

    BatonChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatonChildMesh"));
    BatonChildMesh->SetupAttachment(BatonMesh); // 부모 메쉬에 부착
}

void AEnemyGuardianBaton::BeginPlay()
{
    Super::BeginPlay(); // 부모 클래스 BeginPlay 호출
    // 게임 시작 시 월드에 있는 모든 가디언 액터를 찾아 캐시에 저장 (현재는 PerformRaycastAttack에서 매번 다시 찾음)
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyGuardian::StaticClass(), EnemyActorsCache);
}

void AEnemyGuardianBaton::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // 부모 클래스 Tick 호출
    if (bIsAttacking) // 공격 판정이 활성화된 상태일 때만
    {
        PerformRaycastAttack(); // 매 틱마다 공격 판정 함수 호출
    }
}

void AEnemyGuardianBaton::StartAttack()
{
    bIsAttacking = true; // 공격 판정 활성화
    RaycastHitActors.Empty(); // 이전 공격 기록 초기화
    DamagedActors.Empty();
}

void AEnemyGuardianBaton::EndAttack()
{
    bIsAttacking = false; // 공격 판정 비활성화
    RaycastHitActors.Empty(); // 공격 기록 초기화
    DamagedActors.Empty();
}

void AEnemyGuardianBaton::EnableAttackHitDetection()
{
    bIsAttacking = true; // 공격 판정 활성화
    RaycastHitActors.Empty(); // 이전 공격 기록 초기화
    DamagedActors.Empty();
}

void AEnemyGuardianBaton::DisableAttackHitDetection()
{
    bIsAttacking = false; // 공격 판정 비활성화
    RaycastHitActors.Empty(); // 공격 기록 초기화
    DamagedActors.Empty();
}

void AEnemyGuardianBaton::PerformRaycastAttack()
{
    FVector Start = BatonChildMesh->GetComponentLocation(); // 자식 메쉬 위치를 판정 시작점으로 사용
    FVector Forward = BatonChildMesh->GetForwardVector(); // 자식 메쉬의 정면 방향
    float TotalDistance = 120.0f; // 전체 공격 판정 거리
    int NumSteps = 5; // 판정을 나눌 횟수 (정확도)
    float StepLength = TotalDistance / NumSteps; // 한 스텝당 거리

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 자기 자신 무시
    if (GetOwner())
    {
        Params.AddIgnoredActor(GetOwner()); // 소유자(가디언) 무시

        // 월드의 모든 아군 가디언도 무시
        TArray<AActor*> EnemyActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyGuardian::StaticClass(), EnemyActors);
        for (AActor* Enemy : EnemyActors)
        {
            Params.AddIgnoredActor(Enemy);
        }
    }

    // 스윕(Sweep)은 선이 아닌 특정 형태(구체)를 이동시키며 충돌을 검사하는 방식
    for (int i = 0; i < NumSteps; ++i)
    {
        FVector SweepStart = Start + Forward * StepLength * i;
        FVector SweepEnd = Start + Forward * StepLength * (i + 1);

        TArray<FHitResult> OutHits;
        // 반경 30의 구체를 이동시키며 Pawn 채널과 충돌하는 모든 대상을 찾음
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

                if (HitActor->IsA(AMainCharacter::StaticClass())) // 맞은 대상이 플레이어라면
                {
                    RaycastHitActors.Add(HitActor); // 감지 목록에 추가
                    ApplyDamage(HitActor); // 데미지 적용
                    return; // 플레이어를 한 번 때렸으면 이번 틱의 판정은 종료
                }
            }
        }
    }
}

void AEnemyGuardianBaton::ApplyDamage(AActor* OtherActor)
{
    // 대상이 없거나, 이미 데미지를 입혔거나, 소유자 자신이면 데미지 적용 안함
    if (!OtherActor || DamagedActors.Contains(OtherActor) || OtherActor == GetOwner()) return;

    if (RaycastHitActors.Contains(OtherActor)) // 감지 목록에 있는 대상이라면
    {
        UGameplayStatics::ApplyDamage(OtherActor, BatonDamage, nullptr, this, nullptr); // 데미지 적용
        DamagedActors.Add(OtherActor); // 데미지 입힌 목록에 추가 (중복 데미지 방지)
    }
}

void AEnemyGuardianBaton::HideBaton()
{
    // 타이머 및 상태 정리
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    bIsAttacking = false;

    // 데이터 정리
    RaycastHitActors.Empty();
    DamagedActors.Empty();
    EnemyActorsCache.Empty();
    SetOwner(nullptr);

    // 컴포넌트 정리
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

    // 액터 정리
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);

    // 다음 프레임에 안전하게 액터 제거
    GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyGuardianBaton>(this)]()
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
            {
                WeakThis->Destroy();
            }
        });
}