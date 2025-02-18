#include "Enemy.h"
#include "EnemyKatana.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"

AEnemy::AEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
	bCanBeLockedOn = true;
}

void AEnemy::BeginPlay()
{
    Super::BeginPlay();
    SetCanBeDamaged(true);

    // KatanaClass가 설정되어 있다면 Katana 스폰 및 부착
    if (KatanaClass)
    {
        EquippedKatana = GetWorld()->SpawnActor<AEnemyKatana>(KatanaClass);
        if (EquippedKatana)
        {
            USkeletalMeshComponent* MeshComp = GetMesh();
            if (MeshComp)
            {
                FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
                EquippedKatana->AttachToComponent(MeshComp, AttachmentRules, FName("EnemyKatanaSocket"));
            }
        }
    }
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead)  // 이미 죽은 상태면 데미지 무시
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy is already dead! Ignoring further damage."));
        return 0.0f;
    }

    float DamageApplied = FMath::Min(Health, DamageAmount);
    Health -= DamageApplied;

    UE_LOG(LogTemp, Warning, TEXT("Enemy took %f damage, Health remaining: %f"), DamageAmount, Health);

    if (HitSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
    }

    if (Health <= 0.0f)
    {
        Die();
    }

    return DamageApplied;
}

void AEnemy::Die()
{
    if (bIsDead) return;  // 중복 실행 방지

    bIsDead = true;  // 사망 상태 변경
    UE_LOG(LogTemp, Warning, TEXT("Enemy Died! Now in dead state."));

    if (DieSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DieSound, GetActorLocation());
    }
}

// 특정 조건을 만족해야 락온 가능
bool AEnemy::CanBeLockedOn() const
{
    // 특정 체력 이하일 때만 락온 등 확장  
    // if (CurrentHealth < 50.0f) return false;

    return bCanBeLockedOn; // 기본적으로 bCanBeLockedOn이 true인 경우 락온 가능
}
