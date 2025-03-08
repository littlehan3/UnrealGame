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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* DashAction; // 대쉬 인풋액션 추가

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ZoomInAction; // 줌인 인풋액션 추가

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ZoomOutAction; // 줌아웃 인풋액션 추가


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

	FVector LastAttackDirection; // 마지막 공격 방향
	FTimerHandle ComboResetTimerHandle; // 근접 콤보 초기화 타이머
	float ComboResetTime = 1.5f; // 근접 콤보 초기화 시간
	void ResetComboTimer(); // 콤보 초기화 타이머 함수

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

    // 대시 애니메이션
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ForwardDashMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* LeftDashMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* RightDashMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* BackwardDashMontage;

    // 대시 쿨타임
    UPROPERTY(EditDefaultsOnly, Category = "Dash")
    float DashCooldown = 1.0f; // 지정된 시간만큼 쿨타임

    bool bIsDashing = false; // 대시중인지 여부
    bool bCanDash = true; // 대시 가능 여부

    FTimerHandle DashCooldownTimerHandle; // 대시 쿨타임 타이머 핸들

    void Dash(); // 대시 실행 함수
    void PlayDashMontage(UAnimMontage* DashMontage); // 대시 애니메이션 재생 함수
    void ResetDash(UAnimMontage* Montage, bool bInterrupted); // 대시 상태 초기화 함수
    void ResetDashCooldown(); // 대시 쿨타임 해제 함수

    float DefaultZoom = 250.0f; // 기본 줌 거리
	float AimZoom = 100.0f; // 에임 줌 거리
	float MinZoom = 125.0f; // 최소 줌 거리 (확대)
	float MaxZoom = 500.0f; // 최대 줌 거리 (축소)
    float ZoomStep = 20.0f; // 줌 조정 단위 
    float ZoomInterpSpeed = 10.0f; // 줌 변경 속도 (보간)
    float CurrentZoom = DefaultZoom;  // 현재 줌 값
    float TargetZoom = DefaultZoom;   // 목표 줌 값 (보간 대상)

	void ZoomIn();
	void ZoomOut();

	bool bIsLanding = false; // 착지 상태 여부
	FTimerHandle LandingTimerHandle; // 착지 타이머 핸들
	void ResetLandingState(); // 착지 상태 초기화 함수
};