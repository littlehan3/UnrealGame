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

    // 캐릭터의 점프 상태 확인
    bool bIsJumping = OwnerCharacter->GetCharacterMovement()->IsFalling();
    bool bIsInDoubleJump = false;

    // MainCharacter 클래스에서 더블 점프 상태 가져오기
    if (OwnerCharacter->IsA(AMainCharacter::StaticClass()))
    {
        AMainCharacter* MainChar = Cast<AMainCharacter>(OwnerCharacter);
        if (MainChar)
        {
            bIsInDoubleJump = MainChar->IsInDoubleJump();
        }
    }

    // 점프 중이라면 점프 공격 실행
    if (bIsJumping)
    {
        TriggerJumpAttack(bIsInDoubleJump);
        return;
    }

    // 이미 공격 중이면 입력만 큐에 저장
    if (bIsAttacking)
    {
        bComboQueued = true;
        return;
    }

    UWorld* World = OwnerCharacter->GetWorld();
    if (!World) return;

    ResetComboTimer();
    bIsAttacking = true;
    bComboQueued = false; // 큐 초기화

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

    // 콤보가 유지되는 동안만 다음 인덱스로
    if (OwnerCharacter && OwnerCharacter->GetWorldTimerManager().IsTimerActive(ComboResetTimerHandle))
    {
        ComboIndex = (ComboIndex + 1) % ComboMontages.Num();
    }
    else
    {
        ComboIndex = 0; // 콤보 시간이 끝났으면 리셋
    }

    // 다음 콤보를 기다리고 있었다면 바로 실행
    if (bComboQueued)
    {
        bComboQueued = false;
        TriggerComboAttack(); // 다음 콤보 실행
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

    // 이미 공격 중이면 무시
    if (bIsAttacking) return;

    bIsAttacking = true;

    // 적 방향으로 회전
    AdjustAttackDirection();

    // 킥 히트박스 활성화
    EnableKickHitBox();

    // 점프 상태에 따라 적절한 몽타주 재생
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