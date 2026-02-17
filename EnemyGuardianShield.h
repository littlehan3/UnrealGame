#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h" // AActor 클래스 상속
#include "EnemyGuardianShield.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyGuardianShield : public AActor
{
	GENERATED_BODY()

public:
	AEnemyGuardianShield(); // 생성자

	// 방패 밀치기 애니메이션이 짧고, 공격속도가 빠르므로 Tick에서 판정을 수행
	// 기본적으로 끄고 필요시에 on off
	virtual void Tick(float DeltaTime) override; 

	// 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* ShieldMesh; // 방패 외형 및 루트 컴포넌트

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShieldChildMesh; // 공격 판정 위치용 자식 메쉬 (현재 미사용)

	// 판정 데이터
	TSet<AActor*> RaycastHitActors; // 이번 공격에 감지된 액터 목록
	TSet<AActor*> DamagedActors; // 이번 공격에 데미지를 입힌 액터 목록

	void HideShield(); // 방패를 숨기고 정리하는 함수
	void StartShieldAttack(); // 방패 공격 판정 시작
	void EndShieldAttack(); // 방패 공격 판정 종료
	void PerformRaycastAttack(); // 실제 공격 판정을 수행하는 함수

	// 방패 속성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float ShieldHealth = 500.0f; // 방패의 내구도

protected:

private:
	bool bIsAttacking = false; // 현재 공격 판정이 활성화되었는지 여부
	void ApplyDamage(AActor* OtherActor); // 감지된 액터에게 데미지를 적용하는 함수

	bool bHasPlayedHitSound = false;
	void PlayShieldHitSound();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float ShieldDamage = 10.0f; // 방패 공격의 데미지

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float KnockbackStrength = 1000.0f; // 넉백 힘

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float TotalDistance = 60.0f; // 공격 판정의 총 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	int NumSteps = 5; // 몇 번의 스윕으로 나눌지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float SweepRadius = 50.0f; // 스윕 반경
};