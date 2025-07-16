#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossEnemyAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "BossEnemy.generated.h"

UCLASS()
class LOCOMOTION_API ABossEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	ABossEnemy();

	virtual void PostInitializeComponents() override; // AI 이동

	void PlayBossNormalAttackAnimation(); // 일반공격 애니메이션 실행 함수
	void PlayBossUpperBodyAttack(); // 상체 전용 애니메이션 실행 함수
	
	virtual float TakeDamage( // 데미지를 입었을때 호출되는 함수 (AActor의 TakeDamage 오버라이드)
		float DamageAmount, // 입은 데미지 양
		struct FDamageEvent const& DamageEvent, // 데미지 이벤트 정보
		class AController* EventInstigator, // 데미지를 가한 컨트롤러
		AActor* DamageCauser // 데미지를 유발한 엑터
	) override;

	bool bIsBossDead = false; // 사망 여부
	bool bIsBossStrongAttacking = false; // 강공격중 여부
	bool bCanBossAttack = false; // 공격 가능 여부
	bool bIsBossHit = false; // 피격중 여부
	bool bIsFullBodyAttacking = false;// 전신공격중 여부

	float BossHealth = 200.0f; // 체력

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossHitSound; // 피격 사운드

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossDieSound; // 사망 사운드

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossNormalAttackSound; // 일반공격 사운드

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* BossStrongAttackSound; // 강공격 사운드

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UBossEnemyAnimInstance* BossEnemyAnimInstance; // 애니메이션 인스턴스 추가

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossNormalAttackMontages; // 일반 공격 몽타주 배열

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossUpperBodyMontages; // 상체 전용 몽타주 배열

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossHitReactionMontages; // 피격 몽타주 배열

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> BossDeadMontages; // 사망 몽타주 배열

	//float BossHealth = 200.0f; // 체력

	void BossDie(); // 사망 함수
	void SetUpBossAI(); // AI가 네브매쉬에서 이동할수있게 설정하는 함수
	void StopBossActions(); // 모든 동작 정지 함수
	void HideBossEnemy(); // 사망시 보스를 숨기는 함수
	FTimerHandle BossDeathHideTimerHandle; // 사망 몽타주 타이머
	void DisableBossMovement();
	void EnableBossMovement();

	UFUNCTION()
	void OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 일반공격 몽타주 델리게이트

	UFUNCTION()
	void OnUpperBodyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 상체공격 몽타주 델리게이트

	UFUNCTION()
	void OnHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 히트 몽타주 델리게이트

	UFUNCTION()
	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 사망 몽타주 델리게이트
};
