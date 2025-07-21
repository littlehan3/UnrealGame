#include "AnimNotify_BossFireProjectile.h"
#include "BossEnemy.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_BossFireProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;

    // 이 노티파이를 재생한 캐릭터가 보스인지 확인
    if (ABossEnemy* Boss = Cast<ABossEnemy>(MeshComp->GetOwner()))
    {
        Boss->SpawnBossProjectile();   // 이미 있는 함수 호출[2]
    }
}
