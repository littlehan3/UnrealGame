#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "AimSkill2Projectile.generated.h"

class UProjectileMovementComponent; // 투사체 이동컴포넌트 전방선언
class USphereComponent; // 구체충돌 컴포넌트 클래스 전방선언
class UStaticMeshComponent; // 스태틱 메쉬 컴포넌트 클래스 전방선언

UCLASS()
class LOCOMOTION_API AAimSkill2Projectile : public AActor
{
	GENERATED_BODY()

public:
	AAimSkill2Projectile();
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void FireInDirection(const FVector& ShootDirection); // 특정 방향으로 투사체 발사하는 함수
	void SetDamage(float InDamage) { Damage = InDamage; } // 투사체 데미지 설정 함수
	void SetShooter(AActor* InShooter); //{ Shooter = InShooter; } // 발사자 설정 함수

protected:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit); // 충돌 발생시 호출되는 함수

	void ApplyAreaDamage(); // 광역 데미지 적용 함수
	AActor* FindClosestEnemy(); // 가장 가까운 적 탐색 함수
	void EvaluateTargetAfterApex(); // 최고점 도달시 타겟을 정하는 함수
	void AutoExplodeIfNoTarget(); // 타겟이 없으면 자동 폭발하는 함수

	void SpawnPersistentExplosionArea(const FVector& Location); // 지속영역을 생성하는 함수
	void ApplyPersistentEffects(); // 지속영역 끌어당기기 효과 함수
	void ApplyPeriodicDamage(); // 지속영역 주기적 데미지 적용 함수

private:
	UPROPERTY(VisibleAnywhere)
	USphereComponent* CollisionComponent; // 충돌판정 컴포넌트

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement; // 투사체 이동 처리 컴포넌트

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComponent; // 투사체 외형 컴포넌트

	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* ExplosionEffect; // 폭발이펙트 나이아가라 시스템

	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* PersistentAreaEffect; // 지속영역 나이아가라 시스템

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* ExplosionSound; // 폭발 사운드

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* LoopingFlightSound; // 투사체 비행 루프 사운드

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* PersistentAreaSound; // 지속영역 사운드

	UPROPERTY()
	UNiagaraComponent* PersistentAreaNiagaraComponent; // 나이아가라 컴포넌트를 저장하고 지속시간 종료 시 안전 정리하기 위한 포인터

	UPROPERTY()
	UAudioComponent* FlightAudioComponent;  // 비행 사운드 오디오 컴포넌트

	UPROPERTY()
	UAudioComponent* PersistentAreaAudioComponent;  // 폭발 영역 지속 오디오 컴포넌트

	AActor* Shooter; // 투사체를 발사한 액터

	float Damage = 10.0f; // 투사체 데미지
	float DamageRadius = 300.0f; // 투사체 폭발반경
	float PullStrength = 1000.0f; // 끌어당기는 세기

	bool bHasReachedApex = false; // 투사체가 정점에 도달했는지 여부
	float ApexHeight = 500.0f; // 정점 높이
	float DelayBeforeTracking = 0.5f; // 정점 도달 후 타겟 추적 전 딜레이
	float DetectionRadius = 1500.0f; // 감지 범위

	float ExplosionDuration = 7.0f; // 폭발 지속 시간
	float DamageInterval = 1.0f; // 주기적 데미지 적용 간격
	bool bExplosionActive = false; // 폭발 이펙트가 활성화 되었는지 여부

	FVector ExplosionLocation; // 폭발 발생 위치

	FTimerHandle PersistentEffectsTimerHandle; // 지속영역 이펙트 적용을 위한 타이머 핸들
	FTimerHandle PeriodicDamageTimerHandle; // 지속영역 데미지 적용 시간을 위한 타이머 핸들
	FTimerHandle ExplosionDurationTimerHandle; // 지속영역 지속시간을 위한 타이머 핸들
	FTimerHandle DelayTimerHandle; // 정점 딜레이를 위한 타이머 핸들

	TSet<AActor*> DamagedActorsThisTick; // 현재 틱에서 데미지를 받은 액터들을 저장하는 세트

	bool bHasExploded = false;
};
