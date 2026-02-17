#include "MeleeCombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Knife.h"
#include "Enemy.h"
#include "MainCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "EnemyGuardian.h"
#include "EnemyShooter.h"
#include "EnemyDog.h"
#include "BossEnemy.h"
#include "NiagaraFunctionLibrary.h" // 나이아가라 재생용
#include "NiagaraSystem.h"           // UNiagaraSystem 클래스용
#include "Components/BoxComponent.h"

UMeleeCombatComponent::UMeleeCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Tick 사용안함
}

// 캐릭터 생성 후 무기 및 컴포넌트 참조를 안전하게 연결하는 초기화 함수
void UMeleeCombatComponent::InitializeCombatComponent(ACharacter* InOwnerCharacter, UBoxComponent* InKickHitBox, AKnife* InLeftKnife, AKnife* InRightKnife)
{
    if (!InOwnerCharacter) return;

	OwnerCharacter = InOwnerCharacter; // 소유 캐릭터 참조 저장
	LeftKnife = InLeftKnife; // 왼쪽 나이프 참조 저장
	RightKnife = InRightKnife; // 오른쪽 나이프 참조 저장

	if (InKickHitBox) // 킥 히트박스 참조 저장 및 오버랩 이벤트 바인딩
    {
		KickHitBox = InKickHitBox; // 킥 히트박스 참조 저장 
		KickHitBox->OnComponentBeginOverlap.AddDynamic(this, &UMeleeCombatComponent::HandleKickOverlap); // 오버랩 이벤트 바인딩
    }
}

// 콤보 몽타주 설정 함수
void UMeleeCombatComponent::SetComboMontages(const TArray<UAnimMontage*>& InMontages)
{
	ComboMontages = InMontages; // 콤보 몽타주 배열 설정
}

// 콤보 공격 트리거 함수
void UMeleeCombatComponent::TriggerComboAttack()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (!OwnerCharacter || OwnerCharacter->GetCharacterMovement()->IsFalling()) return; // 점프 중이면 콤보 공격 불가

	if (bInputBlocked) // 입력이 블록된 상태인지 확인
    {
        UE_LOG(LogTemp, Warning, TEXT("Combo Attack Input Blocked - Cooldown Active"));
		return; // 입력 블록 상태면 무시
    }
    if (bIsAttacking) // 이미 공격 중이면
    {
		bComboQueued = true; // 콤보 공격 큐잉
        UE_LOG(LogTemp, Warning, TEXT("Combo Attack Queued - Already Attacking"));
		return; // 큐잉 후 종료
    }
	bInputBlocked = true; // 입력 블록 상태 설정
    // 입력 쿨타임 약참조 람다 적용
    TWeakObjectPtr<UMeleeCombatComponent> WeakThis(this);
    World->GetTimerManager().SetTimer(InputCooldownHandle, [WeakThis]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->ResetInputCooldown();
            }
        }, InputCooldownTime, false);

	ResetComboTimer(); // 콤보 리셋 타이머 시작
	bIsAttacking = true; // 공격 상태 설정
	bCanAirAction = false; // 공중 액션 불가 설정
    bComboQueued = false; // 큐 초기화
	AdjustAttackDirection(); // 공격 방향 자동 조정

	if (ComboIndex == 3) // 세번째 콤보인 발차기 공격일 때
    {
		EnableKickHitBox(); // 킥 히트박스 활성화
    }
	else if (IsValid(LeftKnife))// 나이프 공격일 때
    {
		LeftKnife->EnableHitBox(ComboIndex, KnifeKnockbackStrength); // 왼쪽 나이프 히트박스 활성화
    }
	PlayComboMontage(ComboIndex); // 콤보 몽타주 재생
}

// 콤보 몽타주 재생 함수
void UMeleeCombatComponent::PlayComboMontage(int32 Index)
{
	if (!IsValid(OwnerCharacter) || !ComboMontages.IsValidIndex(Index)) return; // 유효성 검사
	UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
	if (!AnimInstance) return; // 유효성 검사
	UAnimMontage* Montage = ComboMontages[Index]; // 재생할 몽타주 가져오기

    AnimInstance->OnMontageEnded.RemoveDynamic(this, &UMeleeCombatComponent::OnComboMontageEnded); // 델리게이트 중복 등록 방지 제거

    // 기존 몽타주가 재생 중이면 먼저 정지
	if (AnimInstance->Montage_IsPlaying(Montage)) // 이미 재생 중인지 확인
    {
		AnimInstance->Montage_Stop(MontageBlendOutTime, Montage); // 몽타주 블랜드아웃 시간으로 부드럽게 정지
    }
	float PlayResult = AnimInstance->Montage_Play(Montage, MontageDefaultPlayRate); // 몽타주 재생
	if (PlayResult > 0.f) // 재생 성공 시 콜백 등록
    {
        // 콜백 등록 전에 기존 콜백 제거
        AnimInstance->OnMontageEnded.RemoveDynamic(this, &UMeleeCombatComponent::OnComboMontageEnded);
        // 새로운 콜백 등록
		FOnMontageEnded EndDelegate; // 콤보 몽타주 종료 콜백 델리게이트
		EndDelegate.BindUObject(this, &UMeleeCombatComponent::OnComboMontageEnded); // 델리게이트 바인딩
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage); // 콜백 설정
    }
    else // 재생 실패 시
    {
        bIsAttacking = false; // 상태 복구
    }
}

// 콤보 리셋 함수
void UMeleeCombatComponent::ResetCombo()
{
    UWorld* World = GetWorld();
    if (!World) return;
	bIsAttacking = false; // 공격 상태 해제
	ComboIndex = 0; // 콤보 인덱스 리셋

	if (OwnerCharacter) // 소유 캐릭터가 유효하면
    {
		World->GetTimerManager().ClearTimer(ComboResetTimerHandle); // 콤보 리셋 타이머 정리
		UE_LOG(LogTemp, Warning, TEXT("Combo Reset")); // 디버그 로그
		if (OwnerCharacter->GetCharacterMovement()) // 캐릭터 무브먼트가 유효하면
        {
			OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향으로 회전 설정
        }
    }
}

// 콤보 리셋 타이머 시작 함수
void UMeleeCombatComponent::ResetComboTimer()
{
	if (OwnerCharacter) // 소유 캐릭터가 유효하면
    {
        UE_LOG(LogTemp, Warning, TEXT("ResetComboTimer Started"));
        UE_LOG(LogTemp, Warning, TEXT("ComboResetTime: %f"), ComboResetTime);
		// 타이머 설정
        OwnerCharacter->GetWorldTimerManager().SetTimer(ComboResetTimerHandle, this, &UMeleeCombatComponent::ResetCombo, ComboResetTime, false);
    }
}

// 콤보 몽타주 종료 콜백
void UMeleeCombatComponent::OnComboMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Warning, TEXT("OnComboMontageEnded called - Montage: %s"), Montage ? *Montage->GetName() : TEXT("NULL"));

	bIsAttacking = false; // 공격 상태 해제
    bCanAirAction = true; // 콤보 완료 시 공중 액션 허용

	if (OwnerCharacter && OwnerCharacter->GetCharacterMovement()) // 캐릭터 무브먼트가 유효하면
    {
		OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향으로 회전 설정
    }
	if (LeftKnife) LeftKnife->DisableHitBox(); // 왼쪽 나이프 히트박스 비활성화
	if (RightKnife) RightKnife->DisableHitBox(); // 오른쪽 나이프 히트박스 비활성화
	DisableKickHitBox(); // 킥 히트박스 비활성화

    // 콜백 제거
	if (IsValid(OwnerCharacter)) // 소유 캐릭터가 유효하면
    {
        if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh()) // 메시 가져옴
        {
            if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance()) // 애님 인스턴스 가져옴
            {
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &UMeleeCombatComponent::OnComboMontageEnded); // 델리게이트 제거
            }
        }
    }
	if (!LastAttackDirection.IsNearlyZero()) // 마지막 공격 방향이 유효하면
    {
		LastAttackDirection = OwnerCharacter->GetActorForwardVector(); // 기본 전방 방향으로 리셋
    }
	// 콤보 인덱스 증가 또는 리셋
    if (OwnerCharacter && OwnerCharacter->GetWorldTimerManager().IsTimerActive(ComboResetTimerHandle))
    {
		//ComboIndex = (ComboIndex + 1) % ComboMontages.Num(); // 콤보 시간이 남아있으면 인덱스 증가
        // 이 경우 몽타주 배열이 비어있을경우 나누기 0 방지하기 위해 아래와 같이 수정

        int32 MaxIndex = ComboMontages.Num();
        if (MaxIndex > 0) // 배열이 비어있지 않을 경우에만
        {
            // 인덱스 증가
            ComboIndex = (ComboIndex + 1) % MaxIndex;
        }
        else
        {
            // 배열이 비어있다면 인덱스는 0 고정
            ComboIndex = 0;
        }
    }
    else
    {
        // 콤보 시간이 끝났거나 유효하지 않으면 리셋 
        ComboIndex = 0; 
    }
	if (OwnerCharacter && (bComboQueued)) // 콤보가 큐에 있으면
    {
		TWeakObjectPtr<UMeleeCombatComponent> WeakThis(this); // 약한 참조 생성
		// SetTimer 람다를 사용할대 [this] 캡처는 잠재적인 NULL 포인터 오류가 발생할 수 있으므로 약한 참조 사용
        // 타이머가 동작하기 전에 객체가 파괴될 경우 오류 발생 방지

		// 입력 쿨타임 후에 콤보 공격 처리
        OwnerCharacter->GetWorldTimerManager().SetTimer(InputCooldownHandle, [WeakThis]()
        {
            // 타이머 실행 시점에 객체가 살아있는지 확인
            if (WeakThis.IsValid()) // 유효성 검사
            {
                if (WeakThis->bComboQueued && WeakThis->OwnerCharacter && !WeakThis->OwnerCharacter->GetCharacterMovement()->IsFalling()) 
                {
                    WeakThis->bComboQueued = false; // 큐 초기화
                    WeakThis->TriggerComboAttack(); // 콤보 공격 트리거
                }
                else if (WeakThis->bComboQueued) // 점프 중이면
                {
                    WeakThis->bComboQueued = false; // 큐 초기화
                }
            }
        }, 0.1f, false); // 짧은 딜레이 후 실행
    }
	// 콤보 리셋 타이머 시작
    if (!OwnerCharacter || !OwnerCharacter->GetWorld() || !OwnerCharacter->GetWorldTimerManager().IsTimerActive(TeleportCooldownHandle))
    {
		bCanTeleport = true; // 텔레포트 가능 상태로 설정
    }
}

// 입력 쿨타임 리셋 함수
void UMeleeCombatComponent::AdjustAttackDirection()
{
    UWorld* World = GetWorld();
    if (!World) return;
	if (!IsValid(OwnerCharacter)) return; // 소유 캐릭터 유효성 검사

	AActor* TargetEnemy = nullptr; // 찾은 타겟 적
	float ClosestDistance = MaxAutoAimDistance; // 가장 가까운 적과의 거리

    TArray<FHitResult> HitResults;
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn)); // 주변 Pawn 채널만 탐색하여 감지

    // 현재 위치 캐싱 (반복 호출 오버헤드 및 역참조 위험 방지)
    const FVector MyLocation = OwnerCharacter->GetActorLocation();

    // 주변 적 탐색 (SphereTrace)
    bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
        World, MyLocation, MyLocation, MaxAutoAimDistance,
        ObjectTypes, false, { OwnerCharacter }, EDrawDebugTrace::None, HitResults, true);

    if (bHit)
    {
        for (const FHitResult& Hit : HitResults)
        {
            AActor* Enemy = Hit.GetActor();
            if (!IsValid(Enemy)) continue;

            // 적 유형 및 상태 유효성 검사
            bool bIsTargetValid = false;
            if (AEnemy* GenericEnemy = Cast<AEnemy>(Enemy)) bIsTargetValid = (!GenericEnemy->bIsDead && !GenericEnemy->bIsInAirStun);
            else if (AEnemyDog* Dog = Cast<AEnemyDog>(Enemy)) bIsTargetValid = (!Dog->bIsDead && !Dog->bIsInAirStun);
            else if (AEnemyShooter* Shooter = Cast<AEnemyShooter>(Enemy)) bIsTargetValid = (!Shooter->bIsDead && !Shooter->bIsInAirStun);
            else if (AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(Enemy)) bIsTargetValid = !Guardian->bIsDead;
            else if (ABossEnemy* Boss = Cast<ABossEnemy>(Enemy)) bIsTargetValid = !Boss->IsDead();

            if (!bIsTargetValid) continue;

            const FVector EnemyLocation = Enemy->GetActorLocation();
            float Distance = FVector::Dist(MyLocation, EnemyLocation);

            if (Distance < ClosestDistance)
            {
                // 장애물 검사 (LineTrace)
                FHitResult ObstacleHit;
                FCollisionQueryParams Params;
                Params.AddIgnoredActor(OwnerCharacter);
                Params.AddIgnoredActor(Enemy);

                if (!World->LineTraceSingleByChannel(ObstacleHit, MyLocation, EnemyLocation, ECC_Visibility, Params))
                {
                    ClosestDistance = Distance;
                    TargetEnemy = Enemy;
                }
            }
        }
    }

    // 2차 방어: 최종 타겟과 캐릭터가 모두 유효한지 다시 확인 (분석 도구 경고 해결)
    if (IsValid(TargetEnemy) && IsValid(OwnerCharacter))
    {
        const FVector MyLoc = OwnerCharacter->GetActorLocation();
        const FVector TargetLoc = TargetEnemy->GetActorLocation();
        const float DistanceToEnemy = FVector::Dist(MyLoc, TargetLoc);

        // 공통 방향 계산 (Z축을 무시하는 GetSafeNormal2D 활용)
        const FVector DirectionToEnemy = (TargetLoc - MyLoc).GetSafeNormal2D();
        const FRotator NewRotation = DirectionToEnemy.Rotation();

        // 조건에 따른 분기 (텔레포트 vs 방향 보정)
        if (DistanceToEnemy >= MinTeleportDistance && ShouldTeleportToTarget(DistanceToEnemy))
        {
            // 텔레포트 수행
            TeleportToTarget(TargetEnemy);
        }
        else
        {
            // 텔레포트 거리가 안 되거나 쿨타임인 경우 단순 방향 보정
            OwnerCharacter->SetActorRotation(NewRotation);
            LastAttackDirection = DirectionToEnemy;
        }
    }
	//FVector StartLocation = OwnerCharacter->GetActorLocation(); // 시작 위치는 캐릭터의 위치

 //   // SphereTrace를 사용하여 주변의 적을 한 번에 찾기
 //   bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(World, StartLocation, StartLocation, MaxAutoAimDistance, 
 //       ObjectTypes, false, { OwnerCharacter }, EDrawDebugTrace::None, HitResults, true); // 디버그는 끔

	//if (bHit) // 히트 결과가 있으면
 //   {
	//	for (const FHitResult& Hit : HitResults) // 각 히트 결과 검사
 //       {
	//		AActor* Enemy = Hit.GetActor(); // 히트한 액터 가져옴
	//		if (!IsValid(Enemy)) continue; // 유효성 검사

 //           // 각 적 유형에 맞는 유효성 검사
	//		bool bIsTargetValid = false; // 타겟 유효성 초기화
 //           if (AEnemy* GenericEnemy = Cast<AEnemy>(Enemy)) bIsTargetValid = (!GenericEnemy->bIsDead && !GenericEnemy->bIsInAirStun); // Enemy 검사
	//		else if (AEnemyDog* Dog = Cast<AEnemyDog>(Enemy)) bIsTargetValid = (!Dog->bIsDead && !Dog->bIsInAirStun); // EnemyDog 검사
	//		else if (AEnemyShooter* Shooter = Cast<AEnemyShooter>(Enemy)) bIsTargetValid = (!Shooter->bIsDead && !Shooter->bIsInAirStun); // EnemyShooter 검사
	//		else if (AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(Enemy)) bIsTargetValid = !Guardian->bIsDead; // EnemyGuardian 검사
	//		else if (ABossEnemy* Boss = Cast<ABossEnemy>(Enemy)) bIsTargetValid = !Boss->bIsBossDead; // BossEnemy 검사
	//		if (!bIsTargetValid) continue; // 유효하지 않은 타겟이면 건너뜀

 //           // 거리 및 장애물 검사
	//		float Distance = FVector::Dist(StartLocation, Enemy->GetActorLocation()); // 적과의 거리 계산
	//		if (Distance < ClosestDistance) // 가장 가까운 거리보다 작으면
 //           {
 //               // 중간에 벽이 있는지 확인하는 레이캐스트
	//			FHitResult ObstacleHit; // 장애물 히트 결과
	//			FCollisionQueryParams Params; // 레이캐스트 파라미터 설정
	//			Params.AddIgnoredActor(OwnerCharacter); // 소유 캐릭터 무시
	//			Params.AddIgnoredActor(Enemy); // 적 무시
	//			// 레이캐스트 실행
 //               if (World->LineTraceSingleByChannel(ObstacleHit, StartLocation, Enemy->GetActorLocation(), ECC_Visibility, Params))
 //               {
	//				ClosestDistance = Distance; // 가장 가까운 거리 업데이트
	//				TargetEnemy = Enemy; // 타겟 적 설정
 //               }
 //           }
 //       }
 //   }

 //   // 최종 타겟 방향 보정 및 텔레포트
 //   if (IsValid(TargetEnemy) && IsValid(OwnerCharacter))
 //   {
	//	float DistanceToEnemy = FVector::Dist(OwnerCharacter->GetActorLocation(), TargetEnemy->GetActorLocation()); // 적과의 거리 계산
 //       
	//	if (DistanceToEnemy < MinTeleportDistance) // 최소 텔레포트 거리보다 가까우면
 //       {
 //           // 방향 보정만 수행
	//		FVector DirectionToEnemy = (TargetEnemy->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal(); // 적 방향 계산
	//		DirectionToEnemy.Z = 0.0f; // 수평면으로 제한
	//		FRotator NewRot = DirectionToEnemy.Rotation(); // 회전 계산
	//		OwnerCharacter->SetActorRotation(NewRot); // 회전 적용
	//		LastAttackDirection = DirectionToEnemy; // 마지막 공격 방향 저장
 //       }
	//	else // 최소 텔레포트 거리 이상이면
 //       {
	//		if (ShouldTeleportToTarget(DistanceToEnemy)) // 텔레포트 조건 체크
 //           {
	//			TeleportToTarget(TargetEnemy); // 텔레포트 수행
 //           } 
 //           else //조건이 안맞으면
 //           {
 //               // 방향 보정만 수행
	//			FVector DirectionToEnemy = (TargetEnemy->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal(); // 적 방향 계산
	//			DirectionToEnemy.Z = 0.0f; // 수평면으로 제한
	//			FRotator NewRot = DirectionToEnemy.Rotation(); // 회전 계산
	//			OwnerCharacter->SetActorRotation(NewRot); // 회전 적용
	//			LastAttackDirection = DirectionToEnemy; // 마지막 공격 방향 저장
 //           }
 //       }
 //   }
}

//void UMeleeCombatComponent::AdjustAttackDirection()
//{
//    if (!OwnerCharacter) return;
//
//    float MaxAutoAimDistance = 300.0f;
//    AActor* TargetEnemy = nullptr;
//    float ClosestDistance = MaxAutoAimDistance;
//
//    // 1. 모든 종류의 적을 하나의 배열로 통합
//    TArray<AActor*> AllEnemies;
//    TArray<AActor*> FoundActors;
//
//    // 각 적 유형별로 액터를 찾아 AllEnemies 배열에 추가
//    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), FoundActors);
//    AllEnemies.Append(FoundActors);
//
//    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDog::StaticClass(), FoundActors);
//    AllEnemies.Append(FoundActors);
//
//    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyShooter::StaticClass(), FoundActors);
//    AllEnemies.Append(FoundActors);
//
//    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyGuardian::StaticClass(), FoundActors);
//    AllEnemies.Append(FoundActors);
//
//    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABossEnemy::StaticClass(), FoundActors);
//    AllEnemies.Append(FoundActors);
//
//    FVector StartLocation = OwnerCharacter->GetActorLocation();
//
//    // 2. 통합된 모든 적들을 검사하여 가장 가까운 유효한 적 찾기
//    for (AActor* Enemy : AllEnemies)
//    {
//        if (!IsValid(Enemy)) continue;
//
//        // 3. 각 적 유형에 맞춰 상태(사망, 스턴 등)를 확인하여 유효한 타겟인지 검사
//        bool bIsTargetValid = true;
//        if (AEnemy* GenericEnemy = Cast<AEnemy>(Enemy))
//        {
//            if (GenericEnemy->bIsDead || GenericEnemy->bIsInAirStun)
//            {
//                bIsTargetValid = false;
//            }
//        }
//        else if (AEnemyDog* Dog = Cast<AEnemyDog>(Enemy))
//        {
//            if (Dog->bIsDead || Dog->bIsInAirStun)
//            {
//                bIsTargetValid = false;
//            }
//        }
//        else if (AEnemyShooter* Shooter = Cast<AEnemyShooter>(Enemy))
//        {
//            if (Shooter->bIsDead || Shooter->bIsInAirStun)
//            {
//                bIsTargetValid = false;
//            }
//        }
//        else if (AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(Enemy))
//        {
//            if (Guardian->bIsDead)
//            {
//                bIsTargetValid = false;
//            }
//        }
//        else if (ABossEnemy* Boss = Cast<ABossEnemy>(Enemy))
//        {
//            if (Boss->bIsBossDead)
//            {
//                bIsTargetValid = false;
//            }
//        }
//
//        if (!bIsTargetValid) continue; // 유효하지 않은 타겟이면 건너뜀
//
//        // 4. 거리 및 장애물 검사
//        FVector EnemyLocation = Enemy->GetActorLocation();
//        float Distance = FVector::Dist(StartLocation, EnemyLocation);
//
//        if (Distance < ClosestDistance)
//        {
//            // 레이캐스트로 직선상에 장애물이 있는지 체크
//            FHitResult HitResult;
//            FCollisionQueryParams Params;
//            Params.AddIgnoredActor(OwnerCharacter);
//            Params.AddIgnoredActor(Enemy);
//
//            FVector EndLocation = EnemyLocation;
//            EndLocation.Z = StartLocation.Z; // 같은 높이로 맞춤
//
//            bool bHit = GetWorld()->LineTraceSingleByChannel(
//                HitResult, StartLocation, EndLocation, ECC_Visibility, Params
//            );
//
//            // 장애물이 없거나 장애물이 적보다 멀리 있으면 타겟으로 설정
//            if (!bHit || FVector::Dist(StartLocation, HitResult.Location) > Distance * 0.9f)
//            {
//                ClosestDistance = Distance;
//                TargetEnemy = Enemy;
//            }
//        }
//    }
//
//    // 5. 최종 타겟이 정해지면 텔레포트 또는 방향 보정 실행
//    if (TargetEnemy)
//    {
//        float DistanceToEnemy = FVector::Dist(OwnerCharacter->GetActorLocation(), TargetEnemy->GetActorLocation());
//
//        if (DistanceToEnemy < MinTeleportDistance)
//        {
//            // 방향 보정만 수행
//            FVector DirectionToEnemy = (TargetEnemy->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
//            DirectionToEnemy.Z = 0.0f;
//            FRotator NewRot = DirectionToEnemy.Rotation();
//            OwnerCharacter->SetActorRotation(NewRot);
//            LastAttackDirection = DirectionToEnemy;
//        }
//        else
//        {
//            if (ShouldTeleportToTarget(DistanceToEnemy))
//            {
//                TeleportToTarget(TargetEnemy);
//            }
//            else
//            {
//                // 텔레포트 조건이 안 맞으면 기존 방향 보정만 수행
//                FVector DirectionToEnemy = (TargetEnemy->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
//                DirectionToEnemy.Z = 0.0f;
//                FRotator NewRot = DirectionToEnemy.Rotation();
//                OwnerCharacter->SetActorRotation(NewRot);
//                LastAttackDirection = DirectionToEnemy;
//            }
//        }
//    }
//}

// 텔레포트 조건 체크 함수
bool UMeleeCombatComponent::ShouldTeleportToTarget(float DistanceToTarget)
{
    // 순간이동 조건들 체크
    if (!OwnerCharacter || !bCanTeleport) return false;
    // 공중에 있으면 순간이동 안함
    if (OwnerCharacter->GetCharacterMovement()->IsFalling()) return false;
    // 최소 텔레포트 거리보다 가까우면 순간이동 안함
    if (DistanceToTarget < MinTeleportDistance) return false;
    // 최대 텔레포트 거리보다 멀면 순간이동 안함
    if (DistanceToTarget > TeleportDistance) return false;
	return true; // 모든 조건 통과 시 순간이동 허용
}

// 텔레포트 실행 함수
void UMeleeCombatComponent::TeleportToTarget(AActor* TargetEnemy)
{
    UWorld* World = GetWorld();
    if (!World) return;
	if (!IsValid(TargetEnemy) || !IsValid(OwnerCharacter)) return; // 유효성 검사
    bCanTeleport = false; // 텔레포트 쿨타임 시작

	TWeakObjectPtr<UMeleeCombatComponent> WeakThis(this); // 약한 참조 생성
	OwnerCharacter->GetWorldTimerManager().SetTimer(TeleportCooldownHandle, [WeakThis]() // 람다 함수로 타이머 설정
    {
       if (WeakThis.IsValid()) // 유효성 검사
       {
		   WeakThis->bCanTeleport = true; // 쿨타임 후 텔레포트 가능 상태로 설정
       }
    }, TeleportCooldownTime, false); // 한 번만 실행

    // 적의 위치와 방향 계산
	FVector EnemyLocation = TargetEnemy->GetActorLocation(); // 적 위치
	FVector EnemyForward = TargetEnemy->GetActorForwardVector(); // 적 전방 벡터

    // 적의 전방으로 순간이동 위치 계산
	FVector TeleportLocation = EnemyLocation + (EnemyForward * TeleportOffset); // 적 전방 오프셋 적용
    TeleportLocation.Z = OwnerCharacter->GetActorLocation().Z; // 높이는 유지

    // 바닥과의 충돌 체크해서 높이 조정
	FHitResult FloorHit; // 바닥 히트 결과
	FCollisionQueryParams FloorParams; // 바닥 충돌 파라미터
	FloorParams.AddIgnoredActor(OwnerCharacter); // 소유 캐릭터 무시
	FloorParams.AddIgnoredActor(TargetEnemy); // 타겟 적 무시

	FVector FloorStart = TeleportLocation + FVector(0, 0, FloorStartOffset); // 위에서 시작
	FVector FloorEnd = TeleportLocation - FVector(0, 0, FloorEndOffset); // 아래로 긴 거리

	// 바닥에 닿는 위치로 Z 조정
    if (GetWorld()->LineTraceSingleByChannel(FloorHit, FloorStart, FloorEnd, ECC_Visibility, FloorParams))
    {
        TeleportLocation.Z = FloorHit.Location.Z + FloorHeightOffset; // 캐릭터 발 위치 조정 
    }

	OwnerCharacter->SetActorLocation(TeleportLocation); // 위치 설정
	FVector DirectionToEnemy = EnemyLocation - TeleportLocation; // 적 방향 계산
	DirectionToEnemy.Z = 0.0f; // 수평면으로 제한
	DirectionToEnemy.Normalize(); // 정규화
	FRotator NewRot = FRotationMatrix::MakeFromX(DirectionToEnemy).Rotator(); // 회전 계산
	OwnerCharacter->SetActorRotation(NewRot); // 회전 적용
	LastAttackDirection = DirectionToEnemy; // 마지막 공격 방향 저장

    // 시각화 디버그
    //DrawDebugSphere(GetWorld(), TeleportLocation, 50.0f, 12, FColor::Green, false, 2.0f, 0, 2.0f);
    //DrawDebugLine(GetWorld(), OwnerCharacter->GetActorLocation(), EnemyLocation, FColor::Yellow, false, 2.0f, 0, 5.0f);

	AMainCharacter* MainChar = Cast<AMainCharacter>(OwnerCharacter); // 메인캐릭터 캐스팅

    // 디버그 로그 캐스팅 성공 여부 확인
    //UE_LOG(LogTemp, Warning, TEXT("Teleport FX Debug: MainChar Valid: %s"),
    //    (MainChar ? TEXT("TRUE") : TEXT("FALSE - Cast Failed!")));

	if (MainChar) // 메인캐릭터가 유효하면
    {
        // 디버그 로그
        UE_LOG(LogTemp, Warning, TEXT("Teleport FX Debug: Sound Asset VALID. Playing now."));
		if (class USoundBase* Sound = MainChar->GetTeleportSound()) // 텔레포트 사운드 가져옴
        {
            // 사운드 재생
			UGameplayStatics::PlaySoundAtLocation(
				GetWorld(), // 월드 컨텍스트
				Sound,  // 사운드 에셋
				OwnerCharacter->GetActorLocation(), // 위치
				OwnerCharacter->GetActorRotation() // 회전
            );
        }
		else // 사운드 에셋이 없으면
        {
            // 디버그 로그
            UE_LOG(LogTemp, Error, TEXT("Teleport FX Debug: Sound Asset is NULL in BP!"));
        }
        // 나이아가라 이펙트 재생
		if (class UNiagaraSystem* Niagara = MainChar->GetTeleportNiagaraEffect()) // 나이아가라 에셋 가져옴
        {
            // 디버그 로그
            UE_LOG(LogTemp, Warning, TEXT("Teleport FX Debug: Niagara Asset VALID. Playing now."));

            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), // 월드 컨텍스트
				Niagara, // 나이아가라 시스템
				OwnerCharacter->GetActorLocation(), // 위치
				OwnerCharacter->GetActorRotation(), // 회전
                EffectDefaultScale, // 스케일
				true, // bAutoDestroy 자동파괴 설정
				true //, bAutoActivate 자동활성화 설정
            );
        }
        else
        {
            // 에셋 포인터 누락 확인
            UE_LOG(LogTemp, Error, TEXT("Teleport FX Debug: Niagara Asset is NULL in BP!"));
        }
    }

	UE_LOG(LogTemp, Warning, TEXT("Teleported to target enemy - Combo Index: %d"), ComboIndex); // 디버그 로그
}

// 콤보 이동 적용 함수
void UMeleeCombatComponent::ApplyComboMovement(float MoveDistance, FVector MoveDirection)
{
	if (!OwnerCharacter || MoveDirection.IsNearlyZero()) return; // 유효성 검사
	MoveDirection.Z = 0; // 수평면으로 제한
	MoveDirection.Normalize(); // 정규화
	OwnerCharacter->LaunchCharacter(MoveDirection * MoveDistance, false, false); // 이동 적용
}

// 킥 공격 관련 함수들
void UMeleeCombatComponent::EnableKickHitBox()
{
	if (KickHitBox) // 킥 히트박스가 유효하면
    {
		KickHitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 충돌 활성화
    }
	KickRaycastAttack(); // 킥 레이캐스트 공격 실행
}

// 킥 히트박스 비활성화 함수
void UMeleeCombatComponent::DisableKickHitBox()
{
	if (KickHitBox) // 킥 히트박스가 유효하면
    {
		KickHitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 비활성화
    }

	KickRaycastHitActor = nullptr; // 레이캐스트 히트 액터 초기화
	KickRaycastHitResult.Reset(); // 레이캐스트 히트 결과 초기화
}

// 킥 레이캐스트 공격 함수
void UMeleeCombatComponent::KickRaycastAttack()
{
	if (!OwnerCharacter) return; // 유효성 검사

	FVector StartLocation = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * KickRaycastStartOffset; // 시작 위치 약간 앞쪽으로 오프셋
	FVector EndLocation = StartLocation + OwnerCharacter->GetActorForwardVector() * KickRaycastEndOffset; // 끝 위치

	FHitResult HitResult; // 히트 결과
	FCollisionQueryParams Params; // 레이캐스트 파라미터 
	Params.AddIgnoredActor(OwnerCharacter); // 소유 캐릭터 무시

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params); // 레이캐스트 실행
    if (bHit)
    {
        KickRaycastHitActor = HitResult.GetActor(); // 히트한 액터 저장
    }
    else
    {
        KickRaycastHitActor = nullptr; // 히트하지 않았다면 초기화
    }
	KickRaycastHitResult = HitResult; // 히트 결과 저장

    // 시각화 디버그
    /*DrawDebugLine(GetWorld(), StartLocation, EndLocation, bHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 3.0f)*/;
}

// 킥 히트박스 오버랩 처리 함수
void UMeleeCombatComponent::HandleKickOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (!OtherActor || OtherActor == OwnerCharacter || OtherComp == KickHitBox) return; // 유효성 검사
	if (OtherActor != KickRaycastHitActor) return; // 레이캐스트로 맞은 액터와 동일한지 확인

	FVector ImpactPoint = KickRaycastHitResult.ImpactPoint; // 레이캐스트 결과를 사용
	const FRotator ImpactRotation = KickRaycastHitResult.Normal.Rotation(); // 수직 벡터로 회전 사용

    UGameplayStatics::ApplyDamage(OtherActor, KickDamage, nullptr, OwnerCharacter, nullptr); // 데미지 적용

	AMainCharacter* MainChar = Cast<AMainCharacter>(OwnerCharacter); // 메인캐릭터 캐스팅
	if (IsValid(MainChar)) // 메인캐릭터가 유효하면
    {
		float Offset = MainChar->GetKickEffectOffset(); // 킥 이펙트 오프셋 값 가져오기
		if (Offset > 0.0f) // 오프셋 값이 양수이면
        {
            // 캐릭터의 정면 방향을 가져와 ImpactPoint에 더함
			FVector ForwardVector = OwnerCharacter->GetActorForwardVector(); // 캐릭터의 정면 방향 가져옴
			ImpactPoint += ForwardVector * Offset; // ImpactPoint에 더해서 오프셋 적용
        }
        // 킥 나이아가라 이펙트 재생
        if (class UNiagaraSystem* KickNiagaraEffect = MainChar->GetKickNiagaraEffect())
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World, // 월드 컨텍스트
				KickNiagaraEffect, // 킥 이펙트 시스템
				ImpactPoint, // 오프셋이 적용된 ImpactPoint 
				ImpactRotation, // 회전
                EffectDefaultScale,  // 스케일
				true, // bAutoDestroy
				true //, bAutoActivate
            );
        }
        // 킥 히트 사운드 재생
        if (class USoundBase* KickHitSound = MainChar->GetKickHitSound())
        {
            UGameplayStatics::PlaySoundAtLocation(
				World, // 월드 컨텍스트
				KickHitSound, // 킥 히트 사운드
                ImpactPoint // Offset이 적용된 ImpactPoint 사용
            );
        }
    }
    // 킥 넉백 
	ACharacter* HitCharacter = Cast<ACharacter>(OtherActor); // 히트한 액터를 캐릭터로 캐스팅
	if (IsValid(HitCharacter) && HitCharacter->GetCharacterMovement() && OwnerCharacter) // 소유 캐릭터가 유효하면
    {
		FVector LaunchDir = OwnerCharacter->GetActorForwardVector(); // 플레이어의 전방 벡터를 넉백 방향으로 설정
        LaunchDir.Z = 0; // 수평으로만 밀어냅니다.
		LaunchDir.Normalize(); // 정규화
		HitCharacter->LaunchCharacter(LaunchDir * KickKnockbackStrength, true, false); // 넉백 강도 만큼 넉백적용
    }
	DisableKickHitBox(); // 킥 히트박스 비활성화
}

// 입력 쿨타임 리셋 함수
void UMeleeCombatComponent::ResetInputCooldown()
{
	bInputBlocked = false; // 입력 차단 해제
    UE_LOG(LogTemp, Warning, TEXT("Input Cooldown Reset"));
}
// 모든 공격 큐 초기화 함수
void UMeleeCombatComponent::ClearAllQueues()
{
    UWorld* World = GetWorld();
    if (!World) return;

	bComboQueued = false; // 콤보 큐 초기화
    World->GetTimerManager().ClearTimer(InputCooldownHandle);
    UE_LOG(LogTemp, Warning, TEXT("All attack queues cleared"));
}
// 콤보 공격 큐 초기화 함수
void UMeleeCombatComponent::ClearComboAttackQueue()
{
	bComboQueued = false; // 콤보 큐 초기화
    UE_LOG(LogTemp, Warning, TEXT("Combo attack queue cleared"));
}
// 텔레포트 쿨타임 진행도 반환 함수
float UMeleeCombatComponent::GetTeleportCooldownPercent() const
{
	// 쿨타임이 없거나 소유 캐릭터가 없으면 0.0 반환
    if (!IsValid(OwnerCharacter) || !OwnerCharacter->GetWorld() || TeleportCooldownTime <= 0.0f)
    {
        return 0.0f;
    }
	FTimerManager& TimerManager = OwnerCharacter->GetWorldTimerManager(); // 타이머 매니저 가져옴
	if (TimerManager.IsTimerActive(TeleportCooldownHandle)) // 쿨타임이 진행 중이면
    {
		float RemainingTime = TimerManager.GetTimerRemaining(TeleportCooldownHandle); // 남은 시간 가져옴

        // 쿨타임 시작 직후에는 즉시 0.0을 반환 (느리게 차는 현상 방지)
        // 쿨타임 총 시간과 잔여 시간이 거의 같다면 (타이머가 막 시작되었다면) 오차값 KINDA_SMALL_NUMBER 로 0.0 반환
		if (RemainingTime >= TeleportCooldownTime - KINDA_SMALL_NUMBER)
        {
            return 1.0f; // 남은 시간이 총 시간과 같으면 1.0을 반환 (BP에서 1.0 - 1.0 = 0.0이 되게 함)
        }
        // 쿨타임이 진행 중인 경우 남은 시간 비율 (1.0 -> 0.0으로 감소)
        return FMath::Clamp(RemainingTime / TeleportCooldownTime, 0.0f, 1.0f);
    }
    // 쿨타임이 완료되었거나 시작되지 않았다면 0.0을 반환
    return 0.0f; // BP에서 1.0 - 0.0 = 1.0 꽉 참
}