#include "AnimNotify_BossFireProjectile.h"
#include "BossEnemy.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_BossFireProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;

    if (ABossEnemy* Boss = Cast<ABossEnemy>(MeshComp->GetOwner()))
    {
        Boss->SpawnBossProjectile(); 
    }
}
