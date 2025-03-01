#include "Knife.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "MainCharacter.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h" // 디버그 시각화용 헤더

AKnife::AKnife()
{
    PrimaryActorTick.bCanEverTick = true;

    // 칼 메시 생성
    KnifeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KnifeMesh"));
    RootComponent = KnifeMesh;

    // 히트 박스 생성
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
    if (ComboIndex == 3) return; // 발차기 예외 처리

    if (ComboDamages.IsValidIndex(ComboIndex)) // 콤보 인덱스가 유효한 경우 해당 데미지 설정
    {
        CurrentDamage = ComboDamages[ComboIndex];
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid ComboIndex: %d! Cannot retrieve damage."), ComboIndex);
        return;
    }

    RaycastAttack(); // 레이캐스트 실행하여 맞은 적 저장

    if (RaycastHitActor) // 레이캐스트에서 감지된 적이 있는 경우
    {
        HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);// 히트박스 활성화
        UE_LOG(LogTemp, Warning, TEXT("Knife HitBox Enabled!")); 
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid target detected by Raycast, HitBox remains disabled."));
    }
}

// 히트 박스 비활성화
void AKnife::DisableHitBox()
{
    HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RaycastHitActor = nullptr; //레이캐스트에서 감지된 적도 초기화하여 다음 공격에서 새롭게 체크 가능하도록 처리
    UE_LOG(LogTemp, Warning, TEXT("Knife HitBox Disabled!"));
}


void AKnife::RaycastAttack()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return;

    FVector StartLocation = OwnerActor->GetActorLocation() + (OwnerActor->GetActorForwardVector() * 20.0f); // 뒤에 있는 적 히트 방지를 위해 캐릭터로부터 해당거리 만큼 떨어진 곳에서 레이캐스트 시작
    FVector EndLocation = StartLocation + (OwnerActor->GetActorForwardVector() * 150.0f); // 해당 길이만큼 레이캐스트 발사

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerActor);  // 자기 자신 무시

    bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params); // 레이캐스트 실행(적이 감지되면 bHit = true)

    // 디버그 시각화
    FColor LineColor = bHit ? FColor::Red : FColor::Green; // 디버그 시각화(빨간색 = 적중, 초록색 = 미적중)
    DrawDebugLine(GetWorld(), StartLocation, EndLocation, LineColor, false, 1.0f, 0, 3.0f); // 앞쪽으로만 공격 범위 표시

    if (bHit)
    {
        DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 10.0f, 12, FColor::Red, false, 1.0f); // 충돌한 지점을 빨간색 구체로 시각화
    }

    RaycastHitActor = bHit ? HitResult.GetActor() : nullptr; // 레이캐스트 적중 여부에 따라 RaycastHitActor 저장 (적이 없으면 nullptr)
}

void AKnife::OnHitBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) // 히트박스 충돌 감지 (히트박스에 감지된 적이 레이캐스트에 감지된 적과 동일해야 데미지 적용)
{
    if (!OtherActor || OtherActor == GetOwner()) return; // 충돌한 액터가 없거나 자기 자신이면 무시

    if (OtherActor == RaycastHitActor) // 레이캐스트에서 감지된 적과 히트박스에서 감지된 적이 동일하다면
    {
        UGameplayStatics::ApplyDamage(OtherActor, CurrentDamage, nullptr, this, nullptr); // 데미지 적용
        UE_LOG(LogTemp, Warning, TEXT("Knife Hit! Applied %f Damage to %s"), CurrentDamage, *OtherActor->GetName()); 

        DisableHitBox(); // 히트박스 비활성화 (중복히트 방지)
    }
    else
    {
        // 레이캐스트에서 감지되지 않은 적은 무효 처리 (앞쪽 적만 맞도록)
        UE_LOG(LogTemp, Warning, TEXT("HitBox ignored %s because Raycast didn't detect it"), *OtherActor->GetName());
    }
}