#include "EnemyShooterGrenade.h"
#include "Components/SphereComponent.h" // 구체 컴포넌트 사용
#include "Components/StaticMeshComponent.h" // 스태틱 메쉬 컴포넌트 사용
#include "GameFramework/ProjectileMovementComponent.h" // 투사체 이동 컴포넌트 사용
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수 사용
#include "NiagaraFunctionLibrary.h" // 나이아가라 이펙트 스폰 함수 사용
#include "MainCharacter.h" // 플레이어 캐릭터 참조
#include "Engine/OverlapResult.h" // FOverlapResult 구조체 사용
#include "Components/AudioComponent.h"

AEnemyShooterGrenade::AEnemyShooterGrenade()
{
	PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화

	// 충돌 컴포넌트 생성 및 설정
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	RootComponent = CollisionComp; // 루트 컴포넌트로 설정
	CollisionComp->SetCollisionProfileName("Projectile"); // 기본 충돌 프로파일 설정
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
	ProjectileMovement->bRotationFollowsVelocity = true; // 이동 방향에 따라 액터가 자동으로 회전하도록 설정
	ProjectileMovement->bShouldBounce = true; // 벽이나 바닥에 닿았을 때 튕기도록 설정
	ProjectileMovement->ProjectileGravityScale = 1.0f; // 중력 적용 (포물선 운동)
	ProjectileMovement->Bounciness = 0.3f; // 탄성 (0~1, 1에 가까울수록 많이 튐)
	ProjectileMovement->Friction = 0.5f; // 마찰력 (바닥에 구를 때의 저항)

	FuseAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FuseAudioComp"));
	FuseAudioComponent->SetupAttachment(RootComponent);
	FuseAudioComponent->bAutoActivate = false; // 기본적으로 비활성화
}

void AEnemyShooterGrenade::BeginPlay()
{
	Super::BeginPlay(); // 부모 클래스 BeginPlay 호출

	if (ProjectileMovement)
	{
		ProjectileMovement->OnProjectileBounce.AddDynamic(this, &AEnemyShooterGrenade::OnBounce);
	}

	// FuseTime(3초) 후에 Explode 함수를 호출하도록 타이머 설정
	FTimerHandle FuseTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(FuseTimerHandle, this, &AEnemyShooterGrenade::Explode, FuseTime, false);
}

void AEnemyShooterGrenade::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // 부모 클래스 Tick 호출

	// 매 프레임마다 수류탄 메쉬를 Yaw축 기준으로 회전시켜 시각적 효과를 줌
	GrenadeMesh->AddLocalRotation(FRotator(0.f, RotationSpeed * DeltaTime, 0.f));
}

void AEnemyShooterGrenade::LaunchGrenade(FVector LaunchVelocity)
{
	// 계산된 발사 속도를 투사체 이동 컴포넌트에 적용
	ProjectileMovement->Velocity = LaunchVelocity;
	ProjectileMovement->Activate(); // 이동 컴포넌트를 활성화하여 물리 시뮬레이션 시작

	// [추가] 수류탄 발사 시 FuseSound 재생 시작 (루프)
	if (FuseSound && FuseAudioComponent)
	{
		FuseAudioComponent->SetSound(FuseSound);
		FuseAudioComponent->Play();
	}
}

// [추가] 튕길 때 호출될 함수 구현
void AEnemyShooterGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	// BounceSound가 설정되어 있으면 재생
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BounceSound, ImpactResult.Location);
	}
}

void AEnemyShooterGrenade::Explode()
{
	if (bHasExploded) return; // 중복 폭발 방지
	bHasExploded = true; // 폭발 상태로 전환

	if (FuseAudioComponent)
	{
		FuseAudioComponent->Stop();
	}

	// 폭발 반경 내 플레이어에게만 피해 적용
	FVector ExplosionCenter = GetActorLocation(); // 폭발 중심점
	TArray<FOverlapResult> Overlaps;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ExplosionRadius); // 폭발 반경만큼의 구체 생성

	// Pawn 채널에 대해서만 오버랩(범위) 검사 수행
	bool bHasOverlaps = GetWorld()->OverlapMultiByChannel(
		Overlaps, ExplosionCenter, FQuat::Identity, ECC_Pawn, CollisionShape
	);

	if (bHasOverlaps) // 범위 내에 Pawn이 감지되었다면
	{
		ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		for (auto& Overlap : Overlaps) // 모든 감지된 액터에 대해
		{
			if (Overlap.GetActor() == PlayerCharacter) // 만약 플레이어라면
			{
				// 데미지 적용
				UGameplayStatics::ApplyDamage(
					PlayerCharacter, ExplosionDamage,
					GetInstigator() ? GetInstigator()->GetController() : nullptr, this, nullptr
				);
				break; // 플레이어는 한 명이므로 찾으면 바로 중단
			}
		}
	}

	//DrawDebugSphere(GetWorld(), ExplosionCenter, ExplosionRadius, 32, FColor::Red, false, 1.0f, 0, 2.0f); // 디버그용 폭발 범위 시각화

	// 폭발 이펙트 생성
	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), ExplosionEffect, GetActorLocation(), GetActorRotation(), FVector(1.0f), true, true
		);
	}

	// 폭발 사운드 재생
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