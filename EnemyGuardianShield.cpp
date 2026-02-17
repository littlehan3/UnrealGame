#include "EnemyGuardianShield.h"
#include "EnemyGuardian.h" // 소유자 클래스 참조
#include "MainCharacter.h" // 플레이어 캐릭터 클래스 참조
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수 사용
#include "DrawDebugHelpers.h" // 디버그 시각화 기능 사용
#include "Engine/World.h" // UWorld 사용

AEnemyGuardianShield::AEnemyGuardianShield()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화
	PrimaryActorTick.bStartWithTickEnabled = false; // 기본적으로 Tick 비활성화

    ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
    RootComponent = ShieldMesh; // 루트 컴포넌트로 설정

    ShieldChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldChildMesh"));
    ShieldChildMesh->SetupAttachment(ShieldMesh); // 부모 메쉬에 부착
}

void AEnemyGuardianShield::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // 부모 클래스 Tick 호출

    // 공격 활성화 시 Tick에서 PerformRaycastAttack 호출
    if (bIsAttacking)
    {
		PerformRaycastAttack(); // 실제 공격 판정 수행
    }
}

void AEnemyGuardianShield::StartShieldAttack()
{
    bIsAttacking = true; // 공격 판정 활성화
    RaycastHitActors.Empty(); // 이전 공격 기록 초기화
    DamagedActors.Empty();
    bHasPlayedHitSound = false;
	SetActorTickEnabled(true); // 공격시작 시 Tick 활성화
}

void AEnemyGuardianShield::EndShieldAttack()
{
    bIsAttacking = false; // 공격 판정 비활성화
    RaycastHitActors.Empty(); // 공격 기록 초기화
    DamagedActors.Empty();
    bHasPlayedHitSound = false;
	SetActorTickEnabled(false); // 공격이 끝나면 Tick 비활성화
}

void AEnemyGuardianShield::PerformRaycastAttack()
{
    if (!bIsAttacking || !GetOwner()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FVector Start = ShieldMesh->GetComponentLocation(); // 방패 위치를 판정 시작점으로 사용
	FVector Forward = GetOwner()->GetActorForwardVector(); // 소유자(가디언)의 정면 방향
    float StepLength = TotalDistance / NumSteps; // 한 스텝당 거리

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 자기 자신 무시
    Params.AddIgnoredActor(GetOwner()); // 소유자(가디언) 무시

    // 월드의 모든 아군 가디언도 무시 (기존 로직 유지)
    TArray<AActor*> EnemyActors;
    UGameplayStatics::GetAllActorsOfClass(World, AEnemyGuardian::StaticClass(), EnemyActors);
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
        bool bHit = World->SweepMultiByChannel(
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

				AMainCharacter* MainCharacter = Cast<AMainCharacter>(HitActor); // 맞은 대상이 플레이어라면
                if (MainCharacter)
                {
                    // 이미 처리된 대상인지 확인 (Tick 중복 실행 방지)
                    if (DamagedActors.Contains(HitActor)) return;

                    // 1. 감지 목록에 추가
                    RaycastHitActors.Add(HitActor);

                    // 2. 넉백 적용 (물리 효과)
					FVector PushDirection = GetOwner()->GetActorForwardVector(); // 가디언의 정면 방향
					PushDirection.Z = 0.0f; // 수평 방향으로만 밀치기
					PushDirection.Normalize(); // 방향 정규화
					FVector LaunchVelocity = PushDirection * KnockbackStrength; // 넉백 속도 계산
					LaunchVelocity.Z = 0.0f; // 수평 넉백만 적용
					MainCharacter->LaunchCharacter(LaunchVelocity, true, false); // 넉백 적용

                    // 3. 빅히트 리액션 호출 (애니메이션 재생)
                    MainCharacter->PlayBigHitReaction();

                    // 4. 사운드 재생
                    PlayShieldHitSound();

                    // 5. 데미지 적용
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

void AEnemyGuardianShield::HideShield()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 1. 타이머 정리
    World->GetTimerManager().ClearAllTimersForObject(this);

    // 2. 상태 정리
    bIsAttacking = false; // 공격 상태 비활성화

	// 3. 배열 데이터 정리
	RaycastHitActors.Empty(); // 히트 액터 목록 완전 삭제
    DamagedActors.Empty();

    // 4. 소유자 관계 해제
	SetOwner(nullptr); // 순환 참조 방지를 위해 소유자 관계 먼저 해제

    // 5. 컴포넌트 정리
	if (ShieldMesh && IsValid(ShieldMesh) && !ShieldMesh->IsBeingDestroyed()) // 메시 컴포넌트가 유효하고 파괴 중이 아니면
    {
        // 렌더링 시스템
		ShieldMesh->SetVisibility(false); // 렌더링 시스템 비활성화
		ShieldMesh->SetHiddenInGame(true); // 게임 내 숨김
        // 물리 시스탬
		ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 물리 시스템 비활성화
		ShieldMesh->SetCollisionResponseToAllChannels(ECR_Ignore); // 모든 채널 무시
		ShieldMesh->SetComponentTickEnabled(false); // 업데이트 시스템 비활성화
		ShieldMesh->SetStaticMesh(nullptr); // 참조 해제
		ShieldMesh->DestroyComponent(); // 컴포넌트 완전 제거
    }

    // 6. 액터 정리
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);

    // 다음 프레임에 안전하게 액터 제거
    TWeakObjectPtr<AEnemyGuardianShield> WeakThis(this);
    World->GetTimerManager().SetTimerForNextTick(
        [WeakThis]()
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
            {
                WeakThis->Destroy();
                UE_LOG(LogTemp, Warning, TEXT("EnemyGuardianShield Successfully Destroyed."));
            }
        });
}