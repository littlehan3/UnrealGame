#include "BossProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "MainCharacter.h"
#include "Components/AudioComponent.h"

ABossProjectile::ABossProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision")); // 충돌 컴포넌트 생성
	CollisionComponent->InitSphereRadius(CollisionRadius); // 구체 충돌체 반경 설정
	CollisionComponent->SetCollisionProfileName(TEXT("EnemyProjectile"));  // 충돌 프로필 설정
	CollisionComponent->OnComponentHit.AddDynamic(this, &ABossProjectile::OnHit); // 충돌 이벤트 바인딩
	RootComponent = CollisionComponent; // 루트 컴포넌트로 설정

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh")); // 메시 컴포넌트 생성
	MeshComponent->SetupAttachment(RootComponent); // 루트에 부착
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 메시 충돌 비활성화

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement")); // 투사체 이동 컴포넌트 생성
	ProjectileMovement->InitialSpeed = 1200.f; // 초기 속도 설정
	ProjectileMovement->MaxSpeed = 1200.f; // 최대 속도 설정
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 중력 영향 없음
	ProjectileMovement->bRotationFollowsVelocity = true; // 속도 방향으로 회전

	InitialLifeSpan = 4.0f; // 수명 설정
}

void ABossProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (!IsValid(Shooter) || !IsValid(LoopingFlightSound)) return;
    
    CollisionComponent->IgnoreActorWhenMoving(Shooter, true); // 발사자와 충돌 무시

    FlightAudioComponent = UGameplayStatics::SpawnSoundAttached( 
        LoopingFlightSound, RootComponent, NAME_None,
        FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true); // 투사체에서 투사체 비행 소리를 반복재생
  
}

void ABossProjectile::FireInDirection(const FVector& ShootDir)
{
	if (!IsValid(ProjectileMovement)) return;

    ProjectileMovement->Velocity = ShootDir.GetSafeNormal() * ProjectileMovement->InitialSpeed;
}

void ABossProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector Impulse,
    const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == this || OtherActor == Shooter) return; // 다른 액터가 없거나 자기 자신이거나 발사자라면

    UWorld* World = GetWorld();
    if (!World) return;

	if (IsValid(FlightAudioComponent))  // 유효성검사
    {
		FlightAudioComponent.Get()->Stop(); // 비행 소리 중지
    }

	if (IsValid(ExplosionEffect)) // 유효성검사
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, ExplosionEffect, GetActorLocation(), FRotator::ZeroRotator); // 폭발 이펙트 재생
    }

	if (IsValid(ExplosionSound)) // 유효성검사
    {
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation()); // 폭발 소리 재생
    }

	ApplyAreaDamage(); // 광역 데미지 적용
    Destroy(); // 파괴
}

void ABossProjectile::ApplyAreaDamage()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (!IsValid(Shooter)) return;

	const FVector Center = GetActorLocation(); // 폭발 중심 위치 선언
    /*DrawDebugSphere(GetWorld(), Center, DamageRadius, 32, FColor::Red, false, 3.0f);*/

    // 반경 내 액터 직접 찾기
	TArray<FOverlapResult> Overlaps; // 오버랩 결과 배열
	FCollisionShape Sphere = FCollisionShape::MakeSphere(DamageRadius); // 구체 충돌체 생성

    bool bHasOverlaps = World->OverlapMultiByObjectType(
		Overlaps, // 오버랩 결과 저장용
		Center, // 중심 위치
		FQuat::Identity, // 회전 없음
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn), // Pawn만 타겟팅
		Sphere // 충돌체
    );

	if (bHasOverlaps) // 오버랩된 액터가 있다면
    {
        // 중복 히트 제거용 TSet
		TSet<AActor*> UniqueActors; // 중복 없는 액터 집합
		for (const FOverlapResult& Overlap : Overlaps) // 오버랩 결과 순회
        {
			AActor* HitActor = Overlap.GetActor(); // 피격된 액터 가져오기
			if (HitActor && HitActor != Shooter && HitActor->IsA<AMainCharacter>()) // 유효한 액터이고 발사자가 아니고	메인 캐릭터라면
            {
				UniqueActors.Add(HitActor); // 중복 제거용 집합에 추가
            }
        }

		AController* InstigatorController = Shooter->GetInstigatorController(); // 발사자의 컨트롤러 가져오기
		for (AActor* HitActor : UniqueActors) // 중복 제거된 액터들에 대해
        {
            UGameplayStatics::ApplyDamage(
				HitActor, // 피격된 액터
				Damage, // 데미지 양
				InstigatorController, // 데미지 주체 (컨트롤러)
				this, // 데미지를 준 액터
                UDamageType::StaticClass() // 데미지 타입
            );
        }
    }
}

