#include "MeleeCombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Knife.h"
#include "Enemy.h"
#include "MainCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "EnemyGuardian.h"
#include "EnemyShooter.h"
#include "EnemyDog.h"
#include "BossEnemy.h"

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
    if (!OwnerCharacter || OwnerCharacter->GetCharacterMovement()->IsFalling())
        return; // ���� ���̸� �޺� ���� �Ұ�

    if (!bCanGroundAction) return; // ���� �׼� �Ұ��� �޺� ����

    if (!CanStartComboAttack()) // ���ο� üũ �Լ� ���
    {
        UE_LOG(LogTemp, Warning, TEXT("Combo Attack Blocked - Jump Attack Cooldown Active"));
        return;
    }

    // �Է��� ��ϵǾ� ������ ����
    if (bInputBlocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("Combo Attack Input Blocked - Cooldown Active"));
        return;
    }

    // �̹� ���� ���̸� �Է¸� ť�� ����
    if (bIsAttacking)
    {
        bComboQueued = true;
        UE_LOG(LogTemp, Warning, TEXT("Combo Attack Queued - Already Attacking"));
        return;
    }

    // �Է� ��ٿ� ����
    bInputBlocked = true;
    if (OwnerCharacter)
    {
        OwnerCharacter->GetWorldTimerManager().SetTimer(
            InputCooldownHandle,
            this,
            &UMeleeCombatComponent::ResetInputCooldown,
            InputCooldownTime,
            false
        );
    }


    UWorld* World = OwnerCharacter->GetWorld();
    if (!World) return;

    ResetComboTimer();
    bIsAttacking = true;
    bCanAirAction = false; // �޺� ���۽� ���� �׼� ����
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

    // ���� ��Ÿ�ְ� ��� ���̸� ���� ����
    if (AnimInstance->Montage_IsPlaying(Montage))
    {
        AnimInstance->Montage_Stop(0.1f, Montage);
    }

    float PlayResult = AnimInstance->Montage_Play(Montage, 1.0f);

    if (PlayResult > 0.f)
    {
        // �ݹ� ��� ���� ���� �ݹ� ����
        AnimInstance->OnMontageEnded.RemoveDynamic(this, &UMeleeCombatComponent::OnComboMontageEnded);

        // ���ο� �ݹ� ���
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &UMeleeCombatComponent::OnComboMontageEnded);
        AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
    }
}

void UMeleeCombatComponent::ResetCombo()
{
    bIsAttacking = false;
    ComboIndex = 0;
    bCanTeleport = true;

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
    UE_LOG(LogTemp, Warning, TEXT("OnComboMontageEnded called - Montage: %s"), Montage ? *Montage->GetName() : TEXT("NULL"));

    bIsAttacking = false;
    bCanAirAction = true; // �޺� �Ϸ� �� ���� �׼� ���
    bCanTeleport = true;

    if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
    {
        OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
    }

    if (LeftKnife) LeftKnife->DisableHitBox();
    if (RightKnife) RightKnife->DisableHitBox();
    DisableKickHitBox();

    // �ݹ� ����
    if (OwnerCharacter)
    {
        UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->OnMontageEnded.RemoveDynamic(this, &UMeleeCombatComponent::OnComboMontageEnded);
        }
    }

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
        bCanTeleport = true;
    }

    // ť ó�� �� ���� üũ ��ȭ
    if (OwnerCharacter && (bComboQueued || bJumpAttackQueued))
    {
        OwnerCharacter->GetWorldTimerManager().SetTimer(
            InputCooldownHandle,
            [this]()
            {
                // �޺����� ť ó�� - ���󿡼���
                if (bComboQueued && OwnerCharacter && !OwnerCharacter->GetCharacterMovement()->IsFalling())
                {
                    bComboQueued = false;
                    UE_LOG(LogTemp, Warning, TEXT("Processing queued combo attack"));
                    TriggerComboAttack();
                }
                // �������� ť ó�� - ���߿�����
                else if (bJumpAttackQueued && OwnerCharacter && OwnerCharacter->GetCharacterMovement()->IsFalling())
                {
                    bJumpAttackQueued = false;
                    UE_LOG(LogTemp, Warning, TEXT("Processing queued jump attack"));
                    TriggerJumpAttack(false);
                }
                // ���ǿ� ���� ������ ť Ŭ����
                else
                {
                    if (bComboQueued)
                    {
                        bComboQueued = false;
                        UE_LOG(LogTemp, Warning, TEXT("Combo attack queue cleared - not on ground"));
                    }
                    if (bJumpAttackQueued)
                    {
                        bJumpAttackQueued = false;
                        UE_LOG(LogTemp, Warning, TEXT("Jump attack queue cleared - not in air"));
                    }
                }
            },
            0.1f, // 100ms ������
            false
        );
    }
}

void UMeleeCombatComponent::AdjustAttackDirection()
{
    if (!OwnerCharacter) return;

    float MaxAutoAimDistance = 300.0f;
    AActor* TargetEnemy = nullptr;
    float ClosestDistance = MaxAutoAimDistance;

    // 1. ��� ������ ���� �ϳ��� �迭�� ����
    TArray<AActor*> AllEnemies;
    TArray<AActor*> FoundActors;

    // �� �� �������� ���͸� ã�� AllEnemies �迭�� �߰�
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), FoundActors);
    AllEnemies.Append(FoundActors);

    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDog::StaticClass(), FoundActors);
    AllEnemies.Append(FoundActors);

    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyShooter::StaticClass(), FoundActors);
    AllEnemies.Append(FoundActors);

    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyGuardian::StaticClass(), FoundActors);
    AllEnemies.Append(FoundActors);

    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABossEnemy::StaticClass(), FoundActors);
    AllEnemies.Append(FoundActors);

    FVector StartLocation = OwnerCharacter->GetActorLocation();

    // 2. ���յ� ��� ������ �˻��Ͽ� ���� ����� ��ȿ�� �� ã��
    for (AActor* Enemy : AllEnemies)
    {
        if (!Enemy) continue;

        // 3. �� �� ������ ���� ����(���, ���� ��)�� Ȯ���Ͽ� ��ȿ�� Ÿ������ �˻�
        bool bIsTargetValid = true;
        if (AEnemy* GenericEnemy = Cast<AEnemy>(Enemy))
        {
            if (GenericEnemy->bIsDead || GenericEnemy->bIsInAirStun)
            {
                bIsTargetValid = false;
            }
        }
        else if (AEnemyDog* Dog = Cast<AEnemyDog>(Enemy))
        {
            if (Dog->bIsDead || Dog->bIsInAirStun)
            {
                bIsTargetValid = false;
            }
        }
        else if (AEnemyShooter* Shooter = Cast<AEnemyShooter>(Enemy))
        {
            if (Shooter->bIsDead || Shooter->bIsInAirStun)
            {
                bIsTargetValid = false;
            }
        }
        else if (AEnemyGuardian* Guardian = Cast<AEnemyGuardian>(Enemy))
        {
            if (Guardian->bIsDead)
            {
                bIsTargetValid = false;
            }
        }
        else if (ABossEnemy* Boss = Cast<ABossEnemy>(Enemy))
        {
             if (Boss->bIsBossDead)
             {
                 bIsTargetValid = false;
             }
        }

        if (!bIsTargetValid) continue; // ��ȿ���� ���� Ÿ���̸� �ǳʶ�

        // 4. �Ÿ� �� ��ֹ� �˻�
        FVector EnemyLocation = Enemy->GetActorLocation();
        float Distance = FVector::Dist(StartLocation, EnemyLocation);

        if (Distance < ClosestDistance)
        {
            // ����ĳ��Ʈ�� ������ ��ֹ��� �ִ��� üũ
            FHitResult HitResult;
            FCollisionQueryParams Params;
            Params.AddIgnoredActor(OwnerCharacter);
            Params.AddIgnoredActor(Enemy);

            FVector EndLocation = EnemyLocation;
            EndLocation.Z = StartLocation.Z; // ���� ���̷� ����

            bool bHit = GetWorld()->LineTraceSingleByChannel(
                HitResult, StartLocation, EndLocation, ECC_Visibility, Params
            );

            // ��ֹ��� ���ų� ��ֹ��� ������ �ָ� ������ Ÿ������ ����
            if (!bHit || FVector::Dist(StartLocation, HitResult.Location) > Distance * 0.9f)
            {
                ClosestDistance = Distance;
                TargetEnemy = Enemy;
            }
        }
    }

    // 5. ���� Ÿ���� �������� �ڷ���Ʈ �Ǵ� ���� ���� ����
    if (TargetEnemy)
    {
        float DistanceToEnemy = FVector::Dist(OwnerCharacter->GetActorLocation(), TargetEnemy->GetActorLocation());

        if (DistanceToEnemy < MinTeleportDistance)
        {
            // ���� ������ ����
            FVector DirectionToEnemy = (TargetEnemy->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
            DirectionToEnemy.Z = 0.0f;
            FRotator NewRot = DirectionToEnemy.Rotation();
            OwnerCharacter->SetActorRotation(NewRot);
            LastAttackDirection = DirectionToEnemy;
        }
        else
        {
            if (ShouldTeleportToTarget(DistanceToEnemy))
            {
                TeleportToTarget(TargetEnemy);
            }
            else
            {
                // �ڷ���Ʈ ������ �� ������ ���� ���� ������ ����
                FVector DirectionToEnemy = (TargetEnemy->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
                DirectionToEnemy.Z = 0.0f;
                FRotator NewRot = DirectionToEnemy.Rotation();
                OwnerCharacter->SetActorRotation(NewRot);
                LastAttackDirection = DirectionToEnemy;
            }
        }
    }
}
bool UMeleeCombatComponent::ShouldTeleportToTarget(float DistanceToTarget)
{
    // �����̵� ���ǵ� üũ
    if (!OwnerCharacter || !bCanTeleport) return false;

    // ���߿� ������ �����̵� ����
    if (OwnerCharacter->GetCharacterMovement()->IsFalling()) return false;

    // �ּ� �ڷ���Ʈ �Ÿ����� ������ �����̵� ����
    if (DistanceToTarget < MinTeleportDistance) return false;

    // �ִ� �ڷ���Ʈ �Ÿ����� �ָ� �����̵� ����
    if (DistanceToTarget > TeleportDistance) return false;

    return true;
}


void UMeleeCombatComponent::TeleportToTarget(AActor* TargetEnemy)
{
    if (!TargetEnemy || !OwnerCharacter) return;

    // ���� ��ġ�� ���� ���
    FVector EnemyLocation = TargetEnemy->GetActorLocation();
    FVector EnemyForward = TargetEnemy->GetActorForwardVector();

    // ���� �������� �����̵� ��ġ ���
    FVector TeleportLocation = EnemyLocation + (EnemyForward * TeleportOffset);
    TeleportLocation.Z = OwnerCharacter->GetActorLocation().Z; // ���̴� ����

    // �ٴڰ��� �浹 üũ�ؼ� ���� ���� (���û���)
    FHitResult FloorHit;
    FCollisionQueryParams FloorParams;
    FloorParams.AddIgnoredActor(OwnerCharacter);
    FloorParams.AddIgnoredActor(TargetEnemy);

    FVector FloorStart = TeleportLocation + FVector(0, 0, 100);
    FVector FloorEnd = TeleportLocation - FVector(0, 0, 500);

    if (GetWorld()->LineTraceSingleByChannel(FloorHit, FloorStart, FloorEnd, ECC_Visibility, FloorParams))
    {
        TeleportLocation.Z = FloorHit.Location.Z + 90.0f; // ĳ���� �� ��ġ ����
    }

    // �����̵� ����
    OwnerCharacter->SetActorLocation(TeleportLocation);

    // ���� ���� ȸ��
    FVector DirectionToEnemy = EnemyLocation - TeleportLocation;
    DirectionToEnemy.Z = 0.0f;
    DirectionToEnemy.Normalize();

    FRotator NewRot = FRotationMatrix::MakeFromX(DirectionToEnemy).Rotator();
    OwnerCharacter->SetActorRotation(NewRot);

    LastAttackDirection = DirectionToEnemy;

    // �ð��� ȿ����
    DrawDebugSphere(GetWorld(), TeleportLocation, 50.0f, 12, FColor::Green, false, 2.0f, 0, 2.0f);
    DrawDebugLine(GetWorld(), OwnerCharacter->GetActorLocation(), EnemyLocation, FColor::Yellow, false, 2.0f, 0, 5.0f);

    UE_LOG(LogTemp, Warning, TEXT("Teleported to target enemy - Combo Index: %d"), ComboIndex);
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
    // MainCharacter ������ ���� ���Ӹ�� ���� Ȯ��
    if (AMainCharacter* MainChar = Cast<AMainCharacter>(OwnerCharacter))
    {
        if (MainChar->IsAiming())
        {
            UE_LOG(LogTemp, Warning, TEXT("Jump Attack Blocked in MeleeCombatComponent - Aim Mode Active"));
            return;
        }
    }

    if (!OwnerCharacter || !OwnerCharacter->GetCharacterMovement()->IsFalling())
        return; // ���󿡼��� ���� ���� �Ұ�

    if (!bCanAirAction) return;

    // �Է��� ��ϵǾ� ������ ����
    if (bInputBlocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("Jump Attack Input Blocked - Cooldown Active"));
        return;
    }


    if (bIsAttacking)
    {
        bJumpAttackQueued = true;
        UE_LOG(LogTemp, Warning, TEXT("Jump Attack Queued - Already Attacking"));
        return;
    }

    // �Է� ��ٿ� ����
    bInputBlocked = true;
    if (OwnerCharacter)
    {
        OwnerCharacter->GetWorldTimerManager().SetTimer(
            InputCooldownHandle,
            this,
            &UMeleeCombatComponent::ResetInputCooldown,
            InputCooldownTime,
            false
        );
    }

    bIsAttacking = true;
    bIsJumpAttacked = true; // �������� ���� ǥ��
    bCanGroundAction = false; // �������� ���� �� ����׼� ����

    // �� �������� ȸ��
    AdjustAttackDirection();

    // ���� ���¿� ���� ������ ��Ÿ�� ���
    UAnimMontage* MontageToPlay = bIsDoubleJump ? DoubleJumpAttackMontage : JumpAttackMontage;

    if (MontageToPlay)
    {
        UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            // ���� ��Ÿ�ְ� ��� ���̸� ����
            if (AnimInstance->Montage_IsPlaying(MontageToPlay))
            {
                UE_LOG(LogTemp, Warning, TEXT("Same Montage Already Playing - Ignoring"));
                bIsAttacking = false;
                bCanGroundAction = true;
                return;
            }

            if (AnimInstance->Montage_IsPlaying(MontageToPlay)) // ���� ��Ÿ�ְ� ��� ���̸�
            {
                AnimInstance->Montage_Stop(0.1f, MontageToPlay); // ����
            }

            float PlayResult = AnimInstance->Montage_Play(MontageToPlay, 1.0f); // ���ο� ��Ÿ�� ���

            if (PlayResult > 0.f)
            {
                // �ݹ� ��� ���� ���� �ݹ� ����
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &UMeleeCombatComponent::OnJumpAttackMontageEnded);

                // ���ο� �ݹ� ���
                FOnMontageEnded EndDelegate;
                EndDelegate.BindUObject(this, &UMeleeCombatComponent::OnJumpAttackMontageEnded);
                AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToPlay);

                UE_LOG(LogTemp, Warning, TEXT("Jump Attack Started: %s"), *MontageToPlay->GetName());
            }
            else
            {
                // ��Ÿ�� ��� ���� �� ���� ����
                bIsAttacking = false;
                bCanGroundAction = true;
                UE_LOG(LogTemp, Error, TEXT("Failed to play jump attack montage"));
            }
        }
    }
    // ű ��Ʈ�ڽ� Ȱ��ȭ
    EnableKickHitBox();
}

void UMeleeCombatComponent::OnJumpAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Warning, TEXT("OnJumpAttackMontageEnded called - Montage: %s, Interrupted: %s"),
        Montage ? *Montage->GetName() : TEXT("NULL"),
        bInterrupted ? TEXT("True") : TEXT("False"));

    // ���� ���� ��Ÿ������ Ȯ��
    if (Montage == JumpAttackMontage || Montage == DoubleJumpAttackMontage)
    {
        // ���� ���� ����
        bIsAttacking = false;
        bCanGroundAction = true; // ���� ���� �Ϸ� �� ���� �׼� ���

        // ��Ʈ�ڽ� ��Ȱ��ȭ
        if (LeftKnife) LeftKnife->DisableHitBox();
        if (RightKnife) RightKnife->DisableHitBox();
        DisableKickHitBox();

        // ĳ���� ȸ�� ����
        if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
        {
            OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
        }

        // �ݹ� ����
        if (OwnerCharacter)
        {
            UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
            if (AnimInstance)
            {
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &UMeleeCombatComponent::OnJumpAttackMontageEnded);
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("Jump Attack Completed"));

        // �������� ��ٿ� ����
        bJumpAttackCooldownActive = true;
        if (OwnerCharacter)
        {
            OwnerCharacter->GetWorldTimerManager().SetTimer(
                JumpAttackCooldownHandle,
                [this]() { bJumpAttackCooldownActive = false; },
                JumpAttackCooldownTime,
                false
            );
        }

        UE_LOG(LogTemp, Warning, TEXT("Jump Attack Cooldown"));

        // ť ó�� �� ���� üũ ��ȭ
        if (OwnerCharacter && (bJumpAttackQueued || bComboQueued))
        {
            OwnerCharacter->GetWorldTimerManager().SetTimer(
                InputCooldownHandle,
                [this]()
                {
                    // �������� ť ó�� - ���߿�����
                    if (bJumpAttackQueued && OwnerCharacter && OwnerCharacter->GetCharacterMovement()->IsFalling() && !bJumpAttackCooldownActive)
                    {
                        bJumpAttackQueued = false;
                        UE_LOG(LogTemp, Warning, TEXT("Processing queued jump attack"));
                        TriggerJumpAttack(false);
                    }
                    // �޺����� ť ó�� - ���󿡼���
                    else if (bComboQueued && OwnerCharacter && !OwnerCharacter->GetCharacterMovement()->IsFalling())
                    {
                        bComboQueued = false;
                        UE_LOG(LogTemp, Warning, TEXT("Processing queued combo attack"));
                        TriggerComboAttack();
                    }
                    // ���ǿ� ���� ������ ť Ŭ����
                    else
                    {
                        if (bJumpAttackQueued)
                        {
                            bJumpAttackQueued = false;
                            UE_LOG(LogTemp, Warning, TEXT("Jump attack queue cleared - not in air"));
                        }
                        if (bComboQueued)
                        {
                            bComboQueued = false;
                            UE_LOG(LogTemp, Warning, TEXT("Combo attack queue cleared - not on ground"));
                        }
                    }
                },
                FMath::Max(0.2f, JumpAttackCooldownTime),
                false
            );
        }
    }
}

void UMeleeCombatComponent::OnCharacterLanded()
{
    bIsJumpAttacked = false; // ���� �� �������� ���� ���� �ʱ�ȭ

    UE_LOG(LogTemp, Warning, TEXT("Character Landed - Jump Attack State Reset"));
}

void UMeleeCombatComponent::ResetInputCooldown()
{
    bInputBlocked = false;
    UE_LOG(LogTemp, Warning, TEXT("Input Cooldown Reset"));
}

void UMeleeCombatComponent::ClearAllQueues()
{
    bComboQueued = false;
    bJumpAttackQueued = false;
    UE_LOG(LogTemp, Warning, TEXT("All attack queues cleared"));
}

void UMeleeCombatComponent::ClearComboAttackQueue()
{
    bComboQueued = false;
    UE_LOG(LogTemp, Warning, TEXT("Combo attack queue cleared"));
}

void UMeleeCombatComponent::ClearJumpAttackQueue()
{
    bJumpAttackQueued = false;
    UE_LOG(LogTemp, Warning, TEXT("Jump attack queue cleared"));
}