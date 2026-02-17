#include "Knife.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "MainCharacter.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h" // 디버그 시각화용 헤더
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

AKnife::AKnife()
{
    PrimaryActorTick.bCanEverTick = false; // Tick 비활성화

    // 칼 메시 생성
    KnifeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KnifeMesh"));
    RootComponent = KnifeMesh; // 루트 컴포넌트로 설정

    // 히트 박스 생성
    HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
    HitBox->SetupAttachment(KnifeMesh); // 나이프 메쉬에 부착
    HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 비활성화

    // 충돌 감지 이벤트 바인딩
    HitBox->OnComponentBeginOverlap.AddDynamic(this, &AKnife::OnHitBoxOverlap);

    // 기본 데이터 세팅
    KnifeType = EKnifeType::Left;
    ComboDamages.SetNum(5); // 콤보데미지 배열 직접 초기화로 BP수정 가능하게끔
    ComboDamages[0] = 20.0f; // 콤보1 데미지
    ComboDamages[1] = 25.0f; // 콤보2 데미지
    ComboDamages[2] = 30.0f; // 콤보3 데미지
    ComboDamages[3] = 0.0f;  // 발차기는 메인캐릭터에서 관리하기에 0
    ComboDamages[4] = 50.0f; // 콤보5 데미지
}

// 칼 초기화
void AKnife::InitializeKnife(EKnifeType NewType)
{
    KnifeType = NewType;
}

// 콤보 인덱스에 맞는 데미지 설정 후 히트박스 활성화
void AKnife::EnableHitBox(int32 ComboIndex, float KnockbackStrength)
{
    if (ComboIndex == 3) return; // 발차기 예외 처리
    if (!ComboDamages.IsValidIndex(ComboIndex)) return; // 콤보 인덱스 유효성 검사

    CurrentDamage = ComboDamages[ComboIndex]; // 콤보에 맞는 데미지 값 저장
    CurrentKnockbackStrength = KnockbackStrength; // 넉백 강도 저장

    // 소유자인 메인캐릭터 유효성 검사 후
    if (AMainCharacter* MainChar = Cast<AMainCharacter>(GetOwner()))
    {
        KnifeHitEffectOffset = MainChar->GetKnifeEffectOffset(); // 이펙트 오프셋값 불러옴
    }

    // 이전 공격 기록을 초기화
    DamagedActors.Empty();
    RaycastHitActors.Empty(); 

    RaycastAttack(); // 공격 수행

    if (RaycastHitActors.Num() > 0) // 감지된 적이 있는 경우
    {
        HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 히트박스 충돌 활성화
        UE_LOG(LogTemp, Warning, TEXT("Knife HitBox Enabled! Targets: %d"), RaycastHitActors.Num());
    }
}

// 히트 박스 비활성화
void AKnife::DisableHitBox()
{
    HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 히트박스 충돌 비활성화
    // 리스트를 비움
    RaycastHitActors.Empty();
    DamagedActors.Empty();
}

void AKnife::RaycastAttack()
{
    AActor* OwnerActor = GetOwner();
    UWorld* World = GetWorld();
    if (!IsValid(OwnerActor) || !World) return;

    // 플레이어 전방 위치에서 연산 시작
    const FVector Forward = OwnerActor->GetActorForwardVector();
    const FVector StartLocation = OwnerActor->GetActorLocation() + (Forward * TraceStartOffset);

    float HalfAngle = AttackAngle / 2.0f; 

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerActor); // 소유자 무시
    Params.AddIgnoredActor(this); // 나이프 본인 무시

    RaycastHitActors.Empty(); // 맞은엑터 목록 초기화
    FirstHitResult.Reset(); // 첫번째 히트 대상 초기화

    bool bNiagaraPlayed = false; // 나이아가라 재생 여부 플래그
    bool bSoundPlayed = false; // 사운드 재생 여부 플래그

    for (int32 i = 0; i < AttackRayCount; i++) 
    {
        // 보간으로 레이 발사 각도 계산
        float t = float(i) / (AttackRayCount - 1);
        float AngleOffset = FMath::Lerp(-HalfAngle, HalfAngle, t);
        FVector Direction = OwnerActor->GetActorForwardVector().RotateAngleAxis(AngleOffset, FVector::UpVector);
        FVector EndLocation = StartLocation + Direction * AttackRadius;

        // Sphere Trace 다중 감지
        TArray<FHitResult> OutHits;
        bool bHit = World->SweepMultiByChannel(
            OutHits,
            StartLocation,
            EndLocation,
            FQuat::Identity,
            ECC_Pawn,
            FCollisionShape::MakeSphere(TraceSphereRadius),
            Params
        );

        // 스피어 트레이스 디버깅
       /* FColor TraceColor = bHit ? FColor::Red : FColor::Green;
        DrawDebugLine(GetWorld(), StartLocation, EndLocation, TraceColor, false, 1.0f, 0, 2.0f);*/

        if (bHit)
        {
            for (const FHitResult& Hit : OutHits)
            {
                AActor* HitActor = Hit.GetActor();
                if (IsValid(HitActor) && !RaycastHitActors.Contains(HitActor))
                {
                    RaycastHitActors.Add(HitActor);
                    
                    // 피격 위치 구체로 디버깅
                    // DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.0f, 12, FColor::Red, false, 1.0f);
                    FVector ImpactPointWithOffset = Hit.ImpactPoint + (Forward * KnifeHitEffectOffset);

                    // 히트 나이아가라 이펙트 재생
                    if (HitNiagaraEffect && !bNiagaraPlayed) // 이펙트는 한 번만 재생
                    {
                        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                            World,
                            HitNiagaraEffect,
                            ImpactPointWithOffset, // 디버그 구체와 동일한 위치
                            Hit.Normal.Rotation(), // 피격 면의 노멀 방향으로 회전
                            FVector(1.0f), // 크기
                            true,
                            true
                        );
                        bNiagaraPlayed = true; // 이펙트 재생 플래그 설정
                    }
                    // 히트 사운드 재생
                    if (HitSound && !bSoundPlayed) // 사운드도 한 번만 재생
                    {
                        UGameplayStatics::PlaySoundAtLocation(
                            World,
                            HitSound, 
                            ImpactPointWithOffset, // 이펙트와 동일한 위치
                            Hit.Normal.Rotation() // 피격 면의 노멀 방향으로 화전
                        );
                        bSoundPlayed = true; // 사운드 재생 플래그 설정
                    }
                }
            }
        }
    }
}

void AKnife::OnHitBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 유효하지 않은 엑터나 본인은 무시
    if (!IsValid(OtherActor) || OtherActor == GetOwner() || OtherActor == this) return;

    // 레이캐스트에 감지된 적이면서 이번 공격에서 아직 데미지를 받지 않은 대상인 경우
    if (RaycastHitActors.Contains(OtherActor) && !DamagedActors.Contains(OtherActor))
    {
        UGameplayStatics::ApplyDamage(OtherActor, CurrentDamage, nullptr, this, nullptr); // 데미지 적용

        ACharacter* HitCharacter = Cast<ACharacter>(OtherActor); // 맞은 캐릭터 가져옴
        AActor* OwnerActor = GetOwner(); // 소유자인 메인 캐릭터 가져옴

        if (HitCharacter && HitCharacter->GetCharacterMovement() && OwnerActor) 
        {
            // 메인캐릭터의 전방 벡터를 넉백 방향으로 사용
            FVector LaunchDir = OwnerActor->GetActorForwardVector(); // 메인캐릭터 전방 벡터 가져옴
            LaunchDir.Z = 0; // 수평으로만 밀어냄
            LaunchDir.Normalize(); // 정규화
            HitCharacter->LaunchCharacter(LaunchDir * CurrentKnockbackStrength, true, false); // 넉백 강도만큼 Launch
        }
        DamagedActors.Add(OtherActor); // 피해를 입은 엑터 목록에 추가
        UE_LOG(LogTemp, Warning, TEXT("Knife Hit! Applied %f Damage to %s"), CurrentDamage, *OtherActor->GetName());
    }
}