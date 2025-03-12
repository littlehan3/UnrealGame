#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Knife.generated.h"

// 전방 선언
class AMainCharacter;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* KnifeMesh; // 나이프 메시

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
    class UBoxComponent* HitBox; // 히트박스

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knife")
    EKnifeType KnifeType; // 나이프 타입 (왼손 / 오른손)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knife")
    TArray<float> ComboDamages; // 콤보별 데미지 설정

    void InitializeKnife(EKnifeType NewType); // 콤보 인덱스를 전달받아 해당 공격의 데미지를 설정
    void EnableHitBox(int32 ComboIndex);
    void DisableHitBox();

private:
    float CurrentDamage;

    // 레이캐스트 공격
    void RaycastAttack();

    // 레이캐스트로 감지된 적을 저장
    AActor* RaycastHitActor;

    // 히트 판정 함수 (충돌 감지)
    UFUNCTION()
    void OnHitBoxOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

};
