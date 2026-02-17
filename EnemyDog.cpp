#include "EnemyDog.h"
#include "EnemyDogAnimInstance.h" // 애님 인스턴스 클래스 참조
#include "EnemyDogAIController.h" // AI 컨트롤러 클래스 참조
#include "Kismet/GameplayStatics.h" // 게임플레이 관련 유틸리티 함수 사용 (UGameplayStatics)
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트 제어
#include "NiagaraFunctionLibrary.h" // 나이아가라 이펙트 스폰 함수 사용
#include "Engine/OverlapResult.h" // FOverlapResult 구조체 사용
#include "MainGameModeBase.h" // 메인 게임 모드 참조 (적 파괴 알림용)

AEnemyDog::AEnemyDog()
{
	PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수를 호출하도록 설정

	AICon = nullptr; // AI 컨트롤러 초기화
	AnimInstance = nullptr; // 애님 인스턴스 초기화
	MoveComp = GetCharacterMovement(); // 무브 컴포넌트 초기화

	ApplyBaseWalkSpeed(); // 기본 이동 속도 적용 함수 호출

	AIControllerClass = AEnemyDogAIController::StaticClass(); // 이 캐릭터가 사용할 AI 컨트롤러를 지정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; // 월드에 배치되거나 스폰될 때 AI 컨트롤러가 자동으로 빙의하도록 설정
}

void AEnemyDog::BeginPlay()
{
	UWorld* World = GetWorld();
	if (!World) return;

	Super::BeginPlay();

	ApplyBaseWalkSpeed(); // 기본 이동 속도 적용 함수 호출

	SetCanBeDamaged(true); // 이 액터가 데미지를 받을 수 있도록 설정
	SetActorTickInterval(AIUpdateInterval); // Tick 함수 호출 주기를 설정하여 최적화
	Health = MaxHealth; //최대 체력으로 현재 체력 초기화

	AICon = Cast<AEnemyDogAIController>(GetController()); // AI 컨트롤러 가져옴
	if (!AICon) // AI 컨트롤러가 할당되지 않았다면
	{
		SpawnDefaultController(); // 기본 컨트롤러를 스폰하여 할당
		AICon = Cast<AEnemyDogAIController>(GetController());  // AI 컨트롤러 다시 가져옴
	}

	if (GetMesh())
	{
		AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	}

	MoveComp = GetCharacterMovement();

	SetUpAI(); // AI 초기 설정 함수 호출
	PlaySpawnIntroAnimation(); // 스폰 인트로 애니메이션 재생 함수 호출
}


void AEnemyDog::PlaySpawnIntroAnimation()
{
	//UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // 애님 인스턴스 가져오기
	if (!AnimInstance) // 애님 인스턴스가 없다면
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyDog AnimInstance not found"));
		return; // 함수 종료
	}

	bIsPlayingIntro = true; // 인트로 재생 중 상태로 변경
	bCanAttack = false; // 등장 중에는 공격 불가 상태로 설정

	// AI 이동 중지
	//AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI 컨트롤러 가져오기
	if (AICon)
	{
		AICon->StopMovement(); // AI의 이동을 멈춤
	}

	PlayAnimMontage(SpawnIntroMontage); // 스폰 인트로 몽타주 재생

	// 몽타주가 끝나면 OnIntroMontageEnded 함수가 호출되도록 델리게이트 바인딩
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyDog::OnIntroMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, SpawnIntroMontage);
}

void AEnemyDog::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Dog Intro Montage Ended"));
	bIsPlayingIntro = false; // 인트로 재생 상태 해제
	bCanAttack = true; // 공격 가능 상태로 변경
}

void AEnemyDog::ApplyBaseWalkSpeed()
{
	//UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->MaxWalkSpeed = BaseMaxWalkSpeed; // 기본 이동 속도로 설정
		MoveComp->MaxAcceleration = BaseMaxAcceleration; // 가속도를 높여 즉시 최대 속도에 도달하게 설정
		MoveComp->BrakingFrictionFactor = BaseBrakingFrictionFactor; // 제동 마찰력을 높여 즉시 멈추도록 설정
	}
}

void AEnemyDog::SetUpAI()
{
	//UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->SetMovementMode(EMovementMode::MOVE_NavWalking); // 네비게이션 메시를 따라 이동하는 모드로 설정
	}
}

void AEnemyDog::PlayNormalAttackAnimation()
{
	//UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // 애님 인스턴스를 가져옴
	if (!AnimInstance || NormalAttackMontages.Num() == 0) return; // 애님 인스턴스가 없거나 공격 몽타주 배열이 비었다면 리턴
	if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return; // 다른 몽타주가 재생 중이라면 리턴

	int32 RandomIndex = FMath::RandRange(0, NormalAttackMontages.Num() - 1); // 공격 몽타주 배열에서 랜덤 인덱스 선택
	UAnimMontage* SelectedMontage = NormalAttackMontages[RandomIndex]; // 선택된 몽타주

	if (SelectedMontage)
	{
		AnimInstance->Montage_Play(SelectedMontage, AttackAnimSpeed); // 선택된 몽타주를 1.5배속으로 재생

		// 몽타주 종료 시 OnAttackMontageEnded 함수가 호출되도록 델리게이트 연결
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyDog::OnAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	//AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI 컨트롤러 가져오기
	if (AICon)
	{
		AICon->StopMovement(); // 공격 애니메이션 동안 이동 중지
	}

	bCanAttack = false; // 공격 중에는 다시 공격할 수 없도록 설정
	bIsAttacking = true; // 공격 중 상태로 변경
}

void AEnemyDog::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bCanAttack = true; // 공격 가능 상태로 복귀
	bIsAttacking = false; // 공격 중 상태 해제
	bHasExecutedRaycast = false; // 다음 공격을 위해 레이캐스트 실행 여부 초기화
}

float AEnemyDog::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || bIsPlayingIntro) return 0.0f; // 이미 죽었거나 인트로 재생 중에는 데미지를 받지 않음

	float DamageApplied = FMath::Min(Health, DamageAmount); // 실제 적용될 데미지 (현재 체력 이상으로 깎이지 않도록)
	Health -= DamageApplied; // 체력 감소
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog took %f damage, Health remaining: %f"), DamageAmount, Health);

	//UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // 애님 인스턴스 가져오기
	if (!AnimInstance || HitMontages.Num() == 0) return 0.0f; // 애님 인스턴스가 없거나 피격 몽타주가 없으면 리턴

	int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1); // 피격 몽타주 배열에서 랜덤 인덱스 선택
	UAnimMontage* SelectedMontage = HitMontages[RandomIndex]; // 선택된 피격 몽타주

	if (SelectedMontage)
	{
		AnimInstance->Montage_Play(SelectedMontage, 1.0f); // 선택된 몽타주 재생

		// 피격 몽타주 종료 시 OnHitMontageEnded 함수가 호출되도록 델리게이트 연결
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyDog::OnHitMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	//AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI 컨트롤러 가져오기
	if (AICon)
	{
		AICon->StopMovement(); // 피격 애니메이션 동안 이동 중지
	}

	if (Health <= 0.0f) // 체력이 0 이하라면
	{
		Die();
	}

	// 모든 로직이 끝난 후 Super::TakeDamage 호출
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	return DamageApplied;
}

float AEnemyDog::GetHealthPercent_Implementation() const
{
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

bool AEnemyDog::IsEnemyDead_Implementation() const
{
	return bIsDead;
}

void AEnemyDog::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (bIsDead) return; // 이미 사망 상태면 아무것도 하지 않음

	//AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI 컨트롤러 가져오기
	//UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // 애님 인스턴스 가져오기
	if (bIsInAirStun) // 공중 스턴 상태에서 피격 몽타주가 끝났다면
	{
		if (AnimInstance && InAirStunMontage)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit animation ended while airborne. Resuming InAirStunMontage."));
			World->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle); // 기존 스턴 반복 타이머를 정지 (중복 방지)

			AnimInstance->Montage_Play(InAirStunMontage, 1.0f); // 다시 공중 스턴 몽타주 재생

			TWeakObjectPtr<AEnemyDog> WeakThis(this); // 약참조 생성
			World->GetTimerManager().SetTimer(
				StunAnimRepeatTimerHandle,
				[WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->PlayStunMontageLoop(); // 스턴 몽타주 반복 함수 호출
					}
				},StunRepeatInterval,true // 간격으로 반복
			);
		}
		else // 지상에서 피격 몽타주가 끝났다면
		{
			if (AICon)
			{
				// AI가 멈춰있다가 다시 플레이어를 추적하게 함
				AICon->MoveToActor(UGameplayStatics::GetPlayerCharacter(World, 0));
			}

			UE_LOG(LogTemp, Warning, TEXT("Hit animation ended on ground. No need to resume InAirStunMontage."));
		}
	}
	else // 지상에서 피격 몽타주가 끝났다면
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit animation ended on ground. No need to resume InAirStunMontage."));
	}
}

void AEnemyDog::Die()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (bIsDead) return; // 이미 사망 처리 중이라면 중복 실행 방지
	bIsDead = true; // 사망 상태로 전환

	// 활성화된 스턴 관련 타이머 모두 정지
	World->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);
	World->GetTimerManager().ClearTimer(StunTimerHandle);

	// 현재 상태에 맞는 사망 몽타주 재생
	//UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if ((bIsInAirStun || GetCharacterMovement()->IsFalling()) && InAirStunDeathMontage) // 공중 스턴 중이거나 낙하 중일 때
	{
		AnimInstance->Montage_Play(InAirStunDeathMontage, 1.0f); // 공중 사망 몽타주 재생
	}
	else if (DeadMontages.Num() > 0) // 지상에 있을 때
	{
		int32 Rand = FMath::RandRange(0, DeadMontages.Num() - 1);
		AnimInstance->Montage_Play(DeadMontages[Rand], 1.0f); // 지상 사망 몽타주 중 하나를 랜덤 재생
	}

	// 폭발 지연시간 후에 폭발 및 숨김 처리 함수를 호출하도록 타이머 설정
	TWeakObjectPtr<AEnemyDog> WeakThis(this); // 약참조 생성
	World->GetTimerManager().SetTimer(
		DeathTimerHandle,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->Explode();
				WeakThis->HideEnemy();
			}
		}, ExplosionDelay, false
	);

	// AI 컨트롤러 정지
	//AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		AICon->StopAI(); // AI 로직 전체 중단
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AIController is NULL! Can not stop AI."));
	}

	//UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		// 이동 관련 컴포넌트 비활성화
		MoveComp->DisableMovement();
		MoveComp->StopMovementImmediately();
		SetActorTickEnabled(false); // 액터의 Tick 비활성화로 성능 부담 감소
	}
	UE_LOG(LogTemp, Warning, TEXT("Die() called: Setting HideEnemy timer"));
}

void AEnemyDog::OnDeadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == InAirStunDeathMontage) // 공중 사망 몽타주가 끝났다면
	{
		UE_LOG(LogTemp, Warning, TEXT("InAir death montage ended, triggering explosion"));
		Explode(); // 폭발 함수 호출
		HideEnemy(); // 숨김 및 제거 함수 호출
	}
	else // 지상 사망 몽타주가 끝났다면
	{
		Explode(); // 폭발 함수 호출
		HideEnemy(); // 숨김 및 제거 함수 호출
	}
}

void AEnemyDog::Explode()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (bIsDead == false) return; // 사망 상태가 아니면 폭발하지 않음

	// 폭발 범위 내 플레이어 감지 및 데미지 처리
	FVector ExplosionCenter = GetActorLocation(); // 폭발 중심점
	float ExplosionRadiusTemp = ExplosionRadius; // 폭발 반경

	// 폭발 반경 내에 있는 Pawn 채널의 액터들을 감지
	TArray<FOverlapResult> Overlaps;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ExplosionRadiusTemp);
	bool bHasOverlaps = World->OverlapMultiByChannel(
		Overlaps,
		ExplosionCenter,
		FQuat::Identity,
		ECC_Pawn,
		CollisionShape
	);

	if (bHasOverlaps) // 감지된 액터가 있다면
	{
		ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(World, 0); // 플레이어 캐릭터 가져오기
		for (auto& Overlap : Overlaps) // 모든 감지된 액터에 대해 반복
		{
			if (Overlap.GetActor() == PlayerCharacter) // 감지된 액터가 플레이어라면
			{
				UGameplayStatics::ApplyDamage( // 데미지 적용
					PlayerCharacter, // 데미지를 받을 액터
					ExplosionDamage, // 데미지 양
					GetInstigator() ? GetInstigator()->GetController() : nullptr, // 데미지 가한 주체의 컨트롤러
					this, // 데미지를 가한 액터
					nullptr // 데미지 타입 (기본값 사용)
				);
				break; // 플레이어는 한 명이므로 찾으면 바로 반복 종료
			}
		}
	}

	// 디버그용 폭발 범위 시각화
	//DrawDebugSphere(GetWorld(), ExplosionCenter, ExplosionRadius, 32, FColor::Red, false, 1.0f, 0, 2.0f);

	// 폭발 이펙트 생성
	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
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
	UWorld* World = GetWorld();
	if (!World) return;
	if (!bIsDead) return; // 사망 상태가 아니면 실행하지 않음

	DisableGravityPull();

	UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyDog - Memory Cleanup"));

	// 게임모드에 적이 파괴되었음을 알림 (적 카운트 감소 등)
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(World->GetAuthGameMode()))
	{
		GameMode->OnEnemyDestroyed(this);
	}

	// 1. 모든 타이머 정리
	World->GetTimerManager().ClearAllTimersForObject(this);

	UAnimInstance* DogAnimInstance = GetMesh()->GetAnimInstance(); // 애님 인스턴스 참조
	if (DogAnimInstance && IsValid(DogAnimInstance)) // 애님 인스턴스가 유효하다면
	{
		// 모든 델리게이트 바인딩 해제
		DogAnimInstance->OnMontageEnded.RemoveAll(this);
		DogAnimInstance->OnMontageBlendingOut.RemoveAll(this);
		DogAnimInstance->OnMontageStarted.RemoveAll(this);
	}

	// 2. AI 컨트롤러 정리
	AEnemyDogAIController* DogAICon = Cast<AEnemyDogAIController>(GetController()); // AI 컨트롤러 참조
	if (DogAICon && IsValid(DogAICon)) // AI 컨트롤러가 유효하다면
	{
		DogAICon->StopAI(); // AI 로직 중단
		DogAICon->UnPossess(); // 컨트롤러와 폰의 연결 해제
		DogAICon->Destroy(); // AI 컨트롤러 액터 자체를 파괴
	}

	// 4. 무브먼트 컴포넌트 정리
	UCharacterMovementComponent* DogMoveComp = GetCharacterMovement(); // 무브먼트 컴포넌트 참조
	if (DogMoveComp && IsValid(DogMoveComp)) // 컴포넌트가 유효하다면
	{
		DogMoveComp->DisableMovement(); // 이동 비활성화
		DogMoveComp->StopMovementImmediately(); // 현재 이동 즉시 중단
		DogMoveComp->SetMovementMode(EMovementMode::MOVE_None); // 이동 모드를 '없음'으로 설정하여 네비게이션에서 제외
		DogMoveComp->SetComponentTickEnabled(false); // 컴포넌트 Tick 비활성화
	}

	// 5. 메쉬 컴포넌트 정리
	USkeletalMeshComponent* MeshComp = GetMesh(); // 메쉬 컴포넌트 참조
	if (MeshComp && IsValid(MeshComp)) // 컴포넌트가 유효하다면
	{
		MeshComp->SetVisibility(false); // 렌더링되지 않도록 가시성 비활성화
		MeshComp->SetHiddenInGame(true); // 게임 내에서 숨김 처리

		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 검사 비활성화
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // 모든 채널에 대한 충돌 응답 무시

		MeshComp->SetComponentTickEnabled(false); // 컴포넌트 Tick 비활성화

		MeshComp->SetAnimInstanceClass(nullptr); // 연결된 애님 인스턴스 참조 해제
		MeshComp->SetSkeletalMesh(nullptr); // 연결된 스켈레탈 메쉬 참조 해제
	}

	// 6. 액터 자체의 시스템 정리
	SetActorHiddenInGame(true); // 액터를 게임 내에서 숨김
	SetActorEnableCollision(false); // 액터의 충돌 비활성화
	SetActorTickEnabled(false); // 액터의 Tick 비활성화
	SetCanBeDamaged(false); // 데미지를 받을 수 없도록 설정

	// 7. 다음 프레임에 안전하게 액터 제거 (크래쉬 방지)
	TWeakObjectPtr<AEnemyDog> WeakThis(this); // 약한 참조(Weak Ptr)를 사용하여 안전하게 지연 실행
	World->GetTimerManager().SetTimerForNextTick(
		[WeakThis]() // 약한 참조(Weak Ptr)를 사용하여 안전하게 지연 실행
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // 참조가 유효하고, 이미 파괴 중이 아니라면
			{
				WeakThis->Destroy(); // 액터를 월드에서 완전히 제거
				UE_LOG(LogTemp, Warning, TEXT("EnemyDog Successfully Destroyed"));
			}
		});
}

void AEnemyDog::EnterInAirStunState(float Duration)
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (bIsDead || bIsInAirStun) return; // 이미 죽었거나 스턴 상태라면 중복 실행 방지
	UE_LOG(LogTemp, Warning, TEXT("Entering InAirStunState..."));

	bIsInAirStun = true; // 공중 스턴 상태로 전환

	// AI 이동 정지
	//AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Stopping AI manually..."));
		AICon->StopMovement();
	}

	// 캐릭터를 위로 띄움
	FVector LaunchDirection = FVector(0.0f, 0.0f, 1.0f); // 런치 방향 (위)
	float LaunchStrength = AirborneLaunchStrength; // 런치 강도
	LaunchCharacter(LaunchDirection * LaunchStrength, true, true); // 캐릭터 런치 실행
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s launched upwards! Current Location: %s"), *GetName(), *GetActorLocation().ToString());

	// 0.3초 후 중력을 비활성화하여 공중에 떠있게 만듦
	TWeakObjectPtr<AEnemyDog> WeakThis(this); // 약참조 생성
	World->GetTimerManager().SetTimer(
		GravityDisableHandle,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				UCharacterMovementComponent* MoveComp = WeakThis->GetCharacterMovement(); // 무브먼트 컴포넌트 참조

				if (IsValid(MoveComp))
				{
					MoveComp->SetMovementMode(MOVE_Flying); // 비행 모드로 전환
					MoveComp->GravityScale = 0.0f; // 중력 비활성화
					MoveComp->Velocity = FVector::ZeroVector; // 속도를 0으로 만들어 위치 고정
					UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s gravity disabled, now floating!"), *WeakThis->GetName());
				}
			}
		}, GravityDisableDelay, false // 지연시간, 단발성
	);

	// 스턴 애니메이션 재생
	//UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance && InAirStunMontage)
	{
		AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
	}

	// 스턴 애니메이션을 반복 재생하기 위한 타이머 시작
	World->GetTimerManager().SetTimer(
		StunAnimRepeatTimerHandle,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->PlayStunMontageLoop();
			}
		}, StunRepeatInterval, true // 간격, 반복
	);

	// 스턴 지속시간이 지나면 상태를 해제하는 타이머 시작
	World->GetTimerManager().SetTimer(
		StunTimerHandle,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ExitInAirStunState();
			}
		}, Duration, false // 지연시간, 단발성
	);

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s is now stunned for %f seconds!"), *GetName(), Duration);
}

void AEnemyDog::PlayStunMontageLoop()
{
	if (bIsDead || !bIsInAirStun) return; // 죽었거나 스턴 상태가 아니면 중지

	//UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance && InAirStunMontage)
	{
		// 피격 또는 사망 몽타주가 재생 중인지 확인
		bool bIsHitPlaying = false;
		bool bIsDeathPlaying = false;

		// HitMontages 배열에 포함된 몽타주 중 하나라도 재생 중인지 체크
		for (auto* HitMontage : HitMontages)
		{
			if (AnimInstance->Montage_IsPlaying(HitMontage))
			{
				bIsHitPlaying = true;
				break;
			}
		}
		// 사망 몽타주 재생 중인지 체크
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

		// 다른 몽타주가 재생 중이 아닐 때만 스턴 몽타주를 반복 재생
		if (!bIsHitPlaying && !bIsDeathPlaying)
		{
			AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
		}
	}
}

void AEnemyDog::ExitInAirStunState()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (bIsDead) return; // 죽었다면 상태 해제 로직 실행 안 함
	UE_LOG(LogTemp, Warning, TEXT("Exiting InAirStunState..."));

	bIsInAirStun = false; // 스턴 상태 해제

	// 스턴 애니메이션 반복 타이머 해제
	World->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);

	// 중력을 다시 적용하고 낙하 상태로 변경
	//UCharacterMovementComponent* MoveComp = GetCharacterMovement(); // 무브먼트 컴포넌트 참조
	MoveComp->SetMovementMode(MOVE_Falling); // 낙하 모드로 전환
	MoveComp->GravityScale = RecoveryGravityScale; // 기본 중력보다 약간 강하게 설정하여 빠르게 낙하
	ApplyBaseWalkSpeed(); // 기본 이동 속도 재적용

	// AI 이동 재활성화
	//AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon || MoveComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reactivating AI movement..."));
		MoveComp->SetMovementMode(MOVE_NavWalking); // 네비게이션 보행 모드로 전환
		MoveComp->SetDefaultMovementMode(); // 기본 이동 모드 설정
		AICon->MoveToActor(UGameplayStatics::GetPlayerCharacter(World, 0)); // 다시 플레이어를 향해 이동 시작
	}

	// 재생 중인 몽타주 정지
	//UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(0.1f); // 부드럽게 정지
	}

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s has recovered from stun and resumed AI behavior!"), *GetName());

	bIsInAirStun = false; // 스턴 상태 최종 해제
}

void AEnemyDog::EnableGravityPull(FVector ExplosionCenter, float PullStrength)
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (bIsDead) return; // 죽은 적은 끌어당기지 않음 [cite: 120]

	// 중력장 상태 업데이트
	bIsTrappedInGravityField = true;
	GravityFieldCenter = ExplosionCenter;

	// 1. AI·이동 전면 차단
	//if (AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()))
	if (AICon)
	{
		AICon->StopMovement();
		AICon->SetActorTickEnabled(false); // AI Tick 완전 중지
	}

	//UCharacterMovementComponent* MoveComp = GetCharacterMovement(); // 무브먼트 컴포넌트 참조
	if (MoveComp)
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
		MoveComp->SetMovementMode(MOVE_Flying); // 중심 고정에 유리
	}

	// 3. 중력장 중앙으로 강제 이동 로직 (AEnemy와 동일)
	FVector Direction = GravityFieldCenter - GetActorLocation();
	float Distance = Direction.Size();

	// 너무 중앙에 가까우면 바로 가운데 고정
	if (Distance < GravityPullTolerance)
	{
		SetActorLocation(GravityFieldCenter, true);
	}
	else
	{
		// AEnemy의 로직을 따름 (단순 SetVelocity가 아닌 SetActorLocation)
		Direction.Normalize();
		float PullSpeed = PullStrength * 10.f; // 강도 강화
		FVector NewLocation = GetActorLocation() + Direction * PullSpeed * World->GetDeltaSeconds();
		SetActorLocation(NewLocation, true);
	}

	// 4. 중력장에 붙잡힌 상태에서 절대 못 빠져나가도록 위치·이동 고정
	MoveComp->Velocity = FVector::ZeroVector;
}

void AEnemyDog::DisableGravityPull()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (!bIsTrappedInGravityField) return;

	bIsTrappedInGravityField = false;
	//UCharacterMovementComponent* MoveComp = GetCharacterMovement(); // 무브먼트 컴포넌트 참조

	// 이동 모드 복구
	ApplyBaseWalkSpeed();

	if (MoveComp)
	{
		MoveComp->SetMovementMode(MOVE_NavWalking);
		MoveComp->SetDefaultMovementMode();
		MoveComp->StopMovementImmediately();
	}

	if (AICon)
	{
		// 다시 플레이어를 추적하게 함
		AICon->MoveToActor(UGameplayStatics::GetPlayerCharacter(World, 0));
		AICon->SetActorTickEnabled(true); // AI Tick 다시 활성화
	}
	
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s has been released from gravity field"), *GetName());
}

void AEnemyDog::RaycastAttack()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (bHasExecutedRaycast) return; // 이번 공격에서 이미 판정이 끝났으면 중복 실행 방지
	bHasExecutedRaycast = true; // 판정 실행됨으로 표시

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // 플레이어 폰 가져오기
	if (!PlayerPawn) return; // 플레이어가 없으면 리턴

	FVector StartLocation = GetActorLocation(); // 레이 시작점 (자신 위치)
	FVector PlayerLocation = PlayerPawn->GetActorLocation(); // 플레이어 위치
	FVector Direction = (PlayerLocation - StartLocation).GetSafeNormal(); // 자신에서 플레이어로 향하는 방향 벡터
	FVector EndLocation = StartLocation + (Direction * AttackTraceDistance); // 레이캐스트 거리까지 레이 발사

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); // 자기 자신은 레이 충돌에서 제외

	// Pawn 채널에 대해 라인 트레이스(레이캐스트) 실행
	bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Pawn,
		CollisionParams
	);

	if (bHit && HitResult.GetActor() == PlayerPawn) // 레이가 플레이어에게 맞았다면
	{
		//// 디버그용 시각화 (빨간색 라인과 구체)
		//DrawDebugLine(GetWorld(), StartLocation, HitResult.Location, FColor::Red, false, 3.0f, 0, 5.0f);
		//DrawDebugSphere(GetWorld(), HitResult.Location, 20.0f, 12, FColor::Red, false, 3.0f);

		// 플레이어에게 포인트 데미지 적용
		UGameplayStatics::ApplyPointDamage(PlayerPawn, Damage, StartLocation, HitResult, nullptr, this, nullptr);

		// *** 나이아가라 이펙트 스폰 로직 수정 ***
		if (UNiagaraSystem* HitNiagara = this->WeaponHitNiagaraEffect)
		{
			// HitResult의 정보를 사용하여 이펙트를 스폰합니다.
			// HitResult.ImpactNormal.Rotation()으로 피격 면의 법선 방향을 회전으로 사용합니다.
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				HitNiagara,
				HitResult.ImpactPoint, // 히트 지점 사용
				HitResult.ImpactNormal.Rotation(), // 피격 면의 노멀 방향 사용
				FVector(1.0f),
				true,
				true
			);
		}

		if (AttackHitSound)
		{
			// 피격 위치(ImpactPoint)에서 사운드 재생
			UGameplayStatics::PlaySoundAtLocation(this, AttackHitSound, HitResult.ImpactPoint);
		}
	}
	else // 레이가 빗나갔다면
	{
		// 디버그용 시각화 (초록색 라인)
		/*DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Green, false, 3.0f, 0, 5.0f);*/
	}
}

void AEnemyDog::StartAttack()
{
	bIsAttacking = true; // 공격 중 상태로 설정
	bHasExecutedRaycast = false; // 레이캐스트 실행 여부 초기화
	RaycastHitActors.Empty(); // 히트 액터 목록 초기화
	DamagedActors.Empty(); // 데미지 입힌 액터 목록 초기화
	bCanAttack = false; // 공격 쿨타임 동안 공격 불가
	RaycastAttack(); // 레이캐스트 공격 실행
}

void AEnemyDog::EndAttack()
{
	bIsAttacking = false; // 공격 중 상태 해제
	bHasExecutedRaycast = false; // 레이캐스트 실행 여부 초기화
	RaycastHitActors.Empty(); // 히트 액터 목록 초기화
	DamagedActors.Empty(); // 데미지 입힌 액터 목록 초기화
	bCanAttack = true; // 다시 공격 가능 상태로 변경
}