#include "BossEnemyAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BossEnemy.h"
#include "Kismet/KismetMathLibrary.h"

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
}

float UBossEnemyAnimInstance::CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation)
{
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER) return 0.0f;

	FVector ForwardVector = BaseRotation.Vector(); // ĳ���� ���� ȸ�������� ���� ���� ���͸� ������
	FVector RightVector = FRotationMatrix(BaseRotation).GetScaledAxis(EAxis::Y); // ĳƽ�� ���� ȸ���� ���� ������ ���� ���͸� ������
	FVector NormalizedVelocity = Velocity.GetSafeNormal2D(); // �̵��ӵ����͸� ����ȭ�ؼ� ��� ������ ������ ����
	float ForwardDot = FVector::DotProduct(ForwardVector, NormalizedVelocity); // ���� ���� ���Ϳ� �̵����� ���� ������ ����ؼ� 1�̸� ���� -1�̸� ���� �̵�
	float RightDot = FVector::DotProduct(RightVector, NormalizedVelocity); // ������ ���� ���Ϳ� �̵����� ���� ������ ����ؼ� 1�̸� ������ -1�̸� ���� �̵�
	float Angle = FMath::Atan2(RightDot, ForwardDot); // �̵����� �������� ĳ���� ���� ������� ���� ��� (���ȴ���)
	return FMath::RadiansToDegrees(Angle); // ���� ���� ������ �� ������ ��ȯ�Ͽ� ��ȯ
}
