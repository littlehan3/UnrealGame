#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyKatana.generated.h"

UCLASS()
class LOCOMOTION_API AEnemyKatana : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AEnemyKatana();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // 나이프 메시 추가
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* KatanaMesh;
};