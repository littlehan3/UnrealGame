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
    // �ǰ� ����Ʈ, ���� �� �ʿ�� �߰�
    return DamageAmount;
}

void AEnemyBase::Die()
{
    bIsDead = true;
    // ��� �ִϸ��̼�, ����� ó�� ��
}
