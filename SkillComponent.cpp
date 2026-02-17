#include "SkillComponent.h"
#include "MainCharacter.h"
#include "Enemy.h"
#include "Skill3Projectile.h"
#include "MachineGun.h"
#include "Knife.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Cannon.h"
#include "Animation/AnimInstance.h"
#include "BossEnemy.h"
#include "EnemyDog.h"
#include "AimSkill3Projectile.h"
#include "EnemyShooter.h"
#include "Components/SkeletalMeshComponent.h"

USkillComponent::USkillComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Tick 사용안함
}

// 캐릭터 생성 직후 호출되어 스킬 시스템에 필요한 모든 엑터와 컴포넌트 참조 연결
// 컴포넌트 간 결합도를 낮추면서도 필요한 데이터에 즉시 접근할 수 있는 런타임 캐싱 
void USkillComponent::InitializeSkills(AMainCharacter* InCharacter, AMachineGun* InMachineGun, AKnife* InLeftKnife, AKnife* InRightKnife, UBoxComponent* InKickHitBox, ACannon* InCannon)
{
	if (!InCharacter) return;

	OwnerCharacter = InCharacter; // 캐릭터 참조 저장
	MachineGun = InMachineGun; // 머신건 참조 저장
	LeftKnife = InLeftKnife; // 왼쪽 칼 참조 저장
	RightKnife = InRightKnife; // 오른쪽 칼 참조 저장
	KickHitBox = InKickHitBox; // 킥 히트박스 참조 저장
	Cannon = InCannon; // 캐논 참조 저장

	// 메인 캐릭터 헤더에 정의된 애니메이션 몽타주 및 투사체 클래스 getter 함수들을 호출하여 참조 저장
	Skill1Montage = InCharacter->GetSkill1AnimMontage(); // 스킬1 애니메이션 몽타주 가져옴
	Skill2Montage = InCharacter->GetSkill2AnimMontage(); // 스킬2 애니메이션 몽타주 가져옴
	Skill3Montage = InCharacter->GetSkill3AnimMontage(); // 스킬3 애니메이션 몽타주 가져옴
	AimSkill1Montage = InCharacter->GetAimSkill1AnimMontage(); // 에임스킬1 애니메이션 몽타주 가져옴
	AimSkill2Montage = InCharacter->GetAimSkill2AnimMontage(); // 에임스킬2 애니메이션 몽타주 가져옴
	AimSkill2StartMontage = InCharacter->GetAimSkill2StartAnimMontage(); // 에임스킬2 시작 애니메이션 몽타주 가져옴
	Skill3ProjectileClass = InCharacter->GetSkill3ProjectileClass(); // 스킬3 투사체 클래스 가져옴
	AimSkill3Montage = InCharacter->GetAimSkill3AnimMontage(); // 에임스킬3 애니메이션 몽타주 가져옴
}

// 스킬 사용 시 캐릭터를 입력 방향으로 회전시키는 함수
void USkillComponent::RotateCharacterToInputDirection()
{
	if (!OwnerCharacter) return; // 소유한 캐릭터가 없으면 종료
	FVector Input = OwnerCharacter->GetCharacterMovement()->GetLastInputVector(); // 마지막 입력 벡터 가져옴
	if (!Input.IsNearlyZero()) // 입력 벡터가 거의 0이 아니면 즉 입력이 있으면
    {
		Input.Normalize(); // 벡터 길이를 1로 만들기 위해 정규화해서 순수 방향 데이터만 남김
		FRotator Rot = Input.Rotation(); // 방향 벡터를 회전값으로 변환
		Rot.Pitch = 0.f; // 피치값 0으로 설정하여 수평 회전만 적용
		OwnerCharacter->SetActorRotation(Rot); // 캐릭터 회전 적용
    }
}

// 스킬1
void USkillComponent::UseSkill1()
{
	UWorld* World = GetWorld();
	if (!World) return;

    // 스킬1 사용 불가 조건들 체크
	if (!OwnerCharacter || bIsUsingSkill1 || !bCanUseSkill1 || bIsUsingAimSkill1) return;
	if (OwnerCharacter->IsDashing() || OwnerCharacter->IsAiming() || OwnerCharacter->IsJumping() || OwnerCharacter->IsInDoubleJump() || bIsUsingAimSkill1) return;

	bIsUsingSkill1 = true; // 스킬1 사용 상태 설정
	bCanUseSkill1 = false; // 스킬1 사용 불가 상태 설정

	RotateCharacterToInputDirection(); // 캐릭터를 입력 방향으로 회전
	PlaySkill1Montage(); // 스킬1 몽타주 재생
	//DrawSkill1Range(); // 스킬1 범위 시각화 함수

	TWeakObjectPtr<USkillComponent> WeakThis(this);
	World->GetTimerManager().SetTimer(
		Skill1EffectHandle, [WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ApplySkill1Effect();
			}
		}, Skill1StunDelayTime, false);
}
//// 스킬1 효과 적용
//void USkillComponent::ApplySkill1Effect()
//{
//	FVector Center = OwnerCharacter->GetActorLocation(); // 스킬 중심 위치는 캐릭터의 위치
//	TArray<AActor*> Enemies; // 모든 Enemy 엑터 가져옴
//	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), Enemies); //Enemy 엑터 배열에 추가
//
//	for (AActor* Actor : Enemies) // 각 Enemy 엑터에 대해
//	{
//		AEnemy* Enemy = Cast<AEnemy>(Actor); // Dog 캐스팅
//		if (Enemy && Enemy->GetCharacterMovement()) // 유효성 검사
//		{
//			// 스킬 범위 내 액터만 선별
//			float Dist = FVector::Dist(Center, Enemy->GetActorLocation()); // 중심과 적 간 거리 계산
//			if (Dist <= Skill1Range) // 범위 내에 있으면
//			{
//				Enemy->EnterInAirStunState(5.0f); // 에어스턴 상태 진입
//			}
//		}
//	}
//	TArray<AActor*> Dogs;// 모든 Dog 엑터 가져오기
//	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDog::StaticClass(), Dogs); // Dog 엑터 배열에 추가
//	for (AActor* Actor : Dogs) // 각 Dog 엑터에 대해
//	{
//		AEnemyDog* Dog = Cast<AEnemyDog>(Actor); // Dog 캐스팅
//		if (Dog && Dog->GetCharacterMovement()) // 유효성 검사
//		{
//			float Dist = FVector::Dist(Center, Dog->GetActorLocation()); // 중심과 적 간 거리 계산
//			if (Dist <= Skill1Range) // 범위 내에 있으면
//			{
//				Dog->EnterInAirStunState(5.0f); // 에어스턴 상태 진입
//			}
//		}
//	}
//	TArray<AActor*> Shooters; // 모든 Shooter 엑터 가져오기
//	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyShooter::StaticClass(), Shooters); // Shooter 엑터 배열에 추가
//	for (AActor* Actor : Shooters) // 각 Shooter 엑터에 대해
//	{
//		AEnemyShooter* Shooter = Cast<AEnemyShooter>(Actor); // Shooter 캐스팅
//		if (Shooter && Shooter->GetCharacterMovement()) // 유효성 검사
//		{
//			float Dist = FVector::Dist(Center, Shooter->GetActorLocation()); // 중심과 적 간 거리 계산
//			if (Dist <= Skill1Range) // 범위 내에 있으면
//			{
//				Shooter->EnterInAirStunState(5.0f); // 에어스턴 상태 진입
//			}
//		}
//	}
//}

void USkillComponent::ApplySkill1Effect()
{
	if (!OwnerCharacter) return; // 유효성 검사

	FVector Center = OwnerCharacter->GetActorLocation(); // 스킬 중심 위치

	// 충돌 결과 및 필터링할 오브젝트 타입 설정
	TArray<FHitResult> HitResults; // 충돌 결과 배열
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes; // 검사할 오브젝트 타입 배열
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn)); // 오브젝트 타입에 pawn 추가

	// 구체 트레이스
	bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(),
		Center,           // 시작점
		Center,           // 시작점과 끝점을 같게 하여 구체 범위 검출
		Skill1Range,      // Skill1의 사거리
		ObjectTypes,
		false,
		{ OwnerCharacter }, // 본인은 제외
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	if (bHit) // 충돌이 발생했으면
	{
		// 중복 계산 방지를 위한 집합 
		TSet<AActor*> ProcessedActors; // 처리된 액터 집합

		for (const FHitResult& Hit : HitResults) // 각 충돌 결과에 대해
		{
			AActor* HitActor = Hit.GetActor(); // 충돌한 액터 가져오기

			if (HitActor && !ProcessedActors.Contains(HitActor)) // 유효성 검사 및 중복 체크
			{
				ProcessedActors.Add(HitActor); // 처리된 액터 집합에 추가

				if (AEnemy* Enemy = Cast<AEnemy>(HitActor)) // Enemy 캐스팅
				{
					Enemy->EnterInAirStunState(Skill1StunDuration); // 공중스턴 상태 진입
				}
				else if (AEnemyDog* Dog = Cast<AEnemyDog>(HitActor)) // Dog 캐스팅
				{
					Dog->EnterInAirStunState(Skill1StunDuration); // 공중스턴 상태 진입
				}
				else if (AEnemyShooter* Shooter = Cast<AEnemyShooter>(HitActor)) // Shooter 캐스팅
				{
					Shooter->EnterInAirStunState(Skill1StunDuration); // 공중스턴 상태 진입
				}
				// 가디언, 보스, 드론은 스턴 면역이므로 제외
			}
		}
	}
}

// 스킬1 몽타주 재생
void USkillComponent::PlaySkill1Montage()
{
	if (!Skill1Montage || !OwnerCharacter) return; // 유효성 검사
	UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
	if (!Anim) return; // 유효성 검사

	float Duration = Anim->Montage_Play(Skill1Montage, Skill1MontagePlayRate); // 몽타주 재생
	if (Duration > 0.0f) // 재생 성공 시
    {
		FOnMontageEnded End; // 몽타주 종료 델리게이트 설정
		End.BindUObject(this, &USkillComponent::ResetSkill1); // 종료 시 리셋 함수 바인딩
		Anim->Montage_SetEndDelegate(End, Skill1Montage); // 델리게이트 설정
    }
}
// 스킬1 범위 시각화 함수
//void USkillComponent::DrawSkill1Range()
//{
//    FVector Center = OwnerCharacter->GetActorLocation(); // 스킬 중심 위치
//    DrawDebugSphere(GetWorld(), Center, Skill1Range, 32, FColor::Red, false, 1.0f, 0, 2.0f); // 스킬 범위 표시
//}
// 스킬1 리셋
void USkillComponent::ResetSkill1(UAnimMontage* Montage, bool bInterrupted)
{
	bIsUsingSkill1 = false; // 스킬1 사용 상태 해제
	UWorld* World = GetWorld();
	if (!World) return;
	TWeakObjectPtr<USkillComponent> WeakThis(this);
	World->GetTimerManager().SetTimer(Skill1CooldownHandle, [WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ResetSkill1Cooldown();
			}
		}, Skill1Cooldown, false);
}
// 스킬1 쿨타임 리셋
void USkillComponent::ResetSkill1Cooldown()
{
	bCanUseSkill1 = true; // 스킬1 사용 가능 상태 설정
	if (OwnerCharacter && OwnerCharacter->GetSkill1ReadySound()) // 스킬1 준비 사운드 재생
    {
        // 사운드 재생
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), OwnerCharacter->GetSkill1ReadySound(), OwnerCharacter->GetActorLocation());
    }
}
// 스킬2
void USkillComponent::UseSkill2()
{
	UWorld* World = GetWorld();
	if (!World) return;
	// 스킬2 사용 불가 조건들 체크
	if (!OwnerCharacter || bIsUsingSkill2 || !bCanUseSkill2 || bIsUsingAimSkill2) return;
    if (OwnerCharacter->IsDashing() || OwnerCharacter->IsAiming() || OwnerCharacter->IsJumping() || OwnerCharacter->IsInDoubleJump() || bIsUsingAimSkill1) return;

	bIsUsingSkill2 = true; // 스킬2 사용 상태 설정
	bCanUseSkill2 = false; // 스킬2 사용 불가 상태 설정
	RotateCharacterToInputDirection(); // 캐릭터를 입력 방향으로 회전
	PlaySkill2Montage(); // 스킬2 몽타주 재생

	TWeakObjectPtr<USkillComponent> WeakThis(this);
	World->GetTimerManager().SetTimer(Skill2EffectHandle, [WeakThis]()
		{
			if (WeakThis.IsValid()) WeakThis->ApplySkill2Effect();
		}, Skill2EffectDelay, false);
}
// 스킬2 몽타주 재생
void USkillComponent::PlaySkill2Montage()
{
	if (!Skill2Montage || !OwnerCharacter) return; // 유효성 검사
	UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
	if (!Anim) return; // 유효성 검사

	float Duration = Anim->Montage_Play(Skill2Montage, Skill2MontagePlayRate); // 몽타주 재생
	if (Duration > 0.0f) // 재생 성공 시
    {
		FOnMontageEnded End; // 몽타주 종료 델리게이트 설정
		End.BindUObject(this, &USkillComponent::ResetSkill2); // 종료 시 리셋 함수 바인딩
		Anim->Montage_SetEndDelegate(End, Skill2Montage); // 델리게이트 설정
    }
}
// 스킬2 범위 시각화 함수
//void USkillComponent::DrawSkill2Range()
//{
//    if (!bIsUsingSkill2) return; // 스킬2 사용 중이 아니면 종료
//
//   /* UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld());*/ // 이전 디버그 라인 제거
//
//    FVector SkillCenter = OwnerCharacter->GetActorLocation(); // 스킬 중심 위치
//    float DebugDuration = 0.1f; // 디버그 라인 지속 시간
//    float DebugRadius = 200.0f; // 디버그 구 반지름
//
//    //DrawDebugSphere(GetWorld(), SkillCenter, DebugRadius, 32, FColor::Blue, false, DebugDuration, 0, 3.0f); // 스킬범위 표시
//
//    UE_LOG(LogTemp, Warning, TEXT("Skill2 Range Circle Drawn at %s with Radius %f"), *SkillCenter.ToString(), DebugRadius); // 로그 출력
//}

// 스킬2 효과 적용
void USkillComponent::ApplySkill2Effect()
{
	FVector Center = OwnerCharacter->GetActorLocation(); // 스킬 중심 위치는 캐릭터의 위치
	OwnerCharacter->LaunchCharacter(FVector(0, 0, Skill2MainCharacterLaunchDist), false, true); // 캐릭터를 위로 띄우기
	TArray<FHitResult> HitResults; // 충돌 결과 배열
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes; // 충돌 검사할 오브젝트 타입 배열
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn)); // 폰 오브젝트 타입 추가
    UKismetSystemLibrary::SphereTraceMultiForObjects(
        GetWorld(), Center, Center, Skill2Range, ObjectTypes,
        false, {}, EDrawDebugTrace::None, HitResults, true
	); // 구체를 사용하여 충돌 검사 실행

	TSet<AActor*> HitActors; // 중복 히트를 방지하기 위한 충돌한 액터 집합
	for (const FHitResult& Hit : HitResults) // 각 충돌 결과에 대해
    {
		AActor* HitActor = Hit.GetActor(); // 충돌한 액터 가져오기
		if (HitActor && HitActor != OwnerCharacter && !HitActors.Contains(HitActor)) // 유효성 검사 및 중복 제거
        {
			// 데미지 적용
            UGameplayStatics::ApplyDamage(HitActor, Skill2Damage, OwnerCharacter->GetController(), OwnerCharacter, UDamageType::StaticClass());
			// 충돌한 액터 집합에 추가
            HitActors.Add(HitActor);
        }
    }
	// 스킬2 범위 시각화
 /*   DrawDebugSphere(GetWorld(), Center, Skill2Range, 32, FColor::Red, false, 0.4f, 0, 3.0f);*/
    //GetWorld()->GetTimerManager().SetTimer(Skill2RangeClearHandle, this, &USkillComponent::ClearSkill2Range, 0.4f, false);
}
// 스킬2 범위 시각화 제거 함수
//void USkillComponent::ClearSkill2Range()
//{
//    UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld()); // 디버그 라인 제거
//}
// 스킬2 리셋
void USkillComponent::ResetSkill2(UAnimMontage* Montage, bool bInterrupted)
{
	UWorld* World = GetWorld();
	if (!World) return;
	bIsUsingSkill2 = false; // 스킬2 사용 상태 해제
	UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld()); // 디버그 라인 제거

	TWeakObjectPtr<USkillComponent> WeakThis(this);
	World->GetTimerManager().SetTimer(Skill2CooldownHandle, [WeakThis]()
		{
			if (WeakThis.IsValid()) WeakThis->ResetSkill2Cooldown();
		}, Skill2Cooldown, false);
}
// 스킬2 쿨타임 리셋
void USkillComponent::ResetSkill2Cooldown()
{
	bCanUseSkill2 = true; // 스킬2 사용 가능 상태 설정
	if (OwnerCharacter && OwnerCharacter->GetSkill2ReadySound()) // 스킬2 준비 사운드 재생
    {
		// 사운드 재생
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), OwnerCharacter->GetSkill2ReadySound(), OwnerCharacter->GetActorLocation());
    }
}
// 스킬3
void USkillComponent::UseSkill3()
{
	// 스킬3 사용 불가 조건들 체크
    if (!OwnerCharacter || bIsUsingSkill3 || !bCanUseSkill3) return;
    if (OwnerCharacter->IsDashing() || OwnerCharacter->IsAiming() || OwnerCharacter->IsJumping() || OwnerCharacter->IsInDoubleJump() || bIsUsingAimSkill1) return;
	bIsUsingSkill3 = true; // 스킬3 사용 상태 설정
	bCanUseSkill3 = false; // 스킬3 사용 불가 상태 설정
	RotateCharacterToInputDirection(); // 캐릭터를 입력 방향으로 회전
	FVector SpawnLoc = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * Skill3SpawnForwardOffset + FVector(0, 0, Skill3SpawnVerticalOffset); // 투사체 스폰 위치 계산
	FRotator SpawnRot = OwnerCharacter->GetActorRotation(); // 투사체 스폰 회전 계산
	if (Skill3ProjectileClass) // 투사체 클래스가 유효하면
    {
		ASkill3Projectile* Projectile = GetWorld()->SpawnActor<ASkill3Projectile>(Skill3ProjectileClass, SpawnLoc, SpawnRot); // 투사체 스폰
		if (Projectile) // 투사체가 유효하면
        {
			Projectile->SetDamage(Skill3Damage); // 데미지 설정
			Projectile->SetShooter(OwnerCharacter); // 발사자 설정
			Projectile->FireInDirection(OwnerCharacter->GetActorForwardVector()); // 발사 방향 설정
        }
    }
	// 스킬3 몽타주 재생
    PlaySkill3Montage();
}
// 스킬3 몽타주 재생
void USkillComponent::PlaySkill3Montage()
{
	// 유효성 검사
    if (!Skill3Montage || !OwnerCharacter) return;
	UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
	if (!Anim) return; // 유효성 검사
	float Duration = Anim->Montage_Play(Skill3Montage, Skill3MontagePlayRate); // 몽타주 재생
	if (Duration > 0.0f) // 재생 성공 시
    {
		FOnMontageEnded End; // 몽타주 종료 델리게이트 설정
		End.BindUObject(this, &USkillComponent::ResetSkill3); // 종료 시 리셋 함수 바인딩
		Anim->Montage_SetEndDelegate(End, Skill3Montage); // 델리게이트 설정
    }
}
// 스킬3 리셋
void USkillComponent::ResetSkill3(UAnimMontage* Montage, bool bInterrupted)
{
	bIsUsingSkill3 = false; // 스킬3 사용 상태 해제
	GetWorld()->GetTimerManager().SetTimer(Skill3CooldownHandle, this, &USkillComponent::ResetSkill3Cooldown, Skill3Cooldown, false); // 스킬3 쿨타임 타이머 설정
}
// 스킬3 쿨타임 리셋
void USkillComponent::ResetSkill3Cooldown()
{
	bCanUseSkill3 = true; // 스킬3 사용 가능 상태 설정
	if (OwnerCharacter && OwnerCharacter->GetSkill3ReadySound()) // 스킬3 준비 사운드 재생
    {
		// 사운드 재생
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), OwnerCharacter->GetSkill3ReadySound(), OwnerCharacter->GetActorLocation());
    }
}
// 에임스킬1
void USkillComponent::UseAimSkill1()
{
	UWorld* World = GetWorld();
	if (!World) return;
	// 에임스킬1 사용 불가 조건들 체크
    if (!OwnerCharacter || bIsUsingAimSkill1 || !bCanUseAimSkill1) return;
    if (OwnerCharacter->IsDashing() || OwnerCharacter->IsJumping() || OwnerCharacter->IsInDoubleJump()) return;

	bIsUsingAimSkill1 = true; // 에임스킬1 사용 상태 설정
	bCanUseAimSkill1 = false; // 에임스킬1 사용 불가 상태 설정

	if (OwnerCharacter->IsAiming()) // 이미 에임모드인 경우
    {
        OwnerCharacter->ExitAimMode();  // 기존 에임모드 강제 종료
    }

    // ExitAimMode가 숨긴 크로스헤어 위젯을 다시 표시
    if (OwnerCharacter)
    {
		OwnerCharacter->ShowCrosshairWidget(); // 크로스헤어 위젯 표시
    }
    OwnerCharacter->AttachRifleToBack(); // 라이플을 등에 장착
	OwnerCharacter->AttachKnifeToBack(); // 칼을 등에 장착
	RotateCharacterToInputDirection(); // 캐릭터를 입력 방향으로 회전
	if (MachineGun) // 머신건 장착 및 설정
    {
        MachineGun->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("AimSkill1Socket")); // 머신건을 소켓에 장착
		MachineGun->SetActorRelativeLocation(MachineGunLocation); // 위치 조정
		MachineGun->SetActorRelativeRotation(MachineGunRotation); // 회전 조정
		MachineGun->SetActorRelativeScale3D(MachineGunScale); // 스케일 조정
		MachineGun->SetActorHiddenInGame(false); // 머신건 숨김 해제 (기본적으로 머신건은 숨겨져있음)
		MachineGun->StartFire(); // 발사 시작
    }
	PlayAimSkill1Montage(); // 에임스킬1 몽타주 재생
	// 람다 적용
	TWeakObjectPtr<USkillComponent> WeakThis(this);
	World->GetTimerManager().SetTimer(AimSkill1RepeatHandle, [WeakThis]()
		{
			if (WeakThis.IsValid()) WeakThis->RepeatAimSkill1Montage();
		}, AimSkill1PlayInterval, true);
}
// 에임스킬1 몽타주 재생
void USkillComponent::PlayAimSkill1Montage()
{
	UWorld* World = GetWorld();
	if (!World) return;
	// 유효성 검사
    if (!AimSkill1Montage || !OwnerCharacter) return; 
	UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
	if (!Anim) return; // 유효성 검사
	float Rate = AimSkill1MontagePlayRate; // 재생 속도 설정
	float Duration = Anim->Montage_Play(AimSkill1Montage, Rate); // 몽타주 재생

	if (Duration > 0.0f) // 재생 성공 시
    {
		// 몽타주 설정을 통해 Start 섹션이 끝나면 Loop 섹션으로 전환되고 Loop 섹션은 계속 반복됨
		Anim->Montage_SetNextSection(FName("Start"), FName("Loop"), AimSkill1Montage); // 섹션 전환 설정
		Anim->Montage_SetNextSection(FName("Loop"), FName("Loop"), AimSkill1Montage); // 섹션 전환 설정
		AimSkill1MontageStartTime = GetWorld()->GetTimeSeconds(); // 몽타주 시작 시간 기록
		TWeakObjectPtr<USkillComponent> WeakThis(this);
		World->GetTimerManager().SetTimer(AimSkill1RepeatHandle, [WeakThis]()
			{
				if (WeakThis.IsValid()) WeakThis->ResetAimSkill1Timer();
			}, AimSkill1Duration, false);
    }
}
// 에임스킬1 몽타주 반복 재생
void USkillComponent::RepeatAimSkill1Montage()
{
	UWorld* World = GetWorld();
	if (!World) return;
	// 유효성 검사
    if (!bIsUsingAimSkill1 || !OwnerCharacter) return;
	float Elapsed = World->GetTimeSeconds() - AimSkill1MontageStartTime; // 경과 시간 계산
	if (Elapsed >= AimSkill1Duration) // 경과 시간이 설정된 지속 시간 이상이면
    {
		ResetAimSkill1(nullptr, false); // 리셋 호출
        return;
    }
	PlayAimSkill1Montage(); // 몽타주 재생
}
// 에임스킬1 리셋 타이머 콜백
void USkillComponent::ResetAimSkill1Timer()
{
	ResetAimSkill1(nullptr, false); // 리셋 호출
}
// 에임스킬1 리셋
void USkillComponent::ResetAimSkill1(UAnimMontage* Montage, bool bInterrupted)
{
	UWorld* World = GetWorld();
	if (!World) return;

	bIsUsingAimSkill1 = false; // 에임스킬1 사용 상태 해제
	World->GetTimerManager().ClearTimer(AimSkill1RepeatHandle); // 타이머 클리어

	if (IsValid(OwnerCharacter)) // 소유한 캐릭터가 유효하면
    { 
		OwnerCharacter->AttachRifleToBack(); // 라이플을 등에 장착
		OwnerCharacter->AttachKnifeToHand(); // 칼을 손에 장착
		OwnerCharacter->HideCrosshairWidget(); // 크로스헤어 위젯 숨기기

		USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh(); // 메쉬 컴포넌트 가져오기

		if (IsValid(MachineGun) && IsValid(CharacterMesh))
		{
			MachineGun->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("AimSkill1Socket")); // 머신건을 에임스킬1 소켓에 장착
			MachineGun->SetActorRelativeLocation(MachineGunLocation); // 위치 설정
			MachineGun->SetActorRelativeRotation(MachineGunRotation); // 회전 설정
			MachineGun->SetActorRelativeScale3D(MachineGunScale); // 스케일 설정
			MachineGun->SetActorHiddenInGame(true); // 머신건 숨기기
			MachineGun->StopFire(); // 발사 중지
		}

		if (UAnimInstance* Anim = CharacterMesh->GetAnimInstance())
		{
			if (AimSkill1Montage) // 몽타주가 유효하면
			{
				Anim->Montage_Stop(AimSkill1StopMontageTime, AimSkill1Montage); // 몽타주 중단
			}
		}
    }
	
	World->GetTimerManager().ClearTimer(AimSkill1RepeatHandle); // 반복 타이머 중지
	World->GetTimerManager().SetTimer(AimSkill1CooldownHandle, this, &USkillComponent::ResetAimSkill1Cooldown, AimSkill1Cooldown, false); // 쿨타임 타이머 설정
}
// 에임스킬1 쿨타임 리셋
void USkillComponent::ResetAimSkill1Cooldown()
{
	bCanUseAimSkill1 = true; // 에임스킬1 사용 가능 상태 설정
	if (OwnerCharacter && OwnerCharacter->GetAimSkill1ReadySound()) // 에임스킬1 준비 사운드 재생
    {
		// 사운드 재생
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), OwnerCharacter->GetAimSkill1ReadySound(), OwnerCharacter->GetActorLocation());
    }
}

// 대시로 인한 에임스킬1 중단
void USkillComponent::CancelAimSkill1ByDash()
{
    // 이 함수는 AMainCharacter::Dash()에 의해 호출되어 스킬을 중단시킵니다.
    // 쿨타임을 즉시 시작해야 합니다.
	bIsUsingAimSkill1 = false; // 에임스킬1 사용 상태 해제
	if (IsValid(OwnerCharacter)) // 소유한 캐릭터가 유효하면
	{
		OwnerCharacter->AttachRifleToBack(); // 라이플을 등에 장착
		OwnerCharacter->AttachKnifeToHand(); // 칼을 손에 장착
		OwnerCharacter->HideCrosshairWidget(); // 크로스헤어 위젯 숨기기

		USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh(); // 메쉬 컴포넌트 가져오기

		if (IsValid(MachineGun) && IsValid(CharacterMesh)) // 머신건 정리
		{
			MachineGun->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("AimSkill1Socket")); // 머신건을 에임스킬1 소켓에 장착
			MachineGun->SetActorRelativeLocation(MachineGunLocation); // 위치 설정
			MachineGun->SetActorRelativeRotation(MachineGunRotation); // 회전 설정
			MachineGun->SetActorRelativeScale3D(MachineGunScale); // 스케일 설정
			MachineGun->SetActorHiddenInGame(true); // 머신건 숨기기
			MachineGun->StopFire(); // 발사 중지
		}
		// 애니메이션 중단 로직
		if (UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance()) //애니메이션 인스턴스 가져오기
		{
			if (AimSkill1Montage) // 몽타주가 유효하면
			{
				Anim->Montage_Stop(AimSkill1StopMontageTime, AimSkill1Montage); // 몽타주 중단
			}
		}
	}

	if (UWorld* World = GetWorld()) // 월드가 유효하면
	{
		World->GetTimerManager().ClearTimer(AimSkill1RepeatHandle); // 반복 타이머 중지
		World->GetTimerManager().SetTimer(AimSkill1CooldownHandle, this, &USkillComponent::ResetAimSkill1Cooldown, AimSkill1Cooldown, false); // 쿨타임 타이머 설정
	}
}
// 에임스킬2
void USkillComponent::UseAimSkill2()
{
	// 에임스킬2 사용 불가 조건들 체크
	if (!OwnerCharacter || bIsUsingAimSkill2 || !bCanUseAimSkill2) return; // 유효성 검사
	if (OwnerCharacter->IsDashing() || OwnerCharacter->IsJumping() || OwnerCharacter->IsInDoubleJump()) return; // 점프 중일 때 사용 불가
	if (OwnerCharacter->IsAiming()) // 이미 에임모드인 경우
    {
        OwnerCharacter->ExitAimMode();  // 기존 에임모드 강제 종료
    }
	bIsUsingAimSkill2 = true; // 에임스킬2 사용 상태 설정
	bCanUseAimSkill2 = false; // 에임스킬2 사용 불가 상태 설정
    // ExitAimMode가 숨긴 크로스헤어 위젯을 다시 표시
    if (OwnerCharacter) 
    {
		OwnerCharacter->ShowCrosshairWidget(); // 크로스헤어 위젯 표시
    }
	OwnerCharacter->AttachRifleToBack(); // 라이플을 등에 장착
	OwnerCharacter->AttachKnifeToBack(); // 칼을 등에 장착
	RotateCharacterToInputDirection(); // 캐릭터를 입력 방향으로 회전
	if (Cannon) // 캐논 장착 및 설정
    {
		// 캐논을 에임스킬2 소켓에 장착
		Cannon->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("AimSkill2Socket")); // 캐논을 소켓에 장착
		Cannon->SetActorHiddenInGame(false); // 블루프린트에서 확인한 상대 위치 및 회전 값
		Cannon->SetShooter(OwnerCharacter); // 캐논의 발사자를 소유한 캐릭터로 설정
        //Cannon->FireProjectile();
    }
    
	PlayAimSkill2StartMontage(); // 에임스킬2 시작 몽타주 재생
}
// 에임스킬2 몽타주 재생
void USkillComponent::PlayAimSkill2Montage()
{
	// 유효성 검사
    if (!AimSkill2Montage || !OwnerCharacter) return; 
	UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
	if (!Anim) return; // 유효성 검사

	float Duration = Anim->Montage_Play(AimSkill2Montage, AimSkill2MontagePlayRate); // 몽타주 재생
	if (Duration > 0.0f) // 재생 성공 시
    {
		FOnMontageEnded End; // 몽타주 종료 델리게이트 설정
		End.BindUObject(this, &USkillComponent::ResetAimSkill2); // 종료 시 리셋 함수 바인딩
		Anim->Montage_SetEndDelegate(End, AimSkill2Montage); // 델리게이트 설정
		if (Cannon) // 캐논이 유효하면
        {
			Cannon->FireProjectile(); // 캐논 발사
        }
    }
}
// 에임스킬2 시작 몽타주 재생
void USkillComponent::PlayAimSkill2StartMontage()
{
	// 유효성 검사
    if (!AimSkill2StartMontage || !OwnerCharacter) 
	{
        PlayAimSkill2Montage(); // 시작 몽타주가 없으면 바로 스킬 몽타주 재생
        return;
    }
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh) return;

	UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
	if (!Anim) return; // 유효성 검사

	float Duration = Anim->Montage_Play(AimSkill2StartMontage, AimSkill2StartMontagePlayRate);
	if (Duration > 0.0f)
	{
		float JumpToTime = Duration * AimSkill2TransitionThreshold; // 몽타주 재생 시간의 임계값 퍼센트 지점으로 설정

		//  약참조 생성 
		TWeakObjectPtr<USkillComponent> WeakThis(this);

		// 월드 유효성 확인 후 타이머 설정
		UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().SetTimer(AimSkill2TransitionHandle,[WeakThis]()
			{
			  // 람다 실행 시점에 객체 생존 여부 확인 
			    if (WeakThis.IsValid())
				{
					WeakThis->PlayAimSkill2Montage(); // 에임스킬2 몽타주 재생
				}
			},
			JumpToTime, // 한 번만 실행
			false // 반복 안함
			);
		}
	}
	else
	{
		PlayAimSkill2Montage();
	}
}
// 에임스킬2 시작 몽타주 종료 콜백
void USkillComponent::OnAimSkill2StartMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	PlayAimSkill2Montage(); // 다음 몽타주 재생
}
// 에임스킬2 리셋
void USkillComponent::ResetAimSkill2(UAnimMontage* Montage, bool bInterrupted)
{
	bIsUsingAimSkill2 = false; // 에임스킬2 사용 상태 해제
	if (OwnerCharacter) // 소유한 캐릭터가 유효하면
    {
		OwnerCharacter->AttachRifleToBack(); // 라이플을 등에 장착
		OwnerCharacter->AttachKnifeToHand(); // 칼을 손에 장착
		OwnerCharacter->HideCrosshairWidget(); // 크로스헤어 위젯 숨기기

		// 역참조 방지를 위해 메시를 가져와서 확인 
		USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh();

		if (Cannon && CharacterMesh) // 캐릭터 메쉬와 캐논이 유효하면
		{
			Cannon->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("AimSkill2Socket")); // 캐논을 에임스킬2 소켓에 장착
			Cannon->SetActorHiddenInGame(true); // 캐논을 숨김
		}
    } 
	if (UWorld* World = GetWorld()) // 월드가 유효하면
	{
		World->GetTimerManager().SetTimer(AimSkill2CooldownHandle, this, &USkillComponent::ResetAimSkill2Cooldown, AimSkill2Cooldown, false); // 에임스킬2 쿨타임 타이머 설정
	}
}
// 에임스킬2 쿨타임 리셋
void USkillComponent::ResetAimSkill2Cooldown()
{
	bCanUseAimSkill2 = true; // 에임스킬2 사용 가능 상태 설정
	if (OwnerCharacter && OwnerCharacter->GetAimSkill2ReadySound()) // 에임스킬2 준비 사운드 재생
    {
		// 사운드 재생
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), OwnerCharacter->GetAimSkill2ReadySound(), OwnerCharacter->GetActorLocation());
    }
}
// 에임스킬3
void USkillComponent::UseAimSkill3()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 에임스킬3 사용 불가 조건들 체크
    if (!OwnerCharacter || bIsUsingAimSkill3 || !bCanUseAimSkill3 || !AimSkill3ProjectileClass) return;
	bIsUsingAimSkill3 = true; // 에임스킬3 사용 상태 설정
	bCanUseAimSkill3 = false; // 에임스킬3 사용 불가 상태 설정
	if (OwnerCharacter->IsAiming()) // 이미 에임모드인 경우
    {
        OwnerCharacter->ExitAimMode();  // 기존 에임모드 강제 종료
    }
    // 캐릭터 앞 AimSkill3Distance만큼 라인 트레이스
	FVector StartLoc = OwnerCharacter->GetActorLocation(); // 시작 위치
	FVector EndLoc = StartLoc + OwnerCharacter->GetActorForwardVector() * AimSkill3Distance; // 끝 위치
	FHitResult HitResult; // 히트 결과 저장용
	FVector TargetLocation = EndLoc; // 기본 타겟 위치 설정
	if (World->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility)) // 라인 트레이스 실행
    {
		TargetLocation = HitResult.ImpactPoint; // 충돌 지점으로 타겟 위치 설정
    }
	DrawAimSkill3Range(TargetLocation); // 에임스킬3 범위 시각화
	CachedAimSkill3Target = TargetLocation; // 타겟 위치 캐싱
    GetWorld()->GetTimerManager().SetTimer(
        AimSkill3DropTimeHandle,
        this,
        &USkillComponent::SpawnAimSkill3Projectiles,
        AimSkill3ProjectileDelay,
        false
	); // 한 번만 실행
	PlayAimSkill3Montage(); // 에임스킬3 몽타주 재생
}
// 에임스킬3 몽타주 재생
void USkillComponent::PlayAimSkill3Montage()
{
	// 유효성 검사
    if (!AimSkill3Montage || !OwnerCharacter) return;
	UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
	if (!Anim) return; // 유효성 검사
	float Duration = Anim->Montage_Play(AimSkill3Montage, AimSkill3MontagePlayRate);
	if (Duration > 0.0f) // 이 체크를 반드시 추가
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &USkillComponent::OnAimSkill3MontageEnded);
		Anim->Montage_SetEndDelegate(EndDelegate, AimSkill3Montage);
	}
	else {
		bIsUsingAimSkill3 = false; // 실패 시 상태 복구
	}
}
// 에임스킬3 몽타주 종료 콜백
void USkillComponent::OnAimSkill3MontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsUsingAimSkill3 = false; // 에임스킬3 사용 상태 해제
	ResetAimSkill3(Montage, bInterrupted);
	//GetWorld()->GetTimerManager().SetTimer(AimSkill3CooldownHandle, this, &USkillComponent::ResetAimSkill3Cooldown, AimSkill3Cooldown, false); // 쿨타임 타이머 설정
}
// 에임스킬3 범위 시각화
void USkillComponent::DrawAimSkill3Range(const FVector& TargetLocation)
{
	if (!bDrawDebugRange || !OwnerCharacter) return; // 유효성 검사
    AimSkill3DropPoints.Empty(); // 배열 초기화

	UWorld* World = GetWorld(); // 월드 가져오기
	if (!World) return; // 유효성 검사

	FVector Forward = OwnerCharacter->GetActorForwardVector(); // 캐릭터의 앞 방향 벡터
	FVector Start = OwnerCharacter->GetActorLocation(); // 캐릭터 위치

	float TotalLength = AimSkill3Distance; // 전체 길이
	int32 Num = NumProjectiles; // 투사체 수
	float Step = TotalLength / Num; // 각 투사체 간격
	for (int32 i = 0; i < Num; ++i) // 각 투사체 위치 계산
    {
        FVector GroundTarget = Start + Forward * (Step * (i + 1)); // 캐릭터 앞쪽부터 시작

        // 지면 높이 보정
		FHitResult Hit; // 히트 결과 저장용
		FVector TraceStart = GroundTarget + FVector(0, 0, AimSKill3TraceHeight); // 위에서 아래로 트레이스 시작 위치
		FVector TraceEnd = GroundTarget - FVector(0, 0, AimSkill3SpawnHeight); // 트레이스 끝 위치
		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility)) // 라인 트레이스 실행
        {
			GroundTarget.Z = AimSkill3GroundOffset; // 지면 높이로 조정
        }
        else
        {
			GroundTarget.Z = AimSkill3GroundOffset; // 지면이 없으면 기본 높이 설정
        }
		AimSkill3DropPoints.Add(GroundTarget); // 투사체 낙하 위치 저장
        // 하늘색 원 즉시 표시
        DrawDebugCircle(
			World, // 월드
			GroundTarget, // 원의 중심 위치
			AimSkill3Radius, // 반지름
			32, // 세그먼트 수
			FColor::Cyan, // 색상 하늘색
			false, // 지속시간 여부
            6.0f, // 지속시간
			0, // 깊이 우선순위
			10.0f, // 두께
			FVector(1, 0, 0), // 축 벡터
			FVector(0, 1, 0), // 두 번째 축 벡터
			false // 캡션
        );
    }
}
// 에임스킬3 투사체 스폰
void USkillComponent::SpawnAimSkill3Projectiles()
{
	// 유효성 검사
    if (!IsValid(OwnerCharacter) || !AimSkill3ProjectileClass) return;
	UWorld* World = GetWorld(); // 월드 가져오기
	if (!World) return; // 유효성 검사

    int32 Num = AimSkill3DropPoints.Num(); // 투사체수
	for (int32 i = 0; i < Num; ++i) // 각 투사체 스폰
    {
		FVector GroundTarget = AimSkill3DropPoints[i]; // 낙하 위치
		FVector SpawnLoc = GroundTarget + FVector(0, 0, AimSkill3SpawnHeight); // 스폰 위치 (낙하 위치 위)
		FRotator SpawnRot = FRotator(AimSkill3SpawnRotation, 0, 0); // 아래로 향하게 회전

		FActorSpawnParameters Params; // 스폰 파라미터 설정
		Params.Owner = OwnerCharacter; // 소유자 설정
		Params.Instigator = OwnerCharacter; // 인스티게이터 설정

        AAimSkill3Projectile* Proj = World->SpawnActor<AAimSkill3Projectile>( 
			AimSkill3ProjectileClass, SpawnLoc, SpawnRot, Params); // 투사체 스폰

		if (IsValid(Proj)) // 투사체가 유효하면
        {
			Proj->FireInDirection(FVector::DownVector); // 아래 방향으로 발사
        }
    }
}
// 에임스킬3 리셋
void USkillComponent::ResetAimSkill3(UAnimMontage* Montage, bool bInterrupted)
{
	bIsUsingAimSkill3 = false; // 에임스킬3 사용 상태 해제
	UWorld* World = GetWorld();
	if (!World) return;

	TWeakObjectPtr<USkillComponent> WeakThis(this);
	World->GetTimerManager().SetTimer(AimSkill3CooldownHandle, [WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ResetAimSkill3Cooldown();
			}
		}, AimSkill3Cooldown, false);
}
// 에임스킬3 쿨타임 리셋
void USkillComponent::ResetAimSkill3Cooldown()
{
	bCanUseAimSkill3 = true; // 에임스킬3 사용 가능 상태 설정
	if (OwnerCharacter && OwnerCharacter->GetAimSkill3ReadySound()) // 에임스킬3 준비 사운드 재생
    {
		// 사운드 재생
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), OwnerCharacter->GetAimSkill3ReadySound(), OwnerCharacter->GetActorLocation());
    }
}
// 모든 스킬 취소
void USkillComponent::CancelAllSkills()
{
	UWorld* World = GetWorld(); // 월드 가져옴
	if (!World) return; // 유효성 검사

	FTimerManager& TM = World->GetTimerManager();
	// 모든 타이머 핸들 명시적 정지
	TM.ClearTimer(Skill1EffectHandle);
	TM.ClearTimer(Skill1CooldownHandle);
	TM.ClearTimer(Skill2EffectHandle);
	TM.ClearTimer(Skill2CooldownHandle);
	TM.ClearTimer(Skill3CooldownHandle);
	TM.ClearTimer(AimSkill1RepeatHandle);
	TM.ClearTimer(AimSkill1CooldownHandle);
	TM.ClearTimer(AimSkill2TransitionHandle);
	TM.ClearTimer(AimSkill2CooldownHandle);
	TM.ClearTimer(AimSkill3DropTimeHandle);
	TM.ClearTimer(AimSkill3CooldownHandle);

    // 에임스킬1 취소 (머신건)
    if (bIsUsingAimSkill1) 
    {
		bIsUsingAimSkill1 = false; // 에임스킬1 사용 상태 해제
		if (IsValid(MachineGun)) // 머신건 정리
        {
            MachineGun->StopFire(); // 발사 중지
            MachineGun->SetActorHiddenInGame(true); // 오브젝트 숨김
        }
        //World->GetTimerManager().ClearAllTimersForObject(this); // 관련 타이머 제거
    }
    // 에임스킬2 취소 (캐논)
    if (bIsUsingAimSkill2)
    {
		bIsUsingAimSkill2 = false; // 에임스킬2 사용 상태 해제
		if (IsValid(Cannon)) // 캐논 정리
        {
            Cannon->SetActorHiddenInGame(true); // 오브젝트 숨김
        }
		//World->GetTimerManager().ClearAllTimersForObject(this); // 관련 타이머 제거
    }
    // 기타 스킬 상태 초기화
    bIsUsingSkill1 = bIsUsingSkill2 = bIsUsingSkill3 = false; 
    bIsUsingAimSkill3 = false;
}

// 스킬 사용 중인지 확인
bool USkillComponent::IsCastingSkill() const
{
	// 각 스킬 사용 상태 확인
    return bIsUsingSkill1 ||
        bIsUsingSkill2 ||
        bIsUsingSkill3 ||
        bIsUsingAimSkill1 ||
        bIsUsingAimSkill2 ||
        bIsUsingAimSkill3;
}
// 스킬 쿨타임 퍼센트 반환
float USkillComponent::GetSkill1CooldownPercent() const
{
	if (Skill1Cooldown <= 0.0f) // 분모가 0이거나 음수인지 먼저 확인
    {
		return 0.0f; // 쿨타임이 아니면 0
    }
	UWorld* World = GetWorld();
	if (World && World->GetTimerManager().IsTimerActive(Skill1CooldownHandle)) // 쿨타임이 진행 중인지 확인
    {
        // 0으로 나눌 걱정 없이 안전하게 계산
		return World->GetTimerManager().GetTimerRemaining(Skill1CooldownHandle) / Skill1Cooldown; // 남은 시간 / 전체 쿨타임
    }
    return 0.0f; // 쿨타임이 아니면 0
}
// 스킬 쿨타임 퍼센트 반환
float USkillComponent::GetSkill2CooldownPercent() const
{
	if (Skill2Cooldown <= 0.0f) // 분모가 0이거나 음수인지 먼저 확인
    {
		return 0.0f; // 쿨타임이 아니면 0
    }
	UWorld* World = GetWorld();
    if (World && World->GetTimerManager().IsTimerActive(Skill2CooldownHandle))
    {
        // 이제 0으로 나눌 걱정 없이 안전하게 계산
        return World->GetTimerManager().GetTimerRemaining(Skill2CooldownHandle) / Skill2Cooldown;
    }
    return 0.0f; // 쿨타임이 아니면 0
}
// 스킬 쿨타임 퍼센트 반환
float USkillComponent::GetSkill3CooldownPercent() const
{
    if (Skill3Cooldown <= 0.0f) // 분모가 0이거나 음수인지 먼저 확인
    {
		return 0.0f; // 쿨타임이 아니면 0
    }
	UWorld* World = GetWorld();
    // GetWorld()와 타이머 상태를 확인
	if (World && World->GetTimerManager().IsTimerActive(Skill3CooldownHandle))
    {
        // 이제 0으로 나눌 걱정 없이 안전하게 계산
        return World->GetTimerManager().GetTimerRemaining(Skill3CooldownHandle) / Skill3Cooldown;
    }
    return 0.0f; // 쿨타임이 아니면 0
}
// 에임스킬 쿨타임 퍼센트 반환
float USkillComponent::GetAimSkill1CooldownPercent() const
{
	// 분모가 0이거나 음수인지 먼저 확인합니다.
	if (AimSkill1Cooldown <= 0.0f)
	{
		return 0.0f; // 쿨타임이 아니면 0
	}
	UWorld* World = GetWorld();
	// GetWorld()와 타이머 상태를 확인
	if (World && World->GetTimerManager().IsTimerActive(AimSkill1CooldownHandle))
	{
		// 이제 0으로 나눌 걱정 없이 안전하게 계산
		return World->GetTimerManager().GetTimerRemaining(AimSkill1CooldownHandle) / AimSkill1Cooldown;
	}
	return 0.0f; // 쿨타임이 아니면 0
}
// 에임스킬 쿨타임 퍼센트 반환
float USkillComponent::GetAimSkill2CooldownPercent() const
{
	// 분모가 0이거나 음수인지 먼저 확인합니다.
	if (AimSkill2Cooldown <= 0.0f)
	{
		return 0.0f;
	}
	UWorld* World = GetWorld();
	// GetWorld()와 타이머 상태를 확인
	if (World && World->GetTimerManager().IsTimerActive(AimSkill2CooldownHandle))
	{
		// 이제 0으로 나눌 걱정 없이 안전하게 계산
		return World->GetTimerManager().GetTimerRemaining(AimSkill2CooldownHandle) / AimSkill2Cooldown;
	}
	return 0.0f; // 쿨타임이 아니면 0
}
// 에임스킬 쿨타임 퍼센트 반환
float USkillComponent::GetAimSkill3CooldownPercent() const
{
    // 분모가 0이거나 음수인지 먼저 확인합니다.
    if (AimSkill3Cooldown <= 0.0f)
    {
        return 0.0f;
    }
	UWorld* World = GetWorld();
    // GetWorld()와 타이머 상태를 확인
    if (World && World->GetTimerManager().IsTimerActive(AimSkill3CooldownHandle))
    {
        // 이제 0으로 나눌 걱정 없이 안전하게 계산
        return World->GetTimerManager().GetTimerRemaining(AimSkill3CooldownHandle) / AimSkill3Cooldown;
    }
    return 0.0f; // 쿨타임이 아니면 0
}