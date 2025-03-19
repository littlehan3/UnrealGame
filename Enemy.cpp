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

        // 히트 애니메이션 종료 후 스턴 애니메이션 재생
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &AEnemy::OnHitMontageEnded);
        EnemyAnimInstance->Montage_SetEndDelegate(EndDelegate, HitReactionMontage);
    }

    if (Health <= 0.0f)
    {
        Die();
    }

    return DamageApplied;
}

void AEnemy::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (bIsDead) return;

    // 공중 스턴 상태일 때만 다시 스턴 애니메이션 실행
    if (bIsInAirStun)
    {
        if (EnemyAnimInstance && InAirStunMontage)
        {
            UE_LOG(LogTemp, Warning, TEXT("Hit animation ended while airborne. Resuming InAirStunMontage."));
            EnemyAnimInstance->Montage_Play(InAirStunMontage, 1.0f);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Hit animation ended on ground. No need to resume InAirStunMontage."));
    }
}


void AEnemy::Die()
{
    if (bIsDead) return;

    bIsDead = true;
    StopActions();

    float HideTime = 0.0f;

    if (bIsInAirStun && InAirStunDeathMontage) // 공중에서 사망 시
    {
        float AirDeathDuration = InAirStunDeathMontage->GetPlayLength();
		EnemyAnimInstance->Montage_Play(InAirStunDeathMontage, 1.0f); // 애니메이션 재생속도 조절
        HideTime = AirDeathDuration * 0.35f; // 애니메이션 재생 시간의 설정한 % 만큼 재생 후 사라짐
    }
    else if (EnemyAnimInstance && DeadMontage) // 일반 사망 시
    {
        float DeathAnimDuration = DeadMontage->GetPlayLength();
        EnemyAnimInstance->Montage_Play(DeadMontage, 1.0f);
        HideTime = DeathAnimDuration * 0.6f; // 애니메이션 재생 시간의 설정한 % 만큼 재생 후 사라짐
    }
    else
    {
        // 사망 애니메이션이 없을 경우 즉시 사라지게 함
        HideEnemy();
        return;
    }

    // 일정 시간 후 사라지도록 설정
    GetWorld()->GetTimerManager().SetTimer(DeathTimerHandle, this, &AEnemy::HideEnemy, HideTime, false);

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

    // 모든 입력 및 이동 차단
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();

    // 모든 공격 중지
    if (EnemyAnimInstance)
    {
        EnemyAnimInstance->Montage_Stop(0.1f);
    }

    // 스턴 상태일 경우 추가 조치
    if (bIsInAirStun)
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy is stunned! Forcing all actions to stop."));
        bCanAttack = false; // 공격 불가 상태 유지
        GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle); // 스턴 해제 타이머 취소
    }

    UE_LOG(LogTemp, Warning, TEXT("All actions stopped for dead enemy."));
}

void AEnemy::HideEnemy()
{
    if (!bIsDead) return;

    SetActorHiddenInGame(true);  // 렌더링 숨김
    SetActorEnableCollision(false); // 충돌 제거
    SetActorTickEnabled(false); // AI 전체 Tick 비활성화

    // 카타나도 함께 숨기기
    if (EquippedKatana)
    {
        EquippedKatana->HideKatana();
    }

    UE_LOG(LogTemp, Warning, TEXT("Enemy %s and katana disappeared mid-death animation!"), *GetName());
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
    if (!EnemyAnimInstance || NormalAttackMontages.Num() == 0) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

    // 애니메이션 실행 중이라면 공격 실행 금지
    if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayNormalAttackAnimation() blocked: Animation still playing."));
        return;
    }

    int32 RandomIndex = FMath::RandRange(0, NormalAttackMontages.Num() - 1);
    UAnimMontage* SelectedMontage = NormalAttackMontages[RandomIndex];

    if (SelectedMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy is playing attack montage: %s"), *SelectedMontage->GetName());

        float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f);
        if (PlayResult == 0.0f)
        {
            UE_LOG(LogTemp, Error, TEXT("Montage_Play failed! Check slot settings."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Montage successfully playing."));
        }

        // 공격 실행 후 AI 이동 정지
        AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());
        if (AICon)
        {
            AICon->StopMovement();
            UE_LOG(LogTemp, Warning, TEXT("Enemy stopped moving to attack!"));
        }

        bCanAttack = false;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Selected Montage is NULL!"));
    }
}

void AEnemy::PlayStrongAttackAnimation()
{
    if (!EnemyAnimInstance || !StrongAttackMontage) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

    // 애니메이션 실행 중이라면 강공격 실행 금지
    if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayStrongAttackAnimation() blocked: Animation still playing."));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Enemy is performing StrongAttack: %s"), *StrongAttackMontage->GetName());
    AnimInstance->Montage_Play(StrongAttackMontage, 1.0f);

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

void AEnemy::EnterInAirStunState(float Duration)
{
    if (bIsDead || bIsInAirStun) return;
    UE_LOG(LogTemp, Warning, TEXT("Entering InAirStunState..."));

	bIsInAirStun = true;

    // AI 멈추기 (바로 이동 정지하지 않고, 스턴 종료 시점에서 다시 활성화)
    AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());
    if (AICon)
    {
        UE_LOG(LogTemp, Warning, TEXT("Stopping AI manually..."));
        AICon->StopMovement();
    }

    // 적을 위로 띄우기 (LaunchCharacter 먼저 실행)
    FVector LaunchDirection = FVector(0.0f, 0.0f, 1.0f); // 위쪽 방향
    float LaunchStrength = 600.0f; // 강한 힘 적용
    LaunchCharacter(LaunchDirection * LaunchStrength, true, true);

    UE_LOG(LogTemp, Warning, TEXT("Enemy %s launched upwards! Current Location: %s"), *GetName(), *GetActorLocation().ToString());

    // 일정 시간 후 중력 제거 (즉시 0으로 만들면 착지가 방해될 수 있음)
    FTimerHandle GravityDisableHandle;
    GetWorld()->GetTimerManager().SetTimer(
        GravityDisableHandle,
        [this]()
        {
            GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            GetCharacterMovement()->GravityScale = 0.0f;
            GetCharacterMovement()->Velocity = FVector::ZeroVector; // 위치 고정
            UE_LOG(LogTemp, Warning, TEXT("Enemy %s gravity disabled, now floating!"), *GetName());
        },
        0.3f, // 0.3초 후 중력 제거
        false
    );

    // 스턴 애니메이션 실행
    if (EnemyAnimInstance && InAirStunMontage)
    {
        EnemyAnimInstance->Montage_Play(InAirStunMontage, 1.0f);
    }

    // 일정 시간이 지나면 원래 상태로 복귀
    GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, this, &AEnemy::ExitInAirStunState, Duration,false);
    UE_LOG(LogTemp, Warning, TEXT("Enemy %s is now stunned for %f seconds!"), *GetName(), Duration);
}

void AEnemy::ExitInAirStunState()
{
    if (bIsDead) return;
    UE_LOG(LogTemp, Warning, TEXT("Exiting InAirStunState..."));

	bIsInAirStun = false;

    // 중력 복구 및 낙하 상태로 변경
    GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    GetCharacterMovement()->GravityScale = 1.5f; // 조금 더 빠르게 낙하

    // AI 이동 다시 활성화
    AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());
    if (AICon)
    {
        UE_LOG(LogTemp, Warning, TEXT("Reactivating AI movement..."));
        GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
        GetCharacterMovement()->SetDefaultMovementMode();

        // 다시 이동 시작
        AICon->MoveToActor(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    }

    // 애니메이션 정지
    if (EnemyAnimInstance)
    {
        EnemyAnimInstance->Montage_Stop(0.1f);
    }

    UE_LOG(LogTemp, Warning, TEXT("Enemy %s has recovered from stun and resumed AI behavior!"), *GetName());

    bIsInAirStun = false;
}