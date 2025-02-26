#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyKatana.h" 
#include "LockOnComponent.h"
#include "EnemyAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "Enemy.generated.h"

class AEnemyKatana;

UCLASS()
class LOCOMOTION_API AEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemy();

    // 락온이 가능한 대상인지 확인하는 변수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock-On")
    bool bCanBeLockedOn;

    // 락온 가능 여부를 결정하는 함수 추가
    bool CanBeLockedOn() const;

    virtual void PostInitializeComponents() override; // AI 이동

    void PlayNormalAttackAnimation(); // 일반 공격 애니메이션 실행 함수
    void PlayDodgeAnimation(bool bDodgeLeft); // 닷지 애니메이션 실행
    float GetDodgeLeftDuration() const;  // 왼쪽 닷지 몽타주 길이 반환
    float GetDodgeRightDuration() const; // 오른쪽 닷지 몽타주 길이 반환

    void PlayStrongAttackAnimation(); // 강 공격 애니메이션 실행 함수
    void PlayJumpAttackAnimation(); // 점프 공격 애니메이션 실행 함수
    float GetJumpAttackDuration() const; // 점프 공격 몽타주 길이 반환

protected:
    virtual void BeginPlay() override;

public:
    bool bIsDead = false;

    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator,
        AActor* DamageCauser
    ) override;

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

    UPROPERTY(EditAnywhere, Category = "Combat")
    float Health = 100.0f;

    bool bCanAttack = false; // 공격가능 상태 추적을 위한 변수

    void Die();

    void SetUpAI(); // AI가 NavMesh에서 이동할 수 있도록 설정

    void StopActions(); // 모든 동작 정지
    void FreezeDeadPose(); // 사망 후 물리 효과 활성화(시체 유지)
    FTimerHandle DeathTimerHandle; // 사망 애니메이션 완료 처리를 위한 타이머

    // 애니메이션 인스턴스 추가
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    UEnemyAnimInstance* EnemyAnimInstance;

    // 카타나 부착을 위한 변수 추가
    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    TSubclassOf<AEnemyKatana> KatanaClass;

    AEnemyKatana* EquippedKatana;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TArray<UAnimMontage*> NormalAttackMontages; // 일반 공격 몽타주를 저장하는 배열

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* StrongAttackMontage; // 강공격 몽타주

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* DodgeLeftMontage; // 왼쪽 닷지 몽타주

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* DodgeRightMontage; // 오른쪽 닷지 몽타주

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* JumpAttackMontage; // 점프 공격 애니메이션

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* HitReactionMontage; // 피격 애니메이션

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* DeadMontage; // 사망 애니메이션

};