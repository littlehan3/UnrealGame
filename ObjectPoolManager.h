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

    // �⺻ �Լ��� (����� �ƹ� ��� ����)
    UFUNCTION(BlueprintCallable)
    void InitializePool();

    UFUNCTION(BlueprintCallable)
    void CleanupPool();

private:
    // �⺻ ������
    UPROPERTY(EditAnywhere, Category = "Pool Settings")
    int32 MaxPoolSize = 100;
};
