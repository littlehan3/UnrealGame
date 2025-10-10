#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h" // UAnimInstance 클래스 상속
#include "Kismet/KismetMathLibrary.h" // 수학 라이브러리 함수 사용
#include "EnemyShooterAnimInstance.generated.h"

UCLASS()
class LOCOMOTION_API UEnemyShooterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UEnemyShooterAnimInstance(); // 생성자

	// 블루프린트에서 호출 가능한 Getter 함수들
	UFUNCTION(BlueprintCallable, Category = "Animation")
	float GetSpeed() const { return Speed; }

	UFUNCTION(BlueprintCallable, Category = "Animation")
	float GetDirection() const { return Direction; }

	UFUNCTION(BlueprintCallable, Category = "Animation")
	bool GetIsDead() const { return bIsDead; }

	// 애니메이션 블루프린트와 직접 연동될 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsDead; // 사망 여부

protected:
	virtual void NativeInitializeAnimation() override; // 애님 인스턴스 초기화 시 호출
	virtual void NativeUpdateAnimation(float DeltaTime) override; // 매 프레임 애니메이션 갱신 시 호출

private:
	// C++ 내부에서만 사용할 변수들
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class AEnemyShooter* EnemyCharacter; // 이 애님 인스턴스의 소유자 캐릭터에 대한 참조

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	float Speed; // 캐릭터의 현재 속도

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	float Direction; // 캐릭터의 현재 이동 방향 (전방 기준 -180 ~ 180도)

	// 속도와 회전값을 기반으로 이동 방향 각도를 계산하는 함수
	float CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation);
};