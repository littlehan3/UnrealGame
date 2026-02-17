#include "EnemyShooterAIController.h"
#include "EnemyShooter.h" // 제어할 대상
#include "EnemyGuardian.h" // 상호작용할 아군 방패병
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트
#include "Engine/World.h" // UWorld 참조
#include "DrawDebugHelpers.h" // 디버그 시각화 기능
#include "EngineUtils.h" // TActorIterator 사용
#include "EnemyShooterGrenade.h" // 투척할 수류탄

AEnemyShooterAIController::AEnemyShooterAIController()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화
}

void AEnemyShooterAIController::BeginPlay()
{
    Super::BeginPlay(); // 부모 클래스 BeginPlay 호출

	UWorld* World = GetWorld();
	if (!World) return;

    PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0); // 플레이어 폰 찾아 저장
    // Pawn이 아직 없으면 다음 프레임에 재시도
    if (!GetPawn())
    {
        World->GetTimerManager().SetTimerForNextTick([this]()
            {
                CachedShooter = Cast<AEnemyShooter>(GetPawn());
                CachedGuardian = Cast<AEnemyGuardian>(GetPawn());
            });
    }
    else
    {
        CachedShooter = Cast<AEnemyShooter>(GetPawn());
        CachedGuardian = Cast<AEnemyGuardian>(GetPawn());
    }
	CurrentState = EEnemyShooterAIState::Idle; // 기본적으로 Idle 상태로 시작
}

void AEnemyShooterAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // 부모 클래스 Tick 호출

	UWorld* World = GetWorld();
	if (!World) return;

    // CachedShooter가 없으면 다시 가져오기 시도
    if (!IsValid(CachedShooter))
    {
        CachedShooter = Cast<AEnemyShooter>(GetPawn());
        if (!IsValid(CachedShooter)) return;
    }

    if (CachedShooter->bIsDead || CachedShooter->bIsPlayingIntro || CachedShooter->bIsInAirStun
		|| CachedShooter->bIsTrappedInGravityField) return; 

    if (!IsValid(PlayerPawn)) // 플레이어 폰 참조가 없다면 (플레이어가 죽었다 부활하는 등)
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0); // 다시 찾아봄
        if (!IsValid(PlayerPawn)) return; // 그래도 없으면 로직 중단
    }

    LookAtPlayerWithConstraints(DeltaTime); // 매 틱마다 플레이어를 부드럽게 바라봄

    float CurrentTime = World->GetTimeSeconds(); // 현재 게임 시간 선언

	// 주변 동료 검색 빈도
    if (CurrentTime - LastAlliesSearch >= AlliesSearchInterval)
    {
        UpdateCachedAllies(); // 동료 목록 캐시 갱신
        LastAlliesSearch = CurrentTime; // 마지막 검색 시간 기록
    }

    // 사선 확 검사 빈도
    if (CurrentTime - LastClearShotCheck >= ClearShotCheckInterval)
    {
        UpdateCachedClearShot(); // 사선 확보 여부 캐시 갱신
        LastClearShotCheck = CurrentTime; // 마지막 검사 시간 기록
    }

    // 자신의 포메이션 위치 갱신 빈도
    if (CurrentTime - LastPositionUpdate >= PositionUpdateInterval)
    {
		UpdateFormationPosition(); // 포메이션 위치 갱신
		LastPositionUpdate = CurrentTime; // 마지막 갱신 시간 기록
    }

    // 상태 머신 업데이트: 현재 상태에 따라 행동 결정
    UpdateAIState();
}

// 주변 아군 목록을 검색하여 캐시에 저장하는 함수 (1초에 한 번 호출)
void AEnemyShooterAIController::UpdateCachedAllies()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (!GetPawn()) return;

    CachedAllies.Empty(); // 기존 캐시 초기화

    // 월드에 있는 모든 AEnemyShooter 액터를 순회
	// Iterator로 순회
    // Iteraor는 후위 연산 사용시 불필요한 임시 객체 복사가 발생하므로 전위를 사용하는 룰이 있음
    for (TActorIterator<AEnemyShooter> ActorIterator(World); ActorIterator; ++ActorIterator)
    {
		AEnemyShooter* OtherShooter = *ActorIterator; // 현재 반복 중인 슈터
		if (!OtherShooter) continue; // 유효하지 않으면 건너뜀
        if (OtherShooter == GetPawn()) continue; // 자기 자신은 제외
        if (OtherShooter->bIsDead) continue; // 죽은 동료는 제외

        // Iterator는 월드에서 모든 액터를 순회하므로 찰나에 파괴된 액터가 잡힐 수 있으므로 약참조로 변환하여 저장
		CachedAllies.Add(TWeakObjectPtr<AEnemyShooter>(OtherShooter)); // 캐시에 추가
    }
}

// 플레이어까지의 사선이 확보되었는지 확인하여 캐시에 저장하는 함수 (0.3초에 한 번 호출)
void AEnemyShooterAIController::UpdateCachedClearShot()
{
	UWorld* World = GetWorld();
	if (!World) return;

    if (!IsValid(CachedShooter) || !IsValid(PlayerPawn))
    {
        bCachedHasClearShot = false; // 검사 불가 시 false로 설정
        return;
    }

    FVector StartLocation = CachedShooter->GetActorLocation(); // 라인 트레이스 시작점 (자신 위치)
    FVector EndLocation = PlayerPawn->GetActorLocation(); // 라인 트레이스 끝점 (플레이어 위치)

	FHitResult HitResult; // 충돌 결과 저장용
	FCollisionQueryParams CollisionParams; // 충돌 검사 파라미터 설정
    CollisionParams.AddIgnoredActor(CachedShooter); // 자기 자신은 무시
    CollisionParams.AddIgnoredActor(PlayerPawn); // 플레이어도 무시 (중간에 다른 액터가 있는지 확인하기 위함)
    CollisionParams.bTraceComplex = false; // 복잡한 충돌 검사 비활성화 (성능)

    // Pawn 채널에 대해 라인 트레이스 실행
    bool bHit = World->LineTraceSingleByChannel(
        HitResult, StartLocation, EndLocation, ECollisionChannel::ECC_Pawn, CollisionParams);

    bIsBlockedByGuardian = false; // 가디언 막힘 여부 초기화

    if (bHit) // 무언가에 맞았다면
    {
        if (Cast<AEnemyShooter>(HitResult.GetActor())) // 다른 슈터에 의해 막혔다면
        {
            bCachedHasClearShot = false; // 사선 미확보
            return;
        }

        if (AEnemyGuardian* HitGuardian = Cast<AEnemyGuardian>(HitResult.GetActor())) // 가디언에 의해 막혔다면
        {
            if (!HitGuardian->bIsDead) // 살아있는 가디언이라면
            {
                bCachedHasClearShot = false; // 사선 미확보
                bIsBlockedByGuardian = true; // 가디언에 의해 막혔다고 기록 (수류탄 투척 조건)
                return;
            }
        }
    }

    // 아무것에도 막히지 않았다면 사선 확보
    bCachedHasClearShot = true;
    bIsBlockedByGuardian = false;
}

// 제한된 각도 내에서 플레이어를 부드럽게 바라보는 함수
void AEnemyShooterAIController::LookAtPlayerWithConstraints(float DeltaTime)
{
	if (!CachedShooter || !IsValid(PlayerPawn)) return;

    // 목표 회전값 계산 (상하 각도 제한 적용)
	FRotator TargetRotation = CalculateConstrainedLookRotation(PlayerPawn->GetActorLocation()); // 목표 회전값
    FRotator CurrentRotation = CachedShooter->GetActorRotation(); // 현재 회전값

    // 현재 회전값에서 목표 회전값으로 부드럽게 보간
    FRotator NewRotation = FMath::RInterpTo(
        CurrentRotation, TargetRotation, DeltaTime, RotationInterpSpeed);

    CachedShooter->SetActorRotation(NewRotation); // 최종 회전값 적용
}

// 상하 각도 제한을 적용한 최종 회전값을 계산하는 함수
FRotator AEnemyShooterAIController::CalculateConstrainedLookRotation(FVector TargetLocation) const
{
    if (!CachedShooter) return FRotator::ZeroRotator;

	FVector ShooterLocation = CachedShooter->GetActorLocation(); // 자신 위치
    FVector ToTarget = TargetLocation - ShooterLocation; // 자신에서 타겟을 향하는 방향 벡터

    FRotator LookRotation = ToTarget.Rotation(); // 기본 회전값 계산

    // Pitch(상하 회전) 값을 설정된 최대/최소값 사이로 제한
    float ClampedPitch = FMath::Clamp(LookRotation.Pitch, MaxLookDownAngle, MaxLookUpAngle);

    // Yaw(좌우 회전)는 그대로, Roll(기울기)은 0으로 고정한 최종 회전값 반환
    FRotator ConstrainedRotation = FRotator(ClampedPitch, LookRotation.Yaw, 0.0f);

	return ConstrainedRotation; // 최종 회전값 반환
}

// AI 상태 머신 업데이트 함수
void AEnemyShooterAIController::UpdateAIState()
{
    // 현재 상태에 따라 적절한 핸들러 함수 호출
    switch (CurrentState)
    {
    case EEnemyShooterAIState::Idle:
        HandleIdleState();
        break; // 대기 상태
	case EEnemyShooterAIState::Detecting:
        HandleDetectingState();
        break; // 탐지 상태
	case EEnemyShooterAIState::Moving:
        HandleMovingState();
        break; // 이동 상태
	case EEnemyShooterAIState::Shooting:
        HandleShootingState();
        break; // 사격 상태
    case EEnemyShooterAIState::Retreating:
        HandleRetreatingState();
        break; // 후퇴 상태
    }
}

// 대기 상태: 플레이어가 탐지 범위에 들어오면 Detecting 상태로 전환
void AEnemyShooterAIController::HandleIdleState()
{
	if (IsPlayerInDetectionRange()) // 플레이어가 탐지 범위에 들어오면
    {
		CurrentState = EEnemyShooterAIState::Detecting; // 탐지 상태로 전환
    }
}

// 탐지 상태: 플레이어와의 거리를 기반으로 다음 행동(후퇴, 사격, 이동)을 결정
void AEnemyShooterAIController::HandleDetectingState()
{
    if (!CachedShooter) return;

    if (IsPlayerTooClose()) // 너무 가까우면
    {
        CurrentState = EEnemyShooterAIState::Retreating; // 후퇴
    }
    else if (IsPlayerInShootRange() && HasClearShotToPlayer()) // 사격 가능 거리이고 사선이 확보되면
    {
        CurrentState = EEnemyShooterAIState::Shooting; // 사격
        StopMovementInput();
    }
    else if (IsPlayerInDetectionRange()) // 탐지 거리 안에 있다면
    {
        CurrentState = EEnemyShooterAIState::Moving; // 이동
    }
    else // 탐지 거리를 벗어났다면
    {
        CurrentState = EEnemyShooterAIState::Idle; // 다시 대기 상태로
        StopMovementInput();
    }
}

// 이동 상태: 사격 위치나 진형 위치로 이동. 이동 중에도 계속 사격 가능 여부를 확인.
void AEnemyShooterAIController::HandleMovingState()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 최우선 순위: 이동 중이라도 사격이 가능해지면 즉시 사격 상태로 전환
    if (IsPlayerInShootRange() && HasClearShotToPlayer())
    {
        CurrentState = EEnemyShooterAIState::Shooting; // 사격 상태로 전환
        StopMovementInput(); // 이동 중지
        return;
    }
    // 차순위: 다른 상태 전환 조건 확인
    if (IsPlayerTooClose())
    {
        CurrentState = EEnemyShooterAIState::Retreating; // 후퇴 상태로 전환
        return; // 후퇴 상태로 전환
    }
    if (!IsPlayerInDetectionRange())
    {
        CurrentState = EEnemyShooterAIState::Idle; // 대기 상태로 전환
        StopMovementInput(); // 이동 중지
        return;
    }
    if (bIsBlockedByGuardian) // 가디언에 막혔다면 불필요한 이동 중지 (수류탄 각 재기)
    {
        if (!CachedShooter) return; // Shooter 
        ThrowGrenade(); // 수류탄 투척 시도
        return;
    }

    // 위의 모든 조건에 해당하지 않을 때만 이동 로직 수행
    if (ShouldMoveToFormation()) // 진형 위치에서 벗어났다면
    {
        MoveTowardsTarget(AssignedPosition); // 진형 위치로 이동
    }
    else // 진형 위치에 있다면
    {
        MoveTowardsPlayer(); // 플레이어를 향해 접근
    }
}

// 사격 상태: 사격을 수행하고, 상황 변화에 따라 다른 상태로 전환
void AEnemyShooterAIController::HandleShootingState()
{
	UWorld* World = GetWorld();
	if (!World) return;
    if (!CachedShooter || !PlayerPawn) return;

	if (!HasClearShotToPlayer()) // 사선이 막혔고
    {
		if (bIsBlockedByGuardian) // 가디언에 막혔다면
        {
			ThrowGrenade(); // 수류탄 투척 시도
            return;
        }
        // 가디언이 아닌 다른 이유로 사선이 막혔다면 다시 이동 상태로 전환
        CurrentState = EEnemyShooterAIState::Moving;
        return;
    }

    // 사선이 확보된 경우: 거리 재확인
	if (IsPlayerTooClose()) // 너무 가까우면
    {
		CurrentState = EEnemyShooterAIState::Retreating; // 후퇴 상태로 전환
        return;
    }
	else if (!IsPlayerInShootRange()) // 사격 거리 밖이면
    {
		CurrentState = IsPlayerInDetectionRange() ? EEnemyShooterAIState::Moving : EEnemyShooterAIState::Idle; // 이동 또는 대기 상태로 전환
		StopMovementInput(); // 이동 중지
        return;
    }

    // 모든 조건이 만족하면 사격
	if (CanShoot() && !CachedShooter->bIsAttacking && !CachedShooter->bIsDead) // 사격 가능하면
    {
		PerformShooting(); // 사격 실행
    }
}

// 후퇴 상태: 플레이어로부터 멀어지면서 거리를 벌림
void AEnemyShooterAIController::HandleRetreatingState()
{
    if (!CachedShooter || !PlayerPawn) return;

    // 플레이어 반대 방향으로 이동
	FVector ToPlayer = PlayerPawn->GetActorLocation() - CachedShooter->GetActorLocation(); // 플레이어로 향하는 벡터
    FVector RetreatDir = -ToPlayer.GetSafeNormal(); // 방향 벡터를 반대로
	CachedShooter->AddMovementInput(RetreatDir, 1.0f); // 후퇴 방향으로 이동 입력
	float Distance = GetDistanceToPlayer(); // 플레이어와의 거리 계산
    // 충분히 거리를 벌렸고 사선이 확보되면 다시 사격 상태로
	if (Distance >= (MinShootDistance + RetreatBuffer) && Distance <= ShootRadius && HasClearShotToPlayer()) // 사격 가능 거리이면
    {
		CurrentState = EEnemyShooterAIState::Shooting; // 사격 상태로 전환
		StopMovementInput(); // 이동 중지
    }
    else if (Distance > ShootRadius && IsPlayerInDetectionRange()) // 너무 멀어졌으면 다시 이동 상태로
    {
		CurrentState = EEnemyShooterAIState::Moving; // 이동 상태로 전환
    }
    else if (!IsPlayerInDetectionRange()) // 탐지 범위를 벗어나면 대기 상태로
    {
		CurrentState = EEnemyShooterAIState::Idle; // 대기 상태로 전환
		StopMovementInput(); // 이동 중지
    }
}

// 자신의 진형 위치를 계산하고 갱신
void AEnemyShooterAIController::UpdateFormationPosition()
{
	AssignedPosition = CalculateFormationPosition(); // 진형 위치 계산
	bHasAssignedPosition = true; // 위치 할당 플래그 설정
}

FVector AEnemyShooterAIController::CalculateFormationPosition()
{
    if (!PlayerPawn || !GetPawn()) return GetPawn()->GetActorLocation();

    // 시작 시에만 정제된 아군 목록을 가져옵니다
	TArray<AActor*> ValidAllies = GetAllAllies(); // 유효한 아군 목록 가져오기
	int32 TotalShooters = ValidAllies.Num() + 1; // 자신 포함
	FVector PlayerLocation = PlayerPawn->GetActorLocation(); // 플레이어 위치 가져오기

    // 내 고유 인덱스 계산
	int32 MyIndex = 0; // 자신의 인덱스 초기화
	for (AActor* Ally : ValidAllies) // 모든 아군 순회
    {
		if (GetPawn() > Ally) MyIndex++; // 메모리 주소 비교로 인덱스 증가
    }

	float AngleStep = 360.0f / TotalShooters; // 각 슈터가 차지할 각도
	float MyAngle = MyIndex * AngleStep; // 자신의 목표 각도

    // IsPositionOccupied에 매번 배열을 복사하지 말고 정제된 목록을 넘김
	auto GetPos = [&](float Angle) { // 람다 함수로 위치 계산
        return PlayerLocation + FVector(
			FMath::Cos(FMath::DegreesToRadians(Angle)) * FormationRadius, // X축
			FMath::Sin(FMath::DegreesToRadians(Angle)) * FormationRadius, // Y축
			0.0f); // Z축은 0으로 고정
        };

	FVector IdealPosition = GetPos(MyAngle); // 이상적인 진형 위치 계산

	int32 Attempts = 0; // 시도 횟수 카운터
	while (Attempts < 8) // 최대 8번 시도
    {
		bool bOccupied = false; // 자리 점유 여부 체크
		for (AActor* Ally : ValidAllies) // 모든 아군 순회
        {
			// Dist 대신 DistSquared 사용으로 성능 최적화
			if (FVector::DistSquared(IdealPosition, Ally->GetActorLocation()) < FMath::Square(150.0f)) // 150 유닛 이내에 있으면
            {
				bOccupied = true; // 자리 점유됨
				break; // 더 이상 확인할 필요 없음
            }
        }

        if (!bOccupied) break; // 빈 자리 찾음

		MyAngle += 45.0f; // 45도 회전
		IdealPosition = GetPos(MyAngle); // 이상적인 진형 위치 재계산
		Attempts++; // 시도 횟수 증가
    }

	return IdealPosition; // 최종 진형 위치 반환
}

//// 플레이어와 아군 위치를 기반으로 자신의 이상적인 진형 위치를 계산
//FVector AEnemyShooterAIController::CalculateFormationPosition()
//{
//    if (!PlayerPawn || !GetPawn()) return GetPawn()->GetActorLocation();
//
//	FVector PlayerLocation = PlayerPawn->GetActorLocation(); // 플레이어 위치 가져오기
//    TArray<AActor*> AllAllies = GetAllAllies(); // 캐시된 아군 목록 가져오기
//
//    // 자신을 포함한 전체 슈터 수 계산
//	int32 TotalShooters = AllAllies.Num() + 1; // 자신 포함
//    // 고유한 인덱스 부여 (메모리 주소 비교로 순서 결정)
//    int32 MyIndex = 0;
//	for (int32 i = 0; i < AllAllies.Num(); i++) // 모든 아군 순회
//    {
//		if (GetPawn() > AllAllies[i]) // 메모리 주소 비교
//        {
//			MyIndex++; // 자신의 인덱스 증가
//        }
//    }
//
//    // 원형 진형 계산
//    float AngleStep = 360.0f / TotalShooters; // 각 슈터가 차지할 각도
//    float MyAngle = MyIndex * AngleStep; // 자신의 목표 각도
//
//    // 플레이어 위치를 중심으로 원 위의 좌표 계산
//    FVector ToFormationPos = FVector(
//		FMath::Cos(FMath::DegreesToRadians(MyAngle)) * FormationRadius, // X축
//		FMath::Sin(FMath::DegreesToRadians(MyAngle)) * FormationRadius, // Y축
//		0.0f); // Z축은 0으로 고정
//	FVector IdealPosition = PlayerLocation + ToFormationPos; // 이상적인 진형 위치
//
//    // 만약 계산된 위치에 이미 다른 아군이 있다면 자리를 찾을 때까지 45도씩 회전하며 재탐색
//	int32 Attempts = 0; // 시도 횟수 카운터
//	while (IsPositionOccupied(IdealPosition) && Attempts < 8) // 최대 8번 시도
//    {
//		MyAngle += 45.0f; // 45도 회전
//        ToFormationPos = FVector(
//			FMath::Cos(FMath::DegreesToRadians(MyAngle)) * FormationRadius, // X축
//			FMath::Sin(FMath::DegreesToRadians(MyAngle)) * FormationRadius, // Y축
//			0.0f // Z축은 0으로 고정
//        );
//		IdealPosition = PlayerLocation + ToFormationPos; // 이상적인 진형 위치 재계산
//		Attempts++; // 시도 횟수 증가
//    }
//
//	return IdealPosition; // 최종 진형 위치 반환
//}

//// 특정 위치가 다른 아군에 의해 점유되었는지 확인
//bool AEnemyShooterAIController::IsPositionOccupied(FVector Position, float CheckRadius)
//{
//    TArray<AActor*> AllAllies = GetAllAllies();
//    for (AActor* Ally : AllAllies)
//    {
//        if (FVector::Dist(Position, Ally->GetActorLocation()) < CheckRadius)
//        {
//            return true; // 점유됨
//        }
//    }
//    return false; // 비어있음
//}

// 할당된 진형 위치로 이동해야 하는지 판단
bool AEnemyShooterAIController::ShouldMoveToFormation() const
{
    if (!bHasAssignedPosition) return false;
    // 현재 위치가 목표 위치에서 200 유닛 이상 떨어져 있다면 이동 필요
    //return FVector::Dist(GetPawn()->GetActorLocation(), AssignedPosition) > 200.0f;

    // 200.0f의 제곱인 40000.0f와 비교
    return FVector::DistSquared(GetPawn()->GetActorLocation(), AssignedPosition) > 40000.0f;
}

// 캐시된 사선 확보 여부 반환
bool AEnemyShooterAIController::HasClearShotToPlayer() const
{
    return bCachedHasClearShot;
}

// 캐시된 아군 목록을 유효한 AActor 배열로 변환하여 반환
TArray<AActor*> AEnemyShooterAIController::GetAllAllies() const
{
    TArray<AActor*> ValidAllies;
    for (const auto& WeakAlly : CachedAllies)
    {
        if (WeakAlly.IsValid()) // 약한 참조가 유효한지(액터가 파괴되지 않았는지) 확인
        {
            ValidAllies.Add(WeakAlly.Get());
        }
    }
    return ValidAllies;
}

// 목표 지점으로 이동 (이동만 담당, 회전은 Tick에서 처리)
void AEnemyShooterAIController::MoveTowardsTarget(FVector Target)
{
	if (!CachedShooter) return;

	FVector ToTarget = Target - CachedShooter->GetActorLocation(); // 목표 지점으로 향하는 벡터
	FVector MoveDirection = ToTarget.GetSafeNormal(); // 정규화된 이동 방향 벡터
	CachedShooter->AddMovementInput(MoveDirection, 1.0f); // 이동 입력 추가
}

// 유틸리티 함수들
float AEnemyShooterAIController::GetDistanceToPlayer() const
{
    if (!PlayerPawn || !CachedShooter) return FLT_MAX;
    return FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
}

bool AEnemyShooterAIController::IsPlayerInDetectionRange() const
{
    return GetDistanceToPlayer() <= DetectionRadius;
}

bool AEnemyShooterAIController::IsPlayerInShootRange() const
{
    float Distance = GetDistanceToPlayer();
    return (Distance >= MinShootDistance && Distance <= ShootRadius);
}

bool AEnemyShooterAIController::IsPlayerTooClose() const
{
    return GetDistanceToPlayer() < MinShootDistance;
}

void AEnemyShooterAIController::MoveTowardsPlayer()
{
    if (!CachedShooter || !PlayerPawn) return;

    FVector ToPlayer = PlayerPawn->GetActorLocation() - CachedShooter->GetActorLocation();
    FVector MoveDirection = ToPlayer.GetSafeNormal();
    CachedShooter->AddMovementInput(MoveDirection, 1.0f);
}

void AEnemyShooterAIController::StopMovementInput()
{
    if (!CachedShooter) return;

    CachedShooter->GetCharacterMovement()->StopMovementImmediately();
}

void AEnemyShooterAIController::PerformShooting()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (!CachedShooter) return;

    CachedShooter->Shoot(); // 캐릭터의 Shoot 함수 호출

    LastShootTime = World->GetTimeSeconds(); // 마지막 사격 시간 기록
}

void AEnemyShooterAIController::ThrowGrenade()
{
	UWorld* World = GetWorld();
	if (!World) return;

    if (World->GetTimeSeconds() - LastGrenadeThrowTime < GrenadeCooldown) // 쿨타임 확인
    {
        StopMovementInput(); // 쿨타임 중에는 대기
        return;
    }

    TSubclassOf<AEnemyShooterGrenade> GrenadeToSpawn = CachedShooter->GrenadeClass; // 던질 수류탄 클래스 가져오기
    if (!GrenadeToSpawn) return; // 수류탄 클래스 없으면 중단

    FVector StartLocation = CachedShooter->GetActorLocation() + FVector(0, 0, 100.0f); // 약간 위에서 발사

    // 플레이어 위치보다 약간 뒤를 조준 (플레이어가 피할 것을 예측)
    FVector PlayerLocation = PlayerPawn->GetActorLocation(); // 플레이어 위치
    FVector DirectionToPlayer = PlayerLocation - StartLocation; // 플레이어 방향 벡터
    DirectionToPlayer.Z = 0; // 수평 방향만 사용
    FVector EndLocation = PlayerLocation - (DirectionToPlayer.GetSafeNormal() * GrenadeTargetOffset); // 플레이어 약간 뒤 조준
    EndLocation.Z += 60.0f; // 약간의 높이 보정

    // 계산된 포물선 궤도에 따른 발사 속도 제안 받기
    FVector LaunchVelocity; // 발사 속도 벡터
    bool bHaveAimSolution = UGameplayStatics::SuggestProjectileVelocity(
        this, LaunchVelocity, StartLocation, EndLocation, GrenadeLaunchSpeed, false, 0.0f, World->GetGravityZ()); // 포물선 궤도 계산

    if (bHaveAimSolution) // 발사 각도가 나왔다면
    {
        FActorSpawnParameters SpawnParams; // 수류탄 스폰 파라미터 설정
        SpawnParams.Owner = GetPawn(); // 소유자 설정
        SpawnParams.Instigator = GetPawn(); // 인스티게이터 설정

        AEnemyShooterGrenade* Grenade = World->SpawnActor<AEnemyShooterGrenade>(
            GrenadeToSpawn, StartLocation, FRotator::ZeroRotator, SpawnParams); // 수류탄 스폰
        if (Grenade) // 수류탄이 정상적으로 스폰되었다면
        {
            Grenade->LaunchGrenade(LaunchVelocity); // 수류탄 발사
            LastGrenadeThrowTime = GetWorld()->GetTimeSeconds(); // 쿨타임 타이머 시작
            CachedShooter->PlayThrowingGrenadeAnimation(); // 캐릭터의 투척 애니메이션 재생
            StopMovementInput(); // 이동 중지
        }
    }
}

bool AEnemyShooterAIController::CanShoot() const
{
    UWorld* World = GetWorld();
    if (!World) return false;

    return World->GetTimeSeconds() - LastShootTime >= ShootCooldown; // 쿨타임이 지났는지 확인
}