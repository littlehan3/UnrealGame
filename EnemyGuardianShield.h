#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h" // AActor Ŭ���� ���
#include "DrawDebugHelpers.h" // ����� �ð�ȭ ��� ���
#include "EnemyGuardianShield.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyGuardianShield : public AActor
{
	GENERATED_BODY()

public:
	AEnemyGuardianShield(); // ������

	virtual void Tick(float DeltaTime) override; // �� ������ ȣ��

	void HideShield(); // ���и� ����� �����ϴ� �Լ�
	void StartShieldAttack(); // ���� ���� ���� ����
	void EndShieldAttack(); // ���� ���� ���� ����
	void PerformRaycastAttack(); // ���� ���� ������ �����ϴ� �Լ�

	// ������Ʈ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* ShieldMesh; // ���� ���� �� ��Ʈ ������Ʈ

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShieldChildMesh; // ���� ���� ��ġ�� �ڽ� �޽� (���� �̻��)

	// ���� �Ӽ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float ShieldHealth = 200.0f; // ������ ������

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ShieldDamage = 40.0f; // ���� ������ ������

	// ���� ������
	TSet<AActor*> RaycastHitActors; // �̹� ���ݿ� ������ ���� ���
	TSet<AActor*> DamagedActors; // �̹� ���ݿ� �������� ���� ���� ���

protected:
	virtual void BeginPlay() override; // ���� ���� �� ȣ��

private:
	bool bIsAttacking = false; // ���� ���� ������ Ȱ��ȭ�Ǿ����� ����
	void ApplyDamage(AActor* OtherActor); // ������ ���Ϳ��� �������� �����ϴ� �Լ�
};