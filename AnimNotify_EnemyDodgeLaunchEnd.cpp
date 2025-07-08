#include "AnimNotify_EnemyDodgeLaunchEnd.h"
#include "Enemy.h"

void UAnimNotify_EnemyDodgeLaunchEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;
    AEnemy* Enemy = Cast<AEnemy>(MeshComp->GetOwner());
    if (Enemy)
        Enemy->OnDodgeLaunchEndNotify();
}