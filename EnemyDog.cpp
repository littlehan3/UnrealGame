#include "EnemyDog.h"
#include "EnemyDogAnimInstance.h"
#include "EnemyDogAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/OverlapResult.h" // FOverlapResult 구조체 사용

AEnemyDog::AEnemyDog()
{
	PrimaryActorTick.bCanEverTick = true;
	bCanAttack = true;
	bIsAttacking = false;

	AIControllerClass = AEnemyDogAIController::StaticClass(); // AI 컨트롤러 설정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; //스폰 시에도 AI 컨트롤러 자동 할당
	GetMesh()->SetAnimInstanceClass(UEnemyDogAnimInstance::StaticClass()); // 애님 인스턴스 설정으로 보장

	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // 기본 이동속도 세팅
}

void AEnemyDog::BeginPlay()
{
	Super::BeginPlay();
	SetCanBeDamaged(true); // 피해를 받을 수 있는지 여부 true
	SetActorTickInterval(0.05f); // Tick 빈도 제한 20fps
	
	if (!GetController()) // AI 컨트롤러가 없다면
	{
		SpawnDefaultController(); // 스폰하여 할당
		UE_LOG(LogTemp, Warning, TEXT("EnemyDog AI Controller manually spawned"));
	}

	AAIController* AICon = Cast<AAIController>(GetController()); // AI컨트롤러를 가져옴
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyDog AI Controller Assigend %s"), *AICon->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyDog AI Controller is Null!"));
	}

	SetUpAI(); // AI 설정함수 호출
	PlaySpawnIntroAnimation(); // 스폰 애니메이션 재생
}

void AEnemyDog::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	GetWorld()->GetTimerManager().SetTimerForNextTick([this]() // 다음 Tick에
		{
			AAIController* AICon = Cast<AAIController>(GetController()); // AI 컨트롤러 할당
			if (AICon)
			{
				UE_LOG(LogTemp, Warning, TEXT("AEnemyDog AIController Assigned Later: %s"), *AICon->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("AEnemyDog AIController STILL NULL!"));
			}
		});
}

void AEnemyDog::PlaySpawnIntroAnimation()
{
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyDog AnimInstance not found"));
		return;
	}

	bIsPlayingIntro = true; // 인트로 재생중 여부 true
	bCanAttack = false; // 등장 중에는 공격 불가

	// AI 이동 중지
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement();
	}

	PlayAnimMontage(SpawnIntroMontage);

	// 몽타주 종료 델리게이트 바인딩
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyDog::OnIntroMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, SpawnIntroMontage);
}

void AEnemyDog::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Dog Intro Montage Ended"));
	bIsPlayingIntro = false;
	bCanAttack = true;

}

void AEnemyDog::ApplyBaseWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // 이동 속도 600
	GetCharacterMovement()->MaxAcceleration = 5000.0f; // 즉시 최대속도 도달
	GetCharacterMovement()->BrakingFrictionFactor = 10.0f;
}

void AEnemyDog::SetUpAI()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking); // 네브워킹 모드로 설정
}

void AEnemyDog::PlayNormalAttackAnimation()
{
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // 애님인스턴스를 불러옴
	if (!AnimInstance || NormalAttackMontages.Num() == 0) return; // 애님인스턴스가 없거나 몽타주배열이 비었다면 리턴
	if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return; // 애니메이션 재생중에는 공격 재생 금지

	int32 RandomIndex = FMath::RandRange(0, NormalAttackMontages.Num() - 1); // 공격 몽타주 랜덤재생 범위
	UAnimMontage* SelectedMontage = NormalAttackMontages[RandomIndex]; // 해당 범위 안에서 몽타주를 선택

	if (SelectedMontage)
	{
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.5f); // 선택된 몽타주를 재생

		// 몽타주 종료 델리게이트 연결
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyDog::OnAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // 공격 실행 후
	if (AICon)
	{
		AICon->StopMovement(); // 이동 중지
	}
	
	bCanAttack = false;
	bIsAttacking = true;
}

void AEnemyDog::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bCanAttack = true;
	bIsAttacking = false;
	bHasExecutedRaycast = false;
}

float AEnemyDog::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || bIsPlayingIntro) return 0.0f; // 이미 사망했거나 인트로 재생중엔 피해를 받지않음

	float DamageApplied = FMath::Min(Health, DamageAmount); // 데미지적용값: 현재 체력과 받은 데미지 중 더 작은값
	Health -= DamageApplied; // 데미지 적용값만큼 체력에서 차감
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog took %f damage, Health remaining: %f"), DamageAmount, Health);

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // 애님인스턴스를 불러옴
	if (!AnimInstance || HitMontages.Num() == 0) return 0.0f; // 애님인스턴스가 없거나 몽타주배열이 비었다면 리턴
	int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1); // 히트 몽타주 랜덤재생 범위
	UAnimMontage* SelectedMontage = HitMontages[RandomIndex]; // 해당 범위 안에서 몽타주를 선택

	if (SelectedMontage)
	{
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // 선택된 몽타주를 재생

		// 히트 애니메이션 종료 후 스턴 애니메이션 재생
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyDog::OnHitMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // 히트 애니메이션 실행 후
	if (AICon)
	{
		AICon->StopMovement(); // 이동 중지
	}

	if (Health <= 0.0f) // 체력이 0 이하일 경우 사망
	{
		Die();
	}

	return DamageApplied;
}

void AEnemyDog::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bIsDead) return;

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (bIsInAirStun)
	{
		if (AnimInstance && InAirStunMontage)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit animation ended while airborne. Resuming InAirStunMontage."));
			// 스턴 몽타주 재생 전에 히트 몽타주 타이머가 반복하지 못하게 막기 위해 타이머 정지
			GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);

			AnimInstance->Montage_Play(InAirStunMontage, 1.0f);

			// 스턴 몽타주 반복 타이머 재개
			GetWorld()->GetTimerManager().SetTimer(
				StunAnimRepeatTimerHandle,
				this,
				&AEnemyDog::PlayStunMontageLoop,
				0.4f,
				true
			);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit animation ended on ground. No need to resume InAirStunMontage."));
	}
}

void AEnemyDog::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	// 타이머
	GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

	// 몽타주 재생(지상/공중 구분)
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if ((bIsInAirStun || GetCharacterMovement()->IsFalling()) && InAirStunDeathMontage)
	{
		AnimInstance->Montage_Play(InAirStunDeathMontage, 1.0f);
	}
	else if (DeadMontages.Num() > 0)
	{
		int32 Rand = FMath::RandRange(0, DeadMontages.Num() - 1);
		AnimInstance->Montage_Play(DeadMontages[Rand], 1.0f);
	}

	// 사망 호출 시점 기준 고정 시간 후 폭발/제거
	constexpr float FixedExplosionDelay = 0.5f;
	GetWorld()->GetTimerManager().SetTimer(
		DeathTimerHandle,
		[this]()
		{
			Explode();
			HideEnemy();
		},
		FixedExplosionDelay,
		false
	);

	// AI 컨트롤러 중지
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		AICon->StopAI();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AIController is NULL! Can not stop AI."));
	}

	// 이동 비활성화
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	SetActorTickEnabled(false); // AI Tick 중지

	UE_LOG(LogTemp, Warning, TEXT("Die() called: Setting HideEnemy timer"));
}

void AEnemyDog::OnDeadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == InAirStunDeathMontage) // 인에어 사망이면
	{
		UE_LOG(LogTemp, Warning, TEXT("InAir death montage ended, triggering explosion"));
		Explode();
		HideEnemy();
	}
	else
	{
		Explode();
		HideEnemy();
	}
}

void AEnemyDog::Explode()
{
	if (bIsDead == false) return; // 사망하지 않은 상태에서 폭발 방지

	// 폭발 반경 내 플레이어만 피해 적용
	FVector ExplosionCenter = GetActorLocation();
	float ExplosionRadiusTemp = ExplosionRadius; // 변수 그대로 사용

	// Pawn 채널로 플레이어 감지
	TArray<FOverlapResult> Overlaps;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ExplosionRadiusTemp);

	bool bHasOverlaps = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		ExplosionCenter,
		FQuat::Identity,
		ECC_Pawn,
		CollisionShape
	);

	if (bHasOverlaps)
	{
		ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		for (auto& Overlap : Overlaps)
		{
			if (Overlap.GetActor() == PlayerCharacter)
			{
				UGameplayStatics::ApplyDamage(
					PlayerCharacter,
					ExplosionDamage,
					GetInstigator() ? GetInstigator()->GetController() : nullptr,
					this,
					nullptr
				);
				break; // 플레이어 1명만 처리
			}
		}
	}

	// 폭발 범위 시각화
	DrawDebugSphere(GetWorld(), ExplosionCenter, ExplosionRadius, 32, FColor::Red, false, 1.0f, 0, 2.0f);

	// 폭발 이펙트 생성
	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ExplosionEffect,
			GetActorLocation(),
			GetActorRotation(),
			FVector(1.0f),
			true,
			true
		);
	}

	// 폭발 사운드 재생
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}
}

void AEnemyDog::HideEnemy()
{
	if (!bIsDead) return; // 사망하지 않았으면 리턴

	UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyDog - Memory Cleanup"));

	//// GameMode에 Enemy 파괴 알림
	//if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	//{
	//	GameMode->OnEnemyDestroyed(this);
	//}

	// 1. 이벤트 및 델리게이트 정리 (최우선)
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 해제

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 애님 인스턴스 참조 받아옴
	if (AnimInstance && IsValid(AnimInstance)) // 애님 인스턴스 유효성 검사
	{
		// 애니메이션 이벤트 바인딩 완전 해제
		AnimInstance->OnMontageEnded.RemoveAll(this); // 몽타주 종료 이벤트 바인딩 해제
		AnimInstance->OnMontageBlendingOut.RemoveAll(this); // 몽타주 블랜드 아웃 이벤트 바인딩 해체
		AnimInstance->OnMontageStarted.RemoveAll(this); // 몽타주 시작 이벤트 바인딩 해제
	}

	// 2. AI 시스템 완전 정리
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI 컨트롤러 참조 받아옴
	if (AICon && IsValid(AICon)) // AI 컨트롤러 유효성 검사
	{
		AICon->StopAI(); // AI 로직 중단
		AICon->UnPossess(); // 컨트롤러-폰 관계 해제
		AICon->Destroy(); // AI 컨트롤러 완전 제거
	}

	// 4. 무브먼트 시스템 정리
	UCharacterMovementComponent* MovementComp = GetCharacterMovement(); // 캐릭터 무브먼트 컴포넌트 참조 받아옴
	if (MovementComp && IsValid(MovementComp)) // 무브먼트 컴포넌트 유효성 검사
	{
		MovementComp->DisableMovement(); // 이동 비활성화
		MovementComp->StopMovementImmediately(); // 현재 이동 즉시 중단
		MovementComp->SetMovementMode(EMovementMode::MOVE_None); // Move모드 None 설정으로 네비게이션에서 제외
		MovementComp->SetComponentTickEnabled(false); // 무브먼트 컴포넌트 Tick 비활성화
	}

	// 5. 메쉬 컴포넌트 정리
	USkeletalMeshComponent* MeshComp = GetMesh(); // 메쉬 컴포넌트 참조 받아옴
	if (MeshComp && IsValid(MeshComp)) // 메쉬 컴포넌트 유효성 검사
	{
		// 렌더링 시스템 비활성화
		MeshComp->SetVisibility(false); // 메쉬 가시성 비활성화
		MeshComp->SetHiddenInGame(true); // 게임 내 숨김 처리

		// 물리 시스템 비활성화
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // NoCollision 설정으로 충돌검사 비활성화
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // ECR_Ignore 설정으로 충돌응답 무시

		// 업데이트 시스템 비활성화
		MeshComp->SetComponentTickEnabled(false); // Tick 비활성화

		// 애니메이션 참조 해제
		MeshComp->SetAnimInstanceClass(nullptr); // ABP 참조 해제
		MeshComp->SetSkeletalMesh(nullptr); // 스켈레탈 메쉬 참조 해제
	}

	// 6. 액터 레벨 시스템 정리
	SetActorHiddenInGame(true); // 액터 렌더링 비활성화
	SetActorEnableCollision(false); // 액터 충돌 비활성화
	SetActorTickEnabled(false); // 액터 Tick 비활성화
	SetCanBeDamaged(false); // 데미지 처리 비활성화

	// 7. 현재 프레임 처리 완료 후 다음 프레임에 안전하게 엑터 제거 (크래쉬 방지)
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyDog>(this)]() // 스마트 포인터 WeakObjectPtr로 약한 참조를 사용하여 안전하게 지연 실행
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // 약한 참조한 엑터가 유효하고 파괴되지 않았다면
			{
				WeakThis->Destroy(); // 액터 완전 제거
				UE_LOG(LogTemp, Warning, TEXT("EnemyDog Successfully Destroyed"));
			}
		});
}

void AEnemyDog::EnterInAirStunState(float Duration)
{
	if (bIsDead || bIsInAirStun) return;
	UE_LOG(LogTemp, Warning, TEXT("Entering InAirStunState..."));

	bIsInAirStun = true;

	// AI 멈추기 (바로 이동 정지하지 않고, 스턴 종료 시점에서 다시 활성화)
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Stopping AI manually..."));
		AICon->StopMovement();
	}

	// 적을 위로 띄우기 (LaunchCharacter 먼저 실행)
	FVector LaunchDirection = FVector(0.0f, 0.0f, 1.0f); // 위쪽 방향
	float LaunchStrength = 600.0f; // 강한 힘 적용
	LaunchCharacter(LaunchDirection * LaunchStrength, true, true);

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s launched upwards! Current Location: %s"), *GetName(), *GetActorLocation().ToString());

	// 일정 시간 후 중력 제거 (즉시 0으로 만들면 착지가 방해될 수 있음)
	FTimerHandle GravityDisableHandle;
	GetWorld()->GetTimerManager().SetTimer(
		GravityDisableHandle,
		[this]()
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			GetCharacterMovement()->GravityScale = 0.0f;
			GetCharacterMovement()->Velocity = FVector::ZeroVector; // 위치 고정
			UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s gravity disabled, now floating!"), *GetName());
		},
		0.3f, // 0.3초 후 중력 제거
		false
	);

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // 애님인스턴스를 불러옴
	// 스턴 애니메이션 실행
	if (AnimInstance && InAirStunMontage)
	{
		AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
	}

	// 스턴 루프 타이머 시작 (에셋이 약 0.6정도 길이)
	GetWorld()->GetTimerManager().SetTimer(
		StunAnimRepeatTimerHandle,
		this,
		&AEnemyDog::PlayStunMontageLoop,
		0.4f,    // 몽타주 길이보다 짧게
		true     // 루프
	);

	// 스턴 전체 지속시간 타이머 끝나면 Exit
	GetWorld()->GetTimerManager().SetTimer(
		StunTimerHandle,
		this,
		&AEnemyDog::ExitInAirStunState,
		Duration,
		false
	);

	// 일정 시간이 지나면 원래 상태로 복귀
	GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, this, &AEnemyDog::ExitInAirStunState, Duration, false);
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s is now stunned for %f seconds!"), *GetName(), Duration);
}

void AEnemyDog::PlayStunMontageLoop()
{
	if (bIsDead || !bIsInAirStun) return;

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance && InAirStunMontage)
	{
		// 히트 몽타주 또는 사망 몽타주가 재생중일 경우 스턴 몽타주 반복 재생 중지
		bool bIsHitPlaying = false;
		bool bIsDeathPlaying = false;

		// HitMontages 배열에 대해 한 번이라도 재생중인지 체크
		for (auto* HitMontage : HitMontages)
		{
			if (AnimInstance->Montage_IsPlaying(HitMontage))
			{
				bIsHitPlaying = true;
				break;
			}
		}
		// 사망 몽타주 재생 체크
		if (AnimInstance->Montage_IsPlaying(InAirStunDeathMontage))
		{
			bIsDeathPlaying = true;
		}
		else
		{
			for (auto* DeathMontage : DeadMontages)
			{
				if (AnimInstance->Montage_IsPlaying(DeathMontage))
				{
					bIsDeathPlaying = true;
					break;
				}
			}
		}
		if (!bIsHitPlaying && !bIsDeathPlaying)
		{
			AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
		}
	}
}

void AEnemyDog::ExitInAirStunState()
{
	if (bIsDead) return;
	UE_LOG(LogTemp, Warning, TEXT("Exiting InAirStunState..."));

	bIsInAirStun = false;

	// 타이머 해제
	GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);

	// 중력 복구 및 낙하 상태로 변경
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	GetCharacterMovement()->GravityScale = 1.5f; // 조금 더 빠르게 낙하
	ApplyBaseWalkSpeed();

	// AI 이동 다시 활성화
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reactivating AI movement..."));
		GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
		GetCharacterMovement()->SetDefaultMovementMode();

		// 다시 이동 시작
		AICon->MoveToActor(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	}

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // 애님인스턴스를 불러옴
	// 애니메이션 정지
	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(0.1f);
	}

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s has recovered from stun and resumed AI behavior!"), *GetName());

	bIsInAirStun = false;
}

void AEnemyDog::ApplyGravityPull(FVector ExplosionCenter, float PullStrength)
{
	if (bIsDead) return; // 죽은 적은 끌어당기지 않음

	// 폭발 범위 내에서 적을 빨아들이는 로직
	FVector Direction = ExplosionCenter - GetActorLocation();
	float Distance = Direction.Size();
	Direction.Normalize();  // 방향 벡터 정규화

	// 거리에 따라 힘 조절 (가까울수록 더 강하게)
	float DistanceFactor = FMath::Clamp(1.0f - (Distance / 500.0f), 0.1f, 1.0f);
	float AdjustedPullStrength = PullStrength * DistanceFactor;

	// 캐릭터가 공중에 있는 상태라면 더 강한 힘 적용
	if (GetCharacterMovement()->IsFalling())
	{
		AdjustedPullStrength *= 1.5f;
	}

	// 새로운 속도 설정
	FVector NewVelocity = Direction * AdjustedPullStrength;
	GetCharacterMovement()->Velocity = NewVelocity;

	// 잠시 네비게이션 이동 비활성화
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	// 일정 시간 후 네비게이션 이동 다시 활성화
	FTimerHandle ResetMovementHandle;
	GetWorld()->GetTimerManager().SetTimer(
		ResetMovementHandle,
		[this]()
		{
			GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
		},
		0.5f, // 0.5초 후 원래 이동 모드로 복귀
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s pulled toward explosion center with strength %f"),
		*GetName(), AdjustedPullStrength);
}

void AEnemyDog::RaycastAttack()
{
	if (bHasExecutedRaycast) return;
	bHasExecutedRaycast = true;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;

	FVector StartLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector Direction = (PlayerLocation - StartLocation).GetSafeNormal();
	FVector EndLocation = StartLocation + (Direction * 150.0f);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Pawn,
		CollisionParams
	);

	// 시각화 추가 - 히트 여부에 따라 색상 변경
	if (bHit && HitResult.GetActor() == PlayerPawn)
	{
		// 히트 시 빨간색 라인
		DrawDebugLine(
			GetWorld(),
			StartLocation,
			HitResult.Location,
			FColor::Red,
			false,
			3.0f, // 3초간 표시
			0,
			5.0f // 굵기
		);

		// 히트 지점에 구체 표시
		DrawDebugSphere(
			GetWorld(),
			HitResult.Location,
			20.0f,
			12,
			FColor::Red,
			false,
			3.0f
		);

		UGameplayStatics::ApplyPointDamage(
			PlayerPawn, Damage, StartLocation, HitResult, nullptr, this, nullptr
		);

	}
	else
	{
		// 미스 시 초록색 라인
		DrawDebugLine(
			GetWorld(),
			StartLocation,
			EndLocation,
			FColor::Green,
			false,
			3.0f, // 3초간 표시
			0,
			5.0f // 굵기
		);
	}
}

void AEnemyDog::StartAttack()
{
	bIsAttacking = true;
	bHasExecutedRaycast = false;
	RaycastHitActors.Empty();
	DamagedActors.Empty();
	bCanAttack = false;
	RaycastAttack();
}

void AEnemyDog::EndAttack()
{
	bIsAttacking = false;
	bHasExecutedRaycast = false;
	RaycastHitActors.Empty();
	DamagedActors.Empty();
	bCanAttack = true;
}