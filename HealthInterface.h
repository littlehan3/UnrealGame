#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HealthInterface.generated.h"

// 이 UINTERFACE는 편집할 필요 없습니다.
UINTERFACE(MinimalAPI, Blueprintable)
class UHealthInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 체력 바가 필요한 모든 Actor(적)가 구현해야 하는 인터페이스입니다.
 */
class LOCOMOTION_API IHealthInterface
{
	GENERATED_BODY()

public:
	/** 현재 체력 비율 (0.0 ~ 1.0)을 반환합니다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Health")
	float GetHealthPercent() const;

	/** 현재 액터가 죽었는지 여부를 반환합니다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Health")
	bool IsEnemyDead() const;
};// Fill out your copyright notice in the Description page of Project Settings.
