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

    // ���� ���� ��Ÿ�� ���� �Լ�
    void SetJumpAttackMontages(UAnimMontage* InJumpAttackMontage, UAnimMontage* InDoubleJumpAttackMontage);

    // ���� ���� ���� �Լ�
    void TriggerJumpAttack(bool bIsDoubleJump);

    void OnCharacterLanded();

    FORCEINLINE bool IsJumpAttacked() const { return bIsJumpAttacked; }
    FORCEINLINE bool CanGroundAction() const { return bCanGroundAction; }
    FORCEINLINE bool CanAirAction() const { return bCanAirAction; }

    // ť�� �Լ��� �߰�
    void QueueJumpAttack() { if (!bJumpAttackQueued) bJumpAttackQueued = true; }
    void QueueComboAttack() { if (!bComboQueued) bComboQueued = true; }

    // ť ���� Ȯ�� �Լ��� �߰�
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

    // ���� ���� ���� ��Ÿ��
    UPROPERTY()
    UAnimMontage* DoubleJumpAttackMontage;

    UFUNCTION()
    void OnJumpAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    bool bIsJumpAttacked = false; // �������� ���� ����
    bool bCanGroundAction = true; // ����׼�(�޺�) ���� ����
    bool bCanAirAction = true; // ���� �׼� (����, ��������) ���� ����

    bool bComboQueued = false; // ������ �ִ� �Է� ó��
    bool bJumpAttackQueued = false; // �������� ť��

    FTimerHandle InputCooldownHandle;
    bool bInputBlocked = false;
    float InputCooldownTime = 0.1f;
    void ResetInputCooldown();

    FTimerHandle JumpAttackCooldownHandle;
    bool bJumpAttackCooldownActive = false;
    float JumpAttackCooldownTime = 0.3f;

    float TeleportDistance = 1200.0f; // �����̵� ���� �Ÿ�
    float TeleportOffset = 50.0f; // �� �������� �󸶳� �������� ��Ÿ����
    float MinTeleportDistance = 150.0f;
    bool bCanTeleport = true; // �����̵� ���� ����

};
