#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Knife.generated.h"

// 전방 선언
class AMainCharacter;
class UNiagaraSystem;
class USoundBase;
class UBoxComponent;
class UStaticMeshComponent;

// 나이프 장착 및 공격 위치를 정의하는 열거형
UENUM(BlueprintType) 
enum class EKnifeType : uint8
{
    Left  UMETA(DisplayName = "Left"), // 왼손 장착 및 공격 판정
    Right UMETA(DisplayName = "Right") // 오른손 장착 및 공격 판정
};

UCLASS()
class LOCOMOTION_API AKnife : public AActor
{
    GENERATED_BODY()

public:
    AKnife();

    void InitializeKnife(EKnifeType NewType); // 나이프 초기화 함수
    void EnableHitBox(int32 ComboIndex, float KnockbackStrength); // 공격 활성화
    void DisableHitBox(); // 공격 비활성화

private:
    void RaycastAttack(); // 레이캐스트 발사 함수

    // 레이캐스트로 감지된 적을 저장
    TArray<AActor*> RaycastHitActors; // 레이캐스트로 감지된 적들
    TSet<AActor*> DamagedActors; // 중복 히트를 방지하는 집합
    FHitResult FirstHitResult; // 래이캐스트로 감지된 첫 번째 피격 정보를 저장할 변수

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
    
    UPROPERTY()
    EKnifeType KnifeType; // 나이프 타입 (왼손 / 오른손)

    UPROPERTY()
    float CurrentDamage; // 현재 적용할 데미지 변수
    UPROPERTY()
    float CurrentKnockbackStrength; // 현재 적용할 넉백 강도
    UPROPERTY()
    float KnifeHitEffectOffset; // 히트 이펙트 위치 오프셋값

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent* KnifeMesh; // 나이프 메시

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UBoxComponent* HitBox; // 히트박스

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knife|VFX", meta = (AllowPrivateAccess = "true"))
    class UNiagaraSystem* HitNiagaraEffect; // 히트 이펙트

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knife|VFX", meta = (AllowPrivateAccess = "true"))
    class USoundBase* HitSound; // 히트 사운드

    // 콤보별 데미지를 배열로 관리 (0: 약공격1, 1: 약공격2, 2: 약공격3, 3: 발차기, 4: 강공격)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knife|Stats", meta = (AllowPrivateAccess = "true"))
    TArray<float> ComboDamages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knife|Stats", meta = (AllowPrivateAccess = "true"))
    float TraceStartOffset = 20.0f; // 캐릭터 중심으로부터 공격 판정이 시작될 전방 오프셋
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knife|Stats", meta = (AllowPrivateAccess = "true"))
    float AttackRadius = 180.0f; // 공격 사거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knife|Stats", meta = (AllowPrivateAccess = "true"))
    float AttackAngle = 60.0f; // 부채꼴 공격 각도
    // 공격각도에 레이개수-1 을 나누면 레이 간의 간격이 나오는데 각 레이마다 반지름을 가지기 때문에 빈틈없이 레이를 쏠 수 있음
    // 적으면 성능 유리 but 판정 느슨, 많으면 판정 유리 but 성능 부담
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knife|Stats", meta = (AllowPrivateAccess = "true"))
    int32 AttackRayCount = 9; // 감지 정밀도 (레이 개수) 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knife|Stats", meta = (AllowPrivateAccess = "true"))
    float TraceSphereRadius = 20.0f; // 레이별 구체 반지름

};