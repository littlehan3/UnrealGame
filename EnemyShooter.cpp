#include "EnemyShooter.h"
#include "EnemyShooterAIController.h" // AI 컨트롤러 클래스
#include "EnemyShooterGun.h" // 총 클래스
#include "EnemyShooterAnimInstance.h" // 애님 인스턴스 클래스
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트
#include "MainGameModeBase.h" // 게임모드 참조 (적 사망 알림용)

AEnemyShooter::AEnemyShooter()
{
	PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화
	MoveComp = GetCharacterMovement();
	
	ApplyBaseWalkSpeed();
	
	MoveComp->bOrientRotationToMovement = false;
    MoveComp->bUseControllerDesiredRotation = false;

	bUseControllerRotationYaw = true; // 컨트롤러의 Yaw 회전값을 캐릭터에 직접 적용
	AIControllerClass = AEnemyShooterAIController::StaticClass(); // 이 캐릭터가 사용할 AI 컨트롤러 지정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; // 월드에 배치/스폰 시 AI가 자동 빙의
}

void AEnemyShooter::BeginPlay()
{
	Super::BeginPlay(); // 부모 클래스 BeginPlay 호출
	UWorld* World = GetWorld();
	if (!World) return;

	SetCanBeDamaged(true); // 데미지를 받을 수 있도록 설정
	SetActorTickInterval(0.2f); // Tick 주기 0.2초로 최적화
	Health = MaxHealth; //최대 체력으로 현재 체력 초기화

	AICon = Cast<AEnemyShooterAIController>(GetController()); // AI 컨트롤러 가져옴
	if (!AICon) // AI 컨트롤러가 없다면
	{
		SpawnDefaultController(); // AI 컨트롤러 스폰
		AICon = Cast<AEnemyShooterAIController>(GetController()); // AI 컨트롤러 다시 가져옴
	}

	USkeletalMeshComponent* MeshComp = GetMesh(); // 캐릭터 메쉬 컴포넌트 참조
	if (GetMesh()) // 메쉬 컴포넌트가 유효하다면
	{
		AnimInstance = Cast<UEnemyShooterAnimInstance>(MeshComp->GetAnimInstance()); // 애님 인스턴스 가져옴
	}

	MoveComp = GetCharacterMovement(); // 무브먼트 컴포넌트 참조

	// 총 생성 및 장착
	if (GunClass) // 블루프린트에서 설정한 총 클래스가 있다면
	{
		EquippedGun = World->SpawnActor<AEnemyShooterGun>(GunClass); // 월드에 총 액터 스폰
		if (EquippedGun)
		{
			EquippedGun->SetOwner(this); // 총의 소유자를 이 캐릭터로 설정
			if (MeshComp) // 메쉬 컴포넌트가 유효하다면
			{
				// 스켈레탈 메쉬의 특정 소켓에 총을 부착
				FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
				EquippedGun->AttachToComponent(MeshComp, AttachmentRules, FName("EnemyShooterGunSocket"));
			}
		}
	}

	PlaySpawnIntroAnimation(); // 스폰 인트로 애니메이션 재생
}


void AEnemyShooter::PlaySpawnIntroAnimation()
{
	//UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance || !AICon) return;

	bIsPlayingIntro = true; // 인트로 재생 중 상태로 전환
	bCanAttack = false; // 등장 중에는 공격 불가 상태
	
	AICon->StopMovement(); // 등장 애니메이션 동안 이동 중지

	PlayAnimMontage(SpawnIntroMontage); // 스폰 인트로 몽타주 재생

	// 몽타주가 끝나면 OnIntroMontageEnded 함수가 호출되도록 델리게이트 바인딩
	FOnMontageEnded EndDelegate; // 델리게이트 생성
	EndDelegate.BindUObject(this, &AEnemyShooter::OnIntroMontageEnded); // 델리게이트에 함수 바인딩
	AnimInstance->Montage_SetEndDelegate(EndDelegate, SpawnIntroMontage); // 델리게이트 설정
}

void AEnemyShooter::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsPlayingIntro = false; // 인트로 재생 상태 해제
	bCanAttack = true; // 공격 가능 상태로 전환
}

void AEnemyShooter::ApplyBaseWalkSpeed()
{
	if (MoveComp)
	{
		MoveComp->MaxWalkSpeed = 250.0f; // 기본 이동 속도 250으로 설정
		MoveComp->MaxAcceleration = 5000.0f; // 가속도를 높여 즉각 반응
		MoveComp->BrakingFrictionFactor = 10.0f; // 제동력을 높여 즉각 정지
	}
}

void AEnemyShooter::PlayShootingAnimation()
{
	if (!AnimInstance || !ShootingMontage)
	{
		// 애니메이션이 없더라도 공격 상태는 초기화해야 AI가 멈추지 않음
		bCanAttack = true;
		bIsAttacking = false;
		return;
	}

	PlayAnimMontage(ShootingMontage); // 사격 몽타주 재생

	if (AICon)
	{
		AICon->StopMovement(); // 이동 중지
	}

	bCanAttack = false; // 공격 쿨타임 시작
	bIsAttacking = true; // 공격 애니메이션 재생 중

	// 몽타주 종료 시 OnShootingMontageEnded 호출하도록 델리게이트 바인딩
	FOnMontageEnded EndDelegate; // 델리게이트 생성
	EndDelegate.BindUObject(this, &AEnemyShooter::OnShootingMontageEnded); // 델리게이트에 함수 바인딩
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ShootingMontage); // 델리게이트 설정
}

void AEnemyShooter::OnShootingMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bCanAttack = true; // 공격 가능 상태로 복귀
	bIsAttacking = false; // 공격 애니메이션 종료
}

void AEnemyShooter::PlayThrowingGrenadeAnimation()
{
	if (!AnimInstance || !ThrowingGrenadeMontage)
	{
		// 애니메이션이 없더라도 공격 상태는 초기화해야 AI가 멈추지 않음
		bCanAttack = true;
		bIsAttacking = false;
		return;
	}

	// 수류탄 투척 시에는 애니메이션 방향으로 몸을 고정해야 하므로 AI가 강제로 몸을 회전시키는 기능을 잠시 비활성화
	bUseControllerRotationYaw = false;

	PlayAnimMontage(ThrowingGrenadeMontage); // 수류탄 투척 몽타주 재생

	if (AICon)
	{
		AICon->StopMovement(); // 이동 중지
	}

	bCanAttack = false; // 공격 쿨타임 시작
	bIsAttacking = true; // 공격 애니메이션 재생 중

	// 몽타주 종료 시 OnThrowingGrenadeMontageEnded 호출하도록 델리게이트 바인딩
	FOnMontageEnded EndDelegate; // 델리게이트 생성
	EndDelegate.BindUObject(this, &AEnemyShooter::OnThrowingGrenadeMontageEnded); // 델리게이트에 함수 바인딩
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ThrowingGrenadeMontage); // 델리게이트 설정
}

void AEnemyShooter::OnThrowingGrenadeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bCanAttack = true; // 공격 가능 상태로 복귀
	bIsAttacking = false; // 공격 애니메이션 종료
	bUseControllerRotationYaw = true; // 비활성화했던 AI의 강제 회전 기능을 다시 활성화
}

float AEnemyShooter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || bIsPlayingIntro) return 0.0f; // 이미 사망했거나 인트로 재생중엔 피해를 받지 않음

	if (!AICon || !AnimInstance) return 0.0f; // AI 컨트롤러나 애님 인스턴스가 없으면 데미지 무시

	float DamageApplied = FMath::Min(Health, DamageAmount); // 실제 적용될 데미지 계산
	Health -= DamageApplied; // 체력 감소
	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter took %f damage, Health remaining: %f"), DamageAmount, Health);

	if (HitMontages.Num() == 0) return 0.0f; // 피격 몽타주 배열이 비었다면 리턴

	int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1); // 피격 몽타주 배열에서 랜덤 인덱스 선택
	UAnimMontage* SelectedMontage = HitMontages[RandomIndex]; // 선택된 몽타주

	if (SelectedMontage) // 선택된 몽타주가 유효하다면
	{
		AnimInstance->Montage_Play(SelectedMontage, 1.0f); // 선택된 몽타주 재생

		// 피격 몽타주 종료 후 스턴 상태를 처리하기 위해 델리게이트 바인딩
		FOnMontageEnded EndDelegate; // 델리게이트 생성
		EndDelegate.BindUObject(this, &AEnemyShooter::OnHitMontageEnded); // 델리게이트에 함수 바인딩
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage); // 델리게이트 설정
	}

	if (AICon)
	{
		AICon->StopMovement(); // 피격 애니메이션 동안 이동 중지
	}

	if (Health <= 0.0f) // 체력이 0 이하라면
	{
		Die(); // 사망 처리
	}

	// 모든 로직이 끝난 후 Super::TakeDamage 호출
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	return DamageApplied; // 실제 적용된 데미지 양 반환
}

float AEnemyShooter::GetHealthPercent_Implementation() const
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

bool AEnemyShooter::IsEnemyDead_Implementation() const
{
	return bIsDead;
}

void AEnemyShooter::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bIsDead) return; // 사망 시에는 아무것도 하지 않음

	UWorld* World = GetWorld();
	if (!World) return;

	if (bIsInAirStun) // 공중 스턴 상태라면
	{
		if (AnimInstance && InAirStunMontage) // 공중 스턴 몽타주가 유효하다면
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit animation ended while airborne. Resuming InAirStunMontage."));
			AnimInstance->Montage_Play(InAirStunMontage, 1.0f); // 공중 스턴 몽타주 재생
		}
	}
	else // 지상에 있을 때
	{
		if (AICon) // AI 컨트롤러가 유효하다면
		{
			// AI가 멈춰있다가 다시 플레이어를 추적하게 함
			APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0); // 플레이어 폰 참조
			if (PlayerPawn) // 플레이어 폰이 유효하다면
			{
				AICon->MoveToActor(PlayerPawn, 5.0f); // 플레이어 폰으로 이동 재개
			}
		}
	}
}

void AEnemyShooter::StopActions()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (!MoveComp || !AnimInstance) return;

	MoveComp->DisableMovement(); // 모든 이동 비활성화
	MoveComp->StopMovementImmediately(); // 즉시 정지
	 
	AnimInstance->Montage_Stop(0.1f); // 재생 중인 모든 몽타주 정지

	if (bIsInAirStun) // 스턴 상태일 경우 
	{
		bCanAttack = false; // 공격 불가 상태 유지
		World->GetTimerManager().ClearTimer(StunTimerHandle); // 스턴 해제 타이머가 있다면 취소
	}
}

void AEnemyShooter::Die()
{
	if (bIsDead) return; // 중복 사망 처리 방지

	if (!MoveComp || !AnimInstance) return;

	UWorld* World = GetWorld();
	if (!World) return;

	bIsDead = true; // 사망 상태로 전환
	Health = 0.0f;  // 체력 0으로 확정
	StopActions();  // 모든 행동 및 타이머 정
	
	AnimInstance->Montage_Stop(0.1f); // 재생 중인 다른 몽타주 중지

	float HideTime = 0.0f; // 정리까지 걸리는 시간 선언
	UAnimMontage* SelectedMontage = nullptr; // 재생할 사망 몽타주

	// 공중 스턴 상태이거나 낙하 중이고 공중 사망 몽타주가 유효한지 확인
	if ((bIsInAirStun || MoveComp->IsFalling()) && InAirStunDeathMontage)
	{
		SelectedMontage = InAirStunDeathMontage; // 공중 사망 몽타주 선택
	}
	// 아니라면 
	else if (DeadMontages.Num() > 0) // 지상 사망 몽타주 배열이 비어있지 않다면
	{
		int32 RandIndex = FMath::RandRange(0, DeadMontages.Num() - 1); // 랜덤 인덱스 선택
		SelectedMontage = DeadMontages[RandIndex]; // 지상 사망 몽타주 랜덤 선택
	}

	// 재생할 몽타주가 결정되었다면
	if (SelectedMontage)
	{
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

		if (PlayResult > 0.0f) // 몽타주 재생에 성공했다면
		{
			float MontageLength = SelectedMontage->GetPlayLength();

			// [수정] 공중 사망 시 더 빨리 사라지도록 HidePercent 조정
			float HidePercent = (SelectedMontage == InAirStunDeathMontage) ? 0.35f : 0.8f;
			HideTime = MontageLength * HidePercent;

			TWeakObjectPtr<AEnemyShooter> WeakThis(this); // 약한 참조 생성
			World->GetTimerManager().SetTimer( 
				DeathTimerHandle, 
				[WeakThis]()
				{ 
					if (WeakThis.IsValid()) // 유효성 검사
					{
						WeakThis->HideEnemy(); // 사망한 적을 숨기는 함수 호출
					}
				},
				HideTime,
				false
			);
		}
		else // 몽타주 재생에 실패했다면
		{
			HideEnemy(); // 즉시 정리
		}
	}
	else // 재생할 몽타주가 아예 없다면
	{
		HideEnemy(); // 즉시 정리
	}

	MoveComp->DisableMovement(); // 이동 비활성화
	MoveComp->StopMovementImmediately(); // 즉시 정지
	SetActorTickEnabled(false); // 성능을 위해 액터 틱 비활성화
}

// 사망 후 메모리 및 시스템 정리 함수
void AEnemyShooter::HideEnemy()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (!bIsDead) return;

	DisableGravityPull(); //정리 작업의 일환으로 중력장 효과를 해제

	UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyShooter - Memory Cleanup"));
	// 게임모드에 적이 파괴되었음을 알림
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->OnEnemyDestroyed(this);
	}

	// 1. 이벤트 및 델리게이트 정리 (가장 먼저)
	World->GetTimerManager().ClearAllTimersForObject(this); // 이 객체에 설정된 모든 타이머 해제

	UAnimInstance* ShooterAnimInstance = GetMesh()->GetAnimInstance();
	if (ShooterAnimInstance && IsValid(ShooterAnimInstance))
	{
		// 델리게이트 바인딩 완전 해제 (메모리 누수 방지)
		ShooterAnimInstance->OnMontageEnded.RemoveAll(this);
		ShooterAnimInstance->OnMontageBlendingOut.RemoveAll(this);
		ShooterAnimInstance->OnMontageStarted.RemoveAll(this);
	}

	// 2. AI 시스템 완전 정리
	AEnemyShooterAIController* ShooterAICon = Cast<AEnemyShooterAIController>(GetController());
	if (ShooterAICon && IsValid(ShooterAICon))
	{
		//ShooterAICon->StopAI(); // AI 로직 중단
		ShooterAICon->UnPossess(); // 컨트롤러와 폰의 연결(빙의) 해제
		ShooterAICon->Destroy(); // AI 컨트롤러 액터 자체를 파괴
	}

	// 3. 무기 시스템 정리
	if (EquippedGun && IsValid(EquippedGun))
	{
		EquippedGun->HideGun(); // 총 숨김 및 정리 함수 호출
		EquippedGun = nullptr; // 총에 대한 참조 해제
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
	TWeakObjectPtr<AEnemyShooter> WeakThis(this);
	World->GetTimerManager().SetTimerForNextTick(
		[WeakThis]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // 안전성 검사
			{
				WeakThis->Destroy(); // 액터를 월드에서 완전히 제거
				UE_LOG(LogTemp, Warning, TEXT("EnemyShooter Successfully Destroyed"));
			}
		});
}

void AEnemyShooter::EnterInAirStunState(float Duration) 
{
	if (bIsDead || bIsInAirStun) return;

	if (!AnimInstance || !AICon) return;

	UWorld* World = GetWorld();
	if (!World) return;

	bIsInAirStun = true;
	
	AICon->StopMovement();

	// 캐릭터를 위로 띄움
	FVector LaunchDirection = FVector(0.0f, 0.0f, 1.0f);
	float LaunchStrength = 600.0f;
	LaunchCharacter(LaunchDirection * LaunchStrength, true, true);
	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter %s launched upwards!"), *GetName());

	TWeakObjectPtr<AEnemyShooter> WeakThis(this); // 약한 참조 생성
	World->GetTimerManager().SetTimer( 
		GravityDisableHandle,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				UCharacterMovementComponent* MC = WeakThis->GetCharacterMovement(); // 무브먼트 컴포넌트 참조
				if (MC) // 무브먼트 컴포넌트가 유효하다면
				{
					MC->SetMovementMode(MOVE_Flying); // 비행 모드로 변경
					MC->GravityScale = 0.0f; // 중력 비활성화
					MC->Velocity = FVector::ZeroVector; // 속도 초기화
					UE_LOG(LogTemp, Warning, TEXT("EnemyShooter %s gravity disabled, now floating!"), *WeakThis->GetName());
				}
			}
		}, 0.3f, false // 한 번만 실행
	);

	if (InAirStunMontage) // 공중 스턴 몽타주가 유효하다면
	{
		AnimInstance->Montage_Play(InAirStunMontage, 1.0f); // 공중 스턴 몽타주 재생
	}

	World->GetTimerManager().SetTimer(
		StunTimerHandle,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ExitInAirStunState(); // 공중 스턴 상태 종료 함수 호출
			}
		}, Duration, false
	);
}
void AEnemyShooter::ExitInAirStunState()
{
	if (bIsDead) return;
	if (!MoveComp || !AnimInstance || !AICon) return;

	UWorld* World = GetWorld();
	if (!World) return;

	bIsInAirStun = false;

	// 중력 복구 및 낙하 상태로 변경
	MoveComp->SetMovementMode(MOVE_Falling);
	MoveComp->GravityScale = 1.5f;
	ApplyBaseWalkSpeed();
	
	UE_LOG(LogTemp, Warning, TEXT("Reactivating Shooter AI movement..."));
	
	MoveComp->SetMovementMode(MOVE_NavWalking); // 네비워킹 모드로 변경
	MoveComp->SetDefaultMovementMode(); // 기본 이동 모드 복구

	// 다시 이동 시작
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0); // 플레이어 폰 참조
	if (PlayerPawn)
	{
		AICon->MoveToActor(PlayerPawn, 5.0f); // 플레이어 폰으로 이동 재개
	}
	
	AnimInstance->Montage_Stop(0.1f); // 재생 중인 모든 몽타주 정지
}

void AEnemyShooter::EnableGravityPull(FVector ExplosionCenter, float PullStrength)
{
	if (bIsDead) return; // 죽은 적은 끌어당기지 않음

	UWorld* World = GetWorld();
	if (!World) return;

	if (!MoveComp || !AICon) return;

	bIsTrappedInGravityField = true;
	GravityFieldCenter = ExplosionCenter;

	AICon->StopMovement();
	AICon->SetActorTickEnabled(false); // AI Tick 완전 중지

	MoveComp->StopMovementImmediately();
	MoveComp->DisableMovement();
	MoveComp->SetMovementMode(MOVE_Flying);

	// 중력장 중앙으로 강제 이동 로직
	FVector Direction = GravityFieldCenter - GetActorLocation();
	float Distance = Direction.Size();

	if (Distance < 50.f)
	{
		SetActorLocation(GravityFieldCenter, true);
	}
	else
	{
		Direction.Normalize();
		float PullSpeed = PullStrength * 10.f;
		FVector NewLocation = GetActorLocation() + Direction * PullSpeed * World->GetDeltaSeconds();
		SetActorLocation(NewLocation, true);
	}

	MoveComp->Velocity = FVector::ZeroVector;
}

void AEnemyShooter::DisableGravityPull()
{
	if (!bIsTrappedInGravityField) return;

	UWorld* World = GetWorld();
	if (!World) return;

	if (!MoveComp || !AICon) return;

	bIsTrappedInGravityField = false;

	// 이동 모드 복구
	ApplyBaseWalkSpeed();
	MoveComp->SetMovementMode(MOVE_NavWalking);
	MoveComp->SetDefaultMovementMode();
	MoveComp->StopMovementImmediately();

	// 다시 플레이어를 추적하게 함
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (PlayerPawn)
	{
		AICon->MoveToActor(PlayerPawn, 5.0f);
	}
	AICon->SetActorTickEnabled(true); // AI Tick 다시 활성화

	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter %s has been released from gravity field"), *GetName());
}

void AEnemyShooter::Shoot()
{
	if (!IsValid(EquippedGun) || bIsDead || bIsPlayingIntro || bIsInAirStun) return; // 총이 없거나 죽었거나 등장 중이면 발사 불가

	EquippedGun->FireGun(); // 장착된 총의 발사 함수 호출

	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter fired gun!"));
}