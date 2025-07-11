#include "BossEnemy.h"
#include "BossEnemyAnimInstance.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "BossEnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"

ABossEnemy::ABossEnemy()
{
	PrimaryActorTick.bCanEverTick = true; // tick 활성화
	bCanBossAttack = true; // 공격 가능 여부 true
	AIControllerClass = ABossEnemyAIController::StaticClass(); // AI 컨트롤러 설정
	GetMesh()->SetAnimInstanceClass(UBossEnemyAnimInstance::StaticClass()); // 애님 인스턴스 설정
	if (!AIControllerClass) // 컨트롤러가 없다면
	{
		UE_LOG(LogTemp, Error, TEXT("Boss AI Controller NULL"));
	}
	//GetCharacterMovement()->MaxWalkSpeed = 300.0f; // 기본 이동속도 설정
}

void ABossEnemy::BeginPlay()
{
	Super::BeginPlay();
	SetCanBeDamaged(true); // 피해를 입을 수 있는지 여부 true
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

	// AI 컨트롤러에 공격 종료 알림
	ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController());
	if (AICon)
	{
		AICon->OnBossNormalAttackMontageEnded();
	}

	bCanBossAttack = true; // 공격 가능 상태로 복원
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
		UBossEnemyAnimInstance* BossAnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
		if (BossAnimInstance)
		{
			BossAnimInstance->bUseUpperBodyBlend = false;
		}

		int32 RandomIndex = FMath::RandRange(0, BossHitReactionMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossHitReactionMontages[RandomIndex];
		if (SelectedMontage)
		{
			UAnimInstance* HitAnimInstance = GetMesh()->GetAnimInstance(); // 다른 이름 사용
			if (HitAnimInstance)
			{
				UE_LOG(LogTemp, Warning, TEXT("AnimInstance is valid before playing hit montage"));
				HitAnimInstance->Montage_Play(SelectedMontage, 1.0f);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("AnimInstance is NULL before playing hit montage"));
			}
		}
	}

	if (BossHealth <= 0.0f) // 체력이 0이하인 경우
	{
		BossDie(); // 사망함수 호출
	}
	
	return DamageApplied; // 받은 피해 초기화
}

void ABossEnemy::BossDie()
{
	if (bIsBossDead) return; // 이미 사망한 경우 리턴
	bIsBossDead = true; // 사망상태 트루

	UBossEnemyAnimInstance* BossAnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (BossAnimInstance)
	{
		BossAnimInstance->bUseUpperBodyBlend = false;
	}

	StopBossActions();

	if (BossDeadMontages.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, BossDeadMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossDeadMontages[RandomIndex];
		if (SelectedMontage)
		{
			UAnimInstance* DeathAnimInstance = GetMesh()->GetAnimInstance(); // 다른 이름 사용
			if (DeathAnimInstance)
			{
				DeathAnimInstance->Montage_Play(SelectedMontage, 1.0f);
			}

			// 몽타주 길이만큼 타이머 설정
			float MontageLength = SelectedMontage->GetPlayLength();
			GetWorld()->GetTimerManager().SetTimer(
				BossDeathHideTimerHandle, 
				this,
				&ABossEnemy::HideBossEnemy,
				MontageLength * 0.9f, // 90퍼센트 재생 후
				false
			);
			return;
		}
	}
	// 몽타주가 없거나 재생 실패시
	HideBossEnemy(); // 보스를 숨기는 함수 호출
}

void ABossEnemy::StopBossActions()
{
	GetCharacterMovement()->DisableMovement(); // 모든 이동 차단
	GetCharacterMovement()->StopMovementImmediately(); // 모든 이동 즉시 차단

	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance()); // 최신 애님인스턴스를 캐스팅해서 받아옴
	if (AnimInstance) // 애님 인스턴스의
	{
		AnimInstance->Montage_Stop(0.1f); // 모든 몽타주 중지
	}
}

void ABossEnemy::HideBossEnemy()
{
	if (!bIsBossDead) return;
	SetActorHiddenInGame(true); // 렌더링 숨김
	SetActorEnableCollision(false); // 충돌 제거
	SetActorTickEnabled(false); // AI 전체 Tick 비활성화
}
