#include "Enemy.h"
#include "EnemyKatana.h"
#include "EnemyAnimInstance.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "BrainComponent.h" // AI 컨트롤러의 BrainComponent 사용을 위해 추가


AEnemy::AEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    bCanBeLockedOn = true;

    AIControllerClass = AEnemyAIController::StaticClass(); // AI 컨트롤러 설정

    GetMesh()->SetAnimInstanceClass(UEnemyAnimInstance::StaticClass()); // 애님 인스턴스 설정으로 보장

    if (!AIControllerClass)
    {
        UE_LOG(LogTemp, Error, TEXT("AEnemy: AIControllerClass is NULL!"));
    }
}

void AEnemy::BeginPlay()
{
    Super::BeginPlay();
    SetCanBeDamaged(true);

    AAIController* AICon = Cast<AAIController>(GetController());
    if (AICon)
    {
        UE_LOG(LogTemp, Warning, TEXT("AEnemy AIController Assigned: %s"), *AICon->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AEnemy AIController is NULL!"));
    }

    SetUpAI();  // AI 설정 함수 호출

    EnemyAnimInstance = Cast<UEnemyAnimInstance>(GetMesh()->GetAnimInstance()); //  애님 인스턴스 설정

    // KatanaClass가 설정되어 있다면 Katana 스폰 및 부착
    if (KatanaClass)
    {
        EquippedKatana = GetWorld()->SpawnActor<AEnemyKatana>(KatanaClass);
        if (EquippedKatana)
        {
            USkeletalMeshComponent* MeshComp = GetMesh();
            if (MeshComp)
            {
                FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
                EquippedKatana->AttachToComponent(MeshComp, AttachmentRules, FName("EnemyKatanaSocket"));
            }
        }
    }
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead)  // 이미 죽은 상태면 데미지 무시
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy is already dead! Ignoring further damage."));
        return 0.0f;
    }

    float DamageApplied = FMath::Min(Health, DamageAmount);
    Health -= DamageApplied;

    UE_LOG(LogTemp, Warning, TEXT("Enemy took %f damage, Health remaining: %f"), DamageAmount, Health);

    if (HitSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
    }

    if (EnemyAnimInstance && HitReactionMontage) // 히트 시 애니메이션 재생
    {
        EnemyAnimInstance->Montage_Play(HitReactionMontage, 1.0f);
    }

    if (Health <= 0.0f)
    {
        Die();
    }

    return DamageApplied;
}


void AEnemy::Die()
{
    if (bIsDead) return;

    bIsDead = true;
    StopActions();

    // 사망 애니메이션 재생
    if (EnemyAnimInstance && DeadMontage)
    {
        EnemyAnimInstance->Montage_Play(DeadMontage, 1.0f);

        // 해당 사망 애니메이션이 끝나기 직전에 사망 포즈를 고정하기 위한 타이머 설정
        float DeathAnimDuration = DeadMontage->GetPlayLength();
        GetWorld()->GetTimerManager().SetTimer(
            DeathTimerHandle,
            this,
            &AEnemy::FreezeDeadPose,
            DeathAnimDuration - 0.35f, // 애니메이션이 끝나기 N초 전에 멈춤 (애니메이션에 따라 상이하게 설정해야함)
            false);
    }
    else
    {
        // 사망 몽타주가 없으면 즉시 고정
        FreezeDeadPose();
    }

    // AI 컨트롤러 중지
    AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());
    if (AICon)
    {
        AICon->StopAI();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AIController is NULL! Can not stop AI."));
    }

    // 이동 비활성화
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();
    SetActorTickEnabled(false); // AI Tick 중지
}


void AEnemy::StopActions()
{
    AAIController* AICon = Cast<AAIController>(GetController());
    if (AICon)
    {
        AICon->StopMovement();

        // BrainComponent가 nullptr이 아닌지 확인 후 호출
        if (AICon->BrainComponent)
        {
            AICon->BrainComponent->StopLogic(TEXT("Enemy Died"));
            UE_LOG(LogTemp, Warning, TEXT("BrainComponent logic stopped."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("BrainComponent is NULL! AI logic not stopped."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AIController is NULL! AI behavior not stopped."));
    }

    // 모든 입력 및 이동 차단
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();

    // 모든 공격 중지
    if (EnemyAnimInstance)
    {
        EnemyAnimInstance->Montage_Stop(0.1f);
    }

    UE_LOG(LogTemp, Warning, TEXT("All actions stopped for dead enemy."));
}



void AEnemy::FreezeDeadPose()
{
    if (!GetMesh() || !bIsDead) return;

    UE_LOG(LogTemp, Warning, TEXT("%s Freezing DeadPose"), *GetName());

    // 모든 애니메이션을 중지하고 현재 포즈 고정
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance)
    {
        // 재생 중인 몽타주 모두 중지
        AnimInstance->Montage_Stop(0.0f);

        // 사망 애니메이션의 마지막 프레임을 유지하도록 강제 (Idle상태로 돌아가지않게)
        GetMesh()->bPauseAnims = true; // 애니메이션 정지
        GetMesh()->bNoSkeletonUpdate = true; // 스켈레톤 업데이트 중지 (Transform 변화 방지)
    }

    // 추가 AI 액션 방지
    AAIController* AIController = Cast<AAIController>(GetController());
    if (AIController)
    {
        AIController->UnPossess();
    }

    // 모든 이동 비활성화 
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();

    // 틱 비활성화
    SetActorTickEnabled(false); // AI 전체 Tick 비활성화
    GetMesh()->SetComponentTickEnabled(false); // 매쉬 Tick 비활성화

    UE_LOG(LogTemp, Warning, TEXT("DeadPose Freezed. Enemy %s Maintaining DeadPose."), *GetName());
}



// 특정 조건을 만족해야 락온 가능
bool AEnemy::CanBeLockedOn() const
{
    // 특정 체력 이하일 때만 락온 등 확장  
    // if (CurrentHealth < 50.0f) return false;

    return bCanBeLockedOn; // 기본적으로 bCanBeLockedOn이 true인 경우 락온 가능
}


// AI가 NavMesh에서 이동할 수 있도록 설정
void AEnemy::SetUpAI()
{
    GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking);
}

// AI 이동
void AEnemy::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
        {
            AAIController* AICon = Cast<AAIController>(GetController());
            if (AICon)
            {
                UE_LOG(LogTemp, Warning, TEXT("AEnemy AIController Assigned Later: %s"), *AICon->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("AEnemy AIController STILL NULL!"));
            }
        });
}

void AEnemy::PlayNormalAttackAnimation()
{
    if (NormalAttackMontages.Num() > 0)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance && !AnimInstance->IsAnyMontagePlaying())
        {
            int32 RandomIndex = FMath::RandRange(0, NormalAttackMontages.Num() - 1);
            UAnimMontage* SelectedMontage = NormalAttackMontages[RandomIndex];

            if (SelectedMontage)
            {
                UE_LOG(LogTemp, Warning, TEXT("Enemy is playing attack montage: %s"), *SelectedMontage->GetName());

                // 공격 몽타주 실행
                float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);
                if (PlayResult == 0.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("Montage_Play failed! Check slot settings."));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Montage successfully playing."));
                }
                //공격 시 사운드 재생
                if (NormalAttackSound)
                {
                    UGameplayStatics::PlaySoundAtLocation(this, NormalAttackSound, GetActorLocation());
                }

                // AI 이동 멈춤
                AAIController* AICon = Cast<AAIController>(GetController());
                if (AICon)
                {
                    AICon->StopMovement();
                    UE_LOG(LogTemp, Warning, TEXT("Enemy stopped moving to attack!"));
                }

                // 공격 가능 상태 리셋을 애니메이션 끝날 때까지 지연
                bCanAttack = false;
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Selected Montage is NULL!"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Enemy is already playing an animation!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No attack montages available!"));
    }
}

void AEnemy::PlayStrongAttackAnimation()
{
    if (StrongAttackMontage)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            UE_LOG(LogTemp, Warning, TEXT("Enemy is performing StrongAttack: %s"), *StrongAttackMontage->GetName());
            AnimInstance->Montage_Play(StrongAttackMontage, 1.0f);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("StrongAttack montage is NULL! Check BP_Enemy."));
    }

    if (StrongAttackSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, StrongAttackSound, GetActorLocation());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("StrongAttack sound is NULL! Check BP_Enemy."));
    }
}

void AEnemy::PlayDodgeAnimation(bool bDodgeLeft)
{
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance) return;

    UAnimMontage* SelectedMontage = (bDodgeLeft) ? DodgeLeftMontage : DodgeRightMontage;

    if (SelectedMontage && AnimInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy is dodging with montage: %s"), *SelectedMontage->GetName());
        AnimInstance->Montage_Play(SelectedMontage, 1.0f);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Selected dodge montage is NULL!"));
    }
}
float AEnemy::GetDodgeLeftDuration() const
{
    return (DodgeLeftMontage) ? DodgeLeftMontage->GetPlayLength() : 1.0f;
}

float AEnemy::GetDodgeRightDuration() const
{
    return (DodgeRightMontage) ? DodgeRightMontage->GetPlayLength() : 1.0f;
}

void AEnemy::PlayJumpAttackAnimation()
{
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance || !JumpAttackMontage) return;

    AnimInstance->Montage_Play(JumpAttackMontage, 1.0f);
}

float AEnemy::GetJumpAttackDuration() const
{
    return (JumpAttackMontage) ? JumpAttackMontage->GetPlayLength() : 1.0f;
}