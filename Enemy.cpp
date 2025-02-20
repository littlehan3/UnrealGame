#include "Enemy.h"
#include "EnemyKatana.h"
#include "EnemyAnimInstance.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"

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

    if (Health <= 0.0f)
    {
        Die();
    }

    return DamageApplied;
}

void AEnemy::Die()
{
    if (bIsDead) return;  // 중복 실행 방지

    bIsDead = true;  // 사망 상태 변경
    UE_LOG(LogTemp, Warning, TEXT("Enemy Died! Now in dead state."));

    if (DieSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DieSound, GetActorLocation());
    }
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

void AEnemy::PlayAttackAnimation()
{
    if (AttackMontages.Num() > 0)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance && !AnimInstance->IsAnyMontagePlaying())
        {
            int32 RandomIndex = FMath::RandRange(0, AttackMontages.Num() - 1);
            UAnimMontage* SelectedMontage = AttackMontages[RandomIndex];

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
                if (AttackSound)
                {
                    UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation());
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
