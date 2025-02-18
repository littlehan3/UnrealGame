#include "Knife.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "MainCharacter.h" // 여기서만 include
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

AKnife::AKnife()
{
    PrimaryActorTick.bCanEverTick = true;

    // 칼 메시 생성
    KnifeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KnifeMesh"));
    RootComponent = KnifeMesh;

    // 히트 박스 생성 (블루프린트에서 수정 가능)
    HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
    HitBox->SetupAttachment(KnifeMesh);
    HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 기본적으로 비활성화

    // 충돌 감지 이벤트 바인딩
    HitBox->OnComponentBeginOverlap.AddDynamic(this, &AKnife::OnHitBoxOverlap);

    KnifeType = EKnifeType::Left;

    ComboDamages = { 20.0f, 25.0f, 30.0f, 0.0f, 50.0f }; // 4번째(발차기) 공격은 캐릭터에서 독립적으로 데미지 처리

}

void AKnife::BeginPlay()
{
    Super::BeginPlay();
}

void AKnife::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// 칼 초기화
void AKnife::InitializeKnife(EKnifeType NewType)
{
    KnifeType = NewType;
}

// 콤보 인덱스에 맞는 데미지 설정 후 히트박스 활성화
void AKnife::EnableHitBox(int32 ComboIndex)
{
    // 4번째 콤보(발차기)에서는 나이프 히트박스를 활성화하지 않음
    if (ComboIndex == 3)
    {
        return;
    }

    if (ComboDamages.IsValidIndex(ComboIndex))
    {
        CurrentDamage = ComboDamages[ComboIndex];
        UE_LOG(LogTemp, Warning, TEXT("EnableHitBox() called with ComboIndex: %d | Damage Set to: %f"), ComboIndex, CurrentDamage);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid ComboIndex: %d! Cannot retrieve damage."), ComboIndex);
    }

    HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    UE_LOG(LogTemp, Warning, TEXT("Knife HitBox Enabled! Damage: %f | ComboIndex: %d"), CurrentDamage, ComboIndex);

    // 본인과의 충돌 무시
	AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        HitBox->MoveIgnoreActors.Add(OwnerActor); // 본인의 캐릭터와 충돌하지 않도록 설정
        UE_LOG(LogTemp, Warning, TEXT("MoveIgnoreActors 추가: %s"), *OwnerActor->GetName());
    }
    else
    {
		UE_LOG(LogTemp, Error, TEXT("Error: Knife has no owner!"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Knife HitBox Enabled! Damage: %f"), CurrentDamage);
}


// 히트 박스 비활성화
void AKnife::DisableHitBox()
{
    HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    UE_LOG(LogTemp, Warning, TEXT("Knife HitBox Disabled!"));
}

// 히트 판정 처리 (적과 충돌 시 데미지 적용)
void AKnife::OnHitBoxOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherActor)
    {
        return;
    }

    // 본인 또는 본인의 소유 오브젝트인지 확인 후 무시
    if (OtherActor == GetOwner())
    {
        static bool bAlreadyLogged = false; // 처음 한 번만 로그 출력
        if (!bAlreadyLogged)
        {
            UE_LOG(LogTemp, Warning, TEXT("Hit Ignore Self: %s"), *OtherActor->GetName());
            bAlreadyLogged = true; // 이후에는 로그 출력 안 함
        }
        return;
    }

    // 적에게 데미지 적용
    UGameplayStatics::ApplyDamage(OtherActor, CurrentDamage, nullptr, this, nullptr);
    UE_LOG(LogTemp, Warning, TEXT("Knife Hit! Applied %f Damage to %s"), CurrentDamage, *OtherActor->GetName());

    DisableHitBox();
}



