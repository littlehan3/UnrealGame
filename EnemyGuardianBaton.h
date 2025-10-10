#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h" // AActor Ŭ���� ���
#include "EnemyGuardianBaton.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyGuardianBaton : public AActor
{
	GENERATED_BODY()

public:
	AEnemyGuardianBaton(); // ������

	virtual void Tick(float DeltaTime) override; // �� ������ ȣ��

	// ���� ���� Ȱ��ȭ/��Ȱ��ȭ �Լ�
	void EnableAttackHitDetection();
	void DisableAttackHitDetection();
	void StartAttack(); // ���� ���� (���������� EnableAttackHitDetection�� ����)
	void EndAttack(); // ���� ���� (���������� DisableAttackHitDetection�� ����)

	void PerformRaycastAttack(); // ���� ���� ������ �����ϴ� �Լ�

	UFUNCTION()
	void HideBaton(); // ���к��� ����� �����ϴ� �Լ�

	// ������Ʈ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* BatonMesh; // ���к��� ������ ��Ÿ���� ����ƽ �޽�

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BatonChildMesh; // ���� ������ ���������� ����� �ڽ� �޽�

	// ���� ������ (�� ���ݸ��� �ʱ�ȭ��)
	TSet<AActor*> RaycastHitActors; // �̹� ���ݿ��� ������ ������ ���� ���
	TSet<AActor*> DamagedActors; // �̹� ���ݿ��� ������ �������� ���� ���� ���

protected:
	virtual void BeginPlay() override; // ���� ���� �� ȣ��

private:
	bool bIsAttacking = false; // ���� ���� ������ Ȱ��ȭ�Ǿ����� ����
	void ApplyDamage(AActor* OtherActor); // ������ ���Ϳ��� �������� �����ϴ� �Լ�

	TArray<AActor*> EnemyActorsCache; // �Ʊ� ����� ��� ĳ�� (���� ������ ����)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (AllowPrivateAccess = "true"))
	float BatonDamage = 30.0f; // ���к��� ���ݷ�
};