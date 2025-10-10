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

    // 4방향 이동 패턴을 위한 타이머 설정 (현재는 사용되지 않음)
    GetWorld()->GetTimerManager().SetTimer(
        MoveTimerHandle, this, &AEnemyGuardianAIController::MoveInDirection, MoveDuration, true
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
}

// 아군 슈터를 보호하는 AI 행동 로직
void AEnemyGuardianAIController::PerformShooterProtection()
{
    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn());
    if (!Guardian) return;

    // 1. 월드에 있는 모든 살아있는 슈터를 수집
    TArray<AEnemyShooter*> AllShooters;
    for (TActorIterator<AEnemyShooter> It(GetWorld()); It; ++It)
    {
        AEnemyShooter* Shooter = *It;
        if (Shooter && !Shooter->bIsDead && !Shooter->bIsPlayingIntro)
        {
            AllShooters.Add(Shooter);
        }
    }

    if (AllShooters.Num() == 0) // 보호할 슈터가 없으면
    {
        // 플레이어를 바라보고 제자리에 서 있도록 함
        FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
        FRotator LookRotation = ToPlayer.Rotation();
        Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
        return;
    }

    // 2. 자신에게 가장 가까운 슈터를 찾음
    AEnemyShooter* NearestShooter = nullptr;
    float MinDistance = FLT_MAX;
    for (AEnemyShooter* Shooter : AllShooters)
    {
        float Distance = FVector::Dist(Guardian->GetActorLocation(), Shooter->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            NearestShooter = Shooter;
        }
    }
    if (!NearestShooter) return;

    // 3. 자신과 '같은 슈터'를 보호하려는 다른 모든 가디언을 찾음
    TArray<AEnemyGuardian*> GuardiansProtectingSameShooter;
    for (TActorIterator<AEnemyGuardian> It(GetWorld()); It; ++It)
    {
        AEnemyGuardian* OtherGuardian = *It;
        if (OtherGuardian && !OtherGuardian->bIsDead && !OtherGuardian->bIsPlayingIntro &&
            !OtherGuardian->bIsStunned && !OtherGuardian->bIsShieldDestroyed)
        {
            // 다른 가디언의 가장 가까운 슈터도 '나의 목표 슈터'와 같은지 확인
            AEnemyShooter* OtherNearestShooter = nullptr;
            float OtherMinDistance = FLT_MAX;
            for (AEnemyShooter* Shooter : AllShooters)
            {
                float Distance = FVector::Dist(OtherGuardian->GetActorLocation(), Shooter->GetActorLocation());
                if (Distance < OtherMinDistance)
                {
                    OtherMinDistance = Distance;
                    OtherNearestShooter = Shooter;
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

    // 5. 같은 조에 있는 가디언들을 정렬하여 고유한 인덱스 부여 (뭉침 방지)
    GuardiansProtectingSameShooter.Sort([](const AEnemyGuardian& A, const AEnemyGuardian& B) {
        return A.GetUniqueID() < B.GetUniqueID();
        });
    int32 MyIndex = GuardiansProtectingSameShooter.IndexOfByPredicate([Guardian](const AEnemyGuardian* G) {
        return G == Guardian;
        });

    // 6. 인덱스를 기반으로 기본 위치에서 좌/우로 분산된 최종 목표 위치 계산
    FVector FinalTargetLocation = BaseProtectionLocation;
    if (GuardiansProtectingSameShooter.Num() > 1 && MyIndex != INDEX_NONE)
    {
        float SpreadDistance = 80.0f; // 가디언 사이의 간격
        int32 TotalGuardians = GuardiansProtectingSameShooter.Num();
        // (자신의 인덱스) - (전체 인원의 중간값) 을 통해 중앙을 기준으로 한 자신의 상대 위치를 구함
        float OffsetFromCenter = (MyIndex - (TotalGuardians - 1) * 0.5f) * SpreadDistance;
        FinalTargetLocation += NearestShooter->GetActorRightVector() * OffsetFromCenter;
    }

    // 7. 항상 플레이어를 바라보도록 회전
    FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
    FRotator LookRotation = ToPlayer.Rotation();
    Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));

    // 8. 최종 목표 위치로 이동
    float DistanceToTarget = FVector::Dist(Guardian->GetActorLocation(), FinalTargetLocation);
    if (DistanceToTarget > MinDistanceToTarget) // 아직 목표 위치에 도달하지 못했다면
    {
        FVector DirectionToTarget = (FinalTargetLocation - Guardian->GetActorLocation()).GetSafeNormal();
        Guardian->AddMovementInput(DirectionToTarget, 1.0f); // 목표를 향해 이동
    }
}

// 플레이어를 포위하는 AI 행동 로직 (방패 파괴 후)
void AEnemyGuardianAIController::PerformSurroundMovement()
{
    AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(GetPawn());
    if (!Guardian) return;

    // 1. 월드에 있는 모든 살아있는 가디언 수집
    TArray<AActor*> AllGuardians;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyGuardian::StaticClass(), AllGuardians);

    // 그 중 실제로 활동 가능한(죽거나 스턴이 아닌) 가디언만 필터링
    TArray<AEnemyGuardian*> ActiveGuardians;
    for (AActor* Actor : AllGuardians)
    {
        AEnemyGuardian* OtherGuardian = Cast<AEnemyGuardian>(Actor);
        if (OtherGuardian && !OtherGuardian->bIsDead && !OtherGuardian->bIsPlayingIntro && !OtherGuardian->bIsStunned)
        {
            ActiveGuardians.Add(OtherGuardian);
        }
    }

    // 활동 가능한 가디언이 1명 뿐이면 단순하게 플레이어에게 접근
    if (ActiveGuardians.Num() <= 1)
    {
        FVector ToPlayer = PlayerPawn->GetActorLocation() - Guardian->GetActorLocation();
        FRotator LookRotation = ToPlayer.Rotation();
        Guardian->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
        MoveToActor(PlayerPawn, 50.0f);
        return;
    }

    // 2. 전체 가디언 목록에서 자신의 고유한 인덱스를 찾음
    int32 MyIndex = ActiveGuardians.IndexOfByKey(Guardian);
    if (MyIndex == INDEX_NONE)
    {
        MoveToActor(PlayerPawn, 50.0f); // 만약 못찾으면 그냥 플레이어에게 접근
        return;
    }

    // 3. 포위 위치 계산
    float AngleDeg = 360.0f / ActiveGuardians.Num(); // 360도를 가디언 수로 나누어 각도 간격 계산
    float MyAngleDeg = AngleDeg * MyIndex; // 자신의 인덱스에 따른 목표 각도
    float MyAngleRad = FMath::DegreesToRadians(MyAngleDeg); // 라디안으로 변환

    // 4. 플레이어를 중심으로 한 원형 포위 위치 계산
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

void AEnemyGuardianAIController::MoveInDirection()
{
    // 현재 사용되지 않는 4방향 이동 패턴 로직
    DirectionIndex = (DirectionIndex + 1) % 4;
}

void AEnemyGuardianAIController::StopAI() {}