#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossEnemy.h"
#include "NiagaraFunctionLibrary.h"
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

    // 카타나 소유주인 보스
    UPROPERTY()
    ABossEnemy* BossOwner; // 보스 참조용 변수

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

    void SetShooter(ABossEnemy* Shooter);

protected:
    virtual void BeginPlay() override;

private:
    bool bIsAttacking = false;

    void ApplyDamage(AActor* OtherActor);

    void PlayKatanaHitSound();

    TArray<AActor*> EnemyActorsCache;

    // 데미지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (AllowPrivateAccess = "true"))
    float BossDamage = 50.0f;

    bool bHasPlayedHitSound = false;

};
