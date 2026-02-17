#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyGuardianBaton.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyGuardianBaton : public AActor
{
	GENERATED_BODY()

public:
	AEnemyGuardianBaton();

	// 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* BatonMesh; // 진압봉의 외형을 나타내는 스태틱 메쉬

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BatonChildMesh; // 공격 판정의 시작점으로 사용할 자식 메쉬

	// 판정 데이터 (매 공격마다 초기화됨)
	TSet<AActor*> RaycastHitActors; // 이번 공격에서 판정에 감지된 액터 목록
	TSet<AActor*> DamagedActors; // 이번 공격에서 실제로 데미지를 입힌 액터 목록

	// 공격 판정 활성화/비활성화 함수
	void EnableAttackHitDetection();
	void DisableAttackHitDetection();

	void StartAttack(); // 공격 시작 (내부적으로 EnableAttackHitDetection과 동일)
	void EndAttack(); // 공격 종료 (내부적으로 DisableAttackHitDetection과 동일)

	void PerformRaycastAttack(); // 실제 공격 판정을 수행하는 함수

	UFUNCTION()
	void HideBaton(); // 진압봉을 숨기고 정리하는 함수

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsAttacking = false; // 현재 공격 판정이 활성화되었는지 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bHasPlayedHitSound = false;

	void ApplyDamage(AActor* OtherActor); // 감지된 액터에게 데미지를 적용하는 함수

	void PlayWeaponHitSound(); // 무기 히트 사운드 재생 함수
	TArray<AActor*> EnemyActorsCache; // 아군 가디언 목록 캐시 

	FTimerHandle AttackTraceTimerHandle;  // 공격 판정 타이머 핸들

	// Tick 대신 타이머로 TraceInterval 주기로 판정 수행
	// 낮으면 성능 부담, 높으면 판점 누락 가능성 증가
	const float TraceInterval = 0.016f; // 약 60FPS 간격

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (AllowPrivateAccess = "true"))
	float BatonDamage = 30.0f; // 진압봉의 공격력
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	float TotalDistance = 120.0f; // 공격 범위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	int NumSteps = 5; // 몇 번의 스윕으로 나눌지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float HitRadius = 30.0f; // 스윕 반지름
};