#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h" // AActor 클래스 상속
#include "EnemyGuardianBaton.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyGuardianBaton : public AActor
{
	GENERATED_BODY()

public:
	AEnemyGuardianBaton(); // 생성자

	virtual void Tick(float DeltaTime) override; // 매 프레임 호출

	// 공격 판정 활성화/비활성화 함수
	void EnableAttackHitDetection();
	void DisableAttackHitDetection();
	void StartAttack(); // 공격 시작 (내부적으로 EnableAttackHitDetection과 동일)
	void EndAttack(); // 공격 종료 (내부적으로 DisableAttackHitDetection과 동일)

	void PerformRaycastAttack(); // 실제 공격 판정을 수행하는 함수

	UFUNCTION()
	void HideBaton(); // 진압봉을 숨기고 정리하는 함수

	// 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* BatonMesh; // 진압봉의 외형을 나타내는 스태틱 메쉬

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BatonChildMesh; // 공격 판정의 시작점으로 사용할 자식 메쉬

	// 판정 데이터 (매 공격마다 초기화됨)
	TSet<AActor*> RaycastHitActors; // 이번 공격에서 판정에 감지된 액터 목록
	TSet<AActor*> DamagedActors; // 이번 공격에서 실제로 데미지를 입힌 액터 목록

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출

private:
	bool bIsAttacking = false; // 현재 공격 판정이 활성화되었는지 여부
	void ApplyDamage(AActor* OtherActor); // 감지된 액터에게 데미지를 적용하는 함수

	TArray<AActor*> EnemyActorsCache; // 아군 가디언 목록 캐시 (현재 사용되지 않음)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (AllowPrivateAccess = "true"))
	float BatonDamage = 30.0f; // 진압봉의 공격력
};