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
#include "CrossHairWidget.h"  // Widget을 위한 헤더 추가
#include "Components/SlateWrapperTypes.h" // ESlateVisibility를 위한 헤더 추가
#include "Kismet/GameplayStatics.h" // CreateWidget 함수
#include "CrossHairComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/PostProcessComponent.h"
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

    // 킥 이펙트/사운드 Getter (새로 추가)
    FORCEINLINE class UNiagaraSystem* GetKickNiagaraEffect() const { return KickNiagaraEffect; }
    FORCEINLINE class USoundBase* GetKickHitSound() const { return KickHitSound; }
    FORCEINLINE float GetKickEffectOffset() const { return KickEffectOffset; }
    FORCEINLINE float GetKnifeEffectOffset() const { return KnifeEffectOffset; }

    void ExitAimMode();

    UFUNCTION()
    void UseSkill1();

    UFUNCTION()
    void UseSkill2();

    UFUNCTION()
    void UseSkill3();

    void AttachRifleToBack();
    void AttachRifleToHand();
    void AttachKnifeToBack();
    void AttachKnifeToHand();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float MaxHealth = 1000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    float CurrentHealth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    bool bIsDead = false;

    // [신규] 빅 히트 상태 플래그
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    bool bIsBigHitReacting = false;

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

    void Die();

    // 크로스헤어 컴포넌트 접근자 (반환 타입 수정)
    UFUNCTION(BlueprintPure, Category = "Crosshair")
    UCrossHairComponent* GetCrosshairComponent() const { return CrosshairComponent; }

    // 현재 체력 비율 (0.0 ~ 1.0)을 반환
    UFUNCTION(BlueprintPure, Category = "HUD")
    float GetHealthPercent() const
    {
        return (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f;
    }

    /** 현재 장착된 라이플의 현재 탄약 수를 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "HUD")
    int32 GetRifleCurrentAmmo() const
    {
        return (Rifle) ? Rifle->GetCurrentAmmo() : 0;
    }

    /** 현재 장착된 라이플의 최대 탄약 수를 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "HUD")
    int32 GetRifleMaxAmmo() const
    {
        return (Rifle) ? Rifle->GetMaxAmmo() : 0;
    }

    UFUNCTION(BlueprintPure, Category = "HUD")
    int32 GetRifleTotalAmmo() const
    {
        return (Rifle) ? Rifle->GetTotalAmmo() : 0;
    }

    // 스킬 쿨타임 중계 함수들

    UFUNCTION(BlueprintPure, Category = "HUD|Skill")
    float GetSkill1CooldownPercent() const
    {
        return (SkillComponent) ? SkillComponent->GetSkill1CooldownPercent() : 0.0f;
    }

    UFUNCTION(BlueprintPure, Category = "HUD|Skill")
    float GetSkill2CooldownPercent() const
    {
        return (SkillComponent) ? SkillComponent->GetSkill2CooldownPercent() : 0.0f;
    }

    UFUNCTION(BlueprintPure, Category = "HUD|Skill")
    float GetSkill3CooldownPercent() const
    {
        return (SkillComponent) ? SkillComponent->GetSkill3CooldownPercent() : 0.0f;
    }

    UFUNCTION(BlueprintPure, Category = "HUD|Skill")
    float GetAimSkill1CooldownPercent() const
    {
        return (SkillComponent) ? SkillComponent->GetAimSkill1CooldownPercent() : 0.0f;
    }

    UFUNCTION(BlueprintPure, Category = "HUD|Skill")
    float GetAimSkill2CooldownPercent() const
    {
        return (SkillComponent) ? SkillComponent->GetAimSkill2CooldownPercent() : 0.0f;
    }

    UFUNCTION(BlueprintPure, Category = "HUD|Skill")
    float GetAimSkill3CooldownPercent() const
    {
        return (SkillComponent) ? SkillComponent->GetAimSkill3CooldownPercent() : 0.0f;
    }

    // 텔레포트 쿨타임 중계 함수
    UFUNCTION(BlueprintPure, Category = "HUD|Skill")
    float GetTeleportCooldownPercent() const
    {
        return (MeleeCombatComponent) ?
            MeleeCombatComponent->GetTeleportCooldownPercent() : 0.0f;
    }

    /** (BlueprintCallable) UI 버튼에서 호출할 게임 재개 함수 */
    UFUNCTION(BlueprintCallable, Category = "Pause")
    void ResumeGame();

    /** (BlueprintCallable) UI 버튼에서 호출할 게임 재시작 함수 */
    UFUNCTION(BlueprintCallable, Category = "Pause")
    void HandleRestartGame();

    /** (BlueprintCallable) UI 버튼에서 호출할 메인 메뉴 이동 함수 */
    UFUNCTION(BlueprintCallable, Category = "Pause")
    void HandleBackToMainMenu();

    void UpdateMouseSensitivity(float NewSensitivity);

    void ApplyCameraShake();  // 화면 흔들림 함수 추가

    /** 크로스헤어 위젯을 강제로 표시합니다. (SkillComponent용) */
    void ShowCrosshairWidget();

    /** 크로스헤어 위젯을 강제로 숨깁니다. (SkillComponent용) */
    void HideCrosshairWidget();

    void PlayBigHitReaction();

    // 텔레포트 이펙트/사운드 Getter (정의가 포함된 최종 버전)
    FORCEINLINE class UNiagaraSystem* GetTeleportNiagaraEffect() const { return TeleportNiagaraEffect; }
    FORCEINLINE class USoundBase* GetTeleportSound() const { return TeleportSound; }

    FORCEINLINE class USoundBase* GetSkill1ReadySound() const { return Skill1ReadySound; }
    FORCEINLINE class USoundBase* GetSkill2ReadySound() const { return Skill2ReadySound; }
    FORCEINLINE class USoundBase* GetSkill3ReadySound() const { return Skill3ReadySound; }
    FORCEINLINE class USoundBase* GetAimSkill1ReadySound() const { return AimSkill1ReadySound; }
    FORCEINLINE class USoundBase* GetAimSkill2ReadySound() const { return AimSkill2ReadySound; }
    FORCEINLINE class USoundBase* GetAimSkill3ReadySound() const { return AimSkill3ReadySound; }
    FORCEINLINE class USoundBase* GetSkillCooldownSound() const { return SkillCooldownSound; }

    void PlayCooldownSound();

    UFUNCTION(BlueprintCallable, Category = "Sound")
    void PlayWavePrepareSound();

    UFUNCTION(BlueprintCallable, Category = "Combat|Reward")
    void GiveReward(float HealthAmount, int32 AmmoAmount);

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

    // 크로스헤어 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UCrossHairComponent* CrosshairComponent;

    // UMG 위젯 클래스
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UCrossHairWidget> CrossHairWidgetClass;

    // [신규 추가] 일시정지 메뉴 위젯 클래스
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> PauseMenuWidgetClass;

    // [신규 추가] 일시정지 메뉴에서도 로딩 스크린을 띄울 수 있도록 클래스 변수 추가
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> LoadingScreenWidgetClass;

    // [신규 추가] 퍼즈 메뉴에서 옵션 메뉴를 띄울 수 있도록 클래스 변수 추가
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> OptionsWidgetClass;

    // 위젯 인스턴스
    UPROPERTY()
    UCrossHairWidget* CrossHairWidget;

    // [추가] 플레이어 HUD 위젯 클래스
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> PlayerHUDWidgetClass;

    // [추가] 생성된 플레이어 HUD 위젯 인스턴스
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    UUserWidget* PlayerHUDWidget;

    // [신규 추가] 생성된 일시정지 메뉴 위젯 인스턴스
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    UUserWidget* PauseMenuWidget;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Teleport Effects")
    class UNiagaraSystem* TeleportNiagaraEffect = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Teleport Effects")
    class USoundBase* TeleportSound = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Kick Effects")
    float KickEffectOffset = 80.0f; // 이펙트를 전방으로 이동시킬 거리

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Knife Effects")
    float KnifeEffectOffset = 120.0f;

    UPROPERTY(EditDefaultsOnly, Category = "SoundEffects")
    class USoundBase* RewardSound = nullptr;

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
    float AimZoom = 70.0f;
    float MinZoom = 125.0f;
    float MaxZoom = 500.0f;
    float ZoomStep = 20.0f;
    float ZoomInterpSpeed = 10.0f;
    float CurrentZoom = DefaultZoom;
    float TargetZoom = DefaultZoom;
    float PreviousZoom = DefaultZoom; // 에임모드 진입 전 줌 값 저장하는 변수

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

    bool bCanAirAction = true; // 공중 액션 (점프, 점프공격) 가능 여부

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "React", meta = (AllowPrivateAccess = "true"))
    TArray<UAnimMontage*> NormalHitMontages; // 노말 히트 몽타주 저장하는 배열

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "React", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* BigHitMontage; // 빅 히트 몽타주

    // [신규] 빅 히트 리커버 몽타주
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "React", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* BigHitRecoverMontage; // 빅 히트 후 일어나는 몽타주

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "React", meta = (AllowPrivateAccess = "true"))
    TArray<UAnimMontage*> DieMontages; // 사망 몽타주 저장하는 배열

    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    TArray<USoundBase*> NormalHitSounds;

    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    USoundBase* BigHitSound;

    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    USoundBase* DieSound;

    UPROPERTY(EditDefaultsOnly, Category = "SoundEffects")
    class USoundBase* GameStartSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundEffects", meta = (AllowPrivateAccess = "true"))
    USoundBase* AimModeEnterSound;

    // 반동 변수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Recoil", meta = (AllowPrivateAccess = "true"))
    float VerticalRecoilMin = 1.0f;  // 수직 반동 최솟값

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Recoil", meta = (AllowPrivateAccess = "true"))
    float VerticalRecoilMax = 2.0f;  // 수직 반동 최댓값

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Recoil", meta = (AllowPrivateAccess = "true"))
    float HorizontalRecoilMin = -7.0f;  // 수평 반동 최솟값

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Recoil", meta = (AllowPrivateAccess = "true"))
    float HorizontalRecoilMax = 7.0f;   // 수평 반동 최댓값

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Recoil", meta = (AllowPrivateAccess = "true"))
    float RecoilDuration = 0.25f; // 반동 지속시간

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Recoil", meta = (AllowPrivateAccess = "true"))
    float RecoilRecoverySpeed = 5.0f; // 반동 회복속도

    // 격발 시 화면 흔들림 변수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", meta = (AllowPrivateAccess = "true"))
    float ShakeIntensity = 0.8f;  // 흔들림 강도

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", meta = (AllowPrivateAccess = "true"))
    float ShakeDuration = 0.15f;   // 흔들림 지속시간


    // 반동 관련 변수들
    FVector2D CurrentRecoil;
    FVector2D TargetRecoil;
    bool bIsRecoiling;
    FTimerHandle RecoilTimerHandle;

    void ApplyCameraRecoil();
    void ResetRecoil();
    void UpdateRecoil(float DeltaTime);

    // 이동속도 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float DefaultWalkSpeed = 700.0f; // 기본 이동속도

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float AimWalkSpeed = 500.0f; // 에임모드 시 이동속도

    void UpdateMovementSpeed(); // 이동속도 제어 함수

    bool bIsPlayingLandingAnimation = false;  // 착지 애니메이션 재생 중인지 확인
    FTimerHandle LandingAnimationTimerHandle; // 착지 애니메이션 타이머
    void OnLandingAnimationFinished();        // 착지 애니메이션 완료 콜백

    // [신규 추가] 일시정지 입력 액션
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* PauseAction;

    // [신규 추가] 일시정지 토글 함수
    void TogglePauseMenu();

    // [신규 추가] 레벨 로드를 공통으로 처리할 헬퍼 함수
    void ShowLoadingScreenAndLoad(FName LevelName);

    // [신규 추가] 타이머가 호출할 실제 레벨 로드 함수
    void LoadPendingLevel();

    // [신규 추가] 타이머가 레벨을 로드할 수 있도록 레벨 이름을 저장할 변수
    FName PendingLevelToLoad;

    float MouseSensitivityMultiplier = 1.0f;

    FTimerHandle BigHitRecoverTimerHandle; // [신규] 빅 히트 리커버 타이머 핸들

    void StartBigHitRecover(); // [신규] 빅 히트 리커버 몽타주 '시작' 함수

    // [삭제] UFUNCTION()
    // void OnBigHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    // [유지] 빅 히트 리커버 몽타주 종료 델리게이트
    UFUNCTION()
    void OnBigHitRecoverMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    // MainCharacter.h (protected: 또는 private: 섹션)

    // 킥 공격용 이펙트/사운드
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick Effects")
    class UNiagaraSystem* KickNiagaraEffect = nullptr; // [추가] 킥 나이아가라 이펙트

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick Effects")
    class USoundBase* KickHitSound = nullptr;       // [추가] 킥 히트 사운드

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundEffects|SkillReady", meta = (AllowPrivateAccess = "true"))
    class USoundBase* Skill1ReadySound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundEffects|SkillReady", meta = (AllowPrivateAccess = "true"))
    class USoundBase* Skill2ReadySound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundEffects|SkillReady", meta = (AllowPrivateAccess = "true"))
    class USoundBase* Skill3ReadySound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundEffects|SkillReady", meta = (AllowPrivateAccess = "true"))
    class USoundBase* AimSkill1ReadySound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundEffects|SkillReady", meta = (AllowPrivateAccess = "true"))
    class USoundBase* AimSkill2ReadySound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundEffects|SkillReady", meta = (AllowPrivateAccess = "true"))
    class USoundBase* AimSkill3ReadySound = nullptr;

    // 쿨타임 사운드 (통합)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundEffects|Cooldown", meta = (AllowPrivateAccess = "true"))
    class USoundBase* SkillCooldownSound = nullptr;

    // 사운드 중복 재생 방지를 위한 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SoundEffects|Cooldown", meta = (AllowPrivateAccess = "true"))
    class UAudioComponent* CooldownAudioComponent;

    UPROPERTY(EditDefaultsOnly, Category = "SoundEffects")
    class USoundBase* WavePrepareSound = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "PostProcess")
    class UPostProcessComponent* DeathPostProcessComponent;
};
