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
#include "MainGameModeBase.h"

AEnemy::AEnemy()
{
    PrimaryActorTick.bCanEverTick = true;

	AICon = nullptr;
	AnimInstance = nullptr;
	MoveComp = GetCharacterMovement();

	ApplyBaseWalkSpeed(); // 기본 이동 속도 적용 함수 호출

    LastPlayedJumpAttackMontage = nullptr;

    AIControllerClass = AEnemyAIController::StaticClass(); // AI 컨트롤러 설정
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; //스폰 시에도 AI 컨트롤러 자동 할당
}

void AEnemy::BeginPlay()
{
    Super::BeginPlay();
    
    UWorld* World = GetWorld();
    if (!World) return;

    SetCanBeDamaged(true); // 데미지를 받을 수 있음
    // Actor Tick 빈도 제한 (60fps → 20fps)
    SetActorTickInterval(0.05f);

    //// AI 완전 비활성화 테스트
    //SetActorTickEnabled(false);
    //if (AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController()))
    //{
    //    AICon->SetActorTickEnabled(false);
    //}

    AICon = Cast<AEnemyAIController>(GetController()); // AI 컨트롤러 가져옴
    if (!AICon) // AI 컨트롤러가 없다면
    {
        SpawnDefaultController(); // AI 컨트롤러 스폰
        AICon = Cast<AEnemyAIController>(GetController());  // AI 컨트롤러 다시 가져옴
    }

    if (GetMesh())
    {
        AnimInstance = Cast<UEnemyAnimInstance>(GetMesh()->GetAnimInstance());
    }

    MoveComp = GetCharacterMovement();

    SetUpAI();  // AI 설정 함수 호출

    // 앨리트 설정
    if (FMath::FRand() < EliteChance) // 앨리트 스폰 확률로 등장
    {
        bIsEliteEnemy = true; // 앨리트 상태
        ApplyEliteSettings(); // 앨리트 설정 적용함수 호출
        ApplyBaseWalkSpeed(); // 앨리트의 이동속도 적용함수 호출
    }
    Health = MaxHealth; // 체력은 최대체력

    // 무기 설정
    if (IsValid(KatanaClass))
    {
        EquippedKatana = World->SpawnActor<AEnemyKatana>(KatanaClass); // EnemyKatana 클래스를 가져옴
        if (EquippedKatana) // 클래스가 있다면
        {
            EquippedKatana->SetOwner(this); // 소유자를 Enemy로 설정
            if (USkeletalMeshComponent* MeshComp = GetMesh()) // 스켈레탈 메시 컴포넌트를 가져옴
            {
                FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true); // 부착
                EquippedKatana->AttachToComponent(MeshComp, Rules, FName("EnemyKatanaSocket")); // 무기클래스를 해당 소켓에 부착
            }
        }
    }

    PlaySpawnIntroAnimation(); // 스폰 애니메이션 재생
}

float AEnemy::GetHealthPercent_Implementation() const
{
    // 사망 상태일 때는 무조건 0.0 반환
    if (bIsDead || Health <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Enemy is dead - returning 0.0 health percent"));
        return 0.0f;
    }

    if (MaxHealth <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("MaxHealth is 0 or negative: %f"), MaxHealth);
        return 0.0f;
    }

    float HealthPercent = Health / MaxHealth;

    UE_LOG(LogTemp, Error, TEXT("GetHealthPercent: Health=%f, MaxHealth=%f, Percent=%f"),
        Health, MaxHealth, HealthPercent);

    return FMath::Clamp(HealthPercent, 0.0f, 1.0f);
}

bool AEnemy::IsEnemyDead_Implementation() const
{
    return bIsDead;
}

void AEnemy::PlaySpawnIntroAnimation()
{
    UAnimMontage* SelectedIntroMontage = bIsEliteEnemy ? EliteSpawnIntroMontage : SpawnIntroMontage;

    if (!SelectedIntroMontage || !AnimInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("No intro montage found - skipping intro animation"));
        return;
    }

    bIsPlayingIntro = true;
    bCanAttack = false; // 등장 중에는 공격 불가

    // AI 이동 잠시 중지
    //AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());
    if (AICon)
    {
        AICon->StopMovement();
    }

    // 등장 몽타주 재생
    float PlayResult = AnimInstance->Montage_Play(SelectedIntroMontage, 1.0f);

    if (PlayResult > 0.0f)
    {
        // 등장 몽타주 종료 델리게이트 바인딩
        FOnMontageEnded IntroEndDelegate;
        IntroEndDelegate.BindUObject(this, &AEnemy::OnIntroMontageEnded);
        AnimInstance->Montage_SetEndDelegate(IntroEndDelegate, SelectedIntroMontage);

        UE_LOG(LogTemp, Warning, TEXT("%s Enemy intro animation playing"),
            bIsEliteEnemy ? TEXT("Elite") : TEXT("Normal"));
    }
    else
    {
        // 몽타주 재생 실패 시 즉시 활성화
        UE_LOG(LogTemp, Warning, TEXT("Intro montage failed to play"));
        bIsPlayingIntro = false;
        bCanAttack = true;
    }
}

void AEnemy::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Warning, TEXT("Enemy Intro Montage Ended"));

    bIsPlayingIntro = false;
    bCanAttack = true; // 등장 완료 후 공격 가능

    // AI 행동 재개 허용 (AI Controller에서 자동으로 재개됨)
    UE_LOG(LogTemp, Warning, TEXT("Enemy ready for combat after intro"));
}

void AEnemy::ApplyEliteSettings()
{
    MaxHealth = 400.0f;
}

void AEnemy::ApplyBaseWalkSpeed()
{
    if (MoveComp)
    {
        MoveComp->MaxWalkSpeed = bIsEliteEnemy ? 500.0f : 300.0f; // 앨리트 일시 500 아닐시 300
        MoveComp->MaxAcceleration = 5000.0f; // 즉시 최대속도 도달로 가속없이 정해진 이동 속도로 이동
        MoveComp->BrakingFrictionFactor = 10.0f;
    }
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    UWorld* World = GetWorld();
    if (!World) return 0.0f;
    if (bIsDead || bIsPlayingIntro) return 0.0f; // 이미 죽은 상태면 데미지 무시

	if (!AICon || !AnimInstance) return 0.0f; // AI 컨트롤러나 애님 인스턴스가 없으면 데미지 무시

    // Super::TakeDamage를 호출하기 전에 내부 체력 계산, 데미지 적용
    float DamageApplied = FMath::Min(Health, DamageAmount); // 데미지값은 체력에서 차감
    Health -= DamageApplied; // 체력- 데미지값
    UE_LOG(LogTemp, Warning, TEXT("Enemy took %f damage, Health remaining: %f"), DamageAmount, Health);

    if (IsValid(HitSound) && World) // 피격음이 있다면
    {
        UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation()); // 피격음 재생
    }

    // AI 컨트롤러를 가져옴
    //AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());

    // 피격 애니메이션 재생 (강공격시엔 피격 애니메이션이 안나옴)
    if (!bIsStrongAttack && IsValid(AnimInstance) && IsValid(HitReactionMontage))
    {
        AnimInstance->Montage_Play(HitReactionMontage, 1.0f);
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &AEnemy::OnHitMontageEnded);
        AnimInstance->Montage_SetEndDelegate(EndDelegate, HitReactionMontage);

        // 피격 애니메이션 재생중에는
        if (AICon) 
        {
            AICon->StopMovement(); // 움직이지 않음
        }
    }

    // 사망
    if (Health <= 0.0f) // 체력이 0 이하라면
    {
        Die(); // 사망 함수 호출
    }

    // 모든 내부 상태(Health, bIsDead)를 처리한 후에
    // Super::TakeDamage를 호출하여 OnTakeAnyDamage 이벤트를 Broadcast시킴
    // 이를 통해 UHealthBarComponent는 가장 최신 상태(bIsDead=true, Health=0)를 받아가게함
    Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    return DamageApplied;
}

void AEnemy::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (bIsDead) return;

    // 공중 스턴 상태일 때만 다시 스턴 애니메이션 실행
    if (bIsInAirStun)
    {
        if (AnimInstance && InAirStunMontage)
        {
            UE_LOG(LogTemp, Warning, TEXT("Hit animation ended while airborne. Resuming InAirStunMontage."));
            AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
        }
    }
    else
    {
        // AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());
        if (AICon)
        {
            // AI가 멈춰있다가 다시 플레이어를 추적하게 함
            AICon->MoveToActor(UGameplayStatics::GetPlayerCharacter(World, 0));
        }

        UE_LOG(LogTemp, Warning, TEXT("Hit animation ended on ground. No need to resume InAirStunMontage."));
    }
}

void AEnemy::Die()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (bIsDead) return;

    bIsDead = true;
    Health = 0.0f;  // 확실하게 0으로 설정

    if (IsValid(DieSound))
    {
        UGameplayStatics::PlaySoundAtLocation(this, DieSound, GetActorLocation());
    }

    StopActions();

    float HideTime = 0.0f;
    if (bIsInAirStun && InAirStunDeathMontage) // 공중에서 사망 시
    {
        float AirDeathDuration = InAirStunDeathMontage->GetPlayLength(); // 몽타주의 재생시간을 가져옴
        AnimInstance->Montage_Play(InAirStunDeathMontage, 1.0f); // 애니메이션 재생
        HideTime = AirDeathDuration * 0.35f; // 애니메이션 재생 시간의 설정한 % 만큼 재생 후 사라짐
    }
    else if (AnimInstance && DeadMontage) // 일반 사망 시
    {
        float DeathAnimDuration = DeadMontage->GetPlayLength(); // 몽타주의 재생시간을 가져옴
        AnimInstance->Montage_Play(DeadMontage, 1.0f); // 애니메이션 재생
        HideTime = DeathAnimDuration * 0.6f; // 애니메이션 재생 시간의 설정한 % 만큼 재생 후 사라짐
    }
    else // 사망 애니메이션이 없을 경우
    {
        HideEnemy(); // 즉시 숨김
        return;
    }
    // HideTime 만큼의 재생 시간 후 사라지도록 타이머 설정
    TWeakObjectPtr<AEnemy> WeakThis(this); // 약참조 선언
    World->GetTimerManager().SetTimer(DeathTimerHandle, [WeakThis]()
        {
            if (WeakThis.IsValid()) // 유효성 검사
            {
                WeakThis->HideEnemy(); // 사망한 적을 숨기는 함수 호출
            }
        }, HideTime, false); // HideTIme 만큼의 재생 시간 후, 단발성

    // AI 컨트롤러 중지
    //AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());
    if(AICon)
    {
        AICon->StopAI();
    }

    // 이동 중지
    if (MoveComp) // 캐릭터 무브먼트 컴포넌트를 가져옴
    {
        MoveComp->DisableMovement(); // 이동 중지
        MoveComp->StopMovementImmediately(); // 즉시 이동 중지
    }
    SetActorTickEnabled(false); // Tick 종료
}

void AEnemy::PlayWeaponHitSound()
{
    if (IsValid(EnemyWeaponHitSound))
    {
        UGameplayStatics::PlaySoundAtLocation(this, EnemyWeaponHitSound, GetActorLocation());
    }
}

void AEnemy::StopActions()
{
    UWorld* World = GetWorld();
    if (!World) return;
    /*AAIController* AICon = Cast<AAIController>(GetController());*/

    if (MoveComp) // 캐릭터 무브먼트 컴포넌트를 가져옴
    {
        MoveComp->DisableMovement(); // 이동 중지
        MoveComp->StopMovementImmediately(); // 즉시 이동 중지
    }

    // 모든 몽타주 중지
    if (AnimInstance)
    {
        AnimInstance->Montage_Stop(0.1f);
    }

    // 스턴 상태일 경우 추가 조치
    if (bIsInAirStun)
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy is stunned! Forcing all actions to stop."));
        bCanAttack = false; // 공격 불가 상태 유지
        World->GetTimerManager().ClearTimer(StunTimerHandle); // 스턴 해제 타이머 취소
    }

    // 카타나 공격판정 중지
    if (EquippedKatana)
    {
        EquippedKatana->DisableAttackHitDetection();
    }
}

void AEnemy::HideEnemy()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (!bIsDead) return; // 사망하지 않았으면 리턴

    UE_LOG(LogTemp, Warning, TEXT("Hiding Enemy - Memory Cleanup"));

    // GameMode에 파괴 알림
    if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(World->GetAuthGameMode()))
    {
        GameMode->OnEnemyDestroyed(this);
    }

    // 1. 이벤트 및 델리게이트 정리 (최우선)
    World->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 해제

    UAnimInstance* MyAnimInstance = GetMesh()->GetAnimInstance(); // 애님 인스턴스 참조 받아옴
    if (MyAnimInstance && IsValid(MyAnimInstance)) // 애님 인스턴스 유효성 검사
    {
        // 애니메이션 이벤트 바인딩 완전 해제
        MyAnimInstance->OnMontageEnded.RemoveAll(this); // 몽타주 종료 이벤트 바인딩 해제
        MyAnimInstance->OnMontageBlendingOut.RemoveAll(this); // 몽타주 블랜드 아웃 이벤트 바인딩 해체
        MyAnimInstance->OnMontageStarted.RemoveAll(this); // 몽타주 시작 이벤트 바인딩 해제
    }

    // 2. AI 시스템 완전 정리
    AEnemyAIController* EnemyAICon = Cast<AEnemyAIController>(GetController()); // AI 컨트롤러 참조 받아옴
    if (EnemyAICon && IsValid(EnemyAICon)) // AI 컨트롤러 유효성 검사
    {
        EnemyAICon->StopAI(); // AI 로직 중단
        EnemyAICon->UnPossess(); // 컨트롤러-폰 관계 해제
        EnemyAICon->Destroy(); // AI 컨트롤러 완전 제거
    }

    // 3. 무기 시스템 정리 (AI 정리 후 안전하게)
    if (EquippedKatana && IsValid(EquippedKatana)) // 무기 유효성 검사
    {
        EquippedKatana->HideKatana(); // AI가 정리 후 무기를 제거하는 HideKatana 함수 호출
        EquippedKatana = nullptr; // 무기 참조 해제
    }

    // 4. 무브먼트 시스템 정리
    UCharacterMovementComponent* EnemyMoveComp = GetCharacterMovement(); // 캐릭터 무브먼트 컴포넌트 참조 받아옴
    if (EnemyMoveComp && IsValid(EnemyMoveComp)) // 무브먼트 컴포넌트 유효성 검사
    {
        EnemyMoveComp->DisableMovement(); // 이동 비활성화
        EnemyMoveComp->StopMovementImmediately(); // 현재 이동 즉시 중단
        EnemyMoveComp->SetMovementMode(EMovementMode::MOVE_None); // Move모드 None 설정으로 네비게이션에서 제외
        EnemyMoveComp->SetComponentTickEnabled(false); // 무브먼트 컴포넌트 Tick 비활성화
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
	TWeakObjectPtr<AEnemy> WeakThis(this); // 스마트 포인터 WeakObjectPtr로 약한 참조 생성
    World->GetTimerManager().SetTimerForNextTick(
        [WeakThis]() // 스마트 포인터 WeakObjectPtr로 약한 참조를 사용하여 안전하게 지연 실행
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
    if (MoveComp)
    {
        MoveComp->SetMovementMode(EMovementMode::MOVE_NavWalking);
    }
}

void AEnemy::PlayNormalAttackAnimation()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (!AnimInstance || NormalAttackMontages.Num() == 0) return;

    //UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

    // 애니메이션 실행 중이라면 공격 실행 금지
    if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return;
  
    int32 RandomIndex = FMath::RandRange(0, NormalAttackMontages.Num() - 1);
    UAnimMontage* SelectedMontage = NormalAttackMontages[RandomIndex];

    if (IsValid(SelectedMontage))
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

        if (bIsEliteEnemy && AnimInstance && SelectedMontage) // 앨리트 적인 경우
        {
            AnimInstance->Montage_SetPlayRate(SelectedMontage, 1.5f); // 몽타주 배속
        }

        // 공격 실행 후 AI 이동 정지
        //AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());
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
    UWorld* World = GetWorld();
    if (!World) return;
    if (!AnimInstance || !StrongAttackMontage) return;

    bIsStrongAttack = true;

    //UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

    // 애니메이션 실행 중이라면 강공격 실행 금지
    if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
    {
        //UE_LOG(LogTemp, Warning, TEXT("PlayStrongAttackAnimation() blocked: Animation still playing."));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Enemy is performing StrongAttack: %s"), *StrongAttackMontage->GetName());
    AnimInstance->Montage_Play(StrongAttackMontage, 1.0f);

    if (bIsEliteEnemy && AnimInstance) // 앨리트 적인 경우
    {
        AnimInstance->Montage_SetPlayRate(StrongAttackMontage, 1.5f); // 몽타주 배속
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
    //UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
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
    if (AnimInstance)
    {
        AnimInstance->SetRootMotionMode(bEnable ? ERootMotionMode::RootMotionFromMontagesOnly : ERootMotionMode::NoRootMotionExtraction);
    }
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
    UWorld* World = GetWorld();
    if (!World) return;
    if (!AnimInstance || JumpAttackMontages.Num() == 0) return;

    //UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

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
        if (bIsEliteEnemy && AnimInstance && SelectedMontage) // 앨리트 적인 경우
        {
            AnimInstance->Montage_SetPlayRate(SelectedMontage, 1.5f); // 몽타주 배속
        }

        // 공격 실행 후 AI 이동 정지
        // AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());
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
    UWorld* World = GetWorld();
    if (!World) return;
    if (bIsDead || bIsInAirStun) return;
    UE_LOG(LogTemp, Warning, TEXT("Entering InAirStunState..."));

    bIsInAirStun = true;

    // AI 멈추기 (바로 이동 정지하지 않고, 스턴 종료 시점에서 다시 활성화)
    //AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());
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

    // 타이머 선언
    FTimerHandle GravityDisableHandle;
    // 일정 시간 후 중력 제거 (즉시 0으로 만들면 착지가 방해될 수 있음)
    World->GetTimerManager().SetTimer(
        GravityDisableHandle,
        [WeakThis = TWeakObjectPtr<AEnemy>(this)]() // 약참조 적용
        {
            if (WeakThis.IsValid())
            {
                if (auto* MC = WeakThis->GetCharacterMovement())
                {
                    MC->SetMovementMode(MOVE_Flying);
                    MC->GravityScale = 0.0f;
                    MC->Velocity = FVector::ZeroVector;
                }
            }
        },
        0.3f,
        false
    );

    // 스턴 애니메이션 실행
    if (AnimInstance && InAirStunMontage)
    {
        AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
    }

    // 일정 시간이 지나면 원래 상태로 복귀
    World->GetTimerManager().SetTimer(StunTimerHandle, this, &AEnemy::ExitInAirStunState, Duration, false);
    UE_LOG(LogTemp, Warning, TEXT("Enemy %s is now stunned for %f seconds!"), *GetName(), Duration);
}

void AEnemy::ExitInAirStunState()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (bIsDead) return;
    UE_LOG(LogTemp, Warning, TEXT("Exiting InAirStunState..."));

    bIsInAirStun = false;

    //UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp)
    {
        MoveComp->SetMovementMode(MOVE_Falling); // 낙하 상태로 변경
        MoveComp->GravityScale = 1.5f; // 조금 더 빠르게 낙하
    }

    ApplyBaseWalkSpeed();

    // AI 이동 다시 활성화
    //AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController());
    if(AICon)
    {
        UE_LOG(LogTemp, Warning, TEXT("Reactivating AI movement..."));
        MoveComp->SetMovementMode(MOVE_NavWalking);
        MoveComp->SetDefaultMovementMode();
        // 다시 이동 시작
        AICon->MoveToActor(UGameplayStatics::GetPlayerCharacter(World, 0));
    }

    // 애니메이션 정지
    if (AnimInstance)
    {
        AnimInstance->Montage_Stop(0.1f);
    }

    UE_LOG(LogTemp, Warning, TEXT("Enemy %s has recovered from stun and resumed AI behavior!"), *GetName());
    bIsInAirStun = false;
}

void AEnemy::EnableGravityPull(FVector ExplosionCenter, float PullStrength)
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (bIsDead) return; // 죽은 적은 끌어당기지 않음

    // 중력장 상태 업데이트
    bIsTrappedInGravityField = true;
    GravityFieldCenter = ExplosionCenter;

    // AI·이동 전면 차단
    if (AICon)
    {
        AICon->StopMovement();
        AICon->SetActorTickEnabled(false); // AI Tick 완전 중지
    }

    //UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp)
    {
        MoveComp->StopMovementImmediately();
        MoveComp->DisableMovement(); // AI 네비게이션 이동 막음
        MoveComp->SetMovementMode(MOVE_Flying); // 중심 고정에 유리
        MoveComp->Velocity = FVector::ZeroVector;
    }

    // 중력장 중앙으로 강제 이동 로직
    FVector Direction = GravityFieldCenter - GetActorLocation();
    float Distance = Direction.Size();

    // 너무 중앙에 가까우면 바로 가운데 고정
    if (Distance < 50.f)
    {
        SetActorLocation(GravityFieldCenter, true);
    }
    else
    {
        Direction.Normalize();
        float PullSpeed = PullStrength * 10.f; // 강도 강화
        FVector NewLocation = GetActorLocation() + Direction * PullSpeed * World->GetDeltaSeconds();
        SetActorLocation(NewLocation, true);
    }
}

void AEnemy::DisableGravityPull()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (!bIsTrappedInGravityField) return;

    bIsTrappedInGravityField = false;

    // 이동 모드 복구
    ApplyBaseWalkSpeed();

    //UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp)
    {
        MoveComp->SetMovementMode(MOVE_NavWalking);
        MoveComp->SetDefaultMovementMode();
        MoveComp->StopMovementImmediately();
    }

    if (AICon)
    {
        AICon->MoveToActor(UGameplayStatics::GetPlayerCharacter(World, 0));
        AICon->SetActorTickEnabled(true); // AI Tick 완전 중지
    }

    UE_LOG(LogTemp, Warning, TEXT("Enemy %s has been released from gravity field"), *GetName());
}

void AEnemy::StartAttack(EAttackType AttackType)
{
    if (EquippedKatana)
    {
        EquippedKatana->EnableAttackHitDetection(AttackType);
    }
}

void AEnemy::EndAttack()
{
    if (EquippedKatana)
    {
        EquippedKatana->DisableAttackHitDetection();
    }
}
