#include "AnimNotify_EnemyEndAttack.h"
#include "Enemy.h"

void UAnimNotify_EnemyEndAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (AEnemy* Enemy = Cast<AEnemy>(MeshComp->GetOwner()))
		Enemy->EndAttack();
}