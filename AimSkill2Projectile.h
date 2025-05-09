#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "AimSkill2Projectile.generated.h"

class UProjectileMovementComponent; // 투사체 이동컴포넌트 전방선언
class USphereComponent; // 구체콜리전 컴포넌트 클래스 전방선언
class UStaticMeshComponent; // 정적 메시 컴포넌트 클래스 전방선언

UCLASS()
class LOCOMOTION_API AAimSkill2Projectile : public AActor
{
	GENERATED_BODY()

public:
	AAimSkill2Projectile(); // 
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void FireInDirection(const FVector& ShootDirection); // 특정 방향으로 투사체 발사하는 함수
	void SetDamage(float InDamage) { Damage = InDamage; } // 투사체 데미지 설정 함수
	void SetShooter(AActor* InShooter) { Shooter = InShooter; } // 발사자 설정 함수

protected:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit); // 충돌 발생시 호출되는 함수

	void ApplyAreaDamage(); // 범위 데미지 적용 함수
	AActor* FindClosestEnemy(); // 가장 가까운 적 탐색 함수
	void EvaluateTargetAfterApex(); // 최고점 도달시 타켓을 평가하는 함수
	void AutoExplodeIfNoTarget(); // 타켓이 없을때 자동 폭발하는 함수

	void SpawnPersistentExplosionArea(const FVector& Location); // 폭발영역을 남기는 함수
	void ApplyPersistentEffects(); // 폭발영역효과 함수
	void ApplyPeriodicDamage(); // 폭발영역 주기적데미지 적용 함수

private:
	UPROPERTY(VisibleAnywhere)
	USphereComponent* CollisionComponent; // 충돌감지 컴포넌트

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement; // 투사체 이동 처리 컴포넌트

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComponent; // 투사체 매쉬 컴포넌트

	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* ExplosionEffect; // 폭발이펙트 나이아가라 컴포넌트

	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* PersistentAreaEffect; // 폭발영역 나이아가라 컴포넌트

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* ExplosionSound; // 폭발 사운드

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* LoopingFlightSound; // 투사체 비행 시 사운드

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* PersistentAreaSound; // 폭발영역 사운드

	AActor* Shooter; // 투사체를 발사한 엑터

	float Damage = 10.0f; // 투사체 데미지
	float DamageRadius = 150.0f; // 투사체 폭발반경
	float PullStrength = 500.0f; // 끌어당김 세기

	bool bHasReachedApex = false; // 투사체가 고점에 도달했는지 여부
	float ApexHeight = 500.0f; // 고점 높이
	float DelayBeforeTracking = 0.5f; // 고점 도달 후 타겟 추적 전 딜레이
	float DetectionRadius = 1200.0f; // 감지 범위

	float ExplosionDuration = 5.0f; // 폭발 반경
	float DamageInterval = 1.0f; // 주기적 데미지 적용 간격
	bool bExplosionActive = false; // 폭발 이펙트가 활성화 되었는지 여부

	FVector ExplosionLocation; // 폭발 발생 위치

	FTimerHandle PersistentEffectsTimerHandle; // 폭발영역 이펙트 적용을 위한 타이머 핸들
	FTimerHandle PeriodicDamageTimerHandle; // 폭발영역 데미지 적용을 시간을 위한 타이머 핸들
	FTimerHandle ExplosionDurationTimerHandle; // 폭발영역 지속시간을 위한 타이어 핸들
	FTimerHandle DelayTimerHandle; // 지연 동작을 위한 타이머 핸들

	TSet<AActor*> DamagedActorsThisTick; // 현재 틱에서 데미지를 입은 엑터들을 추적하는 집합
};