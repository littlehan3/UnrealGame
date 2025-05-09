#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "AimSkill2Projectile.generated.h"

class UProjectileMovementComponent; // ����ü �̵�������Ʈ ���漱��
class USphereComponent; // ��ü�ݸ��� ������Ʈ Ŭ���� ���漱��
class UStaticMeshComponent; // ���� �޽� ������Ʈ Ŭ���� ���漱��

UCLASS()
class LOCOMOTION_API AAimSkill2Projectile : public AActor
{
	GENERATED_BODY()

public:
	AAimSkill2Projectile(); // 
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void FireInDirection(const FVector& ShootDirection); // Ư�� �������� ����ü �߻��ϴ� �Լ�
	void SetDamage(float InDamage) { Damage = InDamage; } // ����ü ������ ���� �Լ�
	void SetShooter(AActor* InShooter) { Shooter = InShooter; } // �߻��� ���� �Լ�

protected:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit); // �浹 �߻��� ȣ��Ǵ� �Լ�

	void ApplyAreaDamage(); // ���� ������ ���� �Լ�
	AActor* FindClosestEnemy(); // ���� ����� �� Ž�� �Լ�
	void EvaluateTargetAfterApex(); // �ְ��� ���޽� Ÿ���� ���ϴ� �Լ�
	void AutoExplodeIfNoTarget(); // Ÿ���� ������ �ڵ� �����ϴ� �Լ�

	void SpawnPersistentExplosionArea(const FVector& Location); // ���߿����� ����� �Լ�
	void ApplyPersistentEffects(); // ���߿���ȿ�� �Լ�
	void ApplyPeriodicDamage(); // ���߿��� �ֱ��������� ���� �Լ�

private:
	UPROPERTY(VisibleAnywhere)
	USphereComponent* CollisionComponent; // �浹���� ������Ʈ

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement; // ����ü �̵� ó�� ������Ʈ

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComponent; // ����ü �Ž� ������Ʈ

	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* ExplosionEffect; // ��������Ʈ ���̾ư��� ������Ʈ

	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* PersistentAreaEffect; // ���߿��� ���̾ư��� ������Ʈ

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* ExplosionSound; // ���� ����

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* LoopingFlightSound; // ����ü ���� �� ����

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* PersistentAreaSound; // ���߿��� ����

	AActor* Shooter; // ����ü�� �߻��� ����

	float Damage = 10.0f; // ����ü ������
	float DamageRadius = 150.0f; // ����ü ���߹ݰ�
	float PullStrength = 500.0f; // ������ ����

	bool bHasReachedApex = false; // ����ü�� ������ �����ߴ��� ����
	float ApexHeight = 500.0f; // ���� ����
	float DelayBeforeTracking = 0.5f; // ���� ���� �� Ÿ�� ���� �� ������
	float DetectionRadius = 1200.0f; // ���� ����

	float ExplosionDuration = 5.0f; // ���� �ݰ�
	float DamageInterval = 1.0f; // �ֱ��� ������ ���� ����
	bool bExplosionActive = false; // ���� ����Ʈ�� Ȱ��ȭ �Ǿ����� ����

	FVector ExplosionLocation; // ���� �߻� ��ġ

	FTimerHandle PersistentEffectsTimerHandle; // ���߿��� ����Ʈ ������ ���� Ÿ�̸� �ڵ�
	FTimerHandle PeriodicDamageTimerHandle; // ���߿��� ������ ������ �ð��� ���� Ÿ�̸� �ڵ�
	FTimerHandle ExplosionDurationTimerHandle; // ���߿��� ���ӽð��� ���� Ÿ�̾� �ڵ�
	FTimerHandle DelayTimerHandle; // ���� ������ ���� Ÿ�̸� �ڵ�

	TSet<AActor*> DamagedActorsThisTick; // ���� ƽ���� �������� ���� ���͵��� �����ϴ� ����
};