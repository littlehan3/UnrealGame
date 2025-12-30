#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/BoxComponent.h"
#include "MeleeCombatComponent.generated.h"

class UAnimMontage;
class AKnife;
class UBoxComponent;
class ACharacter;
class AEnemy;
class USoundBase;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOCOMOTION_API UMeleeCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMeleeCombatComponent();

    void InitializeCombatComponent(ACharacter* InOwnerCharacter, UBoxComponent* InKickHitBox, AKnife* InLeftKnife, AKnife* InRightKnife);
    void TriggerComboAttack();
    void OnComboMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    void SetComboMontages(const TArray<UAnimMontage*>& InMontages);

    UFUNCTION()
    void ResetCombo();

    void ResetComboTimer();
    void AdjustAttackDirection();
    void ApplyComboMovement(float MoveDistance, FVector MoveDirection);
    void EnableKickHitBox();
    void DisableKickHitBox();
    void KickRaycastAttack();

    UFUNCTION()
    void HandleKickOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    bool IsAttacking() const { return bIsAttacking; }

    // 큐잉 함수들 추가
    void QueueComboAttack() { if (!bComboQueued) bComboQueued = true; }

    // 큐 상태 확인 함수들 추가
    bool IsComboAttackQueued() const { return bComboQueued; }

    void ClearAllQueues();
    void ClearComboAttackQueue();

    bool ShouldTeleportToTarget(float DistanceToTarget);
    void TeleportToTarget(AActor* TargetEnemy);

    // 텔레포트 쿨타임 백분율을 반환하는 Getter
    UFUNCTION(BlueprintPure, Category = "Melee")
    float GetTeleportCooldownPercent() const;

protected:
    virtual void BeginPlay() override;

private:
    void PlayComboMontage(int32 Index);

    ACharacter* OwnerCharacter = nullptr;
    UBoxComponent* KickHitBox = nullptr;
    AKnife* LeftKnife = nullptr;
    AKnife* RightKnife = nullptr;

    int32 ComboIndex = 0;
    bool bIsAttacking = false;
    FVector LastAttackDirection = FVector::ZeroVector;
    AActor* KickRaycastHitActor = nullptr;
    FHitResult KickRaycastHitResult;

    FTimerHandle ComboCooldownHandle;
    FTimerHandle ComboResetTimerHandle;

    float ComboCooldownTime = 2.0f;
    float ComboResetTime = 1.5f;

    UPROPERTY()
    TArray<UAnimMontage*> ComboMontages;

    bool bCanAirAction = true; // 공중 액션 (점프, 점프공격) 가능 여부

    bool bComboQueued = false; // 누르고 있는 입력 처리

    FTimerHandle InputCooldownHandle;
    bool bInputBlocked = false;
    float InputCooldownTime = 0.1f;
    void ResetInputCooldown();

    float TeleportDistance = 1200.0f; // 순간이동 가능 거리
    float TeleportOffset = 50.0f; // 적 앞쪽으로 얼마나 떨어져서 나타날지
    float MinTeleportDistance = 150.0f;
    bool bCanTeleport = true; // 순간이동 가능 여부
    FTimerHandle TeleportCooldownHandle; // 텔레포트 쿨타임 타이머
    float TeleportCooldownTime = 5.0f; // 텔레포트 쿨타임 시간

    // 나이프 콤보 공격 시 넉백 강도
    UPROPERTY(EditAnywhere, Category = "Config")
    float KnifeKnockbackStrength = 1000.0f;

    // 킥(4콤보) 공격 시 넉백 강도
    UPROPERTY(EditAnywhere, Category = "Config")
    float KickKnockbackStrength = 1000.0f;

};