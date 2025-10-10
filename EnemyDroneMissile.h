#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyDroneMissile.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyDroneMissile : public AActor
{
    GENERATED_BODY()

public:
    AEnemyDroneMissile(); // ������

    void SetTarget(AActor* Target); // ������ Ÿ�� ���� �Լ�
    void ResetMissile(FVector SpawnLocation, AActor* NewTarget); // ������Ʈ Ǯ���� ���� ���� �ʱ�ȭ �� ��߻� �Լ�
    void Explode(); // ���� ó�� �Լ�

    // ������Ʈ��
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UProjectileMovementComponent* ProjectileMovement; // ����ü �̵��� �����ϴ� ������Ʈ

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class USphereComponent* CollisionComponent; // �浹�� �����ϴ� ��ü ������Ʈ

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UStaticMeshComponent* MeshComp; // �̻����� ������ ��Ÿ���� ����ƽ �޽�

    // �̻��� �Ӽ�
    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* ExplosionEffect; // ���� �� ����� ���̾ư��� ����Ʈ

    UPROPERTY(EditAnywhere, Category = "Missile")
    float Damage = 10.f; // ���� �� ������

    UPROPERTY(EditAnywhere, Category = "Missile")
    float Health = 10.f; // �̻��� ��ü�� ü�� (�÷��̾ ��� ����)

private:
    AActor* TargetActor; // ���� ��� ����
    FVector LastMoveDirection; // ���������� �̵��ߴ� ���� (Ÿ���� ������ �� ���)
    bool bExploded = false; // �̹� �����ߴ��� ����

    // �浹(OnHit) �̺�Ʈ �߻� �� ȣ��� �Լ�
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    // �̻����� �������� �Ծ��� �� ȣ��� �Լ�
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(EditAnywhere, Category = "Missile")
    float ExplosionRadius = 50.f; // ���� �ݰ�
};