#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" // ACharacter 클래스 상속
#include "HealthInterface.h"
#include "EnemyDrone.generated.h"

class AEnemyDroneMissile; // 전방 선언으로 컴파일 시간 단축
class UNiagaraSystem;
class UAudioComponent;
class USoundBase;

UCLASS(Blueprintable)
class LOCOMOTION_API AEnemyDrone : public ACharacter, public IHealthInterface
{
    GENERATED_BODY()

public:
    AEnemyDrone(); // 생성자

    //// 오브젝트 풀링을 위한 미사일 배열
    //UPROPERTY()
    //TArray<AEnemyDroneMissile*> MissilePool; // 미리 생성해 둔 미사일들을 담아두는 배열

    //// 미사일 풀에서 현재 사용 가능한(비활성화된) 미사일을 가져오는 함수
    //AEnemyDroneMissile* GetAvailableMissileFromPool();

    // 데미지를 입었을 때 호출되는 함수
    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator,
        AActor* DamageCauser
    ) override;

    void Die(); // 사망 처리 함수
    void HideEnemy(); // 사망 후 액터 정리 및 숨김 처리 함수

    UPROPERTY(EditDefaultsOnly, Category = "Health")
    float MaxHealth = 40.0f; // 최대 체력

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float Health = 40.0f; // 현재 체력

    UPROPERTY(VisibleInstanceOnly, Category = "State")
    bool bIsDead = false; // 사망 여부

	// HealthInterface 구현
	virtual float GetHealthPercent_Implementation() const override; // 현재 체력 비율 반환
	virtual bool IsEnemyDead_Implementation() const override; // 적이 사망했는지 여부 반환

protected:
    virtual void BeginPlay() override; 
    virtual void Tick(float DeltaTime) override; 

private:
    // 오브젝트 풀링을 위한 미사일 배열
    UPROPERTY()
    TArray<AEnemyDroneMissile*> MissilePool; // 미리 생성해 둔 미사일들을 담아두는 배열

    // 미사일 풀에서 현재 사용 가능한(비활성화된) 미사일을 가져오는 함수
    AEnemyDroneMissile* GetAvailableMissileFromPool();

    UPROPERTY()
	class AEnemyDroneAIController* AICon; // AI 컨트롤러 참조

    // 블루프린트에서 설정할 미사일 클래스
    UPROPERTY(EditDefaultsOnly, Category = "Pawn", meta = (PrivateAccessAllow = "true"))
    TSubclassOf<AEnemyDroneMissile> MissileClass; // 풀링할 미사일의 원본 클래스

    UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (PrivateAccessAllow = "true"))
    float MissileCooldown = 3.0f; // 미사일 발사 후 다음 발사까지의 대기 시간

    UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
    int32 InitialMissilePoolSize = 10; // 초기 생성 개수

    UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
    float MissileSpawnForwardOffset = 100.0f; // 드론 앞 발사 오프셋 거리

    UPROPERTY(EditDefaultsOnly, Category = "Drone | Movement", meta = (AllowPrivateAccess = "true"))
	float DroneGravityScale = 0.0f; // 드론의 중력 스케일 (중력 영향 없음)

    UPROPERTY()
	float MissileFireInterval = 0.0f; // 미사일 발사 간격

    // 사망 시 재생될 이펙트 및 사운드
    UPROPERTY(EditAnywhere, Category = "Effects", meta = (PrivateAccessAllow = "true"))
    UNiagaraSystem* DeathEffect; // 사망 시 나이아가라 이펙트

    UPROPERTY(EditAnywhere, Category = "Effects", meta = (PrivateAccessAllow = "true"))
    USoundBase* DeathSound; // 사망 시 사운드
    
	UPROPERTY()
    AActor* PlayerActor; // 플레이어 액터에 대한 참조

    void ShootMissile(); // 미사일을 발사하는 함수

    UPROPERTY()
	class UAudioComponent* FlightLoopAudio; // 비행 루핑 사운드 컴포넌트

    UPROPERTY(EditAnyWhere, Category = "Effects", meta = (PrivateAccessAllow = "true"))
	USoundBase* FlightLoopSound; // 비행 루핑 사운드 애셋
};