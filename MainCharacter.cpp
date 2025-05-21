#include "MainCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomAnimInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "Rifle.h"
#include "Knife.h"
#include "Enemy.h" // Enemy 헤더 추가
#include "Skill3Projectile.h" // 스킬3 투사체 헤더 추가
#include "MachineGun.h" // 머신건 헤더 추가
#include "Cannon.h"
#include "Kismet/GameplayStatics.h"


AMainCharacter::AMainCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Camera Boom (Spring Arm)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetRootComponent());
    CameraBoom->TargetArmLength = 250.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f);
    CameraBoom->bUsePawnControlRotation = true;

    CameraBoom->bDoCollisionTest = false;

    // Camera Component
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;

    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

    Speed = 0.0f;
    Direction = 0.0f;
    AimPitch = 0.0f;
    bIsJumping = false;
    bIsInDoubleJump = false;
    bIsInAir = false;
    bCanDoubleJump = true;
    bIsAiming = false;

    // 발차기 히트박스 초기화
    KickHitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("KickHitBox"));
    KickHitBox->SetupAttachment(GetMesh(), TEXT("FootSocket_R")); // 발 위치에 부착
    KickHitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    KickHitBox->SetGenerateOverlapEvents(true);
    KickHitBox->SetCollisionObjectType(ECC_WorldDynamic);  // 충돌 오브젝트 타입 설정
    KickHitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    KickHitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 캐릭터와만 충돌 감지

    MeleeCombatComponent = CreateDefaultSubobject<UMeleeCombatComponent>(TEXT("MeleeCombatComponent"));

    SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));

}

void AMainCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            SubSystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    if (RifleClass)
    {
        Rifle = GetWorld()->SpawnActor<ARifle>(RifleClass);
        if (Rifle)
        {
            Rifle->SetOwner(this);
            AttachRifleToBack();
        }
    }

    if (KnifeClass_L)
    {
        LeftKnife = GetWorld()->SpawnActor<AKnife>(KnifeClass_L);
        if (LeftKnife)
        {
            LeftKnife->InitializeKnife(EKnifeType::Left);
            LeftKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("KnifeSocket_L"));

            LeftKnife->SetOwner(this);
            UE_LOG(LogTemp, Warning, TEXT("LeftKnife spawned,attached and owner set!"));
        }
    }

    if (KnifeClass_R)
    {
        RightKnife = GetWorld()->SpawnActor<AKnife>(KnifeClass_R);
        if (RightKnife)
        {
            RightKnife->InitializeKnife(EKnifeType::Right);
            RightKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("KnifeSocket_R"));

            RightKnife->SetOwner(this);
            UE_LOG(LogTemp, Warning, TEXT("RightKnife spawned,attached and owner set!"));
        }
    }

    if (MeleeCombatComponent)
    {
        MeleeCombatComponent->InitializeCombatComponent(this, KickHitBox, LeftKnife, RightKnife);

        TArray<UAnimMontage*> Montages;
        Montages.Add(ComboAttackMontage1);
        Montages.Add(ComboAttackMontage2);
        Montages.Add(ComboAttackMontage3);
        Montages.Add(ComboAttackMontage4);
        Montages.Add(ComboAttackMontage5);

        MeleeCombatComponent->SetComboMontages(Montages);
        MeleeCombatComponent->SetJumpAttackMontages(JumpAttackMontage, DoubleJumpAttackMontage);
    }

    if (MachineGunClass)
    {
        MachineGun = GetWorld()->SpawnActor<AMachineGun>(MachineGunClass);
        if (MachineGun)
        {
            MachineGun->SetActorHiddenInGame(true); // 초기엔 숨겨놓기
            MachineGun->SetOwner(this);
        }
    }

    if (CannonClass)
    {
        Cannon = GetWorld()->SpawnActor<ACannon>(CannonClass);
        if (Cannon)
        {
            Cannon->SetActorHiddenInGame(true); // 초기엔 숨겨놓기
            Cannon->SetOwner(this);
        }
    }

    if (SkillComponent)
    {
        SkillComponent->InitializeSkills(this, MachineGun, LeftKnife, RightKnife, KickHitBox, Cannon);
    }
}

void AMainCharacter::AttachRifleToBack()
{
    if (Rifle)
    {
        Rifle->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("BackGunSocket"));
        Rifle->SetActorRelativeLocation(FVector(5.0f, 15.0f, -9.0f));
        Rifle->SetActorRelativeRotation(FRotator(0.0f, -45.0f, 0.0f));

        // 총기의 소유자를 캐릭터로 설정 
        Rifle->SetOwner(this);
        UE_LOG(LogTemp, Warning, TEXT("Rifle Owner Set to: %s"), *GetName());
    }
}

void AMainCharacter::AttachRifleToHand()
{
    if (Rifle)
    {
        Rifle->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("GunSocket"));
        Rifle->SetActorRelativeLocation(FVector::ZeroVector);
        Rifle->SetActorRelativeRotation(FRotator::ZeroRotator);

        // 총기의 소유자를 캐릭터로 설정
        Rifle->SetOwner(this);
        UE_LOG(LogTemp, Warning, TEXT("Rifle Owner Set to: %s"), *GetName());
    }
}

void AMainCharacter::AttachKnifeToBack()
{
    if (IsValid(LeftKnife))
    {
        LeftKnife->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        LeftKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("BackKnifeSocket_L"));
        LeftKnife->SetActorRelativeLocation(FVector(-3.0f, 13.5f, 0.0f));
        LeftKnife->SetActorRelativeRotation(FRotator(45.0f, 0.0f, 0.0f));

        UE_LOG(LogTemp, Warning, TEXT("LeftKnife moved to BackKnifeSocket_L!"));

        // 칼이 제대로 붙었는지 검증
        if (LeftKnife->IsAttachedTo(GetMesh()->GetOwner()))
        {
            UE_LOG(LogTemp, Warning, TEXT("LeftKnife is successfully attached to BackKnifeSocket_L!"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("LeftKnife failed to attach to BackKnifeSocket_L!"));
        }
    }

    if (IsValid(RightKnife))
    {
        RightKnife->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        RightKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("BackKnifeSocket_R"));
        RightKnife->SetActorRelativeLocation(FVector(5.0f, 13.5f, 0.0f));
        RightKnife->SetActorRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

        UE_LOG(LogTemp, Warning, TEXT("RightKnife moved to BackKnifeSocket_R!"));

        if (RightKnife->IsAttachedTo(GetMesh()->GetOwner()))
        {
            UE_LOG(LogTemp, Warning, TEXT("RightKnife is successfully attached to BackKnifeSocket_R!"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RightKnife failed to attach to BackKnifeSocket_R!"));
        }
    }
}

void AMainCharacter::AttachKnifeToHand()
{
    if (LeftKnife)
    {
        LeftKnife->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        LeftKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("KnifeSocket_L"));
        LeftKnife->SetActorRelativeLocation(FVector::ZeroVector);
        LeftKnife->SetActorRelativeRotation(FRotator::ZeroRotator);

        LeftKnife->SetActorHiddenInGame(false);

        UE_LOG(LogTemp, Warning, TEXT("LeftKnife moved to hand and visible!"));
    }

    if (RightKnife)
    {
        RightKnife->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        RightKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("KnifeSocket_R"));
        RightKnife->SetActorRelativeLocation(FVector::ZeroVector);
        RightKnife->SetActorRelativeRotation(FRotator::ZeroRotator);

        RightKnife->SetActorHiddenInGame(false);

        UE_LOG(LogTemp, Warning, TEXT("RightKnife moved to hand and visible!"));
    }
}

void AMainCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FVector Velocity = GetVelocity();
    Speed = Velocity.Size();

    if (!Velocity.IsNearlyZero())
    {
        FVector ForwardVector = GetActorForwardVector();
        FVector RightVector = GetActorRightVector();
        FVector NormalizedVelocity = Velocity.GetSafeNormal();

        float AngleRadians = FMath::Atan2(FVector::DotProduct(NormalizedVelocity, RightVector),
            FVector::DotProduct(NormalizedVelocity, ForwardVector));
        Direction = FMath::RadiansToDegrees(AngleRadians);
    }
    else if (bIsInAir)
    {
        Direction = Direction;
    }
    else
    {
        Direction = 0.0f;
    }

    if (GetCharacterMovement()->IsFalling())
    {
        bIsInAir = true;
    }
    else if (GetCharacterMovement()->IsMovingOnGround())
    {
        bIsInAir = false;
        bIsJumping = false;
        bIsInDoubleJump = false;
        bCanDoubleJump = true;
    }

    if (UCustomAnimInstance* AnimInstance = Cast<UCustomAnimInstance>(GetMesh()->GetAnimInstance()))
    {
        AnimInstance->Speed = Speed;
        AnimInstance->Direction = Direction;
        AnimInstance->bIsJumping = bIsJumping;
        AnimInstance->bIsInDoubleJump = bIsInDoubleJump;
        AnimInstance->bIsInAir = bIsInAir;
        AnimInstance->bIsAiming = bIsAiming;
        AnimInstance->AimPitch = AimPitch; // Pitch 값 전달
        AnimInstance->bIsUsingAimSkill1 = (SkillComponent && SkillComponent->IsUsingAimSkill1());
        AnimInstance->bIsUsingAimSkill2 = (SkillComponent && SkillComponent->IsUsingAimSkill2());
    }

    if (bIsAiming || (SkillComponent && SkillComponent->IsUsingAimSkill1() || (SkillComponent && SkillComponent->IsUsingAimSkill2())))
    {
        APlayerController* PlayerController = Cast<APlayerController>(GetController());
        if (PlayerController)
        {
            FRotator ControlRotation = PlayerController->GetControlRotation();
            AimPitch = FMath::Clamp(FMath::UnwindDegrees(ControlRotation.Pitch), -90.0f, 90.0f);

            // 대쉬 중이거나 점프중이거나 착지중이라면 회전하지 않음
            if (!bIsDashing && !bIsJumping && !bIsInDoubleJump && !bIsLanding)
            {
                FVector LastInput = GetCharacterMovement()->GetLastInputVector();

                // 이동 중이면 카메라 방향을 따라 캐릭터 회전   
                if (!LastInput.IsNearlyZero())
                {
                    FRotator NewRotation(0.0f, ControlRotation.Yaw, 0.0f);
                    SetActorRotation(NewRotation);
                }
                // 이동 입력이 없더라도 카메라 방향을 따라 캐릭터 회전
                else
                {
                    FRotator NewRotation(0.0f, ControlRotation.Yaw, 0.0f);
                    SetActorRotation(NewRotation);
                }
            }
        }

        CurrentZoom = FMath::FInterpTo(CurrentZoom, AimZoom, DeltaTime, ZoomInterpSpeed); // 에임모드줌 보간
    }
    else
    {
        CurrentZoom = FMath::FInterpTo(CurrentZoom, TargetZoom, DeltaTime, ZoomInterpSpeed); // 기본모드줌 보간
    }

    CameraBoom->TargetArmLength = CurrentZoom; // 줌 값 적용

    if (bApplyRootMotionRotation)
    {
        UE_LOG(LogTemp, Warning, TEXT("Root Motion Applied! Rotation Set To: %s"), *TargetRootMotionRotation.ToString());
        SetActorRotation(TargetRootMotionRotation); // 루트모션이 적용되는 동안 방향유지 
    }
}

void AMainCharacter::HandleJump()
{
    if (SkillComponent &&
        (SkillComponent->IsUsingSkill1() || SkillComponent->IsUsingSkill2() || SkillComponent->IsUsingSkill3() ||
            SkillComponent->IsUsingAimSkill1() || SkillComponent->IsUsingAimSkill2()))
        return;

    // 공격 중이거나 대쉬중에 점프 불가
    if (bIsDashing)
    {
        return;
    }

    if (!bIsJumping)
    {
        Jump();
        bIsJumping = true;
    }
    else if (bCanDoubleJump)
    {
        HandleDoubleJump();
    }
}

void AMainCharacter::HandleDoubleJump()
{
    LaunchCharacter(FVector(0, 0, 1000), false, true);
    bCanDoubleJump = false;
    bIsInDoubleJump = true;

    // 더블 점프 후 중력 강화 (빠르게 착지)
    GetCharacterMovement()->GravityScale = 2.3f;

    // 착지 속도 증가 (낙하 중 마찰력 감소)
    GetCharacterMovement()->FallingLateralFriction = 0.1f;  // 기본값은 0.5~1.0, 낮출수록 빨리 떨어짐

    if (UCustomAnimInstance* AnimInstance = Cast<UCustomAnimInstance>(GetMesh()->GetAnimInstance()))
    {
        AnimInstance->bIsInDoubleJump = true;
    }
}

void AMainCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    // 착지 상태 활성화
    bIsLanding = true;

    // 착지 후 설정한 시간 뒤에 상태 초기화
    GetWorldTimerManager().SetTimer(LandingTimerHandle, this, &AMainCharacter::ResetLandingState, 0.5f, false);

    // 착지 시 중력을 원래대로 복구
    GetCharacterMovement()->GravityScale = 1.0f;

    // 낙하 속도 원래대로 복구
    GetCharacterMovement()->FallingLateralFriction = 0.5f;  // 기본값 복구

    UE_LOG(LogTemp, Warning, TEXT("Landed! Gravity & Falling Speed Reset. Character Rotation: %s"), *GetActorRotation().ToString());
}

void AMainCharacter::ResetLandingState()
{
    bIsLanding = false;
    UE_LOG(LogTemp, Warning, TEXT("Landing State Reset!"));
}

void AMainCharacter::Move(const FInputActionValue& Value)
{
    if (SkillComponent && SkillComponent->IsUsingAimSkill1()) return;

    FVector2D MovementVector = Value.Get<FVector2D>();

    if (!Controller) return;

    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDirection, MovementVector.Y);
    AddMovementInput(RightDirection, MovementVector.X);
}

void AMainCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookVector = Value.Get<FVector2D>();

    if (!Controller) return;

    AddControllerYawInput(LookVector.X);
    AddControllerPitchInput(LookVector.Y);
}

void AMainCharacter::FireWeapon()
{
    if (SkillComponent &&
        (SkillComponent->IsUsingSkill1() ||
            SkillComponent->IsUsingSkill2() ||
            SkillComponent->IsUsingSkill3() ||
            SkillComponent->IsUsingAimSkill1()))
        return;


    if (bIsAiming && Rifle)
    {
        Rifle->Fire();
    }
    else
    {
        // 에임 모드가 아닐 때 콤보 공격을 시작
        ComboAttack();
    }
}

void AMainCharacter::ReloadWeapon()
{
    if (Rifle)
    {
        Rifle->Reload();
    }
}

void AMainCharacter::EnterAimMode()
{
    if (SkillComponent &&
        (SkillComponent->IsUsingSkill1() ||
            SkillComponent->IsUsingSkill2() ||
            SkillComponent->IsUsingSkill3() ||
            SkillComponent->IsUsingAimSkill1() ||
            SkillComponent->IsUsingAimSkill2()))
        return;

    if (!bIsAiming)
    {
        bIsAiming = true;
        TargetZoom = AimZoom; // 에임 모드 진입 시 즉시 줌 변경
        UE_LOG(LogTemp, Warning, TEXT("Entered Aim Mode"));
        CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f);

        AttachRifleToHand(); // 손으로 이동
        AttachKnifeToBack();
    }
}

void AMainCharacter::ExitAimMode()
{
    if (bIsAiming)
    {
        bIsAiming = false;
        TargetZoom = DefaultZoom; // 기본 줌 값으로 복귀
        UE_LOG(LogTemp, Warning, TEXT("Exited Aim Mode"));
        CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f);

        AttachRifleToBack(); // 다시 등에 이동
        AttachKnifeToHand();
    }
}

void AMainCharacter::ComboAttack()
{
    if (!MeleeCombatComponent || !SkillComponent) return;

    if (bIsDashing || SkillComponent->IsUsingAimSkill1())
    {
        return;
    }

    // 이미 공격 중이면 막기
    if (MeleeCombatComponent->IsAttacking())
    {
        return;
    }

    MeleeCombatComponent->TriggerComboAttack();
}

void AMainCharacter::Dash()
{
    if (!bCanDash || bIsDashing || !Controller || bIsJumping || bIsInDoubleJump) return;

    FVector DashDirection = FVector::ZeroVector;
    FVector InputVector = GetCharacterMovement()->GetLastInputVector();

    // 이동 입력이 있다면 그대로 대시 방향으로 설정
    if (!InputVector.IsNearlyZero())
    {
        DashDirection = InputVector.GetSafeNormal();
    }
    else
    {
        // 입력이 없으면 현재 캐릭터가 바라보는 방향으로 대시
        DashDirection = GetActorForwardVector();
    }

    // 에임 모드 여부에 따라 ForwardVector, RightVector를 다르게 설정
    FVector ForwardVector;
    FVector RightVector;

    if (bIsAiming)
    {
        // 에임 모드일 때: 캐릭터의 현재 회전을 기준으로 설정
        ForwardVector = GetActorForwardVector();
        RightVector = GetActorRightVector();
    }
    else
    {
        // 일반 모드일 때: 컨트롤러(카메라) 방향을 기준으로 설정
        FRotator ControlRotation = Controller->GetControlRotation();
        FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

        ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
    }

    DashDirection.Normalize();

    float ForwardDot = FVector::DotProduct(DashDirection, ForwardVector);
    float RightDot = FVector::DotProduct(DashDirection, RightVector);

    // 방향을 확인하는 로그 
    UE_LOG(LogTemp, Warning, TEXT("DashDirection: %s"), *DashDirection.ToString());
    UE_LOG(LogTemp, Warning, TEXT("ForwardVector: %s, RightVector: %s"), *ForwardVector.ToString(), *RightVector.ToString());
    UE_LOG(LogTemp, Warning, TEXT("ForwardDot: %f, RightDot: %f"), ForwardDot, RightDot);

    UAnimMontage* DashMontage = nullptr;

    // 대쉬 판별 로직 
    if (ForwardDot > 0.8f) //앞으로 대쉬
    {
        DashMontage = ForwardDashMontage;
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: ForwardDashMontage"));
    }
    else if (ForwardDot > 0.6f && RightDot > 0.6f) //오른쪽 앞으로 대쉬
    {
        DashMontage = RightDashMontage;
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: RightDashMontage (Front-Right)"));
    }
    else if (ForwardDot > 0.6f && RightDot < -0.6f) //왼쪽 앞으로 대쉬
    {
        DashMontage = LeftDashMontage;
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: LeftDashMontage (Front-Left)"));
    }
    else if (ForwardDot < -0.3f) //뒤로 대쉬
    {
        DashMontage = BackwardDashMontage;
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: BackwardDashMontage"));
    }
    else if (RightDot > 0.5f) //오른쪽으로 대쉬
    {
        DashMontage = RightDashMontage;
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: RightDashMontage"));
    }
    else if (RightDot < -0.5f) //왼쪽으로 대쉬
    {
        DashMontage = LeftDashMontage;
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: LeftDashMontage"));
    }

    if (!DashMontage)
    {
        UE_LOG(LogTemp, Error, TEXT("Dash Failed: No valid DashMontage found!"));
        return;
    }

    // 회전 적용 (DotProduct 계산 이후에 적용으로 정확한 방향 비교)
    SetActorRotation(DashDirection.Rotation());

    // 대시 실행
    bIsDashing = true;
    bCanDash = false;

    PlayDashMontage(DashMontage);

    GetWorldTimerManager().SetTimer(DashCooldownTimerHandle, this, &AMainCharacter::ResetDashCooldown, DashCooldown, false);
}

void AMainCharacter::PlayDashMontage(UAnimMontage* DashMontage)
{
    if (!DashMontage) // 애니메이션이 없으면 실행 취소
    {
        UE_LOG(LogTemp, Error, TEXT("PlayDashMontage Failed: DashMontage is NULL"));
        return;
    }

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 캐릭터와 애님 인스턴스를 가져옴
    if (!AnimInstance) // 애님 인스턴스가 없으면 실행 취소
    {
        UE_LOG(LogTemp, Error, TEXT("PlayDashMontage Failed: AnimInstance is NULL"));
        return;
    }

    float MontageDuration = AnimInstance->Montage_Play(DashMontage, 1.3f); // 대쉬 애니메이션 실행 (1.3배속)
    if (MontageDuration <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Montage_Play Failed: %s"), *DashMontage->GetName()); // 애니메이션 실행 실패 로그
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Montage_Play Success: %s"), *DashMontage->GetName()); // 애니메이션 실행 성공 로그

    // 애니메이션 종료 시 대시 상태를 초기화하도록 콜백 함수 설정
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &AMainCharacter::ResetDash);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, DashMontage);
}

void AMainCharacter::ResetDash(UAnimMontage* Montage, bool bInterrupted) // 대쉬 상태 초기화
{
    bIsDashing = false;
    // 대시 종료 시 에임 모드 상태라면 컨트롤러 방향을 바라보게 조정
    if (bIsAiming)
    {
        if (Controller)
        {
            FRotator ControlRotation = Controller->GetControlRotation();
            FRotator NewRotation(0.0f, ControlRotation.Yaw, 0.0f);
            SetActorRotation(NewRotation);

            UE_LOG(LogTemp, Warning, TEXT("Dash Reset! Character Rotated to Aim Direction."));
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Dash Reset! Ready for Next Dash."));
}

void AMainCharacter::ResetDashCooldown() // 대쉬 쿨타임 해제
{
    bCanDash = true;
    UE_LOG(LogTemp, Warning, TEXT("Dash Cooldown Over: Ready to Dash Again."));
}

void AMainCharacter::ZoomIn()
{
    if (bIsAiming) return; // 에임 모드일 때는 줌 불가능

    TargetZoom = FMath::Clamp(TargetZoom - ZoomStep, MinZoom, MaxZoom);
    UE_LOG(LogTemp, Warning, TEXT("Zoom In: %f"), CurrentZoom);
}

void AMainCharacter::ZoomOut()
{
    if (bIsAiming) return; // 에임 모드일 때는 줌 불가능

    TargetZoom = FMath::Clamp(TargetZoom + ZoomStep, MinZoom, MaxZoom);
    UE_LOG(LogTemp, Warning, TEXT("Zoom Out: %f"), CurrentZoom);
}

void AMainCharacter::UseSkill1()
{
    UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::UseSkill1 triggered"));
    if (SkillComponent)
    {
        if (bIsAiming && SkillComponent->CanUseAimSkill1()) // 에임 중이고, 에임스킬1 사용 가능하면
        {
            UE_LOG(LogTemp, Warning, TEXT("UseAimSkill1"));
            SkillComponent->UseAimSkill1(); // 에임 스킬1
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UseSkill1"));
            SkillComponent->UseSkill1(); // 일반 스킬1
        }
    }
}

void AMainCharacter::UseSkill2()
{
    UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::UseSkill2 triggered"));
    if (SkillComponent)
    {
        if (bIsAiming && SkillComponent->CanUseAimSkill2()) // 에임 중이고, 에임스킬2 사용 가능하면
        {
            UE_LOG(LogTemp, Warning, TEXT("UseAimSkill2"));
            SkillComponent->UseAimSkill2(); // 에임 스킬2
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UseSkill2"));
            SkillComponent->UseSkill2(); // 일반 스킬2
        }
    }
}

void AMainCharacter::UseSkill3()
{
    UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::UseSkill3 triggered"));
    if (SkillComponent)
    {
        if (bIsAiming && SkillComponent->CanUseAimSkill3()) // 에임 중이고, 에임스킬3 사용 가능하면
        {
            UE_LOG(LogTemp, Warning, TEXT("UseAimSkill3"));
            SkillComponent->UseAimSkill3(); // 에임 스킬3
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UseSkill3"));
            SkillComponent->UseSkill3(); // 일반 스킬3
        }
    }
}

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMainCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMainCharacter::Look);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMainCharacter::HandleJump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AMainCharacter::EnterAimMode);
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AMainCharacter::ExitAimMode);
        EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &AMainCharacter::FireWeapon);
        EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &AMainCharacter::ReloadWeapon);
        EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &AMainCharacter::Dash);
        EnhancedInputComponent->BindAction(ZoomInAction, ETriggerEvent::Triggered, this, &AMainCharacter::ZoomIn);
        EnhancedInputComponent->BindAction(ZoomOutAction, ETriggerEvent::Triggered, this, &AMainCharacter::ZoomOut);
        EnhancedInputComponent->BindAction(Skill1Action, ETriggerEvent::Triggered, this, &AMainCharacter::UseSkill1);
        EnhancedInputComponent->BindAction(Skill2Action, ETriggerEvent::Triggered, this, &AMainCharacter::UseSkill2);
        EnhancedInputComponent->BindAction(Skill3Action, ETriggerEvent::Triggered, this, &AMainCharacter::UseSkill3);
    }
}