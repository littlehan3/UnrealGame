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
    bCanAirAction = true; // 콤보 완료 시 공중 액션 허용
    bCanTeleport = true;

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
        bCanTeleport = true;
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

    // 1. 모든 종류의 적을 하나의 배열로 통합
    TArray<AActor*> AllEnemies;
    TArray<AActor*> FoundActors;

    // 각 적 유형별로 액터를 찾아 AllEnemies 배열에 추가
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

    // 2. 통합된 모든 적들을 검사하여 가장 가까운 유효한 적 찾기
    for (AActor* Enemy : AllEnemies)
    {
        if (!Enemy) continue;

        // 3. 각 적 유형에 맞춰 상태(사망, 스턴 등)를 확인하여 유효한 타겟인지 검사
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

        if (!bIsTargetValid) continue; // 유효하지 않은 타겟이면 건너뜀

        // 4. 거리 및 장애물 검사
        FVector EnemyLocation = Enemy->GetActorLocation();
        float Distance = FVector::Dist(StartLocation, EnemyLocation);

        if (Distance < ClosestDistance)
        {
            // 레이캐스트로 직선상에 장애물이 있는지 체크
            FHitResult HitResult;
            FCollisionQueryParams Params;
            Params.AddIgnoredActor(OwnerCharacter);
            Params.AddIgnoredActor(Enemy);

            FVector EndLocation = EnemyLocation;
            EndLocation.Z = StartLocation.Z; // 같은 높이로 맞춤

            bool bHit = GetWorld()->LineTraceSingleByChannel(
                HitResult, StartLocation, EndLocation, ECC_Visibility, Params
            );

            // 장애물이 없거나 장애물이 적보다 멀리 있으면 타겟으로 설정
            if (!bHit || FVector::Dist(StartLocation, HitResult.Location) > Distance * 0.9f)
            {
                ClosestDistance = Distance;
                TargetEnemy = Enemy;
            }
        }
    }

    // 5. 최종 타겟이 정해지면 텔레포트 또는 방향 보정 실행
    if (TargetEnemy)
    {
        float DistanceToEnemy = FVector::Dist(OwnerCharacter->GetActorLocation(), TargetEnemy->GetActorLocation());

        if (DistanceToEnemy < MinTeleportDistance)
        {
            // 방향 보정만 수행
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
                // 텔레포트 조건이 안 맞으면 기존 방향 보정만 수행
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
    // 순간이동 조건들 체크
    if (!OwnerCharacter || !bCanTeleport) return false;

    // 공중에 있으면 순간이동 안함
    if (OwnerCharacter->GetCharacterMovement()->IsFalling()) return false;

    // 최소 텔레포트 거리보다 가까우면 순간이동 안함
    if (DistanceToTarget < MinTeleportDistance) return false;

    // 최대 텔레포트 거리보다 멀면 순간이동 안함
    if (DistanceToTarget > TeleportDistance) return false;

    return true;
}


void UMeleeCombatComponent::TeleportToTarget(AActor* TargetEnemy)
{
    if (!TargetEnemy || !OwnerCharacter) return;

    // 적의 위치와 방향 계산
    FVector EnemyLocation = TargetEnemy->GetActorLocation();
    FVector EnemyForward = TargetEnemy->GetActorForwardVector();

    // 적의 전방으로 순간이동 위치 계산
    FVector TeleportLocation = EnemyLocation + (EnemyForward * TeleportOffset);
    TeleportLocation.Z = OwnerCharacter->GetActorLocation().Z; // 높이는 유지

    // 바닥과의 충돌 체크해서 높이 조정 (선택사항)
    FHitResult FloorHit;
    FCollisionQueryParams FloorParams;
    FloorParams.AddIgnoredActor(OwnerCharacter);
    FloorParams.AddIgnoredActor(TargetEnemy);

    FVector FloorStart = TeleportLocation + FVector(0, 0, 100);
    FVector FloorEnd = TeleportLocation - FVector(0, 0, 500);

    if (GetWorld()->LineTraceSingleByChannel(FloorHit, FloorStart, FloorEnd, ECC_Visibility, FloorParams))
    {
        TeleportLocation.Z = FloorHit.Location.Z + 90.0f; // 캐릭터 발 위치 조정
    }

    // 순간이동 실행
    OwnerCharacter->SetActorLocation(TeleportLocation);

    // 적을 향해 회전
    FVector DirectionToEnemy = EnemyLocation - TeleportLocation;
    DirectionToEnemy.Z = 0.0f;
    DirectionToEnemy.Normalize();

    FRotator NewRot = FRotationMatrix::MakeFromX(DirectionToEnemy).Rotator();
    OwnerCharacter->SetActorRotation(NewRot);

    LastAttackDirection = DirectionToEnemy;

    // 시각적 효과들
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
    // MainCharacter 참조를 통해 에임모드 상태 확인
    if (AMainCharacter* MainChar = Cast<AMainCharacter>(OwnerCharacter))
    {
        if (MainChar->IsAiming())
        {
            UE_LOG(LogTemp, Warning, TEXT("Jump Attack Blocked in MeleeCombatComponent - Aim Mode Active"));
            return;
        }
    }

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