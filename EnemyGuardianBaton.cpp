#include "EnemyGuardianBaton.h"
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수 사용
#include "MainCharacter.h" // 플레이어 캐릭터 클래스 참조
#include "DrawDebugHelpers.h" // 디버그 시각화 기능 사용
#include "EnemyGuardian.h" // 소유자 클래스 참조
#include "NiagaraFunctionLibrary.h" // 나이아가라 이펙트 스폰 함수 사용

AEnemyGuardianBaton::AEnemyGuardianBaton()
{
    PrimaryActorTick.bCanEverTick = false; // Tick 비활성화

	// 진압봉 메시 초기화 및 RootComponent로 설정
	BatonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatonMesh")); // 메시 컴포넌트 생성
    RootComponent = BatonMesh; // 루트 컴포넌트로 설정

	BatonChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatonChildMesh")); // 자식 메시 컴포넌트 생성
    BatonChildMesh->SetupAttachment(BatonMesh); // 부모 메쉬에 부착
}

void AEnemyGuardianBaton::BeginPlay()
{
	UWorld* World = GetWorld();
	if (!World) return;

    Super::BeginPlay(); // 부모 클래스 BeginPlay 호출

	UGameplayStatics::GetAllActorsOfClass(World, AEnemyGuardian::StaticClass(), EnemyActorsCache); // 모든 아군 가디언 액터 캐싱
}

void AEnemyGuardianBaton::StartAttack()
{
    bIsAttacking = true; // 공격 판정 활성화
    RaycastHitActors.Empty(); // 이전 공격 기록 초기화
    DamagedActors.Empty();
    bHasPlayedHitSound = false;
}

void AEnemyGuardianBaton::EndAttack()
{
    bIsAttacking = false; // 공격 판정 비활성화
    RaycastHitActors.Empty(); // 공격 기록 초기화
    DamagedActors.Empty();
    bHasPlayedHitSound = false;
}

void AEnemyGuardianBaton::EnableAttackHitDetection()
{
	UWorld* World = GetWorld();
	if (!World) return;

    bIsAttacking = true; // 공격 판정 활성화
    RaycastHitActors.Empty(); // 이전 공격 기록 초기화
    DamagedActors.Empty();
    bHasPlayedHitSound = false;

    //TIck 대신 타이머 시작
	// TraceInterval마다 PerformRaycastAttack 함수를 반복 호출
	TWeakObjectPtr<AEnemyGuardianBaton> WeakThis(this); // 약참조 생성
	World->GetTimerManager().SetTimer(
		AttackTraceTimerHandle,
		[WeakThis]()
		{
			if (WeakThis.IsValid()) // 유효성 검사
			{
				WeakThis->PerformRaycastAttack(); // 레이캐스트 공격 함수 호출
			}
		}, TraceInterval, true); // 반복 실행
}

void AEnemyGuardianBaton::DisableAttackHitDetection()
{
	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().ClearTimer(AttackTraceTimerHandle); // 판정 종료 시 타이머 해제
    bIsAttacking = false; // 공격 판정 비활성화
    RaycastHitActors.Empty(); // 공격 기록 초기화
    DamagedActors.Empty();
    bHasPlayedHitSound = false;
}

void AEnemyGuardianBaton::PerformRaycastAttack()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (!bIsAttacking || !IsValid(BatonChildMesh)) return;

    FVector Start = BatonChildMesh->GetComponentLocation(); // 자식 메쉬 위치를 판정 시작점으로 사용
    FVector Forward = BatonChildMesh->GetForwardVector(); // 자식 메쉬의 정면 방향
    float StepLength = TotalDistance / NumSteps; // 한 스텝당 거리

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 자기 자신 무시
    if (GetOwner())
    {
        Params.AddIgnoredActor(GetOwner()); // 소유자(가디언) 무시
    }

    // 스윕(Sweep)은 선이 아닌 특정 형태(구체)를 이동시키며 충돌을 검사하는 방식
    for (int i = 0; i < NumSteps; ++i)
    {
        FVector SweepStart = Start + Forward * StepLength * i;
        FVector SweepEnd = Start + Forward * StepLength * (i + 1);

        TArray<FHitResult> OutHits;
        // 반경 30의 구체를 이동시키며 Pawn 채널과 충돌하는 모든 대상을 찾음
        bool bHit = World->SweepMultiByChannel(
            OutHits, 
            SweepStart, 
            SweepEnd, 
            FQuat::Identity,
            ECC_Pawn, 
            FCollisionShape::MakeSphere(HitRadius),
            Params
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

                    AEnemyGuardian* GuardianOwner = Cast<AEnemyGuardian>(GetOwner());
                    if (GuardianOwner)
                    {
                        // AEnemyGuardian 클래스의 GetWeaponHitNiagaraEffect()
                        if (class UNiagaraSystem* HitNiagara = GuardianOwner->GetWeaponHitNiagaraEffect())
                        {
                            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                                World,
                                HitNiagara,
                                Hit.ImpactPoint, // 히트 지점 사용
                                Hit.Normal.Rotation(), // 피격 면의 노멀 방향 사용
                                FVector(1.0f),
                                true,
                                true
                            );
                        }
                    }
                    PlayWeaponHitSound();

                    return; // 플레이어를 한 번 때렸으면 이번 틱의 판정은 종료
                }
            }
        }
    }
}

void AEnemyGuardianBaton::ApplyDamage(AActor* OtherActor)
{
    // 대상이 없거나, 이미 데미지를 입혔거나, 소유자 자신이면 데미지 적용 안함
    if (!OtherActor || DamagedActors.Contains(OtherActor)) return; 

    AActor* BatonOwner = GetOwner(); // 카타나 소유자 가져옴
    if (OtherActor == BatonOwner) return; // 자기 자신 무시

    // 다른 적 캐릭터인지 확인
    if (OtherActor->IsA(AEnemyGuardian::StaticClass())) return; // 적 캐릭터 무시

    if (RaycastHitActors.Contains(OtherActor)) // 감지 목록에 있는 대상이라면
    {
        UGameplayStatics::ApplyDamage(OtherActor, BatonDamage, nullptr, this, nullptr); // 데미지 적용
        DamagedActors.Add(OtherActor); // 데미지 입힌 목록에 추가 (중복 데미지 방지)
    }
}

void AEnemyGuardianBaton::PlayWeaponHitSound()
{
    // 이미 재생했다면 중복 실행 방지
    if (bHasPlayedHitSound) return;

    // 소유자(가디언)에게 캐스팅하여 사운드 재생 함수 호출
    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetOwner());
    if (Guardian)
    {
        Guardian->PlayWeaponHitSound();
        bHasPlayedHitSound = true; // 플래그 설정
    }
}

void AEnemyGuardianBaton::HideBaton()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 1. 타이머 정리
	World->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 해제

    // 2. 상태정리
	bIsAttacking = false; // 공격 상태 비활성화

    // 3. 배열 데이터 정리
	RaycastHitActors.Empty(); // 히트 액터 목록 완전 삭제
	DamagedActors.Empty(); // 데미지 액터 목록 완전 삭제
	EnemyActorsCache.Empty(); // 캐시된 적 목록 완전 삭제

	// 4. 소유자 관계 해제 (컴포넌트 정리 전)
	SetOwner(nullptr); // 순환 참조 방지를 위해 소유자 관계 먼저 해제

    // 5. 컴포넌트 정리
	if (BatonMesh && IsValid(BatonMesh) && !BatonMesh->IsBeingDestroyed()) // 메시 컴포넌트가 유효하고 파괴 중이 아니면
    {
        // 렌더링 시스템 비활성화
        BatonMesh->SetVisibility(false);
        BatonMesh->SetHiddenInGame(true);

		// 물리 시스템 비활성화
        BatonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BatonMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

		// 업데이트 시스템 비활성화
        BatonMesh->SetComponentTickEnabled(false);

		// 참조 해제
        BatonMesh->SetStaticMesh(nullptr);

		// 컴포넌트 완전 제거 (무기는 별도 컴포넌트이므로 안전)
        BatonMesh->DestroyComponent();
    }

    // 6. 액터 레벨 시스템 정리
	SetActorHiddenInGame(true); // 액터 렌더링 비활성화
	SetActorEnableCollision(false); // 액터 충돌 비활성화
	SetActorTickEnabled(false); // 액터 틱 비활성화

    // 7. 현재 프레임 처리 완료 후 다음 프레임에 안전하게 액터 제거 (크래쉬 방지)
	TWeakObjectPtr<AEnemyGuardianBaton> WeakThis(this); // 약참조 생성
	World->GetTimerManager().SetTimerForNextTick( // 다음 틱에 실행
		[WeakThis]() // 람다 함수
        {
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // 약참조한 엑터가 유효하고 파괴되지 않았다면
            {
				WeakThis->Destroy(); // 액터 완전 제거
                UE_LOG(LogTemp, Warning, TEXT("EnemyGuardianBaton Successfully Destroyed."));
            }
        });
}