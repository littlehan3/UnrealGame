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
    class UStaticMeshComponent* KatanaMesh; // ���� ������ ���� ��Ʈ ������Ʈ

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* KatanaChildMesh; // �������� ����ĳ��Ʈ ��ġ ������ ���� �޽�

    // �ǰݵ� ���� ����
    TSet<AActor*> RaycastHitActors;
    TSet<AActor*> DamagedActors;

    void EnableAttackHitDetection();
    void DisableAttackHitDetection();

    void PerformRaycastAttack(); // ƽ Ȥ�� AnimNotify���� ȣ��

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

    // ������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (AllowPrivateAccess = "true"))
    float BossDamage = 50.0f;

};
