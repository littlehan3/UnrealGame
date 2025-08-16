#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyGuardianShield.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyGuardianShield : public AActor
{
	GENERATED_BODY()
	
public:	
	AEnemyGuardianShield();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* ShieldMesh; // ���� ������ ���� ��Ʈ ������Ʈ

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShieldChildMesh; // �������� ����ĳ��Ʈ ��ġ ������ ���� �޽�

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
