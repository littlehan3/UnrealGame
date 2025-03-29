#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Rifle.h" 
#include "Knife.h"
#include "Components/BoxComponent.h"  // BoxComponent 추가 (히트박스 용)
#include "Animation/AnimMontage.h"
#include "MainCharacter.generated.h"

class ARifle;
class AKnife;
class AMachineGun; // 전방 선언

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
    class UInputAction* DashAction; // 대쉬 인풋액션 추가

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ZoomInAction; // 줌인 인풋액션 추가

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ZoomOutAction; // 줌아웃 인풋액션 추가

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* Skill1Action; // 스킬1 인풋액션 추가

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* Skill2Action; // 스킬2 인풋액션 추가

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* Skill3Action; // 스킬3 인풋액션 추가

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

    int32 ComboIndex = 0;  // 콤보 단계

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
    bool bIsAttacking = false;  // 공격중인지 여부

    FTimerHandle ComboCooldownHandle; //콤보 쿨다운 타이머

    float ComboCooldownTime = 2.0f; //각 콤보 간 딜레이 시간

    UFUNCTION()
    void OnComboMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    FVector LastAttackDirection; // 마지막 공격 방향
    FTimerHandle ComboResetTimerHandle; // 근접 콤보 초기화 타이머
    float ComboResetTime = 1.5f; // 근접 콤보 초기화 시간
    void ResetComboTimer(); // 콤보 초기화 타이머 함수

    void KickRaycastAttack();
    AActor* KickRaycastHitActor; // 발차기 레이캐스트 적용 대상

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill1 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* Skill1AnimMontage; // 스킬1 애니메이션 추가

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill2 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* Skill2AnimMontage; // 스킬2 애니메이션 추가

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill3 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* Skill3AnimMontage; // 스킬3 애니메이션 추가

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AimSkill1 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* AimSkill1AnimMontage; // 에임모드스킬1 애니메이션 추가

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

    void Skill1(); // 스킬1 사용 함수
    void PlaySkill1Montage(UAnimMontage* Skill1Montage); // 스킬1 애니메이션 재생 함수
    FTimerHandle Skill1CooldownTimerHandle; // 스킬1 쿨다운 타이머 핸들
    void ResetSkill1(UAnimMontage* Montage, bool bInterrupted); // 스킬1 상태 초기화 함수
    void ResetSkill1Cooldown(); // 스킬1 쿨다운 해제 함수
    float Skill1Cooldown = 5.0f; // 스킬1 쿨다운 시간
    bool bIsUsingSkill1 = false; // 스킬1 사용중인지 여부
    bool bCanUseSkill1 = true; // 스킬1 사용 가능 여부
    float SkillRange = 400.0f; // 스킬 범위
    FTimerHandle SkillEffectTimerHandle; // 스킬1 효과 타이머
    void DrawSkill1Range(); // 스킬1 범위 표시 함수
    void ApplySkill1Effect(); // 스킬1 효과 적용 함수

    void AdjustComboAttackDirection(); // 콤보 공격 방향 보정 함수
    // 콤보 공격 방향 보정 관련 변수
    bool bApplyRootMotionRotation = false; // 루트모션 적용 여부
    FRotator TargetRootMotionRotation; // 루트모션 중 유지할 회전값

    void Skill2(); // 스킬2 사용 함수
    void PlaySkill2Montage(UAnimMontage* Skill2Montage); // 스킬2 애니메이션 재생 함수
    FTimerHandle Skill2CooldownTimerHandle; // 스킬2 쿨다운 타이머
    void ResetSkill2(UAnimMontage* Montage, bool bInterrupted); // 스킬2 상태 초기화 함수
    void ResetSkill2Cooldown(); // 스킬 2 쿨다운 해제 함수
    float Skill2Cooldown = 3.0f; // 스킬2 쿨다운 시간
    bool bIsUsingSkill2 = false; // 스킬2 사용중인지 여부
    bool bCanUseSkill2 = true; // 스킬2 사용 가능 여부
    FTimerHandle Skill2EffectTimerHandle; // 스킬2 효과 타이머
    void DrawSkill2Range(); // 스킬2 범위 표시 함수
    void ApplySkill2Effect(); // 스킬2 효과 적용함수
    void ClearSkill2Range(); // 스킬2 범위 삭제 함수
    float Skill2Damage = 50.0f; // 스킬2 데미지
    float Skill2Range = 200.0f; // 스킬2 범위
    float Skill2EffectDelay = 0.5f; // 스킬2 효과 발생 지연시간
    FTimerHandle Skill2RangeClearTimerHandle; // 스킬2 범위 삭제 타이머

    void Skill3();
    void PlaySkill3Montage(UAnimMontage* Skill3Montage); // 스킬3 애니메이션 재생 함수
    FTimerHandle Skill3CooldownTimerHandle; // 스킬3 쿨다운 타이머
    void ResetSkill3(UAnimMontage* Montage, bool bInterrupted); // 스킬3 상태 초기화 함수
    void ResetSkill3Cooldown(); // 스킬3 쿨다운 해제 함수
    float Skill3Cooldown = 3.0f; // 스킬3 쿨다운 시간
    bool bIsUsingSkill3 = false; // 스킬3 사용중인지 여부
    bool bCanUseSkill3 = true; // 스킬3 사용 가능 여부
    
    UPROPERTY(EditDefaultsOnly, Category = "Skill3")
    TSubclassOf<class ASkill3Projectile> Skill3ProjectileClass; // 투사체 클래스를 BP에서 장착
    float Skill3Damage = 60.0f; // 스킬 3 데미지

    void AimSkill1(); // 에임모드스킬1
    void PlayAimSkill1Montage(UAnimMontage* AimSkill1Montage); // 에임모드 스킬1 애니메이션 재생 함수
    FTimerHandle AimSkill1CooldownTimerHandle; // 에임모드 스킬1 쿨다운 타이머
    void ResetAimSkill1(UAnimMontage* Montage, bool bInterrupted); // 에임모드 스킬1 상태 초기화 함수
    void ResetAimSkill1Cooldown(); // 에임모드 스킬1 쿨다운 해제함수
    float AimSkill1Cooldown = 3.0f; // 에임모드 스킬1 쿨다운 시간
    bool bIsUsingAimSkill1 = false; // 에임모드 스킬1 사용중 여부
    bool bCanUseAimSkill1 = true; // 에임모드 스킬1 사용가능 여부
    FTimerHandle AimSkill1RepeatTimerHandle; // 반복 애니메이션 재생 타이머
    float AimSkill1Duration = 5.0f; // 에임모드 스킬1 지속시간
    float AimSkill1PlayInterval = 0.85f; // 에임모드 스킬1 애니메이션 시작 시간 저장 변수
    void RepeatAImSkill1Montage(); // 에임모드 스킬1 애니메이션 반복 재생 함수

    float AimSkill1MontageStartTime = 0.0f; // 에임모드 스킬1 애니메이션 시작 시간 저장용 변수

    UFUNCTION()
    void ResetAimSkill1Timer(); // 에임모드 스킬 1 종료를 위한 인자 없는 타이머 중계 함수

    // BP_MachineGun을 기반으로 생성할 클래스
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TSubclassOf<AMachineGun> MachineGunClass;

    // 현재 소환된 머시건 인스턴스
    UPROPERTY()
    AMachineGun* MachineGun;
};