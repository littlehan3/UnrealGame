#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Rifle.h" 
#include "Knife.h"
#include "MeleeCombatComponent.h"
#include "SkillComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "MainCharacter.generated.h"

class ARifle;
class AKnife;
class AMachineGun;
class ASkill3Projectile;
class ACannon;

UCLASS()
class LOCOMOTION_API AMainCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AMainCharacter();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void Landed(const FHitResult& Hit) override;

    // 애님 몽타주 Getter
    FORCEINLINE UAnimMontage* GetSkill1AnimMontage() const { return Skill1AnimMontage; }
    FORCEINLINE UAnimMontage* GetSkill2AnimMontage() const { return Skill2AnimMontage; }
    FORCEINLINE UAnimMontage* GetSkill3AnimMontage() const { return Skill3AnimMontage; }
    FORCEINLINE UAnimMontage* GetAimSkill1AnimMontage() const { return AimSkill1AnimMontage; }
    FORCEINLINE UAnimMontage* GetAimSkill2AnimMontage() const { return AimSkill2AnimMontage; }
    FORCEINLINE UAnimMontage* GetAimSkill2StartAnimMontage() const { return AimSkill2StartAnimMontage; }
    FORCEINLINE UAnimMontage* GetAimSkill3AnimMontage() const { return AimSkill3AnimMontage; }

    // 스킬3 투사체 클래스 Getter
    FORCEINLINE TSubclassOf<ASkill3Projectile> GetSkill3ProjectileClass() const { return Skill3ProjectileClass; }

    // 캐릭터 상태 Getter
    FORCEINLINE bool IsDashing() const { return bIsDashing; }
    FORCEINLINE bool IsAiming() const { return bIsAiming; }
    FORCEINLINE bool IsJumping() const { return bIsJumping; }
    FORCEINLINE bool IsInDoubleJump() const { return bIsInDoubleJump; }

    // SkillComponent 접근할 수 있도록 Getter
    FORCEINLINE USkillComponent* GetSkillComponent() const { return SkillComponent; }

    void ExitAimMode();

    UFUNCTION()
    void UseSkill1();

    UFUNCTION()
    void UseSkill2();

    UFUNCTION()
    void UseSkill3();

    //UFUNCTION()
    //void UseAimSkill1();

    //UFUNCTION()
    //void UseAimSkill2();

    void AttachRifleToBack();
    void AttachRifleToHand();
    void AttachKnifeToBack();
    void AttachKnifeToHand();

protected:
    virtual void BeginPlay() override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void HandleJump();
    void HandleDoubleJump();
    void EnterAimMode();
    void FireWeapon();
    void ReloadWeapon();
    void ComboAttack();

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    class USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    class UCameraComponent* Camera;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* AimAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* FireAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ReloadAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* DashAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ZoomInAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ZoomOutAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* Skill1Action;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* Skill2Action;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* Skill3Action;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    bool bIsAiming;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TSubclassOf<ARifle> RifleClass;

    UPROPERTY(EditAnywhere, CateGory = "Combat")
    class ARifle* Rifle;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TSubclassOf<AKnife> KnifeClass_L;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TSubclassOf<AKnife> KnifeClass_R;

    UPROPERTY()
    AKnife* LeftKnife;

    UPROPERTY()
    AKnife* RightKnife;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ComboAttackMontage1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ComboAttackMontage2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ComboAttackMontage3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ComboAttackMontage4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ComboAttackMontage5;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    float Speed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    float Direction;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    float AimPitch;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump", meta = (AllowPrivateAccess = "true"))
    bool bIsJumping;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump", meta = (AllowPrivateAccess = "true"))
    bool bIsInDoubleJump;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump", meta = (AllowPrivateAccess = "true"))
    bool bIsInAir;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump", meta = (AllowPrivateAccess = "true"))
    bool bCanDoubleJump;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    class UBoxComponent* KickHitBox;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ForwardDashMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* LeftDashMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* RightDashMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* BackwardDashMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill1 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* Skill1AnimMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill2 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* Skill2AnimMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill3 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* Skill3AnimMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AimSkill1 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* AimSkill1AnimMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AimSkill2 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* AimSkill2AnimMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AimSkill2 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* AimSkill2StartAnimMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AimSkill3 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* AimSkill3AnimMontage;

    float DashCooldown = 1.0f;
    bool bIsDashing = false;
    bool bCanDash = true;

    FTimerHandle DashCooldownTimerHandle;

    void Dash();
    void PlayDashMontage(UAnimMontage* DashMontage);
    void ResetDash(UAnimMontage* Montage, bool bInterrupted);
    void ResetDashCooldown();

    float DefaultZoom = 250.0f;
    float AimZoom = 100.0f;
    float MinZoom = 125.0f;
    float MaxZoom = 500.0f;
    float ZoomStep = 20.0f;
    float ZoomInterpSpeed = 10.0f;
    float CurrentZoom = DefaultZoom;
    float TargetZoom = DefaultZoom;

    void ZoomIn();
    void ZoomOut();

    bool bIsLanding = false;
    FTimerHandle LandingTimerHandle;
    void ResetLandingState();

    bool bApplyRootMotionRotation = false;
    FRotator TargetRootMotionRotation;

    UPROPERTY(EditDefaultsOnly, Category = "Skill")
    TSubclassOf<ASkill3Projectile> Skill3ProjectileClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    UMeleeCombatComponent* MeleeCombatComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    USkillComponent* SkillComponent;

    UPROPERTY(EditDefaultsOnly, Category = "Skill")
    TSubclassOf<AMachineGun> MachineGunClass;

    UPROPERTY()
    AMachineGun* MachineGun;

    UPROPERTY(EditDefaultsOnly, Category = "Skill")
    TSubclassOf<ACannon> CannonClass;

    UPROPERTY()
    ACannon* Cannon;

    // 점프 공격 몽타주
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* JumpAttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* DoubleJumpAttackMontage;

};