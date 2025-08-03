#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyBossKatana.generated.h"

class AEnemyBoss;

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

    // 피격된 액터 관리
    TSet<AActor*> RaycastHitActors;
    TSet<AActor*> DamagedActors;

    void EnableAttackHitDetection();
    void DisableAttackHitDetection();

    void PerformRaycastAttack(); // 틱 혹은 AnimNotify에서 호출

    UFUNCTION()
    void HideKatana();

    virtual void Tick(float DeltaTime) override;

    void StartAttack();
    void EndAttack();

protected:
    virtual void BeginPlay() override;

private:
    bool bIsAttacking = false;

    void ApplyDamage(AActor* OtherActor);

    TArray<AActor*> EnemyActorsCache;

    // 데미지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (AllowPrivateAccess = "true"))
    float BossDamage = 50.0f;

};
