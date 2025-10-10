#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" // ACharacter 클래스 상속
#include "EnemyDroneMissile.h" // 드론이 발사할 미사일 클래스를 알아야 하므로 포함
#include "NiagaraSystem.h" // 나이아가라 이펙트 시스템 사용
#include "EnemyDrone.generated.h"

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyDrone : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemyDrone(); // 생성자

    // 오브젝트 풀링을 위한 미사일 배열
    UPROPERTY()
    TArray<AEnemyDroneMissile*> MissilePool; // 미리 생성해 둔 미사일들을 담아두는 배열

    // 미사일 풀에서 현재 사용 가능한(비활성화된) 미사일을 가져오는 함수
    AEnemyDroneMissile* GetAvailableMissileFromPool();

    // 데미지를 입었을 때 호출되는 함수
    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator,
        AActor* DamageCauser
    ) override;

    void Die(); // 사망 처리 함수
    void HideEnemy(); // 사망 후 액터 정리 및 숨김 처리 함수
    float Health = 40.0f; // 현재 체력
    bool bIsDead = false; // 사망 상태 여부

protected:
    virtual void BeginPlay() override; // 게임 시작 시 호출되는 함수
    virtual void Tick(float DeltaTime) override; // 매 프레임 호출되는 함수

    // 블루프린트에서 설정할 미사일 클래스
    UPROPERTY(EditDefaultsOnly, Category = "Pawn")
    TSubclassOf<AEnemyDroneMissile> MissileClass; // 풀링할 미사일의 원본 클래스

    float MissileCooldown = 3.0f; // 미사일 발사 후 다음 발사까지의 대기 시간
    float MissileTimer = 0.0f; // 미사일 쿨타임을 계산하기 위한 타이머 변수

    // 사망 시 재생될 이펙트 및 사운드
    UPROPERTY(EditAnywhere, Category = "Effects")
    UNiagaraSystem* DeathEffect; // 사망 시 나이아가라 이펙트

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundBase* DeathSound; // 사망 시 사운드

    AActor* PlayerActor; // 플레이어 액터에 대한 참조
    void ShootMissile(); // 미사일을 발사하는 함수
};