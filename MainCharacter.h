#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Rifle.h" 
#include "Knife.h"
#include "LockOnComponent.h"
#include "Components/BoxComponent.h"  // BoxComponent 추가 (히트박스 용)
#include "Animation/AnimMontage.h"
#include "MainCharacter.generated.h"

class ARifle;
class AKnife;

UCLASS()
class LOCOMOTION_API AMainCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AMainCharacter();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void Landed(const FHitResult& Hit) override;

protected:
    virtual void BeginPlay() override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void HandleJump();
    void HandleDoubleJump();
    void EnterAimMode();
    void ExitAimMode();
    void FireWeapon();
    void ReloadWeapon();
    void AttachRifleToBack();
    void AttachRifleToHand();
    void AttachKnifeToBack();
    void AttachKnifeToHand();
    void ComboAttack();
    void ResetCombo();;
    void PlayComboAttackAnimation1();
    void PlayComboAttackAnimation2();
    void PlayComboAttackAnimation3();
    void PlayComboAttackAnimation4();
    void PlayComboAttackAnimation5();
    void ApplyComboMovement(float MoveDistance, FVector MoveDirection);
    void EnableKickHitBox();
    void DisableKickHitBox();

	void ToggleLockOn(); // 락온 토글 함수 추가

    UFUNCTION()
    void OnKickHitBoxOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );


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
    class UInputAction* LockOnAction; // 락온 인풋액션 추가


    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    bool bIsAiming;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TSubclassOf<ARifle> RifleClass;

    UPROPERTY(EditAnywhere, CateGory = "Combat")
    class ARifle* Rifle;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TSubclassOf<AKnife> KnifeClass_L;  // 왼손 나이프
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TSubclassOf<AKnife> KnifeClass_R;  // 오른손 나이프

    UPROPERTY()
    AKnife* LeftKnife;
    UPROPERTY()
    AKnife* RightKnife;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
    int32 ComboIndex = 0;  // 콤보 단계

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
    bool bIsAttacking = false;  // 공격중인지 여부

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
    FTimerHandle ComboCooldownHandle; //콤보 쿨다운 타이머

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
    float ComboCooldownTime = 2.0f; //각 콤보 간 딜레이 시간

    UFUNCTION()
    void OnComboMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    FVector LastAttackDirection;

    void KickRaycastAttack();
    AActor* KickRaycastHitActor; // 발차기 레이캐스트 적용 대상

    UPROPERTY()
    class ULockOnSystem* LockOnComponent; //락온 컴포넌트 변수추가

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lock-On", meta = (AllowPrivateAccess = "true"))
    bool bIsLockedOn;

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

    // 발차기 히트박스 변수 추가
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    class UBoxComponent* KickHitBox;
};