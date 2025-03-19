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
        SetActorRotation(TargetRootMotionRotation); // 루트모션(콤보공격)이 적용되는 동안 방향유지 
    }
}

void AMainCharacter::HandleJump()
{
    // 공격 중이거나 대쉬중에 점프 불가
    if (bIsAttacking || bIsDashing)
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
    if (bIsAttacking || bIsJumping || bIsInDoubleJump || bIsDashing) return; // 이미 공격중이거나 점프중이면 공격 불가

    GetWorldTimerManager().ClearTimer(ComboCooldownHandle); // 콤보 쿨다운 타이머 초기화
    GetWorldTimerManager().SetTimer(ComboCooldownHandle, this, &AMainCharacter::ResetCombo, ComboCooldownTime, false); // 콤보 쿨다운 타이머 설정

    bIsAttacking = true; // 공격 상태 변경

    // 콤보 중 자동 회전 비활성화 (카메라 영향을 막음)
    GetCharacterMovement()->bOrientRotationToMovement = false;

    AdjustComboAttackDirection(); // 공격 방향 보정

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

void AMainCharacter::ResetComboTimer()
{
    ResetCombo(); // 콤보 초기화
}
// 콤보 리셋 함수 (공격 후 콤보 상태 초기화)
void AMainCharacter::ResetCombo()
{
    bIsAttacking = false; // 공격 상태 초기화
    ComboIndex = 0; // 콤보 인덱스 초기화

    GetWorldTimerManager().ClearTimer(ComboCooldownHandle); // 콤보 쿨다운 타이머 초기화
    UE_LOG(LogTemp, Warning, TEXT("Combo Reset!"));
}

void AMainCharacter::AdjustComboAttackDirection()
{
    float MaxAutoAimDistance = 200.0f; // 자동 보정이 적용되는 거리
    float RotationSpeed = 8.0f; // 회전 속도

    AActor* TargetEnemy = nullptr; // 가장 가까운 적을 저장할 변수
    float ClosestDistance = MaxAutoAimDistance; // 현재 가장 가까운 적과의 거리 초기값은 최대 보정값

    TArray<AActor*> FoundEnemies; // 모든 적을 저장할 배열
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), FoundEnemies); // 모든 적을 찾아 배열에 저장

    for (AActor* Enemy : FoundEnemies) // 모든 적을 순회하며 가장 가까운 적을 찾음
    {
        AEnemy* EnemyCharacter = Cast<AEnemy>(Enemy); // AActor 타입을 AEnemy 타입으로 변환 AActor는 bIsDead 변수를 사용할 수 없음

        // nullptr이거나 죽은 상태인 적은 무시
        if (!EnemyCharacter || EnemyCharacter->bIsDead) // EnemyCharacter은 AEneymy 타입이므로 bIsDead 변수 사용 가능
        {
            continue;
        }

        float Distance = FVector::Dist(GetActorLocation(), Enemy->GetActorLocation()); // 플레이어와 적 사이의 거리 계산

        if (Distance < ClosestDistance) // 현재 가장 가까운 적보다 더 가까운 적을 찾았다면
        {
            ClosestDistance = Distance; // 가장 가까운 적의 거리 업데이트
            TargetEnemy = Enemy; // 보정할 적을 현재 적으로 설정
        }
    }

    if (TargetEnemy) // 보정 대상이 되는 적의 경우
    {
        FVector DirectionToEnemy = (TargetEnemy->GetActorLocation() - GetActorLocation()).GetSafeNormal(); // 플레이어에서 적을 향하는 방향 벡터 계산
        TargetRootMotionRotation = FRotationMatrix::MakeFromX(DirectionToEnemy).Rotator(); // 적을 향한 방향으로 캐릭터 회전
        bApplyRootMotionRotation = true; // 루트모션 중 보정된 방향을 유지하도록 설정

        UE_LOG(LogTemp, Warning, TEXT("Adjusted Attack Direction to: %s"), *TargetEnemy->GetName());
        UE_LOG(LogTemp, Warning, TEXT("Target Position: %s"), *TargetEnemy->GetActorLocation().ToString());
        UE_LOG(LogTemp, Warning, TEXT("Target RootMotion Rotation: %s"), *TargetRootMotionRotation.ToString());

        // 디버그로 시각화
        FVector Start = GetActorLocation(); // 시작점: 캐릭터 위치
        FVector End = TargetEnemy->GetActorLocation(); // 끝점: 보정 대상 적 위치
        DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f, 0, 2.0f); // 적 방향 (빨간색)
        DrawDebugLine(GetWorld(), Start, Start + GetActorForwardVector() * 200.0f, FColor::Green, false, 2.0f, 0, 2.0f); // 현재 공격 방향 (초록색)
    }
}

void AMainCharacter::OnComboMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    bIsAttacking = false; // 공격 상태 초기화
    GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 시 자동 회전 다시 활성화
    bApplyRootMotionRotation = false; // 루트모션 방향 보정 해제

    UE_LOG(LogTemp, Warning, TEXT("Root Motion Stopped. Player Control Restored!"));

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
        KickHitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 히트박스 충돌 활성화
        UE_LOG(LogTemp, Warning, TEXT("Kick HitBox Enabled!"));
    }


    KickRaycastAttack(); // 레이캐스트 실행
}

void AMainCharacter::DisableKickHitBox()
{
    if (KickHitBox)
    {
        KickHitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 히트박스 충돌 비활성화
        UE_LOG(LogTemp, Warning, TEXT("Kick HitBox Disabled!"));
    }

    KickRaycastHitActor = nullptr; // 다음 공격을 위해 레이캐스트 적중 객체 초기화
}

void AMainCharacter::KickRaycastAttack()
{
    AActor* OwnerActor = this;
    if (!OwnerActor) return;

    FVector StartLocation = OwnerActor->GetActorLocation() + (OwnerActor->GetActorForwardVector() * 20.0f); // 뒤에 있는 적 히트 방지를 위해 캐릭터로부터 해당거리 만큼 떨어진 곳에서 레이캐스트 시작
    FVector EndLocation = StartLocation + (OwnerActor->GetActorForwardVector() * 150.0f); // 해당 길이만큼 레이캐스트 발사

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerActor); // 자기 자신과의 충돌 무시

    bool bRaycastHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params); // 레이캐스트 실행(적이 감지되면 bRaycastHit = true)

    FColor LineColor = bRaycastHit ? FColor::Red : FColor::Green; // 디버그 시각화(빨간색 = 적중, 초록색 = 미적중)
    DrawDebugLine(GetWorld(), StartLocation, EndLocation, LineColor, false, 1.0f, 0, 3.0f); // 앞쪽으로만 공격 범위 표시

    if (bRaycastHit)
    {
        KickRaycastHitActor = HitResult.GetActor(); // 레이캐스트에서 감지된 적 저장
        UE_LOG(LogTemp, Warning, TEXT("Kick Raycast Hit: %s"), *KickRaycastHitActor->GetName());

        // 충돌한 지점을 빨간색 구체로 시각화
        DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 10.0f, 12, FColor::Red, false, 1.0f);
    }
    else
    {
        KickRaycastHitActor = nullptr; // 적중 실패 시 초기화
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
    if (!OtherActor || OtherActor == this || OtherComp == KickHitBox) // 자기 자신이거나 잘못된 객체일 경우 무시
    {
        return;
    }

    // 히트박스에 감지되었으나 레이캐스트에서 감지되지 않으면 무효
    if (OtherActor != KickRaycastHitActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("Kick HitBox Detected, But No Raycast Hit: %s"), *OtherActor->GetName());
        return;
    }

    float KickDamage = 35.0f; // 발차기 데미지 적용
    UGameplayStatics::ApplyDamage(OtherActor, KickDamage, nullptr, this, nullptr);

    UE_LOG(LogTemp, Warning, TEXT("Kick Hit! Applied %f Damage to %s"), KickDamage, *OtherActor->GetName());

    DisableKickHitBox(); // 히트박스 비활성화
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

void AMainCharacter::Skill1()
{
    if (bIsUsingSkill1) return; // 스킬 사용 중일 때는 스킬 사용 불가
    if (!bCanUseSkill1) return; // 스킬 쿨다운 중일 때는 스킬 사용 불가

    if (bIsDashing || bIsAiming || bIsJumping || bIsInDoubleJump) return; // 대쉬, 에임, 점프 중일 때는 스킬 사용 불가

    bIsUsingSkill1 = true; // 스킬 사용 상태로 변경
    bCanUseSkill1 = false; // 스킬 쿨다운 시작

    // 이동 입력 방향 감지
    FVector InputDirection = GetCharacterMovement()->GetLastInputVector(); // 현재 캐릭터의 이동 입력 방향을 감지하여 해당 방향으로 회전
    if (!InputDirection.IsNearlyZero())
    {
        InputDirection.Normalize();
        FRotator NewRotation = InputDirection.Rotation();
        NewRotation.Pitch = 0.0f; // 피치값 유지로 고개 숙임 방지
        SetActorRotation(NewRotation); // 캐릭터를 입력 방향으로 회전
    }

    PlaySkill1Montage(Skill1AnimMontage); // 스킬 애니메이션 실행

    DrawSkill1Range(); // 스킬 범위 표시

    GetWorldTimerManager().SetTimer(SkillEffectTimerHandle, this, &AMainCharacter::ApplySkill1Effect, 0.5f, false);  // 일정 시간 후 스킬 적용 실행

    GetWorldTimerManager().SetTimer(Skill1CooldownTimerHandle, this, &AMainCharacter::ResetSkill1Cooldown, Skill1Cooldown, false); // 몽타주가 시작되면 쿨다운 타이머 시작
}

void AMainCharacter::DrawSkill1Range()
{
    FVector SkillCenter = GetActorLocation(); // 현재 캐릭터 위치를 중심으로 스킬 발동
    float Duration = 1.0f; // 디버그 지속 시간

    DrawDebugSphere(GetWorld(), SkillCenter, SkillRange, 32, FColor::Red, false, Duration, 0, 2.0f); // 스킬 범위 표시
}

void AMainCharacter::ApplySkill1Effect()
{
    FVector SkillCenter = GetActorLocation(); // 현재 캐릭터 위츠를 중심으로 스킬 발동

    TArray<AActor*> OverlappingEnemies; // 현재 맵에 존재하는 모든적 클래스를 탐색
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), OverlappingEnemies); // 현재 맵에 존재하는 모든적 클래스를 탐색

    float InAirTime = 4.0f; // 공준 스턴 지속 시간

    for (AActor* Actor : OverlappingEnemies) // 찾은 적들에게 
    {
        AEnemy* Enemy = Cast<AEnemy>(Actor); // 스킬 효과 적용

        if (Enemy && Enemy->GetCharacterMovement()) // 적이 존재하고 이동이 가능할때만
        {
            float Distance = FVector::Dist(SkillCenter, Enemy->GetActorLocation());

            if (Distance <= SkillRange) // 스킬 범위 안에 있는 적만 효과 적용
            {
                Enemy->EnterInAirStunState(InAirTime); // 공중 스턴 적용
                UE_LOG(LogTemp, Warning, TEXT("Enemy %s stunned by Skill1!"), *Enemy->GetName());
            }
        }
    }
}

void AMainCharacter::PlaySkill1Montage(UAnimMontage* Skill1Montage)
{
    if (!Skill1Montage) // 애니메이션이 없으면 실행 취소
    {
        UE_LOG(LogTemp, Error, TEXT("PlaySkill1Montage Failed: SkillMontage is NULL"));
        return;
    }

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 캐릭터와 애님 인스턴스를 가져옴
    if (!AnimInstance) // 애님 인스턴스가 없으면 실행 취소
    {
        UE_LOG(LogTemp, Error, TEXT("PlaySkillMontage Failed: AnimInstance is NULL"));
        return;
    }

    float MontageDuration = AnimInstance->Montage_Play(Skill1Montage, 1.0f); // 스킬 애니메이션 실행
    if (MontageDuration <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Montage_Play Failed: %s"), *Skill1Montage->GetName()); // 애니메이션 실행 실패 로그
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Montage_Play Success: %s"), *Skill1Montage->GetName()); // 애니메이션 실행 성공 로그

    // 애니메이션 종료 시 스킬1 상태를 초기화하도록 콜백 함수 설정
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &AMainCharacter::ResetSkill1);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, Skill1Montage);
}

void AMainCharacter::ResetSkill1(UAnimMontage* Montage, bool bInterrupted)
{
    bIsUsingSkill1 = false; // 스킬1 사용 상태 해제
    GetWorldTimerManager().SetTimer(Skill1CooldownTimerHandle, this, &AMainCharacter::ResetSkill1Cooldown, Skill1Cooldown, false); // 쿨다운 타이머 시작
    UE_LOG(LogTemp, Warning, TEXT("Skill1 Cooldown Started!"));
}

void AMainCharacter::ResetSkill1Cooldown()
{
    bCanUseSkill1 = true; // 스킬1 사용 가능상태로 변경
    UE_LOG(LogTemp, Warning, TEXT("Skill1 Cooldown Over! Ready to Use Skill1 Again."));
}

void AMainCharacter::Skill2()
{
    if (bIsUsingSkill2) return; // 사용중이면 사용 불가
    if (!bCanUseSkill2) return; // 쿨다운 상태면 사용 불가

    if (bIsDashing || bIsAiming || bIsJumping || bIsInDoubleJump) return; // 대쉬, 에임, 점프 중일 때는 스킬 사용 불가

    bIsUsingSkill2 = true; // 스킬 사용 상태 활성화
    bCanUseSkill2 = false; // 스킬 쿨다운 시작

    // 이동 입력 방향 감지
    FVector InputDirection = GetCharacterMovement()->GetLastInputVector(); // 현재 캐릭터의 이동 입력 방향을 감지하여 해당 방향으로 회전
    if (!InputDirection.IsNearlyZero())
    {
        InputDirection.Normalize();
        FRotator NewRotation = InputDirection.Rotation();
        NewRotation.Pitch = 0.0f; // 피치 값 유지로 고개 숙임 방지
        SetActorRotation(NewRotation); // 캐릭터를 입력 방향으로 회전
    }

    PlaySkill2Montage(Skill2AnimMontage); // 스킬 애니메이션 실행

    GetWorldTimerManager().SetTimer(Skill2EffectTimerHandle, this, &AMainCharacter::ApplySkill2Effect, Skill2EffectDelay, false); // 설정된 Skill2EffectDelay 값만큼 후에 스킬 히트 판정 실행

    GetWorldTimerManager().SetTimer(Skill2CooldownTimerHandle, this, &AMainCharacter::ResetSkill2Cooldown, Skill2Cooldown, false); // 몽타주가 시작되면 쿨다운 타이머 시작
}

void AMainCharacter::DrawSkill2Range()
{
    if (!bIsUsingSkill2) return; // 스킬이 끝났으면 범위를 갱신하지 않음

    UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld());

    FVector SkillCenter = GetActorLocation(); // 루트모션이 적용되는 동안 실시간 위치 가져오기
    float DebugDuration = 0.1f; // 빠른 갱신을 위해 0.1초로 설정
    float DebugRadius = 200.0f; // 원형 범위 반경

    DrawDebugSphere(GetWorld(), SkillCenter, DebugRadius, 32, FColor::Blue, false, DebugDuration, 0, 3.0f); // 스킬범위 표시

    UE_LOG(LogTemp, Warning, TEXT("Skill2 Range Circle Drawn at %s with Radius %f"), *SkillCenter.ToString(), DebugRadius);
}

void AMainCharacter::ApplySkill2Effect()
{
    FVector SkillCenter = GetActorLocation(); // 현재 캐릭터 위치를 기준으로 효과 적용

    // 스킬2 사용시캐릭터 높이 조절 (루트모션 무시 가능)
    float JumpHeight = 300.0f;  // 설정한 도약 높이 만큼
    LaunchCharacter(FVector(0, 0, JumpHeight), false, true);

    TArray<FHitResult> HitResults; // 히트된 적을 저장할 배열

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes; 
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));  // 적 탐색을 위해 Pawn을 대상으로 설정

    // Sphere Trace를 사용하여 스킬 범위 내에 있는 적을 감지
    bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
        GetWorld(),
        SkillCenter, // 탐색 중심 (캐릭터 위치)
        SkillCenter, // 탐색 중심 (캐릭터 위치)
        Skill2Range, // 범위 반경
        ObjectTypes, // 감지할 오브젝트타입 (Pawn)
        false,       // 단단한 벽 오브젝트 무시 여부
        TArray<AActor*>(), // 무시할 엑터 목록 
        EDrawDebugTrace::None, // 디버그 모드 설정
        HitResults, // 감지된 결과 저장
        true
    );

    TSet<AEnemy*> HitEnemies; // 이미 맞은 적을 저장할 Set (중복 공격 방지)

    if (bHit)
    {
        DrawDebugSphere(GetWorld(), SkillCenter, Skill2Range, 32, FColor::Red, false, 0.4f, 0, 3.0f); // 스킬 범위 표시

        for (const FHitResult& Hit : HitResults) // 감지된 적들에게 데미지 적용
        {
            AEnemy* Enemy = Cast<AEnemy>(Hit.GetActor());
            if (Enemy && !HitEnemies.Contains(Enemy)) // 중복 방지
            {
                float AppliedDamage = Skill2Damage;
                UGameplayStatics::ApplyDamage(Enemy, AppliedDamage, GetController(), this, UDamageType::StaticClass());

                UE_LOG(LogTemp, Warning, TEXT("Skill2 Hit Enemy: %s | Damage: %f"), *Enemy->GetName(), AppliedDamage);

                HitEnemies.Add(Enemy); // 한 번 맞은 적은 추가
            }
        }
    }

    GetWorldTimerManager().SetTimer(Skill2RangeClearTimerHandle, this, &AMainCharacter::ClearSkill2Range, 0.4f, false); // 디버그 제거 (0.2초 후)
}

void AMainCharacter::ClearSkill2Range()
{
    UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld()); // 디버그 범위 제거
}

void AMainCharacter::PlaySkill2Montage(UAnimMontage* Skill2Montage)
{
    if (!Skill2Montage) // 애니메이션이 없으면 실행 취소
    {
        UE_LOG(LogTemp, Error, TEXT("PlaySkill1Montage Failed: SkillMontage is NULL"));
        return;
    }

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 캐릭터와 애님 인스턴스를 가져옴
    if (!AnimInstance) // 애님 인스턴스가 없으면 실행 취소
    {
        UE_LOG(LogTemp, Error, TEXT("PlaySkillMontage Failed: AnimInstance is NULL"));
        return;
    }

    float MontageDuration = AnimInstance->Montage_Play(Skill2Montage, 1.2f); // 스킬 애니메이션 실행
    if (MontageDuration <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Montage_Play Failed: %s"), *Skill2Montage->GetName()); // 애니메이션 실행 실패 로그
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Montage_Play Success: %s"), *Skill2Montage->GetName()); // 애니메이션 실행 성공 로그

    // 애니메이션 종료 시 대시 상태를 초기화하도록 콜백 함수 설정
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &AMainCharacter::ResetSkill2);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, Skill2Montage);
}

void AMainCharacter::ResetSkill2(UAnimMontage* Montage, bool bInterrupted)
{
    bIsUsingSkill2 = false; // 스킬 사용 상태 해제
    // 스킬 종료 시 디버그 삭제
    UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld());

    GetWorldTimerManager().SetTimer(Skill2CooldownTimerHandle, this, &AMainCharacter::ResetSkill2Cooldown, Skill2Cooldown, false); // 쿨다운 타이머 시작
    UE_LOG(LogTemp, Warning, TEXT("Skill2 Cooldown Started!"));
}

void AMainCharacter::ResetSkill2Cooldown()
{
    bCanUseSkill2 = true; // 스킬 사용 가능상태로 변경
    UE_LOG(LogTemp, Warning, TEXT("Skill2 Cooldown Over! Ready to Use Skill2 Again."));
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
        EnhancedInputComponent->BindAction(Skill1Action, ETriggerEvent::Triggered, this, &AMainCharacter::Skill1);
        EnhancedInputComponent->BindAction(Skill2Action, ETriggerEvent::Triggered, this, &AMainCharacter::Skill2);
    }
}