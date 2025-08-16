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
	class UStaticMeshComponent* ShieldMesh; // 계층 관리를 위한 루트 컴포넌트

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShieldChildMesh; // 실질적인 레이캐스트 위치 설정을 위한 메시

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
