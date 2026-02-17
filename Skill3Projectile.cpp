#include "Skill3Projectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h"
#include "Enemy.h"

ASkill3Projectile::ASkill3Projectile()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent")); // 충돌 컴포넌트 생성
    CollisionComponent->InitSphereRadius(CollisionRadius); // 반경 설정 생성
    CollisionComponent->SetCollisionProfileName("BlockAllDynamic"); // 충돌 성정
    CollisionComponent->OnComponentHit.AddDynamic(this, &ASkill3Projectile::OnHit); // 히트 이벤트 바인딩
    RootComponent = CollisionComponent; // 루트로 설정

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement")); // 투사체이동 컴포넌트 생성
    ProjectileMovement->InitialSpeed = 1500.0f; // 초기속도
    ProjectileMovement->MaxSpeed = 1500.0f; // 최대 속도
    ProjectileMovement->ProjectileGravityScale = 0.0f; // 중력영향 0으로 직사
    ProjectileMovement->bRotationFollowsVelocity = true; // 이동 방향에 맞춰서 회전

    InitialLifeSpan = 3.0f; // 수명 설정
}

void ASkill3Projectile::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld(); // 월드 가져옴
    if (!World) return; // 유효성 검사

    if (IsValid(Shooter)) // 시전자가 존재하면
    {
        CollisionComponent->IgnoreActorWhenMoving(Shooter, true); // 본인과 충돌하지 않음
    }

    // 비행 사운드 재생
    if (LoopingFlightSound)
    {
        FlightAudioComponent = UGameplayStatics::SpawnSoundAttached(
            LoopingFlightSound,
            RootComponent,
            NAME_None,
            FVector::ZeroVector,
            EAttachLocation::KeepRelativeOffset,
            true  // 부모 파괴시 정지
        );
    }
}

void ASkill3Projectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (!OtherActor || OtherActor == this || OtherActor == Shooter) return;

    // 비행 사운드 정지
    if (IsValid(FlightAudioComponent))
    {
        FlightAudioComponent->Stop();
    }

    // 폭발 이펙트 생성
    if (ExplosionEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            ExplosionEffect,
            GetActorLocation(),
            GetActorRotation(),
            FVector(1.0f),
            true, // 자동파괴
            true // 자동 활성화
        );
    }
    // 폭발 사운드 재생
    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
    }
    ApplyAreaDamage(); // 광역 피해 적용

    Destroy(); // 적과 충돌 시 파괴
}

void ASkill3Projectile::ApplyAreaDamage()
{
    UWorld* World = GetWorld();
    if (!World) return;

    FVector ExplosionCenter = GetActorLocation();

    //// 디버그용 폭발 범위 시각화 (빨간색 원)
    //DrawDebugSphere(
    //    GetWorld(),
    //    ExplosionCenter,
    //    DamageRadius,
    //    32,
    //    FColor::Red,
    //    false,
    //    1.5f,     // 지속 시간
    //    0,
    //    2.0f      // 선 두께
    //);

    // 피해 대상 탐색 설정
    TArray<FHitResult> HitResults; // 충돌 검사 결과를 담을 배열 선언
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes; // 검사할 엑터의 오브젝트 타입을 저장하는 배열
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn)); // 폰 채널을 감지 대상으로 설정

    // 탐색 제외 대상 설정
    TArray<AActor*> IgnoredActors; // 제외할 엑터 배열 선언
    IgnoredActors.Add(this); // 투사체 자신은 제외
    if (IsValid(Shooter)) 
    {
        IgnoredActors.Add(Shooter); // 시전자 제외
    }

    // 스피어 트레이스로 범위 내 모든 적 감지
    bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
        World, // 월드
        ExplosionCenter, // 트레이스 시작점
        ExplosionCenter, // 트레이스 끝점
        DamageRadius, // 탐색 반경
        ObjectTypes, // 폰 채널만 감지
        false, // 복합 충돌 사용 여부
        IgnoredActors, // 제외 목록
        EDrawDebugTrace::None, // 디버그 표시 여부
        HitResults, // 감지된 결과들은 이 배열에 저장
        true // 자기 자신 무시 여부
    );

    if (bHit)
    {
        // 중복 데미지 방지를 위해 TSet 사용
        TSet<AActor*> DamagedActors;

        for (const FHitResult& Hit : HitResults) 
        {
            AActor* HitActor = Hit.GetActor(); // 히트 정보에서 엑터 포인터 가져옴
            if (IsValid(HitActor) && !DamagedActors.Contains(HitActor)) // 유효한 엑터이고 아직 데미지를 입지 않았다면
            {
                // 피해량, 시전자 정보를 함께 전달하여 누가 누구에게 데미지를 입혔는지 기록
                UGameplayStatics::ApplyDamage(HitActor, Damage, nullptr, Shooter, nullptr); // 데미지 적용 
                DamagedActors.Add(HitActor); // 데미지를 입은 엑터를 Tset에 기록하여 중복데미지 방지
            }
        }
    }
}

void ASkill3Projectile::FireInDirection(const FVector& ShootDirection)
{
    if (IsValid(ProjectileMovement)) // 투사체 이동 컴포넌트가 유효하면 
    {
        ProjectileMovement->Velocity = ShootDirection * ProjectileMovement->InitialSpeed; // 발사 방향으로 초기 속도 만큼 곱해서 투사체 발사
    }
}