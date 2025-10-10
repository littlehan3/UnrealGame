#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h" // AActor 클래스 상속
#include "DrawDebugHelpers.h" // 디버그 시각화 기능 사용
#include "EnemyGuardianShield.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyGuardianShield : public AActor
{
	GENERATED_BODY()

public:
	AEnemyGuardianShield(); // 생성자

	virtual void Tick(float DeltaTime) override; // 매 프레임 호출

	void HideShield(); // 방패를 숨기고 정리하는 함수
	void StartShieldAttack(); // 방패 공격 판정 시작
	void EndShieldAttack(); // 방패 공격 판정 종료
	void PerformRaycastAttack(); // 실제 공격 판정을 수행하는 함수

	// 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* ShieldMesh; // 방패 외형 및 루트 컴포넌트

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShieldChildMesh; // 공격 판정 위치용 자식 메쉬 (현재 미사용)

	// 방패 속성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float ShieldHealth = 200.0f; // 방패의 내구도

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ShieldDamage = 40.0f; // 방패 공격의 데미지

	// 판정 데이터
	TSet<AActor*> RaycastHitActors; // 이번 공격에 감지된 액터 목록
	TSet<AActor*> DamagedActors; // 이번 공격에 데미지를 입힌 액터 목록

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출

private:
	bool bIsAttacking = false; // 현재 공격 판정이 활성화되었는지 여부
	void ApplyDamage(AActor* OtherActor); // 감지된 액터에게 데미지를 적용하는 함수
};