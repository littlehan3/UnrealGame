#include "EnemyDogAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h" // ĳ���� �����Ʈ ������Ʈ ����
#include "EnemyDog.h" // ������ Ŭ������ EnemyDog ����
#include "Kismet/KismetMathLibrary.h" // ���� ���� ��ƿ��Ƽ �Լ� ���

UEnemyDogAnimInstance::UEnemyDogAnimInstance()
{
	// ��� ���� �ʱ�ȭ
	Speed = 0.0f;
	Direction = 0.0f;
	bIsDead = false;
}

void UEnemyDogAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation(); // �θ� Ŭ���� �Լ� ȣ��
	// �ִ� �ν��Ͻ��� ������ ���� EnemyDog Ŭ������ ĳ�����Ͽ� ����
	EnemyDogCharacter = Cast<AEnemyDog>(TryGetPawnOwner());
}

void UEnemyDogAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime); // �θ� Ŭ���� �Լ� ȣ��

	// ������ ĳ���� ������ ���ٸ� �ٽ� �ѹ� �������� �õ�
	if (!EnemyDogCharacter)
	{
		EnemyDogCharacter = Cast<AEnemyDog>(TryGetPawnOwner());
	}
	if (!EnemyDogCharacter) return; // �׷��� ���ٸ� ������Ʈ �ߴ�

	if (bIsDead) // ��� ���¶��
	{
		Speed = 0.0f; // �ӵ��� 0���� ����
		return; // �� �̻� �ٸ� �� ������Ʈ �� ��
	}

	// EnemyDog ĳ������ ���� ���� �о�ͼ� �ִ� �ν��Ͻ��� ������ �ݿ�
	bIsInAirStun = EnemyDogCharacter->bIsInAirStun; // ���� ���� ���� ����ȭ
	Speed = EnemyDogCharacter->GetVelocity().Size(); // ���� �ӵ�(���� ũ��)�� Speed ������ ����
	Direction = CalculateDirection(EnemyDogCharacter->GetVelocity(), EnemyDogCharacter->GetActorRotation()); // ���� ��� �� Direction ������ ����
}

float UEnemyDogAnimInstance::CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation)
{
	// �ӵ��� ���� 0�� �����ٸ� ������ 0 (����)
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	// ĳ������ ���� ���� ���Ϳ� ������ ���� ���͸� ����
	FVector ForwardVector = BaseRotation.Vector();
	FVector RightVector = FRotationMatrix(BaseRotation).GetScaledAxis(EAxis::Y);

	// �̵� �ӵ� ���͸� ����ȭ�Ͽ� ������ ���� ������ ���� (2D ��� ����)
	FVector NormalizedVelocity = Velocity.GetSafeNormal2D();

	// ���� ���Ϳ� �̵� ���� ���͸� �����Ͽ� '��/��' ������ ����
	float ForwardDot = FVector::DotProduct(ForwardVector, NormalizedVelocity);
	// ������ ���Ϳ� �̵� ���� ���͸� �����Ͽ� '��/��' ������ ����
	float RightDot = FVector::DotProduct(RightVector, NormalizedVelocity);

	// Atan2 �Լ��� �̿��� �� �������� ���� ������ ��� (����)
	float Angle = FMath::Atan2(RightDot, ForwardDot);

	// ���� ���� ������ ��(Degree) ������ ��ȯ�Ͽ� ��ȯ
	return FMath::RadiansToDegrees(Angle);
}