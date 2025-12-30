#include "EnemyGuardian.h"
#include "EnemyGuardianAnimInstance.h" // 애님 인스턴스 클래스
#include "GameFramework/Actor.h" // AActor 클래스
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수
#include "EnemyGuardianShield.h" // 방패 클래스
#include "EnemyGuardianAIController.h" // AI 컨트롤러 클래스
#include "EnemyGuardianBaton.h" // 진압봉 클래스
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트
#include "MainGameModeBase.h" // 게임모드 참조 (적 사망 알림용)

AEnemyGuardian::AEnemyGuardian()
{
	PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화
	GetCharacterMovement()->MaxWalkSpeed = 300.0f; // 기본 이동 속도 설정

	// 회전 설정: AI 컨트롤러의 결정에 따라 캐릭터가 회전하도록 설정
	GetCharacterMovement()->bOrientRotationToMovement = false; // 이동 방향으로 자동 회전 비활성화
	GetCharacterMovement()->bUseControllerDesiredRotation = false; // 컨트롤러의 희망 회전값 사용 비활성화
	bUseControllerRotationYaw = true; // 컨트롤러의 Yaw 회전값을 캐릭터에 직접 적용
	AIControllerClass = AEnemyGuardianAIController::StaticClass(); // 사용할 AI 컨트롤러 지정
}

void AEnemyGuardian::BeginPlay()
{
	Super::BeginPlay(); // 부모 클래스 BeginPlay 호출

	// 최대 체력으로 현재 체력 초기화
	Health = MaxHealth;

	if (!GetController()) // AI 컨트롤러가 할당되지 않았다면
	{
		// 새 컨트롤러를 스폰하여 이 캐릭터에 빙의시킴
		AEnemyGuardianAIController* NewController = GetWorld()->SpawnActor<AEnemyGuardianAIController>();
		if (NewController)
		{
			NewController->Possess(this);
		}
	}

	// 방패 생성 및 장착
	if (ShieldClass)
	{
		EquippedShield = GetWorld()->SpawnActor<AEnemyGuardianShield>(ShieldClass); // 월드에 방패 액터 스폰
		if (EquippedShield)
		{
			EquippedShield->SetOwner(this); // 방패의 소유자를 이 캐릭터로 설정
			USkeletalMeshComponent* MeshComp = GetMesh();
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
		EquippedBaton = GetWorld()->SpawnActor<AEnemyGuardianBaton>(BatonClass); // 월드에 진압봉 액터 스폰
		if (EquippedBaton)
		{
			EquippedBaton->SetOwner(this); // 진압봉의 소유자를 이 캐릭터로 설정
			USkeletalMeshComponent* MeshComp = GetMesh();
			if (MeshComp)
			{
				// 캐릭터 메쉬의 특정 소켓에 진압봉을 부착
				FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
				EquippedBaton->AttachToComponent(
					MeshComp, AttachmentRules, FName("EnemyGuardianBatonSocket")
				);
			}
		}
	}

	SetUpAI(); // AI 초기 설정 함수 호출
	PlaySpawnIntroAnimation(); // 스폰 인트로 애니메이션 재생
}

void AEnemyGuardian::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemyGuardian::PlaySpawnIntroAnimation()
{
	UEnemyGuardianAnimInstance* AnimInstance = Cast<UEnemyGuardianAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance) return;

	bIsPlayingIntro = true; // 인트로 재생 중 상태로 전환
	bCanAttack = false; // 등장 중에는 공격 불가

	AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement(); // 등장 애니메이션 동안 이동 중지
	}

	PlayAnimMontage(SpawnIntroMontage); // 스폰 인트로 몽타주 재생

	// 몽타주가 끝나면 OnIntroMontageEnded 함수가 호출되도록 델리게이트 바인딩
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyGuardian::OnIntroMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, SpawnIntroMontage);
}

void AEnemyGuardian::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsPlayingIntro = false; // 인트로 재생 상태 해제
	bCanAttack = true; // 공격 가능 상태로 전환
}

void AEnemyGuardian::SetUpAI()
{
	if (GetController())
	{
		UE_LOG(LogTemp, Warning, TEXT("AI Controller is set up"));
	}
}

void AEnemyGuardian::PlayShieldAttackAnimation()
{
	// 공격이 불가능한 상태(인트로, 방패파괴, 다른공격중, 스턴, 사망)이면 함수 종료
	if (ShieldAttackMontages.Num() == 0 || bIsPlayingIntro || bIsShieldDestroyed || bIsShieldAttacking || bIsBatonAttacking || bIsStunned || bIsDead)
		return;

	int32 RandomIndex = FMath::RandRange(0, ShieldAttackMontages.Num() - 1); // 방패 공격 몽타주 중 하나를 랜덤 선택
	UAnimMontage* SelectedMontage = ShieldAttackMontages[RandomIndex];

	if (SelectedMontage)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			bIsShieldAttacking = true; // 방패 공격 중 상태로 전환

			AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
			if (AICon)
			{
				AICon->StopMovement(); // 공격 애니메이션 동안 이동 중지
			}

			AnimInstance->Montage_Play(SelectedMontage, 0.6f); // 선택된 몽타주를 0.7배속으로 재생
			//EquippedShield->StartShieldAttack(); // 방패의 공격 판정 활성화

			// 몽타주 종료 시 OnShieldAttackMontageEnded 호출하도록 델리게이트 바인딩
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AEnemyGuardian::OnShieldAttackMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
		}
	}
}

void AEnemyGuardian::OnShieldAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsShieldAttacking = false; // 방패 공격 중 상태 해제
	if (EquippedShield)
	{
		EquippedShield->EndShieldAttack(); // 방패의 공격 판정 비활성화
	}
}

void AEnemyGuardian::PlayNormalAttackAnimation()
{
	// 공격이 불가능한 상태(방패 미파괴, 다른공격중 등)이면 함수 종료
	if (NormalAttackMontages.Num() == 0 || bIsPlayingIntro || bIsBatonAttacking || !bIsShieldDestroyed || bIsShieldAttacking || bIsStunned || bIsDead)
		return;

	int32 RandomIndex = FMath::RandRange(0, NormalAttackMontages.Num() - 1); // 진압봉 공격 몽타주 중 하나를 랜덤 선택
	UAnimMontage* SelectedMontage = NormalAttackMontages[RandomIndex];

	if (SelectedMontage)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			bIsBatonAttacking = true; // 진압봉 공격 중 상태로 전환

			AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
			if (AICon)
			{
				AICon->StopMovement(); // 이동 중지
			}

			AnimInstance->Montage_Play(SelectedMontage, 1.0f); // 몽타주 재생
			if (EquippedBaton)
			{
				EquippedBaton->StartAttack(); // 진압봉의 공격 판정 활성화
			}

			// 몽타주 종료 시 OnNormalAttackMontageEnded 호출하도록 델리게이트 바인딩
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AEnemyGuardian::OnNormalAttackMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
		}
	}
}
void AEnemyGuardian::OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsBatonAttacking = false; // 진압봉 공격 중 상태 해제
	if (EquippedBaton)
	{
		EquippedBaton->EndAttack(); // 진압봉의 공격 판정 비활성화
	}
}

void AEnemyGuardian::PlayBlockAnimation()
{
	if (BlockMontage && !bIsPlayingIntro)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		// 이미 방어 애니메이션을 재생 중이 아니라면 재생
		if (AnimInstance && !AnimInstance->Montage_IsPlaying(BlockMontage))
		{
			AnimInstance->Montage_Play(BlockMontage);
		}
	}
}

void AEnemyGuardian::Stun()
{
	if (StunMontage && !bIsPlayingIntro)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && !AnimInstance->Montage_IsPlaying(StunMontage))
		{
			bIsStunned = true; // 스턴 상태로 전환
			AnimInstance->Montage_Play(StunMontage); // 스턴 몽타주 재생

			// 몽타주 종료 시 OnStunMontageEnded 호출하도록 델리게이트 바인딩
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AEnemyGuardian::OnStunMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, StunMontage);
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
	// 피격 몽타주가 끝났을 때만 처리합니다.
	if (Montage != HitMontage || bIsDead) return;

	// AI 컨트롤러를 가져와 이동을 재개합니다.
	AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (AICon)
	{
		// 플레이어를 추적하는 AI 로직을 재개하도록 명령합니다.
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (PlayerPawn)
		{
			AICon->MoveToActor(PlayerPawn, 5.0f); // 일반적인 추적 거리로 이동 재개
		}
	}
}

float AEnemyGuardian::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || bIsPlayingIntro) return 0.0f;

	AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());

	// 방패가 파괴되지 않았고 장착되어 있다면,
	if (!bIsShieldDestroyed && EquippedShield)
	{
		PlayBlockAnimation();
		EquippedShield->ShieldHealth -= DamageAmount;

		if (EquippedShield->ShieldHealth <= 0)
		{
			EquippedShield->SetActorHiddenInGame(true);
			bIsShieldDestroyed = true;
			Stun();
		}

		// Super::TakeDamage를 호출하지 *않습니다*.
		// 따라서 헬스바 컴포넌트가 이벤트를 받지 못해 갱신되지 않습니다.
		return 0.0f;
	}

	// 진압봉 공격 중일 때는 피격 애니메이션 없이 체력만 감소
	if (bIsBatonAttacking)
	{
		Health -= DamageAmount;
		if (Health <= 0) // 피해를 입고 체력이 0 이하가 되면
		{
			// 현재 공격을 즉시 중단
			bIsBatonAttacking = false;
			if (EquippedBaton)
			{
				EquippedBaton->EndAttack();
			}
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				AnimInstance->Montage_Stop(0.1f); // 재생중인 몽타주를 부드럽게 중지
			}
			Die(); // 사망 처리
		}
		// 본체가 맞았으므로 Super::TakeDamage 호출
		Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		return DamageAmount;
	}


	// 스턴 중일 때도 피격 애니메이션 없이 체력만 감소
	if (bIsStunned)
	{
		Health -= DamageAmount;
		if (Health <= 0)
		{
			Die();
		}
		// 본체가 맞았으므로 Super::TakeDamage 호출
		Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		return DamageAmount;
	}

	if (AICon) AICon->StopMovement();

	// 방패가 파괴된 후에는 가디언 본체가 직접 피해를 받음
	Health -= DamageAmount;
	if (HitMontage && Health > 0) // 체력이 남아있다면 피격 몽타주 재생
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && !AnimInstance->Montage_IsPlaying(HitMontage))
		{
			// 피격 몽타주 종료 후 AI 이동 재개를 위해 델리게이트 바인딩이 필요합니다.
			FOnMontageEnded EndDelegate; // <-- [추가] 델리게이트 생성
			EndDelegate.BindUObject(this, &AEnemyGuardian::OnHitMontageEnded); // <-- [추가] 바인딩
			AnimInstance->Montage_SetEndDelegate(EndDelegate, HitMontage); // <-- [추가] 델리게이트 설정

			AnimInstance->Montage_Play(HitMontage);
		}
	}

	if (Health <= 0) // 체력이 0 이하면 사망
	{
		Die();
	}
	// 본체가 맞았으므로 Super::TakeDamage 호출
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	return DamageAmount; // 실제 적용된 데미지 양 반환
}

float AEnemyGuardian::GetHealthPercent_Implementation() const
{
	// 방패가 파괴되기 전에는 헬스바가 떠도 100%로 보이도록 함 (갱신이 안되겠지만)
	if (!bIsShieldDestroyed)
	{
		return 1.0f;
	}

	// 방패 파괴 후
	if (bIsDead || Health <= 0.0f)
	{
		return 0.0f;
	}
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);
}

bool AEnemyGuardian::IsEnemyDead_Implementation() const
{
	return bIsDead;
}
void AEnemyGuardian::PlayWeaponHitSound()
{
	if (WeaponHitSound)
	{
		// 가디언의 위치에서 사운드 재생
		UGameplayStatics::PlaySoundAtLocation(this, WeaponHitSound, GetActorLocation());
	}
}

void AEnemyGuardian::PlayShieldHitSound()
{
	if (ShieldHitSound)
	{
		// 가디언의 위치에서 사운드 재생
		UGameplayStatics::PlaySoundAtLocation(this, ShieldHitSound, GetActorLocation());
	}
}

void AEnemyGuardian::Die()
{
	if (bIsDead) return; // 중복 사망 처리 방지
	bIsDead = true; // 사망 상태로 전환
	UEnemyGuardianAnimInstance* AnimInstance = Cast<UEnemyGuardianAnimInstance>(GetMesh()->GetAnimInstance());

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
		GetWorld()->GetTimerManager().SetTimer(
			DeathTimerHandle, this, &AEnemyGuardian::HideEnemy, HideTime, false
		);
	}
	else // 몽타주 재생에 실패했다면
	{
		HideEnemy(); // 즉시 정리
	}

	AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (AICon)
	{
		// AICon->StopAI(); // AI 로직 중단 (정리 함수에서 처리)
	}

	// 이동 관련 컴포넌트 비활성화
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	SetActorTickEnabled(false); // 성능을 위해 액터 틱 비활성화
}

void AEnemyGuardian::ApplyBaseWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = 250.0f;
	GetCharacterMovement()->MaxAcceleration = 5000.0f;
}

void AEnemyGuardian::HideEnemy()
{
	if (!bIsDead) return; // 사망 상태가 아니라면 실행 안 함

	// 게임모드에 적이 파괴되었음을 알림
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->OnEnemyDestroyed(this);
	}

	// 1. 이벤트 및 델리게이트 정리
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // 이 객체에 설정된 모든 타이머 해제

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && IsValid(AnimInstance))
	{
		// 델리게이트 바인딩 완전 해제 (메모리 누수 방지)
		AnimInstance->OnMontageEnded.RemoveAll(this);
		AnimInstance->OnMontageBlendingOut.RemoveAll(this);
		AnimInstance->OnMontageStarted.RemoveAll(this);
	}

	// 2. AI 시스템 완전 정리
	AEnemyGuardianAIController* AICon = Cast<AEnemyGuardianAIController>(GetController());
	if (AICon && IsValid(AICon))
	{
		// AICon->StopAI(); // AI 로직 중단
		AICon->UnPossess(); // 컨트롤러와 폰의 연결 해제
		AICon->Destroy(); // AI 컨트롤러 액터 자체를 파괴
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
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp && IsValid(MovementComp))
	{
		MovementComp->DisableMovement(); // 이동 비활성화
		MovementComp->StopMovementImmediately(); // 현재 이동 즉시 중단
		MovementComp->SetMovementMode(EMovementMode::MOVE_None); // 네비게이션 시스템에서 제외
		MovementComp->SetComponentTickEnabled(false); // 컴포넌트 틱 비활성화
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
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyGuardian>(this)]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
			{
				WeakThis->Destroy(); // 액터를 월드에서 완전히 제거
			}
		});
}

void AEnemyGuardian::StartAttack()
{
	// [수정] 방패 공격 중인 경우, 방패의 StartShieldAttack을 호출합니다.
	if (bIsShieldAttacking && EquippedShield)
	{
		EquippedShield->StartShieldAttack();
	}
	// 진압봉 공격 중인 경우
	else if (bIsBatonAttacking && EquippedBaton)
	{
		EquippedBaton->EnableAttackHitDetection(); // 진압봉 공격 판정 활성화 [cite: 103]
	}
}

void AEnemyGuardian::EndAttack()
{
	// [수정] 방패 공격 중인 경우, 방패의 StartShieldAttack을 호출합니다.
	if (bIsShieldAttacking && EquippedShield)
	{
		EquippedShield->EndShieldAttack();
	}
	// 진압봉 공격 중인 경우
	else if (bIsBatonAttacking && EquippedBaton)
	{
		EquippedBaton->DisableAttackHitDetection(); // 진압봉 공격 판정 활성화 [cite: 103]
	}
}
