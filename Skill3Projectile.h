#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Skill3Projectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UNiagaraSystem;
class UAudioComponent;
class USoundBase;

UCLASS()
class LOCOMOTION_API ASkill3Projectile : public AActor
{
    GENERATED_BODY()

public:
    ASkill3Projectile();

    void SetDamage(float InDamage) { Damage = InDamage; } // 데미지 설정 함수
    void SetShooter(AActor* InShooter) { Shooter = InShooter; } // 시전자 설정 함수
    void FireInDirection(const FVector& ShootDirection); // 발사 방향설정 속도 적용 함수

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit); // 충돌 이밴트 발생 시 호출되는 함수
    void ApplyAreaDamage(); // 광역 피해 함수

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
    USphereComponent* CollisionComponent = nullptr; // 충돌을 감지할 구체 컴포넌트

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision", meta = (AllowPrivateAccess = "true"))
    float CollisionRadius = 20.f; // 충돌 반경

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
    UProjectileMovementComponent* ProjectileMovement = nullptr; // 투사체 이동 제어 컴포넌트

    UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects", meta = (AllowPrivateAccess = "true"))
    UNiagaraSystem* ExplosionEffect; // 폭발 나이아가라 이펙트

    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    USoundBase* ExplosionSound = nullptr; // 폭발 사운드 이펙트

    UPROPERTY(EditAnywhere, Category = "SoundEffects")
    USoundBase* LoopingFlightSound = nullptr; // 투사체 비행중 사운드

    UPROPERTY()
    UAudioComponent* FlightAudioComponent = nullptr; // 비행 사운드용 컴포넌트

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
    float Damage = 60.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
    float DamageRadius = 150.0f; // 광역 피해 반경
     
    UPROPERTY()
    AActor* Shooter = nullptr; // 시전자 슈터 포인터
};