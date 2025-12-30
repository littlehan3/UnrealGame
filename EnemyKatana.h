#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MainCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "EnemyKatana.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8
{
    Normal,
    Strong,
    Jump
};

class AEnemy;
class UNiagaraSystem;

UCLASS()
class LOCOMOTION_API AEnemyKatana : public AActor
{
    GENERATED_BODY()

public:
    AEnemyKatana();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* KatanaMesh; // 계층 관리를 위한 루트 컴포넌트

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* KatanaChildMesh; // 실질적인 레이캐스트 위치 설정을 위한 메시

    // 판정 데이터
    TSet<AActor*> RaycastHitActors;
    TSet<AActor*> DamagedActors;

    void EnableAttackHitDetection(EAttackType AttackType);
    void DisableAttackHitDetection();

    void PerformRaycastAttack(); // 공격 프레임마다 AnimNotify 등에서 호출

    UFUNCTION()
    void HideKatana();

    virtual void Tick(float DeltaTime) override;

    void StartAttack();
    void EndAttack();

    void SetShooter(AEnemy* Shooter);

protected:
    virtual void BeginPlay() override;

private:
    bool bIsAttacking = false;
    bool bIsStrongAttack = false;

    void ApplyDamage(AActor* OtherActor);
    
    TArray<AActor*> EnemyActorsCache;

    EAttackType CurrentAttackType;

    void PlayKatanaHitSound();

    bool bHasPlayedHitSound = false;
};