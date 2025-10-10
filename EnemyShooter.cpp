#include "EnemyShooter.h"
#include "EnemyShooterAIController.h" // AI 컨트롤러 클래스
#include "EnemyShooterGun.h" // 총 클래스
#include "EnemyShooterAnimInstance.h" // 애님 인스턴스 클래스
#include "Animation/AnimInstance.h" // UAnimInstance 클래스
#include "GameFramework/Actor.h" // AActor 클래스
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수
#include "Components/SkeletalMeshComponent.h" // 스켈레탈 메쉬 컴포넌트
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트
#include "MainGameModeBase.h" // 게임모드 참조 (적 사망 알림용)

AEnemyShooter::AEnemyShooter()
{
	PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화
	bCanAttack = true; // 기본적으로 공격 가능한 상태로 설정
	bIsAttacking = false; // 처음에는 공격 중이 아닌 상태로 설정

	// AI 및 애니메이션 클래스 설정
	AIControllerClass = AEnemyShooterAIController::StaticClass(); // 이 캐릭터가 사용할 AI 컨트롤러 지정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; // 월드에 배치/스폰 시 AI가 자동 빙의
	GetMesh()->SetAnimInstanceClass(UEnemyShooterAnimInstance::StaticClass()); // 사용할 애님 인스턴스 클래스 지정

	// 회전 설정: AI 컨트롤러의 결정에 따라 캐릭터가 회전하도록 설정
	GetCharacterMovement()->bOrientRotationToMovement = false; // 이동 방향으로 자동 회전 비활성화
	GetCharacterMovement()->bUseControllerDesiredRotation = false; // 컨트롤러의 희망 회전값 사용 비활성화
	bUseControllerRotationYaw = true; // 컨트롤러의 Yaw 회전값을 캐릭터에 직접 적용 (가장 중요)

	GetCharacterMovement()->MaxWalkSpeed = 250.0f; // 기본 이동 속도 설정

	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter Generated"))
}

void AEnemyShooter::BeginPlay()
{
	Super::BeginPlay(); // 부모 클래스 BeginPlay 호출
	SetCanBeDamaged(true); // 데미지를 받을 수 있도록 설정
	SetActorTickInterval(0.2f); // Tick 주기 0.2초로 최적화

	SetupAI(); // AI 초기 설정 함수 호출

	if (!GetController()) // AI 컨트롤러가 할당되지 않았다면 (만약을 위한 방어 코드)
	{
		SpawnDefaultController(); // 기본 컨트롤러를 스폰하여 할당
		UE_LOG(LogTemp, Warning, TEXT("EnemyShooter AI Controller manually spawned"));
	}

	AAIController* AICon = Cast<AAIController>(GetController()); // AI 컨트롤러 가져오기
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyShooter AI Controller Assigend %s"), *AICon->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyShooter AI Controller is Null!"));
	}

	// 총 생성 및 장착
	if (GunClass) // 블루프린트에서 설정한 총 클래스가 있다면
	{
		EquippedGun = GetWorld()->SpawnActor<AEnemyShooterGun>(GunClass); // 월드에 총 액터 스폰
		if (EquippedGun)
		{
			EquippedGun->SetOwner(this); // 총의 소유자를 이 캐릭터로 설정
			USkeletalMeshComponent* MeshComp = GetMesh(); // 캐릭터의 스켈레탈 메쉬를 가져옴
			if (MeshComp)
			{
				// 스켈레탈 메쉬의 특정 소켓에 총을 부착
				FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
				EquippedGun->AttachToComponent(MeshComp, AttachmentRules, FName("EnemyShooterGunSocket"));
			}
		}
	}

	PlaySpawnIntroAnimation(); // 스폰 인트로 애니메이션 재생
}

void AEnemyShooter::PostInitializeComponents()
{
	Super::PostInitializeComponents(); // 부모 클래스 함수 호출
	// 다음 틱에 실행되도록 타이머를 설정 (컨트롤러가 확실히 할당된 후 로그를 찍기 위함)
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			AAIController* AICon = Cast<AAIController>(GetController()); // AI 컨트롤러 할당
			if (AICon)
			{
				UE_LOG(LogTemp, Warning, TEXT("AEnemyShooter AIController Assigned Later: %s"), *AICon->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("AEnemyShooter AIController STILL NULL!"));
			}
		});
}

void AEnemyShooter::PlaySpawnIntroAnimation()
{
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyShooter AnimInstance not found"));
		return;
	}

	bIsPlayingIntro = true; // 인트로 재생 중 상태로 전환
	bCanAttack = false; // 등장 중에는 공격 불가 상태

	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement(); // 등장 애니메이션 동안 이동 중지
	}

	PlayAnimMontage(SpawnIntroMontage); // 스폰 인트로 몽타주 재생

	// 몽타주가 끝나면 OnIntroMontageEnded 함수가 호출되도록 델리게이트 바인딩
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyShooter::OnIntroMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, SpawnIntroMontage);
}

void AEnemyShooter::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Shooter Intro Montage Ended"));
	bIsPlayingIntro = false; // 인트로 재생 상태 해제
	bCanAttack = true; // 공격 가능 상태로 전환
}

void AEnemyShooter::ApplyBaseWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = 250.0f; // 기본 이동 속도 250으로 설정
	GetCharacterMovement()->MaxAcceleration = 5000.0f; // 가속도를 높여 즉각 반응
	GetCharacterMovement()->BrakingFrictionFactor = 10.0f; // 제동력을 높여 즉각 정지
}

void AEnemyShooter::SetupAI()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking); // 네비게이션 메시 위를 걷는 모드로 설정
}

void AEnemyShooter::PlayShootingAnimation()
{
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance || !ShootingMontage)
	{
		// 애니메이션이 없더라도 공격 상태는 초기화해야 AI가 멈추지 않음
		bCanAttack = true;
		bIsAttacking = false;
		return;
	}

	PlayAnimMontage(ShootingMontage); // 사격 몽타주 재생

	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement(); // 이동 중지
	}

	bCanAttack = false; // 공격 쿨타임 시작
	bIsAttacking = true; // 공격 애니메이션 재생 중

	// 몽타주 종료 시 OnShootingMontageEnded 호출하도록 델리게이트 바인딩
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyShooter::OnShootingMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ShootingMontage);
}

void AEnemyShooter::OnShootingMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bCanAttack = true; // 공격 가능 상태로 복귀
	bIsAttacking = false; // 공격 애니메이션 종료
}

void AEnemyShooter::PlayThrowingGrenadeAnimation()
{
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance || !ThrowingGrenadeMontage)
	{
		bCanAttack = true;
		bIsAttacking = false;
		return;
	}

	// ★★★ 중요: 수류탄 투척 시에는 애니메이션 방향으로 몸을 고정해야 하므로, AI가 강제로 몸을 회전시키는 기능을 잠시 비활성화.
	bUseControllerRotationYaw = false;

	PlayAnimMontage(ThrowingGrenadeMontage); // 수류탄 투척 몽타주 재생

	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement(); // 이동 중지
	}

	bCanAttack = false; // 공격 쿨타임 시작
	bIsAttacking = true; // 공격 애니메이션 재생 중

	// 몽타주 종료 시 OnThrowingGrenadeMontageEnded 호출하도록 델리게이트 바인딩
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyShooter::OnThrowingGrenadeMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ThrowingGrenadeMontage);
}

void AEnemyShooter::OnThrowingGrenadeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bCanAttack = true; // 공격 가능 상태로 복귀
	bIsAttacking = false; // 공격 애니메이션 종료
	bUseControllerRotationYaw = true; // ★★★ 중요: 비활성화했던 AI의 강제 회전 기능을 다시 활성화.
}

float AEnemyShooter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || bIsPlayingIntro) return 0.0f; // 이미 사망했거나 인트로 재생중엔 피해를 받지 않음

	float DamageApplied = FMath::Min(Health, DamageAmount); // 실제 적용될 데미지 계산
	Health -= DamageApplied; // 체력 감소
	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter took %f damage, Health remaining: %f"), DamageAmount, Health);

	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance()); // 애님 인스턴스를 가져옴
	if (!AnimInstance || HitMontages.Num() == 0) return 0.0f; // 애님 인스턴스가 없거나 피격 몽타주 배열이 비었다면 리턴

	int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1); // 피격 몽타주 배열에서 랜덤 인덱스 선택
	UAnimMontage* SelectedMontage = HitMontages[RandomIndex]; // 선택된 몽타주

	if (SelectedMontage)
	{
		AnimInstance->Montage_Play(SelectedMontage, 1.0f); // 선택된 몽타주 재생

		// 피격 몽타주 종료 후 스턴 상태를 처리하기 위해 델리게이트 바인딩
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyShooter::OnHitMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement(); // 피격 애니메이션 동안 이동 중지
	}

	if (Health <= 0.0f) // 체력이 0 이하라면
	{
		Die(); // 사망 처리
	}

	return DamageApplied; // 실제 적용된 데미지 양 반환
}

void AEnemyShooter::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bIsDead) return; // 사망 시에는 아무것도 하지 않음
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());
	// 공중 스턴 상태에서 피격 애니메이션이 끝났을 경우, 다시 스턴 애니메이션을 이어서 재생
	if (bIsInAirStun)
	{
		if (AnimInstance && InAirStunMontage)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit animation ended while airborne. Resuming InAirStunMontage."));
			AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit animation ended on ground. No need to resume InAirStunMontage."));
	}
}

void AEnemyShooter::StopActions()
{
	AAIController* AICon = Cast<AAIController>(GetController());
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());

	GetCharacterMovement()->DisableMovement(); // 모든 이동 비활성화
	GetCharacterMovement()->StopMovementImmediately(); // 즉시 정지

	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(0.1f); // 재생 중인 모든 몽타주 정지
	}

	if (bIsInAirStun) // 스턴 상태일 경우 추가 조치
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy is stunned! Forcing all actions to stop."));
		bCanAttack = false; // 공격 불가 상태 유지
		GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle); // 스턴 해제 타이머가 있다면 취소
	}
}

void AEnemyShooter::Die()
{
	if (bIsDead) return; // 중복 사망 처리 방지
	bIsDead = true; // 사망 상태로 전환
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());

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
		float HidePercent = 0.8f; // 몽타주의 80% 지점에서 정리 시작
		float HideTime = MontageLength * HidePercent; // 실제 시간 계산

		// 계산된 시간 후에 HideEnemy 함수를 호출하도록 타이머 설정
		GetWorld()->GetTimerManager().SetTimer(
			DeathTimerHandle,
			this,
			&AEnemyShooter::HideEnemy,
			HideTime,
			false
		);
		UE_LOG(LogTemp, Warning, TEXT("EnemyShooter death montage playing, will hide after %.2f seconds (%.0f%% of %.2f total)"),
			HideTime, HidePercent * 100.0f, MontageLength);
	}
	else // 몽타주 재생에 실패했다면
	{
		HideEnemy(); // 즉시 정리
	}

	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon)
	{
		// AICon->StopAI(); // AI 로직 중단 (정리 함수에서 처리하므로 주석)
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AIController is NULL! Cannot stop AI."));
	}

	GetCharacterMovement()->DisableMovement(); // 이동 비활성화
	GetCharacterMovement()->StopMovementImmediately(); // 즉시 정지
	SetActorTickEnabled(false); // 성능을 위해 액터 틱 비활성화
	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter Die() completed"));
}

// 사망 후 메모리 및 시스템 정리 함수
void AEnemyShooter::HideEnemy()
{
	if (!bIsDead) return;

	UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyShooter - Memory Cleanup"));
	// 게임모드에 적이 파괴되었음을 알림
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->OnEnemyDestroyed(this);
	}

	// 1. 이벤트 및 델리게이트 정리 (가장 먼저)
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
	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon && IsValid(AICon))
	{
		//AICon->StopAI(); // AI 로직 중단
		AICon->UnPossess(); // 컨트롤러와 폰의 연결(빙의) 해제
		AICon->Destroy(); // AI 컨트롤러 액터 자체를 파괴
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
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyShooter>(this)]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
			{
				WeakThis->Destroy(); // 액터를 월드에서 완전히 제거
				UE_LOG(LogTemp, Warning, TEXT("EnemyShooter Successfully Destroyed"));
			}
		});
}

// 미구현 함수들
void AEnemyShooter::EnerInAirStunState(float Duration) {}
void AEnemyShooter::ExitInAirStunState() {}
void AEnemyShooter::ApplyGravityPull(FVector ExlplosionCenter, float PullStrengh) {}
//

void AEnemyShooter::Shoot()
{
	if (!EquippedGun || bIsDead || bIsPlayingIntro) // 총이 없거나, 죽었거나, 등장 중이면 발사 불가
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot shoot: Gun not available or invalid state"));
		return;
	}

	EquippedGun->FireGun(); // 장착된 총의 발사 함수 호출
	bIsShooting = true; // 사격 중 상태로 전환 (현재는 사용되지 않음)

	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter fired gun!"));
}