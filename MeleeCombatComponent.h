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

    // 점프 공격 몽타주 설정 함수
    void SetJumpAttackMontages(UAnimMontage* InJumpAttackMontage, UAnimMontage* InDoubleJumpAttackMontage);

    // 점프 공격 실행 함수
    void TriggerJumpAttack(bool bIsDoubleJump);

    void OnCharacterLanded();

    FORCEINLINE bool IsJumpAttacked() const { return bIsJumpAttacked; }
    FORCEINLINE bool CanGroundAction() const { return bCanGroundAction; }
    FORCEINLINE bool CanAirAction() const { return bCanAirAction; }

    // 큐잉 함수들 추가
    void QueueJumpAttack() { if (!bJumpAttackQueued) bJumpAttackQueued = true; }
    void QueueComboAttack() { if (!bComboQueued) bComboQueued = true; }

    // 큐 상태 확인 함수들 추가
    bool IsJumpAttackQueued() const { return bJumpAttackQueued; }
    bool IsComboAttackQueued() const { return bComboQueued; }

    void ClearAllQueues();
    void ClearComboAttackQueue();
    void ClearJumpAttackQueue();

    bool CanStartComboAttack() const { return !bJumpAttackCooldownActive && bCanGroundAction; }

    bool ShouldTeleportToTarget(float DistanceToTarget);
    void TeleportToTarget(AActor* TargetEnemy);

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

    FTimerHandle ComboCooldownHandle;
    FTimerHandle ComboResetTimerHandle;

    float ComboCooldownTime = 2.0f;
    float ComboResetTime = 1.5f;

    UPROPERTY()
    TArray<UAnimMontage*> ComboMontages;

    UPROPERTY()
    UAnimMontage* JumpAttackMontage;

    // 더블 점프 공격 몽타주
    UPROPERTY()
    UAnimMontage* DoubleJumpAttackMontage;

    UFUNCTION()
    void OnJumpAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    bool bIsJumpAttacked = false; // 점프공격 실행 여부
    bool bCanGroundAction = true; // 지상액션(콤보) 가능 여부
    bool bCanAirAction = true; // 공중 액션 (점프, 점프공격) 가능 여부

    bool bComboQueued = false; // 누르고 있는 입력 처리
    bool bJumpAttackQueued = false; // 점프공격 큐잉

    FTimerHandle InputCooldownHandle;
    bool bInputBlocked = false;
    float InputCooldownTime = 0.1f;
    void ResetInputCooldown();

    FTimerHandle JumpAttackCooldownHandle;
    bool bJumpAttackCooldownActive = false;
    float JumpAttackCooldownTime = 0.3f;

    float TeleportDistance = 1200.0f; // 순간이동 가능 거리
    float TeleportOffset = 50.0f; // 적 앞쪽으로 얼마나 떨어져서 나타날지
    float MinTeleportDistance = 150.0f;
    bool bCanTeleport = true; // 순간이동 가능 여부

};
