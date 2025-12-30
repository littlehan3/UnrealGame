#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyKatana.h" 
#include "EnemyAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "AimSkill2Projectile.h"
#include "HealthInterface.h"
#include "Enemy.generated.h"

//class AEnemyKatana;
class AMainGameModeBase;
class UNiagaraSystem;

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemy : public ACharacter, public IHealthInterface
{
    GENERATED_BODY()

public:
    AEnemy();

    virtual void PostInitializeComponents() override; // AI 컨트롤러 확인 함수

    void PlayNormalAttackAnimation(); // 일반 공격 애니메이션 실행 함수
    void PlayDodgeAnimation(bool bDodgeLeft); // 닷지 애니메이션 실행
    float GetDodgeLeftDuration() const;  // 왼쪽 닷지 몽타주 길이 반환
    float GetDodgeRightDuration() const; // 오른쪽 닷지 몽타주 길이 반환

    void PlayStrongAttackAnimation(); // 강 공격 애니메이션 실행 함수
    void PlayJumpAttackAnimation(); // 점프 공격 애니메이션 실행 함수
    float GetJumpAttackDuration() const; // 점프 공격 몽타주 길이 반환

    bool bIsDead = false;

    virtual float TakeDamage( // 데미지를 입었을때 호출되는 함수 (AActor의 TakeDamage 오버라이드)
        float DamageAmount, // 입은 데미지 양
        struct FDamageEvent const& DamageEvent, // 데미지 이벤트 정보 
        class AController* EventInstigator, // 데미지를 가한 컨트롤러
        AActor* DamageCauser // 데미지를 유발한 엑터
    ) override;

    void EnterInAirStunState(float Duration); // 공중 스턴 상태 진입

    bool bIsInAirStun = false; // 공중 스턴 상태 여부

    void EnableGravityPull(FVector ExplosionCenter, float PullStrength); // 중력장으로 적을 잡아둠
    void DisableGravityPull(); // 중력장 효과 해제

    bool bIsTrappedInGravityField = false; // 중력장 상태 여부
    FVector GravityFieldCenter = FVector::ZeroVector; // 중력장 중심 좌표

    // 공격 시작/종료 함수
    void StartAttack(EAttackType AttackType); // true: 강공격, false: 일반공격
    void EndAttack();

    bool bIsStrongAttack = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    AEnemyKatana* EquippedKatana; // 무기 참조 

    void OnDodgeLaunchNotify(bool bDodgeLeft);
    void SetRootMotionEnable(bool bEnable);
    void OnDodgeLaunchEndNotify();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EnemyType")
    bool bIsEliteEnemy = false; // 앨리트 적 여부

    void ApplyBaseWalkSpeed();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float MaxHealth = 100.0f; // [추가] MaxHealth (기존 Health를 CurrentHealth로 사용)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float Health = 100.0f; // (CurrentHealth로 사용)

    /** 현재 체력 비율 (0.0 ~ 1.0)을 반환합니다. */
    virtual float GetHealthPercent_Implementation() const override;

    /** 현재 액터가 죽었는지 여부를 반환합니다. */
    virtual bool IsEnemyDead_Implementation() const override;

    void PlaySpawnIntroAnimation(); // 등장 애니메이션 재생 함수
    bool bIsPlayingIntro = false; // 등장 애니메이션 재생 중 여부

    void Die();

    void PlayWeaponHitSound();

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* HitReactionMontage; // 피격 애니메이션

    FORCEINLINE class UNiagaraSystem* GetWeaponHitNiagaraEffect() const { return WeaponHitNiagaraEffect; }

protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    USoundBase* HitSound;

    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    USoundBase* DieSound;

    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    USoundBase* NormalAttackSound;

    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    USoundBase* StrongAttackSound;

    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    USoundBase* DodgeSound;

    bool bCanAttack = false; // 공격가능 상태 추적을 위한 변수

    void SetUpAI(); // AI가 NavMesh에서 이동할 수 있도록 설정

    void StopActions(); // 모든 동작 정지
    FTimerHandle DeathTimerHandle; // 사망 애니메이션 처리를 위한 타이머

    // 애니메이션 인스턴스 추가
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    UEnemyAnimInstance* EnemyAnimInstance;

    // 카타나 부착을 위한 변수 추가
    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    TSubclassOf<AEnemyKatana> KatanaClass;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TArray<UAnimMontage*> NormalAttackMontages; // 일반 공격 몽타주를 저장하는 배열

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* StrongAttackMontage; // 강공격 몽타주

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* DodgeLeftMontage; // 왼쪽 닷지 몽타주

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* DodgeRightMontage; // 오른쪽 닷지 몽타주

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TArray<UAnimMontage*> JumpAttackMontages; // 점프 공격 애니메이션

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* DeadMontage; // 사망 애니메이션

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* InAirStunMontage; // 공중스턴 애니메이션

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* InAirStunDeathMontage; // 공중스턴 사망 애니메이션

    void ExitInAirStunState(); // 공중 스턴 상태 해제
    FTimerHandle StunTimerHandle; // 스턴 상태 해제를 위한 타이머
    void HideEnemy(); // 적 숨기기
    void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 에어본 상태에서 히트시 상태를 관리하기 위한 함수

    UPROPERTY()
    UAnimMontage* LastPlayedJumpAttackMontage;

    void ApplyEliteSettings();

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* SpawnIntroMontage; // 일반 적 등장 몽타주

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* EliteSpawnIntroMontage; // 엘리트 적 등장 몽타주

    UFUNCTION()
    void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 등장 몽타주 종료 델리게이트

    UPROPERTY(EditAnywhere, Category = "Effets")
    USoundBase* EnemyWeaponHitSound;

    UPROPERTY(EditDefaultsOnly, Category = "FX")
    class UNiagaraSystem* WeaponHitNiagaraEffect = nullptr;
};
