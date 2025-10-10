#include "EnemyDogAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트 참조
#include "EnemyDog.h" // 소유자 클래스인 EnemyDog 참조
#include "Kismet/KismetMathLibrary.h" // 수학 관련 유틸리티 함수 사용

UEnemyDogAnimInstance::UEnemyDogAnimInstance()
{
	// 멤버 변수 초기화
	Speed = 0.0f;
	Direction = 0.0f;
	bIsDead = false;
}

void UEnemyDogAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation(); // 부모 클래스 함수 호출
	// 애님 인스턴스가 소유한 폰을 EnemyDog 클래스로 캐스팅하여 저장
	EnemyDogCharacter = Cast<AEnemyDog>(TryGetPawnOwner());
}

void UEnemyDogAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime); // 부모 클래스 함수 호출

	// 소유자 캐릭터 참조가 없다면 다시 한번 가져오기 시도
	if (!EnemyDogCharacter)
	{
		EnemyDogCharacter = Cast<AEnemyDog>(TryGetPawnOwner());
	}
	if (!EnemyDogCharacter) return; // 그래도 없다면 업데이트 중단

	if (bIsDead) // 사망 상태라면
	{
		Speed = 0.0f; // 속도를 0으로 고정
		return; // 더 이상 다른 값 업데이트 안 함
	}

	// EnemyDog 캐릭터의 상태 값을 읽어와서 애님 인스턴스의 변수에 반영
	bIsInAirStun = EnemyDogCharacter->bIsInAirStun; // 공중 스턴 상태 동기화
	Speed = EnemyDogCharacter->GetVelocity().Size(); // 현재 속도(벡터 크기)를 Speed 변수에 저장
	Direction = CalculateDirection(EnemyDogCharacter->GetVelocity(), EnemyDogCharacter->GetActorRotation()); // 방향 계산 후 Direction 변수에 저장
}

float UEnemyDogAnimInstance::CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation)
{
	// 속도가 거의 0에 가깝다면 방향은 0 (정면)
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	// 캐릭터의 정면 방향 벡터와 오른쪽 방향 벡터를 구함
	FVector ForwardVector = BaseRotation.Vector();
	FVector RightVector = FRotationMatrix(BaseRotation).GetScaledAxis(EAxis::Y);

	// 이동 속도 벡터를 정규화하여 순수한 방향 정보만 남김 (2D 평면 기준)
	FVector NormalizedVelocity = Velocity.GetSafeNormal2D();

	// 정면 벡터와 이동 방향 벡터를 내적하여 '앞/뒤' 성분을 구함
	float ForwardDot = FVector::DotProduct(ForwardVector, NormalizedVelocity);
	// 오른쪽 벡터와 이동 방향 벡터를 내적하여 '좌/우' 성분을 구함
	float RightDot = FVector::DotProduct(RightVector, NormalizedVelocity);

	// Atan2 함수를 이용해 두 성분으로 최종 각도를 계산 (라디안)
	float Angle = FMath::Atan2(RightDot, ForwardDot);

	// 계산된 라디안 각도를 도(Degree) 단위로 변환하여 반환
	return FMath::RadiansToDegrees(Angle);
}