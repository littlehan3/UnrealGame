#include "BossEnemyAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BossEnemy.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "NavigationSystem.h"           // UNavigationSystemV1을 위해 추가
#include "Components/CapsuleComponent.h" // UCapsuleComponent를 위해 추가

UBossEnemyAnimInstance::UBossEnemyAnimInstance()
{
    Speed = 0.0f;
    Direction = 0.0f;
    bIsDead = false;
}

void UBossEnemyAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    BossEnemyCharacter = Cast<ABossEnemy>(TryGetPawnOwner());
}

void UBossEnemyAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    Super::NativeUpdateAnimation(DeltaTime);

    if (!BossEnemyCharacter)
        BossEnemyCharacter = Cast<ABossEnemy>(TryGetPawnOwner());

    if (!BossEnemyCharacter) return;

    if (bIsDead)
    {
        Speed = 0.0f;
        return;
    }

    Speed = BossEnemyCharacter->GetVelocity().Size();
    Direction = CalculateDirection(BossEnemyCharacter->GetVelocity(), BossEnemyCharacter->GetActorRotation());

    // 플레이어 바라보기 업데이트
    if (bShouldLookAtPlayer)
    {
        UpdateLookAtRotation(DeltaTime);
    }
}

void UBossEnemyAnimInstance::UpdateLookAtRotation(float DeltaTime)
{
    APawn* OwnerPawn = TryGetPawnOwner();
    if (!OwnerPawn) return;

    // 플레이어 찾기
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(OwnerPawn->GetWorld(), 0);
    if (!PlayerPawn) return;

    // 목표 회전 계산
    FVector ToPlayer = PlayerPawn->GetActorLocation() - OwnerPawn->GetActorLocation();
    ToPlayer.Z = 0.0f; // 수평 회전만
    FRotator TargetRotation = ToPlayer.Rotation();

    // 부드러운 회전 적용
    FRotator CurrentRotation = OwnerPawn->GetActorRotation();
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, LookAtSpeed);

    OwnerPawn->SetActorRotation(NewRotation);
    LookAtRotation = NewRotation;
}

float UBossEnemyAnimInstance::CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation)
{
    if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER) return 0.0f;

    FVector ForwardVector = BaseRotation.Vector(); // 캐릭터 현재 회전값에서 정면 방향 벡터를 가져옴
    FVector RightVector = FRotationMatrix(BaseRotation).GetScaledAxis(EAxis::Y); // 캐틱터 현재 회전값 기준 오른쪽 방향 벡터를 가져옴
    FVector NormalizedVelocity = Velocity.GetSafeNormal2D(); // 이동속도벡터를 정규화해서 평면 기준의 방향을 얻음
    float ForwardDot = FVector::DotProduct(ForwardVector, NormalizedVelocity); // 정면 방향 벡터와 이동방향 벡터 내적을 계산해서 1이면 정면 -1이면 뒤쪽 이동
    float RightDot = FVector::DotProduct(RightVector, NormalizedVelocity); // 오른쪽 방향 벡터와 이동방향 벡터 내적을 계산해서 1이면 오른쪽 -1이면 왼쪽 이동
    float Angle = FMath::Atan2(RightDot, ForwardDot); // 이동방향 기준으로 캐릭터 정면 방향과의 각도 계산 (라디안단위)
    return FMath::RadiansToDegrees(Angle); // 계산된 라디안 각도를 도 단위로 변환하여 반환
}
