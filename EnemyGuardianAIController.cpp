#include "EnemyGuardianAIController.h"
#include "EnemyGuardian.h" // 제어할 대상
#include "EnemyShooter.h" // 보호할 대상
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수
#include "TimerManager.h" // FTimerManager 사용
#include "GameFramework/Character.h" // ACharacter 클래스 참조
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트

AEnemyGuardianAIController::AEnemyGuardianAIController()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화
}

void AEnemyGuardianAIController::BeginPlay()
{
    Super::BeginPlay(); // 부모 클래스 BeginPlay 호출
    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // 플레이어 폰 찾아 저장

    // 게임 시작 시 한 번, 그리고 AllyCacheUpdateInterval(2초)마다 주기적으로 UpdateAllyCaches 함수를 호출하도록 타이머 설정
    GetWorld()->GetTimerManager().SetTimer(
        AllyCacheUpdateTimerHandle,
        this,
        &AEnemyGuardianAIController::UpdateAllyCaches,
        AllyCacheUpdateInterval,
        true, // 반복 실행
        0.1f  // 초기 지연 (모든 액터가 스폰될 시간을 줌)
    );
}

void AEnemyGuardianAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // 부모 클래스 Tick 호출

    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn()); // 제어 중인 폰 가져오기
    // 폰이 없거나, 죽었거나, 등장 중이거나, 스턴 상태일 때는 아무런 로직도 실행하지 않음
    if (!Guardian || Guardian->bIsDead || Guardian->bIsPlayingIntro || Guardian->bIsStunned)
        return;

    if (!PlayerPawn) // 플레이어 폰 참조가 없다면
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // 다시 찾아봄
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
                FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
                FRotator LookRotation = ToPlayer.Rotation();
                Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
                // 방패 공격 애니메이션을 재생
                Guardian->PlayShieldAttackAnimation();
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
                FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
                FRotator LookRotation = ToPlayer.Rotation();
                Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
                // 진압봉 공격 애니메이션을 재생
                Guardian->PlayNormalAttackAnimation();
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

    // EnemyAnimInstance를 가져와 현재 HitReactionMontage를 재생 중인지 확인합니다.
    UEnemyGuardianAnimInstance* AnimInstance = Cast<UEnemyGuardianAnimInstance>(Guardian->GetMesh()->GetAnimInstance());
    if (AnimInstance && Guardian->HitMontage && AnimInstance->Montage_IsPlaying(Guardian->HitMontage))
    {
        StopMovement();
        return;
    }
}

// 주기적으로 호출되어 아군 목록을 갱신하고 캐시에 저장하는 함수
void AEnemyGuardianAIController::UpdateAllyCaches()
{
    // 기존 캐시 초기화
    CachedShooters.Empty();
    CachedGuardians.Empty();

    // 월드에 있는 모든 슈터를 찾아 유효한 경우 캐시에 추가
    for (TActorIterator<AEnemyShooter> It(GetWorld()); It; ++It)
    {
        AEnemyShooter* Shooter = *It;
        if (Shooter && !Shooter->bIsDead && !Shooter->bIsPlayingIntro)
        {
            CachedShooters.Add(Shooter);
        }
    }

    // 월드에 있는 모든 가디언을 찾아 유효한 경우 캐시에 추가
    for (TActorIterator<AEnemyGuardian> It(GetWorld()); It; ++It)
    {
        AEnemyGuardian* Guardian = *It;
        if (Guardian && !Guardian->bIsDead && !Guardian->bIsPlayingIntro && !Guardian->bIsStunned)
        {
            CachedGuardians.Add(Guardian);
        }
    }
}

// 아군 슈터를 보호하는 AI 행동 로직
void AEnemyGuardianAIController::PerformShooterProtection()
{
    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn());
    if (!Guardian) return;

    // 1. 캐시된 슈터 목록이 비어있으면 보호 행동 중단
    if (CachedShooters.Num() == 0)
    {
        FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
        FRotator LookRotation = ToPlayer.Rotation();
        Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
        return;
    }

    // 2. 자신에게 가장 가까운 슈터를 캐시된 목록에서 찾음
    AEnemyShooter* NearestShooter = nullptr;
    float MinDistance = FLT_MAX;
    for (const auto& WeakShooter : CachedShooters)
    {
        if (WeakShooter.IsValid()) // 약한 참조가 유효한지 확인
        {
            AEnemyShooter* Shooter = WeakShooter.Get();
            float Distance = FVector::Dist(Guardian->GetActorLocation(), Shooter->GetActorLocation());
            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                NearestShooter = Shooter;
            }
        }
    }
    if (!NearestShooter) return; // 유효한 슈터를 찾지 못했다면 중단

    // 3. 자신과 '같은 슈터'를 보호하려는 다른 모든 가디언을 캐시된 목록에서 찾음
    TArray<AEnemyGuardian*> GuardiansProtectingSameShooter;
    for (const auto& WeakGuardian : CachedGuardians)
    {
        if (WeakGuardian.IsValid())
        {
            AEnemyGuardian* OtherGuardian = WeakGuardian.Get();
            // 방패가 파괴된 가디언은 보호 임무에서 제외
            if (OtherGuardian->bIsShieldDestroyed) continue;

            // 다른 가디언의 가장 가까운 슈터도 '나의 목표 슈터'와 같은지 확인
            AEnemyShooter* OtherNearestShooter = nullptr;
            float OtherMinDistance = FLT_MAX;
            for (const auto& WeakShooter : CachedShooters)
            {
                if (WeakShooter.IsValid())
                {
                    AEnemyShooter* Shooter = WeakShooter.Get();
                    float Distance = FVector::Dist(OtherGuardian->GetActorLocation(), Shooter->GetActorLocation());
                    if (Distance < OtherMinDistance)
                    {
                        OtherMinDistance = Distance;
                        OtherNearestShooter = Shooter;
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
    FVector ShooterForward = NearestShooter->GetActorForwardVector();
    FVector BaseProtectionLocation = NearestShooter->GetActorLocation() + ShooterForward * ProtectionDistance;

    // 5. 같은 조에 있는 가디언들을 정렬하여 고유한 인덱스 부여
    GuardiansProtectingSameShooter.Sort([](const AEnemyGuardian& A, const AEnemyGuardian& B) {
        return A.GetUniqueID() < B.GetUniqueID();
        });
    int32 MyIndex = GuardiansProtectingSameShooter.IndexOfByPredicate([Guardian](const AEnemyGuardian* G) {
        return G == Guardian;
        });

    // 6. 인덱스를 기반으로 최종 목표 위치 계산
    FVector FinalTargetLocation = BaseProtectionLocation;
    if (GuardiansProtectingSameShooter.Num() > 1 && MyIndex != INDEX_NONE)
    {
        float SpreadDistance = 80.0f; // 가디언 사이의 간격
        int32 TotalGuardians = GuardiansProtectingSameShooter.Num();
        float OffsetFromCenter = (MyIndex - (TotalGuardians - 1) * 0.5f) * SpreadDistance;
        FinalTargetLocation += NearestShooter->GetActorRightVector() * OffsetFromCenter;
    }

    // 7. 항상 플레이어를 바라보도록 회전
    FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
    FRotator LookRotation = ToPlayer.Rotation();
    Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));

    // 8. 최종 목표 위치로 이동
    float DistanceToTarget = FVector::Dist(Guardian->GetActorLocation(), FinalTargetLocation);
    if (DistanceToTarget > MinDistanceToTarget)
    {
        FVector DirectionToTarget = (FinalTargetLocation - Guardian->GetActorLocation()).GetSafeNormal();
        Guardian->AddMovementInput(DirectionToTarget, 1.0f);
    }
}

// 플레이어를 포위하는 AI 행동 로직 (방패 파괴 후)
void AEnemyGuardianAIController::PerformSurroundMovement()
{
    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn());
    if (!Guardian) return;

    // 1. 활동 가능한(죽거나 스턴이 아닌) 가디언만 캐시에서 필터링
    TArray<AEnemyGuardian*> ActiveGuardians;
    for (const auto& WeakGuardian : CachedGuardians)
    {
        if (WeakGuardian.IsValid())
        {
            // 방패가 파괴된 가디언만 포위 기동에 참여
            if (WeakGuardian.Get()->bIsShieldDestroyed)
            {
                ActiveGuardians.Add(WeakGuardian.Get());
            }
        }
    }
    // 자기 자신 추가 (캐시에는 방패 파괴 전의 자신이 포함되지 않을 수 있으므로)
    if (!ActiveGuardians.Contains(Guardian))
    {
        ActiveGuardians.Add(Guardian);
    }


    // 2. 활동 가능한 가디언이 1명 뿐이면 단순하게 플레이어에게 접근
    if (ActiveGuardians.Num() <= 1)
    {
        MoveToActor(PlayerPawn, 50.0f);
        return;
    }

    // 3. 전체 가디언 목록에서 자신의 고유한 인덱스를 찾음
    int32 MyIndex = ActiveGuardians.IndexOfByKey(Guardian);
    if (MyIndex == INDEX_NONE)
    {
        MoveToActor(PlayerPawn, 50.0f);
        return;
    }

    // 4. 포위 위치 계산
    float AngleDeg = 360.0f / ActiveGuardians.Num();
    float MyAngleDeg = AngleDeg * MyIndex;
    float MyAngleRad = FMath::DegreesToRadians(MyAngleDeg);
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    FVector Offset = FVector(FMath::Cos(MyAngleRad), FMath::Sin(MyAngleRad), 0) * SurroundRadius;
    FVector TargetLocation = PlayerLocation + Offset;

    // 5. 항상 플레이어를 바라보도록 회전
    FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
    FRotator LookRotation = ToPlayer.Rotation();
    Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));

    // 6. 계산된 포위 위치로 이동
    MoveToLocation(TargetLocation, 50.0f);
}

void AEnemyGuardianAIController::StopAI() 
{
    // AI 동작 중지 로직
    GetWorld()->GetTimerManager().ClearTimer(AllyCacheUpdateTimerHandle); // 타이머 정리
    StopMovement();
}