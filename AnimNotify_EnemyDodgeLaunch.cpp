#include "AnimNotify_EnemyDodgeLaunch.h"
#include "Enemy.h"

void UAnimNotify_EnemyDodgeLaunch::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;
	AEnemy* Enemy = Cast<AEnemy>(MeshComp->GetOwner());
	if (Enemy)
	{
		Enemy->OnDodgeLaunchNotify(bDodgeLeft);
	}
}
