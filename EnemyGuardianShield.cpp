#include "EnemyGuardianShield.h"
#include "EnemyGuardian.h" // 소유자 클래스 참조
#include "MainCharacter.h" // 플레이어 캐릭터 클래스 참조
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수 사용
#include "Engine/World.h" // UWorld 사용

AEnemyGuardianShield::AEnemyGuardianShield()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화

    ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
    RootComponent = ShieldMesh; // 루트 컴포넌트로 설정

    ShieldChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldChildMesh"));
    ShieldChildMesh->SetupAttachment(ShieldMesh); // 부모 메쉬에 부착
}

void AEnemyGuardianShield::BeginPlay()
{
    Super::BeginPlay(); // 부모 클래스 BeginPlay 호출
}

void AEnemyGuardianShield::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // 부모 클래스 Tick 호출
    if (bIsAttacking) // 공격 판정이 활성화된 상태일 때만
    {
        PerformRaycastAttack(); // 매 틱마다 공격 판정 함수 호출
    }
}

void AEnemyGuardianShield::HideShield()
{
    // 타이머 및 상태 정리
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

    // 소유자 관계 해제
    SetOwner(nullptr);

    // 컴포넌트 정리
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

    // 액터 정리
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);

    // 다음 프레임에 안전하게 액터 제거
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
    bIsAttacking = true; // 공격 판정 활성화
    RaycastHitActors.Empty(); // 이전 공격 기록 초기화
    DamagedActors.Empty();
}

void AEnemyGuardianShield::EndShieldAttack()
{
    bIsAttacking = false; // 공격 판정 비활성화
    RaycastHitActors.Empty(); // 공격 기록 초기화
    DamagedActors.Empty();
}

void AEnemyGuardianShield::PerformRaycastAttack()
{
    if (!bIsAttacking || !GetOwner()) return;

    FVector Start = GetOwner()->GetActorLocation(); // 판정 시작점 (가디언의 위치)
    FVector Forward = GetOwner()->GetActorForwardVector(); // 가디언의 정면 방향
    float AttackDistance = 200.0f; // 방패 공격 판정 거리
    FVector End = Start + (Forward * AttackDistance); // 판정 끝점

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 자기 자신 무시
    Params.AddIgnoredActor(GetOwner()); // 소유자(가디언) 무시

    // 월드의 모든 아군 가디언도 무시
    TArray<AActor*> EnemyActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyGuardian::StaticClass(), EnemyActors);
    for (AActor* Enemy : EnemyActors)
    {
        Params.AddIgnoredActor(Enemy);
    }

    FHitResult HitResult;
    // Pawn 채널에 대해 라인 트레이스(레이캐스트) 실행
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult, Start, End, ECC_Pawn, Params
    );

    if (bHit) // 무언가에 맞았다면
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor && HitActor->IsA(AMainCharacter::StaticClass())) // 맞은 대상이 플레이어라면
        {
            RaycastHitActors.Add(HitActor); // 감지 목록에 추가
            ApplyDamage(HitActor); // 데미지 적용
        }
    }
}

void AEnemyGuardianShield::ApplyDamage(AActor* OtherActor)
{
    // 대상이 없거나, 이미 데미지를 입혔거나, 소유자 자신이면 데미지 적용 안함
    if (!OtherActor || DamagedActors.Contains(OtherActor) || OtherActor == GetOwner()) return;

    if (RaycastHitActors.Contains(OtherActor)) // 감지 목록에 있는 대상이라면
    {
        UGameplayStatics::ApplyDamage(OtherActor, ShieldDamage, nullptr, this, nullptr); // 데미지 적용
        DamagedActors.Add(OtherActor); // 데미지 입힌 목록에 추가 (중복 데미지 방지)
    }
}