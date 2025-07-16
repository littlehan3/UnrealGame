#include "Enemy.h"
#include "EnemyKatana.h"
#include "EnemyAnimInstance.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "ObjectPoolManager.h"


AEnemy::AEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    LastPlayedJumpAttackMontage = nullptr;
    bCanAttack = true;

    AIControllerClass = AEnemyAIController::StaticClass(); // AI 컨트롤러 설정
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; //스폰 시에도 AI 컨트롤러 자동 할당

    GetMesh()->SetAnimInstanceClass(UEnemyAnimInstance::StaticClass()); // 애님 인스턴스 설정으로 보장

    if (!AIControllerClass)
    {
        //UE_LOG(LogTemp, Error, TEXT("AEnemy: AIControllerClass is NULL!"));
    }

    GetCharacterMovement()->MaxWalkSpeed = 300.0f; // 기본 이동속도 세팅
}

void AEnemy::BeginPlay()
{
    Super::BeginPlay();
    SetCanBeDamaged(true);

    // Actor Tick 빈도 제한 (60fps → 20fps)
    SetActorTickInterval(0.05f);

    //// AI 완전 비활성화 테스트
    //SetActorTickEnabled(false);
    //if (AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController()))
    //{
    //    AICon->SetActorTickEnabled(false);
    //}
    
    // AI 컨트롤러 강제 할당
    if (!GetController())
    {
        SpawnDefaultController();
        UE_LOG(LogTemp, Warning, TEXT("Enemy AI Controller manually spawned"));
    }

    AAIController* AICon = Cast<AAIController>(GetController());
    if (AICon)
    {
        //UE_LOG(LogTemp, Warning, TEXT("AEnemy AIController Assigned: %s"), *AICon->GetName());
    }
    else
    {
        //UE_LOG(LogTemp, Error, TEXT("AEnemy AIController is NULL!"));
    }

    SetUpAI();  // AI 설정 함수 호출

    EnemyAnimInstance = Cast<UEnemyAnimInstance>(GetMesh()->GetAnimInstance());

    // 앨리트 적 확률 판정
    float EliteChance = 0.1f;
    if (FMath::FRand() < EliteChance)
    {
        bIsEliteEnemy = true;
        ApplyEliteSettings();
        ApplyBaseWalkSpeed();
    }

    // KatanaClass가 설정되어 있다면 Katana 스폰 및 부착
    if (KatanaClass)
    {
        EquippedKatana = GetWorld()->SpawnActor<AEnemyKatana>(KatanaClass);
        if (EquippedKatana)
        {
            EquippedKatana->SetOwner(this);
            USkeletalMeshComponent* MeshComp = GetMesh();
            if (MeshComp)
            {
                FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
                EquippedKatana->AttachToComponent(MeshComp, AttachmentRules, FName("EnemyKatanaSocket"));
            }
        }
    }
}

void AEnemy::ApplyEliteSettings()
{
    Health = 200.0f;
}

void AEnemy::ApplyBaseWalkSpeed() 
{
    GetCharacterMovement()->MaxWalkSpeed = bIsEliteEnemy ? 500.0f : 300.0f; // 앨리트 일시 500 아닐시 300
    GetCharacterMovement()->MaxAcceleration = 5000.0f; // 즉시 최대속도 도달
    GetCharacterMovement()->BrakingFrictionFactor = 10.0f;
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead)  // 이미 죽은 상태면 데미지 무시
    {
        //UE_LOG(LogTemp, Warning, TEXT("Enemy is already dead! Ignoring further damage."));
        return 0.0f;
    }

    float DamageApplied = FMath::Min(Health, DamageAmount);
    Health -= DamageApplied;

    UE_LOG(LogTemp, Warning, TEXT("Enemy took %f damage, Health remaining: %f"), DamageAmount, Health);

    if (HitSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
    }

    if (!bIsStrongAttack && EnemyAnimInstance && HitReactionMontage) // 히트 시 애니메이션 재생 (강공격중엔 히트몽타주 재생안함)
    {
        EnemyAnimInstance->Montage_Play(HitReactionMontage, 1.0f);

        // 히트 애니메이션 종료 후 스턴 애니메이션 재생
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &AEnemy::OnHitMontageEnded);
        EnemyAnimInstance->Montage_SetEndDelegate(EndDelegate, HitReactionMontage);
    }

    if (Health <= 0.0f) // 체력이 0 이하일 경우 사망 (강공격중에도 호출)
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


void AEnemy::InstantDeath()
{
    if (bIsDead)
    {
        return; // 이미 죽은 상태면 리턴
    }

    UE_LOG(LogTemp, Warning, TEXT("Enemy %s killed instantly for boss spawn"), *GetName());

    // 현재 체력을 0으로 설정
    Health = 0.0f;

    // 기존 Die() 함수 호출 (자연스러운 사망 처리)
    Die();
}

void AEnemy::StopActions()
{
    AAIController* AICon = Cast<AAIController>(GetController());

    GetCharacterMovement()->DisableMovement(); // 모든 이동 차단
    GetCharacterMovement()->StopMovementImmediately(); // 모든 이동 즉시차단

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

    // 카타나 공격판정 중지
    if (EquippedKatana)
    {
        EquippedKatana->DisableAttackHitDetection();
    }
}

void AEnemy::HideEnemy()
{
    if (!bIsDead) return; // 사망하지 않았으면 리턴

    UE_LOG(LogTemp, Warning, TEXT("Hiding Enemy - Memory Cleanup"));

    // GameMode에 Enemy 파괴 알림
    if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
    {
        GameMode->OnEnemyDestroyed(this);
    }

    // 1. 이벤트 및 델리게이트 정리 (최우선)
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 해제

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 애님 인스턴스 참조 받아옴
    if (AnimInstance && IsValid(AnimInstance)) // 애님 인스턴스 유효성 검사
    {
        // 애니메이션 이벤트 바인딩 완전 해제
        AnimInstance->OnMontageEnded.RemoveAll(this); // 몽타주 종료 이벤트 바인딩 해제
        AnimInstance->OnMontageBlendingOut.RemoveAll(this); // 몽타주 블랜드 아웃 이벤트 바인딩 해체
        AnimInstance->OnMontageStarted.RemoveAll(this); // 몽타주 시작 이벤트 바인딩 해제
    }

    // 2. AI 시스템 완전 정리
    AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController()); // AI 컨트롤러 참조 받아옴
    if (AICon && IsValid(AICon)) // AI 컨트롤러 유효성 검사
    {
        AICon->StopAI(); // AI 로직 중단
        AICon->UnPossess(); // 컨트롤러-폰 관계 해제
        AICon->Destroy(); // AI 컨트롤러 완전 제거
    }

    // 3. 무기 시스템 정리 (AI 정리 후 안전하게)
    if (EquippedKatana && IsValid(EquippedKatana)) // 무기 유효성 검사
    {
        EquippedKatana->HideKatana(); // AI가 정리 후 무기를 제거하는 HideKatana 함수 호출
        EquippedKatana = nullptr; // 무기 참조 해제
    }

    // 4. 무브먼트 시스템 정리
    UCharacterMovementComponent* MovementComp = GetCharacterMovement(); // 캐릭터 무브먼트 컴포넌트 참조 받아옴
    if (MovementComp && IsValid(MovementComp)) // 무브먼트 컴포넌트 유효성 검사
    {
        MovementComp->DisableMovement(); // 이동 비활성화
        MovementComp->StopMovementImmediately(); // 현재 이동 즉시 중단
        MovementComp->SetMovementMode(EMovementMode::MOVE_None); // Move모드 None 설정으로 네비게이션에서 제외
        MovementComp->SetComponentTickEnabled(false); // 무브먼트 컴포넌트 Tick 비활성화
    }

    // 5. 메쉬 컴포넌트 정리
    USkeletalMeshComponent* MeshComp = GetMesh(); // 메쉬 컴포넌트 참조 받아옴
    if (MeshComp && IsValid(MeshComp)) // 메쉬 컴포넌트 유효성 검사
    {
        // 렌더링 시스템 비활성화
        MeshComp->SetVisibility(false); // 메쉬 가시성 비활성화
        MeshComp->SetHiddenInGame(true); // 게임 내 숨김 처리

        // 물리 시스템 비활성화
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // NoCollision 설정으로 충돌검사 비활성화
        MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // ECR_Ignore 설정으로 충돌응답 무시

        // 업데이트 시스템 비활성화
        MeshComp->SetComponentTickEnabled(false); // Tick 비활성화

        // 애니메이션 참조 해제
        MeshComp->SetAnimInstanceClass(nullptr); // ABP 참조 해제
        MeshComp->SetSkeletalMesh(nullptr); // 스켈레탈 메쉬 참조 해제
    }

    // 6. 액터 레벨 시스템 정리
    SetActorHiddenInGame(true); // 액터 렌더링 비활성화
    SetActorEnableCollision(false); // 액터 충돌 비활성화
    SetActorTickEnabled(false); // 액터 Tick 비활성화
    SetCanBeDamaged(false); // 데미지 처리 비활성화

    // 7. 현재 프레임 처리 완료 후 다음 프레임에 안전하게 엑터 제거 (크래쉬 방지)
    GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemy>(this)]() // 스마트 포인터 WeakObjectPtr로 약한 참조를 사용하여 안전하게 지연 실행
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // 약한 참조한 엑터가 유효하고 파괴되지 않았다면
            {
                WeakThis->Destroy(); // 액터 완전 제거
                UE_LOG(LogTemp, Warning, TEXT("Enemy Successfully Destroyed"));
            }
        });
}

// AI가 NavMesh에서 이동할 수 있도록 설정
void AEnemy::SetUpAI()
{
    GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking);
}

// AI 컨트롤러 확인
void AEnemy::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
        {
            AAIController* AICon = Cast<AAIController>(GetController());
            if (AICon)
            {
                //UE_LOG(LogTemp, Warning, TEXT("AEnemy AIController Assigned Later: %s"), *AICon->GetName());
            }
            else
            {
                //UE_LOG(LogTemp, Error, TEXT("AEnemy AIController STILL NULL!"));
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
        //UE_LOG(LogTemp, Warning, TEXT("PlayNormalAttackAnimation() blocked: Animation still playing."));
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

        if (bIsEliteEnemy && EnemyAnimInstance && SelectedMontage) // 앨리트 적인 경우
        {
            EnemyAnimInstance->Montage_SetPlayRate(SelectedMontage, 1.5f); // 몽타주 배속
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

    bIsStrongAttack = true;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

    // 애니메이션 실행 중이라면 강공격 실행 금지
    if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
    {
        //UE_LOG(LogTemp, Warning, TEXT("PlayStrongAttackAnimation() blocked: Animation still playing."));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Enemy is performing StrongAttack: %s"), *StrongAttackMontage->GetName());
    AnimInstance->Montage_Play(StrongAttackMontage, 1.0f);

    if (bIsEliteEnemy && EnemyAnimInstance) // 앨리트 적인 경우
    {
        EnemyAnimInstance->Montage_SetPlayRate(StrongAttackMontage, 1.5f); // 몽타주 배속
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

    if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return;

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

void AEnemy::SetRootMotionEnable(bool bEnable)
{
    if (EnemyAnimInstance)
        EnemyAnimInstance->SetRootMotionMode(bEnable ? ERootMotionMode::RootMotionFromMontagesOnly : ERootMotionMode::NoRootMotionExtraction);
}

void AEnemy::OnDodgeLaunchNotify(bool bDodgeLeft)
{
    SetRootMotionEnable(false); // 루트모션 비활성화
    FVector LaunchDir = bDodgeLeft ? -GetActorRightVector() : GetActorRightVector();
    float LaunchStrength = 1000.0f;
    LaunchCharacter(LaunchDir * LaunchStrength, true, true);
    UE_LOG(LogTemp, Warning, TEXT("Dodge Launch! Direction: %s, Strength: %f"), *LaunchDir.ToString(), LaunchStrength);
}

void AEnemy::OnDodgeLaunchEndNotify()
{
    SetRootMotionEnable(true); // 루트모션 활성화
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
    if (!EnemyAnimInstance || JumpAttackMontages.Num() == 0) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

    if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return;
  
    int32 RandomIndex = FMath::RandRange(0, JumpAttackMontages.Num() - 1);
    UAnimMontage* SelectedMontage = JumpAttackMontages[RandomIndex];

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
        if (bIsEliteEnemy && EnemyAnimInstance && SelectedMontage) // 앨리트 적인 경우
        {
            EnemyAnimInstance->Montage_SetPlayRate(SelectedMontage, 1.5f); // 몽타주 배속
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
        LastPlayedJumpAttackMontage = nullptr;
    }
}

float AEnemy::GetJumpAttackDuration() const
{
    return (LastPlayedJumpAttackMontage) ? LastPlayedJumpAttackMontage->GetPlayLength() : 1.0f;
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

    // 카타나 공격판정 중지
    if (EquippedKatana)
    {
        EquippedKatana->DisableAttackHitDetection();
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
    ApplyBaseWalkSpeed();

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

void AEnemy::ApplyGravityPull(FVector ExplosionCenter, float PullStrength)
{
    if (bIsDead) return; // 죽은 적은 끌어당기지 않음

    // 폭발 범위 내에서 적을 빨아들이는 로직
    FVector Direction = ExplosionCenter - GetActorLocation();
    float Distance = Direction.Size();
    Direction.Normalize();  // 방향 벡터 정규화

    // 거리에 따라 힘 조절 (가까울수록 더 강하게)
    float DistanceFactor = FMath::Clamp(1.0f - (Distance / 500.0f), 0.1f, 1.0f);
    float AdjustedPullStrength = PullStrength * DistanceFactor;

    // 캐릭터가 공중에 있는 상태라면 더 강한 힘 적용
    if (GetCharacterMovement()->IsFalling())
    {
        AdjustedPullStrength *= 1.5f;
    }

    // 새로운 속도 설정
    FVector NewVelocity = Direction * AdjustedPullStrength;
    GetCharacterMovement()->Velocity = NewVelocity;

    // 잠시 네비게이션 이동 비활성화
    GetCharacterMovement()->SetMovementMode(MOVE_Flying);

    // 일정 시간 후 네비게이션 이동 다시 활성화
    FTimerHandle ResetMovementHandle;
    GetWorld()->GetTimerManager().SetTimer(
        ResetMovementHandle,
        [this]()
        {
            GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
        },
        0.5f, // 0.5초 후 원래 이동 모드로 복귀
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("Enemy %s pulled toward explosion center with strength %f"),
        *GetName(), AdjustedPullStrength);
}

void AEnemy::StartAttack(EAttackType AttackType)
{
    if (EquippedKatana)
    {
        //EquippedKatana->StartAttack();
        EquippedKatana->EnableAttackHitDetection(AttackType);
    }
}

void AEnemy::EndAttack()
{
    if (EquippedKatana)
    {
        //EquippedKatana->EndAttack();
        EquippedKatana->DisableAttackHitDetection();
    }
}