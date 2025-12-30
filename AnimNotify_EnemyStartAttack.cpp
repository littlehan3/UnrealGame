#include "AnimNotify_EnemyStartAttack.h"
#include "BossEnemy.h"
#include "Enemy.h"
#include "EnemyDog.h"
#include "EnemyGuardian.h"
#include "EnemyGuardianShield.h"

void UAnimNotify_EnemyStartAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (AEnemy* Enemy = Cast<AEnemy>(MeshComp->GetOwner()))
    {
        Enemy->StartAttack(AttackType);
    }
    else if (ABossEnemy* Boss = Cast<ABossEnemy>(MeshComp->GetOwner()))
    {
        Boss->StartAttack();
    }
    else if (AEnemyDog* Dog = Cast<AEnemyDog>(MeshComp->GetOwner()))
    {
        Dog->StartAttack();
    }
    else if (AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(MeshComp->GetOwner()))
    {
        Guardian->StartAttack();
    }
}