#include "EnemyKatana.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MainCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Enemy.h"
#include "MainCharacter.h"
#include "NiagaraFunctionLibrary.h"

AEnemyKatana::AEnemyKatana()
{
	PrimaryActorTick.bCanEverTick = false; // Tick 비활성화

    // 카타나 메시 초기화 및 RootComponent로 설정
	KatanaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaMesh")); // 메시 컴포넌트 생성
	RootComponent = KatanaMesh; // 루트 컴포넌트로 설정

	KatanaChildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaChildMesh")); // 자식 메시 컴포넌트 생성
    KatanaChildMesh->SetupAttachment(KatanaMesh);  // 루트 컴포넌트의 자식으로 붙이고 초기화
}

void AEnemyKatana::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (!World) return;

    TArray<AActor*> TempEnemyActorsCache;
    UGameplayStatics::GetAllActorsOfClass(World, AEnemy::StaticClass(), TempEnemyActorsCache);

    EnemyActorsCache.Empty();
    EnemyActorsCache.Append(TempEnemyActorsCache);
}

//void AEnemyKatana::Tick(float DeltaTime) 
//{
//    Super::Tick(DeltaTime);
//    //UE_LOG(LogTemp, Warning, TEXT("Katana Tick: %d"), bIsAttacking); // Tick 활성화 확인
//
//    if (bIsAttacking)
//    {
//        //UE_LOG(LogTemp, Warning, TEXT("Performing Raycast Attack")); // 공격 실행 확인
//        PerformRaycastAttack();
//    }
//}

// 공격 시작
void AEnemyKatana::StartAttack() 
{
	bIsAttacking = true; // 공격 상태 활성화
	RaycastHitActors.Empty(); // 히트 액터 목록 초기화
	DamagedActors.Empty(); // 데미지 액터 목록 초기화
	bHasPlayedHitSound = false; // 히트 사운드 재생 플래그 초기화
}

// 공격 종료
void AEnemyKatana::EndAttack()
{
	bIsAttacking = false; // 공격 상태 비활성화
	bIsStrongAttack = false; // 강공격 상태 비활성화
	RaycastHitActors.Empty(); // 히트 액터 목록 초기화
	DamagedActors.Empty(); // 데미지 액터 목록 초기화
	bHasPlayedHitSound = false; // 히트 사운드 재생 플래그 초기화
}

void AEnemyKatana::EnableAttackHitDetection(EAttackType AttackType)
{
    UWorld* World = GetWorld();
    if (!World) return;

    bIsAttacking = true; // 공격 상태 활성화 추가
	CurrentAttackType = AttackType; // 현재 공격 타입 설정
	RaycastHitActors.Empty(); // 히트 액터 목록 초기화
	DamagedActors.Empty(); // 데미지 액터 목록 초기화
	bHasPlayedHitSound = false; // 히트 사운드 재생 플래그 초기화

    // Tick 대신 타이머 시작
    // TraceInterval마다 PerformRaycastAttack 함수를 반복 호출
	TWeakObjectPtr<AEnemyKatana> WeakThis(this); // 약참조 생성
    World->GetTimerManager().SetTimer(
        AttackTraceTimerHandle,
        [WeakThis]()
        {
			if (WeakThis.IsValid()) // 유효성 검사
            {
				WeakThis->PerformRaycastAttack(); // 레이캐스트 공격 함수 호출
            }
        }, TraceInterval, true); // 반복 실행

    UE_LOG(LogTemp, Warning, TEXT("StrongAttack Active: %d"), bIsStrongAttack);
}

void AEnemyKatana::DisableAttackHitDetection()
{
    UWorld* World = GetWorld();
	if (!World) return;

    World->GetTimerManager().ClearTimer(AttackTraceTimerHandle);
	bIsAttacking = false; // 공격 상태 비활성화
	RaycastHitActors.Empty(); // 히트 액터 목록 초기화
	DamagedActors.Empty(); // 데미지 액터 목록 초기화
}

void AEnemyKatana::PerformRaycastAttack()
{
    UWorld* World = GetWorld(); 
    if (!World) return;
    if (!bIsAttacking || !IsValid(KatanaChildMesh)) return;

    FVector Start = KatanaChildMesh->GetComponentLocation(); // 자식 메쉬 기준 start
    FVector Forward = KatanaChildMesh->GetForwardVector();  // 자식 메쉬 기준 end
	float StepLength = TotalDistance / NumSteps; // 각 스텝의 길이

	FCollisionQueryParams Params; // 충돌 쿼리 파라미터 설정
	Params.AddIgnoredActor(this); // 자기 자신 무시

	AActor* MyOwner = GetOwner(); // 오너 가져옴
    if (MyOwner) 
    {
		Params.AddIgnoredActor(GetOwner()); // 오너 무시
    }

    // Sweep 전체 경로에 초록색 선만 한 번만 그림
 /*   FVector SweepLineEnd = Start + Forward * TotalDistance;
    DrawDebugLine(GetWorld(), Start, SweepLineEnd, FColor::Green, false, 5.0f, SDPG_Foreground, 3.0f);*/

	for (int i = 0; i < NumSteps; i++) // 각 스텝마다 스윕 수행
    {
		FVector SweepStart = Start + Forward * StepLength * i; // 각 스텝의 시작 지점
		FVector SweepEnd = Start + Forward * StepLength * (i + 1); // 각 스텝의 끝 지점

		TArray<FHitResult> OutHits; // 히트 결과 배열
        bool bHit = World->SweepMultiByChannel(
			OutHits, // 여러 히트 결과 수집
			SweepStart, // 스윕 시작 지점
			SweepEnd, // 스윕 끝 지점
			FQuat::Identity, // 회전 없음
			ECC_Pawn, // 폰 채널로 충돌 검사
			FCollisionShape::MakeSphere(HitRadius), // 반지름 30cm 구체 스윕
			Params // 충돌 쿼리 파라미터
        );

		if (bHit) // 히트가 발생한 경우
        {
			for (const FHitResult& Hit : OutHits) // 각 히트 결과에 대해
            {
				AActor* HitActor = Hit.GetActor(); // 히트한 액터 가져오기
				if (!HitActor) continue; // 유효성 검사

                // 메인 캐릭터만 피해 처리
				if (HitActor->IsA(AMainCharacter::StaticClass())) // 메인 캐릭터인지 확인
                {
					RaycastHitActors.Add(HitActor); // 히트한 액터 기록
					ApplyDamage(HitActor); // 데미지 적용

					AEnemy* EnemyOwner = Cast<AEnemy>(GetOwner()); // 오너를 Enemy로 캐스팅
					if (EnemyOwner) // Niagar 효과 재생
                    {
						if (class UNiagaraSystem* HitNiagara = EnemyOwner->GetWeaponHitNiagaraEffect()) // 니아가라 이펙트 가져오기
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
                    // 맞은 위치에 빨간 구 표시
                    /*DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 30.0f, 12, FColor::Red, false, 0.3f, SDPG_Foreground, 4.0f);*/
                    return;  // 첫 번째 맞은 메인 캐릭터에서 함수 종료
                }
            }
        }
    }
}

void AEnemyKatana::ApplyDamage(AActor* OtherActor)
{
	if (!OtherActor || DamagedActors.Contains(OtherActor)) return; // 이미 데미지 적용된 액터는 무시

	AActor* KatanaOwner = GetOwner(); // 카타나 소유자 가져옴
	if (OtherActor == KatanaOwner) return; // 자기 자신 무시

    // 다른 적 캐릭터인지 확인
	if (OtherActor->IsA(AEnemy::StaticClass())) return; // 적 캐릭터 무시

    // 플레이어만 데미지 적용
	if (RaycastHitActors.Contains(OtherActor)) // 플레이어인지 확인
    {
		//float DamageAmount = 0.0f; // 데미지 양
		FString AttackTypeStr; // 공격 타입 문자열

        AEnemy* EnemyOwner = Cast<AEnemy>(KatanaOwner); // Enemy 클래스에만 bIsEliteEnemy가 있으니까 카타나 오너를 Enemy로 캐스팅
		bool bIsElite = (EnemyOwner && EnemyOwner->bIsEliteEnemy); // 앨리트 적인지 확인
		AMainCharacter* MainCharacter = Cast<AMainCharacter>(OtherActor); // 다른 액터를 메인 캐릭터로 캐스팅

		switch (CurrentAttackType) // 공격 타입에 따른 데미지 설정
        {
		case EAttackType::Normal: // 일반 공격
			DamageAmount = bIsElite ? EliteDamageAmount : DamageAmount; // 일반이면 20, 엘리트면 30
            AttackTypeStr = TEXT("NormalAttack");
            break;
		case EAttackType::Strong: // 강공격 
            DamageAmount = bIsElite ? EliteStrongDamageAmount : StrongDamageAmount; // 강공격이면 50, 엘리트면 60
            AttackTypeStr = TEXT("StrongAttack");

			if (MainCharacter) // 메인캐릭터가 맞았을
            {
				MainCharacter->PlayBigHitReaction(); // 강공격 리액션 함수 호출
            }

            break;
		case EAttackType::Jump:  // 점프 공격
			DamageAmount = bIsElite ? EliteJumpDamageAmount : JumpDamageAmount; // 점프 공격이면 30, 엘리트면 40
            AttackTypeStr = TEXT("JumpAttack");
            break;
        }

		UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, nullptr, this, nullptr); // 데미지 적용
        UE_LOG(LogTemp, Warning, TEXT("AttackType: %s, Damage: %f IsElite: %d"), *AttackTypeStr, DamageAmount, bIsElite);

		DamagedActors.Add(OtherActor); // 데미지 적용된 액터 기록
    }
}

void AEnemyKatana::PlayKatanaHitSound()
{
	if (bHasPlayedHitSound) return; // 이미 사운드 재생되었으면 무시

	AEnemy* Enemy = Cast<AEnemy>(GetOwner()); // 소류자를 Enemy로 캐스팅
	if (Enemy) // 유효하면 사운드 재생
    {
		Enemy->PlayWeaponHitSound(); // 사운드 재생 함수 호출
		bHasPlayedHitSound = true; // 재생 플래그 설정
    }
}

void AEnemyKatana::HideKatana()
{
	UWorld* World = GetWorld();
	if (!World) return;

    UE_LOG(LogTemp, Warning, TEXT("Hiding Katana Memory Cleanup"));

    // 1. 이벤트 및 타이머 정리 (최우선)
    World->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 해제로 콜백 함수 호출 차단

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
	if (KatanaMesh && IsValid(KatanaMesh) && !KatanaMesh->IsBeingDestroyed()) // 메시 컴포넌트가 유효하고 파괴 중이 아니면
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
	TWeakObjectPtr<AEnemyKatana> WeakThis(this);
    World->GetTimerManager().SetTimerForNextTick(
        [WeakThis]() // 스마트 포인터 WeakObjectPtr로 약한 참조를 사용하여 안전하게 지연 실행
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // 약한 참조한 엑터가 유효하고 파괴되지 않았다면
            {
                WeakThis->Destroy(); // 액터 완전 제거
                UE_LOG(LogTemp, Warning, TEXT("EnemyKatana Successfully Destroyed."));
            }
        });
}
