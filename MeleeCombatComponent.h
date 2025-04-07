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

    bool bComboQueued = false; // 누르고 있는 입력 처리
};
