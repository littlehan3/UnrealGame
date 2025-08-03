#include "AnimNotify_EnemyEndAttack.h"
#include "Enemy.h"
#include "BossEnemy.h"

void UAnimNotify_EnemyEndAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (AEnemy* Enemy = Cast<AEnemy>(MeshComp->GetOwner()))
	{
		Enemy->EndAttack();
	}
	else if (ABossEnemy* Boss = Cast<ABossEnemy>(MeshComp->GetOwner()))
	{
		Boss->EndAttack();
	}
}