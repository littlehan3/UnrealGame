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

	FVector ForwardVector = BaseRotation.Vector(); // BaseRotation(ĳ������ ���� ȸ�� ��)���� ����(Forward) ���� ���͸� ������.
	FVector RightVector = FRotationMatrix(BaseRotation).GetScaledAxis(EAxis::Y); // BaseRotation�� �������� ������(Right) ���� ���͸� ������.
	FVector NormalizedVelocity = Velocity.GetSafeNormal2D(); // Velocity(�̵� �ӵ� ����)�� ����ȭ�Ͽ� ������ ����. (XY ��� ����)
	float ForwardDot = FVector::DotProduct(ForwardVector, NormalizedVelocity); // ���� ���� ���Ϳ� �̵� ���� ���� ���� ����(Dot Product) ��� �� ���� 1�̸� ���� �̵�, -1�̸� �ݴ� ���� �̵�
	float RightDot = FVector::DotProduct(RightVector, NormalizedVelocity);// ������ ���� ���Ϳ� �̵� ���� ���� ���� ���� ��� �� ���� 1�̸� ������ �̵�, -1�̸� ���� �̵�
	float Angle = FMath::Atan2(RightDot, ForwardDot); // �̵� ������ �������� ĳ���� ���� ������� ������ ��� (���� ����)
	return FMath::RadiansToDegrees(Angle); // ���� ������ ���ȿ��� ��(Degree) ������ ��ȯ�Ͽ� ��ȯ
}
