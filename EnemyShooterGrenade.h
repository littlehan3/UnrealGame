#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h" // AActor Ŭ���� ���
#include "NiagaraSystem.h" // ���̾ư��� ����Ʈ �ý��� ���
#include "EnemyShooterGrenade.generated.h"

// ���� ����
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class LOCOMOTION_API AEnemyShooterGrenade : public AActor
{
	GENERATED_BODY()

public:
	AEnemyShooterGrenade(); // ������
	void LaunchGrenade(FVector LaunchVelocity); // ����ź�� ������ �ӵ��� �߻��ϴ� �Լ�

protected:
	virtual void BeginPlay() override; // ���� ���� �� ȣ��
	virtual void Tick(float DeltaTime) override; // �� ������ ȣ��

	void Explode(); // ������ ó���ϴ� �Լ�

	// ������Ʈ
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	USphereComponent* CollisionComp; // �浹�� �����ϴ� ��ü ������Ʈ

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UStaticMeshComponent* GrenadeMesh; // ����ź�� ������ ��Ÿ���� ����ƽ �޽�

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UProjectileMovementComponent* ProjectileMovement; // ����ü �̵��� �����ϴ� ������Ʈ

	// ����Ʈ �� ����
	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* ExplosionEffect; // ���� �� ����� ���̾ư��� ����Ʈ

	UPROPERTY(EditAnywhere, Category = "SoundEffects")
	USoundBase* ExplosionSound; // ���� �� ����� ����

	// ���� ���� �Ӽ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ExplosionDamage = 50.0f; // ���� ������

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ExplosionRadius = 150.0f; // ���� �ݰ�

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float FuseTime = 3.0f; // �߻� �� ���߱��� �ɸ��� �ð�

	// �̵� ���� �Ӽ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RotationSpeed = 360.0f; // ���߿��� ȸ���ϴ� �ӵ� (�ʴ� 360��)

private:
	bool bHasExploded = false; // �ߺ� ������ �����ϱ� ���� �÷���
};