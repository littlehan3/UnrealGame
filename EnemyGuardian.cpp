#include "EnemyGuardian.h"
#include "EnemyGuardianAnimInstance.h" // 애님 인스턴스 클래스
#include "GameFramework/Actor.h" // AActor 클래스
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수
#include "EnemyGuardianShield.h" // 방패 클래스
#include "EnemyGuardianAIController.h" // AI 컨트롤러 클래스
#include "EnemyGuardianBaton.h" // 진압봉 클래스
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트
#include "MainGameModeBase.h" // 게임모드 참조 (적 사망 알림용)
#include "GameFramework/Character.h" // ACharacter 클래스 상속
#include "EnemyGuardianShield.h" // 장착할 방패 클래스 포함

AEnemyGuardian::AEnemyGuardian()
{
	PrimaryActorTick.bCanEverTick = false; // Tick 함수 비활성화

	AICon = nullptr;
	AnimInstance = nullptr;
	MoveComp = GetCharacterMovement();
	EquippedShield = nullptr;
	EquippedBaton = nullptr;

	ApplyBaseWalkSpeed(); // 기본 이동 속도 적용

	if (MoveComp)
	{
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->bUseControllerDesiredRotation = false;
	}

	bUseControllerRotationYaw = true; // 컨트롤러의 Yaw 회전값을 캐릭터에 직접 적용
	AIControllerClass = AEnemyGuardianAIController::StaticClass(); // 사용할 AI 컨트롤러 지정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; // 월드에 배치되거나 스폰될 때 자동으로 AI 소유
}

void AEnemyGuardian::BeginPlay()
{
	UWorld* World = GetWorld();
	if (!World) return;
	Super::BeginPlay(); // 부모 클래스 BeginPlay 호출

	SetCanBeDamaged(true); // 데미지를 받을 수 있음
	Health = MaxHealth; // 최대 체력으로 초기화

	AICon = Cast<AEnemyGuardianAIController>(GetController()); // AI 컨트롤러 가져옴
	if (!AICon) // AI 컨트롤러가 없다면
	{
		SpawnDefaultController(); // AI 컨트롤러 스폰
		AICon = Cast<AEnemyGuardianAIController>(GetController());  // AI 컨트롤러 다시 가져옴
	}

	USkeletalMeshComponent* MeshComp = GetMesh(); // 캐릭터 메쉬 컴포넌트 참조
	if (GetMesh())
	{
		AnimInstance = Cast<UEnemyGuardianAnimInstance>(MeshComp->GetAnimInstance()); // 애님 인스턴스 가져옴
	}

	MoveComp = GetCharacterMovement(); // 무브먼트 컴포넌트 참조

	// 방패 생성 및 장착
	if (ShieldClass)
	{
		EquippedShield = World->SpawnActor<AEnemyGuardianShield>(ShieldClass); // 월드에 방패 액터 스폰
		if (EquippedShield)
		{
			EquippedShield->SetOwner(this); // 방패의 소유자를 이 캐릭터로 설정
			if (MeshComp)
			{
				// 캐릭터 메쉬의 특정 소켓에 방패를 부착
				FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
				EquippedShield->AttachToComponent(MeshComp, AttachmentRules, FName("EnemyGuardianShieldSocket"));
			}
		}
	}

	// 진압봉 생성 및 장착
	if (BatonClass)
	{
		EquippedBaton = World->SpawnActor<AEnemyGuardianBaton>(BatonClass); // 월드에 진압봉 액터 스폰
		if (EquippedBaton)
		{
			EquippedBaton->SetOwner(this); // 진압봉의 소유자를 이 캐릭터로 설정
			if (MeshComp)
			{
				// 캐릭터 메쉬의 특정 소켓에 진압봉을 부착
				FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
				EquippedBaton->AttachToComponent(MeshComp, AttachmentRules, FName("EnemyGuardianBatonSocket"));
			}
		}
	}
	PlaySpawnIntroAnimation(); // 스폰 인트로 애니메이션 재생
}

void AEnemyGuardian::PlaySpawnIntroAnimation()
{
	//UEnemyGuardianAnimInstance* AnimInstance = Cast<UEnemyGuardianAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance) return;

	//AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (!AICon) return;

	bIsPlayingIntro = true; // 인트로 재생 중 상태로 전환
	bCanAttack = false; // 등장 중에는 공격 불가
	
	AICon->StopMovement(); // 등장 애니메이션 동안 이동 중지

	PlayAnimMontage(SpawnIntroMontage); // 스폰 인트로 몽타주 재생

	// 몽타주가 끝나면 OnIntroMontageEnded 함수가 호출되도록 델리게이트 바인딩
	FOnMontageEnded EndDelegate; // 델리게이트 생성
	EndDelegate.BindUObject(this, &AEnemyGuardian::OnIntroMontageEnded); // 델리게이트에 함수 바인딩
	AnimInstance->Montage_SetEndDelegate(EndDelegate, SpawnIntroMontage); // 델리게이트 설정
}

void AEnemyGuardian::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsPlayingIntro = false; // 인트로 재생 상태 해제
	bCanAttack = true; // 공격 가능 상태로 전환
}

void AEnemyGuardian::PlayShieldAttackAnimation()
{
	if (ShieldAttackMontages.Num() == 0 || bIsPlayingIntro || bIsShieldDestroyed || bIsShieldAttacking || bIsBatonAttacking || bIsStunned || bIsDead)
		return;
	//UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	//AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (!AICon) return;


	int32 RandomIndex = FMath::RandRange(0, ShieldAttackMontages.Num() - 1); // 방패 공격 몽타주 중 하나를 랜덤 선택
	UAnimMontage* SelectedMontage = ShieldAttackMontages[RandomIndex]; // 선택된 몽타주

	if (SelectedMontage) // 선택된 몽타주가 유효하다면
	{
		if (AnimInstance) // 애님 인스턴스가 유효하다면
		{
			bIsShieldAttacking = true; // 방패 공격 중 상태로 전환
			AICon->StopMovement(); // 공격 애니메이션 동안 이동 중지
			AnimInstance->Montage_Play(SelectedMontage, 0.6f); // 선택된 몽타주를 배속으로 재생

			// 몽타주 종료 시 OnShieldAttackMontageEnded 호출하도록 델리게이트 바인딩
			FOnMontageEnded EndDelegate; // 델리게이트 생성
			EndDelegate.BindUObject(this, &AEnemyGuardian::OnShieldAttackMontageEnded); // 델리게이트에 함수 바인딩
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage); // 델리게이트 설정
		}
	}
}

void AEnemyGuardian::OnShieldAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsShieldAttacking = false; // 방패 공격 중 상태 해제
	if (IsValid(EquippedShield))
	{
		EquippedShield->EndShieldAttack(); // 방패의 공격 판정 비활성화
	}
}

void AEnemyGuardian::PlayNormalAttackAnimation()
{
	// 공격이 불가능한 상태(방패 미파괴, 다른공격중 등)이면 함수 종료
	if (NormalAttackMontages.Num() == 0 || bIsPlayingIntro || bIsBatonAttacking || !bIsShieldDestroyed || bIsShieldAttacking || bIsStunned || bIsDead) 
		return;

	//UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;
	//AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (!AICon) return;


	int32 RandomIndex = FMath::RandRange(0, NormalAttackMontages.Num() - 1); // 진압봉 공격 몽타주 중 하나를 랜덤 선택
	UAnimMontage* SelectedMontage = NormalAttackMontages[RandomIndex];

	if (SelectedMontage)
	{
		if (AnimInstance)
		{
			bIsBatonAttacking = true; // 진압봉 공격 중 상태로 전환

			AICon->StopMovement(); // 이동 중지
		
			AnimInstance->Montage_Play(SelectedMontage, 1.0f); // 몽타주 재생
			if (IsValid(EquippedBaton))
			{
				EquippedBaton->StartAttack(); // 진압봉의 공격 판정 활성화
			}
			// 몽타주 종료 시 OnNormalAttackMontageEnded 호출하도록 델리게이트 바인딩
			FOnMontageEnded EndDelegate; // 델리게이트 생성
			EndDelegate.BindUObject(this, &AEnemyGuardian::OnNormalAttackMontageEnded); // 델리게이트에 함수 바인딩
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage); // 델리게이트 설정
		}
	}
}
void AEnemyGuardian::OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsBatonAttacking = false; // 진압봉 공격 중 상태 해제
	if (IsValid(EquippedBaton))
	{
		EquippedBaton->EndAttack(); // 진압봉의 공격 판정 비활성화
	}
}

void AEnemyGuardian::PlayBlockAnimation()
{
	//UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	if (BlockMontage && !bIsPlayingIntro)
	{
		// 이미 방어 애니메이션을 재생 중이 아니라면 재생
		if (!AnimInstance->Montage_IsPlaying(BlockMontage))
		{
			AnimInstance->Montage_Play(BlockMontage);
		}
	}
}

void AEnemyGuardian::Stun()
{
	//UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;
	if (StunMontage && !bIsPlayingIntro) // 스턴 애니메이션이 유효하고 인트로 중이 아닐 때
	{
		if (!AnimInstance->Montage_IsPlaying(StunMontage)) // 이미 스턴 몽타주를 재생 중이 아니라면
		{
			bIsStunned = true; // 스턴 상태로 전환
			AnimInstance->Montage_Play(StunMontage); // 스턴 몽타주 재생

			// 몽타주 종료 시 OnStunMontageEnded 호출하도록 델리게이트 바인딩
			FOnMontageEnded EndDelegate; // 델리게이트 생성
			EndDelegate.BindUObject(this, &AEnemyGuardian::OnStunMontageEnded); // 델리게이트에 함수 바인딩
			AnimInstance->Montage_SetEndDelegate(EndDelegate, StunMontage); // 델리게이트 설정
		}
	}
}

void AEnemyGuardian::OnStunMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == StunMontage)
	{
		bIsStunned = false; // 스턴 상태 해제
	}
}

void AEnemyGuardian::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != HitMontage || bIsDead) return;

	//AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (!AICon) return;
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;
	
	AICon->MoveToActor(PlayerPawn, 5.0f); // 일반적인 추적 거리로 이동 재개

}

float AEnemyGuardian::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || bIsPlayingIntro) return 0.0f;

	//AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (!AICon) return 0.0f;
	//UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return 0.0f;

	// 방패가 파괴되지 않았고 장착되어 있다면,
	if (!bIsShieldDestroyed && IsValid(EquippedShield))
	{
		PlayBlockAnimation(); // 방어 애니메이션 재생
		EquippedShield->ShieldHealth -= DamageAmount; // 방패 체력 감소

		if (EquippedShield->ShieldHealth <= 0) // 방패 체력이 0 이하가 되면
		{
			EquippedShield->SetActorHiddenInGame(true); // 방패 숨기기
			bIsShieldDestroyed = true; // 방패 파괴 상태로 전환
			Stun(); // 스턴 함수 호출
		} 

		return 0.0f; // 방패가 먼저 피해를 받으므로 본체는 데미지를 받지 않음
	}

	Health -= DamageAmount; // 본체 체력 감소

	// 진압봉 공격 중일 때는 피격 애니메이션 없이 체력만 감소
	if (bIsBatonAttacking)
	{
		if (Health <= 0) // 피해를 입고 체력이 0 이하가 되면
		{
			// 현재 공격을 즉시 중단
			bIsBatonAttacking = false; // 공격 상태 해제
			if (IsValid(EquippedBaton))
			{
				EquippedBaton->EndAttack(); // 진압봉의 공격 판정 비활성화
			}
			AnimInstance->Montage_Stop(0.1f); // 재생중인 몽타주를 부드럽게 중지
		
			Die(); // 사망 함수 호출
		}
	}

	// 스턴 중일 때도 피격 애니메이션 없이 체력만 감소
	else if (bIsStunned)
	{
		if (Health <= 0) // 피해를 입고 체력이 0 이하가 되면
		{
			Die(); // 사망 함수 호출
		}
	}
	else  // 일반 피격 처리
	{
		AICon->StopMovement(); // 피격 시 이동 중지

		if (HitMontage && Health > 0) // 체력이 남아있다면 피격 몽타주 재생
		{
			if (!AnimInstance->Montage_IsPlaying(HitMontage)) // 이미 피격 몽타주를 재생 중이 아니라면
			{
				// 피격 몽타주 종료 후 AI 이동 재개를 위해 델리게이트 바인딩
				FOnMontageEnded EndDelegate; // 델리게이트 생성
				EndDelegate.BindUObject(this, &AEnemyGuardian::OnHitMontageEnded); // 델리게이트에 함수 바인딩
				AnimInstance->Montage_SetEndDelegate(EndDelegate, HitMontage); // 델리게이트 설정
				AnimInstance->Montage_Play(HitMontage); // 피격 몽타주 재생
			}
		}
		else if (Health <= 0)
		{
			Die();
		}
	}

	// 본체가 맞았으므로 Super::TakeDamage 호출하고
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	return DamageAmount; // 실제 적용된 데미지 양 반환
}

float AEnemyGuardian::GetHealthPercent_Implementation() const
{
	// 방패가 파괴되기 전에는 헬스바가 100퍼센트를 유지한채로 보임
	if (!bIsShieldDestroyed)
	{
		return 1.0f;
	}
	// 방패 파괴 후
	if (bIsDead || Health <= 0.0f) // 사망 상태이거나 체력이 0 이하일 때
	{
		return 0.0f; // 체력 비율 0 반환
	}
	if (MaxHealth <= 0.0f) // 최대 체력이 0 이하일 때
	{
		return 0.0f; // 체력 비율 0 반환
	}
	return FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f); // 현재 체력 비율 반환
}

bool AEnemyGuardian::IsEnemyDead_Implementation() const // 사망 상태 반환
{
	return bIsDead; // 사망 상태 반환
}

void AEnemyGuardian::PlayWeaponHitSound()
{
	if (IsValid(WeaponHitSound))
	{
		// 가디언의 위치에서 사운드 재생
		UGameplayStatics::PlaySoundAtLocation(this, WeaponHitSound, GetActorLocation());
	}
}

void AEnemyGuardian::PlayShieldHitSound()
{
	if (IsValid(ShieldHitSound))
	{
		// 가디언의 위치에서 사운드 재생
		UGameplayStatics::PlaySoundAtLocation(this, ShieldHitSound, GetActorLocation());
	}
}

void AEnemyGuardian::Die()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (bIsDead) return; // 중복 사망 처리 방지
	//UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;
	//UEnemyGuardianAnimInstance* AnimInstance = Cast<UEnemyGuardianAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance) return;

	bIsDead = true; // 사망 상태로 전환

	if (!AnimInstance || DeadMontages.Num() == 0) // 애니메이션 재생이 불가능할 경우
	{
		HideEnemy(); // 즉시 정리 함수 호출
		return;
	}

	int32 RandIndex = FMath::RandRange(0, DeadMontages.Num() - 1); // 사망 몽타주 중 하나를 랜덤 선택
	UAnimMontage* SelectedMontage = DeadMontages[RandIndex];
	float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // 몽타주 재생

	if (PlayResult > 0.0f) // 몽타주 재생에 성공했다면
	{
		float MontageLength = SelectedMontage->GetPlayLength(); // 몽타주의 전체 길이
		float HidePercent = 0.9f; // 몽타주의 90% 지점에서 정리 시작
		float HideTime = MontageLength * HidePercent; // 실제 시간 계산

		// 계산된 시간 후에 HideEnemy 함수를 호출하도록 타이머 설정
		TWeakObjectPtr<AEnemyGuardian> WeakThis(this); // 약한 참조 생성
		World->GetTimerManager().SetTimer(
			DeathTimerHandle, 
			[WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HideEnemy();
				}
			}, HideTime, false
		);
	}
	else // 몽타주 재생에 실패했다면
	{
		HideEnemy(); // 즉시 정리
	}

	if (MoveComp)
	{
		// 이동 관련 컴포넌트 비활성화
		MoveComp->DisableMovement(); // 이동	비활성화
		MoveComp->StopMovementImmediately(); // 현재 이동 즉시 중단
		SetActorTickEnabled(false); // 성능을 위해 액터 틱 비활성화
	}
}

void AEnemyGuardian::ApplyBaseWalkSpeed()
{
	//UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->MaxWalkSpeed = 250.0f;
		MoveComp->MaxAcceleration = 5000.0f;
	}
}

void AEnemyGuardian::HideEnemy()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (!bIsDead) return; // 사망 상태가 아니라면 실행 안 함

	// 게임모드에 적이 파괴되었음을 알림
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(World->GetAuthGameMode()))
	{
		GameMode->OnEnemyDestroyed(this);
	}

	// 1. 이벤트 및 델리게이트 정리
	World->GetTimerManager().ClearAllTimersForObject(this); // 이 객체에 설정된 모든 타이머 해제

	UAnimInstance* GuardianAnimInstance = GetMesh()->GetAnimInstance();
	if (GuardianAnimInstance && IsValid(GuardianAnimInstance))
	{
		// 델리게이트 바인딩 완전 해제 (메모리 누수 방지)
		GuardianAnimInstance->OnMontageEnded.RemoveAll(this);
		GuardianAnimInstance->OnMontageBlendingOut.RemoveAll(this);
		GuardianAnimInstance->OnMontageStarted.RemoveAll(this);
	}

	// 2. AI 시스템 완전 정리
	AEnemyGuardianAIController* GuardianAICon = Cast<AEnemyGuardianAIController>(GetController());
	if (GuardianAICon && IsValid(GuardianAICon))
	{
		GuardianAICon->StopAI(); // AI 로직 중단
		GuardianAICon->UnPossess(); // 컨트롤러와 폰의 연결 해제
		GuardianAICon->Destroy(); // AI 컨트롤러 액터 자체를 파괴
	}

	// 3. 무기 시스템 정리
	if (EquippedShield && IsValid(EquippedShield))
	{
		EquippedShield->HideShield(); // 방패 숨김 및 정리 함수 호출
		EquippedShield = nullptr; // 방패 참조 해제
	}
	if (EquippedBaton && IsValid(EquippedBaton))
	{
		EquippedBaton->HideBaton(); // 진압봉 숨김 및 정리 함수 호출
		EquippedBaton = nullptr; // 진압봉 참조 해제
	}

	// 4. 무브먼트 시스템 정리
	UCharacterMovementComponent* GuardianMoveComp = GetCharacterMovement();
	if (GuardianMoveComp && IsValid(GuardianMoveComp))
	{
		GuardianMoveComp->DisableMovement(); // 이동 비활성화
		GuardianMoveComp->StopMovementImmediately(); // 현재 이동 즉시 중단
		GuardianMoveComp->SetMovementMode(EMovementMode::MOVE_None); // 네비게이션 시스템에서 제외
		GuardianMoveComp->SetComponentTickEnabled(false); // 컴포넌트 틱 비활성화
	}

	// 5. 메쉬 컴포넌트 정리
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp && IsValid(MeshComp))
	{
		MeshComp->SetVisibility(false); // 렌더링되지 않도록 가시성 비활성화
		MeshComp->SetHiddenInGame(true); // 게임 내에서 숨김 처리
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 검사 비활성화
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // 모든 채널에 대한 충돌 응답 무시
		MeshComp->SetComponentTickEnabled(false); // 컴포넌트 틱 비활성화
		MeshComp->SetAnimInstanceClass(nullptr); // 연결된 애님 인스턴스 참조 해제
		MeshComp->SetSkeletalMesh(nullptr); // 연결된 스켈레탈 메쉬 참조 해제
	}

	// 6. 액터 자체의 시스템 정리
	SetActorHiddenInGame(true); // 액터를 게임 내에서 숨김
	SetActorEnableCollision(false); // 액터의 충돌 비활성화
	SetActorTickEnabled(false); // 액터의 Tick 비활성화
	SetCanBeDamaged(false); // 데미지를 받을 수 없도록 설정

	// 7. 다음 프레임에 안전하게 액터 제거 (크래쉬 방지)
	TWeakObjectPtr<AEnemyGuardian> WeakThis(this);
	World->GetTimerManager().SetTimerForNextTick(
		[WeakThis]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
			{
				WeakThis->Destroy(); // 액터를 월드에서 완전히 제거
			}
		}
	);
}

void AEnemyGuardian::StartAttack()
{
	if (!IsValid(EquippedShield) && !IsValid(EquippedBaton)) return;

	if (bIsShieldAttacking && EquippedShield) // 방패 공격 중인 경우
	{ 
		EquippedShield->StartShieldAttack(); // 방패 공격 판정 활성화
	}
	// 진압봉 공격 중인 경우
	else if (bIsBatonAttacking && EquippedBaton)
	{
		EquippedBaton->EnableAttackHitDetection(); // 진압봉 공격 판정 활성화
	}
}

void AEnemyGuardian::EndAttack()
{
	if (!IsValid(EquippedShield) && !IsValid(EquippedBaton)) return;

	if (bIsShieldAttacking && EquippedShield) // 방패 공격 중인 경우
	{
		EquippedShield->EndShieldAttack(); // 방패 공격 판정 비활성화
	}
	// 진압봉 공격 중인 경우
	else if (bIsBatonAttacking && EquippedBaton)
	{
		EquippedBaton->DisableAttackHitDetection(); // 진압봉 공격 판정 비활성화 
	}
}
