#include "EnemyGuardianAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h" // ĳ���� �����Ʈ ������Ʈ ����
#include "EnemyGuardian.h" // ������ Ŭ������ EnemyGuardian ����
#include "Kismet/KismetMathLibrary.h" // ���� ��ƿ��Ƽ �Լ� ���

UEnemyGuardianAnimInstance::UEnemyGuardianAnimInstance()
{
	// ��� ���� �ʱ�ȭ
	Speed = 0.0f;
	Direction = 0.0f;
	bIsDead = false;
	bIsShieldDestroyed = false;
}

void UEnemyGuardianAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation(); // �θ� Ŭ���� �Լ� ȣ��
	// �ִ� �ν��Ͻ��� ������ ���� EnemyGuardian Ŭ������ ĳ�����Ͽ� ����
	EnemyCharacter = Cast<AEnemyGuardian>(TryGetPawnOwner());
}

void UEnemyGuardianAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime); // �θ� Ŭ���� �Լ� ȣ��

	if (!EnemyCharacter) // ������ ĳ���� ������ ���ٸ�
	{
		EnemyCharacter = Cast<AEnemyGuardian>(TryGetPawnOwner()); // �ٽ� �ѹ� �������� �õ�
	}
	if (!EnemyCharacter) return; // �׷��� ���ٸ� ������Ʈ �ߴ�

	bIsDead = EnemyCharacter->bIsDead; // ĳ������ ��� ���¸� ����ȭ

	if (bIsDead) // ��� ���¶��
	{
		Speed = 0.0f; // �ӵ��� 0���� ����
		return; // �� �̻� �ٸ� �� ������Ʈ �� ��
	}

	// ĳ������ ���� ���� �о�ͼ� �ִ� �ν��Ͻ��� ������ �ݿ�
	bIsShieldDestroyed = EnemyCharacter->bIsShieldDestroyed; // ���� �ı� ���� ����ȭ
	Speed = EnemyCharacter->GetVelocity().Size(); // ���� �ӵ��� Speed ������ ����
	Direction = CalculateDirection(EnemyCharacter->GetVelocity(), EnemyCharacter->GetActorRotation()); // ���� ���
}

// ĳ������ �ӵ� ���Ϳ� ���� ȸ������ ������� ������� �̵� ����(����)�� ���
float UEnemyGuardianAnimInstance::CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation)
{
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER) // �ӵ��� ���� 0�̶��
	{
		return 0.0f; // ������ 0 (����)
	}

	FVector ForwardVector = BaseRotation.Vector(); // ĳ������ ���� ���� ����
	FVector RightVector = FRotationMatrix(BaseRotation).GetScaledAxis(EAxis::Y); // ĳ������ ������ ���� ����
	FVector NormalizedVelocity = Velocity.GetSafeNormal2D(); // �̵� �ӵ� ���͸� ����ȭ (2D ��� ����)

	// ����(Dot Product)�� �̿��� �̵� ������ ����/������ ���Ϳ� �󸶳� ��ġ�ϴ��� ���
	float ForwardDot = FVector::DotProduct(ForwardVector, NormalizedVelocity); // ��/�� ����
	float RightDot = FVector::DotProduct(RightVector, NormalizedVelocity); // ��/�� ����

	// Atan2 �Լ��� �̿��� �� �������� ���� ������ ��� (����)
	float Angle = FMath::Atan2(RightDot, ForwardDot);

	return FMath::RadiansToDegrees(Angle); // ���� ������ ��(Degree) ������ ��ȯ�Ͽ� ��ȯ
}