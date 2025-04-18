#include "PlaySlashEffectForward.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GameFramework/Character.h"

void UAnimNotify_PlaySlashEffectForward::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp || !SlashEffect) return; // �޽�, ����Ʈ�� �־�� ����
     
    ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner()); // �޽��� ������ ���� ������
    if (!Character) return; 

    // ���� ��ǥ ������ ����
    const FVector SpawnLocation = MeshComp->GetComponentLocation()
        + MeshComp->GetForwardVector() * LocationOffset.X
        + MeshComp->GetRightVector() * LocationOffset.Y
        + MeshComp->GetUpVector() * LocationOffset.Z;

    const FRotator OriginalRotation = Character->GetActorRotation(); // ĳ���� ȸ�� ���� ������


    // �ش� ���̾ư��� ����Ʈ ������ ȸ�� ����� ĳ���Ͱ� �ٶ󺸴� ������ �����ֱ� ���� ������ ����
    const FRotator RotatedYaw = OriginalRotation + FRotator(0.f, 180.f + 80.f, 0.f); // 180�� ���� �� �߰��� 80�� ȸ��

    FVector Direction = RotatedYaw.Vector(); // ȸ���� ������ ���ͷ�

    // ȸ�� �����ؼ� ���̾ư��� ����
    UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        MeshComp->GetWorld(), // ���� ���ؽ�Ʈ
        SlashEffect,          // ����� ����Ʈ
        SpawnLocation,        // ��ġ
        RotatedYaw,           // ȸ���� (��������)
        FVector(1.0f),        // ������
        true,                 // �ڵ�����
        true,                 // �ڵ�Ȱ��ȭ
        ENCPoolMethod::AutoRelease, // ������Ʈ Ǯ���Ͽ� ����
        true                  // ��������ȭ
    );

    if (NiagaraComponent)
    {
        // Niagara �Ķ���ͷ� ���Ⱚ �ѱ� (Niagara ���ο��� Orientation ����)
        NiagaraComponent->SetVariableVec3(FName("User_ForwardVector"), Direction);
        NiagaraComponent->SetVariableVec3(FName("User_RightVector"), FVector::CrossProduct(Direction, FVector::UpVector));
    }
}
