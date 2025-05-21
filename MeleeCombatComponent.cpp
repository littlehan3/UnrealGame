#include "MeleeCombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Knife.h"
#include "Enemy.h"
#include "MainCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UMeleeCombatComponent::UMeleeCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMeleeCombatComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UMeleeCombatComponent::InitializeCombatComponent(ACharacter* InOwnerCharacter, UBoxComponent* InKickHitBox, AKnife* InLeftKnife, AKnife* InRightKnife)
{
    OwnerCharacter = InOwnerCharacter;
    LeftKnife = InLeftKnife;
    RightKnife = InRightKnife;

    if (InKickHitBox)
    {
        KickHitBox = InKickHitBox;
        KickHitBox->OnComponentBeginOverlap.AddDynamic(this, &UMeleeCombatComponent::HandleKickOverlap);
    }
}

void UMeleeCombatComponent::SetComboMontages(const TArray<UAnimMontage*>& InMontages)
{
    ComboMontages = InMontages;
}

void UMeleeCombatComponent::TriggerComboAttack()
{
    if (!OwnerCharacter) return;

    // ĳ������ ���� ���� Ȯ��
    bool bIsJumping = OwnerCharacter->GetCharacterMovement()->IsFalling();
    bool bIsInDoubleJump = false;

    // MainCharacter Ŭ�������� ���� ���� ���� ��������
    if (OwnerCharacter->IsA(AMainCharacter::StaticClass()))
    {
        AMainCharacter* MainChar = Cast<AMainCharacter>(OwnerCharacter);
        if (MainChar)
        {
            bIsInDoubleJump = MainChar->IsInDoubleJump();
        }
    }

    // ���� ���̶�� ���� ���� ����
    if (bIsJumping)
    {
        TriggerJumpAttack(bIsInDoubleJump);
        return;
    }

    // �̹� ���� ���̸� �Է¸� ť�� ����
    if (bIsAttacking)
    {
        bComboQueued = true;
        return;
    }

    UWorld* World = OwnerCharacter->GetWorld();
    if (!World) return;

    ResetComboTimer();
    bIsAttacking = true;
    bComboQueued = false; // ť �ʱ�ȭ

    AdjustAttackDirection();

    if (ComboIndex == 3)
    {
        EnableKickHitBox();
    }
    else
    {
        if (LeftKnife) LeftKnife->EnableHitBox(ComboIndex);
        if (RightKnife) RightKnife->EnableHitBox(ComboIndex);
    }

    PlayComboMontage(ComboIndex);
}


void UMeleeCombatComponent::PlayComboMontage(int32 Index)
{
    if (!OwnerCharacter || !ComboMontages.IsValidIndex(Index)) return;

    UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
    if (!AnimInstance) return;

    UAnimMontage* Montage = ComboMontages[Index];
    if (AnimInstance->Montage_Play(Montage, 1.0f) > 0.f)
    {
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &UMeleeCombatComponent::OnComboMontageEnded);
        AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
    }
}

void UMeleeCombatComponent::ResetCombo()
{
    bIsAttacking = false;
    ComboIndex = 0;

    if (OwnerCharacter)
    {
        UWorld* World = OwnerCharacter->GetWorld();
        if (World)
        {
            World->GetTimerManager().ClearTimer(ComboResetTimerHandle);
            UE_LOG(LogTemp, Warning, TEXT("Combo Reset"));
        }

        if (OwnerCharacter->GetCharacterMovement())
        {
            OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
        }
    }
}

void UMeleeCombatComponent::ResetComboTimer()
{
    if (OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("ResetComboTimer Started"));
        UE_LOG(LogTemp, Warning, TEXT("ComboResetTime: %f"), ComboResetTime);

        OwnerCharacter->GetWorldTimerManager().SetTimer(
            ComboResetTimerHandle,
            this,
            &UMeleeCombatComponent::ResetCombo,
            ComboResetTime,
            false
        );
    }
}

void UMeleeCombatComponent::OnComboMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    bIsAttacking = false;

    if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
    {
        OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
    }

    if (LeftKnife) LeftKnife->DisableHitBox();
    if (RightKnife) RightKnife->DisableHitBox();

    DisableKickHitBox();

    if (!LastAttackDirection.IsNearlyZero())
    {
        LastAttackDirection = OwnerCharacter->GetActorForwardVector();
    }

    // �޺��� �����Ǵ� ���ȸ� ���� �ε�����
    if (OwnerCharacter && OwnerCharacter->GetWorldTimerManager().IsTimerActive(ComboResetTimerHandle))
    {
        ComboIndex = (ComboIndex + 1) % ComboMontages.Num();
    }
    else
    {
        ComboIndex = 0; // �޺� �ð��� �������� ����
    }

    // ���� �޺��� ��ٸ��� �־��ٸ� �ٷ� ����
    if (bComboQueued)
    {
        bComboQueued = false;
        TriggerComboAttack(); // ���� �޺� ����
    }
}

void UMeleeCombatComponent::AdjustAttackDirection()
{
    if (!OwnerCharacter) return;

    float MaxAutoAimDistance = 300.0f;
    AActor* TargetEnemy = nullptr;
    float ClosestDistance = MaxAutoAimDistance;

    TArray<AActor*> FoundEnemies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), FoundEnemies);

    for (AActor* Enemy : FoundEnemies)
    {
        AEnemy* EnemyCharacter = Cast<AEnemy>(Enemy);
        if (!EnemyCharacter || EnemyCharacter->bIsDead || EnemyCharacter->bIsInAirStun) continue;

        float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Enemy->GetActorLocation());
        if (Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            TargetEnemy = Enemy;
        }
    }

    if (TargetEnemy)
    {
        FVector DirectionToEnemy = TargetEnemy->GetActorLocation() - OwnerCharacter->GetActorLocation();
        DirectionToEnemy.Z = 0.0f;
        DirectionToEnemy.Normalize();

        FRotator NewRot = FRotationMatrix::MakeFromX(DirectionToEnemy).Rotator();
        OwnerCharacter->SetActorRotation(NewRot);

        LastAttackDirection = DirectionToEnemy;
    }
}

void UMeleeCombatComponent::ApplyComboMovement(float MoveDistance, FVector MoveDirection)
{
    if (!OwnerCharacter || MoveDirection.IsNearlyZero()) return;

    MoveDirection.Z = 0;
    MoveDirection.Normalize();

    OwnerCharacter->LaunchCharacter(MoveDirection * MoveDistance, false, false);
}

void UMeleeCombatComponent::EnableKickHitBox()
{
    if (KickHitBox)
    {
        KickHitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }

    KickRaycastAttack();
}

void UMeleeCombatComponent::DisableKickHitBox()
{
    if (KickHitBox)
    {
        KickHitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    KickRaycastHitActor = nullptr;
}

void UMeleeCombatComponent::KickRaycastAttack()
{
    if (!OwnerCharacter) return;

    FVector StartLocation = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 20.0f;
    FVector EndLocation = StartLocation + OwnerCharacter->GetActorForwardVector() * 150.0f;

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerCharacter);

    bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);

    KickRaycastHitActor = bHit ? HitResult.GetActor() : nullptr;

    DrawDebugLine(GetWorld(), StartLocation, EndLocation, bHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 3.0f);
}

void UMeleeCombatComponent::HandleKickOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == OwnerCharacter || OtherComp == KickHitBox) return;

    if (OtherActor != KickRaycastHitActor) return;

    float KickDamage = 35.0f;
    UGameplayStatics::ApplyDamage(OtherActor, KickDamage, nullptr, OwnerCharacter, nullptr);
    DisableKickHitBox();
}

void UMeleeCombatComponent::SetJumpAttackMontages(UAnimMontage* InJumpAttackMontage, UAnimMontage* InDoubleJumpAttackMontage)
{
    JumpAttackMontage = InJumpAttackMontage;
    DoubleJumpAttackMontage = InDoubleJumpAttackMontage;
}

void UMeleeCombatComponent::TriggerJumpAttack(bool bIsDoubleJump)
{
    if (!OwnerCharacter) return;

    // �̹� ���� ���̸� ����
    if (bIsAttacking) return;

    bIsAttacking = true;

    // �� �������� ȸ��
    AdjustAttackDirection();

    // ű ��Ʈ�ڽ� Ȱ��ȭ
    EnableKickHitBox();

    // ���� ���¿� ���� ������ ��Ÿ�� ���
    UAnimMontage* MontageToPlay = bIsDoubleJump ? DoubleJumpAttackMontage : JumpAttackMontage;

    if (MontageToPlay)
    {
        UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            if (AnimInstance->Montage_Play(MontageToPlay, 1.0f) > 0.f)
            {
                FOnMontageEnded EndDelegate;
                EndDelegate.BindUObject(this, &UMeleeCombatComponent::OnComboMontageEnded);
                AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToPlay);
            }
        }
    }
}