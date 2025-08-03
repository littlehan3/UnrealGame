// EnemyBase.cpp
#include "EnemyBase.h"

AEnemyBase::AEnemyBase()
{
    PrimaryActorTick.bCanEverTick = true;
    Health = 100.0f;
    bIsDead = false;
}

void AEnemyBase::BeginPlay()
{
    Super::BeginPlay();
}

float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f;
    Health -= DamageAmount;
    if (Health <= 0.f)
    {
        Die();
    }
    // 피격 이펙트, 사운드 등 필요시 추가
    return DamageAmount;
}

void AEnemyBase::Die()
{
    bIsDead = true;
    // 사망 애니메이션, 사라짐 처리 등
}
