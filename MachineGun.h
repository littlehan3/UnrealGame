#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MachineGun.generated.h"

UCLASS()
class LOCOMOTION_API AMachineGun : public AActor
{
    GENERATED_BODY()

public:
    AMachineGun(); // ������

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* GunMesh; // �޽� ������Ʈ
};
