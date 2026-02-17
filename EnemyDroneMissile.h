#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyDroneMissile.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyDroneMissile : public AActor
{
    GENERATED_BODY()

public:
    AEnemyDroneMissile(); 

    void SetTarget(AActor* Target); // 추적할 타겟 설정 함수
    void ResetMissile(const FVector& SpawnLocation, AActor* NewTarget); // 오브젝트 풀링을 위한 상태 초기화 및 재발사 함수
    void Explode(); // 폭발 처리 함수

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UProjectileMovementComponent* ProjectileMovement; // 투사체 이동을 관리하는 컴포넌트

protected:
    virtual void Tick(float DeltaTime) override;

    // 미사일이 데미지를 입었을 때 호출될 함수
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, CateGory = "Missile", meta = (AllowPrivateAccess = "true"))
    class USphereComponent* CollisionComponent; // 충돌을 감지하는 구체 컴포넌트

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, CateGory = "Missile", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* MeshComp; // 미사일의 외형을 나타내는 스태틱 메쉬

	UPROPERTY()
    AActor* TargetActor; // 추적 대상 액터

    FVector LastMoveDirection; // 마지막으로 이동했던 방향 (타겟을 놓쳤을 때 사용)

    UPROPERTY(VisibleInstanceOnly, Category = "Missile", meta = (AllowPrivateAccess = "true"))
    bool bExploded = false; // 이미 폭발했는지 여부

    // 충돌(OnHit) 이벤트 발생 시 호출될 함수
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UPROPERTY(EditAnywhere, Category = "Missile", meta = (AllowPrivateAccess = "true"))
    float ExplosionRadius = 50.0f; // 폭발 반경

    // 미사일 속성
    UPROPERTY(EditAnywhere, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    class UNiagaraSystem* ExplosionEffect; // 폭발 시 재생될 나이아가라 이펙트

    UPROPERTY(EditAnywhere, Category = "Missile", meta = (AllowPrivateAccess = "true"))
    float Damage = 10.0f; // 폭발 시 데미지

    UPROPERTY(EditAnywhere, Category = "Missile", meta = (AllowPrivateAccess = "true"))
    float Health = 10.0f; // 미사일 자체의 체력 (플레이어가 요격 가능)

    UPROPERTY(EditAnywhere, Category = "Audio", meta = (AllowPrivateAccess = "true"))
    class UAudioComponent* FlightLoopAudio; // 비행 루핑 사운드 컴포넌트

    UPROPERTY(EditAnywhere, Category = "Audio", meta = (AllowPrivateAccess = "true"))
    USoundBase* FlightLoopSound; // 비행 루핑 사운드 애셋

    UPROPERTY(EditAnywhere, Category = "Audio", meta = (AllowPrivateAccess = "true"))
	USoundBase* ExplosionSound; // 폭발 사운드 애셋

};