#include "EnemyShooter.h"
#include "EnemyShooterAIController.h" // AI ��Ʈ�ѷ� Ŭ����
#include "EnemyShooterGun.h" // �� Ŭ����
#include "EnemyShooterAnimInstance.h" // �ִ� �ν��Ͻ� Ŭ����
#include "Animation/AnimInstance.h" // UAnimInstance Ŭ����
#include "GameFramework/Actor.h" // AActor Ŭ����
#include "Kismet/GameplayStatics.h" // �����÷��� ��ƿ��Ƽ �Լ�
#include "Components/SkeletalMeshComponent.h" // ���̷�Ż �޽� ������Ʈ
#include "GameFramework/CharacterMovementComponent.h" // ĳ���� �����Ʈ ������Ʈ
#include "MainGameModeBase.h" // ���Ӹ�� ���� (�� ��� �˸���)

AEnemyShooter::AEnemyShooter()
{
	PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ� ȣ�� Ȱ��ȭ
	bCanAttack = true; // �⺻������ ���� ������ ���·� ����
	bIsAttacking = false; // ó������ ���� ���� �ƴ� ���·� ����

	// AI �� �ִϸ��̼� Ŭ���� ����
	AIControllerClass = AEnemyShooterAIController::StaticClass(); // �� ĳ���Ͱ� ����� AI ��Ʈ�ѷ� ����
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; // ���忡 ��ġ/���� �� AI�� �ڵ� ����
	GetMesh()->SetAnimInstanceClass(UEnemyShooterAnimInstance::StaticClass()); // ����� �ִ� �ν��Ͻ� Ŭ���� ����

	// ȸ�� ����: AI ��Ʈ�ѷ��� ������ ���� ĳ���Ͱ� ȸ���ϵ��� ����
	GetCharacterMovement()->bOrientRotationToMovement = false; // �̵� �������� �ڵ� ȸ�� ��Ȱ��ȭ
	GetCharacterMovement()->bUseControllerDesiredRotation = false; // ��Ʈ�ѷ��� ��� ȸ���� ��� ��Ȱ��ȭ
	bUseControllerRotationYaw = true; // ��Ʈ�ѷ��� Yaw ȸ������ ĳ���Ϳ� ���� ���� (���� �߿�)

	GetCharacterMovement()->MaxWalkSpeed = 250.0f; // �⺻ �̵� �ӵ� ����

	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter Generated"))
}

void AEnemyShooter::BeginPlay()
{
	Super::BeginPlay(); // �θ� Ŭ���� BeginPlay ȣ��
	SetCanBeDamaged(true); // �������� ���� �� �ֵ��� ����
	SetActorTickInterval(0.2f); // Tick �ֱ� 0.2�ʷ� ����ȭ

	SetupAI(); // AI �ʱ� ���� �Լ� ȣ��

	if (!GetController()) // AI ��Ʈ�ѷ��� �Ҵ���� �ʾҴٸ� (������ ���� ��� �ڵ�)
	{
		SpawnDefaultController(); // �⺻ ��Ʈ�ѷ��� �����Ͽ� �Ҵ�
		UE_LOG(LogTemp, Warning, TEXT("EnemyShooter AI Controller manually spawned"));
	}

	AAIController* AICon = Cast<AAIController>(GetController()); // AI ��Ʈ�ѷ� ��������
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyShooter AI Controller Assigend %s"), *AICon->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyShooter AI Controller is Null!"));
	}

	// �� ���� �� ����
	if (GunClass) // �������Ʈ���� ������ �� Ŭ������ �ִٸ�
	{
		EquippedGun = GetWorld()->SpawnActor<AEnemyShooterGun>(GunClass); // ���忡 �� ���� ����
		if (EquippedGun)
		{
			EquippedGun->SetOwner(this); // ���� �����ڸ� �� ĳ���ͷ� ����
			USkeletalMeshComponent* MeshComp = GetMesh(); // ĳ������ ���̷�Ż �޽��� ������
			if (MeshComp)
			{
				// ���̷�Ż �޽��� Ư�� ���Ͽ� ���� ����
				FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
				EquippedGun->AttachToComponent(MeshComp, AttachmentRules, FName("EnemyShooterGunSocket"));
			}
		}
	}

	PlaySpawnIntroAnimation(); // ���� ��Ʈ�� �ִϸ��̼� ���
}

void AEnemyShooter::PostInitializeComponents()
{
	Super::PostInitializeComponents(); // �θ� Ŭ���� �Լ� ȣ��
	// ���� ƽ�� ����ǵ��� Ÿ�̸Ӹ� ���� (��Ʈ�ѷ��� Ȯ���� �Ҵ�� �� �α׸� ��� ����)
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			AAIController* AICon = Cast<AAIController>(GetController()); // AI ��Ʈ�ѷ� �Ҵ�
			if (AICon)
			{
				UE_LOG(LogTemp, Warning, TEXT("AEnemyShooter AIController Assigned Later: %s"), *AICon->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("AEnemyShooter AIController STILL NULL!"));
			}
		});
}

void AEnemyShooter::PlaySpawnIntroAnimation()
{
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyShooter AnimInstance not found"));
		return;
	}

	bIsPlayingIntro = true; // ��Ʈ�� ��� �� ���·� ��ȯ
	bCanAttack = false; // ���� �߿��� ���� �Ұ� ����

	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement(); // ���� �ִϸ��̼� ���� �̵� ����
	}

	PlayAnimMontage(SpawnIntroMontage); // ���� ��Ʈ�� ��Ÿ�� ���

	// ��Ÿ�ְ� ������ OnIntroMontageEnded �Լ��� ȣ��ǵ��� ��������Ʈ ���ε�
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyShooter::OnIntroMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, SpawnIntroMontage);
}

void AEnemyShooter::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Shooter Intro Montage Ended"));
	bIsPlayingIntro = false; // ��Ʈ�� ��� ���� ����
	bCanAttack = true; // ���� ���� ���·� ��ȯ
}

void AEnemyShooter::ApplyBaseWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = 250.0f; // �⺻ �̵� �ӵ� 250���� ����
	GetCharacterMovement()->MaxAcceleration = 5000.0f; // ���ӵ��� ���� �ﰢ ����
	GetCharacterMovement()->BrakingFrictionFactor = 10.0f; // �������� ���� �ﰢ ����
}

void AEnemyShooter::SetupAI()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking); // �׺���̼� �޽� ���� �ȴ� ���� ����
}

void AEnemyShooter::PlayShootingAnimation()
{
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance || !ShootingMontage)
	{
		// �ִϸ��̼��� ������ ���� ���´� �ʱ�ȭ�ؾ� AI�� ������ ����
		bCanAttack = true;
		bIsAttacking = false;
		return;
	}

	PlayAnimMontage(ShootingMontage); // ��� ��Ÿ�� ���

	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement(); // �̵� ����
	}

	bCanAttack = false; // ���� ��Ÿ�� ����
	bIsAttacking = true; // ���� �ִϸ��̼� ��� ��

	// ��Ÿ�� ���� �� OnShootingMontageEnded ȣ���ϵ��� ��������Ʈ ���ε�
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyShooter::OnShootingMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ShootingMontage);
}

void AEnemyShooter::OnShootingMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bCanAttack = true; // ���� ���� ���·� ����
	bIsAttacking = false; // ���� �ִϸ��̼� ����
}

void AEnemyShooter::PlayThrowingGrenadeAnimation()
{
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance || !ThrowingGrenadeMontage)
	{
		bCanAttack = true;
		bIsAttacking = false;
		return;
	}

	// �ڡڡ� �߿�: ����ź ��ô �ÿ��� �ִϸ��̼� �������� ���� �����ؾ� �ϹǷ�, AI�� ������ ���� ȸ����Ű�� ����� ��� ��Ȱ��ȭ.
	bUseControllerRotationYaw = false;

	PlayAnimMontage(ThrowingGrenadeMontage); // ����ź ��ô ��Ÿ�� ���

	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement(); // �̵� ����
	}

	bCanAttack = false; // ���� ��Ÿ�� ����
	bIsAttacking = true; // ���� �ִϸ��̼� ��� ��

	// ��Ÿ�� ���� �� OnThrowingGrenadeMontageEnded ȣ���ϵ��� ��������Ʈ ���ε�
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyShooter::OnThrowingGrenadeMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ThrowingGrenadeMontage);
}

void AEnemyShooter::OnThrowingGrenadeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bCanAttack = true; // ���� ���� ���·� ����
	bIsAttacking = false; // ���� �ִϸ��̼� ����
	bUseControllerRotationYaw = true; // �ڡڡ� �߿�: ��Ȱ��ȭ�ߴ� AI�� ���� ȸ�� ����� �ٽ� Ȱ��ȭ.
}

float AEnemyShooter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || bIsPlayingIntro) return 0.0f; // �̹� ����߰ų� ��Ʈ�� ����߿� ���ظ� ���� ����

	float DamageApplied = FMath::Min(Health, DamageAmount); // ���� ����� ������ ���
	Health -= DamageApplied; // ü�� ����
	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter took %f damage, Health remaining: %f"), DamageAmount, Health);

	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance()); // �ִ� �ν��Ͻ��� ������
	if (!AnimInstance || HitMontages.Num() == 0) return 0.0f; // �ִ� �ν��Ͻ��� ���ų� �ǰ� ��Ÿ�� �迭�� ����ٸ� ����

	int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1); // �ǰ� ��Ÿ�� �迭���� ���� �ε��� ����
	UAnimMontage* SelectedMontage = HitMontages[RandomIndex]; // ���õ� ��Ÿ��

	if (SelectedMontage)
	{
		AnimInstance->Montage_Play(SelectedMontage, 1.0f); // ���õ� ��Ÿ�� ���

		// �ǰ� ��Ÿ�� ���� �� ���� ���¸� ó���ϱ� ���� ��������Ʈ ���ε�
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyShooter::OnHitMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement(); // �ǰ� �ִϸ��̼� ���� �̵� ����
	}

	if (Health <= 0.0f) // ü���� 0 ���϶��
	{
		Die(); // ��� ó��
	}

	return DamageApplied; // ���� ����� ������ �� ��ȯ
}

void AEnemyShooter::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bIsDead) return; // ��� �ÿ��� �ƹ��͵� ���� ����
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());
	// ���� ���� ���¿��� �ǰ� �ִϸ��̼��� ������ ���, �ٽ� ���� �ִϸ��̼��� �̾ ���
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
		UE_LOG(LogTemp, Warning, TEXT("Hit animation ended on ground. No need to resume InAirStunMontage."));
	}
}

void AEnemyShooter::StopActions()
{
	AAIController* AICon = Cast<AAIController>(GetController());
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());

	GetCharacterMovement()->DisableMovement(); // ��� �̵� ��Ȱ��ȭ
	GetCharacterMovement()->StopMovementImmediately(); // ��� ����

	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(0.1f); // ��� ���� ��� ��Ÿ�� ����
	}

	if (bIsInAirStun) // ���� ������ ��� �߰� ��ġ
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy is stunned! Forcing all actions to stop."));
		bCanAttack = false; // ���� �Ұ� ���� ����
		GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle); // ���� ���� Ÿ�̸Ӱ� �ִٸ� ���
	}
}

void AEnemyShooter::Die()
{
	if (bIsDead) return; // �ߺ� ��� ó�� ����
	bIsDead = true; // ��� ���·� ��ȯ
	UEnemyShooterAnimInstance* AnimInstance = Cast<UEnemyShooterAnimInstance>(GetMesh()->GetAnimInstance());

	if (!AnimInstance || DeadMontages.Num() == 0) // �ִϸ��̼� ����� �Ұ����� ���
	{
		HideEnemy(); // ��� ���� �Լ� ȣ��
		return;
	}

	int32 RandIndex = FMath::RandRange(0, DeadMontages.Num() - 1); // ��� ��Ÿ�� �� �ϳ��� ���� ����
	UAnimMontage* SelectedMontage = DeadMontages[RandIndex];
	float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // ��Ÿ�� ���

	if (PlayResult > 0.0f) // ��Ÿ�� ����� �����ߴٸ�
	{
		float MontageLength = SelectedMontage->GetPlayLength(); // ��Ÿ���� ��ü ����
		float HidePercent = 0.8f; // ��Ÿ���� 80% �������� ���� ����
		float HideTime = MontageLength * HidePercent; // ���� �ð� ���

		// ���� �ð� �Ŀ� HideEnemy �Լ��� ȣ���ϵ��� Ÿ�̸� ����
		GetWorld()->GetTimerManager().SetTimer(
			DeathTimerHandle,
			this,
			&AEnemyShooter::HideEnemy,
			HideTime,
			false
		);
		UE_LOG(LogTemp, Warning, TEXT("EnemyShooter death montage playing, will hide after %.2f seconds (%.0f%% of %.2f total)"),
			HideTime, HidePercent * 100.0f, MontageLength);
	}
	else // ��Ÿ�� ����� �����ߴٸ�
	{
		HideEnemy(); // ��� ����
	}

	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon)
	{
		// AICon->StopAI(); // AI ���� �ߴ� (���� �Լ����� ó���ϹǷ� �ּ�)
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AIController is NULL! Cannot stop AI."));
	}

	GetCharacterMovement()->DisableMovement(); // �̵� ��Ȱ��ȭ
	GetCharacterMovement()->StopMovementImmediately(); // ��� ����
	SetActorTickEnabled(false); // ������ ���� ���� ƽ ��Ȱ��ȭ
	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter Die() completed"));
}

// ��� �� �޸� �� �ý��� ���� �Լ�
void AEnemyShooter::HideEnemy()
{
	if (!bIsDead) return;

	UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyShooter - Memory Cleanup"));
	// ���Ӹ�忡 ���� �ı��Ǿ����� �˸�
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->OnEnemyDestroyed(this);
	}

	// 1. �̺�Ʈ �� ��������Ʈ ���� (���� ����)
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // �� ��ü�� ������ ��� Ÿ�̸� ����

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && IsValid(AnimInstance))
	{
		// ��������Ʈ ���ε� ���� ���� (�޸� ���� ����)
		AnimInstance->OnMontageEnded.RemoveAll(this);
		AnimInstance->OnMontageBlendingOut.RemoveAll(this);
		AnimInstance->OnMontageStarted.RemoveAll(this);
	}

	// 2. AI �ý��� ���� ����
	AEnemyShooterAIController* AICon = Cast<AEnemyShooterAIController>(GetController());
	if (AICon && IsValid(AICon))
	{
		//AICon->StopAI(); // AI ���� �ߴ�
		AICon->UnPossess(); // ��Ʈ�ѷ��� ���� ����(����) ����
		AICon->Destroy(); // AI ��Ʈ�ѷ� ���� ��ü�� �ı�
	}

	// 3. ���� �ý��� ����
	if (EquippedGun && IsValid(EquippedGun))
	{
		EquippedGun->HideGun(); // �� ���� �� ���� �Լ� ȣ��
		EquippedGun = nullptr; // �ѿ� ���� ���� ����
	}

	// 4. �����Ʈ �ý��� ����
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp && IsValid(MovementComp))
	{
		MovementComp->DisableMovement(); // �̵� ��Ȱ��ȭ
		MovementComp->StopMovementImmediately(); // ���� �̵� ��� �ߴ�
		MovementComp->SetMovementMode(EMovementMode::MOVE_None); // �׺���̼� �ý��ۿ��� ����
		MovementComp->SetComponentTickEnabled(false); // ������Ʈ ƽ ��Ȱ��ȭ
	}

	// 5. �޽� ������Ʈ ����
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp && IsValid(MeshComp))
	{
		MeshComp->SetVisibility(false); // ���������� �ʵ��� ���ü� ��Ȱ��ȭ
		MeshComp->SetHiddenInGame(true); // ���� ������ ���� ó��
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �浹 �˻� ��Ȱ��ȭ
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // ��� ä�ο� ���� �浹 ���� ����
		MeshComp->SetComponentTickEnabled(false); // ������Ʈ ƽ ��Ȱ��ȭ
		MeshComp->SetAnimInstanceClass(nullptr); // ����� �ִ� �ν��Ͻ� ���� ����
		MeshComp->SetSkeletalMesh(nullptr); // ����� ���̷�Ż �޽� ���� ����
	}

	// 6. ���� ��ü�� �ý��� ����
	SetActorHiddenInGame(true); // ���͸� ���� ������ ����
	SetActorEnableCollision(false); // ������ �浹 ��Ȱ��ȭ
	SetActorTickEnabled(false); // ������ Tick ��Ȱ��ȭ
	SetCanBeDamaged(false); // �������� ���� �� ������ ����

	// 7. ���� �����ӿ� �����ϰ� ���� ���� (ũ���� ����)
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyShooter>(this)]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
			{
				WeakThis->Destroy(); // ���͸� ���忡�� ������ ����
				UE_LOG(LogTemp, Warning, TEXT("EnemyShooter Successfully Destroyed"));
			}
		});
}

// �̱��� �Լ���
void AEnemyShooter::EnerInAirStunState(float Duration) {}
void AEnemyShooter::ExitInAirStunState() {}
void AEnemyShooter::ApplyGravityPull(FVector ExlplosionCenter, float PullStrengh) {}
//

void AEnemyShooter::Shoot()
{
	if (!EquippedGun || bIsDead || bIsPlayingIntro) // ���� ���ų�, �׾��ų�, ���� ���̸� �߻� �Ұ�
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot shoot: Gun not available or invalid state"));
		return;
	}

	EquippedGun->FireGun(); // ������ ���� �߻� �Լ� ȣ��
	bIsShooting = true; // ��� �� ���·� ��ȯ (����� ������ ����)

	UE_LOG(LogTemp, Warning, TEXT("EnemyShooter fired gun!"));
}