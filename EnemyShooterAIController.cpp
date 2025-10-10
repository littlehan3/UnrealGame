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
    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // 플레이어 폰 찾아 저장
}

void AEnemyShooterAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // 부모 클래스 Tick 호출
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn()); // 제어 중인 폰 가져오기
    // 폰이 없거나, 죽었거나, 등장 중일 때는 아무런 로직도 실행하지 않음
    if (!Shooter || Shooter->bIsDead || Shooter->bIsPlayingIntro)
        return;

    if (!PlayerPawn) // 플레이어 폰 참조가 없다면 (플레이어가 죽었다 부활하는 등)
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // 다시 찾아봄
        if (!PlayerPawn) return; // 그래도 없으면 로직 중단
    }

    LookAtPlayerWithConstraints(DeltaTime); // 매 틱마다 플레이어를 부드럽게 바라봄

    float CurrentTime = GetWorld()->GetTimeSeconds(); // 현재 게임 시간

    // --- 성능 최적화: 타이머를 이용해 비싼 연산을 나눠서 실행 ---
    // 1초마다: 주변 동료 검색
    if (CurrentTime - LastAlliesSearch >= AlliesSearchInterval)
    {
        UpdateCachedAllies(); // 동료 목록 캐시 갱신
        LastAlliesSearch = CurrentTime; // 마지막 검색 시간 기록
    }

    // 0.3초마다: 사선 확보(LOS) 검사
    if (CurrentTime - LastClearShotCheck >= ClearShotCheckInterval)
    {
        UpdateCachedClearShot(); // 사선 확보 여부 캐시 갱신
        LastClearShotCheck = CurrentTime; // 마지막 검사 시간 기록
    }

    // 1초마다: 자신의 포메이션 위치 갱신
    if (CurrentTime - LastPositionUpdate >= PositionUpdateInterval)
    {
        UpdateFormationPosition();
        LastPositionUpdate = CurrentTime;
    }

    // 상태 머신 업데이트: 현재 상태에 따라 행동 결정
    UpdateAIState();
}

// 주변 아군 목록을 검색하여 캐시에 저장하는 함수 (1초에 한 번 호출)
void AEnemyShooterAIController::UpdateCachedAllies()
{
    CachedAllies.Empty(); // 기존 캐시 초기화
    if (!GetPawn()) return;

    // 월드에 있는 모든 AEnemyShooter 액터를 순회
    for (TActorIterator<AEnemyShooter> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
    {
        AEnemyShooter* OtherShooter = *ActorIterator;
        if (OtherShooter == GetPawn()) continue; // 자기 자신은 제외
        if (OtherShooter->bIsDead) continue; // 죽은 동료는 제외

        CachedAllies.Add(TWeakObjectPtr<AEnemyShooter>(OtherShooter)); // 약한 참조(Weak Ptr)로 캐시에 추가
    }
}

// 플레이어까지의 사선이 확보되었는지 확인하여 캐시에 저장하는 함수 (0.3초에 한 번 호출)
void AEnemyShooterAIController::UpdateCachedClearShot()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter || !PlayerPawn)
    {
        bCachedHasClearShot = false; // 검사 불가 시 false로 설정
        return;
    }

    FVector StartLocation = Shooter->GetActorLocation(); // 라인 트레이스 시작점 (자신 위치)
    FVector EndLocation = PlayerPawn->GetActorLocation(); // 라인 트레이스 끝점 (플레이어 위치)

    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(Shooter); // 자기 자신은 무시
    CollisionParams.AddIgnoredActor(PlayerPawn); // 플레이어도 무시 (중간에 다른 액터가 있는지 확인하기 위함)
    CollisionParams.bTraceComplex = false; // 복잡한 충돌 검사 비활성화 (성능)

    // Pawn 채널에 대해 라인 트레이스 실행
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult, StartLocation, EndLocation, ECollisionChannel::ECC_Pawn, CollisionParams
    );

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
                UE_LOG(LogTemp, Warning, TEXT("shooter stopped shooting because guardian is blocking forward. now start grenade attack"));
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
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter || !PlayerPawn) return;

    // 목표 회전값 계산 (상하 각도 제한 적용)
    FRotator TargetRotation = CalculateConstrainedLookRotation(PlayerPawn->GetActorLocation());
    FRotator CurrentRotation = Shooter->GetActorRotation(); // 현재 회전값

    // 현재 회전값에서 목표 회전값으로 부드럽게 보간
    FRotator NewRotation = FMath::RInterpTo(
        CurrentRotation, TargetRotation, DeltaTime, RotationInterpSpeed
    );
    Shooter->SetActorRotation(NewRotation); // 최종 회전값 적용
}

// 상하 각도 제한을 적용한 최종 회전값을 계산하는 함수
FRotator AEnemyShooterAIController::CalculateConstrainedLookRotation(FVector TargetLocation) const
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter) return FRotator::ZeroRotator;

    FVector ShooterLocation = Shooter->GetActorLocation();
    FVector ToTarget = TargetLocation - ShooterLocation; // 자신에서 타겟을 향하는 방향 벡터

    FRotator LookRotation = ToTarget.Rotation(); // 기본 회전값 계산

    // Pitch(상하 회전) 값을 설정된 최대/최소값 사이로 제한
    float ClampedPitch = FMath::Clamp(LookRotation.Pitch, MaxLookDownAngle, MaxLookUpAngle);

    // Yaw(좌우 회전)는 그대로, Roll(기울기)은 0으로 고정한 최종 회전값 반환
    FRotator ConstrainedRotation = FRotator(ClampedPitch, LookRotation.Yaw, 0.0f);

    return ConstrainedRotation;
}

// AI 상태 머신 업데이트 함수
void AEnemyShooterAIController::UpdateAIState()
{
    // 현재 상태에 따라 적절한 핸들러 함수 호출
    switch (CurrentState)
    {
    case EEnemyShooterAIState::Idle:         HandleIdleState();       break;
    case EEnemyShooterAIState::Detecting:    HandleDetectingState();  break;
    case EEnemyShooterAIState::Moving:       HandleMovingState();     break;
    case EEnemyShooterAIState::Shooting:     HandleShootingState();   break;
    case EEnemyShooterAIState::Retreating:   HandleRetreatingState(); break;
    }
}

// 대기 상태: 플레이어가 탐지 범위에 들어오면 Detecting 상태로 전환
void AEnemyShooterAIController::HandleIdleState()
{
    if (IsPlayerInDetectionRange())
    {
        CurrentState = EEnemyShooterAIState::Detecting;
    }
}

// 탐지 상태: 플레이어와의 거리를 기반으로 다음 행동(후퇴, 사격, 이동)을 결정
void AEnemyShooterAIController::HandleDetectingState()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter) return;

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
    // 최우선 순위: 이동 중이라도 사격이 가능해지면 즉시 사격 상태로 전환
    if (IsPlayerInShootRange() && HasClearShotToPlayer())
    {
        CurrentState = EEnemyShooterAIState::Shooting;
        StopMovementInput();
        return;
    }

    // 차순위: 다른 상태 전환 조건 확인
    if (IsPlayerTooClose())
    {
        CurrentState = EEnemyShooterAIState::Retreating;
        return;
    }
    if (!IsPlayerInDetectionRange())
    {
        CurrentState = EEnemyShooterAIState::Idle;
        StopMovementInput();
        return;
    }
    if (bIsBlockedByGuardian) // 가디언에 막혔다면 불필요한 이동 중지 (수류탄 각 재기)
    {
        StopMovementInput();
        return;
    }

    // 최후순위: 위의 모든 조건에 해당하지 않을 때만 이동 로직 수행
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
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter || !PlayerPawn) return;

    // 사선이 확보되지 않았을 경우 (핵심 전술 판단)
    if (!HasClearShotToPlayer())
    {
        // 만약 '가디언'에 의해 막혔다면, 수류탄 투척 시도
        if (bIsBlockedByGuardian)
        {
            if (GetWorld()->GetTimeSeconds() - LastGrenadeThrowTime < GrenadeCooldown) // 쿨타임 확인
            {
                StopMovementInput(); // 쿨타임 중에는 대기
                return;
            }

            TSubclassOf<AEnemyShooterGrenade> GrenadeToSpawn = Shooter->GrenadeClass; // 던질 수류탄 클래스 가져오기
            if (!GrenadeToSpawn) return;

            FVector StartLocation = Shooter->GetActorLocation() + FVector(0, 0, 100.0f); // 약간 위에서 발사

            // 수류탄 목표 지점 계산: 플레이어 위치보다 약간 '뒤'를 조준 (플레이어가 피할 것을 예측)
            FVector PlayerLocation = PlayerPawn->GetActorLocation();
            FVector DirectionToPlayer = PlayerLocation - StartLocation;
            DirectionToPlayer.Z = 0; // 수평 방향만 사용
            FVector EndLocation = PlayerLocation - (DirectionToPlayer.GetSafeNormal() * GrenadeTargetOffset);
            EndLocation.Z += 60.0f; // 약간의 높이 보정

            // 계산된 포물선 궤도에 따른 발사 속도 제안 받기
            FVector LaunchVelocity;
            bool bHaveAimSolution = UGameplayStatics::SuggestProjectileVelocity(
                this, LaunchVelocity, StartLocation, EndLocation, GrenadeLaunchSpeed, false, 0.0f, GetWorld()->GetGravityZ()
            );

            if (bHaveAimSolution) // 발사 각도가 나왔다면
            {
                FActorSpawnParameters SpawnParams;
                SpawnParams.Owner = GetPawn();
                SpawnParams.Instigator = GetPawn();

                // 수류탄 스폰 및 발사
                AEnemyShooterGrenade* Grenade = GetWorld()->SpawnActor<AEnemyShooterGrenade>(
                    GrenadeToSpawn, StartLocation, FRotator::ZeroRotator, SpawnParams
                );
                if (Grenade)
                {
                    Grenade->LaunchGrenade(LaunchVelocity);
                    LastGrenadeThrowTime = GetWorld()->GetTimeSeconds(); // 쿨타임 타이머 시작
                    Shooter->PlayThrowingGrenadeAnimation(); // 캐릭터의 투척 애니메이션 재생
                }
            }
            StopMovementInput();
            return;
        }

        // 가디언이 아닌 다른 이유로 사선이 막혔다면 다시 이동 상태로 전환
        CurrentState = EEnemyShooterAIState::Moving;
        return;
    }

    // 사선이 확보된 경우: 거리 재확인
    if (IsPlayerTooClose())
    {
        CurrentState = EEnemyShooterAIState::Retreating;
        return;
    }
    else if (!IsPlayerInShootRange())
    {
        CurrentState = IsPlayerInDetectionRange() ? EEnemyShooterAIState::Moving : EEnemyShooterAIState::Idle;
        StopMovementInput();
        return;
    }

    // 모든 조건이 만족하면 사격
    if (CanShoot() && !Shooter->bIsAttacking && !Shooter->bIsDead)
    {
        PerformShooting();
    }
}

// 후퇴 상태: 플레이어로부터 멀어지면서 거리를 벌림
void AEnemyShooterAIController::HandleRetreatingState()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter || !PlayerPawn) return;

    // 플레이어 반대 방향으로 이동
    FVector ToPlayer = PlayerPawn->GetActorLocation() - Shooter->GetActorLocation();
    FVector RetreatDir = -ToPlayer.GetSafeNormal(); // 방향 벡터를 반대로
    Shooter->AddMovementInput(RetreatDir, 1.0f);

    float Distance = GetDistanceToPlayer();
    // 충분히 거리를 벌렸고 사선이 확보되면 다시 사격 상태로
    if (Distance >= (MinShootDistance + RetreatBuffer) && Distance <= ShootRadius && HasClearShotToPlayer())
    {
        CurrentState = EEnemyShooterAIState::Shooting;
        StopMovementInput();
    }
    else if (Distance > ShootRadius && IsPlayerInDetectionRange()) // 너무 멀어졌으면 다시 이동 상태로
    {
        CurrentState = EEnemyShooterAIState::Moving;
    }
    else if (!IsPlayerInDetectionRange()) // 탐지 범위를 벗어나면 대기 상태로
    {
        CurrentState = EEnemyShooterAIState::Idle;
        StopMovementInput();
    }
}

// 자신의 진형 위치를 계산하고 갱신
void AEnemyShooterAIController::UpdateFormationPosition()
{
    AssignedPosition = CalculateFormationPosition();
    bHasAssignedPosition = true;
}

// 플레이어와 아군 위치를 기반으로 자신의 이상적인 진형 위치를 계산
FVector AEnemyShooterAIController::CalculateFormationPosition()
{
    if (!PlayerPawn || !GetPawn()) return GetPawn()->GetActorLocation();

    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    TArray<AActor*> AllAllies = GetAllAllies(); // 캐시된 아군 목록 가져오기

    // 자신을 포함한 전체 슈터 수 계산
    int32 TotalShooters = AllAllies.Num() + 1;
    // 고유한 인덱스 부여 (메모리 주소 비교로 순서 결정)
    int32 MyIndex = 0;
    for (int32 i = 0; i < AllAllies.Num(); i++)
    {
        if (GetPawn() > AllAllies[i])
        {
            MyIndex++;
        }
    }

    // 원형 진형 계산
    float AngleStep = 360.0f / TotalShooters; // 각 슈터가 차지할 각도
    float MyAngle = MyIndex * AngleStep; // 자신의 목표 각도

    // 플레이어 위치를 중심으로 원 위의 좌표 계산
    FVector ToFormationPos = FVector(
        FMath::Cos(FMath::DegreesToRadians(MyAngle)) * FormationRadius,
        FMath::Sin(FMath::DegreesToRadians(MyAngle)) * FormationRadius,
        0.0f
    );
    FVector IdealPosition = PlayerLocation + ToFormationPos;

    // 만약 계산된 위치에 이미 다른 아군이 있다면, 자리를 찾을 때까지 45도씩 회전하며 재탐색
    int32 Attempts = 0;
    while (IsPositionOccupied(IdealPosition) && Attempts < 8)
    {
        MyAngle += 45.0f;
        ToFormationPos = FVector(
            FMath::Cos(FMath::DegreesToRadians(MyAngle)) * FormationRadius,
            FMath::Sin(FMath::DegreesToRadians(MyAngle)) * FormationRadius,
            0.0f
        );
        IdealPosition = PlayerLocation + ToFormationPos;
        Attempts++;
    }

    return IdealPosition;
}

// 특정 위치가 다른 아군에 의해 점유되었는지 확인
bool AEnemyShooterAIController::IsPositionOccupied(FVector Position, float CheckRadius)
{
    TArray<AActor*> AllAllies = GetAllAllies();
    for (AActor* Ally : AllAllies)
    {
        if (FVector::Dist(Position, Ally->GetActorLocation()) < CheckRadius)
        {
            return true; // 점유됨
        }
    }
    return false; // 비어있음
}

// 할당된 진형 위치로 이동해야 하는지 판단
bool AEnemyShooterAIController::ShouldMoveToFormation() const
{
    if (!bHasAssignedPosition) return false;
    // 현재 위치가 목표 위치에서 200 유닛 이상 떨어져 있다면 이동 필요
    return FVector::Dist(GetPawn()->GetActorLocation(), AssignedPosition) > 200.0f;
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
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter) return;

    FVector ToTarget = Target - Shooter->GetActorLocation();
    FVector MoveDirection = ToTarget.GetSafeNormal();
    Shooter->AddMovementInput(MoveDirection, 1.0f);
}

// 유틸리티 함수들
float AEnemyShooterAIController::GetDistanceToPlayer() const
{
    if (!PlayerPawn || !GetPawn()) return FLT_MAX;
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
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter || !PlayerPawn) return;

    FVector ToPlayer = PlayerPawn->GetActorLocation() - Shooter->GetActorLocation();
    FVector MoveDirection = ToPlayer.GetSafeNormal();
    Shooter->AddMovementInput(MoveDirection, 1.0f);
}

//void AEnemyShooterAIController::RetreatFromPlayer()
//{
//    // 실제 이동 로직은 HandleRetreatingState에서 직접 처리됨
//}

void AEnemyShooterAIController::StopMovementInput()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (Shooter)
    {
        Shooter->GetCharacterMovement()->StopMovementImmediately();
    }
}

void AEnemyShooterAIController::PerformShooting()
{
    AEnemyShooter* Shooter = Cast<AEnemyShooter>(GetPawn());
    if (!Shooter) return;

    // Shooter->PlayShootingAnimation(); // 애니메이션 재생 방식 대신 직접 총 발사

    Shooter->Shoot(); // 캐릭터의 Shoot 함수 호출

    LastShootTime = GetWorld()->GetTimeSeconds(); // 마지막 사격 시간 기록
}

bool AEnemyShooterAIController::CanShoot() const
{
    return GetWorld()->GetTimeSeconds() - LastShootTime >= ShootCooldown; // 쿨타임이 지났는지 확인
}