#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyKatana.generated.h"

class AEnemy;

UCLASS()
class LOCOMOTION_API AEnemyKatana : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AEnemyKatana();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* KatanaMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
    class UBoxComponent* HitBox;

    // 판정 데이터
    TSet<AActor*> OverlapHitActors;
    TSet<AActor*> RaycastHitActors;
    TSet<AActor*> DamagedActors;

    void EnableAttackHitDetection(bool bStrongAttack);
    void DisableAttackHitDetection();

    void PerformRaycastAttack(); // 공격 프레임마다 AnimNotify 등에서 호출

    UFUNCTION()
    void HideKatana();

    virtual void Tick(float DeltaTime) override;

    void StartAttack();
    void EndAttack();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnHitBoxOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

private:
    bool bIsAttacking = false;
    bool bIsStrongAttack = false;

    void TryApplyDamage(AActor* OtherActor);
    
    TArray<AActor*> EnemyActorsCache;
};