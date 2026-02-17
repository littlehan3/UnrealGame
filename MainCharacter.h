#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "MainCharacter.generated.h"

class ARifle;
class AKnife;
class AMachineGun;
class ASkill3Projectile; 
class ACannon; 
class UMeleeCombatComponent;
class USkillComponent;
class UCrossHairComponent;
class UCrossHairWidget;
class UNiagaraSystem;
class USoundBase;
class UAnimMontage;
class UBoxComponent;
class UUserWidget;
class UPostProcessComponent;
class UAudioComponent;

UCLASS()
class LOCOMOTION_API AMainCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AMainCharacter();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void Landed(const FHitResult& Hit) override;

    // 스킬컴포넌트 외부에서 스킬 애니메이션 로직 수행 시 필요한 데이터에 접근할 수 있도록 getter
    // 빈번한 호출에 의한 오버헤드 방지를 위해 Force 인라인으로 정의
    // FORCE 인라인을 하면 코드 크기(Binary Size)는 커지지만 로직이 매우 짧고 호출 빈도가 극도로 높기 때문에 
    // 코드 비대화로 인한 캐시 미스 손실보다 함수 호출 오버헤드 제거로 얻는 이득이 더 크다고 판단하여 사용
	FORCEINLINE UAnimMontage* GetSkill1AnimMontage() const { return Skill1AnimMontage; } // 스킬1 애니메이션 몽타주 반환
	FORCEINLINE UAnimMontage* GetSkill2AnimMontage() const { return Skill2AnimMontage; } // 스킬2 애니메이션 몽타주 반환
	FORCEINLINE UAnimMontage* GetSkill3AnimMontage() const { return Skill3AnimMontage; } // 스킬3 애니메이션 몽타주 반환
	FORCEINLINE UAnimMontage* GetAimSkill1AnimMontage() const { return AimSkill1AnimMontage; } // 에임스킬1 애니메이션 몽타주 반환
	FORCEINLINE UAnimMontage* GetAimSkill2AnimMontage() const { return AimSkill2AnimMontage; } // 에임스킬2 애니메이션 몽타주 반환
	FORCEINLINE UAnimMontage* GetAimSkill2StartAnimMontage() const { return AimSkill2StartAnimMontage; } // 에임스킬2 시작 애니메이션 몽타주 반환
	FORCEINLINE UAnimMontage* GetAimSkill3AnimMontage() const { return AimSkill3AnimMontage; } // 에임스킬3 애니메이션 몽타주 반환

    // 스킬 컴포넌트에서 투사체를 스폰 할 수 있도록 (메인캐릭터 BP에서 수정할 수 있도록) getter
	FORCEINLINE TSubclassOf<ASkill3Projectile> GetSkill3ProjectileClass() const { return Skill3ProjectileClass; } // 스킬3 투사체 클래스 반환
    
    // BP나 다른 컴포넌트에서 캐릭터의 현재 상태를 알 수 있도록 getter
	FORCEINLINE bool IsDashing() const { return bIsDashing; } // 대시 상태 반환
	FORCEINLINE bool IsAiming() const { return bIsAiming; } // 조준 상태 반환
	FORCEINLINE bool IsJumping() const { return bIsJumping; } // 점프 상태 반환
	FORCEINLINE bool IsInDoubleJump() const { return bIsInDoubleJump; } // 더블점프 상태 반환

    // 캐릭터가 소유한 SkillComponent에 접근할 수 있도록 getter
	FORCEINLINE USkillComponent* GetSkillComponent() const { return SkillComponent; } // 스킬 컴포넌트 반환

    // 시각 효과, 사운드 자원을 외부 모듈과 공유할 수 있도록 getter
	FORCEINLINE class UNiagaraSystem* GetKickNiagaraEffect() const { return KickNiagaraEffect; } // 킥 이펙트 반환
	FORCEINLINE class USoundBase* GetKickHitSound() const { return KickHitSound; } // 킥 히트 사운드 반환
	FORCEINLINE float GetKickEffectOffset() const { return KickEffectOffset; } // 킥 이펙트 오프셋 반환
	FORCEINLINE float GetKnifeEffectOffset() const { return KnifeEffectOffset; } // 나이프 이펙트 오프셋 반환
	FORCEINLINE class UNiagaraSystem* GetTeleportNiagaraEffect() const { return TeleportNiagaraEffect; } // 텔레포트 이펙트 반환
	FORCEINLINE class USoundBase* GetTeleportSound() const { return TeleportSound; } // 텔레포트 사운드 반환

    void ExitAimMode(); // 조준 해제 시 시야와 무기 위치 초기화 함수

    // 스킬 입력 처리함수
    UFUNCTION() void UseSkill1();
    UFUNCTION() void UseSkill2();
    UFUNCTION() void UseSkill3();

    // 장비 탈부착 함수
    void AttachRifleToBack();
    void AttachRifleToHand();
    void AttachKnifeToBack();
    void AttachKnifeToHand();

    // 스탯 변수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float MaxHealth = 1000.0f; // 최대 체력
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float CurrentHealth; // 런타임 중 계산되는 현재 체력
    UPROPERTY(VisibleInstanceOnly, Category = "State|Combat")
    bool bIsDead = false; // 사망 여부 플래그
    UPROPERTY(VisibleInstanceOnly, Category = "State|Combat")
    bool bIsBigHitReacting = false; // 빅 히트 여부 플래그

    // 데미지 및 사망
    // 가상 함수 뒤에 override를 붙여 부모(엑터)의 함수를 재정의 보장
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
    void Die(); 

    // HUD 인터페이스
    UFUNCTION(BlueprintPure, Category = "Crosshair")
    UCrossHairComponent* GetCrosshairComponent() const { return CrosshairComponent; }  // 크로스헤어 컴포넌트 접근자 (반환 타입 수정)
	UFUNCTION(BlueprintPure, Category = "HUD") float GetHealthPercent() const; // 체력 비율 반환 함수
	UFUNCTION(BlueprintPure, Category = "HUD") int32 GetRifleCurrentAmmo() const; // 현재 탄약 수 반환 함수
	UFUNCTION(BlueprintPure, Category = "HUD") int32 GetRifleMaxAmmo() const; // 최대 탄약 수 반환 함수
	UFUNCTION(BlueprintPure, Category = "HUD") int32 GetRifleTotalAmmo() const; // 전체 탄약 수 반환 함수

    // UI 바인딩을 위한 스킬 쿨타임 Getter 함수들
    // // BP에서 직접 바인딩해서 사용할 수 있도록 UFUNCTION 매크로 사용
    UFUNCTION(BlueprintPure, Category = "HUD|Skill")
	float GetSkill1CooldownPercent() const; // 스킬 1 쿨타임 진행률 반환 함수
    UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
	float GetSkill2CooldownPercent() const; // 스킬 2 쿨타임 진행률 반환 함수
    UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
	float GetSkill3CooldownPercent() const; // 스킬 3 쿨타임 진행률 반환 함수
    UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
	float GetAimSkill1CooldownPercent() const; // 에임스킬 1 쿨타임 진행률 반환 함수
    UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
	float GetAimSkill2CooldownPercent() const; // 에임스킬 2 쿨타임 진행률 반환 함수
    UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
	float GetAimSkill3CooldownPercent() const; // 에임스킬 3 쿨타임 진행률 반환 함수
    UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
	float GetTeleportCooldownPercent() const; // 텔레포트 쿨타임 진행률 반환 함수

    // 시스템 제어
    // 메뉴 UI와 게임 세팅 관리
    UFUNCTION(BlueprintCallable, Category = "Pause")
    void ResumeGame(); // 일시정지 해제 함수
    UFUNCTION(BlueprintCallable, Category = "Pause")
    void HandleRestartGame(); // 게임 재시작 함수
    UFUNCTION(BlueprintCallable, Category = "Pause")
    void HandleBackToMainMenu(); // 메인 메뉴 이동 함수
    void UpdateMouseSensitivity(float NewSensitivity); // 마우스 감도 설정 함수

    void ApplyCameraShake();  // 화면 흔들림 함수 추가
    void ShowCrosshairWidget(); // 에임 스킬 사용 시 크로스헤어 강제 표시 함수
    void HideCrosshairWidget(); // 스킬 종류 시 크로스헤어 숨김 함수
    void PlayBigHitReaction(); // 빅히트 함수

    //// 텔레포트 이펙트/사운드 Getter (정의가 포함된 최종 버전)
    //FORCEINLINE class UNiagaraSystem* GetTeleportNiagaraEffect() const { return TeleportNiagaraEffect; }
    //FORCEINLINE class USoundBase* GetTeleportSound() const { return TeleportSound; }

    // 스킬 쿨타임 및 준비 사운드 자원을 제공할 수 있도록 getter
    FORCEINLINE class USoundBase* GetSkill1ReadySound() const { return Skill1ReadySound; }
    FORCEINLINE class USoundBase* GetSkill2ReadySound() const { return Skill2ReadySound; }
    FORCEINLINE class USoundBase* GetSkill3ReadySound() const { return Skill3ReadySound; }
    FORCEINLINE class USoundBase* GetAimSkill1ReadySound() const { return AimSkill1ReadySound; }
    FORCEINLINE class USoundBase* GetAimSkill2ReadySound() const { return AimSkill2ReadySound; }
    FORCEINLINE class USoundBase* GetAimSkill3ReadySound() const { return AimSkill3ReadySound; }
    FORCEINLINE class USoundBase* GetSkillCooldownSound() const { return SkillCooldownSound; }

    void PlayCooldownSound(); // 쿨타임 미충족시 사운드 재생 함수
    void PlayWavePrepareSound(); // 웨이브 시작 예고 사운드 재생 함수
    UFUNCTION(BlueprintCallable, Category = "Combat|Reward")
    void GiveReward(float HealthAmount, int32 AmmoAmount); // 각 웨이브 클리어마다 보상으로 체력과 탄약을 지급해주는 함수

protected:
    virtual void BeginPlay() override;

    void Move(const FInputActionValue& Value); // 이동 벡터를 입력받아 처리하는 함수
    void Look(const FInputActionValue& Value); // 시선 회전을 제어하는 함수
    void HandleJump(); // 일반, 더블 점프 함수
    void HandleDoubleJump(); // 2단 점프시 중력 마찰력 조절 함수
    void EnterAimMode(); // 조준 모드 진입 시 카메라 속도 변경 함수
    void FireWeapon(); // 무기 발사, 콤보 공격 함수
    void ReloadWeapon(); // 장전 명령을 rifle에 전달하는 함수
    void ComboAttack(); // 근접 공격 시스템 함수

    // UI 구성요소
    // BP에서 위젯 클래스를 할당받아서 관리
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UCrossHairComponent* CrosshairComponent; // 크로스헤어 컴포넌트
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UCrossHairWidget> CrossHairWidgetClass; // UMG 위젯 클래스
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> PauseMenuWidgetClass; // 일시정지 메뉴 위젯 클래스
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> LoadingScreenWidgetClass; // 일시정지 메뉴에서도 로딩 스크린을 띄울 수 있도록 하는 클래스
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> OptionsWidgetClass; // 일시정지 메뉴에서 옵션 메뉴를 띄울 수 있도록 하는 클래스 변수
    UPROPERTY()
    UCrossHairWidget* CrossHairWidget; // 크로스헤어 위젯 인스턴스
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> PlayerHUDWidgetClass; // 플레이어 HUD 위젯 클래스
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    UUserWidget* PlayerHUDWidget; // 생성된 플레이어 HUD 위젯 인스턴스
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    UUserWidget* PauseMenuWidget; // 생성된 일시정지 메뉴 위젯 인스턴스

    // 전투 이펙트
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Teleport Effects")
    class UNiagaraSystem* TeleportNiagaraEffect = nullptr; // 텔레포트 공격 나이아가라 이펙트
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Teleport Effects")
    class USoundBase* TeleportSound = nullptr; // 텔레포트 공격 사운드
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Kick Effects")
    float KickEffectOffset = 80.0f; // 발차기 이펙트를 전방으로 이동시킬 거리
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Knife Effects")
    float KnifeEffectOffset = 120.0f; // 나이프 이펙트를 전방으로 이동시킬 거리
    UPROPERTY(EditDefaultsOnly, Category = "SoundEffects")
    class USoundBase* RewardSound = nullptr; // 웨이브 클리어 보상 사운드

    // 무기 부착 시 상대 위치 및 회전값을 에디터에 수정가능하게 선언
    UPROPERTY(EditAnywhere, Category = "Combat|SocketSettings")
    FVector RifleBackLocation = FVector(5.0f, 15.0f, -9.0f);
    UPROPERTY(EditAnywhere, Category = "Combat|SocketSettings")
    FRotator RifleBackRotation = FRotator(0.0f, -45.0f, 0.0f);
    UPROPERTY(EditAnywhere, Category = "Combat|SocketSettings")
	FVector LeftKnifeBackLocation = FVector(-3.0f, 13.5f, 0.0f);
	UPROPERTY(EditAnywhere, Category = "Combat|SocketSettings")
	FRotator LeftKnifeBackRotation = FRotator(45.0f, 0.0f, 0.0f);
    UPROPERTY(EditAnywhere, Category = "Combat|SocketSettings")
    FVector RightKnifeBackLocation = FVector(5.0f, 13.5f, 0.0f);
    UPROPERTY(EditAnywhere, Category = "Combat|SocketSettings")
    FRotator RightKnifeBackRotation = FRotator(-90.0f, 0.0f, 0.0f);

private:
    // 애님인스턴스 캐싱을 위한 변수 선언
	// 기존에는 Tick에서 매 프레임마다 GetAnimInstance 호출로 가져왔음
    UPROPERTY()
    class UCustomAnimInstance* CachedAnimInstance = nullptr;

    // 캐릭터 시점을 구성하기 위한 선언
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    class USpringArmComponent* CameraBoom = nullptr; // 스프링암 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    class UCameraComponent* Camera = nullptr; // 카메라 컴포넌트

    // 인헨스드 인풋 시스템의 액션을 매칭하기 위한 선언
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputMappingContext* DefaultMappingContext = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* JumpAction = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* MoveAction = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* LookAction = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* AimAction = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* FireAction = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ReloadAction = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* DashAction = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ZoomInAction = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ZoomOutAction = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* Skill1Action = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* Skill2Action = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* Skill3Action = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* PauseAction = nullptr;

    // 장착된 무기들의 클래스와 인턴스 포인터들
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<ARifle> RifleClass;
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<AKnife> KnifeClass_L;
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<AKnife> KnifeClass_R;

    UPROPERTY()
    class ARifle* Rifle = nullptr;
    UPROPERTY()
    AKnife* LeftKnife = nullptr;
    UPROPERTY()
    AKnife* RightKnife = nullptr;

    // 각 상황에 맞는 몽타주 에셋들
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ComboAttackMontage1 = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ComboAttackMontage2 = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ComboAttackMontage3 = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ComboAttackMontage4 = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ComboAttackMontage5 = nullptr;

    // ABP 공유용 스탯 틱 마다 갱신하여 애니메이션을 결정하는 변수들
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State", meta = (AllowPrivateAccess = "true"))
    float Speed = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State", meta = (AllowPrivateAccess = "true"))
    float Direction = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State", meta = (AllowPrivateAccess = "true"))
    float AimPitch = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State", meta = (AllowPrivateAccess = "true"))
    bool bIsJumping = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State", meta = (AllowPrivateAccess = "true"))
    bool bIsInDoubleJump = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State", meta = (AllowPrivateAccess = "true"))
    bool bIsInAir = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State", meta = (AllowPrivateAccess = "true"))
    bool bCanDoubleJump = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State", meta = (AllowPrivateAccess = "true"))
    bool bIsAiming = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* KickHitBox = nullptr; // 발차기 히트박스 컴포넌트

    // 대쉬 및 스킬 몽타주들
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* ForwardDashMontage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* LeftDashMontage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* RightDashMontage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* BackwardDashMontage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill1 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* Skill1AnimMontage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill2 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* Skill2AnimMontage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill3 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* Skill3AnimMontage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AimSkill1 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* AimSkill1AnimMontage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AimSkill2 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* AimSkill2AnimMontage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AimSkill2 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* AimSkill2StartAnimMontage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AimSkill3 Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* AimSkill3AnimMontage = nullptr;

    // 대쉬 로직 및 변수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash", meta = (AllowPrivateAccess = "true"))
    float DashCooldown = 1.0f; // 쿨타임
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Dash", meta = (AllowPrivateAccess = "true"))
    bool bIsDashing = false; // 대쉬 중 여부
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Dash", meta = (AllowPrivateAccess = "true"))
    bool bCanDash = true; // 대쉬 가능 여부
	FTimerHandle DashCooldownTimerHandle; // 대쉬 쿨타임 타이머 핸들
    UFUNCTION()
    void Dash(); // 대쉬 함수
    void PlayDashMontage(UAnimMontage* DashMontage); // 대쉬 몽타주 재생 함수
    UFUNCTION()
	void ResetDash(UAnimMontage* Montage, bool bInterrupted); // 대쉬 상태 초기화 함수
    UFUNCTION()
	void ResetDashCooldown(); // 대쉬 쿨타임 초기화 함수

	// 줌 제어 변수 및 함수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom", meta = (AllowPrivateAccess = "true"))
	float DefaultZoom = 250.0f; // 기본 줌 값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom", meta = (AllowPrivateAccess = "true"))
	float AimZoom = 70.0f; // 조준 모드 줌 값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom", meta = (AllowPrivateAccess = "true"))
	float MinZoom = 125.0f; // 최소 줌 값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom", meta = (AllowPrivateAccess = "true"))
	float MaxZoom = 500.0f; // 최대 줌 값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom", meta = (AllowPrivateAccess = "true"))
	float ZoomStep = 20.0f; // 줌 인/아웃 시 변화량
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom", meta = (AllowPrivateAccess = "true"))
	float ZoomInterpSpeed = 10.0f; // 줌 보간 속도

	float CurrentZoom = DefaultZoom; // 현재 줌 값
	float TargetZoom = DefaultZoom; // 목표 줌 값
    float PreviousZoom = DefaultZoom; // 에임모드 진입 전 줌 값 저장하는 변수
	void ZoomIn(); // 줌 인 함수
	void ZoomOut(); // 줌 아웃 함수

	// 착지, 회전 내부 로직 변수 및 함수
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State", meta = (AllowPrivateAccess = "true"))
	bool bIsLanding = false; // 착지 상태 플래그
	FTimerHandle LandingTimerHandle; // 착지 타이머 핸들
    UFUNCTION()
	void ResetLandingState(); // 착지 상태 초기화 함수
	bool bApplyRootMotionRotation = false; // 루트 모션 회전 적용 여부
	FRotator TargetRootMotionRotation; // 목표 루트 모션 회전 값

	// 스킬 사용 시 소환되는 무기 및 투사체 컴포넌트
    UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<ASkill3Projectile> Skill3ProjectileClass;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    UMeleeCombatComponent* MeleeCombatComponent = nullptr;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    USkillComponent* SkillComponent = nullptr;
    UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<AMachineGun> MachineGunClass;
    UPROPERTY()
    AMachineGun* MachineGun = nullptr;
    UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<ACannon> CannonClass;
    UPROPERTY()
    ACannon* Cannon = nullptr;

	// 피격 및 사망 몽타주와 사운드
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "React", meta = (AllowPrivateAccess = "true"))
    TArray<UAnimMontage*> NormalHitMontages; // 노말 히트 몽타주 저장하는 배열
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "React", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* BigHitMontage = nullptr; // 빅 히트 몽타주
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "React", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* BigHitRecoverMontage = nullptr; // 빅 히트 후 일어나는 몽타주
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "React", meta = (AllowPrivateAccess = "true"))
    TArray<UAnimMontage*> DieMontages; // 사망 몽타주 저장하는 배열
    UPROPERTY(EditAnywhere, Category = "SoundEffects")
	TArray<USoundBase*> NormalHitSounds; // 노말 히트 사운드 배열
    UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BigHitSound = nullptr; // 빅 히트 사운드
    UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* DieSound = nullptr; // 사망 사운드

    UPROPERTY(EditDefaultsOnly, Category = "SoundEffects")
	class USoundBase* GameStartSound = nullptr; // 게임 시작 사운드
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundEffects", meta = (AllowPrivateAccess = "true"))
	USoundBase* AimModeEnterSound = nullptr; // 조준 모드 진입 사운드

	// 사격시 카메라 반동 시스템 변수
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

    // 반동 내부 계산 변수
    UPROPERTY(VisibleInstanceOnly, Category = "State|Camera")
	FVector2D CurrentRecoil; // 현재 반동 값
    UPROPERTY(VisibleInstanceOnly, Category = "State|Camera")
	FVector2D TargetRecoil; // 목표 반동 값
    UPROPERTY(VisibleInstanceOnly, Category = "State|Movement")
	bool bIsRecoiling;  // 반동 적용 중인지 여부
	FTimerHandle RecoilTimerHandle; // 반동 타이머 핸들
	void ApplyCameraRecoil(); // 카메라 반동 적용 함수
	void ResetRecoil(); // 반동 초기화 함수
	void UpdateRecoil(float DeltaTime); // 매 틱마다 반동 업데이트 함수

	// 이동속도 정의 및 제어
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float DefaultWalkSpeed = 700.0f; // 기본 이동속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float AimWalkSpeed = 500.0f; // 에임모드 시 이동속도
    void UpdateMovementSpeed(); // 이동속도 제어 함수

	// 착지 애니메이션 관련 변수 및 함수
    UPROPERTY(VisibleInstanceOnly, Category = "State|Movement")
    bool bIsPlayingLandingAnimation = false; // 착지 애니메이션 재생 중인지 확인
    FTimerHandle LandingAnimationTimerHandle; // 착지 애니메이션 타이머
    UFUNCTION()
    void OnLandingAnimationFinished(); // 착지 애니메이션 완료 콜백

	// 레벨 및 로딩 관리
	void TogglePauseMenu(); // 일시정지 메뉴 토글 함수
	void ShowLoadingScreenAndLoad(FName LevelName); // 로딩 스크린 표시 및 레벨 로드 함수
	void LoadPendingLevel(); // 레벨 로드 함수
	FName PendingLevelToLoad; // 로드할 대기 중인 레벨 이름 변수

	// 마우스 감도 제어 변수
    UPROPERTY(VisibleInstanceOnly, Category = "State|Input")
	float MouseSensitivityMultiplier = 1.0f; // 마우스 감도 배율 변수

	// 빅 히트 리커버 몽타주 관련 변수 및 함수
    FTimerHandle BigHitRecoverTimerHandle; // 빅 히트 리커버 타이머 핸들
    UFUNCTION()
    void StartBigHitRecover(); // 빅 히트 리커버 시작 함수
    UFUNCTION() 
    void OnBigHitRecoverMontageEnded(UAnimMontage* Montage, bool bInterrupted);  // 빅 히트 리커버 몽타주 종료 델리게이트

    // 킥 공격용 이펙트/사운드
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick Effects", meta = (AllowPrivateAccess = "true"))
    class UNiagaraSystem* KickNiagaraEffect = nullptr; // 킥 나이아가라 이펙트
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick Effects", meta = (AllowPrivateAccess = "true"))
    class USoundBase* KickHitSound = nullptr; // 킥 히트 사운드

	// 스킬 쿨타임 및 준비 사운드 자원
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundEffects|Cooldown", meta = (AllowPrivateAccess = "true"))
    class USoundBase* SkillCooldownSound = nullptr;  // 쿨타임 사운드 (통합)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SoundEffects|Cooldown", meta = (AllowPrivateAccess = "true"))
    class UAudioComponent* CooldownAudioComponent; // 사운드 중복 재생 방지를 위한 컴포넌트
    UPROPERTY(EditDefaultsOnly, Category = "SoundEffects", meta = (AllowPrivateAccess = "true"))
	class USoundBase* WavePrepareSound = nullptr; // 웨이브 시작 예고 사운드

	// 사망 시 포스트 프로세스 효과
    UPROPERTY(VisibleAnywhere, Category = "PostProcess", meta = (AllowPrivateAccess = "true"))
	class UPostProcessComponent* DeathPostProcessComponent = nullptr; // 사망 시 포스트 프로세스 컴포넌트
};
