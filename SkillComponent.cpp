#include "SkillComponent.h"
#include "MainCharacter.h"
#include "Enemy.h"
#include "Skill3Projectile.h"
#include "MachineGun.h"
#include "Knife.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Cannon.h"
#include "Animation/AnimInstance.h"

USkillComponent::USkillComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USkillComponent::BeginPlay()
{
    Super::BeginPlay();
}

void USkillComponent::InitializeSkills(AMainCharacter* InCharacter, AMachineGun* InMachineGun, AKnife* InLeftKnife, AKnife* InRightKnife, UBoxComponent* InKickHitBox, ACannon* InCannon)
{
    OwnerCharacter = InCharacter;
    MachineGun = InMachineGun;
    LeftKnife = InLeftKnife;
    RightKnife = InRightKnife;
    KickHitBox = InKickHitBox;
    Cannon = InCannon;

    Skill1Montage = InCharacter->GetSkill1AnimMontage();
    Skill2Montage = InCharacter->GetSkill2AnimMontage();
    Skill3Montage = InCharacter->GetSkill3AnimMontage();
    AimSkill1Montage = InCharacter->GetAimSkill1AnimMontage();
    AimSkill2Montage = InCharacter->GetAimSkill2AnimMontage();
    Skill3ProjectileClass = InCharacter->GetSkill3ProjectileClass();
}

void USkillComponent::RotateCharacterToInputDirection()
{
    if (!OwnerCharacter) return;
    FVector Input = OwnerCharacter->GetCharacterMovement()->GetLastInputVector();
    if (!Input.IsNearlyZero())
    {
        Input.Normalize();
        FRotator Rot = Input.Rotation();
        Rot.Pitch = 0.f;
        OwnerCharacter->SetActorRotation(Rot);
    }
}

// 스킬1
void USkillComponent::UseSkill1()
{
    if (!OwnerCharacter || bIsUsingSkill1 || !bCanUseSkill1 || bIsUsingAimSkill1) return;
    if (OwnerCharacter->IsDashing() || OwnerCharacter->IsAiming() || OwnerCharacter->IsJumping() || OwnerCharacter->IsInDoubleJump() || bIsUsingAimSkill1) return;

    bIsUsingSkill1 = true;
    bCanUseSkill1 = false;

    RotateCharacterToInputDirection();
    PlaySkill1Montage();
    DrawSkill1Range();

    GetWorld()->GetTimerManager().SetTimer(Skill1EffectHandle, this, &USkillComponent::ApplySkill1Effect, 0.5f, false);
    GetWorld()->GetTimerManager().SetTimer(Skill1CooldownHandle, this, &USkillComponent::ResetSkill1Cooldown, Skill1Cooldown, false);
}

void USkillComponent::PlaySkill1Montage()
{
    if (!Skill1Montage || !OwnerCharacter) return;
    UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance();
    if (!Anim) return;

    float Duration = Anim->Montage_Play(Skill1Montage, 1.0f);
    if (Duration > 0.0f)
    {
        FOnMontageEnded End;
        End.BindUObject(this, &USkillComponent::ResetSkill1);
        Anim->Montage_SetEndDelegate(End, Skill1Montage);
    }
}

void USkillComponent::DrawSkill1Range()
{
    FVector Center = OwnerCharacter->GetActorLocation();
    DrawDebugSphere(GetWorld(), Center, Skill1Range, 32, FColor::Red, false, 1.0f, 0, 2.0f);
}

void USkillComponent::ApplySkill1Effect()
{
    FVector Center = OwnerCharacter->GetActorLocation();
    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), Enemies);

    for (AActor* Actor : Enemies)
    {
        AEnemy* Enemy = Cast<AEnemy>(Actor);
        if (Enemy && Enemy->GetCharacterMovement())
        {
            float Dist = FVector::Dist(Center, Enemy->GetActorLocation());
            if (Dist <= Skill1Range)
            {
                Enemy->EnterInAirStunState(4.0f);
            }
        }
    }
}

void USkillComponent::ResetSkill1(UAnimMontage* Montage, bool bInterrupted)
{
    bIsUsingSkill1 = false;
    GetWorld()->GetTimerManager().SetTimer(Skill1CooldownHandle, this, &USkillComponent::ResetSkill1Cooldown, Skill1Cooldown, false);
}

void USkillComponent::ResetSkill1Cooldown()
{
    bCanUseSkill1 = true;
}

// 스킬2
void USkillComponent::UseSkill2()
{
    if (!OwnerCharacter || bIsUsingSkill2 || !bCanUseSkill2 || bIsUsingAimSkill2) return;
    if (OwnerCharacter->IsDashing() || OwnerCharacter->IsAiming() || OwnerCharacter->IsJumping() || OwnerCharacter->IsInDoubleJump() || bIsUsingAimSkill1) return;

    bIsUsingSkill2 = true;
    bCanUseSkill2 = false;

    RotateCharacterToInputDirection();
    PlaySkill2Montage();

    GetWorld()->GetTimerManager().SetTimer(Skill2EffectHandle, this, &USkillComponent::ApplySkill2Effect, Skill2EffectDelay, false);
    GetWorld()->GetTimerManager().SetTimer(Skill2CooldownHandle, this, &USkillComponent::ResetSkill2Cooldown, Skill2Cooldown, false);
}

void USkillComponent::PlaySkill2Montage()
{
    if (!Skill2Montage || !OwnerCharacter) return;
    UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance();
    if (!Anim) return;

    float Duration = Anim->Montage_Play(Skill2Montage, 1.2f);
    if (Duration > 0.0f)
    {
        FOnMontageEnded End;
        End.BindUObject(this, &USkillComponent::ResetSkill2);
        Anim->Montage_SetEndDelegate(End, Skill2Montage);
    }
}

void USkillComponent::DrawSkill2Range()
{
    if (!bIsUsingSkill2) return; 

    UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld());

    FVector SkillCenter = OwnerCharacter->GetActorLocation(); 
    float DebugDuration = 0.1f; 
    float DebugRadius = 200.0f; 

    DrawDebugSphere(GetWorld(), SkillCenter, DebugRadius, 32, FColor::Blue, false, DebugDuration, 0, 3.0f); // 스킬범위 표시

    UE_LOG(LogTemp, Warning, TEXT("Skill2 Range Circle Drawn at %s with Radius %f"), *SkillCenter.ToString(), DebugRadius);
}

void USkillComponent::ApplySkill2Effect()
{
    FVector Center = OwnerCharacter->GetActorLocation();
    OwnerCharacter->LaunchCharacter(FVector(0, 0, 300.0f), false, true);

    TArray<FHitResult> HitResults;
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    UKismetSystemLibrary::SphereTraceMultiForObjects(
        GetWorld(), Center, Center, Skill2Range, ObjectTypes,
        false, {}, EDrawDebugTrace::None, HitResults, true
    );

    TSet<AEnemy*> HitEnemies;
    for (const FHitResult& Hit : HitResults)
    {
        AEnemy* Enemy = Cast<AEnemy>(Hit.GetActor());
        if (Enemy && !HitEnemies.Contains(Enemy))
        {
            UGameplayStatics::ApplyDamage(Enemy, Skill2Damage, OwnerCharacter->GetController(), OwnerCharacter, UDamageType::StaticClass());
            HitEnemies.Add(Enemy);
        }
    }

    DrawDebugSphere(GetWorld(), Center, Skill2Range, 32, FColor::Red, false, 0.4f, 0, 3.0f);
    GetWorld()->GetTimerManager().SetTimer(Skill2RangeClearHandle, this, &USkillComponent::ClearSkill2Range, 0.4f, false);
}

void USkillComponent::ClearSkill2Range()
{
    UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld());
}

void USkillComponent::ResetSkill2(UAnimMontage* Montage, bool bInterrupted)
{
    bIsUsingSkill2 = false;
    UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld());
    GetWorld()->GetTimerManager().SetTimer(Skill2CooldownHandle, this, &USkillComponent::ResetSkill2Cooldown, Skill2Cooldown, false);
}

void USkillComponent::ResetSkill2Cooldown()
{
    bCanUseSkill2 = true;
}

// 스킬3
void USkillComponent::UseSkill3()
{
    if (!OwnerCharacter || bIsUsingSkill3 || !bCanUseSkill3) return;
    if (OwnerCharacter->IsDashing() || OwnerCharacter->IsAiming() || OwnerCharacter->IsJumping() || OwnerCharacter->IsInDoubleJump() || bIsUsingAimSkill1) return;

    bIsUsingSkill3 = true;
    bCanUseSkill3 = false;

    RotateCharacterToInputDirection();

    FVector SpawnLoc = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 120.f + FVector(0, 0, 30.f);
    FRotator SpawnRot = OwnerCharacter->GetActorRotation();

    if (Skill3ProjectileClass)
    {
        ASkill3Projectile* Projectile = GetWorld()->SpawnActor<ASkill3Projectile>(Skill3ProjectileClass, SpawnLoc, SpawnRot);
        if (Projectile)
        {
            Projectile->SetDamage(Skill3Damage);
            Projectile->SetShooter(OwnerCharacter);
            Projectile->FireInDirection(OwnerCharacter->GetActorForwardVector());
        }
    }

    PlaySkill3Montage();
    GetWorld()->GetTimerManager().SetTimer(Skill3CooldownHandle, this, &USkillComponent::ResetSkill3Cooldown, Skill3Cooldown, false);
}

void USkillComponent::PlaySkill3Montage()
{
    if (!Skill3Montage || !OwnerCharacter) return;
    UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance();
    if (!Anim) return;

    float Duration = Anim->Montage_Play(Skill3Montage, 1.0f);
    if (Duration > 0.0f)
    {
        FOnMontageEnded End;
        End.BindUObject(this, &USkillComponent::ResetSkill3);
        Anim->Montage_SetEndDelegate(End, Skill3Montage);
    }
}

void USkillComponent::ResetSkill3(UAnimMontage* Montage, bool bInterrupted)
{
    bIsUsingSkill3 = false;
    GetWorld()->GetTimerManager().SetTimer(Skill3CooldownHandle, this, &USkillComponent::ResetSkill3Cooldown, Skill3Cooldown, false);
}

void USkillComponent::ResetSkill3Cooldown()
{
    bCanUseSkill3 = true;
}

// 에임스킬1
void USkillComponent::UseAimSkill1()
{
    if (!OwnerCharacter || bIsUsingAimSkill1 || !bCanUseAimSkill1) return;
    if (OwnerCharacter->IsDashing() || OwnerCharacter->IsJumping() || OwnerCharacter->IsInDoubleJump()) return;

    if (OwnerCharacter->IsAiming())
    {
        OwnerCharacter->ExitAimMode();  // 기존 에임모드 강제 종료
    }

    bIsUsingAimSkill1 = true;
    bCanUseAimSkill1 = false;

    OwnerCharacter->AttachRifleToBack();
    OwnerCharacter->AttachKnifeToBack();
    RotateCharacterToInputDirection();

    if (MachineGun)
    {
        MachineGun->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("AimSkill1Socket"));
        MachineGun->SetActorRelativeLocation(FVector(-5.f, -20.f, 0.f));
        MachineGun->SetActorRelativeRotation(FRotator(90.f, 180.f, 0.f));
        MachineGun->SetActorRelativeScale3D(FVector(0.3f));
        MachineGun->SetActorHiddenInGame(false);
        MachineGun->StartFire();
    }

    PlayAimSkill1Montage();

    GetWorld()->GetTimerManager().SetTimer(AimSkill1RepeatHandle, this, &USkillComponent::RepeatAimSkill1Montage, AimSkill1PlayInterval, true);
    GetWorld()->GetTimerManager().SetTimer(AimSkill1CooldownHandle, this, &USkillComponent::ResetAimSkill1Cooldown, AimSkill1Cooldown, false);
}

void USkillComponent::PlayAimSkill1Montage()
{
    if (!AimSkill1Montage || !OwnerCharacter) return;
    UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance();
    if (!Anim) return;

    float Rate = 1.0f;
    float Duration = Anim->Montage_Play(AimSkill1Montage, Rate);

    if (Duration > 0.0f)
    {
        Anim->Montage_SetNextSection(FName("Start"), FName("Loop"), AimSkill1Montage);
        Anim->Montage_SetNextSection(FName("Loop"), FName("Loop"), AimSkill1Montage);
        AimSkill1MontageStartTime = GetWorld()->GetTimeSeconds();
        GetWorld()->GetTimerManager().SetTimer(AimSkill1RepeatHandle, this, &USkillComponent::ResetAimSkill1Timer, AimSkill1Duration, false);
    }
}

void USkillComponent::RepeatAimSkill1Montage()
{
    if (!bIsUsingAimSkill1 || !OwnerCharacter) return;
    float Elapsed = GetWorld()->GetTimeSeconds() - AimSkill1MontageStartTime;
    if (Elapsed >= AimSkill1Duration)
    {
        ResetAimSkill1(nullptr, false);
        return;
    }
    PlayAimSkill1Montage();
}

void USkillComponent::ResetAimSkill1Timer()
{
    ResetAimSkill1(nullptr, false);
}

void USkillComponent::ResetAimSkill1(UAnimMontage* Montage, bool bInterrupted)
{
    bIsUsingAimSkill1 = false;

    if (OwnerCharacter)
    {
        OwnerCharacter->AttachRifleToBack();
        OwnerCharacter->AttachKnifeToHand();
    }

    if (MachineGun)
    {
        MachineGun->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("AimSkill1Socket"));
        MachineGun->SetActorRelativeLocation(FVector(-5.f, -20.f, 0.f));
        MachineGun->SetActorRelativeRotation(FRotator(90.f, 180.f, 0.f));
        MachineGun->SetActorRelativeScale3D(FVector(0.3f));
        MachineGun->SetActorHiddenInGame(true);
        MachineGun->StopFire();
    }

    if (UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance())
    {
        if (AimSkill1Montage)
        {
            Anim->Montage_Stop(0.3f, AimSkill1Montage);
        }
    }

    GetWorld()->GetTimerManager().ClearTimer(AimSkill1RepeatHandle);
    GetWorld()->GetTimerManager().SetTimer(AimSkill1CooldownHandle, this, &USkillComponent::ResetAimSkill1Cooldown, AimSkill1Cooldown, false);
}

void USkillComponent::ResetAimSkill1Cooldown()
{
    bCanUseAimSkill1 = true;
}

// 에임스킬2
void USkillComponent::UseAimSkill2()
{
    if (!OwnerCharacter || bIsUsingAimSkill2 || !bCanUseAimSkill2) return;
    if (OwnerCharacter->IsDashing() || OwnerCharacter->IsJumping() || OwnerCharacter->IsInDoubleJump()) return;

    if (OwnerCharacter->IsAiming())
    {
        OwnerCharacter->ExitAimMode();  // 기존 에임모드 강제 종료
    }

    bIsUsingAimSkill2 = true;
    bCanUseAimSkill2 = false;

    OwnerCharacter->AttachRifleToBack();
    OwnerCharacter->AttachKnifeToBack();
    RotateCharacterToInputDirection();

    if (Cannon)
    {
        Cannon->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("AimSkill2Socket"));
        Cannon->SetActorRelativeLocation(FVector(0.f, 0.f, 0.f));
        Cannon->SetActorRelativeRotation(FRotator(0.f, 0.f, 0.f));
        Cannon->SetActorRelativeScale3D(FVector(1.0f));
        Cannon->SetActorHiddenInGame(false);
        Cannon->SetShooter(OwnerCharacter);
        Cannon->FireProjectile();
    }

    AimSkill2MontageStartTime = GetWorld()->GetTimeSeconds();

    PlayAimSkill2Montage();

    GetWorld()->GetTimerManager().SetTimer(AimSkill2RepeatHandle, this, &USkillComponent::RepeatAimSkill2Montage, AimSkill2PlayInterval, true);

    GetWorld()->GetTimerManager().SetTimer(AimSkill2CooldownHandle, this, &USkillComponent::ResetAimSkill2Cooldown, AimSkill2Cooldown, false);
}

void USkillComponent::PlayAimSkill2Montage()
{
    if (!AimSkill2Montage || !OwnerCharacter) return;
    UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance();
    if (!Anim) return;

    float Rate = 2.0f;
    float Duration = Anim->Montage_Play(AimSkill2Montage, Rate);

    if (Duration > 0.0f)
    {
        Anim->Montage_SetNextSection(FName("Start"), FName("Loop"), AimSkill2Montage);
        Anim->Montage_SetNextSection(FName("Loop"), FName("Loop"), AimSkill2Montage);
        AimSkill2MontageStartTime = GetWorld()->GetTimeSeconds();
        GetWorld()->GetTimerManager().SetTimer(AimSkill2RepeatHandle, this, &USkillComponent::ResetAimSkill2Timer, AimSkill2Duration, false);
    }
}

void USkillComponent::RepeatAimSkill2Montage()
{
    if (!bIsUsingAimSkill2 || !OwnerCharacter) return;
    float Elapsed = GetWorld()->GetTimeSeconds() - AimSkill2MontageStartTime;
    if (Elapsed >= AimSkill2Duration)
    {
        ResetAimSkill2(nullptr, false);
        return;
    }
    PlayAimSkill2Montage();
}

void USkillComponent::ResetAimSkill2Timer()
{
    ResetAimSkill2(nullptr, false);
}

void USkillComponent::ResetAimSkill2(UAnimMontage* Montage, bool bInterrupted)
{
    bIsUsingAimSkill2 = false;

    if (OwnerCharacter)
    {
        OwnerCharacter->AttachRifleToBack();
        OwnerCharacter->AttachKnifeToHand();
    }

    if (Cannon)
    {
        Cannon->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("AimSkill2Socket"));
        Cannon->SetActorRelativeLocation(FVector(0.f, 0.f, 0.f));
        Cannon->SetActorRelativeRotation(FRotator(0.f, 0.f, 0.f));
        Cannon->SetActorRelativeScale3D(FVector(1.0f));
        Cannon->SetActorHiddenInGame(true);

    }

    GetWorld()->GetTimerManager().ClearTimer(AimSkill2RepeatHandle);

    if (UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance())
    {
        if (AimSkill2Montage)
        {
            Anim->Montage_Stop(0.3f, AimSkill2Montage);
        }
    }

    GetWorld()->GetTimerManager().ClearTimer(AimSkill2RepeatHandle);
    GetWorld()->GetTimerManager().SetTimer(AimSkill2CooldownHandle, this, &USkillComponent::ResetAimSkill2Cooldown, AimSkill2Cooldown, false);
}

void USkillComponent::ResetAimSkill2Cooldown()
{
    bCanUseAimSkill2 = true;
}