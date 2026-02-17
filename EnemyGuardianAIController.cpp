#include "EnemyGuardianAIController.h"
#include "EnemyGuardian.h" // 제어할 대상
#include "EnemyShooter.h" // 보호할 대상
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수
#include "TimerManager.h" // FTimerManager 사용
#include "GameFramework/Character.h" // ACharacter 클래스 참조
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트
#include "EnemyGuardianAnimInstance.h"
#include "EngineUtils.h" // TActorIterator 사용

AEnemyGuardianAIController::AEnemyGuardianAIController()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화
}

void AEnemyGuardianAIController::BeginPlay()
{
    UWorld* World = GetWorld();
    if (!World) return;
    Super::BeginPlay(); // 부모 클래스 BeginPlay 호출
    PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0); // 플레이어 폰 찾아 저장

    // 게임 시작 시 한 번, 그리고 AllyCacheUpdateInterval(2초)마다 주기적으로 UpdateAllyCaches 함수를 호출하도록 타이머 설정
    TWeakObjectPtr<AEnemyGuardianAIController> WeakThis(this);
    World->GetTimerManager().SetTimer(
        AllyCacheUpdateTimerHandle,
        [WeakThis]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->UpdateAllyCaches();
            }
        },
        AllyCacheUpdateInterval, true, 0.1f  // 반복, 초기 지연 (모든 액터가 스폰될 시간을 줌)
    );
}

void AEnemyGuardianAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // 부모 클래스 Tick 호출

    UWorld* World = GetWorld();
    if (!World) return;

    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn()); // 제어 중인 폰 가져오기
    if (!Guardian) return;
    UEnemyGuardianAnimInstance* AnimInstance = Cast<UEnemyGuardianAnimInstance>(Guardian->GetMesh()->GetAnimInstance());
    if (!AnimInstance) return;
    // 폰이 없거나, 죽었거나, 등장 중이거나, 스턴 상태일 때는 아무런 로직도 실행하지 않음
    if (Guardian->bIsDead || Guardian->bIsPlayingIntro || Guardian->bIsStunned)
        return;
   
    if (!PlayerPawn) // 플레이어 폰 참조가 없다면
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0); // 다시 찾아봄
        if (!PlayerPawn) return; // 그래도 없으면 로직 중단
    }

    // 공격 중일 때는 다른 모든 행동을 중단하고 이동을 멈춤
    if (Guardian->bIsShieldAttacking || Guardian->bIsBatonAttacking)
    {
        StopMovement();
        return;
    }

    // 방패가 파괴되지 않았을 때: 플레이어가 공격 범위에 들어오면 방패 공격 시도
    if (!Guardian->bIsShieldDestroyed && Guardian->EquippedShield)
    {
        float DistanceToPlayer = FVector::Dist(Guardian->GetActorLocation(), PlayerPawn->GetActorLocation());
        if (DistanceToPlayer <= Guardian->ShieldAttackRadius)
        {
            if (!Guardian->bIsShieldAttacking) // 아직 공격 중이 아니라면
            {
                StopMovement(); // 이동을 멈추고
                // 플레이어를 바라본 뒤
                FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation(); // 방향 벡터 계산
                FRotator LookRotation = ToPlayer.Rotation(); // 회전 계산
                Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f)); // 수평 회전만 적용
                Guardian->PlayShieldAttackAnimation(); // 방패 공격 애니메이션 재생
            }
            return; // 공격 범위 내에서는 다른 이동 로직을 실행하지 않음
        }
    }

    // 방패가 파괴된 후: 플레이어가 공격 범위에 들어오면 진압봉 공격 시도
    if (Guardian->bIsShieldDestroyed && Guardian->EquippedBaton)
    {
        float DistanceToPlayer = FVector::Dist(Guardian->GetActorLocation(), PlayerPawn->GetActorLocation());
        if (DistanceToPlayer <= Guardian->BatonAttackRadius)
        {
            if (!Guardian->bIsBatonAttacking) // 아직 공격 중이 아니라면
            {
                StopMovement(); // 이동을 멈추고
                // 플레이어를 바라본 뒤
                FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation(); // 방향 벡터 계산
                FRotator LookRotation = ToPlayer.Rotation(); // 회전 계산
                Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f)); // 수평 회전만 적용
                Guardian->PlayNormalAttackAnimation(); // 진압봉 공격 애니메이션 재생
            }
            return; // 공격 범위 내에서는 다른 이동 로직을 실행하지 않음
        }
    }

    // 공격 중이 아닐 때, 가디언의 상태에 따라 이동 패턴 결정
    if (!Guardian->bIsShieldDestroyed && Guardian->EquippedShield)
    {
        PerformShooterProtection(); // 방패가 있으면 슈터 보호
    }
    else
    {
        PerformSurroundMovement(); // 방패가 없으면 플레이어 포위
    }

    if (Guardian->HitMontage && AnimInstance->Montage_IsPlaying(Guardian->HitMontage)) // 피격 몽타주 재생 중이면
    {
        StopMovement(); // 이동 중지
        return;
    }
}

// 주기적으로 호출되어 아군 목록을 갱신하고 캐시에 저장하는 함수
void AEnemyGuardianAIController::UpdateAllyCaches()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 기존 캐시 초기화
    CachedShooters.Empty();
    CachedGuardians.Empty();

    // 월드에 있는 모든 슈터를 찾아 유효한 경우 캐시에 추가
    for (TActorIterator<AEnemyShooter> It(World); It; ++It) // TActorIterator 사용
    {
        AEnemyShooter* Shooter = *It; // 현재 반복 중인 슈터
        if (Shooter && !Shooter->bIsDead && !Shooter->bIsPlayingIntro) // 유효한 슈터인지 확인
        {
            CachedShooters.Add(Shooter); // 캐시에 추가
        }
    }

    // 월드에 있는 모든 가디언을 찾아 유효한 경우 캐시에 추가
    for (TActorIterator<AEnemyGuardian> It(World); It; ++It) // TActorIterator 사용
    {
        AEnemyGuardian* Guardian = *It; // 현재 반복 중인 가디언
        if (Guardian && !Guardian->bIsDead && !Guardian->bIsPlayingIntro && !Guardian->bIsStunned) // 유효한 가디언인지 확인
        {
            CachedGuardians.Add(Guardian); // 캐시에 추가
        }
    }
}

// 아군 슈터를 보호하는 AI 행동 로직
void AEnemyGuardianAIController::PerformShooterProtection()
{
    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn());
    if (!Guardian) return;

    // 1. 캐시된 슈터 목록이 비어있으면 보호 행동 중단하고 캐릭터만 바라봄
    if (CachedShooters.Num() == 0)
    {
        FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation(); // 방향 벡터 계산
        FRotator LookRotation = ToPlayer.Rotation(); // 회전 계산
        Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f)); // 수평 회전만 적용
        return;
    }

    // 2. 자신에게 가장 가까운 슈터를 캐시된 목록에서 찾음
    AEnemyShooter* NearestShooter = nullptr; // 가장 가까운 슈터 포인터
    float MinDistance = FLT_MAX; // 최소 거리 초기화
    for (const auto& WeakShooter : CachedShooters) // 캐시된 슈터 목록 순회
    {
        if (WeakShooter.IsValid()) // 약한 참조가 유효한지 확인
        {
            AEnemyShooter* Shooter = WeakShooter.Get(); // 약한 참조에서 실제 포인터 가져오기
            float Distance = FVector::Dist(Guardian->GetActorLocation(), Shooter->GetActorLocation()); // 거리 계산
            if (Distance < MinDistance) // 최소 거리 갱신
            {
                MinDistance = Distance; // 최소 거리 업데이트
                NearestShooter = Shooter; // 가장 가까운 슈터 업데이트
            }
        }
    }
    if (!NearestShooter) return; // 유효한 슈터를 찾지 못했다면 중단

    // 3. 동일한 슈터를 보호하려는 다른 모든 가디언을 캐시된 목록에서 찾음
    TArray<AEnemyGuardian*> GuardiansProtectingSameShooter; // 같은 슈터를 보호하는 가디언들
    for (const auto& WeakGuardian : CachedGuardians) // 캐시된 가디언 목록 순회
    {
        if (WeakGuardian.IsValid()) // 약한 참조가 유효한지 확인
        {
            AEnemyGuardian* OtherGuardian = WeakGuardian.Get(); // 실제 포인터 가져오기
            if (OtherGuardian->bIsShieldDestroyed) continue; // 방패 파괴된 가디언은 보호를 하지않음

            // 다른 가디언의 가장 가까운 슈터도 나의 목표 슈'와 같은지 확인
            AEnemyShooter* OtherNearestShooter = nullptr; // 다른 가디언의 가장 가까운 슈터
            float OtherMinDistance = FLT_MAX; // 최소 거리 초기화
            for (const auto& WeakShooter : CachedShooters) // 캐시된 슈터 목록 순회
            {
                if (WeakShooter.IsValid()) // 약한 참조가 유효한지 확인
                {
                    AEnemyShooter* Shooter = WeakShooter.Get(); // 실제 포인터 가져오기
                    float Distance = FVector::Dist(OtherGuardian->GetActorLocation(), Shooter->GetActorLocation()); // 거리 계산
                    if (Distance < OtherMinDistance) // 최소 거리 갱신
                    {
                        OtherMinDistance = Distance; // 최소 거리 업데이트
                        OtherNearestShooter = Shooter; // 가장 가까운 슈터 업데이트
                    }
                }
            }
            if (OtherNearestShooter == NearestShooter) // 목표가 같다면
            {
                GuardiansProtectingSameShooter.Add(OtherGuardian); // 같은 조로 편성
            }
        }
    }

    // 4. 기본 보호 위치 계산 (가장 가까운 슈터의 정면)
    FVector ShooterForward = NearestShooter->GetActorForwardVector(); // 슈터의 정면 방향
    FVector BaseProtectionLocation = NearestShooter->GetActorLocation() + ShooterForward * ProtectionDistance; // 기본 보호 위치

    // 5. 같은 조에 있는 가디언들을 정렬하여 고유한 인덱스 부여
    GuardiansProtectingSameShooter.Sort([](const AEnemyGuardian& A, const AEnemyGuardian& B) { // 고유 ID 기준 정렬
        return A.GetUniqueID() < B.GetUniqueID(); // 오름차순
        }); // 람다 함수 종료
    int32 MyIndex = GuardiansProtectingSameShooter.IndexOfByPredicate([Guardian](const AEnemyGuardian* G) { // 자신의 인덱스 찾기
        return G == Guardian; // 일치하는 가디언 찾기
        });

    // 6. 인덱스를 기반으로 최종 목표 위치 계산
    FVector FinalTargetLocation = BaseProtectionLocation; // 최종 목표 위치 초기화
    if (GuardiansProtectingSameShooter.Num() > 1 && MyIndex != INDEX_NONE) // 여러 가디언이 같은 슈터를 보호할 때
    {
        float SpreadDistance = 80.0f; // 가디언 사이의 간격
        int32 TotalGuardians = GuardiansProtectingSameShooter.Num(); // 같은 슈터를 보호하는 가디언 수
        float OffsetFromCenter = (MyIndex - (TotalGuardians - 1) * 0.5f) * SpreadDistance; // 중앙에서의 오프셋 계산
        FinalTargetLocation += NearestShooter->GetActorRightVector() * OffsetFromCenter; // 오른쪽 방향으로 오프셋 적용
    }

    // 7. 항상 플레이어를 바라보도록 회전
    FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation(); // 방향 벡터 계산
    FRotator LookRotation = ToPlayer.Rotation(); // 회전 계산
    Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f)); // 수평 회전만 적용

    // 8. 최종 목표 위치로 이동
    float DistanceToTarget = FVector::Dist(Guardian->GetActorLocation(), FinalTargetLocation); // 현재 위치와 목표 위치 간 거리 계산
    if (DistanceToTarget > MinDistanceToTarget) // 충분히 멀리 떨어져 있으면 이동
    {
        FVector DirectionToTarget = (FinalTargetLocation - Guardian->GetActorLocation()).GetSafeNormal(); // 목표 방향 단위 벡터 계산
        Guardian->AddMovementInput(DirectionToTarget, 1.0f); // 이동 입력 추가
    }
}

// 플레이어를 포위하는 AI 행동 로직 (방패 파괴 후)
void AEnemyGuardianAIController::PerformSurroundMovement()
{
    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn());
    if (!Guardian) return;

    // 1. 활동 가능한(죽거나 스턴이 아닌) 가디언만 캐시에서 필터링
    TArray<AEnemyGuardian*> ActiveGuardians; // 활동 중인 가디언들
    for (const auto& WeakGuardian : CachedGuardians) // 캐시된 가디언들 순회
    {
        if (WeakGuardian.IsValid()) // 약한 참조가 유효한지 확인
        {
            // 방패가 파괴된 가디언만 포위 기동에 참여
            if (WeakGuardian.Get()->bIsShieldDestroyed) // 방패 파괴 확인
            {
                ActiveGuardians.Add(WeakGuardian.Get()); // 활동 중인 가디언 목록에 추가
            }
        }
    }
    // 자기 자신 추가 (캐시에는 방패 파괴 전의 자신이 포함되지 않을 수 있으므로)
    if (!ActiveGuardians.Contains(Guardian)) // 자기 자신이 목록에 없으면 추가
    {
        ActiveGuardians.Add(Guardian); // 자기 자신 추가
    }

    // 2. 활동 가능한 가디언이 1명 뿐이면 단순하게 플레이어에게 접근
    if (ActiveGuardians.Num() <= 1) // 한 명 뿐이면
    {
        MoveToActor(PlayerPawn, 50.0f); // 플레이어에게 접근
        return;
    }

    // 3. 전체 가디언 목록에서 자신의 고유한 인덱스를 찾음
    int32 MyIndex = ActiveGuardians.IndexOfByKey(Guardian); // 자신의 인덱스 찾기
    if (MyIndex == INDEX_NONE) // 인덱스를 찾지 못했으면
    {
        MoveToActor(PlayerPawn, 50.0f); // 플레이어에게 접근
        return;
    }

    // 4. 포위 위치 계산
    float AngleDeg = 360.0f / ActiveGuardians.Num(); // 각 가디언이 차지할 각도
    float MyAngleDeg = AngleDeg * MyIndex; // 자신의 각도 계산
    float MyAngleRad = FMath::DegreesToRadians(MyAngleDeg); // 라디안으로 변환
    FVector PlayerLocation = PlayerPawn->GetActorLocation(); // 플레이어 위치
    FVector Offset = FVector(FMath::Cos(MyAngleRad), FMath::Sin(MyAngleRad), 0) * SurroundRadius; // 오프셋 계산
    FVector TargetLocation = PlayerLocation + Offset; // 포위 목표 위치 계산

    // 5. 항상 플레이어를 바라보도록 회전
    FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation(); // 방향 벡터 계산
    FRotator LookRotation = ToPlayer.Rotation(); // 회전 계산
    Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f)); // 수평 회전만 적용

    // 6. 계산된 포위 위치로 이동
    MoveToLocation(TargetLocation, 50.0f); // 목표 위치로 이동
}

void AEnemyGuardianAIController::StopAI()
{
    UWorld* World = GetWorld();
    if (!World) return;
    World->GetTimerManager().ClearTimer(AllyCacheUpdateTimerHandle); // 타이머 정리
    StopMovement(); // 이동 중지
}
