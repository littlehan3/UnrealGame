#include "EnemyDroneAIController.h"
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수 사용
#include "GameFramework/Character.h" // ACharacter 클래스 참조
#include "EnemyDrone.h" // 제어할 대상인 EnemyDrone 클래스 참조

AEnemyDroneAIController::AEnemyDroneAIController()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화
}

void AEnemyDroneAIController::BeginPlay()
{
	UWorld* World = GetWorld();
	if (!World) return;
    Super::BeginPlay(); // 부모 클래스 BeginPlay 호출
    PlayerActor = UGameplayStatics::GetPlayerCharacter(World, 0); // 플레이어 액터 찾아 저장
}

void AEnemyDroneAIController::Tick(float DeltaTime)
{
	APawn* DronePawn = GetPawn(); // 제어 중인 폰(드론) 참조
	if (!IsValid(DronePawn) || !IsValid(PlayerActor)) return; // 플레이어나 제어할 폰이 없으면 로직 중단

    Super::Tick(DeltaTime); // 부모 클래스 Tick 호출

    const FVector DroneLocation = DronePawn->GetActorLocation(); // 현재 드론 위치
    const FVector PlayerLocation = PlayerActor->GetActorLocation(); // 현재 플레이어 위치
	const UWorld* World = GetWorld(); // const로 월드를 고정하여 참조

    // 궤도 이탈 감지 및 회피 로직
    float DistFromPlayer = FVector::Dist2D(DroneLocation, PlayerLocation); // 플레이어와의 수평 거리 계산
    // 현재 거리가 목표 궤도 반경에서 허용 오차 이상으로 벗어났는지 확인
    if (DistFromPlayer > OrbitRadius + RadiusTolerance || DistFromPlayer < OrbitRadius - RadiusTolerance)
    {
        TimeOutOfRadius += DeltaTime; // 벗어난 시간 누적
        if (TimeOutOfRadius >= OutOfRadiusLimit) // 일정 시간 이상 벗어나 있었다면
        {
            TargetHeight = DroneLocation.Z + EscapeHight; // 현재 고도보다 탈출시 상승할 고도 만큼 높은 곳을 목표로 설정
            bRising = true; // 긴급 상승 상태로 전환
            TimeOutOfRadius = 0.0f; // 타이머 초기화
        }
    }
    else
    {
        TimeOutOfRadius = 0.0f; // 궤도 안에 있다면 타이머 초기화
    }

    // 긴급 상승 로직
    if (bRising) // 긴급 상승 상태일 경우
    {
        FVector MoveUpLocation = DroneLocation; // 현재 위치에서 Z값만 변경
        MoveUpLocation.Z = FMath::FInterpTo(DroneLocation.Z, TargetHeight, DeltaTime, DefaultMoveInterpSpeed); // 목표 높이까지 부드럽게 상승
        DronePawn->SetActorLocation(MoveUpLocation, true); // 위치 업데이트

        if (FMath::IsNearlyEqual(MoveUpLocation.Z, TargetHeight, HeightArrivalTolerance)) // 목표 높이에 거의 도달했다면
        {
            bRising = false; // 상승 상태 종료
        }
    }

    // 궤도 비행 로직
    float Direction = bClockwise ? 1.0f : -1.0f; // 회전 방향 결정
    CurrentAngle += OrbitSpeed * DeltaTime * Direction; // 시간에 따라 현재 각도 업데이트
    if (CurrentAngle >= 360.f) CurrentAngle -= 360.f; // 360도 넘어가면 0으로
    if (CurrentAngle < 0.0f) CurrentAngle += 360.f; // 0도 미만이면 360으로

    float Radian = FMath::DegreesToRadians(CurrentAngle); // 각도를 라디안으로 변환
    // 원 궤적 계산: 플레이어 위치를 중심으로 (cos, sin)을 이용해 원 위의 좌표를 구함
    FVector Offset = FVector(FMath::Cos(Radian) * OrbitRadius, FMath::Sin(Radian) * OrbitRadius, HeightOffset);
    FVector TargetLocation = PlayerLocation + Offset; // 다음 프레임의 목표 위치

    // 장애물 감지 로직 (라인 트레이스)
	FCollisionQueryParams QueryParams; // 트레이스 쿼리 파라미터 설정
    QueryParams.AddIgnoredActor(DronePawn); // 자기 자신은 무시
	FHitResult HitResult; // 히트 결과 저장용
    // 현재 위치에서 목표 위치까지 라인 트레이스를 쏴서 장애물이 있는지 확인
    if (World->LineTraceSingleByChannel(HitResult, DroneLocation, TargetLocation, ECC_Visibility, QueryParams))
    {
		AActor* HitActor = HitResult.GetActor(); // 충돌한 액터 가져오기
		if (HitActor && HitActor != DronePawn) // 자기 자신이 아닌 다른 액터와 충돌했다면
        {
            if (HitActor->IsA(AEnemyDrone::StaticClass())) // 그중 다른 드론에 부딪혔다면
            {
                float RandomSign = (FMath::RandBool()) ? 1.0f : -1.0f; // 위 또는 아래 랜덤
                HeightOffset = FMath::Clamp(HeightOffset + DroneAvoidanceStep * RandomSign, MinFlightHeight, MaxFlightHeight); // 고도를 랜덤하게 변경하여 회피
            }
            else // 벽이나 건물 등 다른 장애물에 부딪혔다면
            {
                bClockwise = !bClockwise; // 회전 방향을 반대로 전환
                if (bTriedReverse) // 이미 한 번 방향을 바꿨는데 또 부딪혔다면 (끼임 상태로 간주)
                {
                    TimeStuck += DeltaTime; // 끼인 시간 누적
                    if (TimeStuck >= MaxStuckTime) // 일정 시간 이상 끼어있다면
                    {
                        HeightOffset = BaseHeight + EscapeHight; // 고도를 크게 높여서 탈출 시도
                        TimeStuck = 0.0f;
                        bTriedReverse = false;
                    }
                }
                else // 처음 부딪혔다면
                {
                    bTriedReverse = true; // 방향을 바꿨다고 기록
                    TimeStuck = 0.0f; // 끼인 시간 초기화
                }
            }
            /*DrawDebugLine(GetWorld(), DroneLocation, HitResult.ImpactPoint, FColor::Green, false, 0.1f, 0, 1.f);*/
        }
    }
    else // 장애물이 없다면
    {
        // 서서히 기본 고도로 복귀
        HeightOffset = FMath::FInterpTo(HeightOffset, BaseHeight, DeltaTime, HeightReturnInterpSpeed);
        // 끼임 관련 상태 변수들 초기화
        bTriedReverse = false;
        TimeStuck = 0.0f;
    }

    // 최종 이동 및 회전
    FVector NewLocation = FMath::VInterpTo(DroneLocation, TargetLocation, DeltaTime, MoveInterpSpeed); // 계산된 목표 위치로 부드럽게 이동
    DronePawn->SetActorLocation(NewLocation, true);

    // 항상 플레이어를 바라보도록 회전
	FVector ToPlayer = PlayerLocation - NewLocation; // 플레이어를 향하는 벡터
	FRotator LookAtRotation = FRotationMatrix::MakeFromX(ToPlayer).Rotator(); // 플레이어를 향하는 회전 계산
	DronePawn->SetActorRotation(FMath::RInterpTo(DronePawn->GetActorRotation(), LookAtRotation, DeltaTime, RotationInterpSpeed)); // 부드럽게 회전
}