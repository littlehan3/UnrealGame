#include "EnemyBossKatana.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MainCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "BossEnemy.h"
#include "NiagaraFunctionLibrary.h"

AEnemyBossKatana::AEnemyBossKatana()
{
    PrimaryActorTick.bCanEverTick = false;

    KatanaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaMesh"));
    RootComponent = KatanaMesh; // 루트 컴포넌트 초기화

    KatanaChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaChildMesh"));
    KatanaChildMesh->SetupAttachment(KatanaMesh);  // 루트 컴포넌트의 자식으로 붙이고 초기화
}

void AEnemyBossKatana::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (!World) return;

	TArray<AActor*> TempBossEnemyActorsCache; // 임시 보스 적 액터 캐시
	UGameplayStatics::GetAllActorsOfClass(World, ABossEnemy::StaticClass(), TempBossEnemyActorsCache); // 모든 보스 적 액터 가져오기

	EnemyActorsCache.Empty(); // 캐시 초기화
	EnemyActorsCache.Append(TempBossEnemyActorsCache); // 캐시 채우기
}

//void AEnemyBossKatana::Tick(float DeltaTime)
//{
//    Super::Tick(DeltaTime);
//
//    if (bIsAttacking)
//    {
//        PerformRaycastAttack();
//    }
//}

void AEnemyBossKatana::StartAttack()
{
	bIsAttacking = true; // 공격 상태 활성화
	RaycastHitActors.Empty(); // 히트 액터 목록 초기화
	DamagedActors.Empty(); // 데미지 액터 목록 초기화
	bHasPlayedHitSound = false; // 히트 사운드 재생 플래그 초기화
}

void AEnemyBossKatana::EndAttack()
{
	bIsAttacking = false; // 공격 상태 비활성화
	RaycastHitActors.Empty(); // 히트 액터 목록 초기화
	DamagedActors.Empty(); // 데미지 액터 목록 초기화
	bHasPlayedHitSound = false; // 히트 사운드 재생 플래그 초기화
}

void AEnemyBossKatana::EnableAttackHitDetection()
{
	UWorld* World = GetWorld();
	if (!World) return;

	bIsAttacking = true; // 공격 상태 활성화
	RaycastHitActors.Empty(); // 히트 액터 목록 초기화
	DamagedActors.Empty(); // 데미지 액터 목록 초기화
	bHasPlayedHitSound = false; // 히트 사운드 재생 플래그 초기화

	// Tick 대신 타이머 시작
	TWeakObjectPtr<AEnemyBossKatana> WeakThis(this); // 약참조 생성
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

void AEnemyBossKatana::DisableAttackHitDetection()
{
    UWorld* World = GetWorld();
    if (!World) return;

    World->GetTimerManager().ClearTimer(AttackTraceTimerHandle);

	bIsAttacking = false; // 공격 상태 비활성화
	RaycastHitActors.Empty(); // 히트 액터 목록 초기화
	DamagedActors.Empty(); // 데미지 액터 목록 초기화
}

void AEnemyBossKatana::PerformRaycastAttack()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (!bIsAttacking || !IsValid(KatanaChildMesh)) return;

    FVector Start = KatanaChildMesh->GetComponentLocation(); // 자식 메쉬 기준 start
    FVector Forward = KatanaChildMesh->GetForwardVector();  // 자식 메쉬 기준 end

    float TotalDistance = 150.0f;
    int NumSteps = 5;
    float StepLength = TotalDistance / NumSteps;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		Params.AddIgnoredActor(GetOwner());
	}

    /*FVector SweepLineEnd = Start + Forward * TotalDistance;*/
 /*   DrawDebugLine(GetWorld(), Start, SweepLineEnd, FColor::Green, false, 5.0f, SDPG_Foreground, 3.0f);*/

    for (int i = 0; i < NumSteps; i++)
    {
        FVector SweepStart = Start + Forward * StepLength * i;
        FVector SweepEnd = Start + Forward * StepLength * (i + 1);

		TArray<FHitResult> OutHits; // 히트 결과 배열
        bool bHit = World->SweepMultiByChannel(
			OutHits, //  여러 히트 결과 수집
			SweepStart, // 스윕 시작 지점
			SweepEnd, // 스윕 끝 지점
			FQuat::Identity, // 회전 없음
			ECC_Pawn, // 폰 채널로 충돌 검사
			FCollisionShape::MakeSphere(30.0f), // 반지름 30cm 구체 스윕
			Params // 충돌 쿼리 파라미터
        );

		if (bHit) // 히트가 발생한 경우
        {
			for (const FHitResult& Hit : OutHits) // 각 히트 결과에 대해
            {
				AActor* HitActor = Hit.GetActor(); // 히트한 액터 가져오기
				if (!HitActor) continue; // 유효성 검사

                // 메인 캐릭터만 필터링
				if (HitActor->IsA(AMainCharacter::StaticClass())) // 메인 캐릭터인지 확인
                {
					RaycastHitActors.Add(HitActor); // 히트한 액터 기록
					ApplyDamage(HitActor); // 데미지 적용

					ABossEnemy* BossEnemyOwner = Cast<ABossEnemy>(GetOwner()); // 오너를 BossEnemy로 캐스팅
					if (BossEnemyOwner) // Niagar 효과 재생
                    {
						if (class UNiagaraSystem* HitNiagara = BossEnemyOwner->GetWeaponHitNiagaraEffect()) // 니아가라 이펙트 가져오기
                        {
                            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                                World,
                                HitNiagara,
                                Hit.ImpactPoint, // 히트 지점 사용
                                Hit.Normal.Rotation(), // 피격 면의 노멀 방향 사용
								FVector(1.0f), // 스케일
								true, // 자동 파괴
								true //, 자동 활성화
                            );
                        }
                    }

					PlayKatanaHitSound(); // 히트 사운드 재생
                   /* DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 30.0f, 12, FColor::Red, false, 0.3f, SDPG_Foreground, 4.0f);*/
                    return; // 첫 맞은 메인 캐릭터에서 종료
                }
            }
        }
    }
}

void AEnemyBossKatana::ApplyDamage(AActor* OtherActor)
{
	if (!OtherActor || DamagedActors.Contains(OtherActor)) return; // 유효성 검사 및 중복 데미지 방지

    AActor* KatanaOwner = GetOwner(); // 카타나 소유자 가져옴
    if (OtherActor == KatanaOwner) return; // 자기 자신 무시

	if (OtherActor->IsA(ABossEnemy::StaticClass())) return; // 적 캐릭터 무시

	if (RaycastHitActors.Contains(OtherActor)) // 플레이어인지 확인
    {
		UGameplayStatics::ApplyDamage(OtherActor, BossDamage, nullptr, this, nullptr); // 데미지 적용
		DamagedActors.Add(OtherActor); // 데미지 적용된 액터 기록
    }
}

void AEnemyBossKatana::PlayKatanaHitSound()
{
	if (bHasPlayedHitSound) return; // 이미 사운드 재생되었으면 무시

	ABossEnemy* Boss = Cast<ABossEnemy>(GetOwner()); // 소유자를 BossEnemy로 캐스팅
	if (Boss) // 유효하면 사운드 재생
    { 
		Boss->PlayWeaponHitSound(); // 사운드 재생 함수 호출
		bHasPlayedHitSound = true; // 재생 플래그 설정
    }
}

void AEnemyBossKatana::HideKatana()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 1. 이벤트 및 타이머 정리 (최우선)
	World->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 해제로 콜백 함수 호출 차단

	// 2. 상태 플래그 정리
    bIsAttacking = false;

	// 3. 배열 데이터 정리
	RaycastHitActors.Empty(); // 히트 액터 목록 완전 삭제
	DamagedActors.Empty(); // 데미지 액터 목록 완전 삭제
	EnemyActorsCache.Empty(); // 캐시된 적 목록 완전 삭제

	// 4. 소유자 관계 해제 (컴포넌트 정리 전)
	SetOwner(nullptr); // 소유자 관계 해제

	// 5. 컴포넌트 시스템 정리
	if (KatanaMesh && IsValid(KatanaMesh) && !KatanaMesh->IsBeingDestroyed()) // 메시 컴포넌트가 유효하고 파괴 중이 아니면
    {
		// 렌더링 시스템 비활성화
        KatanaMesh->SetVisibility(false);
		KatanaMesh->SetHiddenInGame(true);

		KatanaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 물리 시스템 비활성화
		KatanaMesh->SetCollisionResponseToAllChannels(ECR_Ignore); // 모든 채널 무시

		KatanaMesh->SetComponentTickEnabled(false); // 업데이트 시스템 비활성화

		KatanaMesh->SetStaticMesh(nullptr); // 참조 해제

		KatanaMesh->DestroyComponent(); // 컴포넌트 완전 제거
    }

	// 6. 액터 자체의 시스템 정리
	SetActorHiddenInGame(true); // 액터를 게임 내에서 숨김
	SetActorEnableCollision(false); // 액터의 충돌 비활성화
	SetActorTickEnabled(false); // 액터의 틱 비활성화
	
	// 7. 다음 프레임에 안전하게 액터 제거 (크래쉬 방지)
	TWeakObjectPtr<AEnemyBossKatana>WeakThis(this);
    World->GetTimerManager().SetTimerForNextTick(
		[WeakThis]() // 람다 함수
        {
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // 유효성 검사
            {
				WeakThis->Destroy(); // 액터 제거
            }
        });
}
