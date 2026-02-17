#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyShooterGrenade.generated.h"

// 전방 선언
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UAudioComponent;
class URotatingMovementComponent;
class UNiagaraSystem;

UCLASS()
class LOCOMOTION_API AEnemyShooterGrenade : public AActor
{
	GENERATED_BODY()

public:
	AEnemyShooterGrenade(); // 생성자
	void LaunchGrenade(FVector LaunchVelocity); // 수류탄을 지정된 속도로 발사하는 함수

protected:
	virtual void BeginPlay() override; // 게임 시작 시 호출
	//virtual void Tick(float DeltaTime) override; // 매 프레임 호출

	void Explode(); // 폭발을 처리하는 함수

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	// 컴포넌트
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComp = nullptr; // 충돌을 감지하는 구체 컴포넌트

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GrenadeMesh = nullptr; // 수류탄의 외형을 나타내는 스태틱 메쉬

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement = nullptr; // 투사체 이동을 관리하는 컴포넌트

	// Tick에서 수류탄을 회전시키는 대신 URotatingMovementComponent를 사용하여 자동으로 회전
	UPROPERTY(VisibleAnywhere, Category = "Movement")
	TObjectPtr<URotatingMovementComponent> RotatingMovement = nullptr; 

	// 이펙트 및 사운드
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> ExplosionEffect = nullptr; // 폭발 시 재생될 나이아가라 이펙트

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	TObjectPtr<USoundBase> ExplosionSound = nullptr; // 폭발 시 재생될 사운드

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Audio")
	TObjectPtr<UAudioComponent> FuseAudioComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	TObjectPtr<USoundBase> BounceSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	TObjectPtr<USoundBase> FuseSound = nullptr;

	// 전투 관련 속성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ExplosionDamage = 50.0f; // 폭발 데미지

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ExplosionRadius = 150.0f; // 폭발 반경

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float FuseTime = 3.0f; // 발사 후 폭발까지 걸리는 시간

	// 이동 관련 속성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RotationSpeed = 360.0f; // 공중에서 회전하는 속도 (초당 360도)

private:
	bool bHasExploded = false; // 중복 폭발을 방지하기 위한 플래그
	FTimerHandle FuseTimerHandle;
};