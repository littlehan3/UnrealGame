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
#include "CrossHairWidget.h"  // 헤더 추가
#include "Blueprint/UserWidget.h"
#include "SettingsSaveGame.h"
#include "SettingsGameInstance.h"
#include "SkillComponent.h"
#include "Animation/AnimInstance.h"
#include "MainGameModeBase.h"

AMainCharacter::AMainCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // 기본 이동속도 설정
    GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;

    // Camera Boom (Spring Arm)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetRootComponent());
    CameraBoom->TargetArmLength = 250.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f);
    CameraBoom->bUsePawnControlRotation = true;

    // [수정 1] 이 값을 'true'로 변경하여 카메라 충돌을 활성화합니다.
    CameraBoom->bDoCollisionTest = true; //true로 변경

    // [수정 2 - 권장] 충돌 테스트 시 얇은 선 대신 구체(Sphere)를 사용합니다.
    // 이렇게 하면 얇은 모서리 등을 '뚫고' 지나가는 현상을 방지할 수 있습니다.
    CameraBoom->ProbeSize = 12.0f; // (적절한 기본값)

    // [수정 3 - 권장] 충돌로 인해 카메라가 '순간이동'하는 것을 방지하고
    // 부드럽게 따라오도록 카메라 래그(Lag)를 활성화합니다.
    CameraBoom->bEnableCameraLag = true;

    CameraBoom->CameraLagSpeed = 10.0f; // (이 값을 조절하여 부드러운 정도를 설정)

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

    CurrentHealth = MaxHealth;
    bIsDead = false;
    bIsBigHitReacting = false;

    // 크로스헤어 컴포넌트 생성
    CrosshairComponent = CreateDefaultSubobject<UCrossHairComponent>(TEXT("CrosshairComponent"));

    // [추가] 쿨타임 사운드 재생용 컴포넌트 생성 (사운드 중복 방지용)
    CooldownAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("CooldownAudioComponent"));
    CooldownAudioComponent->SetupAttachment(GetRootComponent());
    CooldownAudioComponent->bAutoActivate = false; // 자동 재생 방지

    DeathPostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("DeathPostProcess"));
    DeathPostProcessComponent->SetupAttachment(RootComponent); // 캐릭터에 부착

    // 초기 설정: 기본적으로 이펙트의 가중치(Blend Weight)는 0으로 둡니다.
    // 죽을 때 1.0으로 올릴 것이기 때문입니다.
    DeathPostProcessComponent->BlendWeight = 0.0f;

    // 이 컴포넌트가 활성화되도록 설정
    DeathPostProcessComponent->bEnabled = true;
}

void AMainCharacter::ResumeGame()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    if (PauseMenuWidget)
    {
        // [수정] 위젯을 뷰포트에서 제거
        PauseMenuWidget->RemoveFromParent(); // <--- RemoveFromViewport() 대신 사용
    }

    // (중요) 게임 일시정지 해제
    PC->SetPause(false);

    // (중요) 마우스 커서 숨김
    PC->SetShowMouseCursor(false);

    // (중요) 입력 모드를 '게임 전용'으로 변경
    FInputModeGameOnly InputMode;
    PC->SetInputMode(InputMode);
}

void AMainCharacter::HandleRestartGame()
{
    // 현재 레벨 이름을 가져옵니다.
    FName CurrentLevelName = FName(UGameplayStatics::GetCurrentLevelName(this));

    // [수정] 공통 로딩 함수를 호출합니다.
    ShowLoadingScreenAndLoad(CurrentLevelName);
}

void AMainCharacter::HandleBackToMainMenu()
{
    // [수정] 공통 로딩 함수를 호출합니다. (MainMenuLevel 이름은 실제 이름으로 사용하세요)
    ShowLoadingScreenAndLoad(FName("MainMenutest"));
}

void AMainCharacter::PlayCooldownSound()
{
    if (!SkillCooldownSound || !CooldownAudioComponent) return;

    // 이미 재생 중이면 다시 재생하지 않고 함수 종료
    if (CooldownAudioComponent->IsPlaying())
    {
        return;
    }

    // 쿨타임 사운드 설정 및 재생
    CooldownAudioComponent->SetSound(SkillCooldownSound);
    CooldownAudioComponent->Play();

    UE_LOG(LogTemp, Warning, TEXT("Cooldown Sound Played."));
}

void AMainCharacter::PlayWavePrepareSound()
{
    if (WavePrepareSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), WavePrepareSound);
        UE_LOG(LogTemp, Warning, TEXT("Wave Prepare Sound Played."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WavePrepareSound is NULL! Cannot play prepare sound."));
    }
}

void AMainCharacter::GiveReward(float HealthAmount, int32 AmmoAmount)
{
    // 1. 체력 지급
    float HealthBefore = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth + HealthAmount, 0.0f, MaxHealth);

    if (CurrentHealth > HealthBefore)
    {
        UE_LOG(LogTemp, Warning, TEXT("Character gained %f Health. Current: %f/%f"),
            HealthAmount, CurrentHealth, MaxHealth);
    }

    // 2. 탄약 지급 (Rifle이 장착되어 있고 유효한 경우)
    if (Rifle)
    {
        Rifle->AddTotalAmmo(AmmoAmount);
        UE_LOG(LogTemp, Warning, TEXT("Character gained %d Ammo. Total: %d"),
            AmmoAmount, Rifle->GetTotalAmmo());
    }

    // 3. 보상 획득 사운드 재생
    if (RewardSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), RewardSound);
    }
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

    // 크로스헤어 위젯 생성
    if (CrossHairWidgetClass)
    {
        CrossHairWidget = CreateWidget<UCrossHairWidget>(GetWorld(), CrossHairWidgetClass);
        if (CrossHairWidget)
        {
            CrossHairWidget->AddToViewport();
            CrossHairWidget->SetVisibility(ESlateVisibility::Hidden);

            // 컴포넌트 참조 재확인 
            if (!CrossHairWidget->IsComponentValid())
            {
                CrossHairWidget->SetCrossHairComponentReference(CrosshairComponent);
            }
        }
    }

    // 플레이어 HUD 위젯 생성 
    if (PlayerHUDWidgetClass)
    {
        PlayerHUDWidget = CreateWidget<UUserWidget>(GetWorld(), PlayerHUDWidgetClass);

        if (PlayerHUDWidget)
        {
            PlayerHUDWidget->AddToViewport();
            // HUD는 기본적으로 보여야 하므로 Hidden 처리 안 함
        }
    }

    // [신규 추가] 일시정지 메뉴 위젯 생성 (하지만 뷰포트에는 추가하지 않음)
    if (PauseMenuWidgetClass)
    {
        // GetOwningController() 대신 GetController()를 사용합니다.
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (PC)
        {
            PauseMenuWidget = CreateWidget<UUserWidget>(PC, PauseMenuWidgetClass);
            // 위젯은 생성만 해두고, 뷰포트에 AddToViewport()는 하지 않습니다.
            // TogglePauseMenu()에서 필요할 때 추가합니다.
        }
    }

    // [신규 추가] 마우스 감도 설정 로드
    // 1. GameInstance를 가져옵니다.
    USettingsGameInstance* SettingsGI = Cast<USettingsGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

    // 2. GameInstance와 CurrentSettings가 유효한지 확인합니다.
    if (SettingsGI && SettingsGI->CurrentSettings)
    {
        // 3. GameInstance에 저장된 값을 캐릭터의 변수로 복사합니다.
        MouseSensitivityMultiplier = SettingsGI->CurrentSettings->MouseSensitivity;
    }

    if (GameStartSound)
    {
        // ImpactPoint 대신 캐릭터의 위치(GetActorLocation())에서 사운드를 재생합니다.
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(),
            GameStartSound,
            GetActorLocation() // 재생 위치를 캐릭터의 위치로 설정
        );
    }
}

void AMainCharacter::UpdateMovementSpeed()
{
    if (!GetCharacterMovement()) return;

    // 에임모드 또는 에임스킬 사용 중일 때 속도 감소
    if (bIsAiming ||
        (SkillComponent && (SkillComponent->IsUsingAimSkill1() || SkillComponent->IsUsingAimSkill2())))
    {
        GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed;
    }
    else
    {
        GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
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

    if (bIsBigHitReacting)
    {
        // 빅 히트 중에도 AimPitch는 업데이트 (카메라는 움직일 수 있으므로)
        if (bIsAiming || (SkillComponent && SkillComponent->IsUsingAimSkill1() || (SkillComponent && SkillComponent->IsUsingAimSkill2())))
        {
            APlayerController* PlayerController = Cast<APlayerController>(GetController());
            if (PlayerController)
            {
                FRotator ControlRotation = PlayerController->GetControlRotation();
                AimPitch = FMath::Clamp(FMath::UnwindDegrees(ControlRotation.Pitch), -90.0f, 90.0f);
            }
        }

        // 애님 인스턴스에 상태 전달
        if (UCustomAnimInstance* AnimInstance = Cast<UCustomAnimInstance>(GetMesh()->GetAnimInstance()))
        {
            // 빅히트 중에는 속도/방향을 강제로 0으로 설정
            AnimInstance->Speed = 0.0f;
            AnimInstance->Direction = 0.0f;
            AnimInstance->bIsJumping = bIsJumping;
            AnimInstance->bIsInDoubleJump = bIsInDoubleJump;
            AnimInstance->bIsInAir = bIsInAir;
            AnimInstance->bIsAiming = bIsAiming;
            AnimInstance->AimPitch = AimPitch;
            AnimInstance->bIsUsingAimSkill1 = (SkillComponent && SkillComponent->IsUsingAimSkill1());
            AnimInstance->bIsUsingAimSkill2 = (SkillComponent && SkillComponent->IsUsingAimSkill2());
        }
        return; // 나머지 틱 로직(이동, 줌, 반동) 중단
    }

    bool bCurrentlyFalling = GetCharacterMovement()->IsFalling();
    bool bCurrentlyOnGround = GetCharacterMovement()->IsMovingOnGround();
    //[수정] 이동 중 크로스헤어 확산 적용 로직(Tick 함수 후반부)
    // [수정] 크로스헤어 활성화 조건 (라이플 에임 또는 머신건 스킬)
    bool bIsCrosshairActive = bIsAiming || (SkillComponent && SkillComponent->IsUsingAimSkill1()); // [cite: 65, 210]

    if (bCurrentlyFalling && !bIsInAir)
    {
        bIsInAir = true;
    }
    else if (bCurrentlyOnGround && bIsInAir)
    {
        // 이미 Landed에서 처리했으므로 중복 처리 방지
        if (!bIsLanding)
        {
            bIsInAir = false;
            bIsJumping = false;
            bIsInDoubleJump = false;
            bCanDoubleJump = true;
        }
    }

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
        // 착지 애니메이션 상태도 전달 (필요시 CustomAnimInstance에 변수 추가)
        // AnimInstance->bIsPlayingLandingAnimation = bIsPlayingLandingAnimation;
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

    // 이동 중 크로스헤어 확산 적용
    if (bIsAiming && CrosshairComponent)
    {
        // 이동 중 크로스헤어 확산 증가
        float MovementSpeed = GetVelocity().Size();
        float MovementSpread = 0.0f;

        if (MovementSpeed > 50.0f) // 이동 중일 때
        {
            // 이동 속도에 비례하여 크로스헤어 확산 증가
            MovementSpread = FMath::Clamp(MovementSpeed / 600.0f * 20.0f, 0.0f, 30.0f);
        }

        CrosshairComponent->SetMovementSpread(MovementSpread);
        CrosshairComponent->SetCrosshairActive(true);

        // 반동 업데이트는 라이플 에임(bIsAiming) 중에만 작동
        if (bIsAiming && bIsRecoiling)
        {
            UpdateRecoil(DeltaTime);
        }
    }
    else
    {
        // 에임 모드가 아닐 때는 크로스헤어 비활성화
        if (CrosshairComponent)
        {
            CrosshairComponent->SetCrosshairActive(false);
        }
    }

    // 이동속도 업데이트
    UpdateMovementSpeed();
}

void AMainCharacter::HandleJump()
{
    // [신규] 빅 히트 중 행동 불가
    if (bIsBigHitReacting || bIsDead) return;

    if (SkillComponent &&
        (SkillComponent->IsUsingSkill1() || SkillComponent->IsUsingSkill2() || SkillComponent->IsUsingSkill3() ||
            SkillComponent->IsUsingAimSkill1() || SkillComponent->IsUsingAimSkill2() || SkillComponent->IsUsingAimSkill3()))
        return;
    if (bIsDead) return;

    // 대쉬 중이거나 착지 애니메이션 재생 중일 때 점프 불가
    if (bIsDashing || bIsPlayingLandingAnimation)
    {
        if (bIsPlayingLandingAnimation)
        {
            UE_LOG(LogTemp, Warning, TEXT("Jump Blocked - Landing animation is playing"));
        }
        return;
    }

    // 빠른 연타 방지: 이미 공중에 있고 아직 착지하지 않았다면 더블점프만 허용
    if (GetCharacterMovement()->IsFalling() && !bCanDoubleJump)
    {
        return;  // 이미 더블점프를 사용했다면 더 이상 점프 불가
    }

    if (!bIsJumping && GetCharacterMovement()->IsMovingOnGround())
    {
        LaunchCharacter(FVector(0, 0, 500), false, true);
        bIsJumping = true;
        bIsInAir = true;  // 즉시 공중 상태로 설정
    }
    else if (bCanDoubleJump && GetCharacterMovement()->IsFalling())
    {
        HandleDoubleJump();
    }
}


void AMainCharacter::HandleDoubleJump()
{
    // [신규] 빅 히트 중 행동 불가
    if (bIsBigHitReacting || bIsDead) return;

    if (bIsDead) return;

    LaunchCharacter(FVector(0, 0, 1200), false, true);
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

    // 즉시 점프 상태 초기화
    bIsJumping = false;
    bIsInDoubleJump = false;
    bIsInAir = false;
    bCanDoubleJump = true;

    // 착지 상태 활성화 (짧게 설정)
    bIsLanding = true;
    bIsPlayingLandingAnimation = true;  // 착지 애니메이션 재생 상태 활성화

    // 착지 애니메이션 지속시간 계산 (더블점프 착지인지 확인)
    float LandingAnimDuration = 0.5f;  // 기본 착지 애니메이션 시간

    // 더블점프에서 착지했다면 더 긴 애니메이션 시간 적용 (필요시)
    // if (bWasInDoubleJump) LandingAnimDuration = 0.7f;

    // 착지 애니메이션이 완전히 끝날 때까지 대기
    GetWorldTimerManager().SetTimer(LandingAnimationTimerHandle, this, &AMainCharacter::OnLandingAnimationFinished, LandingAnimDuration, false);

    // 착지 상태를 더 짧게 유지 (0.2초로 단축)
    GetWorldTimerManager().SetTimer(LandingTimerHandle, this, &AMainCharacter::ResetLandingState, 0.2f, false);

    // 중력과 낙하 속도 즉시 복구
    GetCharacterMovement()->GravityScale = 1.0f;
    GetCharacterMovement()->FallingLateralFriction = 0.5f;

    UE_LOG(LogTemp, Warning, TEXT("Landed! All jump states reset immediately"));
}

void AMainCharacter::OnLandingAnimationFinished()
{
    bIsPlayingLandingAnimation = false;
    UE_LOG(LogTemp, Warning, TEXT("Landing Animation Finished! Ready for normal jump"));
}

void AMainCharacter::TogglePauseMenu()
{
    // 1. 메인 메뉴 씬인지 확인
    FName CurrentLevelName = FName(UGameplayStatics::GetCurrentLevelName(this));
    if (CurrentLevelName == FName("MainMenutest"))
    {
        return; // 메인 메뉴에서는 일시정지 작동 안 함
    }

    // 2. PlayerController 가져오기
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    // 3. 위젯이 유효한지 확인
    if (!PauseMenuWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("PauseMenuWidget is not valid!"));
        return;
    }

    // 4. 이미 일시정지 상태인지 확인 (위젯이 뷰포트에 있거나, 게임이 Paused 상태)
    if (PC->IsPaused()) // 또는 if (PauseMenuWidget->IsInViewport())
    {
        // 이미 켜져있음 -> 게임 재개
        ResumeGame();
    }
    else
    {
        // 5. 일시정지 상태가 아님 -> 일시정지

        // 위젯을 뷰포트에 추가
        PauseMenuWidget->AddToViewport();

        // (중요) 게임을 일시정지시킴
        PC->SetPause(true);

        // (중요) 마우스 커서 표시
        PC->SetShowMouseCursor(true);

        // (중요) 입력 모드를 '게임 및 UI'로 변경
        // 이렇게 해야 UI 버튼도 클릭되고, ESC 키 입력도 계속 받을 수 있습니다.
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(PauseMenuWidget->GetCachedWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
    }
}

void AMainCharacter::ShowLoadingScreenAndLoad(FName LevelName)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    // 1. 일시정지 해제 및 커서 숨김 (커서는 UI모드에서 다시 처리)
    PC->SetPause(false);
    // PC->SetShowMouseCursor(false); // <- 일단 대기

 // [수정] 2. 일시정지 메뉴 제거
    if (PauseMenuWidget)
    {
        PauseMenuWidget->RemoveFromParent(); // <--- RemoveFromViewport() 대신 사용
    }

    // 3. 로딩 위젯 생성 및 표시
    UUserWidget* LoadingWidget = nullptr; // 위젯 포인터를 저장
    if (LoadingScreenWidgetClass)
    {
        LoadingWidget = CreateWidget<UUserWidget>(PC, LoadingScreenWidgetClass);
        if (LoadingWidget)
        {
            LoadingWidget->AddToViewport();
            UE_LOG(LogTemp, Warning, TEXT("Loading Screen displayed."));
        }
    }

    // 4. [수정] 입력 모드를 '게임 전용'이 아닌 'UI 전용'으로 변경
    // FInputModeGameOnly InputMode; // <-- 이 코드 대신
    // PC->SetInputMode(InputMode); // <-- 이 코드 대신

    FInputModeUIOnly InputMode;
    //if (LoadingWidget)
    //{
    //    // 로딩 위젯이 혹시 포커스를 받을 수 있다면 설정 (필수는 아님)
    //    InputMode.SetWidgetToFocus(LoadingWidget->TakeWidget());
    //}

    // 마우스가 로딩 화면 밖으로 나가지 않게 잠급니다.
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
    PC->SetInputMode(InputMode);

    // 로딩 화면에서는 마우스 커서가 필요 없을 수 있으니 숨깁니다.
    PC->SetShowMouseCursor(false);


    // 5. 로드할 레벨 이름 저장
    PendingLevelToLoad = LevelName;

    // 6. 2초 뒤 레벨 로드
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AMainCharacter::LoadPendingLevel, 2.0f, false);
}

void AMainCharacter::LoadPendingLevel()
{
    if (PendingLevelToLoad != NAME_None)
    {
        UGameplayStatics::OpenLevel(this, PendingLevelToLoad, true);
        PendingLevelToLoad = NAME_None; // 혹시 모르니 초기화
    }
}

void AMainCharacter::UpdateMouseSensitivity(float NewSensitivity)
{
    MouseSensitivityMultiplier = NewSensitivity;
}

void AMainCharacter::ResetLandingState()
{
    bIsLanding = false;
    UE_LOG(LogTemp, Warning, TEXT("Landing State Reset!"));
}

void AMainCharacter::Move(const FInputActionValue& Value)
{
    if (SkillComponent && (SkillComponent->IsUsingAimSkill1() || SkillComponent->IsUsingAimSkill2())) return;

    FVector2D MovementVector = Value.Get<FVector2D>();

    if (!Controller) return;
    if (bIsBigHitReacting || bIsDead) return;

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
    if (bIsDead) return;

    AddControllerYawInput(LookVector.X * MouseSensitivityMultiplier);
    AddControllerPitchInput(LookVector.Y * MouseSensitivityMultiplier);
}

void AMainCharacter::FireWeapon()
{
    if (bIsBigHitReacting || bIsDead) return;

    if (SkillComponent &&
        (SkillComponent->IsUsingSkill1() ||
            SkillComponent->IsUsingSkill2() ||
            SkillComponent->IsUsingSkill3() ||
            SkillComponent->IsUsingAimSkill1()))
        return;

    if (bIsDead) return;

    if (bIsAiming && Rifle)
    {
        // getter로 받아온 상태를 이용해 재장전 중 사용 불가
        if (Rifle->IsReloading())
        {
            UE_LOG(LogTemp, Warning, TEXT("FireWeapon Blocked: Rifle is Reloading"));
            return;
        }
        // getter로 받아온 상태를 이용해 총알이 없으면 재장전만 수행
        if (Rifle->GetCurrentAmmo() <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("No Ammo. Triggering Reload. No recoil or shake."));
            Rifle->Reload();
            return;
        }

        // 총알이 있고 재장전이 아닐때만 리코일, 흔들림, 발사, 크로스헤어 퍼짐
        float CurrentSpreadAngle = 0.0f;
        if (CrosshairComponent)
        {
            CurrentSpreadAngle = CrosshairComponent->GetBulletSpreadAngle();
            CrosshairComponent->StartExpansion(1.0f);
            ApplyCameraRecoil();
        }
        Rifle->Fire(CurrentSpreadAngle);
    }
    else
    {
        // 에임 모드 아닐 때 콤보 공격
        ComboAttack();
    }

    // 크로스헤어 확산 트리거
    if (CrosshairComponent)
    {
        CrosshairComponent->StartExpansion(1.0f);
    }
}

void AMainCharacter::ReloadWeapon()
{
    if (bIsBigHitReacting || bIsDead) return;

    if (!Rifle)
    {
        return;
    }

    if (bIsDead) return;

    Rifle->Reload();
}

void AMainCharacter::EnterAimMode()
{
    if (bIsBigHitReacting || bIsDead) return;

    if (SkillComponent &&
        (SkillComponent->IsUsingSkill1() ||
            SkillComponent->IsUsingSkill2() ||
            SkillComponent->IsUsingSkill3() ||
            SkillComponent->IsUsingAimSkill1() ||
            SkillComponent->IsUsingAimSkill2()))
        return;

    if (bIsDead) return;

    if (!bIsAiming)
    {
        PreviousZoom = TargetZoom; // 현재 타겟 줌 값을 저장
        bIsAiming = true;
        TargetZoom = AimZoom; // 에임 모드 진입 시 즉시 줌 변경
        UE_LOG(LogTemp, Warning, TEXT("Entered Aim Mode"));
        CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f);

        // [신규 추가] 에임 모드 진입 사운드 재생
        if (AimModeEnterSound)
        {
            // 2D 사운드로 재생 (UI 피드백에 적합)
            UGameplayStatics::PlaySound2D(GetWorld(), AimModeEnterSound);
        }

        AttachRifleToHand(); // 손으로 이동
        AttachKnifeToBack();

        // 크로스헤어 활성화
        if (CrosshairComponent)
        {
            CrosshairComponent->SetCrosshairActive(true);
        }

        if (CrossHairWidget)
        {
            CrossHairWidget->SetVisibility(ESlateVisibility::Visible);
        }
    }

    // 이동속도 업데이트
    UpdateMovementSpeed();
}

void AMainCharacter::ExitAimMode()
{
    if (bIsDead) return;

    if (bIsAiming)
    {
        bIsAiming = false;
        TargetZoom = PreviousZoom; // 기본값 대신 이전 줌 값으로 복귀
        UE_LOG(LogTemp, Warning, TEXT("Exited Aim Mode"));
        CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f);

        AttachRifleToBack(); // 다시 등에 이동
        AttachKnifeToHand();

        // 크로스헤어 비활성화
        if (CrosshairComponent)
        {
            CrosshairComponent->SetCrosshairActive(false);
        }

        if (CrossHairWidget)
        {
            CrossHairWidget->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    // 이동속도 업데이트
    UpdateMovementSpeed();
}

void AMainCharacter::ApplyCameraRecoil()
{
    // 에임 모드일 때만 반동 적용
    if (!bIsAiming) return;

    // 크로스헤어의 분산 정도에 따라 반동 강도 조절
    float SpreadMultiplier = 1.0f;
    if (CrosshairComponent)
    {
        SpreadMultiplier = 1.0f + CrosshairComponent->GetNormalizedSpread() * 0.5f;
    }

    // 수직 반동: 항상 위쪽으로 (음수값)
    float VerticalRecoil = FMath::RandRange(VerticalRecoilMin, VerticalRecoilMax) * SpreadMultiplier;

    // 수평 반동: 좌우 랜덤
    float HorizontalRecoil = FMath::RandRange(HorizontalRecoilMin, HorizontalRecoilMax) * SpreadMultiplier;

    // 반동 값 설정 (Y는 수직, X는 수평)
    TargetRecoil = FVector2D(HorizontalRecoil, -VerticalRecoil);  // Y값을 음수로 하여 위쪽 반동
    bIsRecoiling = true;

    // 화면 흔들림 효과 추가
    ApplyCameraShake();

    GetWorldTimerManager().SetTimer(RecoilTimerHandle, this, &AMainCharacter::ResetRecoil, RecoilDuration, false);

    UE_LOG(LogTemp, Warning, TEXT("Camera recoil applied: Horizontal=%.2f, Vertical=%.2f, Spread Multiplier=%.2f"),
        HorizontalRecoil, VerticalRecoil, SpreadMultiplier);
}

void AMainCharacter::ResetRecoil()
{
    TargetRecoil = FVector2D::ZeroVector;
}

void AMainCharacter::UpdateRecoil(float DeltaTime)
{
    CurrentRecoil = FMath::Vector2DInterpTo(CurrentRecoil, TargetRecoil, DeltaTime, RecoilRecoverySpeed);

    if (Controller)
    {
        AddControllerPitchInput(CurrentRecoil.Y * DeltaTime);
        AddControllerYawInput(CurrentRecoil.X * DeltaTime);
    }

    if (TargetRecoil.IsNearlyZero() && CurrentRecoil.Size() < 0.1f)
    {
        bIsRecoiling = false;
        CurrentRecoil = FVector2D::ZeroVector;
    }
}

void AMainCharacter::ApplyCameraShake()
{
    // 플레이어 컨트롤러 가져오기
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController) return;

    // 카메라 컴포넌트 흔들기
    if (Camera)
    {
        // 랜덤한 흔들림 값 생성
        FVector ShakeOffset;
        ShakeOffset.X = FMath::RandRange(-ShakeIntensity, ShakeIntensity);
        ShakeOffset.Y = FMath::RandRange(-ShakeIntensity, ShakeIntensity);
        ShakeOffset.Z = FMath::RandRange(-ShakeIntensity * 1.0f, ShakeIntensity * 1.0f);

        // 카메라 위치에 흔들림 적용
        FVector OriginalLocation = Camera->GetRelativeLocation();
        Camera->SetRelativeLocation(OriginalLocation + ShakeOffset);

        // 일정 시간 후 원래 위치로 복구
        FTimerHandle ShakeResetTimer;
        GetWorldTimerManager().SetTimer(ShakeResetTimer, [this, OriginalLocation]()
            {
                if (Camera)
                {
                    Camera->SetRelativeLocation(OriginalLocation);
                }
            }, ShakeDuration, false);
    }
}

void AMainCharacter::ShowCrosshairWidget()
{
    if (CrossHairWidget)
    {
        CrossHairWidget->SetVisibility(ESlateVisibility::Visible);
    }
}

void AMainCharacter::HideCrosshairWidget()
{
    if (CrossHairWidget)
    {
        CrossHairWidget->SetVisibility(ESlateVisibility::Hidden);
    }
}

void AMainCharacter::ComboAttack()
{
    if (bIsBigHitReacting || bIsDead) return;

    if (!MeleeCombatComponent || !SkillComponent) return;

    if (bIsDead) return;

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
    if (bIsBigHitReacting || bIsDead) return;

    if (!bCanDash || bIsDashing || !Controller || bIsJumping || bIsInDoubleJump) return;

    // 에임스킬1 사용 중 대시하면 스킬을 캔슬하고 대시
    if (SkillComponent)
    {
        if (SkillComponent->IsUsingAimSkill1())
        {
            // 스킬 컴포넌트에 캔슬 및 쿨타임 재시작을 요청
            SkillComponent->CancelAimSkill1ByDash();
            // return하지 않고 대시 로직을 계속 진행합니다.
        }
        // 에임스킬2(캐논)는 여전히 대시를 막습니다.
        else if (SkillComponent->IsUsingAimSkill2())
        {
            return;
        }
    }

    if (bIsDead) return;

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
    if (bIsDead) return;

    TargetZoom = FMath::Clamp(TargetZoom - ZoomStep, MinZoom, MaxZoom);

    // 에임모드가 아닐 때만 PreviousZoom 업데이트
    if (!bIsAiming)
    {
        PreviousZoom = TargetZoom;
    }

    UE_LOG(LogTemp, Warning, TEXT("Zoom In: %f"), CurrentZoom);
}

void AMainCharacter::ZoomOut()
{
    if (bIsDead) return;
    if (bIsAiming) return; // 에임 모드일 때는 줌 불가능

    TargetZoom = FMath::Clamp(TargetZoom + ZoomStep, MinZoom, MaxZoom);

    // 에임모드가 아닐 때만 PreviousZoom 업데이트
    if (!bIsAiming)
    {
        PreviousZoom = TargetZoom;
    }

    UE_LOG(LogTemp, Warning, TEXT("Zoom Out: %f"), CurrentZoom);
}

void AMainCharacter::UseSkill1()
{
    if (bIsBigHitReacting || bIsDead) return;

    UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::UseSkill1 triggered"));
    if (SkillComponent)
    {
        if (bIsAiming) // 에임 스킬 1
        {
            if (SkillComponent->CanUseAimSkill1())
            {
                UE_LOG(LogTemp, Warning, TEXT("UseAimSkill1"));
                SkillComponent->UseAimSkill1();
            }
            else // 쿨타임일 경우 사운드 재생 (통합)
            {
                PlayCooldownSound();
            }
        }
        else // 일반 스킬 1
        {
            if (SkillComponent->CanUseSkill1())
            {
                UE_LOG(LogTemp, Warning, TEXT("UseSkill1"));
                SkillComponent->UseSkill1();
            }
            else // 쿨타임일 경우 사운드 재생 (통합)
            {
                PlayCooldownSound();
            }
        }
    }
}

void AMainCharacter::UseSkill2()
{
    if (bIsBigHitReacting || bIsDead) return;

    UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::UseSkill2 triggered"));
    if (SkillComponent)
    {
        if (bIsAiming) // 에임 스킬 2
        {
            if (SkillComponent->CanUseAimSkill2())
            {
                UE_LOG(LogTemp, Warning, TEXT("UseAimSkill2"));
                SkillComponent->UseAimSkill2();
            }
            else // 쿨타임일 경우 사운드 재생 (통합)
            {
                PlayCooldownSound();
            }
        }
        else // 일반 스킬 2
        {
            if (SkillComponent->CanUseSkill2())
            {
                UE_LOG(LogTemp, Warning, TEXT("UseSkill2"));
                SkillComponent->UseSkill2();
            }
            else // 쿨타임일 경우 사운드 재생 (통합)
            {
                PlayCooldownSound();
            }
        }
    }
}

void AMainCharacter::UseSkill3()
{
    UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::UseSkill3 triggered"));
    if (bIsBigHitReacting || bIsDead) return;
    if (SkillComponent)
    {
        if (bIsAiming) // 에임 스킬 3
        {
            if (SkillComponent->CanUseAimSkill3())
            {
                UE_LOG(LogTemp, Warning, TEXT("UseAimSkill3"));
                SkillComponent->UseAimSkill3();
            }
            else // 쿨타임일 경우 사운드 재생 (통합)
            {
                PlayCooldownSound();
            }
        }
        else // 일반 스킬 3
        {
            if (SkillComponent->CanUseSkill3())
            {
                UE_LOG(LogTemp, Warning, TEXT("UseSkill3"));
                SkillComponent->UseSkill3();
            }
            else // 쿨타임일 경우 사운드 재생 (통합)
            {
                PlayCooldownSound();
            }
        }
    }
}

float AMainCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.0f;

    float DamageApplied = FMath::Min(CurrentHealth, DamageAmount);
    CurrentHealth -= DamageApplied;

    UE_LOG(LogTemp, Warning, TEXT("MainCharacter took: %f Damage, Health remaining: %f"), DamageApplied, CurrentHealth);

    // 히트 사운드 랜덤 재생
    if (NormalHitSounds.Num() > 0)
    {
        int32 RandIndex = FMath::RandRange(0, NormalHitSounds.Num() - 1);
        if (NormalHitSounds[RandIndex])
        {
            UGameplayStatics::PlaySoundAtLocation(this, NormalHitSounds[RandIndex], GetActorLocation());
        }
    }

    // [수정] 빅 히트 리액션 중에는 일반 피격 몽타주 재생 안 함
    if (bIsBigHitReacting)
    {
        UE_LOG(LogTemp, Warning, TEXT("Damage received during Big Hit - skip normal hit montage"));
    }

    // 스킬 시전 중이 아닐 때만 히트 몽타주 재생
    else if (SkillComponent && !SkillComponent->IsCastingSkill())
    {
        if (NormalHitMontages.Num() > 0)
        {
            int32 RandomIndex = FMath::RandRange(0, NormalHitMontages.Num() - 1);
            UAnimMontage* SelectedMontage = NormalHitMontages[RandomIndex];
            if (SelectedMontage)
            {
                UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
                if (AnimInstance)
                {
                    AnimInstance->Montage_Play(SelectedMontage, 1.0f);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Damage received during skill cast - skip hit montage"));
    }

    // 사망 체크
    if (CurrentHealth <= 0.0f)
    {
        Die();
    }

    return DamageApplied;
}

void AMainCharacter::Die()
{
    if (bIsDead) return;
    bIsDead = true;
    bIsBigHitReacting = false;
    ExitAimMode();

    // 줌 값 즉시 복구 (ExitAimMode() 내 TargetZoom 복구 외 추가)
    CurrentZoom = TargetZoom = DefaultZoom;
    CameraBoom->TargetArmLength = CurrentZoom;

    // ⭐ 2. [신규/핵심] 애님 인스턴스에 강제 업데이트 요청 ⭐
    // 에임 자세가 즉시 해제되도록 애니메이션을 강제로 한 번 업데이트합니다.
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        // 애님 인스턴스의 플래그 업데이트
        // (ABP에서 bIsAiming 변수명을 사용한다고 가정합니다. 아니라면 변수명을 맞춰야 합니다.)
        // 하지만 C++에서 이미 ExitAimMode()를 통해 bIsAiming = false를 설정했으므로,
        // ABP가 이를 인식하는 데 걸리는 딜레이를 줄여야 합니다.

        // Tick 그룹 변경 시도: 애니메이션 업데이트 우선순위를 높여 다음 틱에서 즉시 반영되도록 합니다.
        GetMesh()->SetTickGroup(ETickingGroup::TG_PostPhysics);
    }

    if (SkillComponent)
    {
        SkillComponent->CancelAllSkills();
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        //DisableInput(PC);
    }

    if (DieSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DieSound, GetActorLocation());
    }

    if (DieMontages.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, DieMontages.Num() - 1);
        UAnimMontage* SelectedMontage = DieMontages[RandomIndex];
        if (SelectedMontage)
        {
            UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
            if (AnimInstance)
            {
                AnimInstance->Montage_Play(SelectedMontage, 1.0f);
            }
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("MainCharacter Dead!"));

    // [추가] 최고 기록 저장 로직
    if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
    {
        // 현재 진행 중이던 웨이브 인덱스 - 1이 마지막 클리어 웨이브 인덱스
        int32 LastClearedWaveIndex = GameMode->GetCurrentWaveIndex() - 1;

        // -1보다 큰 경우(최소 0번 웨이브 이상 클리어)에만 저장 시도
        if (LastClearedWaveIndex >= -1)
        {
            GameMode->CheckAndSaveBestWaveRecord(LastClearedWaveIndex);
        }
    }

    if (DeathPostProcessComponent)
    {
        // 1. BlendWeight는 UPostProcessComponent의 공개 변수에 직접 접근하여 설정합니다.
        // SetBlendWeight() 함수는 존재하지 않습니다.
        DeathPostProcessComponent->BlendWeight = 1.0f; // 즉시 100% 적용

        // 2. 채도(Desaturation)를 0.0으로 설정하여 회색 화면을 만듭니다.
        // 해당 항목의 오버라이드 플래그를 true로 설정합니다.
        DeathPostProcessComponent->Settings.bOverride_ColorSaturation = true;

        // ColorSaturation 항목에 흑백/회색톤(0.0) 적용
        // FVector4(R, G, B, A) 모두 0.0을 주면 Desaturation 효과가 나타납니다.
        DeathPostProcessComponent->Settings.ColorSaturation = FVector4(0.0f, 0.0f, 0.0f, 1.0f);

        // 3. (선택적) 비네팅 효과를 추가하여 화면 가장자리를 어둡게 만듭니다.
        DeathPostProcessComponent->Settings.bOverride_VignetteIntensity = true;
        DeathPostProcessComponent->Settings.VignetteIntensity = 0.5f;

        // 주의: FPostProcessSettings 멤버가 아닌 'bOverride_PostProcessDetails'는 제거되었습니다.
    }
}

// [신규] 빅 히트 리액션 재생 함수
void AMainCharacter::PlayBigHitReaction()
{
    // 이미 죽었거나, 이미 빅 히트 리액션 중이면 중복 실행 방지
    if (bIsDead || bIsBigHitReacting) return;

    if (!BigHitMontage || !BigHitRecoverMontage)
    {
        UE_LOG(LogTemp, Error, TEXT("PlayBigHitReaction Failed: Montages not set!"));
        return;
    }

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance) return;

    UE_LOG(LogTemp, Warning, TEXT("BIG HIT REACTION TRIGGERED!"));

    // 1. 빅 히트 상태로 전환
    bIsBigHitReacting = true;

    // 2. 모든 스킬 및 에임 모드 강제 캔슬
    if (SkillComponent)
    {
        SkillComponent->CancelAllSkills();
    }
    if (bIsAiming)
    {
        ExitAimMode();
    }
    // 콤보 공격도 중단 (MeleeCombatComponent에 중단 함수가 있다면 호출)
    // if (MeleeCombatComponent)
    // {
    //    MeleeCombatComponent->StopComboAttack(); 
    // }

    // 3. 빅 히트 (넘어지는) 몽타주 재생
    float MontageDuration = AnimInstance->Montage_Play(BigHitMontage, 1.0f); // [수정] 재생 시간을 변수에 저장

    // 4. 빅 히트 사운드 재생
    if (BigHitSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, BigHitSound, GetActorLocation());
    }

    // [수정] 5. 첫 번째 몽타주 80% 지점에서 리커버리 몽타주를 재생하도록 타이머 설정
    if (MontageDuration > 0.0f)
    {
        float RecoverPlayTime = MontageDuration * 0.8f;
        GetWorldTimerManager().SetTimer(BigHitRecoverTimerHandle, this, &AMainCharacter::StartBigHitRecover, RecoverPlayTime, false);
    }
    else
    {
        // 몽타주 재생 실패 시 (길이가 0), 즉시 리커버리 시도
        StartBigHitRecover();
    }
}

// [신규] 빅 히트 몽타주 90% 지점에서 타이머로 호출될 함수
void AMainCharacter::StartBigHitRecover()
{
    // 1. 사망했다면 리커버리 재생 안 함
    if (bIsDead)
    {
        bIsBigHitReacting = false; // 혹시 모르니 상태 해제
        return;
    }

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance || !BigHitRecoverMontage)
    {
        bIsBigHitReacting = false; // 몽타주가 없으면 그냥 상태 해제
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Big Hit 90%% reached. Playing Recover Montage..."));

    // 2. 두 번째 몽타주 (일어나기) 재생
    AnimInstance->Montage_Play(BigHitRecoverMontage, 1.0f);

    // 3. 두 번째 몽타주 종료 시 호출될 델리게이트 바인딩
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &AMainCharacter::OnBigHitRecoverMontageEnded);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, BigHitRecoverMontage);
}

// [신규] 빅 히트 리커버 몽타주(일어남)가 끝났을 때 호출됨
void AMainCharacter::OnBigHitRecoverMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Warning, TEXT("Big Hit RECOVERED. Character control restored."));

    // 1. 빅 히트 상태를 최종적으로 해제하여 모든 행동 가능하도록 복구
    bIsBigHitReacting = false;
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
        EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AMainCharacter::ReloadWeapon);
        EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AMainCharacter::Dash);
        EnhancedInputComponent->BindAction(ZoomInAction, ETriggerEvent::Triggered, this, &AMainCharacter::ZoomIn);
        EnhancedInputComponent->BindAction(ZoomOutAction, ETriggerEvent::Triggered, this, &AMainCharacter::ZoomOut);
        EnhancedInputComponent->BindAction(Skill1Action, ETriggerEvent::Started, this, &AMainCharacter::UseSkill1);
        EnhancedInputComponent->BindAction(Skill2Action, ETriggerEvent::Started, this, &AMainCharacter::UseSkill2);
        EnhancedInputComponent->BindAction(Skill3Action, ETriggerEvent::Started, this, &AMainCharacter::UseSkill3);
        EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AMainCharacter::TogglePauseMenu);
    }
}