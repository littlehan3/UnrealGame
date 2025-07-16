#include "BossEnemy.h"
#include "BossEnemyAnimInstance.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "BossEnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "MainGameModeBase.h"

ABossEnemy::ABossEnemy()
{
	PrimaryActorTick.bCanEverTick = true; // tick 활성화
	bCanBossAttack = true; // 공격 가능 여부 true
	AIControllerClass = ABossEnemyAIController::StaticClass(); // AI 컨트롤러 설정

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; //스폰 시에도 AI 컨트롤러 자동 할당

	GetMesh()->SetAnimInstanceClass(UBossEnemyAnimInstance::StaticClass()); // 애님 인스턴스 설정
	if (!AIControllerClass) // 컨트롤러가 없다면
	{
		UE_LOG(LogTemp, Error, TEXT("Boss AI Controller NULL"));
	}
}

void ABossEnemy::BeginPlay()
{
	Super::BeginPlay();
	SetCanBeDamaged(true); // 피해를 입을 수 있는지 여부 true

	// AI 컨트롤러 강제 할당
	if (!GetController())
	{
		SpawnDefaultController();
		UE_LOG(LogTemp, Warning, TEXT("Boss AI Controller manually spawned"));
	}

	AAIController* AICon = Cast<AAIController>(GetController()); // AI 컨트롤러를 불러와 캐스트
	if (AICon) // 컨트롤러가 존재한다면
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss AI Controller Assigend")); // 컨트롤러 할당됨
	}
	else // 아닌경우
	{
		UE_LOG(LogTemp, Error, TEXT("Boss AI Controller NULL")); // 컨트롤러 NULL
	}
	SetUpBossAI(); // AI 설정함수 호출
}

void ABossEnemy::SetUpBossAI()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking); // 캐릭터 무브먼트를 가져와 무브모드에 있는 네브 워킹모드로 설정
}

void ABossEnemy::PostInitializeComponents()
{
	Super::PostInitializeComponents(); // 컴포넌트 초기화 이후 추가 작업을 위해 부모함수 호출

	GetWorld()->GetTimerManager().SetTimerForNextTick([this]() // 다음 Tick에서 AI 컨트롤러 할당 여부를 확인하는 람다 등록
		{
			AAIController* AICon = Cast<AAIController>(GetController()); // 한번더 AI컨트롤러를 캐스팅해서 받아옴
			if (AICon) // 컨트롤러가 존재한다면
			{
				UE_LOG(LogTemp, Warning, TEXT("Boss AICon Assigned Later")); // 컨트롤러 할당됨
			}
			else // 아닌경우
			{
				UE_LOG(LogTemp, Error, TEXT("Boss AICon Still NULL")); // 컨트롤러 NULL
			}
		});
}

void ABossEnemy::PlayBossNormalAttackAnimation()
{
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance()); // 최신 애님인스턴스를 캐스팅해서 받아옴
	if (!AnimInstance || BossNormalAttackMontages.Num() == 0) return; // 애님인스턴스가 없거나 일반공격 몽타주가 없다면 리턴 
	AnimInstance->bUseUpperBodyBlend = false; // 상하체 분리 여부 false
	AnimInstance->Montage_Stop(0.1f, nullptr); // 다른슬롯 몽타주 중지
	if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return; // 애님인스턴스가 있고 애님인스턴스의 몽타주가 실행중이라면 리턴

	int32 RandomIndex = FMath::RandRange(0, BossNormalAttackMontages.Num() - 1); // 일반공격 몽타주배열중에 랜덤으로 선택하는 랜덤인덱스 선언
	UAnimMontage* SelectedMontage = BossNormalAttackMontages[RandomIndex]; // 랜덤인텍스에서 선택한 몽타주를 가져옴
	if (SelectedMontage) // 몽타주가 선택 되었다면
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectedMontage: %s is playing"), *SelectedMontage->GetName());
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // 플레이결과는 애님인스터의 몽타주 플레이를 1.0배속으로 재생
		if (PlayResult == 0.0f) // 플레이 결과가 0이라면
		{
			UE_LOG(LogTemp, Warning, TEXT("Montage Play Failed")); // 몽타주 재생 실패
			// 몽타주 재생 실패 시 즉시 공격 가능 상태로 복원
			bCanBossAttack = true;
			bIsFullBodyAttacking = false;
		}
		else
		{
			// 몽타주 종료 델리게이트 바인딩
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ABossEnemy::OnNormalAttackMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
			UE_LOG(LogTemp, Warning, TEXT("Montage Succesfully Playing")); // 몽타주 재생중
		}

		ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController()); // 컨트롤러를 캐스트해서 가져옴
		if (AICon) // 컨트롤러가 존재할 경우
		{
			AICon->StopMovement(); // 컨트롤러의 이동중지 함수를 호출
			UE_LOG(LogTemp, Warning, TEXT("Enemy Stopped Moving to Attack"));
		}

		bCanBossAttack = false; // 공격 중이므로 공격 불가 false
		bIsFullBodyAttacking = true; 
		DisableBossMovement();
	}
	if (BossNormalAttackSound) // 사운드가 있다면
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossNormalAttackSound, GetActorLocation()); // 해당 액터의 위치에서 소리 재생
	}
}

void ABossEnemy::OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Normal Attack Montage Ended"));

	// 공격 종료시
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // 기본값으로 복원
	}
	// 전신공격 종료 및 이동 허용
	bIsFullBodyAttacking = false;
	EnableBossMovement(); // 이동 허용

	// AI 컨트롤러에 공격 종료 알림
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true; // 공격 가능 상태로 복원
}

// 이동 차단 함수
void ABossEnemy::DisableBossMovement()
{
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->DisableMovement(); // 이동 완전 차단
		MovementComp->StopMovementImmediately(); // 현재 이동 즉시 중지
	}

	UE_LOG(LogTemp, Warning, TEXT("Boss Movement Disabled"));
}

// 이동 허용 함수
void ABossEnemy::EnableBossMovement()
{
	if (bIsBossDead) return; // 사망한 경우 이동 허용하지 않음

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->SetMovementMode(EMovementMode::MOVE_NavWalking); // 네비게이션 워킹 모드로 복원
	}

	UE_LOG(LogTemp, Warning, TEXT("Boss Movement Enabled"));
}

void ABossEnemy::PlayBossUpperBodyAttack()
{
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance || BossUpperBodyMontages.Num() == 0) return;

	AnimInstance->bUseUpperBodyBlend = true; // 상하체분리 여부 true

	int32 RandomIndex = FMath::RandRange(0, BossUpperBodyMontages.Num() - 1);
	UAnimMontage* SelectedMontage = BossUpperBodyMontages[RandomIndex];

	if (SelectedMontage)
	{
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);

		if (PlayResult > 0.0f)
		{
			// 상체 공격 종료 델리게이트 바인딩
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ABossEnemy::OnUpperBodyAttackMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);

			UE_LOG(LogTemp, Warning, TEXT("Upper Body Attack Playing"));
		}
		else
		{
			bCanBossAttack = true; // 재생 실패 시 즉시 복원
		}

		bCanBossAttack = false; // 공격 중이므로 공격 불가
	}
}

void ABossEnemy::OnUpperBodyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Upper Body Attack Montage Ended"));

	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // 기본값으로 복원
	}

	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true; // 공격 가능 상태로 복원
}

float ABossEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsBossDead) return 0.0f; // 이미 사망했다면 데미지 무시
	float DamageApplied = FMath::Min(BossHealth, DamageAmount); // 보스의 체력과 데미지를 불러옴
	BossHealth -= DamageApplied; // 체력에서 데미지만큼 차감
	UE_LOG(LogTemp, Warning, TEXT("Boss took %f damage, Health remaining: %f"), DamageAmount, BossHealth);

	if (BossHitSound) // 사운드가 있다면
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossHitSound, GetActorLocation()); // 해당 엑터의 위치에서 소리재생
	}

	if (BossHitReactionMontages.Num() > 0)
	{
		bIsBossHit = true; // 히트중 여부 true
		bCanBossAttack = false; // 공격가능 여부 false

		// AI 이동 중지
		ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
		if (AICon)
		{
			AICon->StopMovement();
		}

		UBossEnemyAnimInstance* BossAnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
		if (BossAnimInstance)
		{
			BossAnimInstance->bUseUpperBodyBlend = false;
			BossAnimInstance->Montage_Stop(0.5f); // 기존 몽타주 중지
		}

		int32 RandomIndex = FMath::RandRange(0, BossHitReactionMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossHitReactionMontages[RandomIndex];
		if (SelectedMontage)
		{
			UAnimInstance* HitAnimInstance = GetMesh()->GetAnimInstance(); // 다른 이름 사용
			if (HitAnimInstance)
			{
				UE_LOG(LogTemp, Warning, TEXT("AnimInstance is valid before playing hit montage"));
				float PlayResult = HitAnimInstance->Montage_Play(SelectedMontage, 1.0f);

				// 피격 몽타주 종료 델리게이트 추가
				if (PlayResult > 0.0f)
				{
					FOnMontageEnded HitEndDelegate;
					HitEndDelegate.BindUObject(this, &ABossEnemy::OnHitReactionMontageEnded);
					HitAnimInstance->Montage_SetEndDelegate(HitEndDelegate, SelectedMontage);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("AnimInstance is NULL before playing hit montage"));
				bIsBossHit = false; // 히트중 여부 초기화
				bCanBossAttack = true; // 공격가능 여부 초기화
			}
		}
	}

	if (BossHealth <= 0.0f) // 체력이 0이하인 경우
	{
		BossDie(); // 사망함수 호출
	}
	
	return DamageApplied; // 받은 피해 초기화
}

void ABossEnemy::OnHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Hit Reaction Montage Ended"));

	bIsBossHit = false; // 히트중 상태 false

	// 피격 애니메이션 종료 후 상태 복원
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->bUseUpperBodyBlend = false; // 기본값으로 복원
	}

	// AI 컨트롤러에 히트 종료 알림
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded(); // 공격 가능 상태 복원
	}

	bCanBossAttack = true; // 공격가능 여부 초기화
}

void ABossEnemy::BossDie()
{
	if (bIsBossDead) return; // 이미 사망한 경우 리턴
	bIsBossDead = true; // 사망상태 트루

	// GameMode에 보스 사망 알림
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->OnBossDead();
	}

	// 사망 사운드 재생
	if (BossDieSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossDieSound, GetActorLocation());
	}

	StopBossActions();

	UBossEnemyAnimInstance* BossAnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (BossAnimInstance)
	{
		BossAnimInstance->bUseUpperBodyBlend = false;
		BossAnimInstance->Montage_Stop(0.0f); // 몽타주 즉시 중지
	}

	if (BossDeadMontages.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, BossDeadMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossDeadMontages[RandomIndex];
		if (SelectedMontage)
		{
			UAnimInstance* DeathAnimInstance = GetMesh()->GetAnimInstance();
			if (DeathAnimInstance)
			{
				float PlayResult = DeathAnimInstance->Montage_Play(SelectedMontage, 1.0f);

				if (PlayResult > 0.0f)
				{
					// 몽타주 길이의 지정된 시점에 보스 숨김
					float MontageLength = SelectedMontage->GetPlayLength();
					float HideTime = MontageLength * 0.9f; // 90% 시점

					GetWorld()->GetTimerManager().SetTimer(
						BossDeathHideTimerHandle,
						this,
						&ABossEnemy::HideBossEnemy,
						HideTime, // 지정된 시점에 숨김
						false
					);

					UE_LOG(LogTemp, Warning, TEXT("Death montage playing - will hide at 99%% (%.2f seconds)"), HideTime);
					return; // 타이머가 실행될 때까지 대기
				}
			}
		}
	}
	// 몽타주가 없거나 재생 실패시
	HideBossEnemy(); // 보스를 숨기는 함수 호출
}

void ABossEnemy::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Death Montage Ended"));
	HideBossEnemy(); // 사망 몽타주 종료 후 보스 숨김
}

void ABossEnemy::StopBossActions()
{
	GetCharacterMovement()->DisableMovement(); // 모든 이동 차단
	GetCharacterMovement()->StopMovementImmediately(); // 모든 이동 즉시 차단
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None); // 이동 모드 비활성화

	// AI 컨트롤러 즉시 중지
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->StopBossAI(); // AI 중지
	}

	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance()); // 최신 애님인스턴스를 캐스팅해서 받아옴
	if (AnimInstance) // 애님 인스턴스의
	{
		AnimInstance->Montage_Stop(0.0f); // 모든 몽타주 중지
	}

	// 사망시
	if (bIsBossDead)
	{
		SetActorTickEnabled(false); // 틱 비활성화

		// 델리게이트 정리
		UAnimInstance* BaseAnimInstance = GetMesh()->GetAnimInstance();
		if (BaseAnimInstance)
		{
			BaseAnimInstance->OnMontageEnded.RemoveAll(this);
		}
	}
}

void ABossEnemy::HideBossEnemy()
{
	if (!bIsBossDead) return; // 이미 사망한 경우 리턴

	UE_LOG(LogTemp, Warning, TEXT("Hiding Boss Enemy - Memory Cleanup"));

	// 1. 이벤트 및 델리게이트 정리 (최우선)
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 해제

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 애님 인스턴스 참조 받아옴
	if (AnimInstance && IsValid(AnimInstance)) // 애님 인스턴스가 있고 유효하다면
	{
		// 애니메이션 이벤트 바인딩 완전 해제
		AnimInstance->OnMontageEnded.RemoveAll(this); // 몽타주 종료 이벤트 바인딩 해제
		AnimInstance->OnMontageBlendingOut.RemoveAll(this); // 몽타주 블랜드 아웃 바인딩 해제
		AnimInstance->OnMontageStarted.RemoveAll(this); // 몽타주 시작 이벤트 바인딩 해체
	}

	// 2. AI 시스템 완전 정리 
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController()); // AI 컨트롤러 참조 받아옴
	if (AICon && IsValid(AICon)) // AI 컨트롤러가 있고 유효하다면
	{
		AICon->StopBossAI(); // AI 로직 중단
		AICon->UnPossess(); // 컨트롤러-폰 관계 해제
		AICon->Destroy(); // AI 컨트롤러 완전 제거
	}

	// 3. 무브먼트 시스템 정리
	UCharacterMovementComponent* MovementComp = GetCharacterMovement(); // 캐릭터 무브먼트 컴포넌트 참조 받아옴
	if (MovementComp && IsValid(MovementComp)) // 무브먼트 컴포넌트가 있고 유효하다면
	{
		MovementComp->DisableMovement(); // 이동 비활성화
		MovementComp->StopMovementImmediately(); // 현재 이동 즉시 중단
		MovementComp->SetMovementMode(EMovementMode::MOVE_None); // Move모드 None 설정으로 네비게이션에서 제외
		MovementComp->SetComponentTickEnabled(false); // 무브먼트 컴포넌트 Tick 비활성화
	}

	// 4. 메쉬 컴포넌트 정리
	USkeletalMeshComponent* MeshComp = GetMesh(); // 스켈레탈 메쉬 컴포넌트 참조 받아옴
	if (MeshComp && IsValid(MeshComp)) // 메쉬 컴포넌트가 있고 유효하다면
	{
		// 렌더링 시스템 비활성화
		MeshComp->SetVisibility(false); // 메쉬 가시성 비활성화
		MeshComp->SetHiddenInGame(true); // 게임 내 숨김 처리
 
		// 물리 시스템 비활성화
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // NoCollision 설정으로 충돌검사 비활성화
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // ECR_Ignore 설정으로 충돌응답 무시

		// 업데이트 시스템 비활성화
		MeshComp->SetComponentTickEnabled(false); // 메쉬 컴포넌트 Tick 비활성화

		// 애니메이션 참조 해제
		MeshComp->SetAnimInstanceClass(nullptr); // ABP 참조 해제
		MeshComp->SetSkeletalMesh(nullptr); // 스켈레탈 메쉬 참조 해제

	}

	// 5. 액터 레벨 시스템 정리
	SetActorHiddenInGame(true); // 액터 렌더링 비활성화
	SetActorEnableCollision(false); // 액터 충돌 비활성화
	SetActorTickEnabled(false); // 액터 Tick 비활성화
	SetCanBeDamaged(false); // 데미지 처리 비활성화

	// 6. 현재 프레임 처리 완료 후 다음 프레임에 안전하게 엑터 제거 (크래쉬 방지)
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<ABossEnemy>(this)]() // 스마트 포인터 WeakObjectPtr로 약한 참조를 사용하여 안전하게 지연 실행
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // 약한 참조한 엑터가 유효하고 파괴되지 않았다면
			{
				WeakThis->Destroy(); // 액터 완전 제거
				UE_LOG(LogTemp, Warning, TEXT("EnemyBoss Successfully Destroyed."));
			}
		});
}