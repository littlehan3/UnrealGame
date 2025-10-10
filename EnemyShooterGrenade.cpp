#include "EnemyShooterGrenade.h"
#include "Components/SphereComponent.h" // ��ü ������Ʈ ���
#include "Components/StaticMeshComponent.h" // ����ƽ �޽� ������Ʈ ���
#include "GameFramework/ProjectileMovementComponent.h" // ����ü �̵� ������Ʈ ���
#include "Kismet/GameplayStatics.h" // �����÷��� ��ƿ��Ƽ �Լ� ���
#include "NiagaraFunctionLibrary.h" // ���̾ư��� ����Ʈ ���� �Լ� ���
#include "MainCharacter.h" // �÷��̾� ĳ���� ����
#include "Engine/OverlapResult.h" // FOverlapResult ����ü ���

AEnemyShooterGrenade::AEnemyShooterGrenade()
{
	PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ� ȣ�� Ȱ��ȭ

	// �浹 ������Ʈ ���� �� ����
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	RootComponent = CollisionComp; // ��Ʈ ������Ʈ�� ����
	CollisionComp->SetCollisionProfileName("Projectile"); // �⺻ �浹 �������� ����
	CollisionComp->InitSphereRadius(15.0f); // �浹 �ݰ� ����

	// ��(WorldStatic)�� �浹���� �� ��������(Block) �����Ͽ� ���� Ƣ�� �� �ֵ��� ��
	CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	// ���� ������Ʈ(PhysicsBody)�͵� �浹�Ͽ� ��ȣ�ۿ��ϵ��� ����
	CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Block);

	// �޽� ������Ʈ ���� �� ����
	GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	GrenadeMesh->SetupAttachment(RootComponent); // �浹 ������Ʈ�� ����
	GrenadeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �޽� ��ü�� �浹�� ��� ����

	// ����ü �̵� ������Ʈ ���� �� ����
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = RootComponent; // �� ������Ʈ�� ������ ����� ��Ʈ ������Ʈ�� ����
	ProjectileMovement->InitialSpeed = 0.f; // �ʱ� �ӵ��� LaunchGrenade���� ����
	ProjectileMovement->MaxSpeed = 3000.f; // �ִ� �ӵ� ����
	ProjectileMovement->bRotationFollowsVelocity = true; // �̵� ���⿡ ���� ���Ͱ� �ڵ����� ȸ���ϵ��� ����
	ProjectileMovement->bShouldBounce = true; // ���̳� �ٴڿ� ����� �� ƨ�⵵�� ����
	ProjectileMovement->ProjectileGravityScale = 1.0f; // �߷� ���� (������ �)
	ProjectileMovement->Bounciness = 0.3f; // ź�� (0~1, 1�� �������� ���� Ʀ)
	ProjectileMovement->Friction = 0.5f; // ������ (�ٴڿ� ���� ���� ����)
}

void AEnemyShooterGrenade::BeginPlay()
{
	Super::BeginPlay(); // �θ� Ŭ���� BeginPlay ȣ��

	// FuseTime(3��) �Ŀ� Explode �Լ��� ȣ���ϵ��� Ÿ�̸� ����
	FTimerHandle FuseTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(FuseTimerHandle, this, &AEnemyShooterGrenade::Explode, FuseTime, false);
}

void AEnemyShooterGrenade::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // �θ� Ŭ���� Tick ȣ��

	// �� �����Ӹ��� ����ź �޽��� Yaw�� �������� ȸ������ �ð��� ȿ���� ��
	GrenadeMesh->AddLocalRotation(FRotator(0.f, RotationSpeed * DeltaTime, 0.f));
}

void AEnemyShooterGrenade::LaunchGrenade(FVector LaunchVelocity)
{
	// ���� �߻� �ӵ��� ����ü �̵� ������Ʈ�� ����
	ProjectileMovement->Velocity = LaunchVelocity;
	ProjectileMovement->Activate(); // �̵� ������Ʈ�� Ȱ��ȭ�Ͽ� ���� �ùķ��̼� ����
}

void AEnemyShooterGrenade::Explode()
{
	if (bHasExploded) return; // �ߺ� ���� ����
	bHasExploded = true; // ���� ���·� ��ȯ

	// ���� �ݰ� �� �÷��̾�Ը� ���� ����
	FVector ExplosionCenter = GetActorLocation(); // ���� �߽���
	TArray<FOverlapResult> Overlaps;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ExplosionRadius); // ���� �ݰ游ŭ�� ��ü ����

	// Pawn ä�ο� ���ؼ��� ������(����) �˻� ����
	bool bHasOverlaps = GetWorld()->OverlapMultiByChannel(
		Overlaps, ExplosionCenter, FQuat::Identity, ECC_Pawn, CollisionShape
	);

	if (bHasOverlaps) // ���� ���� Pawn�� �����Ǿ��ٸ�
	{
		ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		for (auto& Overlap : Overlaps) // ��� ������ ���Ϳ� ����
		{
			if (Overlap.GetActor() == PlayerCharacter) // ���� �÷��̾���
			{
				// ������ ����
				UGameplayStatics::ApplyDamage(
					PlayerCharacter, ExplosionDamage,
					GetInstigator() ? GetInstigator()->GetController() : nullptr, this, nullptr
				);
				break; // �÷��̾�� �� ���̹Ƿ� ã���� �ٷ� �ߴ�
			}
		}
	}

	DrawDebugSphere(GetWorld(), ExplosionCenter, ExplosionRadius, 32, FColor::Red, false, 1.0f, 0, 2.0f); // ����׿� ���� ���� �ð�ȭ

	// ���� ����Ʈ ����
	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), ExplosionEffect, GetActorLocation(), GetActorRotation(), FVector(1.0f), true, true
		);
	}

	// ���� ���� ���
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}

	// ���� �� ����
	GrenadeMesh->SetVisibility(false); // �޽��� ��� ������ �ʰ� ����
	ProjectileMovement->StopMovementImmediately(); // ������ �������� ����
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �浹 ��Ȱ��ȭ
	SetActorTickEnabled(false); // ƽ ��Ȱ��ȭ
	Destroy(); // ���͸� ���忡�� ����
}