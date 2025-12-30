#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AimSkill3Projectile.h"
#include "Engine/World.h"
#include "SkillComponent.generated.h"

class AMainCharacter;
class UAnimMontage;
class AEnemy;
class ASkill3Projectile;
class AMachineGun;
class AKnife;
class UBoxComponent;
class ACannon;
class AAimSkill3Projectile;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOCOMOTION_API USkillComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USkillComponent();

    void InitializeSkills(AMainCharacter* InCharacter, AMachineGun* InMachineGun, AKnife* InLeftKnife, AKnife* InRightKnife, UBoxComponent* InKickHitBox, ACannon* InCannon);

    void UseSkill1();
    void UseSkill2();
    void UseSkill3();
    void UseAimSkill1();
    void UseAimSkill2();
    void UseAimSkill3();

    bool IsUsingSkill1() const { return bIsUsingSkill1; }
    bool IsUsingSkill2() const { return bIsUsingSkill2; }
    bool IsUsingSkill3() const { return bIsUsingSkill3; }
    bool IsUsingAimSkill1() const { return bIsUsingAimSkill1; }
    bool CanUseAimSkill1() const { return bCanUseAimSkill1; }
    bool IsUsingAimSkill2() const { return bIsUsingAimSkill2; }
    bool CanUseAimSkill2() const { return bCanUseAimSkill2; }
    bool IsUsingAimSkill3() const { return bIsUsingAimSkill3; }
    bool CanUseAimSkill3() const { return bCanUseAimSkill3; }

    UPROPERTY(EditAnywhere, Category = "Skill")
    TSubclassOf<AAimSkill3Projectile> AimSkill3ProjectileClass;

    void CancelAllSkills();

    bool IsCastingSkill() const; // 스킬 시전 상태 체크

    // UI 바인딩을 위한 쿨타임 Getter 함수들

    // 스킬 1의 쿨타임 진행률(0.0~1.0)을 반환 (1.0 = 쿨타임 맥스, 0.0 = 사용 가능) */
    //UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
    float GetSkill1CooldownPercent() const
    {
        // [FIX 1] 분모가 0이거나 음수인지 먼저 확인합니다.
        if (Skill1Cooldown <= 0.0f)
        {
            return 0.0f;
        }

        // [FIX 2] GetWorld()와 타이머 상태를 확인합니다. (이건 이미 잘 하셨습니다)
        if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(Skill1CooldownHandle))
        {
            // 이제 0으로 나눌 걱정 없이 안전하게 계산합니다.
            return GetWorld()->GetTimerManager().GetTimerRemaining(Skill1CooldownHandle) / Skill1Cooldown;
        }

        return 0.0f; // 쿨타임이 아니면 0
    }

    // 스킬 2의 쿨타임 진행률(0.0~1.0)을 반환
    //UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
    float GetSkill2CooldownPercent() const
    {
        // [FIX 1] 분모가 0이거나 음수인지 먼저 확인합니다.
        if (Skill2Cooldown <= 0.0f)
        {
            return 0.0f;
        }

        // [FIX 2] GetWorld()와 타이머 상태를 확인합니다. (이건 이미 잘 하셨습니다)
        if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(Skill2CooldownHandle))
        {
            // 이제 0으로 나눌 걱정 없이 안전하게 계산합니다.
            return GetWorld()->GetTimerManager().GetTimerRemaining(Skill2CooldownHandle) / Skill2Cooldown;
        }

        return 0.0f; // 쿨타임이 아니면 0
    }

    // 스킬 3의 쿨타임 진행률(0.0~1.0)을 반환
   // UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
    float GetSkill3CooldownPercent() const
    {
        // [FIX 1] 분모가 0이거나 음수인지 먼저 확인합니다.
        if (Skill3Cooldown <= 0.0f)
        {
            return 0.0f;
        }

        // [FIX 2] GetWorld()와 타이머 상태를 확인합니다. (이건 이미 잘 하셨습니다)
        if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(Skill3CooldownHandle))
        {
            // 이제 0으로 나눌 걱정 없이 안전하게 계산합니다.
            return GetWorld()->GetTimerManager().GetTimerRemaining(Skill3CooldownHandle) / Skill3Cooldown;
        }

        return 0.0f; // 쿨타임이 아니면 0
    }

    // 에임스킬 1의 쿨타임 진행률(0.0~1.0)을 반환
   // UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
    float GetAimSkill1CooldownPercent() const
    {
        // [FIX 1] 분모가 0이거나 음수인지 먼저 확인합니다.
        if (AimSkill1Cooldown <= 0.0f)
        {
            return 0.0f;
        }

        // [FIX 2] GetWorld()와 타이머 상태를 확인합니다. (이건 이미 잘 하셨습니다)
        if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(AimSkill1CooldownHandle))
        {
            // 이제 0으로 나눌 걱정 없이 안전하게 계산합니다.
            return GetWorld()->GetTimerManager().GetTimerRemaining(AimSkill1CooldownHandle) / AimSkill1Cooldown;
        }

        return 0.0f; // 쿨타임이 아니면 0
    }

    // 에임스킬 2의 쿨타임 진행률(0.0~1.0)을 반환
    //UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
    float GetAimSkill2CooldownPercent() const
    {
        // [FIX 1] 분모가 0이거나 음수인지 먼저 확인합니다.
        if (AimSkill2Cooldown <= 0.0f)
        {
            return 0.0f;
        }

        // [FIX 2] GetWorld()와 타이머 상태를 확인합니다. (이건 이미 잘 하셨습니다)
        if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(AimSkill2CooldownHandle))
        {
            // 이제 0으로 나눌 걱정 없이 안전하게 계산합니다.
            return GetWorld()->GetTimerManager().GetTimerRemaining(AimSkill2CooldownHandle) / AimSkill2Cooldown;
        }

        return 0.0f; // 쿨타임이 아니면 0
    }

    // 에임스킬 3의 쿨타임 진행률(0.0~1.0)을 반환
    //UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
    float GetAimSkill3CooldownPercent() const
    {
        // [FIX 1] 분모가 0이거나 음수인지 먼저 확인합니다.
        if (AimSkill3Cooldown <= 0.0f)
        {
            return 0.0f;
        }

        // [FIX 2] GetWorld()와 타이머 상태를 확인합니다. (이건 이미 잘 하셨습니다)
        if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(AimSkill3CooldownHandle))
        {
            // 이제 0으로 나눌 걱정 없이 안전하게 계산합니다.
            return GetWorld()->GetTimerManager().GetTimerRemaining(AimSkill3CooldownHandle) / AimSkill3Cooldown;
        }

        return 0.0f; // 쿨타임이 아니면 0
    }

    void CancelAimSkill1ByDash(); // 대쉬로 에임스킬1을 캔슬하는 함수

    bool CanUseSkill1() const { return bCanUseSkill1; }
    bool CanUseSkill2() const { return bCanUseSkill2; }
    bool CanUseSkill3() const { return bCanUseSkill3; }

protected:
    virtual void BeginPlay() override;

private:
    AMainCharacter* OwnerCharacter;

    AKnife* LeftKnife;
    AKnife* RightKnife;
    AMachineGun* MachineGun;
    UBoxComponent* KickHitBox;
    ACannon* Cannon;

    // 스킬1
    bool bIsUsingSkill1 = false;
    bool bCanUseSkill1 = true;
    float Skill1Cooldown = 7.0f;
    float Skill1Range = 400.0f;
    FTimerHandle Skill1CooldownHandle;
    FTimerHandle Skill1EffectHandle;
    UAnimMontage* Skill1Montage;

    void PlaySkill1Montage();
    void ApplySkill1Effect();
    void ResetSkill1(UAnimMontage* Montage, bool bInterrupted);
    void ResetSkill1Cooldown();
  /*  void DrawSkill1Range();*/

    // 스킬2
    bool bIsUsingSkill2 = false;
    bool bCanUseSkill2 = true;
    float Skill2Cooldown = 5.0f;
    float Skill2Damage = 50.0f;
    float Skill2Range = 200.0f;
    float Skill2EffectDelay = 0.5f;
    FTimerHandle Skill2CooldownHandle;
    FTimerHandle Skill2EffectHandle;
    FTimerHandle Skill2RangeClearHandle;
    UAnimMontage* Skill2Montage;

    void PlaySkill2Montage();
    void ApplySkill2Effect();
    void ResetSkill2(UAnimMontage* Montage, bool bInterrupted);
    void ResetSkill2Cooldown();
    //void DrawSkill2Range();
    //void ClearSkill2Range();

    // 스킬3
    bool bIsUsingSkill3 = false;
    bool bCanUseSkill3 = true;
    float Skill3Cooldown = 6.0f;
    float Skill3Damage = 60.0f;
    FTimerHandle Skill3CooldownHandle;
    UAnimMontage* Skill3Montage;
    TSubclassOf<ASkill3Projectile> Skill3ProjectileClass;

    void PlaySkill3Montage();
    void ResetSkill3(UAnimMontage* Montage, bool bInterrupted);
    void ResetSkill3Cooldown();

    // 에임스킬1
    bool bIsUsingAimSkill1 = false;
    bool bCanUseAimSkill1 = true;
    float AimSkill1Cooldown = 7.0f;
    float AimSkill1Duration = 5.0f;
    float AimSkill1PlayInterval = 0.85f;
    float AimSkill1MontageStartTime = 0.0f;
    FTimerHandle AimSkill1CooldownHandle;
    FTimerHandle AimSkill1RepeatHandle;
    UAnimMontage* AimSkill1Montage;

    void PlayAimSkill1Montage();
    void RepeatAimSkill1Montage();
    void ResetAimSkill1Timer();
    void ResetAimSkill1(UAnimMontage* Montage, bool bInterrupted);
    void ResetAimSkill1Cooldown();

    // 에임스킬2
    bool bIsUsingAimSkill2 = false;
    bool bCanUseAimSkill2 = true;
    float AimSkill2Cooldown = 6.0f;
    float AimSkill2Duration = 5.0f;
    FTimerHandle AimSkill2CooldownHandle;
    UAnimMontage* AimSkill2Montage;
    UAnimMontage* AimSkill2StartMontage;
    FTimerHandle AimSkill2TransitionHandle;

    void PlayAimSkill2StartMontage();
    void OnAimSkill2StartMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    void PlayAimSkill2Montage();
    void ResetAimSkill2(UAnimMontage* Montage, bool bInterrupted);
    void ResetAimSkill2Cooldown();

    // 에임스킬3
    bool bIsUsingAimSkill3 = false;
    bool bCanUseAimSkill3 = true;
    float AimSkill3Cooldown = 10.0f;
    FTimerHandle AimSkill3CooldownHandle;
    UAnimMontage* AimSkill3Montage;

    float AimSkill3Distance = 1500.0f;
    float AimSkill3Radius = 300.0f;
    int32 NumProjectiles = 5;
    bool bDrawDebugRange = true;
    FVector CachedAimSkill3Target;

    void PlayAimSkill3Montage();
    void OnAimSkill3MontageEnded(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION()
    void SpawnAimSkill3Projectiles();

    void ResetAimSkill3(UAnimMontage* Montage, bool bInterrupted);
    void ResetAimSkill3Cooldown();
    void DrawAimSkill3Range(const FVector& TargetLocation);

    TArray<FVector> AimSkill3DropPoints;

    FTimerHandle AimSkill3DropTimeHandle;
    float AimSkill3ProjectileDelay = 3.0f; // 투사체 낙하까지의 시간(초)

    // 스킬 사용 시 캐릭터 회전
    void RotateCharacterToInputDirection();
};