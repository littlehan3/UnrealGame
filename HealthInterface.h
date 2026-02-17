#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HealthInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable) // 시스템용
class UHealthInterface : public UInterface
{
	GENERATED_BODY()
};

class LOCOMOTION_API IHealthInterface // 실제 상속용
{
	GENERATED_BODY()

public:
	// 현재 체력 비율 (0.0 ~ 1.0)을 반환
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Health")
	float GetHealthPercent() const;

	// 현재 액터가 죽었는지 여부를 반환
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Health")
	bool IsEnemyDead() const;
};
