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

    void PlayAttackAnimation(); // 공격 애니메이션 실행 함수

protected:
    virtual void BeginPlay() override;

public:
    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator,
        AActor* DamageCauser
    ) override;

private:
    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* HitSound;

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* DieSound;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float Health = 100.0f;

    bool bIsDead = false;

    void Die();

    void SetUpAI(); // AI가 NavMesh에서 이동할 수 있도록 설정

    // 애니메이션 인스턴스 추가
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    UEnemyAnimInstance* EnemyAnimInstance;

    // 카타나 부착을 위한 변수 추가
    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    TSubclassOf<AEnemyKatana> KatanaClass;

    AEnemyKatana* EquippedKatana;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TArray<UAnimMontage*> AttackMontages; // 공격 몽타주를 저장하는 배열
};