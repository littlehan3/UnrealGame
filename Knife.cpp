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

    DamagedActors.Empty(); // 중복 히트 방지를 위해 초기화

    if (RaycastHitActors.Num() > 0)
    {
        HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        UE_LOG(LogTemp, Warning, TEXT("Knife HitBox Enabled! Targets: %d"), RaycastHitActors.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid targets in Raycast."));
    }
}

// 히트 박스 비활성화
void AKnife::DisableHitBox()
{
    HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RaycastHitActors.Empty();
    DamagedActors.Empty();
    UE_LOG(LogTemp, Warning, TEXT("Knife HitBox Disabled!"));
}


void AKnife::RaycastAttack()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return;

    FVector StartLocation = OwnerActor->GetActorLocation() + (OwnerActor->GetActorForwardVector() * 20.0f);
    float Radius = 180.0f;
    float Angle = 60.0f;
    int RayCount = 9;
    float HalfAngle = Angle / 2.0f;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerActor);

    RaycastHitActors.Empty();

    for (int i = 0; i < RayCount; ++i)
    {
        float t = float(i) / (RayCount - 1);
        float AngleOffset = FMath::Lerp(-HalfAngle, HalfAngle, t);
        FVector Direction = OwnerActor->GetActorForwardVector().RotateAngleAxis(AngleOffset, FVector::UpVector);
        FVector EndLocation = StartLocation + Direction * Radius;

        // Sphere Trace 다중 감지
        TArray<FHitResult> OutHits;
        bool bHit = GetWorld()->SweepMultiByChannel(
            OutHits,
            StartLocation,
            EndLocation,
            FQuat::Identity,
            ECC_Pawn,
            FCollisionShape::MakeSphere(20.0f),
            Params
        );

        // 디버그 라인 그리기
        FColor TraceColor = bHit ? FColor::Red : FColor::Green;
        DrawDebugLine(GetWorld(), StartLocation, EndLocation, TraceColor, false, 1.0f, 0, 2.0f);

        if (bHit)
        {
            for (const FHitResult& Hit : OutHits)
            {
                AActor* HitActor = Hit.GetActor();
                if (HitActor && !RaycastHitActors.Contains(HitActor))
                {
                    RaycastHitActors.Add(HitActor);
                    DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.0f, 12, FColor::Red, false, 1.0f);
                }
            }
        }
    }
}


void AKnife::OnHitBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == GetOwner()) return;

    if (RaycastHitActors.Contains(OtherActor) && !DamagedActors.Contains(OtherActor))
    {
        UGameplayStatics::ApplyDamage(OtherActor, CurrentDamage, nullptr, this, nullptr);
        DamagedActors.Add(OtherActor);

        UE_LOG(LogTemp, Warning, TEXT("Knife Hit! Applied %f Damage to %s"), CurrentDamage, *OtherActor->GetName());
    }
    else
    {
        //UE_LOG(LogTemp, Warning, TEXT("HitBox ignored %s"), *OtherActor->GetName());
    }
}