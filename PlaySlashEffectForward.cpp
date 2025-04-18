#include "PlaySlashEffectForward.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GameFramework/Character.h"

void UAnimNotify_PlaySlashEffectForward::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp || !SlashEffect) return; // 메쉬, 이펙트가 있어야 동작
     
    ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner()); // 메쉬를 소유한 액터 가져옴
    if (!Character) return; 

    // 로컬 좌표 오프셋 적용
    const FVector SpawnLocation = MeshComp->GetComponentLocation()
        + MeshComp->GetForwardVector() * LocationOffset.X
        + MeshComp->GetRightVector() * LocationOffset.Y
        + MeshComp->GetUpVector() * LocationOffset.Z;

    const FRotator OriginalRotation = Character->GetActorRotation(); // 캐릭터 회전 정보 가져옴


    // 해당 나이아가라 이펙트 에셋의 회전 방향과 캐릭터가 바라보는 방향을 맞춰주기 위해 각도를 조정
    const FRotator RotatedYaw = OriginalRotation + FRotator(0.f, 180.f + 80.f, 0.f); // 180도 돌린 후 추가로 80도 회전

    FVector Direction = RotatedYaw.Vector(); // 회전된 방향을 벡터로

    // 회전 적용해서 나이아가라 생성
    UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        MeshComp->GetWorld(), // 월드 컨텍스트
        SlashEffect,          // 사용할 이펙트
        SpawnLocation,        // 위치
        RotatedYaw,           // 회전값 (방향조절)
        FVector(1.0f),        // 스케일
        true,                 // 자동해제
        true,                 // 자동활성화
        ENCPoolMethod::AutoRelease, // 오브젝트 풀링하여 재사용
        true                  // 성능최적화
    );

    if (NiagaraComponent)
    {
        // Niagara 파라미터로 방향값 넘김 (Niagara 내부에서 Orientation 적용)
        NiagaraComponent->SetVariableVec3(FName("User_ForwardVector"), Direction);
        NiagaraComponent->SetVariableVec3(FName("User_RightVector"), FVector::CrossProduct(Direction, FVector::UpVector));
    }
}
