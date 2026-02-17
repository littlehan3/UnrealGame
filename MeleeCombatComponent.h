#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MeleeCombatComponent.generated.h"

class UAnimMontage;
class AKnife;
class ACharacter;
class AEnemy;
class USoundBase;
class UBoxComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOCOMOTION_API UMeleeCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMeleeCombatComponent();
	// 캐릭터 생성 후 무기 및 컴포넌트 참조를 안전하게 연결하는 초기화 함수
	void InitializeCombatComponent(ACharacter* InOwnerCharacter, UBoxComponent* InKickHitBox, AKnife* InLeftKnife, AKnife* InRightKnife);
	void TriggerComboAttack(); // 콤보 공격 트리거 함수
	UFUNCTION()
	void OnComboMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 콤보 몽타주 종료 콜백
	void SetComboMontages(const TArray<UAnimMontage*>& InMontages); // 콤보 몽타주 설정 함수
	UFUNCTION()
	void ResetCombo(); // 콤보 리셋 함수
	void ResetComboTimer(); // 콤보 리셋 타이머 시작 함수
	void AdjustAttackDirection(); // 공격 방향 자동 조정 함수
	void ApplyComboMovement(float MoveDistance, FVector MoveDirection); // 콤보 이동 적용 함수
	void EnableKickHitBox(); // 킥 히트박스 활성화 함수
	void DisableKickHitBox(); // 킥 히트박스 비활성화 함수
	void KickRaycastAttack(); // 킥 레이캐스트 공격 함수

	UFUNCTION()
	void HandleKickOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult); // 킥 히트박스 오버랩 처리 함수

	UFUNCTION(BlueprintPure, Category = "Melee|State")
	bool IsAttacking() const { return bIsAttacking; } // 공격 중인지 여부 반환 함수
	void QueueComboAttack() { if (!bComboQueued) bComboQueued = true; } // 콤보 공격 큐잉 함수
	bool IsComboAttackQueued() const { return bComboQueued; } // 콤보 공격이 큐에 있는지 여부 반환 함수
	void ClearAllQueues(); // 모든 큐 초기화 함수
	void ClearComboAttackQueue(); // 콤보 공격 큐 초기화 함수
	bool ShouldTeleportToTarget(float DistanceToTarget); // 순간이동 가능 거리 내에 타겟이 있는지 확인
	void TeleportToTarget(AActor* TargetEnemy); // 타겟 앞으로 순간이동 수행 함수
	UFUNCTION(BlueprintPure, Category = "Melee")
	float GetTeleportCooldownPercent() const; // 텔레포트 쿨타임 진행률 반환 함수

protected:

private:
	void PlayComboMontage(int32 Index); // 콤보 몽타주 재생 함수
	UPROPERTY()
	ACharacter* OwnerCharacter = nullptr; // 소유 캐릭터 참조
	UPROPERTY()
	UBoxComponent* KickHitBox = nullptr; // 킥 히트박스 참조
	UPROPERTY()
	AKnife* LeftKnife = nullptr; // 왼쪽 나이프 참조
	UPROPERTY()
	AKnife* RightKnife = nullptr; // 오른쪽 나이프 참조

	int32 ComboIndex = 0; // 현재 콤보 인덱스
	bool bIsAttacking = false; // 공격 중 상태
	FVector LastAttackDirection = FVector::ZeroVector; // 마지막 공격 방향 저장

	UPROPERTY()
	AActor* KickRaycastHitActor = nullptr; // 킥 레이캐스트로 맞은 액터 저장
	FHitResult KickRaycastHitResult; // 킥 레이캐스트 히트 결과 저장

	FTimerHandle ComboCooldownHandle; // 콤보 쿨타임 타이머
	FTimerHandle ComboResetTimerHandle; // 콤보 리셋 타이머

	float ComboCooldownTime = 2.0f; // 콤보 쿨타임 시간
	float ComboResetTime = 1.5f; // 콤보 리셋 대기 시간

	UPROPERTY()
	TArray<UAnimMontage*> ComboMontages; // 콤보 애니메이션 몽타주 배열
	bool bCanAirAction = true; // 공중 액션 가능 여부
	bool bComboQueued = false; // 콤보 공격이 큐에 있는지 여부

	FTimerHandle InputCooldownHandle; // 입력 쿨타임 타이머
	bool bInputBlocked = false; // 입력 차단 상태
	float InputCooldownTime = 0.1f; // 입력 쿨타임 시간
	UFUNCTION()
	void ResetInputCooldown(); // 입력 쿨타임 리셋 함수

	UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float TeleportDistance = 1200.0f; // 순간이동 최대 거리
	UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float TeleportOffset = 50.0f; // 순간이동 시 적 앞에 위치할 오프셋
	UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float MinTeleportDistance = 150.0f; // 순간이동 최소 거리
	UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	bool bCanTeleport = true; // 순간이동 가능 상태
	FTimerHandle TeleportCooldownHandle; // 텔레포트 쿨타임 타이머
	UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float TeleportCooldownTime = 5.0f; // 텔레포트 쿨타임 시간

	UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float KnifeKnockbackStrength = 1000.0f; // 나이프 콤보 공격 시 넉백 강도
	// 킥공격 시 넉백 강도
	UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float KickKnockbackStrength = 1500.0f; // 킥 넉백 강도
	UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float KickDamage = 35.0f; // 킥 데미지
	UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float MaxAutoAimDistance = 300.0f; // 자동 조준 최대 거리
	UPROPERTY(EditAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float KickRaycastDistance = 150.0f; // 킥 레이캐스트 거리

	UPROPERTY(EditAnywhere, Category = "Config|Animation", meta = (AllowPrivateAccess = "true"))
	float MontageBlendOutTime = 0.1f; // 몽타주 블랜드 아웃 시간
	UPROPERTY(EditAnywhere, Category = "Config|Animation", meta = (AllowPrivateAccess = "true"))
	float MontageDefaultPlayRate = 1.0f; // 몽타주 기본 재생 시간
	UPROPERTY(EditAnywhere, Category = "Config|Physics", meta = (AllowPrivateAccess = "true"))
	float FloorStartOffset = 100.0f; // 몽타주 기본 재생 시간
	UPROPERTY(EditAnywhere, Category = "Config|Physics", meta = (AllowPrivateAccess = "true"))
	float FloorEndOffset = 500.0f; // 몽타주 기본 재생 시간
	UPROPERTY(EditAnywhere, Category = "Config|Physics", meta = (AllowPrivateAccess = "true"))
	float FloorHeightOffset = 90.0f; // 몽타주 기본 재생 시간
	UPROPERTY(EditAnywhere, Category = "Config|Effects", meta = (AllowPrivateAccess = "true"))
	FVector EffectDefaultScale = FVector(1.0f);
	UPROPERTY(EditAnywhere, Category = "Config|Physics", meta = (AllowPrivateAccess = "true"))
	float KickRaycastStartOffset = 20.0f;
	UPROPERTY(EditAnywhere, Category = "Config|Physics", meta = (AllowPrivateAccess = "true"))
	float KickRaycastEndOffset = 150.0f;

};
