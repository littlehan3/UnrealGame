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
    if (!OwnerCharacter || OwnerCharacter->GetCharacterMovement()->IsFalling())
        return; // 점프 중이면 콤보 공격 불가

    if (!bCanGroundAction) return; // 지상 액션 불가시 콤보 차단

    if (!CanStartComboAttack()) // 새로운 체크 함수 사용
    {
        UE_LOG(LogTemp, Warning, TEXT("Combo Attack Blocked - Jump Attack Cooldown Active"));
        return;
    }

    // 입력이 블록되어 있으면 무시
    if (bInputBlocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("Combo Attack Input Blocked - Cooldown Active"));
        return;
    }

    // 이미 공격 중이면 입력만 큐에 저장
    if (bIsAttacking)
    {
        bComboQueued = true;
        UE_LOG(LogTemp, Warning, TEXT("Combo Attack Queued - Already Attacking"));
        return;
    }

    // 입력 쿨다운 시작
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
    bCanAirAction = false; // 콤보 시작시 공준 액션 금지
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

    // 기존 몽타주가 재생 중이면 먼저 정지
    if (AnimInstance->Montage_IsPlaying(Montage))
    {
        AnimInstance->Montage_Stop(0.1f, Montage);
    }

    float PlayResult = AnimInstance->Montage_Play(Montage, 1.0f);

    if (PlayResult > 0.f)
    {
        // 콜백 등록 전에 기존 콜백 제거
        AnimInstance->OnMontageEnded.RemoveDynamic(this, &UMeleeCombatComponent::OnComboMontageEnded);

        // 새로운 콜백 등록
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
    UE_LOG(LogTemp, Warning, TEXT("OnComboMontageEnded called - Montage: %s"), Montage ? *Montage->GetName() : TEXT("NULL"));

    bIsAttacking = false;
    bCanAirAction = true; // 콤보 완료 시 공중 액션 허용

    if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
    {
        OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
    }

    if (LeftKnife) LeftKnife->DisableHitBox();
    if (RightKnife) RightKnife->DisableHitBox();
    DisableKickHitBox();

    // 콜백 제거
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

    // 콤보가 유지되는 동안만 다음 인덱스로
    if (OwnerCharacter && OwnerCharacter->GetWorldTimerManager().IsTimerActive(ComboResetTimerHandle))
    {
        ComboIndex = (ComboIndex + 1) % ComboMontages.Num();
    }
    else
    {
        ComboIndex = 0; // 콤보 시간이 끝났으면 리셋
    }

    // 큐 처리 시 상태 체크 강화
    if (OwnerCharacter && (bComboQueued || bJumpAttackQueued))
    {
        OwnerCharacter->GetWorldTimerManager().SetTimer(
            InputCooldownHandle,
            [this]()
            {
                // 콤보공격 큐 처리 - 지상에서만
                if (bComboQueued && OwnerCharacter && !OwnerCharacter->GetCharacterMovement()->IsFalling())
                {
                    bComboQueued = false;
                    UE_LOG(LogTemp, Warning, TEXT("Processing queued combo attack"));
                    TriggerComboAttack();
                }
                // 점프공격 큐 처리 - 공중에서만
                else if (bJumpAttackQueued && OwnerCharacter && OwnerCharacter->GetCharacterMovement()->IsFalling())
                {
                    bJumpAttackQueued = false;
                    UE_LOG(LogTemp, Warning, TEXT("Processing queued jump attack"));
                    TriggerJumpAttack(false);
                }
                // 조건에 맞지 않으면 큐 클리어
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
            0.1f, // 100ms 딜레이
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
    if (!OwnerCharacter || !OwnerCharacter->GetCharacterMovement()->IsFalling())
        return; // 지상에서는 점프 공격 불가

    if (!bCanAirAction) return;

    // 입력이 블록되어 있으면 무시
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

    // 입력 쿨다운 시작
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
    bIsJumpAttacked = true; // 점프공격 실행 표시
    bCanGroundAction = false; // 점프공격 시작 시 지상액션 금지

    // 적 방향으로 회전
    AdjustAttackDirection();

    // 점프 상태에 따라 적절한 몽타주 재생
    UAnimMontage* MontageToPlay = bIsDoubleJump ? DoubleJumpAttackMontage : JumpAttackMontage;

    if (MontageToPlay)
    {
        UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            // 같은 몽타주가 재생 중이면 무시
            if (AnimInstance->Montage_IsPlaying(MontageToPlay))
            {
                UE_LOG(LogTemp, Warning, TEXT("Same Montage Already Playing - Ignoring"));
                bIsAttacking = false;
                bCanGroundAction = true;
                return;
            }

            if (AnimInstance->Montage_IsPlaying(MontageToPlay)) // 기존 몽타주가 재생 중이면
            {
                AnimInstance->Montage_Stop(0.1f, MontageToPlay); // 정지
            }

            float PlayResult = AnimInstance->Montage_Play(MontageToPlay, 1.0f); // 새로운 몽타주 재생

            if (PlayResult > 0.f)
            {
                // 콜백 등록 전에 기존 콜백 제거
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &UMeleeCombatComponent::OnJumpAttackMontageEnded);

                // 새로운 콜백 등록
                FOnMontageEnded EndDelegate;
                EndDelegate.BindUObject(this, &UMeleeCombatComponent::OnJumpAttackMontageEnded);
                AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToPlay);

                UE_LOG(LogTemp, Warning, TEXT("Jump Attack Started: %s"), *MontageToPlay->GetName());
            }
            else
            {
                // 몽타주 재생 실패 시 상태 복원
                bIsAttacking = false;
                bCanGroundAction = true;
                UE_LOG(LogTemp, Error, TEXT("Failed to play jump attack montage"));
            }
        }
    }
    // 킥 히트박스 활성화
    EnableKickHitBox();
}

void UMeleeCombatComponent::OnJumpAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Warning, TEXT("OnJumpAttackMontageEnded called - Montage: %s, Interrupted: %s"),
        Montage ? *Montage->GetName() : TEXT("NULL"),
        bInterrupted ? TEXT("True") : TEXT("False"));

    // 점프 공격 몽타주인지 확인
    if (Montage == JumpAttackMontage || Montage == DoubleJumpAttackMontage)
    {
        // 공격 상태 해제
        bIsAttacking = false;
        bCanGroundAction = true; // 점프 공격 완료 시 지상 액션 허용

        // 히트박스 비활성화
        if (LeftKnife) LeftKnife->DisableHitBox();
        if (RightKnife) RightKnife->DisableHitBox();
        DisableKickHitBox();

        // 캐릭터 회전 복원
        if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
        {
            OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
        }

        // 콜백 제거
        if (OwnerCharacter)
        {
            UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
            if (AnimInstance)
            {
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &UMeleeCombatComponent::OnJumpAttackMontageEnded);
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("Jump Attack Completed"));

        // 점프공격 쿨다운 시작
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

        // 큐 처리 시 상태 체크 강화
        if (OwnerCharacter && (bJumpAttackQueued || bComboQueued))
        {
            OwnerCharacter->GetWorldTimerManager().SetTimer(
                InputCooldownHandle,
                [this]()
                {
                    // 점프공격 큐 처리 - 공중에서만
                    if (bJumpAttackQueued && OwnerCharacter && OwnerCharacter->GetCharacterMovement()->IsFalling() && !bJumpAttackCooldownActive)
                    {
                        bJumpAttackQueued = false;
                        UE_LOG(LogTemp, Warning, TEXT("Processing queued jump attack"));
                        TriggerJumpAttack(false);
                    }
                    // 콤보공격 큐 처리 - 지상에서만
                    else if (bComboQueued && OwnerCharacter && !OwnerCharacter->GetCharacterMovement()->IsFalling())
                    {
                        bComboQueued = false;
                        UE_LOG(LogTemp, Warning, TEXT("Processing queued combo attack"));
                        TriggerComboAttack();
                    }
                    // 조건에 맞지 않으면 큐 클리어
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
    bIsJumpAttacked = false; // 착지 시 점프공격 실행 상태 초기화

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