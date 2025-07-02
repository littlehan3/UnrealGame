#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyKatana.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8
{
    Normal,
    Strong,
    Jump
};

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

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

private:
    bool bIsAttacking = false;
    bool bIsStrongAttack = false;

    void ApplyDamage(AActor* OtherActor);
    
    TArray<AActor*> EnemyActorsCache;

    EAttackType CurrentAttackType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KatanaTransform", meta = (AllowPrivateAccess = "true"))
    FVector KatanaRelativeLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KatanaTransform", meta = (AllowPrivateAccess = "true"))
    FRotator KatanaRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KatanaTransform", meta = (AllowPrivateAccess = "true"))
    FVector KatanaRelativeScale = FVector(1.0f, 1.0f, 1.0f);
};