#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyKatana.generated.h"

// 공격 타입과 앨리트 유무에 따른 데미지 처리를  위한 열거형
UENUM(BlueprintType) 
enum class EAttackType : uint8
{
    Normal, // 일반공격
    Strong, // 강공격
    Jump // 점프공격
};
class AEnemy;
class UNiagaraSystem;
class AMainCharacter;

UCLASS()
class LOCOMOTION_API AEnemyKatana : public AActor
{
    GENERATED_BODY()

public:
    AEnemyKatana();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* KatanaMesh; // 계층 관리를 위한 루트 컴포넌트

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* KatanaChildMesh; // 실질적인 레이캐스트 위치 설정을 위한 메시

    // 판정 데이터
	TSet<AActor*> RaycastHitActors; // 레이캐스트로 맞은 액터들
	TSet<AActor*> DamagedActors; // 데미지 적용된 액터들

	void EnableAttackHitDetection(EAttackType AttackType); // 공격 판정 활성화
	void DisableAttackHitDetection(); // 공격 판정 비활성화

	void PerformRaycastAttack(); // 레이캐스트 공격 판정 수행

    UFUNCTION()
	void HideKatana(); // 카타나 숨기기 및 정리

    //virtual void Tick(float DeltaTime) override;

	void StartAttack(); // 공격 시작
	void EndAttack(); // 공격 종료

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsAttacking = false; // 공격 여부
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsStrongAttack = false; // 강공격 여부
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bHasPlayedHitSound = false; // 히트 사운드 재생 여부

	void ApplyDamage(AActor* OtherActor); // 데미지 적용 함수
    
    UPROPERTY()
	TArray<TObjectPtr<AActor>> EnemyActorsCache; // 모든 적 액터 캐시

    UPROPERTY()
	EAttackType CurrentAttackType; // 현재 공격 타입
     
	void PlayKatanaHitSound(); // 카타나 히트 사운드 재생

	FTimerHandle AttackTraceTimerHandle;  // 공격 판정 타이머 핸들

	// Tick 대신 타이머로 TraceInterval 주기로 판정 수행
	// 낮으면 성능 부담, 높으면 판점 누락 가능성 증가
	const float TraceInterval = 0.016f; // 약 60FPS 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float TotalDistance = 150.0f; // 카타나 공격 범위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	int NumSteps = 5; // 몇 번의 스윕으로 나눌지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float HitRadius = 30.0f; // 스윕 반지름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float DamageAmount = 20.0f; // 기본 데미지 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float EliteDamageAmount = 30.0f; // 엘리트 데미지 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float StrongDamageAmount = 50.0f; // 강공격 데미지 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float EliteStrongDamageAmount = 60.0f; // 엘리트 강공격 데미지 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float JumpDamageAmount = 30.0f; // 점프공격 데미지 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float EliteJumpDamageAmount = 40.0f; // 엘리트 점프공격 데미지 값
};
