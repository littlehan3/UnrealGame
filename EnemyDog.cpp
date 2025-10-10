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
	bCanAttack = true; // 기본적으로 공격 가능한 상태로 설정
	bIsAttacking = false; // 처음에는 공격 중이 아닌 상태로 설정

	AIControllerClass = AEnemyDogAIController::StaticClass(); // 이 캐릭터가 사용할 AI 컨트롤러를 지정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; // 월드에 배치되거나 스폰될 때 AI 컨트롤러가 자동으로 빙의하도록 설정
	GetMesh()->SetAnimInstanceClass(UEnemyDogAnimInstance::StaticClass()); // 스켈레탈 메시에 사용할 애님 인스턴스 클래스 지정

	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // 기본 이동 속도 설정
}

void AEnemyDog::BeginPlay()
{
	Super::BeginPlay(); // 부모 클래스의 BeginPlay 함수 호출
	SetCanBeDamaged(true); // 이 액터가 데미지를 받을 수 있도록 설정
	SetActorTickInterval(0.05f); // Tick 함수 호출 주기를 0.05초로 설정하여 최적화 (초당 20번)

	if (!GetController()) // AI 컨트롤러가 할당되지 않았다면
	{
		SpawnDefaultController(); // 기본 컨트롤러를 스폰하여 할당
		UE_LOG(LogTemp, Warning, TEXT("EnemyDog AI Controller manually spawned"));
	}

	AAIController* AICon = Cast<AAIController>(GetController()); // 현재 할당된 컨트롤러를 AI 컨트롤러로 캐스팅
	if (AICon) // 캐스팅이 성공했다면
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyDog AI Controller Assigend %s"), *AICon->GetName());
	}
	else // 캐스팅이 실패했다면
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyDog AI Controller is Null!"));
	}

	SetUpAI(); // AI 초기 설정 함수 호출
	PlaySpawnIntroAnimation(); // 스폰 인트로 애니메이션 재생 함수 호출
}

void AEnemyDog::PostInitializeComponents()
{
	Super::PostInitializeComponents(); // 부모 클래스의 함수 호출
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]() // 다음 틱에 실행되도록 타이머 설정 (컨트롤러 할당 보장)
		{
			AAIController* AICon = Cast<AAIController>(GetController()); // AI 컨트롤러 가져오기
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
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // 애님 인스턴스 가져오기
	if (!AnimInstance) // 애님 인스턴스가 없다면
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyDog AnimInstance not found"));
		return; // 함수 종료
	}

	bIsPlayingIntro = true; // 인트로 재생 중 상태로 변경
	bCanAttack = false; // 등장 중에는 공격 불가 상태로 설정

	// AI 이동 중지
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI 컨트롤러 가져오기
	if (AICon)
	{
		AICon->StopMovement(); // AI의 이동을 멈춤
	}

	PlayAnimMontage(SpawnIntroMontage); // 스폰 인트로 몽타주 재생

	// 몽타주가 끝나면 OnIntroMontageEnded 함수가 호출되도록 델리게이트 바인딩
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyDog::OnIntroMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, SpawnIntroMontage);
}

void AEnemyDog::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Dog Intro Montage Ended"));
	bIsPlayingIntro = false; // 인트로 재생 상태 해제
	bCanAttack = true; // 공격 가능 상태로 변경

}

void AEnemyDog::ApplyBaseWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // 기본 이동 속도를 600으로 설정
	GetCharacterMovement()->MaxAcceleration = 5000.0f; // 가속도를 높여 즉시 최대 속도에 도달하게 설정
	GetCharacterMovement()->BrakingFrictionFactor = 10.0f; // 제동 마찰력을 높여 즉시 멈추도록 설정
}

void AEnemyDog::SetUpAI()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking); // 네비게이션 메시를 따라 이동하는 모드로 설정
}

void AEnemyDog::PlayNormalAttackAnimation()
{
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // 애님 인스턴스를 가져옴
	if (!AnimInstance || NormalAttackMontages.Num() == 0) return; // 애님 인스턴스가 없거나 공격 몽타주 배열이 비었다면 리턴
	if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return; // 다른 몽타주가 재생 중이라면 리턴

	int32 RandomIndex = FMath::RandRange(0, NormalAttackMontages.Num() - 1); // 공격 몽타주 배열에서 랜덤 인덱스 선택
	UAnimMontage* SelectedMontage = NormalAttackMontages[RandomIndex]; // 선택된 몽타주

	if (SelectedMontage)
	{
		AnimInstance->Montage_Play(SelectedMontage, 1.5f); // 선택된 몽타주를 1.5배속으로 재생

		// 몽타주 종료 시 OnAttackMontageEnded 함수가 호출되도록 델리게이트 연결
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyDog::OnAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI 컨트롤러 가져오기
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

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // 애님 인스턴스 가져오기
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

	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI 컨트롤러 가져오기
	if (AICon)
	{
		AICon->StopMovement(); // 피격 애니메이션 동안 이동 중지
	}

	if (Health <= 0.0f) // 체력이 0 이하라면
	{
		Die(); // 사망 처리 함수 호출
	}

	return DamageApplied; // 실제 적용된 데미지 양 반환
}

void AEnemyDog::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bIsDead) return; // 이미 사망 상태면 아무것도 하지 않음

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (bIsInAirStun) // 공중 스턴 상태에서 피격 몽타주가 끝났다면
	{
		if (AnimInstance && InAirStunMontage)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit animation ended while airborne. Resuming InAirStunMontage."));
			GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle); // 기존 스턴 반복 타이머를 정지 (중복 방지)

			AnimInstance->Montage_Play(InAirStunMontage, 1.0f); // 다시 공중 스턴 몽타주 재생

			// 스턴 몽타주를 계속 반복하기 위해 타이머를 다시 설정
			GetWorld()->GetTimerManager().SetTimer(
				StunAnimRepeatTimerHandle,
				this,
				&AEnemyDog::PlayStunMontageLoop,
				0.4f,
				true
			);
		}
	}
	else // 지상에서 피격 몽타주가 끝났다면
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit animation ended on ground. No need to resume InAirStunMontage."));
	}
}

void AEnemyDog::Die()
{
	if (bIsDead) return; // 이미 사망 처리 중이라면 중복 실행 방지
	bIsDead = true; // 사망 상태로 전환

	// 활성화된 스턴 관련 타이머 모두 정지
	GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

	// 현재 상태에 맞는 사망 몽타주 재생
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if ((bIsInAirStun || GetCharacterMovement()->IsFalling()) && InAirStunDeathMontage) // 공중 스턴 중이거나 낙하 중일 때
	{
		AnimInstance->Montage_Play(InAirStunDeathMontage, 1.0f); // 공중 사망 몽타주 재생
	}
	else if (DeadMontages.Num() > 0) // 지상에 있을 때
	{
		int32 Rand = FMath::RandRange(0, DeadMontages.Num() - 1);
		AnimInstance->Montage_Play(DeadMontages[Rand], 1.0f); // 지상 사망 몽타주 중 하나를 랜덤 재생
	}

	// 0.5초 후에 폭발 및 숨김 처리 함수를 호출하도록 타이머 설정
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

	// AI 컨트롤러 정지
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		AICon->StopAI(); // AI 로직 전체 중단
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AIController is NULL! Can not stop AI."));
	}

	// 이동 관련 컴포넌트 비활성화
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	SetActorTickEnabled(false); // 액터의 Tick 비활성화로 성능 부담 감소

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
	if (bIsDead == false) return; // 사망 상태가 아니면 폭발하지 않음

	// 폭발 범위 내 플레이어 감지 및 데미지 처리
	FVector ExplosionCenter = GetActorLocation(); // 폭발 중심점
	float ExplosionRadiusTemp = ExplosionRadius; // 폭발 반경

	// 폭발 반경 내에 있는 Pawn 채널의 액터들을 감지
	TArray<FOverlapResult> Overlaps;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ExplosionRadiusTemp);
	bool bHasOverlaps = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		ExplosionCenter,
		FQuat::Identity,
		ECC_Pawn,
		CollisionShape
	);

	if (bHasOverlaps) // 감지된 액터가 있다면
	{
		ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0); // 플레이어 캐릭터 가져오기
		for (auto& Overlap : Overlaps) // 모든 감지된 액터에 대해 반복
		{
			if (Overlap.GetActor() == PlayerCharacter) // 감지된 액터가 플레이어라면
			{
				UGameplayStatics::ApplyDamage( // 데미지 적용
					PlayerCharacter,
					ExplosionDamage,
					GetInstigator() ? GetInstigator()->GetController() : nullptr,
					this,
					nullptr
				);
				break; // 플레이어는 한 명이므로 찾으면 바로 반복 종료
			}
		}
	}

	// 디버그용 폭발 범위 시각화
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
	if (!bIsDead) return; // 사망 상태가 아니면 실행하지 않음

	UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyDog - Memory Cleanup"));

	// 게임모드에 적이 파괴되었음을 알림 (적 카운트 감소 등)
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->OnEnemyDestroyed(this);
	}

	// 1. 모든 타이머 정리
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 애님 인스턴스 참조
	if (AnimInstance && IsValid(AnimInstance)) // 애님 인스턴스가 유효하다면
	{
		// 모든 델리게이트 바인딩 해제
		AnimInstance->OnMontageEnded.RemoveAll(this);
		AnimInstance->OnMontageBlendingOut.RemoveAll(this);
		AnimInstance->OnMontageStarted.RemoveAll(this);
	}

	// 2. AI 컨트롤러 정리
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI 컨트롤러 참조
	if (AICon && IsValid(AICon)) // AI 컨트롤러가 유효하다면
	{
		AICon->StopAI(); // AI 로직 중단
		AICon->UnPossess(); // 컨트롤러와 폰의 연결 해제
		AICon->Destroy(); // AI 컨트롤러 액터 자체를 파괴
	}

	// 4. 무브먼트 컴포넌트 정리
	UCharacterMovementComponent* MovementComp = GetCharacterMovement(); // 무브먼트 컴포넌트 참조
	if (MovementComp && IsValid(MovementComp)) // 컴포넌트가 유효하다면
	{
		MovementComp->DisableMovement(); // 이동 비활성화
		MovementComp->StopMovementImmediately(); // 현재 이동 즉시 중단
		MovementComp->SetMovementMode(EMovementMode::MOVE_None); // 이동 모드를 '없음'으로 설정하여 네비게이션에서 제외
		MovementComp->SetComponentTickEnabled(false); // 컴포넌트 Tick 비활성화
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
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyDog>(this)]() // 약한 참조(Weak Ptr)를 사용하여 안전하게 지연 실행
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
	if (bIsDead || bIsInAirStun) return; // 이미 죽었거나 스턴 상태라면 중복 실행 방지
	UE_LOG(LogTemp, Warning, TEXT("Entering InAirStunState..."));

	bIsInAirStun = true; // 공중 스턴 상태로 전환

	// AI 이동 정지
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Stopping AI manually..."));
		AICon->StopMovement();
	}

	// 캐릭터를 위로 띄움
	FVector LaunchDirection = FVector(0.0f, 0.0f, 1.0f); // 런치 방향 (위)
	float LaunchStrength = 600.0f; // 런치 강도
	LaunchCharacter(LaunchDirection * LaunchStrength, true, true); // 캐릭터 런치 실행
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s launched upwards! Current Location: %s"), *GetName(), *GetActorLocation().ToString());

	// 0.3초 후 중력을 비활성화하여 공중에 떠있게 만듦
	FTimerHandle GravityDisableHandle;
	GetWorld()->GetTimerManager().SetTimer(
		GravityDisableHandle,
		[this]()
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Flying); // 비행 모드로 전환
			GetCharacterMovement()->GravityScale = 0.0f; // 중력 비활성화
			GetCharacterMovement()->Velocity = FVector::ZeroVector; // 속도를 0으로 만들어 위치 고정
			UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s gravity disabled, now floating!"), *GetName());
		},
		0.3f, // 지연 시간
		false
	);

	// 스턴 애니메이션 재생
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance && InAirStunMontage)
	{
		AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
	}

	// 스턴 애니메이션을 반복 재생하기 위한 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(
		StunAnimRepeatTimerHandle,
		this,
		&AEnemyDog::PlayStunMontageLoop,
		0.4f,    // 몽타주 길이보다 약간 짧게 설정하여 부드럽게 연결
		true     // 반복
	);

	// 스턴 전체 지속시간이 지나면 상태를 해제하는 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(
		StunTimerHandle,
		this,
		&AEnemyDog::ExitInAirStunState,
		Duration,
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s is now stunned for %f seconds!"), *GetName(), Duration);
}

void AEnemyDog::PlayStunMontageLoop()
{
	if (bIsDead || !bIsInAirStun) return; // 죽었거나 스턴 상태가 아니면 중지

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
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
	if (bIsDead) return; // 죽었다면 상태 해제 로직 실행 안 함
	UE_LOG(LogTemp, Warning, TEXT("Exiting InAirStunState..."));

	bIsInAirStun = false; // 스턴 상태 해제

	// 스턴 애니메이션 반복 타이머 해제
	GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);

	// 중력을 다시 적용하고 낙하 상태로 변경
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	GetCharacterMovement()->GravityScale = 1.5f; // 기본 중력보다 약간 강하게 설정하여 빠르게 낙하
	ApplyBaseWalkSpeed(); // 기본 이동 속도 재적용

	// AI 이동 재활성화
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reactivating AI movement..."));
		GetCharacterMovement()->SetMovementMode(MOVE_NavWalking); // 네비게이션 보행 모드로 전환
		GetCharacterMovement()->SetDefaultMovementMode(); // 기본 이동 모드 설정
		AICon->MoveToActor(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)); // 다시 플레이어를 향해 이동 시작
	}

	// 재생 중인 몽타주 정지
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(0.1f); // 부드럽게 정지
	}

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s has recovered from stun and resumed AI behavior!"), *GetName());

	bIsInAirStun = false; // 스턴 상태 최종 해제
}

void AEnemyDog::ApplyGravityPull(FVector ExplosionCenter, float PullStrength)
{
	if (bIsDead) return; // 죽은 적은 끌어당기지 않음

	// 폭발 중심점 방향으로 힘을 적용하는 로직
	FVector Direction = ExplosionCenter - GetActorLocation(); // 현재 위치에서 중심점까지의 방향 벡터
	float Distance = Direction.Size(); // 중심점까지의 거리
	Direction.Normalize();  // 방향 벡터 정규화

	// 거리에 따라 힘을 조절 (가까울수록 강하게)
	float DistanceFactor = FMath::Clamp(1.0f - (Distance / 500.0f), 0.1f, 1.0f);
	float AdjustedPullStrength = PullStrength * DistanceFactor; // 최종 적용될 힘

	// 캐릭터가 공중에 있다면 더 강한 힘을 적용
	if (GetCharacterMovement()->IsFalling())
	{
		AdjustedPullStrength *= 1.5f;
	}

	// 계산된 힘으로 속도 설정
	FVector NewVelocity = Direction * AdjustedPullStrength;
	GetCharacterMovement()->Velocity = NewVelocity;

	// 잠시 비행 모드로 전환하여 자유롭게 움직이게 함
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	// 0.5초 후에 다시 네비게이션 보행 모드로 복귀
	FTimerHandle ResetMovementHandle;
	GetWorld()->GetTimerManager().SetTimer(
		ResetMovementHandle,
		[this]()
		{
			GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
		},
		0.5f,
		false
	);
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s pulled toward explosion center with strength %f"),
		*GetName(), AdjustedPullStrength);
}

void AEnemyDog::RaycastAttack()
{
	if (bHasExecutedRaycast) return; // 이번 공격에서 이미 판정이 끝났으면 중복 실행 방지
	bHasExecutedRaycast = true; // 판정 실행됨으로 표시

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // 플레이어 폰 가져오기
	if (!PlayerPawn) return; // 플레이어가 없으면 리턴

	FVector StartLocation = GetActorLocation(); // 레이 시작점 (자신 위치)
	FVector PlayerLocation = PlayerPawn->GetActorLocation(); // 플레이어 위치
	FVector Direction = (PlayerLocation - StartLocation).GetSafeNormal(); // 자신에서 플레이어로 향하는 방향 벡터
	FVector EndLocation = StartLocation + (Direction * 150.0f); // 150 유닛 앞까지 레이 발사

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); // 자기 자신은 레이 충돌에서 제외

	// Pawn 채널에 대해 라인 트레이스(레이캐스트) 실행
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Pawn,
		CollisionParams
	);

	if (bHit && HitResult.GetActor() == PlayerPawn) // 레이가 플레이어에게 맞았다면
	{
		// 디버그용 시각화 (빨간색 라인과 구체)
		DrawDebugLine(GetWorld(), StartLocation, HitResult.Location, FColor::Red, false, 3.0f, 0, 5.0f);
		DrawDebugSphere(GetWorld(), HitResult.Location, 20.0f, 12, FColor::Red, false, 3.0f);

		// 플레이어에게 포인트 데미지 적용
		UGameplayStatics::ApplyPointDamage(
			PlayerPawn, Damage, StartLocation, HitResult, nullptr, this, nullptr
		);
	}
	else // 레이가 빗나갔다면
	{
		// 디버그용 시각화 (초록색 라인)
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Green, false, 3.0f, 0, 5.0f);
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