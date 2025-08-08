#include "EnemyDogAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnemyDog.h"
#include "Kismet/KismetMathLibrary.h"

UEnemyDogAnimInstance::UEnemyDogAnimInstance()
{
	Speed = 0.0f;
	Direction = 0.0f;
	bIsDead = false;
}

void UEnemyDogAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	EnemyDogCharacter = Cast<AEnemyDog>(TryGetPawnOwner());
}

void UEnemyDogAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);
	if (!EnemyDogCharacter)
	{
		EnemyDogCharacter = Cast<AEnemyDog>(TryGetPawnOwner());
	}

	if (!EnemyDogCharacter) return;

	if (bIsDead)
	{
		Speed = 0.0f;
		return;
	}

	Speed = EnemyDogCharacter->GetVelocity().Size();
	Direction = CalculateDirection(EnemyDogCharacter->GetVelocity(), EnemyDogCharacter->GetActorRotation());
}

float UEnemyDogAnimInstance::CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation)
{
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	FVector ForwardVector = BaseRotation.Vector(); // BaseRotation(캐릭터의 현재 회전 값)에서 정면(Forward) 방향 벡터를 가져옴.
	FVector RightVector = FRotationMatrix(BaseRotation).GetScaledAxis(EAxis::Y); // BaseRotation을 기준으로 오른쪽(Right) 방향 벡터를 가져옴.
	FVector NormalizedVelocity = Velocity.GetSafeNormal2D(); // Velocity(이동 속도 벡터)를 정규화하여 방향을 얻음. (XY 평면 기준)
	float ForwardDot = FVector::DotProduct(ForwardVector, NormalizedVelocity); // 정면 방향 벡터와 이동 방향 벡터 간의 내적(Dot Product) 계산 → 값이 1이면 정면 이동, -1이면 반대 방향 이동
	float RightDot = FVector::DotProduct(RightVector, NormalizedVelocity);// 오른쪽 방향 벡터와 이동 방향 벡터 간의 내적 계산 → 값이 1이면 오른쪽 이동, -1이면 왼쪽 이동
	float Angle = FMath::Atan2(RightDot, ForwardDot); // 이동 방향을 기준으로 캐릭터 정면 방향과의 각도를 계산 (라디안 단위)
	return FMath::RadiansToDegrees(Angle); // 계산된 각도를 라디안에서 도(Degree) 단위로 변환하여 반환
}
