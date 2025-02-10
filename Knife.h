#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Knife.generated.h"

UENUM(BlueprintType)
enum class EKnifeType : uint8
{
    Left  UMETA(DisplayName = "Left"),
    Right UMETA(DisplayName = "Right")
};

UCLASS()
class LOCOMOTION_API AKnife : public AActor
{
    GENERATED_BODY()

public:
    AKnife();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // 🔹 나이프 메시 추가
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* KnifeMesh;

    // 🔹 나이프 타입 (왼손 / 오른손)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knife")
    EKnifeType KnifeType;

    // 🔹 나이프 초기화 함수
    void InitializeKnife(EKnifeType NewType);
};
