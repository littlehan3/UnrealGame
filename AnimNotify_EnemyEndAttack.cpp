#include "AnimNotify_EnemyEndAttack.h"
#include "Enemy.h"
#include "BossEnemy.h"
#include "EnemyDog.h"

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
	else if (AEnemyDog* Dog = Cast<AEnemyDog>(MeshComp->GetOwner()))
	{
		Dog->EndAttack();
	}
}