#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "SkillComponent.generated.h"

class AMainCharacter;
class UAnimMontage;
class AEnemy;
class ASkill3Projectile;
class AMachineGun;
class AKnife;
class UBoxComponent;
class ACannon;
class AAimSkill3Projectile;
class USkeletalMeshComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOCOMOTION_API USkillComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USkillComponent();
	// 캐릭터 생성 후 무기 및 컴포넌트 참조를 안전하게 연결하는 초기화 함수
    void InitializeSkills(AMainCharacter* InCharacter, AMachineGun* InMachineGun, AKnife* InLeftKnife, AKnife* InRightKnife, UBoxComponent* InKickHitBox, ACannon* InCannon);

	// 스킬 실행 함수들
    void UseSkill1();
    void UseSkill2();
    void UseSkill3();
    void UseAimSkill1();
    void UseAimSkill2();
    void UseAimSkill3();

	// 스킬 사용 상태 확인용 Getter 함수들
    // 메인캐릭터와 마찬가지로 빈번한 호출에 의한 오버헤드 방지를 위해 Force 인라인으로 정의
	FORCEINLINE bool IsUsingSkill1() const { return bIsUsingSkill1; } // 스킬1 사용 상태 반환
	FORCEINLINE bool IsUsingSkill2() const { return bIsUsingSkill2; } // 스킬2 사용 상태 반환
	FORCEINLINE bool IsUsingSkill3() const { return bIsUsingSkill3; } // 스킬3 사용 상태 반환
	FORCEINLINE bool IsUsingAimSkill1() const { return bIsUsingAimSkill1; } // 에임스킬1 사용 상태 반환
	FORCEINLINE bool CanUseAimSkill1() const { return bCanUseAimSkill1; } // 에임스킬1 사용 가능 여부 반환
	FORCEINLINE bool IsUsingAimSkill2() const { return bIsUsingAimSkill2; } // 에임스킬2 사용 상태 반환
	FORCEINLINE bool CanUseAimSkill2() const { return bCanUseAimSkill2; } // 에임스킬2 사용 가능 여부 반환
	FORCEINLINE bool IsUsingAimSkill3() const { return bIsUsingAimSkill3; } // 에임스킬3 사용 상태 반환
	FORCEINLINE bool CanUseAimSkill3() const { return bCanUseAimSkill3; } // 에임스킬3 사용 가능 여부 반환

	void CancelAllSkills(); // 피격 및 상태 이상 시 모든 스킬 취소
    bool IsCastingSkill() const; // 스킬 시전 상태 체크 (UI, 애니메이션 블랜딩용)

    // UI 바인딩을 위한 스킬 쿨타임 Getter 함수들
    // 쿨타임 진행률 계산 시 분모 0 체크 및 타이머 유효성 검사
	// BP에서 직접 바인딩해서 사용할 수 있도록 UFUNCTION 매크로 사용
    UFUNCTION(BlueprintPure, Category = "Skill|Cooldown")
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


    void CancelAimSkill1ByDash(); // 대쉬로 에임스킬1을 캔슬하는 함수

	// 스킬 사용 가능 여부 반환 getter 함수들
    FORCEINLINE bool CanUseSkill1() const { return bCanUseSkill1; } // 스킬1 사용 가능 여부 반환
    FORCEINLINE bool CanUseSkill2() const { return bCanUseSkill2; } // 스킬2 사용 가능 여부 반환
    FORCEINLINE bool CanUseSkill3() const { return bCanUseSkill3; } // 스킬3 사용 가능 여부 반환

protected:

private:
    // 참조 포인터들
	// GC가 추적 가능하게 UPROPERTY() 매크로 사용
	UPROPERTY()
    AMainCharacter* OwnerCharacter = nullptr;
	UPROPERTY()
    AKnife* LeftKnife = nullptr;
	UPROPERTY()
    AKnife* RightKnife = nullptr;
	UPROPERTY()
    AMachineGun* MachineGun = nullptr;
	UPROPERTY()
    UBoxComponent* KickHitBox = nullptr;
	UPROPERTY()
    ACannon* Cannon = nullptr;

	// 스킬 데이터 및 상태 변수들
	// 해당 함수들이 리플렉션 시스템에 등록되게 하여 안전하게 호출하기 위해 타이머 및 델리게이트 콜백 함수는 UFUNCTION() 매크로 사용
    // 스킬1
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsUsingSkill1 = false; // 스킬1 사용 중 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanUseSkill1 = true; // 스킬1 사용 가능 여부
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float Skill1Cooldown = 7.0f; // 스킬1 쿨타임
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float Skill1Range = 400.0f; // 스킬1 범위
	FTimerHandle Skill1CooldownHandle; // 스킬1 쿨타임 타이머 핸들
	FTimerHandle Skill1EffectHandle; // 스킬1 효과 타이머 핸들
	UPROPERTY()
	UAnimMontage* Skill1Montage; // 스킬1 애니메이션 몽타주
	void PlaySkill1Montage(); // 스킬1 몽타주 재생
	UFUNCTION()
	void ApplySkill1Effect(); // 스킬1 효과 적용
	UFUNCTION()
	void ResetSkill1(UAnimMontage* Montage, bool bInterrupted); // 스킬1 리셋
	UFUNCTION()
	void ResetSkill1Cooldown(); // 스킬1 쿨타임 리셋
   //void DrawSkill1Range(); // 스킬1 범위 시각화

    // 스킬2
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsUsingSkill2 = false; // 스킬2 사용 중 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanUseSkill2 = true; // 스킬2 사용 가능 여부
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float Skill2Cooldown = 5.0f; // 스킬2 쿨타임
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float Skill2Damage = 50.0f; // 스킬2 데미지
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float Skill2Range = 200.0f; // 스킬2 범위
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float Skill2EffectDelay = 0.5f; // 스킬2 효과 적용 지연 시간
	FTimerHandle Skill2CooldownHandle; // 스킬2 쿨타임 타이머 핸들
	FTimerHandle Skill2EffectHandle; // 스킬2 효과 타이머 핸들
	FTimerHandle Skill2RangeClearHandle; // 스킬2 범위 시각화 제거 타이머 핸들
	UPROPERTY()
	UAnimMontage* Skill2Montage; // 스킬2 애니메이션 몽타주
	void PlaySkill2Montage(); // 스킬2 몽타주 재생
	UFUNCTION()
	void ApplySkill2Effect(); // 스킬2 효과 적용
	UFUNCTION()
	void ResetSkill2(UAnimMontage* Montage, bool bInterrupted); // 스킬2 리셋
	UFUNCTION()
	void ResetSkill2Cooldown(); // 스킬2 쿨타임 리셋
	//void DrawSkill2Range(); // 스킬2 범위 시각화
	//void ClearSkill2Range(); // 스킬2 범위 시각화 제거

    // 스킬3
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsUsingSkill3 = false; // 스킬3 사용 중 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanUseSkill3 = true; // 스킬3 사용 가능 여부
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float Skill3Cooldown = 6.0f; // 스킬3 쿨타임
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float Skill3Damage = 60.0f; // 스킬3 데미지
	FTimerHandle Skill3CooldownHandle; // 스킬3 쿨타임 타이머 핸들
	UPROPERTY()
	UAnimMontage* Skill3Montage; // 스킬3 애니메이션 몽타주
	UPROPERTY(EditAnywhere, Category = "Skill", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ASkill3Projectile> Skill3ProjectileClass; // 스킬3 투사체 클래스
	void PlaySkill3Montage(); // 스킬3 몽타주 재생
	UFUNCTION()
	void ResetSkill3(UAnimMontage* Montage, bool bInterrupted); // 스킬3 리셋
	UFUNCTION()
	void ResetSkill3Cooldown(); // 스킬3 쿨타임 리셋

    // 에임스킬1
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsUsingAimSkill1 = false; // 에임스킬1 사용 중 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanUseAimSkill1 = true; // 에임스킬1 사용 가능 여부
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float AimSkill1Cooldown = 7.0f; // 에임스킬1 쿨타임
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float AimSkill1Duration = 5.0f; // 에임스킬1 지속 시간
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float AimSkill1PlayInterval = 0.85f; // 에임스킬1 몽타주 반복 재생 간격
	float AimSkill1MontageStartTime = 0.0f; // 에임스킬1 몽타주 시작 시간
	FTimerHandle AimSkill1CooldownHandle; // 에임스킬1 쿨타임 타이머 핸들
	FTimerHandle AimSkill1RepeatHandle; // 에임스킬1 몽타주 반복 타이머 핸들
	UPROPERTY()
	UAnimMontage* AimSkill1Montage; // 에임스킬1 애니메이션 몽타주
	void PlayAimSkill1Montage(); // 에임스킬1 몽타주 재생
	UFUNCTION()
	void RepeatAimSkill1Montage(); // 에임스킬1 몽타주 반복 재생
	UFUNCTION()
	void ResetAimSkill1Timer(); // 에임스킬1 타이머 리셋
	UFUNCTION()
	void ResetAimSkill1(UAnimMontage* Montage, bool bInterrupted); // 에임스킬1 리셋
	UFUNCTION()
	void ResetAimSkill1Cooldown(); // 에임스킬1 쿨타임 리셋

    // 에임스킬2
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsUsingAimSkill2 = false; // 에임스킬2 사용 중 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanUseAimSkill2 = true; // 에임스킬2 사용 가능 여부
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float AimSkill2Cooldown = 6.0f; // 에임스킬2 쿨타임
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float AimSkill2Duration = 5.0f; // 에임스킬2 지속 시간
	FTimerHandle AimSkill2CooldownHandle; // 에임스킬2 쿨타임 타이머 핸들
	UPROPERTY()
	UAnimMontage* AimSkill2Montage; // 에임스킬2 애니메이션 몽타주
	UPROPERTY()
	UAnimMontage* AimSkill2StartMontage; // 에임스킬2 시작 애니메이션 몽타주
	FTimerHandle AimSkill2TransitionHandle; //에임스킬2 시작 몽타주 후 메인 몽타주 전환 타이머 핸들
	void PlayAimSkill2StartMontage(); // 에임스킬2 시작 몽타주 재생
	UFUNCTION()
	void OnAimSkill2StartMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 에임스킬2 시작 몽타주 종료 콜백
	UFUNCTION()
	void PlayAimSkill2Montage(); // 에임스킬2 메인 몽타주 재생
	UFUNCTION()
	void ResetAimSkill2(UAnimMontage* Montage, bool bInterrupted); // 에임스킬2 리셋
	UFUNCTION()
	void ResetAimSkill2Cooldown(); // 에임스킬2 쿨타임 리셋

    // 에임스킬3
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsUsingAimSkill3 = false; // 에임스킬3 사용 중 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bCanUseAimSkill3 = true; // 에임스킬3 사용 가능 여부
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float AimSkill3Cooldown = 10.0f; // 에임스킬3 쿨타임
	FTimerHandle AimSkill3CooldownHandle; // 에임스킬3 쿨타임 타이머 핸들
	UPROPERTY()
	UAnimMontage* AimSkill3Montage; // 에임스킬3 애니메이션 몽타주
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float AimSkill3Distance = 1500.0f; // 에임스킬3 투사체 최대 사거리
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	float AimSkill3Radius = 300.0f; // 에임스킬3 투사체 낙하지점 반경
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
	int32 NumProjectiles = 5; // 에임스킬3 투사체 개수
	bool bDrawDebugRange = true; //	에임스킬3 범위 디버그 시각화 여부
	FVector CachedAimSkill3Target; // 에임스킬3 타겟 위치 캐싱
	void PlayAimSkill3Montage(); // 에임스킬3 몽타주 재생
	UFUNCTION()
	void OnAimSkill3MontageEnded(UAnimMontage* Montage, bool bInterrupted); // 에임스킬3 몽타주 종료 콜백
    UFUNCTION()
	void SpawnAimSkill3Projectiles(); // 에임스킬3 투사체 소환
	UFUNCTION()
	void ResetAimSkill3(UAnimMontage* Montage, bool bInterrupted); // 에임스킬3 리셋
	UFUNCTION()
	void ResetAimSkill3Cooldown(); // 에임스킬3 쿨타임 리셋
	void DrawAimSkill3Range(const FVector& TargetLocation); // 에임스킬3 범위 디버그 시각화
	TArray<FVector> AimSkill3DropPoints; // 에임스킬3 투사체 낙하지점 배열
	FTimerHandle AimSkill3DropTimeHandle; // 에임스킬3 투사체 낙하 타이머 핸들
	UPROPERTY(EditAnywhere, Category = "Skill|Config", meta = (AllowPrivateAccess = "true"))
    float AimSkill3ProjectileDelay = 3.0f; // 투사체 낙하까지의 시간(초)
    void RotateCharacterToInputDirection(); // 스킬 사용 시 캐릭터를 입력 방향으로 회전시키는 함수
    UPROPERTY(EditAnywhere, Category = "Skill", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AAimSkill3Projectile> AimSkill3ProjectileClass; // 에임스킬3 투사체 클래스

	UPROPERTY(EditAnywhere, Category = "Skill|Weapon", meta = (AllowPrivateAccess = "true"))
	FVector MachineGunLocation = FVector(-5.0f, -20.0f, 0.0f); // 머신건 소환 위치 오프셋
	UPROPERTY(EditAnywhere, Category = "Skill|Weapon", meta = (AllowPrivateAccess = "true"))
	FRotator MachineGunRotation = FRotator(90.0f, 180.0f, 0.0f); // 머신건 소환 위치 오프셋
	UPROPERTY(EditAnywhere, Category = "Skill|Weapon", meta = (AllowPrivateAccess = "true"))
	FVector MachineGunScale = FVector(0.3f); // 머신건 소환 위치 오프셋

	UPROPERTY(EditAnywhere, Category = "Skill|Skill1", meta = (AllowPrivateAccess = "true"))
	float Skill1StunDuration = 5.0f; // 스킬1로 적을 기절시키는 시간
	UPROPERTY(EditAnywhere, Category = "Skill|Skill1", meta = (AllowPrivateAccess = "true"))
	float Skill1MontagePlayRate = 1.3f; // 스킬1 몽타주 재생 속도
	UPROPERTY(EditAnywhere, Category = "Skill|Skill1", meta = (AllowPrivateAccess = "true"))
	float Skill1StunDelayTime = 0.5f; // 스킬1 스턴 효과 지연 시간

	UPROPERTY(EditAnywhere, Category = "Skill|Skill2", meta = (AllowPrivateAccess = "true"))
	float Skill2MainCharacterLaunchDist = 300.0f; // 스킬2 시전시 캐릭터가 도약하는 거리
	UPROPERTY(EditAnywhere, Category = "Skill|Skill2", meta = (AllowPrivateAccess = "true"))
	float Skill2MontagePlayRate = 1.5f; // 스킬2 몽타주 재생 속도

	UPROPERTY(EditAnywhere, Category = "Skill|Skill3", meta = (AllowPrivateAccess = "true"))
	float Skill3SpawnForwardOffset = 200.0f; // 스킬3 투사체 전방 오프셋
	UPROPERTY(EditAnywhere, Category = "Skill|Skill3", meta = (AllowPrivateAccess = "true"))
	float Skill3SpawnVerticalOffset = 30.0f; // 스킬3 투사체 수직 오프셋
	UPROPERTY(EditAnywhere, Category = "Skill|Skill3", meta = (AllowPrivateAccess = "true"))
	float Skill3MontagePlayRate = 1.0f; // 스킬3 몽타주 재생 속도

	UPROPERTY(EditAnywhere, Category = "Skill|AimSkill1", meta = (AllowPrivateAccess = "true"))
	float AimSkill1MontagePlayRate = 1.0f; // 에임스킬1 몽타주 재생 속도
	UPROPERTY(EditAnywhere, Category = "Skill|AimSkill1", meta = (AllowPrivateAccess = "true"))
	float AimSkill1StopMontageTime = 0.3f; // 에임스킬1 몽타주 멈춤 시간

	UPROPERTY(EditAnywhere, Category = "Skill|AimSkill2", meta = (AllowPrivateAccess = "true"))
	float AimSkill2MontagePlayRate = 1.0f; // 에임스킬2 몽타주 재생 속도
	UPROPERTY(EditAnywhere, Category = "Skill|AimSkill2", meta = (AllowPrivateAccess = "true"))
	float AimSkill2StartMontagePlayRate = 0.5f; // 에임스킬2 시작 몽타주 재생 속도
	UPROPERTY(EditAnywhere, Category = "Skill|AimSkill2", meta = (AllowPrivateAccess = "true"))
	float AimSkill2TransitionThreshold = 0.9f; // 에임스킬2 시작 몽타주 전환 임계값

	UPROPERTY(EditAnywhere, Category = "Skill|AimSkill3", meta = (AllowPrivateAccess = "true"))
	float AimSkill3MontagePlayRate = 1.8f; // 에임스킬3 몽타주 재생 속도
	UPROPERTY(EditAnywhere, Category = "Skill|AimSkill3", meta = (AllowPrivateAccess = "true"))
	float AimSKill3TraceHeight = 500.0f; // 에임스킬3 라인트레이스 시작 높이
	UPROPERTY(EditAnywhere, Category = "Skill|AimSkill3", meta = (AllowPrivateAccess = "true"))
	float AimSkill3SpawnHeight = 2000.0f; // 에임스킬3 투사체 스폰 높이
	UPROPERTY(EditAnywhere, Category = "Skill|AimSkill3", meta = (AllowPrivateAccess = "true"))
	float AimSkill3SpawnRotation = -90.0f; // 에임스킬3 투사체 스폰 높이
	UPROPERTY(EditAnywhere, Category = "Skill|AimSkill3", meta = (AllowPrivateAccess = "true"))
	int32 AimSkill3GroundOffset = 10; // 에임스킬3 투사체 스폰 높이
};