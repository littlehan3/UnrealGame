#include "EnemyShooterAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h" // ĳ���� �����Ʈ ������Ʈ ����
#include "EnemyShooter.h" // ������ Ŭ������ EnemyShooter ����
#include "Kismet/KismetMathLibrary.h" // ���� ��ƿ��Ƽ �Լ� ���

UEnemyShooterAnimInstance::UEnemyShooterAnimInstance()
{
    // ��� ���� �ʱ�ȭ
    Speed = 0.0f;
    Direction = 0.0f;
    bIsDead = false;
}

void UEnemyShooterAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation(); // �θ� Ŭ���� �Լ� ȣ��
    // �ִ� �ν��Ͻ��� ������ ���� EnemyShooter Ŭ������ ĳ�����Ͽ� ����
    EnemyCharacter = Cast<AEnemyShooter>(TryGetPawnOwner());
}

void UEnemyShooterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    Super::NativeUpdateAnimation(DeltaTime); // �θ� Ŭ���� �Լ� ȣ��

    if (!EnemyCharacter) // ������ ĳ���� ������ ���ٸ�
    {
        EnemyCharacter = Cast<AEnemyShooter>(TryGetPawnOwner()); // �ٽ� �ѹ� �������� �õ�
    }

    if (!EnemyCharacter) return; // �׷��� ���ٸ� ������Ʈ �ߴ�

    bIsDead = EnemyCharacter->bIsDead; // ĳ������ ��� ���¸� ����ȭ

    if (bIsDead) // ��� ���¶��
    {
        Speed = 0.0f; // �ӵ��� 0���� ����
        return; // �� �̻� �ٸ� �� ������Ʈ �� ��
    }

    // ĳ������ ���� �ӵ��� �̵� ������ ����Ͽ� ������ ����
    Speed = EnemyCharacter->GetVelocity().Size();
    Direction = CalculateDirection(EnemyCharacter->GetVelocity(), EnemyCharacter->GetActorRotation());
}

// ĳ������ �ӵ� ���Ϳ� ���� ȸ������ ������� ������� �̵� ����(����)�� ���
float UEnemyShooterAnimInstance::CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation)
{
    if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER) // �ӵ��� ���� 0�̶��
    {
        return 0.0f; // ������ 0 (����)
    }

    FVector ForwardVector = BaseRotation.Vector(); // ĳ������ ���� ���� ����
    FVector RightVector = FRotationMatrix(BaseRotation).GetScaledAxis(EAxis::Y); // ĳ������ ������ ���� ����
    FVector NormalizedVelocity = Velocity.GetSafeNormal2D(); // �̵� �ӵ� ���͸� ����ȭ�Ͽ� ���� ���� ������ ���� (2D ��� ����)

    // ����(Dot Product)�� �̿��� �̵� ������ ����/������ ���Ϳ� �󸶳� ��ġ�ϴ��� ���
    float ForwardDot = FVector::DotProduct(ForwardVector, NormalizedVelocity); // ��/�� ����
    float RightDot = FVector::DotProduct(RightVector, NormalizedVelocity); // ��/�� ����

    // Atan2 �Լ��� �̿��� �� �������� ���� ������ ��� (����)
    float Angle = FMath::Atan2(RightDot, ForwardDot);

    return FMath::RadiansToDegrees(Angle); // ���� ������ ��(Degree) ������ ��ȯ�Ͽ� ��ȯ
}