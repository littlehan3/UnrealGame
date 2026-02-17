#include "EnemyShooterGrenade.h"
#include "Components/SphereComponent.h" // 구체 컴포넌트 사용
#include "Components/StaticMeshComponent.h" // 스태틱 메쉬 컴포넌트 사용
#include "GameFramework/ProjectileMovementComponent.h" // 투사체 이동 컴포넌트 사용
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수 사용
#include "NiagaraFunctionLibrary.h" // 나이아가라 이펙트 스폰 함수 사용
#include "MainCharacter.h" // 플레이어 캐릭터 참조
#include "Engine/OverlapResult.h" // FOverlapResult 구조체 사용
#include "Components/AudioComponent.h"
#include "GameFramework/RotatingMovementComponent.h"

AEnemyShooterGrenade::AEnemyShooterGrenade()
{
	PrimaryActorTick.bCanEverTick = false; // Tick 비활성화

	// 충돌 컴포넌트 생성 및 설정
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	RootComponent = CollisionComp; // 루트 컴포넌트로 설정

	CollisionComp->SetCollisionProfileName("Projectile"); // 기본 충돌 프로파일 설정
	CollisionComp->SetGenerateOverlapEvents(false); // 폭발 전엔 오버랩 필요 없음
	CollisionComp->InitSphereRadius(15.0f); // 충돌 반경 설정

	// 땅(WorldStatic)과 충돌했을 때 막히도록(Block) 설정하여 땅에 튀길 수 있도록 함
	CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	// 물리 오브젝트(PhysicsBody)와도 충돌하여 상호작용하도록 설정
	CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Block);

	// 메쉬 컴포넌트 생성 및 설정
	GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	GrenadeMesh->SetupAttachment(RootComponent); // 충돌 컴포넌트에 부착
	GrenadeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 메쉬 자체의 충돌은 사용 안함

	// 투사체 이동 컴포넌트 생성 및 설정
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = RootComponent; // 이 컴포넌트가 제어할 대상을 루트 컴포넌트로 지정
	ProjectileMovement->InitialSpeed = 0.f; // 초기 속도는 LaunchGrenade에서 설정
	ProjectileMovement->MaxSpeed = 3000.f; // 최대 속도 제한
	ProjectileMovement->bRotationFollowsVelocity = false; // 수류탄이 이동 방향에 따라 회전하지 않도록 설정 (URotatingMovementComponent로 자체 회전 처리)
	ProjectileMovement->bShouldBounce = true; // 벽이나 바닥에 닿았을 때 튕기도록 설정
	ProjectileMovement->ProjectileGravityScale = 1.0f; // 중력 적용 (포물선 운동)
	ProjectileMovement->Bounciness = 0.3f; // 탄성 (0~1, 1에 가까울수록 많이 튐)
	ProjectileMovement->Friction = 0.5f; // 마찰력 (바닥에 구를 때의 저항)

	FuseAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FuseAudioComp"));
	FuseAudioComponent->SetupAttachment(RootComponent);
	FuseAudioComponent->bAutoActivate = false; // 기본적으로 비활성화

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingComp"));
	RotatingMovement->RotationRate = FRotator(-360.0f, 0.0f, 0.0f); // Yaw축을 기준으로 초당 360도 회전하도록 설정하여 수류탄이 발사 방향과 상관없이 자체적으로 회전하도록 함
	RotatingMovement->bRotationInLocalSpace = true; // 로컬 공간에서 회전하도록 설정하여 수류탄이 발사 방향과 상관없이 자체적으로 회전하도록 함
}

void AEnemyShooterGrenade::BeginPlay()
{
	Super::BeginPlay(); // 부모 클래스 BeginPlay 호출

	UWorld* World = GetWorld();
	if (!World) return;

	if (!ProjectileMovement) return;

	ProjectileMovement->OnProjectileBounce.AddDynamic(this, &AEnemyShooterGrenade::OnBounce); // 튕길 때 OnBounce 함수 호출하도록 바인딩

	TWeakObjectPtr<AEnemyShooterGrenade> WeakThis(this); // 람다 함수에서 안전하게 this 참조하기 위한 약한 포인터
	World->GetTimerManager().SetTimer(FuseTimerHandle, [WeakThis]()
		{
			if (WeakThis.IsValid()) // 수류탄이 아직 존재한다면 폭발 처리 함수 호출
			{
				WeakThis->Explode(); // 폭발 처리 함수 호출
			}
		}, FuseTime, false); // 발사 후 FuseTime 초가 지나면 단발성으로 타이머 실행
}

//void AEnemyShooterGrenade::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime); // 부모 클래스 Tick 호출
//
//	// 매 프레임마다 수류탄 메쉬를 Yaw축 기준으로 회전시켜 시각적 효과를 줌
//	GrenadeMesh->AddLocalRotation(FRotator(0.f, RotationSpeed * DeltaTime, 0.f));
//}

void AEnemyShooterGrenade::LaunchGrenade(FVector LaunchVelocity)
{
	if (!ProjectileMovement) return;
	if (!FuseSound && !FuseAudioComponent) return;

	ProjectileMovement->Velocity = LaunchVelocity; // 발사 속도 설정
	ProjectileMovement->Activate(); // 이동 컴포넌트를 활성화하여 물리 시뮬레이션 시작

	FuseAudioComponent->SetSound(FuseSound);
	FuseAudioComponent->Play();
}

void AEnemyShooterGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (!BounceSound) return;

	UGameplayStatics::PlaySoundAtLocation(this, BounceSound, ImpactResult.Location);
}

void AEnemyShooterGrenade::Explode()
{

	if (bHasExploded) return; // 중복 폭발 방지
	bHasExploded = true; // 폭발 상태로 전환

	UWorld* World = GetWorld();
	if (!World) return;

	if (!CollisionComp || !GrenadeMesh || !ProjectileMovement) // 필요한 컴포넌트가 없으면 폭발 처리할 수 없으므로 액터 제거
	{
		Destroy(); // 필요한 컴포넌트가 없으면 액터 제거
		return;
	}
	
	if (FuseAudioComponent)
	{
		FuseAudioComponent->Stop(); // 퓨즈 사운드 중지
	}
	World->GetTimerManager().ClearTimer(FuseTimerHandle); // 타이머 핸들 정리 추가

	// 4. 플레이어 데미지 처리
	FVector ExplosionCenter = GetActorLocation();
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(World, 0);

	if (PlayerCharacter)
	{
		// OverlapMultiByChannel 대신 Dist를 사용하여 기존 TArray<FOverlapResult> 메모리 할당을 하지 않아도 되도록 변경 (히트 대상이 플레이어 뿐이기에)
		float DistanceToPlayer = FVector::Dist(ExplosionCenter, PlayerCharacter->GetActorLocation());
		if (DistanceToPlayer <= ExplosionRadius)
		{
			UGameplayStatics::ApplyDamage(
				PlayerCharacter, ExplosionDamage,
				GetInstigatorController(), this, nullptr
			);
		}
	}

	//DrawDebugSphere(GetWorld(), ExplosionCenter, ExplosionRadius, 32, FColor::Red, false, 1.0f, 0, 2.0f); // 디버그용 폭발 범위 시각화

	if (ExplosionEffect)
	{
		// 폭발 이펙트 생성
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, ExplosionEffect, GetActorLocation(), GetActorRotation(), FVector(1.0f), true, true);
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}

	// 폭발 후 정리
	GrenadeMesh->SetVisibility(false); // 메쉬를 즉시 보이지 않게 설정
	ProjectileMovement->StopMovementImmediately(); // 물리적 움직임을 정지
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 비활성화
	SetActorTickEnabled(false); // 틱 비활성화
	Destroy(); // 액터를 월드에서 제거
}