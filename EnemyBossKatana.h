#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyBossKatana.generated.h"

class AEnemyBoss;
class ABossEnemy;
class UNiagaraSystem;

UCLASS()
class LOCOMOTION_API AEnemyBossKatana : public AActor
{
    GENERATED_BODY()

public:
    AEnemyBossKatana();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* KatanaMesh; // 계층 관리를 위한 루트 컴포넌트

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* KatanaChildMesh; // 실질적인 레이캐스트 위치 설정을 위한 메시

    UPROPERTY() 
    TSet<TObjectPtr<AActor>> RaycastHitActors;

    UPROPERTY()
    TSet<TObjectPtr<AActor>> DamagedActors;

    void EnableAttackHitDetection();
    void DisableAttackHitDetection();

    void PerformRaycastAttack(); // 틱 혹은 AnimNotify에서 호출

    UFUNCTION()
    void HideKatana();

    void StartAttack();
    void EndAttack();

    //void SetShooter(ABossEnemy* Shooter);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category = "State", meta = (AllowPrivateAccess = "true"))
    bool bIsAttacking = false;
    UPROPERTY(VisibleAnywhere, Category = "State", meta = (AllowPrivateAccess = "true"))
    bool bHasPlayedHitSound = false;

    void ApplyDamage(AActor* OtherActor);

	UPROPERTY()
    TArray<TObjectPtr<AActor>> EnemyActorsCache;

    void PlayKatanaHitSound();

    FTimerHandle AttackTraceTimerHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (AllowPrivateAccess = "true"))
    float BossDamage = 50.0f;

    // 카타나 소유주인 보스
    UPROPERTY()
	TObjectPtr<ABossEnemy> BossOwner = nullptr;

    const float TraceInterval = 0.016f; // 약 60FPS 간격
};
