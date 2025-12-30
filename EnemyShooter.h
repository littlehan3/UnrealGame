#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" // ACharacter 클래스 상속
#include "EnemyShooterGun.h" // 장착할 총 클래스를 알아야 하므로 포함
#include "EnemyShooterAIController.h" // 사용할 AI 컨트롤러 클래스를 알아야 하므로 포함
#include "EnemyShooterAnimInstance.h" // 사용할 애님 인스턴스 클래스를 알아야 하므로 포함
#include "Animation/AnimInstance.h" // UAnimInstance 클래스 사용
#include "HealthInterface.h"
#include "EnemyShooter.generated.h"

class AEnemyShooterGrenade; // 수류탄 클래스 전방 선언

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyShooter : public ACharacter, public IHealthInterface
{
	GENERATED_BODY()

public:
	AEnemyShooter(); // 생성자

	virtual void PostInitializeComponents() override; // 컴포넌트 초기화 이후 호출되는 함수

	void PlaySpawnIntroAnimation(); // 스폰 시 등장 애니메이션을 재생하는 함수
	void SetupAI(); // AI 초기 설정을 수행하는 함수
	void PlayShootingAnimation(); // 사격 애니메이션을 재생하는 함수
	void PlayThrowingGrenadeAnimation(); // 수류탄 투척 애니메이션을 재생하는 함수

	// 데미지를 입었을때 호출되는 함수
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	void Die(); // 사망 처리 함수
	void ApplyBaseWalkSpeed(); // 기본 이동 속도를 적용하는 함수
	void HideEnemy(); // 사망 후 액터 정리 및 숨김 처리 함수
	void EnterInAirStunState(float Duration); // 공중 스턴 상태 진입 함수 
	void ExitInAirStunState(); // 공중 스턴 상태 해제 함수 
	void EnableGravityPull(FVector ExplosionCenter, float PullStrength); // (기존 ApplyGravityPull 이름 변경)
	void DisableGravityPull(); // (신규 추가)
	void Shoot(); // 장착된 총을 발사하는 함수

	// 판정 데이터 (현재 사용되지 않음)
	TSet<AActor*> RaycastHitActors;
	TSet<AActor*> DamagedActors;

	// MaxHealth 추가 및 기존 Health 변수 수정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 200.0f; // 최대 체력

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float Health = 200.0f;

	bool bIsPlayingIntro = false; // 등장 애니메이션 재생 여부
	bool bIsDead = false; // 사망 상태 여부
	bool bIsInAirStun = false; // 공중 스턴 상태 여부
	bool bCanAttack = false; // 공격 가능 여부 (쿨타임 등)
	bool bIsAttacking = false; // 현재 공격 애니메이션 재생 여부
	bool bIsShooting = false; // 현재 사격 중인지 여부

	// 장착할 무기 및 수류탄 클래스 (블루프린트에서 설정)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyShooterGun> GunClass; // 장착할 총의 원본 클래스

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	AEnemyShooterGun* EquippedGun; // 현재 장착된 총에 대한 참조

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AEnemyShooterGrenade> GrenadeClass; // 투척할 수류탄의 원본 클래스

	/** IHealthInterface 구현 함수 */
	virtual float GetHealthPercent_Implementation() const override;
	virtual bool IsEnemyDead_Implementation() const override;

	// [신규 추가] 중력장(블랙홀) 상태 변수 (AEnemy와 동일)
	bool bIsTrappedInGravityField = false;
	FVector GravityFieldCenter = FVector::ZeroVector;

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출

private:
	// 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* SpawnIntroMontage; // 등장 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ShootingMontage; // 사격 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> HitMontages; // 피격 애니메이션 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<UAnimMontage*> DeadMontages; // 사망 애니메이션 배열 (랜덤 재생용)

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* InAirStunMontage; // 공중 스턴 상태 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* InAirStunDeathMontage; // 공중 스턴 중 사망 애니메이션

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ThrowingGrenadeMontage; // 수류탄 투척 애니메이션

	// 몽타주 종료 시 호출될 함수들 (델리게이트 바인딩용)
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnShootingMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnThrowingGrenadeMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 타이머 핸들
	FTimerHandle StunTimerHandle; // 스턴 지속시간 제어용 타이머
	FTimerHandle DeathTimerHandle; // 사망 후 정리까지의 지연용 타이머

	void StopActions(); // 모든 행동을 즉시 중지시키는 함수
};