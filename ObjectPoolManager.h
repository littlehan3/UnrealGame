#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObjectPoolManager.generated.h"

UCLASS()
class LOCOMOTION_API AObjectPoolManager : public AActor
{
    GENERATED_BODY()

public:
    AObjectPoolManager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // 기본 함수들 (현재는 아무 기능 없음)
    UFUNCTION(BlueprintCallable)
    void InitializePool();

    UFUNCTION(BlueprintCallable)
    void CleanupPool();

private:
    // 기본 변수들
    UPROPERTY(EditAnywhere, Category = "Pool Settings")
    int32 MaxPoolSize = 100;
};
