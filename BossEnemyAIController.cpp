#include "BossEnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "BossEnemy.h"
#include "BossEnemyAnimInstance.h"

ABossEnemyAIController::ABossEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true; // Tick 활성화
}

void ABossEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0); // 플레이어 폰 캐싱
	CachedBoss = Cast<ABossEnemy>(GetPawn()); // 보스 캐싱
	CurrentState = EBossEnemyAIState::Idle; // 기본적으로 Idle 상태로 시작
}

void ABossEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!(World)) return;

	if (!IsValid(CachedBoss) || CachedBoss->IsDead()) return; // 플레이어 또는 보스가 유효하지 않거나 죽었으면 종료

	if (!IsValid(PlayerPawn))
	{
		PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
		if (!IsValid(PlayerPawn)) return; // 여전히 없으면 다음 틱에 재시도
	}

	if (ShouldHoldForIntroOrStealth()) return; // 인트로 또는 스텔스 공격 중이면 대기

	const float DistanceToPlayer = FVector::Dist(CachedBoss->GetActorLocation(), PlayerPawn->GetActorLocation()); // 보스와 플레이어의 거리 계산

	HandleIdleRecovery(DistanceToPlayer); // Idle 상태에서 다른 상태로 전환

	if (IsBossBusy()) return; // 보스가 바쁜지 확인

	if (IsBossInHitReaction()) // 보스가 바쁘거나 피격 리액션 중이면
	{
		StopMovement();
		return;
	}

	UpdateBossAIState(DistanceToPlayer); // 플레이어와의 거리를 기반으로 AI 상태 업데이트

	switch (CurrentState) // 상태에 따른 행동 수행
	{
	case EBossEnemyAIState::MoveToPlayer: // 플레이어에게 이동
		BossMoveToPlayer(); 
		break;
	case EBossEnemyAIState::NormalAttack: // 근거리 공격
		if (bCanBossAttack) // 공격 가능하면
		{
			BossNormalAttack(); // 근거리 공격 실행
		}
		break;
	case EBossEnemyAIState::Idle: // Idle 상태
		StopMovement(); // 멈춤
		break;
	}
}

void ABossEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CachedBoss = Cast<ABossEnemy>(InPawn);
}

bool ABossEnemyAIController::IsBossBusy() const 
{
	if (!IsValid(CachedBoss)) return false; // 보스가 유효하지 않으면 false 반환
	return CachedBoss->IsPerformingAction(); // true인지 false인지 찾기 위해 보스의 IsPerformingAction 함수 호출
}

bool ABossEnemyAIController::ShouldHoldForIntroOrStealth()
{
	if (!IsValid(CachedBoss)) return true; // 보스가 유효하지 않으면 true 반환해서 멈춤

	if (CachedBoss->IsInState(EBossState::Intro) || IsExecutingStealthAttack() || bIsAIDisabledForStealth) // 인트로재생중이거나 스텔스 공격 중이거나 AI가 비활성화된 경우
	{
		if (CurrentState != EBossEnemyAIState::NormalAttack) // 현재 상태가 NormalAttack이 아니면
		{
			SetBossAIState(EBossEnemyAIState::NormalAttack); // 상태를 NormalAttack으로 설정
		}
		return true; // ture 반환해서 멈춤
	}

	return false; // false 반환해서 멈추지 않음
}

bool ABossEnemyAIController::ShouldHoldForBossAction()
{
	if (!IsValid(CachedBoss) || !CachedBoss->IsPerformingAction()) return false; // 보스가 유효하지 않거나 액션 수행 중이 아니면 false 반환
	
	if (CurrentState != EBossEnemyAIState::NormalAttack) // 현재 상태가 NormalAttack이 아니면
	{
		SetBossAIState(EBossEnemyAIState::NormalAttack); // 상태를 NormalAttack으로 설정
	}

	return true; // true 반환해서 멈춤
}

void ABossEnemyAIController::HandleIdleRecovery(float DistanceToPlayer)
{
	if (!IsValid(CachedBoss)) return; // 보스가 유효하지 않으면 종료
	if (CurrentState != EBossEnemyAIState::Idle) return; // 현재 상태가 Idle이 아니면 종료
	if (CachedBoss->IsInState(EBossState::Intro)) return; // 인트로 재생 중이면 종료
	if (DistanceToPlayer > BossDetectRadius) return; // 플레이어가 감지 반경 밖에 있으면 종료

	SetBossAIState(EBossEnemyAIState::MoveToPlayer); // 상태를 MoveToPlayer로 설정

	if (!bIsAIDisabledForStealth && !IsExecutingStealthAttack() && 
		!CachedBoss->IsPerformingAction() && !CachedBoss->IsInState(EBossState::Intro)) 
	{
		if (IsValid(PlayerPawn)) // 플레이어 폰이 유효하면
		{
			SetFocus(PlayerPawn); // 플레이어 폰을 바라보게 셋 포커스 호출
		}
	}
}

bool ABossEnemyAIController::IsBossInHitReaction() const
{
	if (!IsValid(CachedBoss)) return false; // 보스가 유효하지 않으면 false 반환

	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(CachedBoss->GetMesh()->GetAnimInstance()); // 보스 애니메이션 인스턴스 가져옴
	if (!AnimInstance || CachedBoss->BossHitReactionMontages.Num() == 0) return false; // 애니메이션 인스턴스가 유효하지 않거나 피격 리액션 몽타주가 없으면 false 반환

	for (UAnimMontage* Montage : CachedBoss->BossHitReactionMontages) // 피격 리액션 몽타주들에 대해 반복
	{
		if (Montage && AnimInstance->Montage_IsPlaying(Montage)) // 몽타주가 유효하고 현재 재생 중이면
		{
			return true; // true 반환
		}
	}

	return false; // 그 외 false 반환
}

bool ABossEnemyAIController::TryStartStealthAttack()
{
	if (!IsValid(CachedBoss)) return false; // 보스가 유효하지 않으면 false 반환
	if (!CanExecuteStealthAttack()) return false; // 스텔스 공격을 실행할 수 없으면 false 반환
	if (FMath::RandRange(0.0f, 1.0f) > 0.5f) return false; // 스텔스 공격 발동 (0~100% 중 50% 확률)

	CachedBoss->PlayBossStealthAttackAnimation(); // 스텔스 공격 애니메이션 재생
	return true; // true 반환
}

bool ABossEnemyAIController::TryStartCloseRangeAttack(float DistanceToPlayer)
{
	// 보스가 유효하지 않거나 플레이어와의 거리가 서 있는 공격 범위를 초과하면 false 반환
	if (!IsValid(CachedBoss) || DistanceToPlayer > BossStandingAttackRange) return false; 

	const bool bShouldTeleport = FMath::RandBool(); // 텔레포트 여부 랜덤 결정
	if (bShouldTeleport && CachedBoss->CanTeleport()) // 텔레포트 가능하면
	{
		CachedBoss->PlayBossTeleportAnimation(); // 텔레포트 애니메이션 재생
	}
	else // 텔레포트하지 않으면
	{
		CachedBoss->PlayBossNormalAttackAnimation(); // 일반 공격 애니메이션 재생
		StopMovement(); // 이동 멈춤
	}

	return true; // true 반환
}

bool ABossEnemyAIController::TryStartMidRangeAttack(float DistanceToPlayer, float CurrentTime)
{
	if (!IsValid(CachedBoss)) return false; // 보스가 유효하지 않으면 false 반환
	// 플레이어와의 거리가 근접 공격 범위 이내라면 false 반환
	if (DistanceToPlayer <= BossStandingAttackRange || DistanceToPlayer > BossMovingAttackRange) return false;

	const bool bDoRangedAttack = CanDoRangedAttack(CurrentTime) && FMath::RandBool(); // 원거리 공격 가능 여부 및 랜덤 결정
	if (bDoRangedAttack) // 원거리 공격 수행
	{
		CachedBoss->PlayBossRangedAttackAnimation(); // 원거리 공격 애니메이션 재생
		LastRangedAttackTime = CurrentTime; // 원거리 공격 시간 갱신
		if (bIgnoreRangedCooldownOnce) bIgnoreRangedCooldownOnce = false; // 쿨다운 무시 플래그 초기화
	}
	else
	{
		CachedBoss->PlayBossUpperBodyAttackAnimation(); // 상체 공격 애니메이션 재생
	}

	return true; // true 반환
}

bool ABossEnemyAIController::CanDoRangedAttack(float CurrentTime) const
{
	// 원거리 공격 쿨타운 무시 플래그가 설정되어 있거나 쿨타임이 충족된다면 true 반환 아니면 false 반환
	return bIgnoreRangedCooldownOnce || ((CurrentTime - LastRangedAttackTime) >= RangedAttackCooldown);
}

void ABossEnemyAIController::SetBossAIState(EBossEnemyAIState NewState)
{
	if (CurrentState == NewState) return; // 상태가 동일하면 종료

	// 상태 변경 로그 추가
	FString OldStateStr = StaticEnum<EBossEnemyAIState>()->GetValueAsString(CurrentState);
	FString NewStateStr = StaticEnum<EBossEnemyAIState>()->GetValueAsString(NewState);

	CurrentState = NewState; // 상태 갱신
}

void ABossEnemyAIController::UpdateBossAIState(float DistanceToPlayer)
{
	switch (CurrentState) // 상태에 따른 전환 로직
	{
	case EBossEnemyAIState::Idle: // Idle 상태
		if (DistanceToPlayer <= BossDetectRadius) // 플레이어가 감지 반경 내에 있으면
			SetBossAIState(EBossEnemyAIState::MoveToPlayer); // 상태를 MoveToPlayer로 설정
		break;
	case EBossEnemyAIState::MoveToPlayer: // 플레이어에게 이동
		if (DistanceToPlayer <= BossStandingAttackRange || // 플레이어가 일반공격 범위 내에 있고
			(DistanceToPlayer > BossStandingAttackRange && DistanceToPlayer <= BossMovingAttackRange)) // 플레이어가 이동공격 범위 내에 있으면
		{
			SetBossAIState(EBossEnemyAIState::NormalAttack); // 상태를 NormalAttack으로 설정
		}
		break;
	case EBossEnemyAIState::NormalAttack: // 근거리 공격
		if (DistanceToPlayer > BossMovingAttackRange) // 플레이어가 이동공격 범위 밖에 있으면
			SetBossAIState(EBossEnemyAIState::MoveToPlayer); // 상태를 MoveToPlayer로 설정
		break;
	}
}

void ABossEnemyAIController::BossMoveToPlayer()
{
	if (!IsValid(PlayerPawn) || !IsValid(CachedBoss))
	{
		return;
	}


	if (CachedBoss->IsPerformingAction()) // 액션 수행 중이면
	{
		StopMovement(); // 중지
		return;
	}

	// 실제 이동 명령이 수행됨을 알림
	MoveToActor(PlayerPawn, 5.0f); // 플레이어에게 이동, AcceptanceRadius 5.0f 근방
}

void ABossEnemyAIController::BossNormalAttack()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	if (!IsValid(CachedBoss) || !IsValid(PlayerPawn)) return;
	if (!bCanBossAttack) return;

	// 보스가 피격 리액션 중이거나 죽었거나 액션 수행 중이면 종료
	if (CachedBoss->IsInState(EBossState::HitReaction) || CachedBoss->IsDead() || CachedBoss->IsPerformingAction()) return;

	const float DistanceToPlayer = FVector::Dist(CachedBoss->GetActorLocation(), PlayerPawn->GetActorLocation()); // 보스와 플레이어의 거리 계산
	const float CurrentTime = World->GetTimeSeconds(); // 현재 시간 가져오기

	if (TryStartStealthAttack() || !CachedBoss->CanPerformAttack()) return; // 스텔스 공격 시도중이거나 공격 불가 시 종료

	// 근거리 또는 중거리 공격 시도를 통해 액션 시작
	const bool bStartedAction = TryStartCloseRangeAttack(DistanceToPlayer) || TryStartMidRangeAttack(DistanceToPlayer, CurrentTime);

	if (!bStartedAction) return; // 아무 액션도 시작하지 못했으면 종료

	// 액션이 텔레포트중, 공격텔레포트, 스텔스 공격이 아니면
	if (!CachedBoss->IsInState(EBossState::Teleporting) && !CachedBoss->IsInState(EBossState::AttackTeleport) && !IsExecutingStealthAttack()) 
	{
		bCanBossAttack = false; // 공격 불가
	}
}

void ABossEnemyAIController::OnBossNormalAttackMontageEnded()
{
	bCanBossAttack = true; // 공격 가능
}

void ABossEnemyAIController::OnBossAttackTeleportEnded()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	if (!IsValid(CachedBoss)) return; // 보스가 유효하지 않다면

	if (CachedBoss->bShouldUseRangedAfterTeleport) // 텔레포트 후 원거리 공격 사용 플래그가 설정되어 있다면
	{
		bIgnoreRangedCooldownOnce = true; // 쿨다운 무시 플래그 설정
		CachedBoss->PlayBossRangedAttackAnimation(); // 원거리 공격 애니메이션 재생
		LastRangedAttackTime = World->GetTimeSeconds(); // 원거리 공격 시간 갱신
		CachedBoss->bShouldUseRangedAfterTeleport = false; // 플래그 초기화
	}
	else // 원거리 공격 사용 플래그가 설정되어 있지 않다면
	{
		bCanBossAttack = true; // 공격 가능 플래그 설정
	}
}

void ABossEnemyAIController::OnBossRangedAttackEnded()
{
	bCanBossAttack = true; // 공격 가능
}

bool ABossEnemyAIController::HandlePostTeleportPause()
{
	if (!IsValid(CachedBoss)) return false; // 보스가 유효하지 않다면 false 반환

	const float RandomValue = FMath::FRandRange(0.0f, 100.0f);

	if (RandomValue <= 50.0f) // 공격 텔레포트 사용 시
	{
		CachedBoss->PlayBossAttackTeleportAnimation(); // 공격 텔레포트 애니메이션 재생
		return CachedBoss->IsInState(EBossState::AttackTeleport); // 상태가 AttackTeleport인지 반환
	}
	else
	{
		CachedBoss->PlayBossRangedAttackAnimation(); // 원거리 공격 애니메이션 재생
		return CachedBoss->IsInState(EBossState::RangedAttack); // 상태가 RangedAttack인지 반환
	}
}

void ABossEnemyAIController::HandlePostStealthRecovery()
{
	if (!IsValid(PlayerPawn)) return;

	SetBossAIState(EBossEnemyAIState::MoveToPlayer); // 상태를 MoveToPlayer로 설정

	SetFocus(PlayerPawn); // 플레이어 폰을 바라보게 셋 포커스 호출
}

bool ABossEnemyAIController::CanExecuteStealthAttack() const
{
	if (!IsValid(CachedBoss)) return false; // 보스가 유효하지 않으면 false 반환

	if (!CachedBoss->CanUseStealthAttack()) return false; // 스텔스 공격 사용 불가 시 false 반환

	if (CachedBoss->IsPerformingAction()) return false; // 액션 수행 중이면 false 반환

	if (!IsInOptimalStealthRange()) return false; // 최적 거리 내에 없으면 false 반환

	return true; // 그외 true 반환
}

bool ABossEnemyAIController::IsInOptimalStealthRange() const
{
	if (!IsValid(PlayerPawn) || !IsValid(CachedBoss)) return false;

	const float Distance = FVector::Dist(CachedBoss->GetActorLocation(), PlayerPawn->GetActorLocation()); // 보스와 플레이어 간의 거리 계산

	// 스텔스 공격 최적 거리: 200-300
	return (Distance >= StealthAttackMinRange && Distance <= StealthAttackOptimalRange); // 스텔스 공격 최적 거리 내에 있는지 확인
}

bool ABossEnemyAIController::IsExecutingStealthAttack() const
{
	if (!IsValid(CachedBoss)) return false; // 보스가 유효하지 않으면 false 반환

	return CachedBoss->IsExecutingStealthAttack(); // 보스가 스텔스 공격을 실행 중인지 반환
}

void ABossEnemyAIController::HandleStealthPhaseTransition(int32 NewPhase)
{
	EStealthPhase Phase = static_cast<EStealthPhase>(NewPhase); // NewPhase의 int 값을 EStealthPhase Enum으로 변환

	switch (Phase)
	{
	case EStealthPhase::Starting: // 스텔스 시작
		bIsAIDisabledForStealth = true; // AI 비활성화
		bCanBossAttack = false; // 공격 불가
		StopMovement(); // 멈춤
		break;

	case EStealthPhase::Invisible: // 투명 단계
		SetFocus(nullptr); // SetFocus 해제
		StopMovement(); // 멈춤
		break;

	case EStealthPhase::Kicking: // 킥 공격
		if (PlayerPawn) // 플레이어 폰이 유효하면
		{
			SetFocus(PlayerPawn); // 플레이어 폰을 바라보게 셋 포커스 호출
		}
		break;

	case EStealthPhase::Finishing: // 마무리 공격
		StopMovement(); // 멈춤
		break;

	case EStealthPhase::None: // 스텔스 종료
		bIsAIDisabledForStealth = false; // AI 활성화
		bCanBossAttack = true; // 공격 가능

		if (PlayerPawn) // 플레이어 폰이 유효하면
		{
			SetFocus(PlayerPawn); // 플레이어 폰을 바라보게 셋 포커스 호출
		}

		SetBossAIState(EBossEnemyAIState::MoveToPlayer); // 상태를 MoveToPlayer로 설정
		break;
	}
}

void ABossEnemyAIController::StopBossAI()
{ 
	APawn* ControlledPawn = GetPawn(); // 제어 중인 폰 가져오기
	if (!IsValid(ControlledPawn)) return; // 유효하지 않으면 종료

	StopMovement(); // 이동 중지
	UnPossess(); // 폰 소유 해제

	ControlledPawn->SetActorEnableCollision(false); // 충돌 비활성화
	ControlledPawn->SetActorTickEnabled(false); // 틱 비활성화
}
