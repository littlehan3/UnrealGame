#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillComponent.generated.h"

class AMainCharacter;
class UAnimMontage;
class AEnemy;
class ASkill3Projectile;
class AMachineGun;
class AKnife;
class UBoxComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOCOMOTION_API USkillComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USkillComponent();

    void InitializeSkills(AMainCharacter* InCharacter, AMachineGun* InMachineGun, AKnife* InLeftKnife, AKnife* InRightKnife, UBoxComponent* InKickHitBox);

    void UseSkill1();
    void UseSkill2();
    void UseSkill3();
    void UseAimSkill1();

    bool IsUsingSkill1() const { return bIsUsingSkill1; }
    bool IsUsingSkill2() const { return bIsUsingSkill2; }
    bool IsUsingSkill3() const { return bIsUsingSkill3; }
    bool IsUsingAimSkill1() const { return bIsUsingAimSkill1; }
    bool CanUseAimSkill1() const { return bCanUseAimSkill1; }

protected:
    virtual void BeginPlay() override;

private:
    AMainCharacter* OwnerCharacter;

    AKnife* LeftKnife;
    AKnife* RightKnife;
    AMachineGun* MachineGun;
    UBoxComponent* KickHitBox;

    // ��ų1
    bool bIsUsingSkill1 = false;
    bool bCanUseSkill1 = true;
    float Skill1Cooldown = 5.0f;
    float Skill1Range = 400.0f;
    FTimerHandle Skill1CooldownHandle;
    FTimerHandle Skill1EffectHandle;
    UAnimMontage* Skill1Montage;

    void PlaySkill1Montage();
    void ApplySkill1Effect();
    void ResetSkill1(UAnimMontage* Montage, bool bInterrupted);
    void ResetSkill1Cooldown();
    void DrawSkill1Range();

    // ��ų2
    bool bIsUsingSkill2 = false;
    bool bCanUseSkill2 = true;
    float Skill2Cooldown = 3.0f;
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
    void DrawSkill2Range();
    void ClearSkill2Range();

    // ��ų3
    bool bIsUsingSkill3 = false;
    bool bCanUseSkill3 = true;
    float Skill3Cooldown = 3.0f;
    float Skill3Damage = 60.0f;
    FTimerHandle Skill3CooldownHandle;
    UAnimMontage* Skill3Montage;
    TSubclassOf<ASkill3Projectile> Skill3ProjectileClass;

    void PlaySkill3Montage();
    void ResetSkill3(UAnimMontage* Montage, bool bInterrupted);
    void ResetSkill3Cooldown();

    // ���ӽ�ų1
    bool bIsUsingAimSkill1 = false;
    bool bCanUseAimSkill1 = true;
    float AimSkill1Cooldown = 3.0f;
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

    // ��ų ��� �� ĳ���� ȸ��
    void RotateCharacterToInputDirection();
};
