#include "EnemyGuardianAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트 참조
#include "EnemyGuardian.h" // 소유자 클래스인 EnemyGuardian 참조
#include "Kismet/KismetMathLibrary.h" // 수학 유틸리티 함수 사용

UEnemyGuardianAnimInstance::UEnemyGuardianAnimInstance()
{
	// 멤버 변수 초기화
	Speed = 0.0f;
	Direction = 0.0f;
	bIsDead = false;
	bIsShieldDestroyed = false;
}

void UEnemyGuardianAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation(); // 부모 클래스 함수 호출
	// 애님 인스턴스가 소유한 폰을 EnemyGuardian 클래스로 캐스팅하여 저장
	EnemyCharacter = Cast<AEnemyGuardian>(TryGetPawnOwner());
}

void UEnemyGuardianAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime); // 부모 클래스 함수 호출

	if (!EnemyCharacter) // 소유자 캐릭터 참조가 없다면
	{
		EnemyCharacter = Cast<AEnemyGuardian>(TryGetPawnOwner()); // 다시 한번 가져오기 시도
	}
	if (!EnemyCharacter) return; // 그래도 없다면 업데이트 중단

	bIsDead = EnemyCharacter->bIsDead; // 캐릭터의 사망 상태를 동기화

	if (bIsDead) // 사망 상태라면
	{
		Speed = 0.0f; // 속도를 0으로 고정
		return; // 더 이상 다른 값 업데이트 안 함
	}

	// 캐릭터의 상태 값을 읽어와서 애님 인스턴스의 변수에 반영
	bIsShieldDestroyed = EnemyCharacter->bIsShieldDestroyed; // 방패 파괴 상태 동기화
	Speed = EnemyCharacter->GetVelocity().Size(); // 현재 속도를 Speed 변수에 저장
	Direction = CalculateDirection(EnemyCharacter->GetVelocity(), EnemyCharacter->GetActorRotation()); // 방향 계산
}

// 캐릭터의 속도 벡터와 현재 회전값을 기반으로 상대적인 이동 방향(각도)을 계산
float UEnemyGuardianAnimInstance::CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation)
{
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER) // 속도가 거의 0이라면
	{
		return 0.0f; // 방향은 0 (정지)
	}

	FVector ForwardVector = BaseRotation.Vector(); // 캐릭터의 정면 방향 벡터
	FVector RightVector = FRotationMatrix(BaseRotation).GetScaledAxis(EAxis::Y); // 캐릭터의 오른쪽 방향 벡터
	FVector NormalizedVelocity = Velocity.GetSafeNormal2D(); // 이동 속도 벡터를 정규화 (2D 평면 기준)

	// 내적(Dot Product)을 이용해 이동 방향이 정면/오른쪽 벡터와 얼마나 일치하는지 계산
	float ForwardDot = FVector::DotProduct(ForwardVector, NormalizedVelocity); // 앞/뒤 성분
	float RightDot = FVector::DotProduct(RightVector, NormalizedVelocity); // 좌/우 성분

	// Atan2 함수를 이용해 두 성분으로 최종 각도를 계산 (라디안)
	float Angle = FMath::Atan2(RightDot, ForwardDot);

	return FMath::RadiansToDegrees(Angle); // 라디안 각도를 도(Degree) 단위로 변환하여 반환
}