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

    // [수정] 공격 활성화 시 Tick에서 PerformRaycastAttack 호출
    if (bIsAttacking)
    {
        PerformRaycastAttack();
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
    bHasPlayedHitSound = false;
}

void AEnemyGuardianShield::EndShieldAttack()
{
    bIsAttacking = false; // 공격 판정 비활성화
    RaycastHitActors.Empty(); // 공격 기록 초기화
    DamagedActors.Empty();
    bHasPlayedHitSound = false;
}

// EnemyGuardianShield.cpp - PerformRaycastAttack 함수

void AEnemyGuardianShield::PerformRaycastAttack()
{
    // LineTrace 방식 대신 Sweep 방식으로 변경합니다.
    if (!bIsAttacking || !GetOwner()) return;

    // Start 지점을 ShieldMesh 또는 ShieldChildMesh의 현재 위치로 설정합니다.
    // 기존 코드에서는 GetOwner()->GetActorLocation()을 썼지만,
    // 정확도를 위해 무기 컴포넌트의 위치를 사용하는 것이 좋습니다.
    FVector Start = ShieldMesh->GetComponentLocation();
    FVector Forward = GetOwner()->GetActorForwardVector(); // 가디언의 정면 방향

    // 진압봉과 동일한 스윕 설정 사용
    float TotalDistance = 150.0f; // 판정 거리 (필요에 따라 조정)
    int NumSteps = 5; // 판정을 나눌 횟수
    float StepLength = TotalDistance / NumSteps; // 한 스텝당 거리
    float SweepRadius = 50.0f; // 방패의 크기에 맞게 구체 반지름 설정 (조정 필요)

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 자기 자신 무시
    Params.AddIgnoredActor(GetOwner()); // 소유자(가디언) 무시

    // 월드의 모든 아군 가디언도 무시 (기존 로직 유지)
    TArray<AActor*> EnemyActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyGuardian::StaticClass(), EnemyActors);
    for (AActor* Enemy : EnemyActors)
    {
        Params.AddIgnoredActor(Enemy);
    }

    // 디버그 시각화: 전체 공격 경로를 표시
    //FVector SweepLineEnd = Start + Forward * TotalDistance;
    //DrawDebugLine(GetWorld(), Start, SweepLineEnd, FColor::Green, false, 0.1f, SDPG_Foreground, 3.0f);


    // 다중 스텝 Sweep 실행
    for (int i = 0; i < NumSteps; ++i)
    {
        FVector SweepStart = Start + Forward * StepLength * i;
        FVector SweepEnd = Start + Forward * StepLength * (i + 1);

        TArray<FHitResult> OutHits;
        // 반경 SweepRadius의 구체를 이동시키며 Pawn 채널과 충돌하는 모든 대상을 찾음
        bool bHit = GetWorld()->SweepMultiByChannel(
            OutHits,
            SweepStart,
            SweepEnd,
            FQuat::Identity,
            ECC_Pawn,
            FCollisionShape::MakeSphere(SweepRadius), // 구체 형태의 스윕
            Params
        );

        if (bHit)
        {
            for (const FHitResult& Hit : OutHits)
            {
                AActor* HitActor = Hit.GetActor();
                if (!HitActor) continue;

                AMainCharacter* MainCharacter = Cast<AMainCharacter>(HitActor);
                if (MainCharacter)
                {
                    // 이미 처리된 대상인지 확인 (Tick 중복 실행 방지)
                    if (DamagedActors.Contains(HitActor))
                    {
                        return;
                    }

                    // 1. 감지 목록에 추가
                    RaycastHitActors.Add(HitActor);

                    // 2. 넉백 적용 (물리 효과)
                    FVector PushDirection = GetOwner()->GetActorForwardVector();
                    PushDirection.Z = 0.0f;
                    PushDirection.Normalize();
                    FVector LaunchVelocity = PushDirection * KnockbackStrength;
                    LaunchVelocity.Z = 0.0f;
                    MainCharacter->LaunchCharacter(LaunchVelocity, true, false);

                    // 3. '빅 히트' 리액션 호출 (애니메이션 재생)
                    MainCharacter->PlayBigHitReaction();

                    // 4. 사운드 재생 (1회 보장 로직 내장)
                    PlayShieldHitSound();

                    // 5. 데미지 적용 (DamagedActors에 추가되어 다음 틱 체크에 걸림)
                    ApplyDamage(HitActor);

                    // 플레이어를 한 번 때렸으면 이번 틱의 판정은 즉시 종료
                    return;
                }
            }
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
        PlayShieldHitSound();
        DamagedActors.Add(OtherActor); // 데미지 입힌 목록에 추가 (중복 데미지 방지)
    }
}

void AEnemyGuardianShield::PlayShieldHitSound()
{
    if (bHasPlayedHitSound) return; // 이미 재생했다면 중복 실행 방지

    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetOwner());
    if (Guardian)
    {
        Guardian->PlayShieldHitSound(); // 소유자(가디언)에게 사운드 재생 요청
        bHasPlayedHitSound = true; // 플래그 설정
    }
}