#include "MainCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomAnimInstance.h"
#include "Rifle.h"
#include "Knife.h"
#include "Skill3Projectile.h"
#include "MachineGun.h"
#include "Cannon.h"
#include "Kismet/GameplayStatics.h"
#include "CrossHairWidget.h"
#include "Blueprint/UserWidget.h"
#include "SettingsSaveGame.h"
#include "SettingsGameInstance.h"
#include "SkillComponent.h"
#include "MeleeCombatComponent.h"
#include "MainGameModeBase.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"

AMainCharacter::AMainCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // 기본 이동속도 설정
    GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;

    // 카메라 붐 (스프링암 컴포넌트)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom")); // 스프링암 컴포넌트 생성
    CameraBoom->SetupAttachment(GetRootComponent()); // 루트컴포넌트에 부착 
    CameraBoom->TargetArmLength = 250.0f; // 카메라와 캐릭터 사이의 거리
	CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f); // 카메라 오프셋 위치 조정
	CameraBoom->bUsePawnControlRotation = true; // 회전 입력에 따라 스프링암이 함께 회전
    CameraBoom->bDoCollisionTest = true; // 카메라 벽 뚫기 방지를 위해 충돌 활성화
	CameraBoom->ProbeSize = 12.0f; // 얉은 기둥이나 모서리에서 카메라가 튀지 않도록 충돌 감지 범위를 선이 아닌 반지름 12인 구체로 설정
    CameraBoom->bEnableCameraLag = true; // 충돌 시에도 부드럽게 따라오도록 물리적 지연을 주는 카메라 래그 활성화
	CameraBoom->CameraLagSpeed = 10.0f; // 카메라 래그 속도 설정

    // 카메라 컴포넌트
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera")); // 플레이어에게 보여질 카메라 생성
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // 스프링 암에 카메라를 자식으로 부착
	Camera->bUsePawnControlRotation = false; // 카메라는 회전에 따라 회전하지 않음

	// 캐릭터 회전 및 이동 설정
	bUseControllerRotationYaw = false; // 캐릭터가 컨트롤러의 Yaw 회전에 따라 회전하지 않도록 설정
	GetCharacterMovement()->bOrientRotationToMovement = true; // 캐릭터가 이동 방향으로 회전하도록 설정
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // 회전 속도 설정

	// 이동 관련 변수 초기화
	Speed = 0.0f; // 이동 속도
	Direction = 0.0f; // 이동 방향
	AimPitch = 0.0f; // 에임 피치 초기화
	bIsJumping = false; // 점프 상태 초기화
	bIsInDoubleJump = false; // 더블점프 상태 초기화
	bIsInAir = false; // 공중 상태 초기화
	bCanDoubleJump = true; // 더블점프 가능 상태 초기화
	bIsAiming = false; // 에임 모드 초기화
    CurrentHealth = MaxHealth; // 체력 초기화
    bIsDead = false; // 사망 플래그 초기화
    bIsBigHitReacting = false; // 빅 히트 플래그 초기화

    // 발차기 히트박스 초기화
    KickHitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("KickHitBox")); // 박스 컴포넌트 생성
    KickHitBox->SetupAttachment(GetMesh(), TEXT("FootSocket_R")); // 소켓에 부착
	KickHitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 기본적으로 충돌 비활성화
	KickHitBox->SetGenerateOverlapEvents(true); // 오버랩 이벤트 생성 활성화
    KickHitBox->SetCollisionObjectType(ECC_WorldDynamic);  // 충돌 오브젝트 타입 설정
	KickHitBox->SetCollisionResponseToAllChannels(ECR_Ignore); // 모든 채널에 대해 무시 설정
    KickHitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 캐릭터와만 충돌 감지
    
	MeleeCombatComponent = CreateDefaultSubobject<UMeleeCombatComponent>(TEXT("MeleeCombatComponent")); // 밀리어택 컴포넌트 생성
	SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent")); // 스킬 컴포넌트 생성
    CrosshairComponent = CreateDefaultSubobject<UCrossHairComponent>(TEXT("CrosshairComponent")); // 크로스헤어 컴포넌트 생성

	CooldownAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("CooldownAudioComponent")); // 쿨타임 사운드 오디오 컴포넌트 생성
	CooldownAudioComponent->SetupAttachment(GetRootComponent()); // 루트 컴포넌트에 부착
	CooldownAudioComponent->bAutoActivate = false; // 자동 재생 비활성화

	DeathPostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("DeathPostProcess")); // 사망 포스트 프로세스 컴포넌트 생성
	DeathPostProcessComponent->SetupAttachment(RootComponent); // 루트 컴포넌트에 부착
	DeathPostProcessComponent->BlendWeight = 0.0f; // 초기 가중치 설정 (죽으면 1.0f로 변경)
	DeathPostProcessComponent->bEnabled = true; // 컴포넌트 활성화
}

void AMainCharacter::BeginPlay()
{
    Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) // 플레이어 컨트롤러 가져옴
    {
		if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) // 인핸스드 인풋 서브시스템 가져옴
        {
			SubSystem->AddMappingContext(DefaultMappingContext, 0); // 매핑 컨텍스트 추가
        }
    }

	if (GetMesh()) // 메시가 있으면
    {
        USkeletalMeshComponent* CharacterMesh = GetMesh();
        if (CharacterMesh)
        {
			CachedAnimInstance = Cast<UCustomAnimInstance>(CharacterMesh->GetAnimInstance()); // 커스텀 애니메이션 인스턴스 캐싱
        }
    }

	if (RifleClass) // 라이플 클래스가 설정되어 있으면
    {
		Rifle = GetWorld()->SpawnActor<ARifle>(RifleClass); // 라이플 인스턴스 생성
        if (Rifle) 
        {
			Rifle->SetOwner(this); // 소유자 설정
			AttachRifleToBack(); // 등 뒤에 부착
        }
    }

	if (KnifeClass_L) // 왼쪽 칼 클래스가 설정되어 있으면
    {
		LeftKnife = GetWorld()->SpawnActor<AKnife>(KnifeClass_L); // 왼쪽 칼 인스턴스 생성
        if (LeftKnife)
        {
			LeftKnife->InitializeKnife(EKnifeType::Left); // 왼쪽 칼 초기화
			LeftKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("KnifeSocket_L")); // 왼쪽 칼을 소켓에 부착
			LeftKnife->SetOwner(this); // 소유자 설정
        }
    }

	if (KnifeClass_R) // 오른쪽 칼 클래스가 설정되어 있으면
    {
		RightKnife = GetWorld()->SpawnActor<AKnife>(KnifeClass_R); // 오른쪽 칼 인스턴스 생성
        if (RightKnife)
        {
			RightKnife->InitializeKnife(EKnifeType::Right); // 오른쪽 칼 초기화
			RightKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("KnifeSocket_R")); // 오른쪽 칼을 소켓에 부착
			RightKnife->SetOwner(this); // 소유자 설정
        }
    }

	if (MeleeCombatComponent) // 밀리어택 컴포넌트가 있으면
    {
		MeleeCombatComponent->InitializeCombatComponent(this, KickHitBox, LeftKnife, RightKnife); // 밀리어택 컴포넌트 초기화
		TArray<UAnimMontage*> Montages; // 콤보 몽타주 배열 생성
        // 콤보 몽타주 추가
		Montages.Add(ComboAttackMontage1);
        Montages.Add(ComboAttackMontage2); 
        Montages.Add(ComboAttackMontage3);
        Montages.Add(ComboAttackMontage4);
        Montages.Add(ComboAttackMontage5);
		MeleeCombatComponent->SetComboMontages(Montages); // 콤보 몽타주 설정
    }

	if (MachineGunClass) // 머신건 클래스가 설정되어 있으면
    {
		MachineGun = GetWorld()->SpawnActor<AMachineGun>(MachineGunClass); // 머신건 인스턴스 생성
        if (MachineGun)
        {
            MachineGun->SetActorHiddenInGame(true); // 초기엔 숨김
			MachineGun->SetOwner(this); // 소유자 설정
        }
    }

	if (CannonClass) // 캐논 클래스가 설정되어 있으면
    {
		Cannon = GetWorld()->SpawnActor<ACannon>(CannonClass); // 캐논 인스턴스 생성
        if (Cannon)
        {
            Cannon->SetActorHiddenInGame(true); // 초기엔 숨김
			Cannon->SetOwner(this); // 소유자 설정
        }
    }

	if (SkillComponent) // 스킬 컴포넌트가 있으면
    {
		SkillComponent->InitializeSkills(this, MachineGun, LeftKnife, RightKnife, KickHitBox, Cannon); // 스킬 컴포넌트 초기화
    }

	if (CrossHairWidgetClass) // 크로스헤어 위젯 클래스가 설정되어 있으면
    {
		CrossHairWidget = CreateWidget<UCrossHairWidget>(GetWorld(), CrossHairWidgetClass); // 크로스헤어 위젯 생성
        if (CrossHairWidget) 
        {
			CrossHairWidget->AddToViewport(); // 뷰포트에 추가
			CrossHairWidget->SetVisibility(ESlateVisibility::Hidden); // 초기엔 숨김
			if (!CrossHairWidget->IsComponentValid()) // 컴포넌트 유효성 검사
            {
				CrossHairWidget->SetCrossHairComponentReference(CrosshairComponent); // 컴포넌트 참조 설정
            }
        }
    }

	if (PlayerHUDWidgetClass) // 플레이어 HUD 위젯 클래스가 설정되어 있으면
    {
		PlayerHUDWidget = CreateWidget<UUserWidget>(GetWorld(), PlayerHUDWidgetClass); // 플레이어 HUD 위젯 생성

        if (PlayerHUDWidget)
        {
			PlayerHUDWidget->AddToViewport(); // 뷰포트에 추가
        }
    }

	if (PauseMenuWidgetClass) // 일시정지 메뉴 위젯 클래스가 설정되어 있으면
    {
		APlayerController* PC = Cast<APlayerController>(GetController()); // 플레이어 컨트롤러 가져옴
		if (PC) // 컨트롤러가 유효하면
        {
			PauseMenuWidget = CreateWidget<UUserWidget>(PC, PauseMenuWidgetClass); // 일시정지 메뉴 위젯 생성
            // 일시정지 위젯은 생성만 하고 뷰포트에 추가하지 않음
        }
    }

	if (GameStartSound) // 게임 시작 사운드가 설정되어 있으면
    {
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(),
            GameStartSound,
            GetActorLocation() // 재생 위치를 캐릭터의 위치로 설정
        );
    }

    // 게임 설정 불러오기 및 마우스 감도 적용
    USettingsGameInstance* SettingsGI = Cast<USettingsGameInstance>(UGameplayStatics::GetGameInstance(GetWorld())); // 게임인스턴스 가져옴
    if (SettingsGI && SettingsGI->CurrentSettings) // 유효성 검사
    {
        MouseSensitivityMultiplier = SettingsGI->CurrentSettings->MouseSensitivity; // 게임 인스턴스에 저장된 값을 캐릭터의 변수로 복사
    }
}

void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // 부모 클래스의 Tick 호출

	if (bIsBigHitReacting) // 빅 히트 리액트 중일 때도
    {
		// 에임 피치 값 업데이트
		if (bIsAiming || (SkillComponent && SkillComponent->IsUsingAimSkill1() || (SkillComponent && SkillComponent->IsUsingAimSkill2()))) // 에임 중이거나 에임 스킬 사용 중일 때
        {
			APlayerController* PlayerController = Cast<APlayerController>(GetController()); // 플레이어 컨트롤러 가져옴
			if (PlayerController) // 유효성 검사
            {
				FRotator ControlRotation = PlayerController->GetControlRotation(); // 컨트롤러 회전값 가져옴
				AimPitch = FMath::Clamp(FMath::UnwindDegrees(ControlRotation.Pitch), -90.0f, 90.0f); // 피치 값 계산 및 클램프
            }
        }

        // 애님 인스턴스에 상태 전달
        if (CachedAnimInstance) // 캐싱된 애님 인스턴스 가져옴
        {
            // 1. 빅히트 중에는 이동 불가
            CachedAnimInstance->Speed = 0.0f; // 속도 0
            CachedAnimInstance->Direction = 0.0f; // 방향 0
            // 2. 공중 피격, 조준 중 피격 등 세부적인 애니메이션 분기를 작동시키기 위해 어디서(공중/지상) 어떤 자세(조준)인지는 유지
            CachedAnimInstance->bIsJumping = bIsJumping; // 점프 상태 전달
            CachedAnimInstance->bIsInDoubleJump = bIsInDoubleJump; // 더블점프 상태 전달
            CachedAnimInstance->bIsInAir = bIsInAir; // 공중 상태 전달
            CachedAnimInstance->bIsAiming = bIsAiming; // 에임 상태 전달
            CachedAnimInstance->AimPitch = AimPitch; // 피치 값 전달
            // 3. 스킬 이펙트나 팔의 자세를 결정하기 위해 상태를 지속적으로 업데이트
            CachedAnimInstance->bIsUsingAimSkill1 = (SkillComponent && SkillComponent->IsUsingAimSkill1()); // 에임 스킬1 사용 상태 전달
            CachedAnimInstance->bIsUsingAimSkill2 = (SkillComponent && SkillComponent->IsUsingAimSkill2()); // 에임 스킬2 사용 상태 전달
        }
        return; // 나머지 틱 로직(이동, 줌, 반동 등) 중단
    }

	bool bCurrentlyFalling = GetCharacterMovement()->IsFalling(); // 현재 낙하 중인지 확인
	bool bCurrentlyOnGround = GetCharacterMovement()->IsMovingOnGround(); // 현재 지상에 있는지 확인
	bool bIsCrosshairActive = bIsAiming || (SkillComponent && SkillComponent->IsUsingAimSkill1()); // 크로스헤어 활성화 조건

	if (bCurrentlyFalling && !bIsInAir) // 낙하 중이고 이전에 공중이 아니었다면
    {
		bIsInAir = true; // 공중 상태로 전환
    }
	else if (bCurrentlyOnGround && bIsInAir) // 지상에 있고 이전에 공중이었다면
    {
        // 이미 Landed에서 처리했으므로 중복 처리 방지
        if (!bIsLanding) 
        {
			bIsInAir = false; // 지상 상태로 전환
			bIsJumping = false; // 점프 상태 해제
			bIsInDoubleJump = false; // 더블점프 상태 해제
			bCanDoubleJump = true; // 더블점프 가능 상태로 전환
        }
    }

	FVector Velocity = GetVelocity(); // 현재 속도 벡터(x,y,z) 가져옴
	Speed = Velocity.Size(); // 속도 벡터의 길이를 계산하여 이동 속도 계산
	if (!Velocity.IsNearlyZero()) // 이동 중인지 체크 (0보다 큰지 확인하여 부동소수점 오차 방지)
    {
		FVector ForwardVector = GetActorForwardVector(); // 캐릭터 앞의 방향 벡터 가져옴
		FVector RightVector = GetActorRightVector(); // 캐릭터 오른쪽의 방향 벡터를 가져옴
		FVector NormalizedVelocity = Velocity.GetSafeNormal(); // 속도 벡터를 방향성만 남기기 위해 길이를 1로 만듦 (정규화)
        
        // 내적을 두 번 (Dot Product, Atan2) 하여 캐릭터 정면 기준 이동 방향의 각도를 구함
        float AngleRadians = FMath::Atan2(FVector::DotProduct(NormalizedVelocity, RightVector), // Dot: 캐릭터의 오른쪽과 이동 방향이 얼마나 일치하는지 (오:1 왼: -1)
			FVector::DotProduct(NormalizedVelocity, ForwardVector)); // Dot: 캐릭터의 앞과 이동 방향이 얼마나 차이나는지 (앞: 1 뒤: -1)
        // 이 두값을 Atan2(y,x) 함수에 넣어 캐릭터가 어느 사분면 방향으로 이동 중인지 정확한 각도를 구함
		Direction = FMath::RadiansToDegrees(AngleRadians); // 라디안 단위를 도 단위(-180 ~ 180)로 변환
        // 앞 0도, 오른쪽 90도, 뒤 180도, 왼쪽 -90도
    }
	else if (bIsInAir) // 이동은 멈췄지만 공중에 있을 때 (점프 중 or 낙하 중)
    {
        // 곻중에서 방향 전환 애니메이션이 0으로 튀면 어색하므로 
		Direction = Direction; // 이전 방향 유지
    }
	else // 완전히 지상에 서 있고 이동도 멈췄을 때
    {
		Direction = 0.0f; // 정지 상태 이므로 방향 0으로 설정
    }

	if (GetCharacterMovement()->IsFalling()) // 낙하 중이면
    {
		bIsInAir = true; // 공중 상태로 설정
    }
	else if (GetCharacterMovement()->IsMovingOnGround()) // 지상에 있으면
    {
		bIsInAir = false; // 지상 상태로 설정
		bIsJumping = false; // 점프 상태 해제
		bIsInDoubleJump = false; // 더블점프 상태 해제
		bCanDoubleJump = true; // 더블점프 가능 상태로 전환
    }

    // 애님 인스턴스에 상태 전달
    if (CachedAnimInstance) // 캐싱된 애님 인스턴스 가져옴
    {
        CachedAnimInstance->Speed = Speed; // 이동 속도 전달
        CachedAnimInstance->Direction = Direction; // 이동 방향 전달
        CachedAnimInstance->bIsJumping = bIsJumping; // 점프 상태 전달
        CachedAnimInstance->bIsInDoubleJump = bIsInDoubleJump; // 더블점프 상태 전달
        CachedAnimInstance->bIsInAir = bIsInAir; // 공중 상태 전달
        CachedAnimInstance->bIsAiming = bIsAiming; // 에임 상태 전달
        CachedAnimInstance->AimPitch = AimPitch; // 피치 값 전달
        CachedAnimInstance->bIsUsingAimSkill1 = (SkillComponent && SkillComponent->IsUsingAimSkill1()); // 에임 스킬1 사용 상태 전달
        CachedAnimInstance->bIsUsingAimSkill2 = (SkillComponent && SkillComponent->IsUsingAimSkill2()); // 에임 스킬2 사용 상태 전달
    }

	// 에임 모드 및 카메라 회전 처리
    // 조준 상태나 조준이 필요한 스킬 사용시 캐릭터를 카메라 방향으로 정렬
	if (bIsAiming || (SkillComponent && SkillComponent->IsUsingAimSkill1() || (SkillComponent && SkillComponent->IsUsingAimSkill2()))) // 에임 모드 또는 에임 스킬 사용 중이면
    {
		APlayerController* PlayerController = Cast<APlayerController>(GetController()); // 플레이어 컨트롤러 가져옴
        if (PlayerController) // 유효성 검사
        {
            FRotator ControlRotation = PlayerController->GetControlRotation(); // 컨트롤러가 바라보는 회전값 가져옴
            // UnwindDegrees: 각도를 -180~180 범위로 정규화하여 애니메이션 블루프린트 등에서 사용하기 쉽게 가공
            AimPitch = FMath::Clamp(FMath::UnwindDegrees(ControlRotation.Pitch), -90.0f, 90.0f);

            // 대쉬 중이거나 점프중이거나 착지중이라면 회전하지 않음 (이 액션 중 강제로 회전하면 애니메이션이 어색해지거나 물리 판정이 꼬이는 것을 방지)
            if (!bIsDashing && !bIsJumping && !bIsInDoubleJump && !bIsLanding)
            {
                FVector LastInput = GetCharacterMovement()->GetLastInputVector(); // 마지막 이동 입력 벡터 가져옴

                // 이동 중이면 카메라 방향을 따라 캐릭터 회전   
                if (!LastInput.IsNearlyZero()) // 이동 중인지 체크 (0보다 큰지 확인하여 부동소수점 오차 방지)
                {
                    FRotator NewRotation(0.0f, ControlRotation.Yaw, 0.0f); // 카메라의 Yaw 값으로 새로운 회전값 생성
                    SetActorRotation(NewRotation); // 캐릭터 회전 설정
                }
                // 이동 입력이 없더라도 카메라 방향을 따라 캐릭터 회전
                else
                {
                    FRotator NewRotation(0.0f, ControlRotation.Yaw, 0.0f); // 카메라의 Yaw 값으로 새로운 회전값 생성
                    SetActorRotation(NewRotation); // 캐릭터 회전 설정
                }
            }
        }
        CurrentZoom = FMath::FInterpTo(CurrentZoom, AimZoom, DeltaTime, ZoomInterpSpeed); // 에임모드 전용 줌값으로 보간하여 전환
    }
    else
    {
        CurrentZoom = FMath::FInterpTo(CurrentZoom, TargetZoom, DeltaTime, ZoomInterpSpeed); // 에임모드가 아닐땐 일반 줌값으로 보간하여 전환
    }

	CameraBoom->TargetArmLength = CurrentZoom; // 계산된 줌 값을 실제 카메라 붐 길이에 적용

	// 루트모션 회전 적용 처리
    // 콤보공격과 같은 애니메이션이 강제로 캐릭터 회전을 제어할때 우선 순위 부여
	if (bApplyRootMotionRotation) // 루트모션 회전 적용 중이면
    {
        // 애니메이션에 포함된 회전 데이터(Root Motion)가 있을 경우, 위의 카메라 기반 회전을 덮어쓰고 고정
        UE_LOG(LogTemp, Warning, TEXT("Root Motion Applied! Rotation Set To: %s"), *TargetRootMotionRotation.ToString());
        SetActorRotation(TargetRootMotionRotation); // 루트모션이 적용되는 동안 방향유지 
    }

    // 이동 중 크로스헤어 확산 적용
	if (bIsAiming && CrosshairComponent) // 조준중이고 크로스헤어 컴포넌트가 유효하면
    {
        // 이동 중 크로스헤어 확산 증가
		float MovementSpeed = GetVelocity().Size(); // 현재 이동 속도 계산
		float MovementSpread = 0.0f; // 이동 확산 초기화

        if (MovementSpeed > 50.0f) // 이동 중일 때
        {
            // 이동 속도에 비례하여 크로스헤어 확산 증가
			MovementSpread = FMath::Clamp(MovementSpeed / 600.0f * 20.0f, 0.0f, 30.0f); // 최대 확산값 30으로 클램프
        }

		CrosshairComponent->SetMovementSpread(MovementSpread); // 크로스헤어 컴포넌트에 이동 확산 값 설정
		CrosshairComponent->SetCrosshairActive(true); // 크로스헤어 활성화

        // 반동 업데이트는 라이플 에임 중에만 작동
		if (bIsAiming && bIsRecoiling) // 반동 적용 중이면
        {
			UpdateRecoil(DeltaTime); // 반동 업데이트
        }
    }
    else
    {
        // 에임 모드가 아닐 때는 크로스헤어 비활성화
        if (CrosshairComponent)
        {
			CrosshairComponent->SetCrosshairActive(false); // 크로스헤어 비활성화
        }
    }

	UpdateMovementSpeed(); // 이동 속도에 따른 효과 업데이트
}

// 이동 및 회전 처리 함수들
void AMainCharacter::Move(const FInputActionValue& Value) // 이동 입력 처리
{
    if (SkillComponent && (SkillComponent->IsUsingAimSkill1() || SkillComponent->IsUsingAimSkill2())) return; // 특정 스킬 사용 중에는 이동 불가
	if (!Controller) return; // 컨트롤러 유효성 검사
	if (bIsBigHitReacting || bIsDead) return; // 빅 히트 리액트 중이거나 사망 상태일 때 이동 불가

    FVector2D MovementVector = Value.Get<FVector2D>(); // 입력 값 추출 (좌/우: X축, 상/하: Y축)

    // 이동 방향 계산
	const FRotator Rotation = Controller->GetControlRotation(); // 컨트롤러의 회전값 가져옴
    const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f); // 컨트롤러의 yaw(좌우회전) 만 사용하여 지면과 평행한 회전값 생성
    // 플레이어가 카메라를 돌려도 wasd 이동 입력이 항상 화면 기준으로 작동
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X); // 카메라가 바라보는 기준으로 앞(x축) 방향 벡터 추출
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y); // 카메라가 바라보는 기준으로 오른쪽(y축) 방향 벡터 추출
	// 이동 입력 적용
	AddMovementInput(ForwardDirection, MovementVector.Y); // 앞방향에 상하 입력 ws 를 곱해 이동
    AddMovementInput(RightDirection, MovementVector.X); // 오른쪽방향에 좌우 입력 ad를 곱해 이동
}

// 상태에 따른 속도 관리
void AMainCharacter::UpdateMovementSpeed()
{
	if (!GetCharacterMovement()) return; // 캐릭터 무브먼트 컴포넌트 유효성 검사

    // 에임모드 또는 특정 스킬 사용 중일 때 속도 감소
    if (bIsAiming ||
        (SkillComponent && (SkillComponent->IsUsingAimSkill1() || SkillComponent->IsUsingAimSkill2())))
    {
		GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed; // 감속된 속도(aimwalkspeed) 적용
    }
	else
    {
		GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed; // 기본 속도 적용
    }
}

// 마우스 입력 처리
void AMainCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookVector = Value.Get<FVector2D>(); // 입력 값 추출 (좌/우: X축, 상/하: Y축)

	if (!Controller) return; // 컨트롤러 유효성 검사
	if (bIsDead) return; // 사망 상태일 때 회전 불가

	AddControllerYawInput(LookVector.X * MouseSensitivityMultiplier); // 마우스 X축 입력에 마우스 감도를 곱하여 Yaw(좌우회전) 적용
	AddControllerPitchInput(LookVector.Y * MouseSensitivityMultiplier); // 마우스 Y축 입력에 마우스 감도를 곱하여 Pitch(상하회전) 적용
}

// 점프 및 더블점프 처리
void AMainCharacter::HandleJump() 
{
	if (bIsBigHitReacting || bIsDead) return; // 빅 히트 리액트 중이거나 사망 상태일 때 점프 불가

    if (SkillComponent &&
        (SkillComponent->IsUsingSkill1() || SkillComponent->IsUsingSkill2() || SkillComponent->IsUsingSkill3() ||
            SkillComponent->IsUsingAimSkill1() || SkillComponent->IsUsingAimSkill2() || SkillComponent->IsUsingAimSkill3()))
		return; // 스킬 사용 중일 때 점프 불가
	if (bIsDead) return; // 사망 상태일 때 점프 불가

    // 대쉬 중이거나 착지 애니메이션 재생 중일 때 점프 불가
    if (bIsDashing || bIsPlayingLandingAnimation) return;

    // 빠른 연타 방지를 위해 이미 공중에 있고 아직 착지하지 않았다면 더블점프만 허용
    if (GetCharacterMovement()->IsFalling() && !bCanDoubleJump)
    {
        return;  // 이미 더블점프를 사용했다면 점프 불가
    }

    // 지상에 있을때 
    if (!bIsJumping && GetCharacterMovement()->IsMovingOnGround()) 
    {
		LaunchCharacter(FVector(0, 0, 500), false, true); // 기본 점프 힘 적용
		bIsJumping = true; // 점프 상태로 설정
        bIsInAir = true;  // 공중 상태로 설정
    }
	// 공중에 있을때 더블점프 처리
    else if (bCanDoubleJump && GetCharacterMovement()->IsFalling())
    {
		HandleDoubleJump(); // 더블점프 처리 함수 호출
    }
}

// 더블점프 처리 함수
void AMainCharacter::HandleDoubleJump()
{
	if (bIsBigHitReacting || bIsDead) return; // 빅 히트 리액트 중이거나 사망 상태일 때 점프 불가

	LaunchCharacter(FVector(0, 0, 1200), false, true); // 더블점프 힘 적용
	bCanDoubleJump = false; // 더블점프 사용 완료
	bIsInDoubleJump = true; // 더블점프 상태로 설정

	GetCharacterMovement()->GravityScale = 2.3f; // 더블 점프 후 중력을 증가시켜 빠르게 착지 (기본값: 1.0f)
    GetCharacterMovement()->FallingLateralFriction = 0.1f;  // 낙하 중 마찰력 감소로 착지 속도 증가 (기본값: 0.5f)

	if (UCustomAnimInstance* AnimInstance = Cast<UCustomAnimInstance>(GetMesh()->GetAnimInstance())) // 애님 인스턴스 가져옴
    {
		AnimInstance->bIsInDoubleJump = true; // 더블점프 상태 전달
    }
}

// 착지 처리 함수
void AMainCharacter::Landed(const FHitResult& Hit) 
{
	Super::Landed(Hit); // 부모 클래스 ACharacter의 기본 착지 로직 실행 (필수)
	UWorld* World = GetWorld(); // 월드 객체 가져오기
	if (!World) return; // 월드 유효성 검사

	bIsJumping = false; // 점프 상태 해제
	bIsInDoubleJump = false; // 더블점프 상태 해제
	bIsInAir = false; // 공중 상태 해제
	bCanDoubleJump = true; // 더블점프 가능 상태로 전환
	bIsLanding = true; // 착지 상태 활성화
    bIsPlayingLandingAnimation = true;  // 착지 애니메이션 재생 상태 활성화

    TWeakObjectPtr<AMainCharacter> WeakThis(this);

    // 착지 애니메이션 종료 타이머
    World->GetTimerManager().SetTimer(LandingAnimationTimerHandle, [WeakThis]()
        {
            if (WeakThis.IsValid()) WeakThis->OnLandingAnimationFinished();
        }, 0.5f, false);

    // 착지 상태 리셋 타이머
    World->GetTimerManager().SetTimer(LandingTimerHandle, [WeakThis]()
        {
            if (WeakThis.IsValid()) WeakThis->ResetLandingState();
        }, 0.2f, false);

    // 캐릭터 무브먼트 복구
    if (IsValid(GetCharacterMovement()))
    {
        GetCharacterMovement()->GravityScale = 1.0f;
        GetCharacterMovement()->FallingLateralFriction = 0.5f;
    }
	//// 타이머를 사용하여 착지 애니메이션 재생 시간 동안 점프 불가 상태 유지
 //   float LandingAnimDuration = 0.5f;  // 착지 애니메이션 지속 시간

 //   if (UWorld* World = GetWorld())
 //   {
 //       // 애니메이션 종료 타이머: 착지 애니메이션 지속 시간 후 OnLandingAnimationFinished 호출
 //       World->GetTimerManager().SetTimer(LandingAnimationTimerHandle, this, &AMainCharacter::OnLandingAnimationFinished, LandingAnimDuration, false);
 //       // 착지 상태 리셋 타이머: 0.2초 뒤에 ResetLandingState 호출
 //       // bIsLanding을 짧게 유지하여 착지 직후 다시 점프하거나 이동할 수 있도록 함
	//	World->GetTimerManager().SetTimer(LandingTimerHandle, this, &AMainCharacter::ResetLandingState, 0.2f, false);

 //   }

 //   // 낙하시 조정 되었던 중력과 낙하 속도 즉시 복구
	//GetCharacterMovement()->GravityScale = 1.0f; // 기본 중력 값으로 복구
	//GetCharacterMovement()->FallingLateralFriction = 0.5f; // 기본 마찰력 값으로 복구
}

// 착지 애니메이션 종료 처리 함수
void AMainCharacter::OnLandingAnimationFinished()
{
	bIsPlayingLandingAnimation = false; // 착지 애니메이션 재생 상태 해제
}

// 착지 상태 리셋 함수
void AMainCharacter::ResetLandingState()
{
	bIsLanding = false; // 착지 상태 해제
}

// 대시 처리 함수
void AMainCharacter::Dash()
{
	UWorld* World = GetWorld(); // 월드 객체 가져오기
	if (!World) return; // 월드 유효성 검사

	if (!bCanDash || bIsDashing || !Controller || bIsJumping || bIsInDoubleJump || bIsBigHitReacting || bIsDead) return; 

	// 움직이지 못하고 사용하는 에임 스킬1 사용 중일 때 대시가 가능하고 스킬을 캔슬
	if (SkillComponent) // 스킬 컴포넌트가 유효할 때
    {
		if (SkillComponent->IsUsingAimSkill1()) // 에임스킬1 사용 중일 때
        {
            // 스킬 컴포넌트에 캔슬 및 쿨타임 재시작을 요청
            SkillComponent->CancelAimSkill1ByDash();
        }
        // 비교적 움직이지 못하는 시간이 짧은 에임스킬2는 캔슬 불가
		else if (SkillComponent->IsUsingAimSkill2()) // 에임스킬2 사용 중일 때
        {
			return; // 대시 불가
        }
    }

	// 대시 방향 결정
	FVector DashDirection = FVector::ZeroVector; // 대시 방향 초기화
	FVector InputVector = GetCharacterMovement()->GetLastInputVector(); // 마지막 이동 입력 벡터 가져옴

	if (!InputVector.IsNearlyZero()) // 이동 입력이 있다면
    {
		DashDirection = InputVector.GetSafeNormal(); // 입력 벡터를 정규화하여 대시 방향 설정
    }
    else // 이동 입력이 없다면
    {
		DashDirection = GetActorForwardVector(); // 캐릭터의 앞 방향 벡터 사용
    }

    // 방향 벡터 설정
	FVector ForwardVector; // 캐릭터 앞 방향 벡터
	FVector RightVector; // 캐릭터 오른쪽 방향 벡터

	if (bIsAiming) // 에임 모드일 때
    {
		// 에임모드일때는 캐릭터가 이미 카메라 방향을 향하고 있으므로
		ForwardVector = GetActorForwardVector(); // 캐릭터기준 앞 방향 벡터
		RightVector = GetActorRightVector(); // 캐릭터기준 오른쪽 방향 벡터
    }
    else // 일반 모드일 때
    {
		FRotator ControlRotation = Controller->GetControlRotation(); // 컨트롤러 회전값 가져옴
		FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f); // Yaw 값만 사용하여 지면과 평행한 회전값 생성
		ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X); // 카메라기준 앞 방향 벡터
		RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y); // 카메라기준 오른쪽 방향 벡터
    }

    // 내적을 이용한 4방향 대시 판정
	// 두 벡터가 일치하면 1.0, 직각이면 0.0, 반대면 -1.0
	DashDirection.Normalize(); // 대시 방향 정규화
	float ForwardDot = FVector::DotProduct(DashDirection, ForwardVector); //앞 방향과의 내적
	float RightDot = FVector::DotProduct(DashDirection, RightVector); //오른쪽 방향과의 내적

    // 방향을 확인하는 로그 
    UE_LOG(LogTemp, Warning, TEXT("DashDirection: %s"), *DashDirection.ToString());
    UE_LOG(LogTemp, Warning, TEXT("ForwardVector: %s, RightVector: %s"), *ForwardVector.ToString(), *RightVector.ToString());
    UE_LOG(LogTemp, Warning, TEXT("ForwardDot: %f, RightDot: %f"), ForwardDot, RightDot);

	UAnimMontage* DashMontage = nullptr; // 선택된 대시 애니메이션 몽타주 재생을 위한 선언

    // 대쉬 판별 로직 
    if (ForwardDot > 0.8f) // 0.8보다 크다면 정면으로 간주
    {
		DashMontage = ForwardDashMontage; // 앞으로 대쉬 몽타주 선택
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: ForwardDashMontage"));
    }
	else if (ForwardDot > 0.6f && RightDot > 0.6f) // 전방 0.6 이상, 우측 0.6 이상 이라면 오른쪽 앞으로 간주
    {
		DashMontage = RightDashMontage; // 오른쪽 앞으로 대쉬
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: RightDashMontage (Front-Right)"));
    }
	else if (ForwardDot > 0.6f && RightDot < -0.6f) // 전방 0.6 이상, 좌측 -0.6 이하 라면 왼쪽 앞으로 간주
    {
		DashMontage = LeftDashMontage; // 왼쪽 앞으로 대쉬
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: LeftDashMontage (Front-Left)"));
    }
	else if (ForwardDot < -0.3f) // 전방 -0.3 이하 라면 후방으로 간주
    {
		DashMontage = BackwardDashMontage; // 뒤로 대쉬
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: BackwardDashMontage"));
    }
	else if (RightDot > 0.5f) // 우측 0.5 이상 이라면 오른쪽으로 간주
    {
		DashMontage = RightDashMontage; // 오른쪽으로 대쉬
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: RightDashMontage"));
    }
	else if (RightDot < -0.5f) // 우측 -0.5 이하 라면 왼쪽으로 간주
    {
		DashMontage = LeftDashMontage; // 왼쪽으로 대쉬
        UE_LOG(LogTemp, Warning, TEXT("Selected Dash Montage: LeftDashMontage"));
    }

	if (!DashMontage) // 유효한 대시 몽타주가 없으면 대시 취소
    {
        UE_LOG(LogTemp, Error, TEXT("Dash Failed: No valid DashMontage found!"));
        return;
    }

    // 대시 실행
	SetActorRotation(DashDirection.Rotation()); // 대시 방향으로 캐릭터 회전
	bIsDashing = true; // 대시 상태 활성화
	bCanDash = false; // 대시 쿨타임 시작
	PlayDashMontage(DashMontage); // 방향에 따라 선택된 대시 애니메이션 재생

	TWeakObjectPtr<AMainCharacter> WeakThis(this); // 약참조 생성
	World->GetTimerManager().SetTimer(DashCooldownTimerHandle, [WeakThis]() // 대시 쿨타임 타이머 설정
        {
            if (WeakThis.IsValid())
            {
                WeakThis->ResetDashCooldown(); // 쿨타임 해제 함수 호출
            }
		}, DashCooldown, false); // 타이머 (한번만 실행)
}

void AMainCharacter::PlayDashMontage(UAnimMontage* DashMontage)
{
    if (!DashMontage) // 애니메이션이 없으면 실행 취소
    {
        UE_LOG(LogTemp, Error, TEXT("PlayDashMontage Failed: DashMontage is NULL"));
        return;
    }

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 애님 인스턴스 가져오기
    if (!AnimInstance) // 애님 인스턴스가 없으면 실행 취소
    {
        UE_LOG(LogTemp, Error, TEXT("PlayDashMontage Failed: AnimInstance is NULL"));
        return;
    }

    float MontageDuration = AnimInstance->Montage_Play(DashMontage, 1.3f); // 대쉬 애니메이션 실행 (1.3배속)

	if (MontageDuration <= 0.0f) // 애니메이션 실행 실패 시
    {
        UE_LOG(LogTemp, Error, TEXT("Montage_Play Failed: %s"), *DashMontage->GetName()); // 애니메이션 실행 실패 로그
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("Montage_Play Success: %s"), *DashMontage->GetName()); // 애니메이션 실행 성공 로그

	// 몽타주가 끝났을때 ResetDash 함수 호출하도록 델리게이트 연결
	FOnMontageEnded EndDelegate;  // 델리게이트 선언
	EndDelegate.BindUObject(this, &AMainCharacter::ResetDash); // 대시 종료 시 호출될 함수 바인딩
	AnimInstance->Montage_SetEndDelegate(EndDelegate, DashMontage); // 델리게이트 설정
}

// 대시 상태 초기화 함수
void AMainCharacter::ResetDash(UAnimMontage* Montage, bool bInterrupted)
{
	bIsDashing = false; // 대시 상태 해제

    // 대시 종료 시 에임 모드 상태라면 컨트롤러 방향을 바라보게 조정
    if (bIsAiming)
    {
		if (Controller) // 컨트롤러 유효성 검사
        {
			FRotator ControlRotation = Controller->GetControlRotation(); // 컨트롤러 회전값 가져옴
			FRotator NewRotation(0.0f, ControlRotation.Yaw, 0.0f); // 카메라의 Yaw 값으로 새로운 회전값 생성
			SetActorRotation(NewRotation); // 캐릭터 회전 설정
            UE_LOG(LogTemp, Warning, TEXT("Dash Reset! Character Rotated to Aim Direction."));
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Dash Reset! Ready for Next Dash."));
}

// 대시 쿨타임 해제 함수
void AMainCharacter::ResetDashCooldown()
{
	bCanDash = true; // 대시 가능 상태로 변경
    UE_LOG(LogTemp, Warning, TEXT("Dash Cooldown Over: Ready to Dash Again."));
}

// 기본모드 또는 에임스킬사용 시 라이플을 등에 부착하는 함수
void AMainCharacter::AttachRifleToBack()
{
	if (IsValid(Rifle)) // 라이플이 유효할 때
    {
		Rifle->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("BackGunSocket")); // 소켓에 부착
        // 하드 코딩 된 수치 (에디터 내부의 소켓 설정에 따라 조정 가능)
		Rifle->SetActorRelativeLocation(RifleBackLocation); // 위치 조정
		Rifle->SetActorRelativeRotation(RifleBackRotation); // 회전 조정
		Rifle->SetOwner(this); // 총기의 소유자를 캐릭터로 설정
		UE_LOG(LogTemp, Warning, TEXT("Rifle Owner Set to: %s"), *GetName()); // 소유자 확인 로그
    }
}

// 에임모드 진입시 라이플을 손에 부착하는 함수
void AMainCharacter::AttachRifleToHand()
{
	if (IsValid(Rifle)) // 라이플이 유효할 때
    {
		Rifle->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("GunSocket")); // 소켓에 부착
		Rifle->SetActorRelativeLocation(FVector::ZeroVector); // 위치 초기화
		Rifle->SetActorRelativeRotation(FRotator::ZeroRotator); // 회전 초기화
		Rifle->SetOwner(this); // 총기의 소유자를 캐릭터로 설정
		UE_LOG(LogTemp, Warning, TEXT("Rifle Owner Set to: %s"), *GetName()); // 소유자 확인 로그
    }
}

// 에임모드 진입시 양칼을 허리에 부착하는 함수
void AMainCharacter::AttachKnifeToBack()
{
	if (IsValid(LeftKnife)) // 왼쪽 칼이 유효할 때
    {
		LeftKnife->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); // 왼쪽손에 있던 부착 해제
		LeftKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("BackKnifeSocket_L")); // 왼쪽 허리 소켓에 부착
        // 하드 코딩 된 수치 (에디터 내부의 소켓 설정에 따라 조정 가능)
        LeftKnife->SetActorRelativeLocation(LeftKnifeBackLocation); // 위치 조정
		LeftKnife->SetActorRelativeRotation(LeftKnifeBackRotation); // 회전 조정
        UE_LOG(LogTemp, Warning, TEXT("LeftKnife moved to BackKnifeSocket_L!"));

		// 부착 확인 로그
		if (LeftKnife->IsAttachedTo(GetMesh()->GetOwner())) // 부착 확인
        {
            UE_LOG(LogTemp, Warning, TEXT("LeftKnife is successfully attached to BackKnifeSocket_L!"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("LeftKnife failed to attach to BackKnifeSocket_L!"));
        }
    }

	if (IsValid(RightKnife)) // 오른쪽 칼이 유효할 때
    {
		RightKnife->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); // 오른손에 있던 부착 해제
		RightKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("BackKnifeSocket_R")); // 오른쪽 허리 소켓에 부착
        // 하드 코딩 된 수치 (에디터 내부의 소켓 설정에 따라 조정 가능)
        RightKnife->SetActorRelativeLocation(RightKnifeBackLocation); // 위치 조정
		RightKnife->SetActorRelativeRotation(RightKnifeBackRotation); // 회전 조정
        UE_LOG(LogTemp, Warning, TEXT("RightKnife moved to BackKnifeSocket_R!"));

		// 부착 확인 로그
		if (RightKnife->IsAttachedTo(GetMesh()->GetOwner())) // 부착 확인
        {
            UE_LOG(LogTemp, Warning, TEXT("RightKnife is successfully attached to BackKnifeSocket_R!"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RightKnife failed to attach to BackKnifeSocket_R!"));
        }
    }
}

// 기본모드 진입시 양칼을 손에 부착하는 함수
void AMainCharacter::AttachKnifeToHand()
{
	if (IsValid(LeftKnife)) // 왼쪽 칼이 유효할 때
    {
		LeftKnife->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); // 허리에 있던 부착 해제
		LeftKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("KnifeSocket_L")); // 왼손 소켓에 부착
		LeftKnife->SetActorRelativeLocation(FVector::ZeroVector); // 위치 초기화
		LeftKnife->SetActorRelativeRotation(FRotator::ZeroRotator); // 회전 초기화
		LeftKnife->SetActorHiddenInGame(false); // 보이게 설정
        UE_LOG(LogTemp, Warning, TEXT("LeftKnife moved to hand and visible!"));
    }

	if (IsValid(RightKnife)) // 오른쪽 칼이 유효할 때
    {
		RightKnife->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); // 허리에 있던 부착 해제
		RightKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("KnifeSocket_R")); // 오른손 소켓에 부착
		RightKnife->SetActorRelativeLocation(FVector::ZeroVector); // 위치 초기화
		RightKnife->SetActorRelativeRotation(FRotator::ZeroRotator); // 회전 초기화
		RightKnife->SetActorHiddenInGame(false); // 보이게 설정

        UE_LOG(LogTemp, Warning, TEXT("RightKnife moved to hand and visible!"));
    }
}

// 라이플 발사 처리 함수
void AMainCharacter::FireWeapon()
{
	if (bIsBigHitReacting || bIsDead) return; // 빅 히트 리액트 중이거나 사망 상태일 때 발사 불가

    if (SkillComponent && (SkillComponent->IsUsingSkill1() || SkillComponent->IsUsingSkill2() || SkillComponent->IsUsingSkill3() || SkillComponent->IsUsingAimSkill1()))
		return; // 스킬 사용 중일 때 발사 불가

	if (bIsAiming && Rifle) // 에임 모드일 때 라이플 발사
    {
        // getter로 받아온 상태를 이용
		if (Rifle->IsReloading()) // 재장전 중인지 확인
        {
            UE_LOG(LogTemp, Warning, TEXT("FireWeapon Blocked: Rifle is Reloading"));
			return; // 재장전 중이면 발사 불가
        }
        // getter로 받아온 상태를 이용
		if (Rifle->GetCurrentAmmo() <= 0) // 총알이 없는지 확인
        {
            UE_LOG(LogTemp, Warning, TEXT("No Ammo. Triggering Reload. No recoil or shake."));
			Rifle->Reload(); // 자동으로 rifle 클래스의 재장전 함수 호출
			return; // 총알이 없으면 발사 불가
        }

        // 총알이 있고 재장전이 아닐때만 리코일, 흔들림, 발사, 크로스헤어 퍼짐
		float CurrentSpreadAngle = 0.0f; // 현재 크로스헤어 분산 각도
		if (CrosshairComponent) // 크로스헤어 컴포넌트가 유효할 때
        {
			CurrentSpreadAngle = CrosshairComponent->GetBulletSpreadAngle(); // 현재 분산 각도 받아오기
			CrosshairComponent->StartExpansion(1.0f); // 크로스헤어 확산 시작
			ApplyCameraRecoil(); // 카메라 반동 적용
        }
		Rifle->Fire(CurrentSpreadAngle); // 라이플 발사 함수 호출
    }
	else // 기본 모드일 때
    {
		ComboAttack(); // 근접 공격 콤보 함수 호출
    }
}

// 라이플 재장전 처리 함수
void AMainCharacter::ReloadWeapon()
{
	if (bIsBigHitReacting || bIsDead) return; // 빅 히트 리액트 중이거나 사망 상태일 때 재장전 불가

	if (!Rifle) // 라이플이 없으면
    {
        return; // 재장전 불가
    }

	Rifle->Reload(); // 라이플 재장전 함수 호출
}

// 에임 모드 진입 처리 함수
void AMainCharacter::EnterAimMode()
{
	if (bIsBigHitReacting || bIsDead) return; // 빅 히트 리액트 중이거나 사망 상태일 때 에임 모드 진입 불가

    if (SkillComponent &&
        (SkillComponent->IsUsingSkill1() ||
            SkillComponent->IsUsingSkill2() ||
            SkillComponent->IsUsingSkill3() ||
            SkillComponent->IsUsingAimSkill1() ||
            SkillComponent->IsUsingAimSkill2()))
		return; // 스킬 사용 중일 때 에임 모드 진입 불가

	if (!bIsAiming) // 아직 에임 모드가 아닐 때
    {
        PreviousZoom = TargetZoom; // 복구를 위해 현재 타겟 줌 값을 저장
		bIsAiming = true; // 에임 모드 상태로 설정
        TargetZoom = AimZoom; // 에임 모드 진입 시 즉시 줌 변경
        UE_LOG(LogTemp, Warning, TEXT("Entered Aim Mode"));
		CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f); // 카메라 위치 조정

        // 조준 진입 사운드 피드백
		if (AimModeEnterSound) // 사운드가 설정되어 있을 때
        {
            // 2D 사운드로 재생
            UGameplayStatics::PlaySound2D(GetWorld(), AimModeEnterSound);
        }

        AttachRifleToHand(); // 라이플은 손으로 이동
		AttachKnifeToBack(); // 칼은 허리로 이동

        // 크로스헤어 활성화
		if (CrosshairComponent) // 크로스헤어 컴포넌트가 유효할 때
        {
			CrosshairComponent->SetCrosshairActive(true); // 크로스헤어 활성화
        }

		if (CrossHairWidget) // 크로스헤어 위젯이 유효할 때
        {
			CrossHairWidget->SetVisibility(ESlateVisibility::Visible); // 위젯을 보이게 설정
        }
    }

    UpdateMovementSpeed(); // 조준 중 이동 속도 조정 함수 호출
}

// 에임 모드 해제 처리 함수
void AMainCharacter::ExitAimMode()
{
	if (bIsDead) return; // 사망 상태일 때 에임 모드 해제 불가

	if (bIsAiming) // 에임 모드일 때
    {
		bIsAiming = false; // 에임 모드 해제
        TargetZoom = PreviousZoom; // 기본값 대신 이전 줌 값으로 복귀
        UE_LOG(LogTemp, Warning, TEXT("Exited Aim Mode"));
		CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f); // 카메라 위치 조정

        AttachRifleToBack(); // 라이플은 등으로 이동
		AttachKnifeToHand(); // 칼은 손으로 이동

        // 크로스헤어 비활성화
		if (CrosshairComponent) // 크로스헤어 컴포넌트가 유효할 때
        {
			CrosshairComponent->SetCrosshairActive(false); // 크로스헤어 비활성화
        }

		if (CrossHairWidget) // 크로스헤어 위젯이 유효할 때
        {
			CrossHairWidget->SetVisibility(ESlateVisibility::Hidden); // 위젯을 숨김
        }
    }

	UpdateMovementSpeed(); // 조준 해제 후 이동 속도 복구 함수 호출
}

// 카메라 반동 적용 함수
void AMainCharacter::ApplyCameraRecoil()
{
	if (!bIsAiming) return; // 에임 모드가 아닐 때 반동 적용 안함

    // 크로스헤어의 분산 정도에 따라 반동 강도 조절
	float SpreadMultiplier = 1.0f; // 가중치 초기화
	if (CrosshairComponent) // 크로스헤어 컴포넌트가 유효할 때
    {
        // NormalizedSpread(0.0~1.0)를 사용하여 반동의 최소/최대 범위를 동적으로 확장
        // 1.0 + (0.0~1.0 * 0.5) 반동 강도가 1.0배에서 1.5배까지 커짐
		SpreadMultiplier = 1.0f + CrosshairComponent->GetNormalizedSpread() * 0.5f;
    }

	// 랜덤 범위를 이용한 수직, 수평 반동 값 생성
    float VerticalRecoil = FMath::RandRange(VerticalRecoilMin, VerticalRecoilMax) * SpreadMultiplier; // 수직 반동: 항상 위쪽으로 (음수고정)
    float HorizontalRecoil = FMath::RandRange(HorizontalRecoilMin, HorizontalRecoilMax) * SpreadMultiplier; // 수평 반동: 좌우 랜덤

    // 2D 벡터에 저장하여 다음 단계에 사용알 목표 반동 값 설정 (X는 수평, Y는 수직)
    TargetRecoil = FVector2D(HorizontalRecoil, -VerticalRecoil); // Y값을 음수로 하여 위쪽 반동
	bIsRecoiling = true; // 반동 상태 활성화

	ApplyCameraShake(); // 카메라 흔들림 함수 호출
    
	if (UWorld* World = GetWorld()) // 월드 유효성 검사
	{
        // 일정 시간 후 반동 초기화 타이머 설정
        World->GetTimerManager().SetTimer(RecoilTimerHandle, this, &AMainCharacter::ResetRecoil, RecoilDuration, false);
	}

    UE_LOG(LogTemp, Warning, TEXT("Camera recoil applied: Horizontal=%.2f, Vertical=%.2f, Spread Multiplier=%.2f"),HorizontalRecoil, VerticalRecoil, SpreadMultiplier);
}

// 반동 초기화 함수
void AMainCharacter::ResetRecoil()
{
	TargetRecoil = FVector2D::ZeroVector; // 목표 반동 값을 0으로 설정하여 초기화
}

// 반동 업데이트 함수
void AMainCharacter::UpdateRecoil(float DeltaTime)
{
    // 현재 반동 값을 목표로 보간하여 처리
    CurrentRecoil = FMath::Vector2DInterpTo(CurrentRecoil, TargetRecoil, DeltaTime, RecoilRecoverySpeed);

	if (Controller) // 컨트롤러가 유효할 때
    {
		// DeltaTime을 곱하여 프레임으로부터 독립적으로 초당 동일한 반동거리를 이동
		AddControllerPitchInput(CurrentRecoil.Y * DeltaTime); // 수직 반동 적용
		AddControllerYawInput(CurrentRecoil.X * DeltaTime); // 수평 반동 적용
    }

	if (TargetRecoil.IsNearlyZero() && CurrentRecoil.Size() < 0.1f) // 목표 반동이 거의 0이고 현재 반동이 작을 때
    {
		bIsRecoiling = false; // 반동 상태 비활성화
		CurrentRecoil = FVector2D::ZeroVector; // 현재 반동 값도 0으로 초기화
    }
}

// 카메라 흔들림 적용 함수
void AMainCharacter::ApplyCameraShake()
{
	UWorld* World = GetWorld(); // 월드 가져오기
	if (!World || !IsValid(Camera)) return; // 월드나 카메라가 유효하지 않으면 종료
	APlayerController* PlayerController = Cast<APlayerController>(GetController()); // 플레이어 컨트롤러 가져옴
	if (!PlayerController) return; // 컨트롤러가 없으면 종료

   
    // X,Y,Z 축에 랜덤한 흔들림 오프셋 값 생성
    FVector ShakeOffset; // 흔들림 오프셋
    ShakeOffset.X = FMath::RandRange(-ShakeIntensity, ShakeIntensity); // X축 흔들림 값
    ShakeOffset.Y = FMath::RandRange(-ShakeIntensity, ShakeIntensity); // Y축 흔들림 값
    ShakeOffset.Z = FMath::RandRange(-ShakeIntensity * 1.0f, ShakeIntensity * 1.0f); // Z축 흔들림 값

    FVector OriginalLocation = Camera->GetRelativeLocation(); // 카메라의 원래 위치를 가져옴
    Camera->SetRelativeLocation(OriginalLocation + ShakeOffset); // 가져온 카메라 위치에 랜덤벡터를 더하여 위치를 설정

    // 일정 시간 후 원래 위치로 복구
    FTimerHandle ShakeResetTimer; // 타이머 핸들
    // 캐릭터가 파괴된 후 타이머가 실행되면 this 접근 시 크래쉬 발생 가능성이 있으므로
    TWeakObjectPtr<AMainCharacter> WeakThis(this); // TWeakObjectPtr로 본인을 감싸서 캡처
    // 람다 함수의 오리지널 로케이션을 캡처해뒀다가 쉐이크듀레이션 후에 다시 대입하여 제자리로 복구
    World->GetTimerManager().SetTimer(ShakeResetTimer, [WeakThis, OriginalLocation]()
        {
            if (WeakThis.IsValid() && WeakThis->Camera) // 람다 실행 시점에 객체가 살아있는지 확인
            {
                WeakThis->Camera->SetRelativeLocation(OriginalLocation); // 원래 위치로 복구
            }
        }, ShakeDuration, false); // 단발성 타이머
}

// 크로스헤어 위젯 표시 함수
void AMainCharacter::ShowCrosshairWidget()
{
	if (CrossHairWidget) // 크로스헤어 위젯이 유효할 때
    {
		CrossHairWidget->SetVisibility(ESlateVisibility::Visible); // 위젯을 보이게 설정
    }
}

// 크로스헤어 위젯 숨김 함수
void AMainCharacter::HideCrosshairWidget()
{
	if (CrossHairWidget) // 크로스헤어 위젯이 유효할 때
    {
		CrossHairWidget->SetVisibility(ESlateVisibility::Hidden); // 위젯을 숨김
    }
}

// 근접 공격 콤보 함수
void AMainCharacter::ComboAttack()
{
	if (bIsBigHitReacting || bIsDead) return; // 빅 히트 리액트 중이거나 사망 상태일 때 공격 불가

	if (!MeleeCombatComponent || !SkillComponent) return; // 컴포넌트 유효성 검사

	if (bIsDashing || SkillComponent->IsUsingAimSkill1()) // 대시 중이거나 에임 스킬 1 사용 중이면
    {
		return; // 공격 불가
    }

	if (MeleeCombatComponent->IsAttacking()) // 이미 공격 중이라면
    {
		return; // 추가 공격 불가
    }

	MeleeCombatComponent->TriggerComboAttack(); // 밀리 컴포넌트의 콤보 공격 함수 트리거
}

// 줌인 함수
void AMainCharacter::ZoomIn()
{
	if (bIsAiming || bIsDead) return; // 에임 모드이거나 사망 상태일 때 줌 불가능

    // TargetZoom - ZoomStep 연산 결과가 MinZoom보다 작아지지 않도록 제한
	TargetZoom = FMath::Clamp(TargetZoom - ZoomStep, MinZoom, MaxZoom); // 줌인

	if (!bIsAiming) // 에임모드가 아닐 때만 PreviousZoom 업데이트
    {
		PreviousZoom = TargetZoom; // 이전 줌 값 저장
    }

    UE_LOG(LogTemp, Warning, TEXT("Zoom In: %f"), CurrentZoom); // 줌인 로그
}

// 줌아웃 함수
void AMainCharacter::ZoomOut()
{
	if (bIsDead || bIsAiming) return; // 사망 상태일 때 줌 불가능

    // TargetZoom + ZoomStep 연산 결과가 MaxZoom을 초과하지 않도록 제한
	TargetZoom = FMath::Clamp(TargetZoom + ZoomStep, MinZoom, MaxZoom); // 줌아웃

    if (!bIsAiming) // 에임모드가 아닐 때만 PreviousZoom 업데이트
    {
		PreviousZoom = TargetZoom; // 이전 줌 값 저장
    }

	UE_LOG(LogTemp, Warning, TEXT("Zoom Out: %f"), CurrentZoom); // 줌아웃 로그
}

// 스킬1 입력처리 함수
void AMainCharacter::UseSkill1()
{
	if (bIsBigHitReacting || bIsDead) return; // 빅 히트 리액트 중이거나 사망 상태일 때 스킬 사용 불가

	UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::UseSkill1 triggered")); // 스킬1 사용 로그
	if (SkillComponent) // 스킬 컴포넌트가 유효할 때
    {
        if (bIsAiming) // 에임모드 일때
        {
			if (SkillComponent->CanUseAimSkill1()) // 스킬컴포넌트의 에임 스킬 1 사용 가능 여부 체크
            {
				UE_LOG(LogTemp, Warning, TEXT("UseAimSkill1")); // 에임 스킬 1 사용 로그
				SkillComponent->UseAimSkill1(); // 에임 스킬 1 사용
            }
            else // 쿨타임일 경우
            {
				PlayCooldownSound(); // 쿨타임 사운드 재생 함수 호출
            }
        }
        else // 일반 스킬 1
        {
			if (SkillComponent->CanUseSkill1()) // 스킬컴포넌트의 스킬 1 사용 가능 여부 체크
            {
				UE_LOG(LogTemp, Warning, TEXT("UseSkill1")); // 스킬 1 사용 로그
				SkillComponent->UseSkill1(); // 스킬 1 사용
            }
            else // 쿨타임일 경우
            {
				PlayCooldownSound(); // 쿨타임 사운드 재생 함수 호출
            }
        }
    }
}

// 스킬2 입력처리 함수
void AMainCharacter::UseSkill2()
{
	if (bIsBigHitReacting || bIsDead) return; // 빅 히트 리액트 중이거나 사망 상태일 때 스킬 사용 불가

	UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::UseSkill2 triggered")); // 스킬2 사용 로그
	if (SkillComponent) // 스킬 컴포넌트가 유효할 때
    {
        if (bIsAiming) // 에임 스킬 2
        {
			if (SkillComponent->CanUseAimSkill2()) // 스킬컴포넌트의 에임 스킬 2 사용 가능 여부 체크
            {
				UE_LOG(LogTemp, Warning, TEXT("UseAimSkill2")); // 에임 스킬 2 사용 로그
				SkillComponent->UseAimSkill2(); // 에임 스킬 2 사용
            }
            else // 쿨타임일 경우
            {
				PlayCooldownSound(); // 쿨타임 사운드 재생 함수 호출
            }
        }
        else // 일반 스킬 2
        {
			if (SkillComponent->CanUseSkill2()) // 스킬컴포넌트의 스킬 2 사용 가능 여부 체크
            {
				UE_LOG(LogTemp, Warning, TEXT("UseSkill2")); // 스킬 2 사용 로그
				SkillComponent->UseSkill2(); // 스킬 2 사용
            }
            else // 쿨타임일 경우
            {
				PlayCooldownSound(); // 쿨타임 사운드 재생 함수 호출
            }
        }
    }
}

// 스킬3 입력처리 함수
void AMainCharacter::UseSkill3()
{
	UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::UseSkill3 triggered")); // 스킬3 사용 로그
	if (bIsBigHitReacting || bIsDead) return; // 빅 히트 리액트 중이거나 사망 상태일 때 스킬 사용 불가
	if (SkillComponent) // 스킬 컴포넌트가 유효할 때
    {
        if (bIsAiming) // 에임 스킬 3
        {
			if (SkillComponent->CanUseAimSkill3()) // 스킬컴포넌트의 에임 스킬 3 사용 가능 여부 체크
            {
				UE_LOG(LogTemp, Warning, TEXT("UseAimSkill3")); // 에임 스킬 3 사용 로그
				SkillComponent->UseAimSkill3(); // 에임 스킬 3 사용
            }
            else // 쿨타임일 경우
            {
				PlayCooldownSound(); // 쿨타임 사운드 재생 함수 호출
            }
        }
        else // 일반 스킬 3
        {
			if (SkillComponent->CanUseSkill3()) // 스킬컴포넌트의 스킬 3 사용 가능 여부 체크
            {
				UE_LOG(LogTemp, Warning, TEXT("UseSkill3")); // 스킬 3 사용 로그
				SkillComponent->UseSkill3(); // 스킬 3 사용
            }
            else // 쿨타임일 경우 
            {
				PlayCooldownSound(); // 쿨타임 사운드 재생 함수 호출
            }
        }
    }
}

// 데미지 처리 함수
float AMainCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f; // 이미 사망한 상태면 데미지 무시

	float DamageApplied = FMath::Min(CurrentHealth, DamageAmount); // 현재 체력보다 큰 데미지는 적용하지 않음
	CurrentHealth -= DamageApplied; // 체력 감소
	UE_LOG(LogTemp, Warning, TEXT("MainCharacter took: %f Damage, Health remaining: %f"), DamageApplied, CurrentHealth); // 데미지 및 남은 체력 로그

    // 히트 사운드 랜덤 재생
	if (NormalHitSounds.Num() > 0) // 배열에 사운드가 있을 때
    {
		int32 RandIndex = FMath::RandRange(0, NormalHitSounds.Num() - 1); // 랜덤 인덱스 생성
		if (NormalHitSounds[RandIndex]) // 사운드가 유효할 때
        {
			UGameplayStatics::PlaySoundAtLocation(this, NormalHitSounds[RandIndex], GetActorLocation()); // 사운드 재생
        }
    }

    if (bIsBigHitReacting) // 빅 히트 리액션 중에는 일반 피격 모션을 생략
    {
		UE_LOG(LogTemp, Warning, TEXT("Damage received during Big Hit - skip normal hit montage")); // 로그 출력
    }
    // 스킬 시전 중이 아닐 때만 히트 몽타주 재생
    else if (SkillComponent && !SkillComponent->IsCastingSkill())
    {
		if (NormalHitMontages.Num() > 0) // 히트 몽타주 배열에 몽타주가 있을 때
        {
			int32 RandomIndex = FMath::RandRange(0, NormalHitMontages.Num() - 1); // 랜덤 인덱스 생성
			UAnimMontage* SelectedMontage = NormalHitMontages[RandomIndex]; // 랜덤 몽타주 선택
			if (SelectedMontage) // 몽타주가 유효할 때
            {
				UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
				if (AnimInstance) // 애니메이션 인스턴스가 유효할 때
                {
					AnimInstance->Montage_Play(SelectedMontage, 1.0f); // 몽타주 재생
                }
            }
        }
    }
	else // 스킬 시전 중일 때
    {
		UE_LOG(LogTemp, Warning, TEXT("Damage received during skill cast - skip hit montage")); // 로그 출력
    }

	// 체력이 0 이하로 떨어지면
	if (CurrentHealth <= 0.0f) // 사망 처리
    {
		Die(); // 사망 함수 호출
    }

	return DamageApplied; // 적용된 데미지 반환
}

// 빅 히트 리액션 재생 함수
void AMainCharacter::PlayBigHitReaction()
{
	UWorld* World = GetWorld(); // 월드 가져오기
	if (!World) return; // 월드가 없으면 종료
	if (!IsValid(SkillComponent)) return; // 스킬 컴포넌트가 유효하지 않으면 종료
	if (bIsDead || bIsBigHitReacting) return; // 사망 상태이거나 이미 빅 히트 리액션 중이면 종료

	if (!BigHitMontage || !BigHitRecoverMontage) // 몽타주가 설정되지 않았으면 종료
    {
        UE_LOG(LogTemp, Error, TEXT("PlayBigHitReaction Failed: Montages not set!")); 
        return;
    }

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
	if (!IsValid(AnimInstance)) return; // 애니메이션 인스턴스가 없으면 종료

    // 빅 히트 상태로 전환
    bIsBigHitReacting = true;
    // 모든 스킬 및 에임 모드 강제 캔슬
    if (SkillComponent)
    {
		SkillComponent->CancelAllSkills(); // 스킬 강제 취소
    }
    if (bIsAiming)
    {
		ExitAimMode(); // 에임 모드 종료
    }
    // 빅 히트 몽타주 재생
	float MontageDuration = AnimInstance->Montage_Play(BigHitMontage, 1.0f); // 몽타주 재생
    // 빅 히트 사운드 재생
    if (BigHitSound)
    {
		UGameplayStatics::PlaySoundAtLocation(this, BigHitSound, GetActorLocation());
    }
    // 넘어지는 동작인 첫 번째 몽타주의 80% 지점에서 다시 일어나는 리커버 몽타주를 재생하도록 타이머 설정
	TWeakObjectPtr<AMainCharacter> WeakThis(this); // 람다 내부에서 this를 안전하게 참조하기 위해 TWeakObjectPtr 사용
	if (MontageDuration > 0.0f) // 몽타주 재생에 성공했을 때
    {
		World->GetTimerManager().SetTimer(BigHitRecoverTimerHandle, [WeakThis]() // 람다 함수
            {
				if (WeakThis.IsValid()) // 람다 실행 시점에 객체가 살아있는지 확인
                {
                    WeakThis->StartBigHitRecover(); // 리커버 몽타주 재생
                }
            }, MontageDuration * 0.8f, false); // 80퍼센트 재생되었을때만 단발성으로
    }
    else
    {
        // 몽타주 재생 실패 시 (길이가 0) 즉시 리커버리 시도
        StartBigHitRecover();
    }
}

// 빅 히트 몽타주 80% 지점에서 타이머로 호출될 함수
void AMainCharacter::StartBigHitRecover()
{
    // 사망했다면 리커버 재생 안 함
    if (bIsDead)
    {
        bIsBigHitReacting = false; // 빅히트 상태 해제
        return;
    }

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
	if (!AnimInstance || !BigHitRecoverMontage) // 몽타주가 없으면
    {
        bIsBigHitReacting = false; // 빅히트 상태 해제
        return;
    }
	UE_LOG(LogTemp, Warning, TEXT("Big Hit 90%% reached. Playing Recover Montage...")); // 로그 출력
    AnimInstance->Montage_Play(BigHitRecoverMontage, 1.0f); // 두 번째 리커버리 몽타주 재생

    // 애니메이션이 완전히 끝나면 제어권을 돌려받기 위해 델리게이트 설정
    FOnMontageEnded EndDelegate; // 델리게이트 선언
	EndDelegate.BindUObject(this, &AMainCharacter::OnBigHitRecoverMontageEnded); // 델리게이트 바인딩
	AnimInstance->Montage_SetEndDelegate(EndDelegate, BigHitRecoverMontage); // 몽타주 종료 델리게이트 설정
}

// 빅 히트 리커버 몽타주가 끝났을 때 호출됨
void AMainCharacter::OnBigHitRecoverMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Warning, TEXT("Big Hit RECOVERED. Character control restored."));

    // 빅 히트 상태를 최종적으로 해제하여 모든 행동 가능하도록 복구
    bIsBigHitReacting = false;
}

// 사망 처리 함수
void AMainCharacter::Die()
{
	if (bIsDead) return; // 이미 사망 상태이면 종료
	bIsDead = true; // 사망 상태로 설정
	bIsBigHitReacting = false; // 빅 히트 리액션 상태 해제
	ExitAimMode(); // 1. 에임 모드 강제 종료

    // 캡슐 컴포넌트의 충돌 설정 제거
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (Capsule)
    {
        // 모든 충돌 반응을 무시하도록 설정
        Capsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
        // 충돌 자체를 비활성화 (물리 엔진 연산에서 제외)
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 메시의 충돌 설정
	if (GetMesh()) // 메시가 유효할 때
    {
        // 폰채널 충돌 무시
        GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
        // 지면(WorldStatic)은 여전히 막기(Block) 상태로 유지하여 땅에 꺼지지 않게 함
        GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
    }

    // 카메라 줌값 초기화
    CurrentZoom = TargetZoom = DefaultZoom;
    CameraBoom->TargetArmLength = CurrentZoom;

	// 애님 인스턴스에 사망 플래그 설정
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
        if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
        {
            // ABP에 bIsAiming 플래그가 존재하지만 C++에서 이미 ExitAimMode()를 통해 bIsAiming = false를 설정했으므로 
            // ABP가 이를 인식하는 데 걸리는 딜레이를 줄여야 하므로 애니메이션 업데이트 우선순위를 높여 다음 틱에서 즉시 반영되도록 함
            MeshComp->SetTickGroup(ETickingGroup::TG_PostPhysics);
        }
	}

	if (SkillComponent) // 유효성 검사  
    {
		SkillComponent->CancelAllSkills(); // 스킬 강제 취소
    }

	if (DieSound) // 유효성 검사
    {
		UGameplayStatics::PlaySoundAtLocation(this, DieSound, GetActorLocation()); // 사망 사운드 재생
    }

	if (DieMontages.Num() > 0) // 사망 몽타주 배열에 몽타주가 있을 때
    {
		int32 RandomIndex = FMath::RandRange(0, DieMontages.Num() - 1); // 랜덤 인덱스 생성
		UAnimMontage* SelectedMontage = DieMontages[RandomIndex]; // 랜덤 몽타주 선택
		if (SelectedMontage) // 유효성 검사
        {
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 애니메이션 인스턴스 가져오기
			if (AnimInstance) // 유효성 검사
            {
				AnimInstance->Montage_Play(SelectedMontage, 1.0f); // 몽타주 재생
            }
        }
    }
	UE_LOG(LogTemp, Warning, TEXT("MainCharacter Dead!")); // 사망 로그 출력

    // 사망과 동시에 최고 기록 저장
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode())) // 게임 모드 캐스트
    {
        // 현재 진행 중이던 웨이브 인덱스 - 1이 마지막 클리어 웨이브 인덱스
        int32 LastClearedWaveIndex = GameMode->GetCurrentWaveIndex() - 1;

        // -1보다 큰 경우(최소 0번 웨이브 이상 클리어)에만 저장 시도
        if (LastClearedWaveIndex >= -1)
        {
			GameMode->CheckAndSaveBestWaveRecord(LastClearedWaveIndex); // 최고 기록 저장 함수 호출
        }
    }

	// 포스트 포로세스로 흑백 화면 효과 적용
	if (DeathPostProcessComponent) // 유효성 검사
    {
        // BlendWeight는 UPostProcessComponent의 공개 변수에 직접 접근하여 설정
        DeathPostProcessComponent->BlendWeight = 1.0f; // 효과 강도 즉시 100% 적용
        // 채도(Desaturation)를 0.0으로 설정하여 회색 화면을 만듭니다.
		DeathPostProcessComponent->Settings.bOverride_ColorSaturation = true; // 채도를 0으로 만들기 위해 오버라이드 활성화
        // ColorSaturation 항목에 흑백/회색톤(0.0) 적용
        // FVector4(R, G, B, A) 모두 0.0을 주면 Desaturation(채도 낮아짐) 효과
		DeathPostProcessComponent->Settings.ColorSaturation = FVector4(0.0f, 0.0f, 0.0f, 1.0f); // 알파는 1.0 유지
        // 비네팅 효과를 추가하여 화면 가장자리를 어둡게 만듭니다.
		DeathPostProcessComponent->Settings.bOverride_VignetteIntensity = true; // 비네팅 강도 오버라이드 활성화
		DeathPostProcessComponent->Settings.VignetteIntensity = 0.5f; // 비네팅 강도 설정
    }
}

// 게임 재개 함수
void AMainCharacter::ResumeGame() 
{
	APlayerController* PC = Cast<APlayerController>(GetController()); // PlayerController 가져옴
	if (!IsValid(PC)) return; // 플레이어 컨트롤러가 없으면 종료

	if (IsValid(PauseMenuWidget)) // 유효성 검사
    {
		PauseMenuWidget->RemoveFromParent(); // RemoveFromViewport() 대신 RemoveFromParent() 사용 (위젯이 어디에 있던 관계를 끊음)
    }

    PC->SetPause(false); // 엑터들의 Tick과 물리 연산 재게
    PC->SetShowMouseCursor(false); // 게임 플레이 중 마우스 커서 숨김

	// 입력 모드를 게임 모드로 변경
    FInputModeGameOnly InputMode; // 모든 입력을 UI가 아닌 게임로직으로 전달하기 위해 게임모드로 변경
    PC->SetInputMode(InputMode); // 입력 모드 설정
}

// 게임 재시작 함수
void AMainCharacter::HandleRestartGame()
{
    FName CurrentLevelName = FName(UGameplayStatics::GetCurrentLevelName(this)); // 현재 레벨의 이름을 가져옴
	ShowLoadingScreenAndLoad(CurrentLevelName); // 즉시 재시작을 위해 현재 레벨 이름으로 로드
}

// 게임을 멈추고 메인 메뉴로 돌아가는 함수
void AMainCharacter::HandleBackToMainMenu()
{
	ShowLoadingScreenAndLoad(FName("MainMenutest")); // 메인 메뉴 레벨로 로드
}

void AMainCharacter::TogglePauseMenu()
{
	FName CurrentLevelName = FName(UGameplayStatics::GetCurrentLevelName(this)); // 현재 레벨 이름 가져오기
    if (CurrentLevelName == FName("MainMenutest")) // 메인메뉴인 경우 
    {
        return; // 일시정지 작동 안 함
    }

	APlayerController* PC = Cast<APlayerController>(GetController()); // PlayerController 가져오기
	if (!IsValid(PC)) return; // 플레이어 컨트롤러가 없으면 종료

	if (!IsValid(PauseMenuWidget)) // 일시정지 메뉴 위젯이 유효한지 확인
    {
		UE_LOG(LogTemp, Error, TEXT("PauseMenuWidget is not valid!")); // 오류 로그 출력
        return;
    }

    if (PC->IsPaused()) // 이미 일시 정지 상태라면
    {
        ResumeGame(); // 게임재게
    }
    else // 일시정지 상태가 아니라면
	{
        PauseMenuWidget->AddToViewport(); // 일시정지 위젯을 뷰포트에 추가
        PC->SetPause(true); // 일시정지
        PC->SetShowMouseCursor(true); // 마우스 커서 표시

        // UI 버튼도 클릭하고 일시정지 키인 ESC 키 입력도 받기 위해 입력 모드를 게임 및 UI로 변경
		FInputModeGameAndUI InputMode; // 게임 및 UI 모드 생성
		InputMode.SetWidgetToFocus(PauseMenuWidget->GetCachedWidget()); // 일시정지 화면을 가장 앞에 두기 위해 위젯에 포커스 설정
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // 마우스 잠금 해제
		PC->SetInputMode(InputMode); // 입력 모드 설정
    }
}

// 로딩 화면 표시 및 레벨 로드 함수
void AMainCharacter::ShowLoadingScreenAndLoad(FName LevelName)
{
	APlayerController* PC = Cast<APlayerController>(GetController()); // 플레이어 컨트롤러 가져옴
	if (!IsValid(PC)) return; // 플레이어 컨트롤러가 없으면 종료

    PC->SetPause(false); // 일시정지 해제
	if (IsValid(PauseMenuWidget)) // 일시정지 메뉴 위젯이 유효한지 확인
    {
		PauseMenuWidget->RemoveFromParent(); // 일시정지 메뉴 제거
    }
	// 로딩 화면 위젯 생성 및 뷰포트에 추가
    UUserWidget* LoadingWidget = nullptr; // 위젯 포인터를 저장
	if (LoadingScreenWidgetClass) // 유효성 검사
    {
		LoadingWidget = CreateWidget<UUserWidget>(PC, LoadingScreenWidgetClass); // 로딩 화면 위젯 생성
		if (IsValid(LoadingWidget)) // 유효성 검사
        {
			LoadingWidget->AddToViewport(); // 뷰포트에 추가
			UE_LOG(LogTemp, Warning, TEXT("Loading Screen displayed.")); // 로딩 화면 표시 로그
        }
    }

	FInputModeUIOnly InputMode; // UI 전용 입력 모드 생성
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways); // 뷰포트에 마우스 잠금 설정
	PC->SetInputMode(InputMode); // 입력 모드 설정
	PC->SetShowMouseCursor(false); // 마우스 커서 숨김
	PendingLevelToLoad = LevelName; // 로드할 레벨 이름 저장
	FTimerHandle TimerHandle; // 타이머 핸들 선언
    if (UWorld* World = GetWorld())
    {
		World->GetTimerManager().SetTimer(TimerHandle, this, &AMainCharacter::LoadPendingLevel, 2.0f, false); // 2초 후에 LoadPendingLevel 함수 호출
    }
}

// 레벨 로드 함수
void AMainCharacter::LoadPendingLevel()
{
	if (PendingLevelToLoad != NAME_None) // 유효한 레벨 이름이면
    {
		UGameplayStatics::OpenLevel(this, PendingLevelToLoad, true); // 해당 레벨 로드
		PendingLevelToLoad = NAME_None; // 로드할 레벨 이름 초기화
    }
}

// 마우스 감도 업데이트 함수
void AMainCharacter::UpdateMouseSensitivity(float NewSensitivity) 
{
	MouseSensitivityMultiplier = NewSensitivity; // 새로운 감도 값 설정
}

// 쿨타임 사운드 재생 함수
void AMainCharacter::PlayCooldownSound()
{
	if (!SkillCooldownSound || !IsValid(CooldownAudioComponent)) return; // 유효성 검사

    if (CooldownAudioComponent->IsPlaying()) // 이미 재생중이라면
    {
        return; // 재생하지 않음
    }
	CooldownAudioComponent->SetSound(SkillCooldownSound); // 사운드 설정
	CooldownAudioComponent->Play(); // 사운드 재생
	UE_LOG(LogTemp, Warning, TEXT("Cooldown Sound Played.")); // 로그 출력
}

// 웨이브 준비 사운드 재생 함수
void AMainCharacter::PlayWavePrepareSound()
{
	if (WavePrepareSound) // 유효성 검사
    {
		UGameplayStatics::PlaySound2D(GetWorld(), WavePrepareSound); // 2D 사운드 재생
		UE_LOG(LogTemp, Warning, TEXT("Wave Prepare Sound Played.")); // 로그 출력
    }
    else
    {
		UE_LOG(LogTemp, Error, TEXT("WavePrepareSound is NULL! Cannot play prepare sound.")); // 오류 로그 출력
    }
}

// 웨이브 클리어 보상 지급 함수
void AMainCharacter::GiveReward(float HealthAmount, int32 AmmoAmount)
{
	float HealthBefore = CurrentHealth; // 보상 지급 전 체력 저장
	CurrentHealth = FMath::Clamp(CurrentHealth + HealthAmount, 0.0f, MaxHealth); // 체력 회복 및 최대 체력 초과 방지

	if (CurrentHealth > HealthBefore) // 체력이 증가한 경우에만 로그 출력
    {
        UE_LOG(LogTemp, Warning, TEXT("Character gained %f Health. Current: %f/%f"), HealthAmount, CurrentHealth, MaxHealth); // 체력 증가 로그
    }

    // 탄약지급
	if (IsValid(Rifle)) // 유효성 검사
    {
		Rifle->AddTotalAmmo(AmmoAmount); // 탄약 추가
        UE_LOG(LogTemp, Warning, TEXT("Character gained %d Ammo. Total: %d"), AmmoAmount, Rifle->GetTotalAmmo()); // 탄약 증가 로그
    }

	// 보상 획득 피드백 사운드 재생
    if (RewardSound)
    {
		UGameplayStatics::PlaySound2D(GetWorld(), RewardSound); // 2D 사운드 재생
    }
}

// HUD 에 정보를 반환하는 함수
// 체력 백분율 반환 함수
float AMainCharacter::GetHealthPercent() const // 읽기 전용 함수 const로 BP에서 실행핀 없는 퓨어노드로 사용
{
	if (MaxHealth > 0.0f) // 유효성 검사
    {
		return CurrentHealth / MaxHealth; // 백분율 계산
    }
	return 0.0f; // 최대 체력이 0일 때 0 반환
}

// 탄약 정보 반환 함수
int32 AMainCharacter::GetRifleCurrentAmmo() const // 읽기 전용 함수 const로 BP에서 실행핀 없는 퓨어노드로 사용
{
	if (IsValid(Rifle)) // 유효성 검사
    {
		return Rifle->GetCurrentAmmo(); // 현재 탄약 반환
    } 
	return 0; // 라이플이 없을 때 0 반환
}

// 최대 탄약 반환 함수
int32 AMainCharacter::GetRifleMaxAmmo() const // 읽기 전용 함수 const로 BP에서 실행핀 없는 퓨어노드로 사용
{
	if (IsValid(Rifle)) // 유효성 검사
    {
		return Rifle->GetMaxAmmo(); // 최대 탄약 반환
    }
	return 0; // 라이플이 없을 때 0 반환
}

// 총 탄약 반환 함수
int32 AMainCharacter::GetRifleTotalAmmo() const // 읽기 전용 함수 const로 BP에서 실행핀 없는 퓨어노드로 사용
{
	if (IsValid(Rifle)) // 유효성 검사
    {
		return Rifle->GetTotalAmmo(); // 총 탄약 반환
    }
	return 0; // 라이플이 없을 때 0 반환
}

// 스킬 쿨타임 백분율 반환 함수들
float AMainCharacter::GetSkill1CooldownPercent() const // 읽기 전용 함수 const로 BP에서 실행핀 없는 퓨어노드로 사용
{
	if (IsValid(SkillComponent)) // 유효성 검사
    {
		return SkillComponent->GetSkill1CooldownPercent(); // 백분율 반환
    }
	return 0.0f; // 스킬 컴포넌트가 없을 때 0 반환
}
float AMainCharacter::GetSkill2CooldownPercent() const // 읽기 전용 함수 const로 BP에서 실행핀 없는 퓨어노드로 사용
{
	if (IsValid(SkillComponent)) // 유효성 검사
	{
		return SkillComponent->GetSkill2CooldownPercent(); // 백분율 반환
	}
	return 0.0f; // 스킬 컴포넌트가 없을 때 0 반환
}
float AMainCharacter::GetSkill3CooldownPercent() const // 읽기 전용 함수 const로 BP에서 실행핀 없는 퓨어노드로 사용
{
	if (IsValid(SkillComponent)) // 유효성 검사
	{
		return SkillComponent->GetSkill3CooldownPercent(); // 백분율 반환
	}
	return 0.0f; // 스킬 컴포넌트가 없을 때 0 반환
}
float AMainCharacter::GetAimSkill1CooldownPercent() const // 읽기 전용 함수 const로 BP에서 실행핀 없는 퓨어노드로 사용
{
	if (IsValid(SkillComponent)) // 유효성 검사
	{
		return SkillComponent->GetAimSkill1CooldownPercent(); // 백분율 반환
	}
    return 0.0f; // 스킬 컴포넌트가 없을 때 0 반환
}
float AMainCharacter::GetAimSkill2CooldownPercent() const // 읽기 전용 함수 const로 BP에서 실행핀 없는 퓨어노드로 사용
{
	if (IsValid(SkillComponent)) // 유효성 검사
    {
		return SkillComponent->GetAimSkill2CooldownPercent(); // 백분율 반환
    }
	return 0.0f; // 스킬 컴포넌트가 없을 때 0 반환
}
float AMainCharacter::GetAimSkill3CooldownPercent() const // 읽기 전용 함수 const로 BP에서 실행핀 없는 퓨어노드로 사용
{
	if (IsValid(SkillComponent)) // 유효성 검사
	{
		return SkillComponent->GetAimSkill3CooldownPercent(); // 백분율 반환
	}
	return 0.0f; // 스킬 컴포넌트가 없을 때 0 반환
}

// 텔레포트 쿨타임 백분율 반환 함수
float AMainCharacter::GetTeleportCooldownPercent() const // 읽기 전용 함수 const로 BP에서 실행핀 없는 퓨어노드로 사용
{
	if (IsValid(MeleeCombatComponent)) // 유효성 검사
    {
		return MeleeCombatComponent->GetTeleportCooldownPercent(); // 백분율 반환
    }
	return 0.0f; // 밀리 컴벳 컴포넌트가 없을 때 0 반환
}

// 입력 컴포넌트 설정 함수
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent); // 부모 클래스의 입력 설정 호출

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) // 인헨스드 인풋 컴포넌트 가져옴
    {
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMainCharacter::Move); // 이동 바인딩
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMainCharacter::Look); // 시점 변경 바인딩
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMainCharacter::HandleJump); // 점프 시작 바인딩
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping); // 점프 종료 바인딩
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AMainCharacter::EnterAimMode); // 에임 모드 진입 바인딩
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AMainCharacter::ExitAimMode); // 에임 모드 종료 바인딩
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &AMainCharacter::FireWeapon); // 발사 바인딩
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AMainCharacter::ReloadWeapon); // 재장전 바인딩
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AMainCharacter::Dash); // 대시 바인딩
		EnhancedInputComponent->BindAction(ZoomInAction, ETriggerEvent::Triggered, this, &AMainCharacter::ZoomIn); // 줌인 바인딩
		EnhancedInputComponent->BindAction(ZoomOutAction, ETriggerEvent::Triggered, this, &AMainCharacter::ZoomOut); // 줌아웃 바인딩
		EnhancedInputComponent->BindAction(Skill1Action, ETriggerEvent::Started, this, &AMainCharacter::UseSkill1); // 스킬1 바인딩
		EnhancedInputComponent->BindAction(Skill2Action, ETriggerEvent::Started, this, &AMainCharacter::UseSkill2); // 스킬2 바인딩
		EnhancedInputComponent->BindAction(Skill3Action, ETriggerEvent::Started, this, &AMainCharacter::UseSkill3); // 스킬3 바인딩
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AMainCharacter::TogglePauseMenu); // 일시정지 바인딩
    }
}
