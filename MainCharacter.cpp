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
#include "LockOnComponent.h"
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
    ComboIndex = 0;

    // 발차기 히트박스 초기화
    KickHitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("KickHitBox"));
    KickHitBox->SetupAttachment(GetMesh(), TEXT("FootSocket_R")); // 발 위치에 부착
    KickHitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    KickHitBox->SetGenerateOverlapEvents(true);
    KickHitBox->SetCollisionObjectType(ECC_WorldDynamic);  // 충돌 오브젝트 타입 설정
    KickHitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    KickHitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 캐릭터와만 충돌 감지

    LockOnComponent = CreateDefaultSubobject<ULockOnSystem>(TEXT("LockOnComponent"));
}

void AMainCharacter::BeginPlay()
{
    Super::BeginPlay();

    KickHitBox->OnComponentBeginOverlap.AddDynamic(this, &AMainCharacter::OnKickHitBoxOverlap);


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

    // LockOnComponent가 nullptr이면 직접 추가
    if (!LockOnComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("LockOnComponent가 nullptr! Can not run Lock-On"));

        // LockOnComponent가 없다면 새로 추가
        LockOnComponent = NewObject<ULockOnSystem>(this, ULockOnSystem::StaticClass()); // 런타임에 락온컴포넌트 동적생성 및 현재캐릭터에 속하도록 설정
        if (LockOnComponent)
        {
            LockOnComponent->RegisterComponent(); // 생성한 락온컴포넌트를 게임에서 사용할 수 있도록 등록
            UE_LOG(LogTemp, Warning, TEXT("LockOnComponent Added to Runtime"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("LockOnComponent Generate Failed!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("LockOnComponent Initialization Compelete!"));
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
    }

    if (bIsAiming)
    {
        APlayerController* PlayerController = Cast<APlayerController>(GetController());
        if (PlayerController)
        {
            FRotator ControlRotation = PlayerController->GetControlRotation();
            AimPitch = ControlRotation.Pitch;

            // AimPitch 값을 -180~180 범위로 변환
            AimPitch = FMath::UnwindDegrees(AimPitch);

            // Clamp로 AimPitch 제한 (-90도에서 90도 사이)
            AimPitch = FMath::Clamp(AimPitch, -90.0f, 90.0f);

            // 디버깅 메시지 출력
            if (PlayerController->IsInputKeyDown(EKeys::RightMouseButton))
            {
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(
                        -1,
                        0.1f,
                        FColor::Yellow,
                        FString::Printf(TEXT("Clamped AimPitch: %.2f"), AimPitch)
                    );
                }
            }
        }

        // 캐릭터의 회전 유지
        FRotator ControlRotation = GetControlRotation();
        FRotator NewRotation(0.0f, ControlRotation.Yaw, 0.0f);
        SetActorRotation(NewRotation);
    }

    if (bIsLockedOn && LockOnComponent->IsLockedOn())
    {
        //UE_LOG(LogTemp, Warning, TEXT("Maintaing Lock-On % s"), *LockOnComponent->GetLockedTarget()->GetName());
        LockOnComponent->UpdateLockOnRotation(DeltaTime);
    }
    //else
    //{
        //UE_LOG(LogTemp, Warning, TEXT("Lock-On Released!"));
    //}
}

void AMainCharacter::HandleJump()
{
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

    // 착지 시 중력을 원래대로 복구
    GetCharacterMovement()->GravityScale = 1.0f;

    // 낙하 속도 원래대로 복구
    GetCharacterMovement()->FallingLateralFriction = 0.5f;  // 기본값 복구

    UE_LOG(LogTemp, Warning, TEXT("Landed! Gravity & Falling Speed Reset"));
}


void AMainCharacter::Move(const FInputActionValue& Value)
{
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
    if (!bIsAiming)
    {
        bIsAiming = true;
        UE_LOG(LogTemp, Warning, TEXT("Entered Aim Mode"));
        AttachRifleToHand(); // 손으로 이동
        AttachKnifeToBack();
        CameraBoom->TargetArmLength = 100.0f;
        CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f);

        if (bIsLockedOn)  // 사격 모드 진입 시 락온 자동 해제
        {
            LockOnComponent->UnlockTarget();
            bIsLockedOn = false;
            UE_LOG(LogTemp, Warning, TEXT("Lock-On Automatically Released Due to Aiming"));
        }
    }
}

void AMainCharacter::ExitAimMode()
{
    if (bIsAiming)
    {
        bIsAiming = false;
        UE_LOG(LogTemp, Warning, TEXT("Exited Aim Mode"));
        AttachRifleToBack(); // 다시 등에 이동
        AttachKnifeToHand();
        CameraBoom->TargetArmLength = 250.0f;
        CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f);

    }
}

void AMainCharacter::ToggleLockOn()
{
    if (!LockOnComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("LockOnComponent nullptr! Can not run Lock-On"));
        return;
    }

    if (bIsAiming)
    {
        if (bIsLockedOn)
        {
            LockOnComponent->UnlockTarget();
            bIsLockedOn = false;
            UE_LOG(LogTemp, Warning, TEXT("Aim Mode Activated: Lock-On Automatically Released"));
        }
        return;
    }

    if (bIsLockedOn)
    {
        LockOnComponent->UnlockTarget();
        bIsLockedOn = false;
        //UE_LOG(LogTemp, Warning, TEXT("Lock-On Released"));
    }
    else
    {
        LockOnComponent->FindAndLockTarget();
        if (LockOnComponent->IsLockedOn())
        {
            bIsLockedOn = true;
            UE_LOG(LogTemp, Warning, TEXT("Lock-On Activated"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Lock-On failed: There is no Target to Lock-On"));
        }
    }
}

void AMainCharacter::ComboAttack()
{
    if (bIsAttacking) return; // 이미 공격 중이면 실행 안 함

    bIsAttacking = true; // 공격 상태 변경

    // 콤보 중 자동 회전 비활성화 (카메라 영향을 막음)
    GetCharacterMovement()->bOrientRotationToMovement = false;

    FVector InputDirection = FVector::ZeroVector;

    if (Controller)
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

        FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        if (GetCharacterMovement()->GetLastInputVector().Size() > 0.0f)
        {
            InputDirection = GetCharacterMovement()->GetLastInputVector().GetSafeNormal();
        }
    }

    // 이동 입력이 있다면 방향 업데이트
    if (!InputDirection.IsNearlyZero())
    {
        LastAttackDirection = InputDirection;
    }

    // 캐릭터가 바라보는 방향 설정
    FVector AttackDirection = GetActorForwardVector();

    // 이동 입력이 있었다면 방향 업데이트
    if (!LastAttackDirection.IsNearlyZero())
    {
        FRotator NewRotation = LastAttackDirection.Rotation();
        NewRotation.Pitch = 0.0f;
        SetActorRotation(NewRotation);

        AttackDirection = LastAttackDirection;
    }

    if (ComboIndex == 3)
    {
        EnableKickHitBox();
    }
    else
    {
        if (LeftKnife)
        {
            LeftKnife->EnableHitBox(ComboIndex);
        }
        if (RightKnife)
        {
            RightKnife->EnableHitBox(ComboIndex);
        }
    }
    // 콤보 공격 애니메이션 적용
    switch (ComboIndex)
    {
    case 0:
        PlayComboAttackAnimation1();
        break;

    case 1:
        PlayComboAttackAnimation2();
        break;

    case 2:
        PlayComboAttackAnimation3();
        break;

    case 3:
        PlayComboAttackAnimation4();
        break;

    case 4:
        PlayComboAttackAnimation5();
        break;

    default:
        break;
    }

    // 콤보 인덱스 업데이트
    ComboIndex = (ComboIndex + 1) % 5;
}


// 콤보 리셋 함수 (공격 후 콤보 상태 초기화)
void AMainCharacter::ResetCombo()
{
    bIsAttacking = false; // 공격 상태 초기화
}

void AMainCharacter::OnComboMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    bIsAttacking = false; // 공격 상태 초기화

    // 콤보 종료 후 자동 회전 다시 활성화
    GetCharacterMovement()->bOrientRotationToMovement = true;

    // 공격 종료 시 히트박스 강제 비활성화
    if (LeftKnife)
    {
        LeftKnife->DisableHitBox();
        UE_LOG(LogTemp, Warning, TEXT("LeftKnife HitBox Disabled at Montage End!"));
    }
    if (RightKnife)
    {
        RightKnife->DisableHitBox();
        UE_LOG(LogTemp, Warning, TEXT("RightKnife HitBox Disabled at Montage End!"));
    }

    // 마지막 공격 방향을 유지 (이전 방향을 덮어쓰지 않도록)
    if (!LastAttackDirection.IsNearlyZero())
    {
        LastAttackDirection = GetActorForwardVector();
    }
}


void AMainCharacter::ApplyComboMovement(float MoveDistance, FVector MoveDirection)
{
    if (!MoveDirection.IsNearlyZero())
    {
        MoveDirection.Z = 0; // 수직 이동 방지
        MoveDirection.Normalize();

        // 캐릭터를 입력 방향으로 즉시 이동
        LaunchCharacter(MoveDirection * MoveDistance, false, false);

        UE_LOG(LogTemp, Warning, TEXT("Character launched towards: %s"), *MoveDirection.ToString());
    }
}

void AMainCharacter::EnableKickHitBox()
{
    if (KickHitBox)
    {
        KickHitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        UE_LOG(LogTemp, Warning, TEXT("Kick HitBox Enabled!"));
    }
}


void AMainCharacter::DisableKickHitBox()
{
    if (KickHitBox)
    {
        KickHitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        UE_LOG(LogTemp, Warning, TEXT("Kick HitBox Disabled!"));
    }
}

void AMainCharacter::OnKickHitBoxOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    // 자기 자신과 충돌한 경우 무시
    if (!OtherActor || OtherActor == this || OtherComp == KickHitBox)
    {
        return;
    }

    float KickDamage = 35.0f;

    // ApplyDamage 호출
    UGameplayStatics::ApplyDamage(OtherActor, KickDamage, nullptr, this, nullptr);

    UE_LOG(LogTemp, Warning, TEXT("Kick Hit! Applied %f Damage to %s"), KickDamage, *OtherActor->GetName());

    DisableKickHitBox();
}




void AMainCharacter::PlayComboAttackAnimation1()
{
    if (ComboAttackMontage1)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

        if (AnimInstance)
        {
            AnimInstance->Montage_Play(ComboAttackMontage1, 1.0f);

            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &AMainCharacter::OnComboMontageEnded);
            AnimInstance->Montage_SetEndDelegate(EndDelegate, ComboAttackMontage1);
        }
    }
}

void AMainCharacter::PlayComboAttackAnimation2()
{
    if (ComboAttackMontage2)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

        if (AnimInstance)
        {
            AnimInstance->Montage_Play(ComboAttackMontage2, 1.0f);

            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &AMainCharacter::OnComboMontageEnded);
            AnimInstance->Montage_SetEndDelegate(EndDelegate, ComboAttackMontage2);
        }
    }
}

void AMainCharacter::PlayComboAttackAnimation3()
{
    if (ComboAttackMontage3)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

        if (AnimInstance)
        {
            AnimInstance->Montage_Play(ComboAttackMontage3, 1.0f);

            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &AMainCharacter::OnComboMontageEnded);
            AnimInstance->Montage_SetEndDelegate(EndDelegate, ComboAttackMontage3);
        }
    }
}

void AMainCharacter::PlayComboAttackAnimation4()
{
    if (ComboAttackMontage4)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

        if (AnimInstance)
        {
            AnimInstance->Montage_Play(ComboAttackMontage4, 1.0f);

            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &AMainCharacter::OnComboMontageEnded);
            AnimInstance->Montage_SetEndDelegate(EndDelegate, ComboAttackMontage4);
        }
    }
}

void AMainCharacter::PlayComboAttackAnimation5()
{
    if (ComboAttackMontage5)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

        if (AnimInstance)
        {
            AnimInstance->Montage_Play(ComboAttackMontage5, 1.0f);

            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &AMainCharacter::OnComboMontageEnded);
            AnimInstance->Montage_SetEndDelegate(EndDelegate, ComboAttackMontage5);
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
        EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Triggered, this, &AMainCharacter::ToggleLockOn);
    }
}