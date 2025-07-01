#include "AnimNotify_EnemyStartAttack.h"
#include "Enemy.h"

void UAnimNotify_EnemyStartAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (AEnemy* Enemy = Cast<AEnemy>(MeshComp->GetOwner()))
    {
        Enemy->StartAttack(AttackType);
    }
}