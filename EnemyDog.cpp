#include "EnemyDog.h"
#include "EnemyDogAnimInstance.h"
#include "EnemyDogAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/OverlapResult.h" // FOverlapResult ����ü ���

AEnemyDog::AEnemyDog()
{
	PrimaryActorTick.bCanEverTick = true;
	bCanAttack = true;
	bIsAttacking = false;

	AIControllerClass = AEnemyDogAIController::StaticClass(); // AI ��Ʈ�ѷ� ����
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; //���� �ÿ��� AI ��Ʈ�ѷ� �ڵ� �Ҵ�
	GetMesh()->SetAnimInstanceClass(UEnemyDogAnimInstance::StaticClass()); // �ִ� �ν��Ͻ� �������� ����

	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // �⺻ �̵��ӵ� ����
}

void AEnemyDog::BeginPlay()
{
	Super::BeginPlay();
	SetCanBeDamaged(true); // ���ظ� ���� �� �ִ��� ���� true
	SetActorTickInterval(0.05f); // Tick �� ���� 20fps
	
	if (!GetController()) // AI ��Ʈ�ѷ��� ���ٸ�
	{
		SpawnDefaultController(); // �����Ͽ� �Ҵ�
		UE_LOG(LogTemp, Warning, TEXT("EnemyDog AI Controller manually spawned"));
	}

	AAIController* AICon = Cast<AAIController>(GetController()); // AI��Ʈ�ѷ��� ������
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyDog AI Controller Assigend %s"), *AICon->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyDog AI Controller is Null!"));
	}

	SetUpAI(); // AI �����Լ� ȣ��
	PlaySpawnIntroAnimation(); // ���� �ִϸ��̼� ���
}

void AEnemyDog::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	GetWorld()->GetTimerManager().SetTimerForNextTick([this]() // ���� Tick��
		{
			AAIController* AICon = Cast<AAIController>(GetController()); // AI ��Ʈ�ѷ� �Ҵ�
			if (AICon)
			{
				UE_LOG(LogTemp, Warning, TEXT("AEnemyDog AIController Assigned Later: %s"), *AICon->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("AEnemyDog AIController STILL NULL!"));
			}
		});
}

void AEnemyDog::PlaySpawnIntroAnimation()
{
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyDog AnimInstance not found"));
		return;
	}

	bIsPlayingIntro = true; // ��Ʈ�� ����� ���� true
	bCanAttack = false; // ���� �߿��� ���� �Ұ�

	// AI �̵� ����
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		AICon->StopMovement();
	}

	PlayAnimMontage(SpawnIntroMontage);

	// ��Ÿ�� ���� ��������Ʈ ���ε�
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyDog::OnIntroMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, SpawnIntroMontage);
}

void AEnemyDog::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Dog Intro Montage Ended"));
	bIsPlayingIntro = false;
	bCanAttack = true;

}

void AEnemyDog::ApplyBaseWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // �̵� �ӵ� 600
	GetCharacterMovement()->MaxAcceleration = 5000.0f; // ��� �ִ�ӵ� ����
	GetCharacterMovement()->BrakingFrictionFactor = 10.0f;
}

void AEnemyDog::SetUpAI()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking); // �׺��ŷ ���� ����
}

void AEnemyDog::PlayNormalAttackAnimation()
{
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // �ִ��ν��Ͻ��� �ҷ���
	if (!AnimInstance || NormalAttackMontages.Num() == 0) return; // �ִ��ν��Ͻ��� ���ų� ��Ÿ�ֹ迭�� ����ٸ� ����
	if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return; // �ִϸ��̼� ����߿��� ���� ��� ����

	int32 RandomIndex = FMath::RandRange(0, NormalAttackMontages.Num() - 1); // ���� ��Ÿ�� ������� ����
	UAnimMontage* SelectedMontage = NormalAttackMontages[RandomIndex]; // �ش� ���� �ȿ��� ��Ÿ�ָ� ����

	if (SelectedMontage)
	{
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.5f); // ���õ� ��Ÿ�ָ� ���

		// ��Ÿ�� ���� ��������Ʈ ����
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyDog::OnAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // ���� ���� ��
	if (AICon)
	{
		AICon->StopMovement(); // �̵� ����
	}
	
	bCanAttack = false;
	bIsAttacking = true;
}

void AEnemyDog::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bCanAttack = true;
	bIsAttacking = false;
	bHasExecutedRaycast = false;
}

float AEnemyDog::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || bIsPlayingIntro) return 0.0f; // �̹� ����߰ų� ��Ʈ�� ����߿� ���ظ� ��������

	float DamageApplied = FMath::Min(Health, DamageAmount); // ���������밪: ���� ü�°� ���� ������ �� �� ������
	Health -= DamageApplied; // ������ ���밪��ŭ ü�¿��� ����
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog took %f damage, Health remaining: %f"), DamageAmount, Health);

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // �ִ��ν��Ͻ��� �ҷ���
	if (!AnimInstance || HitMontages.Num() == 0) return 0.0f; // �ִ��ν��Ͻ��� ���ų� ��Ÿ�ֹ迭�� ����ٸ� ����
	int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1); // ��Ʈ ��Ÿ�� ������� ����
	UAnimMontage* SelectedMontage = HitMontages[RandomIndex]; // �ش� ���� �ȿ��� ��Ÿ�ָ� ����

	if (SelectedMontage)
	{
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // ���õ� ��Ÿ�ָ� ���

		// ��Ʈ �ִϸ��̼� ���� �� ���� �ִϸ��̼� ���
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyDog::OnHitMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // ��Ʈ �ִϸ��̼� ���� ��
	if (AICon)
	{
		AICon->StopMovement(); // �̵� ����
	}

	if (Health <= 0.0f) // ü���� 0 ������ ��� ���
	{
		Die();
	}

	return DamageApplied;
}

void AEnemyDog::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bIsDead) return;

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (bIsInAirStun)
	{
		if (AnimInstance && InAirStunMontage)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit animation ended while airborne. Resuming InAirStunMontage."));
			// ���� ��Ÿ�� ��� ���� ��Ʈ ��Ÿ�� Ÿ�̸Ӱ� �ݺ����� ���ϰ� ���� ���� Ÿ�̸� ����
			GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);

			AnimInstance->Montage_Play(InAirStunMontage, 1.0f);

			// ���� ��Ÿ�� �ݺ� Ÿ�̸� �簳
			GetWorld()->GetTimerManager().SetTimer(
				StunAnimRepeatTimerHandle,
				this,
				&AEnemyDog::PlayStunMontageLoop,
				0.4f,
				true
			);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit animation ended on ground. No need to resume InAirStunMontage."));
	}
}

void AEnemyDog::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	// Ÿ�̸�
	GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

	// ��Ÿ�� ���(����/���� ����)
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if ((bIsInAirStun || GetCharacterMovement()->IsFalling()) && InAirStunDeathMontage)
	{
		AnimInstance->Montage_Play(InAirStunDeathMontage, 1.0f);
	}
	else if (DeadMontages.Num() > 0)
	{
		int32 Rand = FMath::RandRange(0, DeadMontages.Num() - 1);
		AnimInstance->Montage_Play(DeadMontages[Rand], 1.0f);
	}

	// ��� ȣ�� ���� ���� ���� �ð� �� ����/����
	constexpr float FixedExplosionDelay = 0.5f;
	GetWorld()->GetTimerManager().SetTimer(
		DeathTimerHandle,
		[this]()
		{
			Explode();
			HideEnemy();
		},
		FixedExplosionDelay,
		false
	);

	// AI ��Ʈ�ѷ� ����
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		AICon->StopAI();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AIController is NULL! Can not stop AI."));
	}

	// �̵� ��Ȱ��ȭ
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	SetActorTickEnabled(false); // AI Tick ����

	UE_LOG(LogTemp, Warning, TEXT("Die() called: Setting HideEnemy timer"));
}

void AEnemyDog::OnDeadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == InAirStunDeathMontage) // �ο��� ����̸�
	{
		UE_LOG(LogTemp, Warning, TEXT("InAir death montage ended, triggering explosion"));
		Explode();
		HideEnemy();
	}
	else
	{
		Explode();
		HideEnemy();
	}
}

void AEnemyDog::Explode()
{
	if (bIsDead == false) return; // ������� ���� ���¿��� ���� ����

	// ���� �ݰ� �� �÷��̾ ���� ����
	FVector ExplosionCenter = GetActorLocation();
	float ExplosionRadiusTemp = ExplosionRadius; // ���� �״�� ���

	// Pawn ä�η� �÷��̾� ����
	TArray<FOverlapResult> Overlaps;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ExplosionRadiusTemp);

	bool bHasOverlaps = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		ExplosionCenter,
		FQuat::Identity,
		ECC_Pawn,
		CollisionShape
	);

	if (bHasOverlaps)
	{
		ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		for (auto& Overlap : Overlaps)
		{
			if (Overlap.GetActor() == PlayerCharacter)
			{
				UGameplayStatics::ApplyDamage(
					PlayerCharacter,
					ExplosionDamage,
					GetInstigator() ? GetInstigator()->GetController() : nullptr,
					this,
					nullptr
				);
				break; // �÷��̾� 1�� ó��
			}
		}
	}

	// ���� ���� �ð�ȭ
	DrawDebugSphere(GetWorld(), ExplosionCenter, ExplosionRadius, 32, FColor::Red, false, 1.0f, 0, 2.0f);

	// ���� ����Ʈ ����
	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ExplosionEffect,
			GetActorLocation(),
			GetActorRotation(),
			FVector(1.0f),
			true,
			true
		);
	}

	// ���� ���� ���
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}
}

void AEnemyDog::HideEnemy()
{
	if (!bIsDead) return; // ������� �ʾ����� ����

	UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyDog - Memory Cleanup"));

	//// GameMode�� Enemy �ı� �˸�
	//if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	//{
	//	GameMode->OnEnemyDestroyed(this);
	//}

	// 1. �̺�Ʈ �� ��������Ʈ ���� (�ֿ켱)
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // ��� Ÿ�̸� ����

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // �ִ� �ν��Ͻ� ���� �޾ƿ�
	if (AnimInstance && IsValid(AnimInstance)) // �ִ� �ν��Ͻ� ��ȿ�� �˻�
	{
		// �ִϸ��̼� �̺�Ʈ ���ε� ���� ����
		AnimInstance->OnMontageEnded.RemoveAll(this); // ��Ÿ�� ���� �̺�Ʈ ���ε� ����
		AnimInstance->OnMontageBlendingOut.RemoveAll(this); // ��Ÿ�� ���� �ƿ� �̺�Ʈ ���ε� ��ü
		AnimInstance->OnMontageStarted.RemoveAll(this); // ��Ÿ�� ���� �̺�Ʈ ���ε� ����
	}

	// 2. AI �ý��� ���� ����
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI ��Ʈ�ѷ� ���� �޾ƿ�
	if (AICon && IsValid(AICon)) // AI ��Ʈ�ѷ� ��ȿ�� �˻�
	{
		AICon->StopAI(); // AI ���� �ߴ�
		AICon->UnPossess(); // ��Ʈ�ѷ�-�� ���� ����
		AICon->Destroy(); // AI ��Ʈ�ѷ� ���� ����
	}

	// 4. �����Ʈ �ý��� ����
	UCharacterMovementComponent* MovementComp = GetCharacterMovement(); // ĳ���� �����Ʈ ������Ʈ ���� �޾ƿ�
	if (MovementComp && IsValid(MovementComp)) // �����Ʈ ������Ʈ ��ȿ�� �˻�
	{
		MovementComp->DisableMovement(); // �̵� ��Ȱ��ȭ
		MovementComp->StopMovementImmediately(); // ���� �̵� ��� �ߴ�
		MovementComp->SetMovementMode(EMovementMode::MOVE_None); // Move��� None �������� �׺���̼ǿ��� ����
		MovementComp->SetComponentTickEnabled(false); // �����Ʈ ������Ʈ Tick ��Ȱ��ȭ
	}

	// 5. �޽� ������Ʈ ����
	USkeletalMeshComponent* MeshComp = GetMesh(); // �޽� ������Ʈ ���� �޾ƿ�
	if (MeshComp && IsValid(MeshComp)) // �޽� ������Ʈ ��ȿ�� �˻�
	{
		// ������ �ý��� ��Ȱ��ȭ
		MeshComp->SetVisibility(false); // �޽� ���ü� ��Ȱ��ȭ
		MeshComp->SetHiddenInGame(true); // ���� �� ���� ó��

		// ���� �ý��� ��Ȱ��ȭ
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // NoCollision �������� �浹�˻� ��Ȱ��ȭ
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // ECR_Ignore �������� �浹���� ����

		// ������Ʈ �ý��� ��Ȱ��ȭ
		MeshComp->SetComponentTickEnabled(false); // Tick ��Ȱ��ȭ

		// �ִϸ��̼� ���� ����
		MeshComp->SetAnimInstanceClass(nullptr); // ABP ���� ����
		MeshComp->SetSkeletalMesh(nullptr); // ���̷�Ż �޽� ���� ����
	}

	// 6. ���� ���� �ý��� ����
	SetActorHiddenInGame(true); // ���� ������ ��Ȱ��ȭ
	SetActorEnableCollision(false); // ���� �浹 ��Ȱ��ȭ
	SetActorTickEnabled(false); // ���� Tick ��Ȱ��ȭ
	SetCanBeDamaged(false); // ������ ó�� ��Ȱ��ȭ

	// 7. ���� ������ ó�� �Ϸ� �� ���� �����ӿ� �����ϰ� ���� ���� (ũ���� ����)
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyDog>(this)]() // ����Ʈ ������ WeakObjectPtr�� ���� ������ ����Ͽ� �����ϰ� ���� ����
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // ���� ������ ���Ͱ� ��ȿ�ϰ� �ı����� �ʾҴٸ�
			{
				WeakThis->Destroy(); // ���� ���� ����
				UE_LOG(LogTemp, Warning, TEXT("EnemyDog Successfully Destroyed"));
			}
		});
}

void AEnemyDog::EnterInAirStunState(float Duration)
{
	if (bIsDead || bIsInAirStun) return;
	UE_LOG(LogTemp, Warning, TEXT("Entering InAirStunState..."));

	bIsInAirStun = true;

	// AI ���߱� (�ٷ� �̵� �������� �ʰ�, ���� ���� �������� �ٽ� Ȱ��ȭ)
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Stopping AI manually..."));
		AICon->StopMovement();
	}

	// ���� ���� ���� (LaunchCharacter ���� ����)
	FVector LaunchDirection = FVector(0.0f, 0.0f, 1.0f); // ���� ����
	float LaunchStrength = 600.0f; // ���� �� ����
	LaunchCharacter(LaunchDirection * LaunchStrength, true, true);

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s launched upwards! Current Location: %s"), *GetName(), *GetActorLocation().ToString());

	// ���� �ð� �� �߷� ���� (��� 0���� ����� ������ ���ص� �� ����)
	FTimerHandle GravityDisableHandle;
	GetWorld()->GetTimerManager().SetTimer(
		GravityDisableHandle,
		[this]()
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			GetCharacterMovement()->GravityScale = 0.0f;
			GetCharacterMovement()->Velocity = FVector::ZeroVector; // ��ġ ����
			UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s gravity disabled, now floating!"), *GetName());
		},
		0.3f, // 0.3�� �� �߷� ����
		false
	);

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // �ִ��ν��Ͻ��� �ҷ���
	// ���� �ִϸ��̼� ����
	if (AnimInstance && InAirStunMontage)
	{
		AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
	}

	// ���� ���� Ÿ�̸� ���� (������ �� 0.6���� ����)
	GetWorld()->GetTimerManager().SetTimer(
		StunAnimRepeatTimerHandle,
		this,
		&AEnemyDog::PlayStunMontageLoop,
		0.4f,    // ��Ÿ�� ���̺��� ª��
		true     // ����
	);

	// ���� ��ü ���ӽð� Ÿ�̸� ������ Exit
	GetWorld()->GetTimerManager().SetTimer(
		StunTimerHandle,
		this,
		&AEnemyDog::ExitInAirStunState,
		Duration,
		false
	);

	// ���� �ð��� ������ ���� ���·� ����
	GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, this, &AEnemyDog::ExitInAirStunState, Duration, false);
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s is now stunned for %f seconds!"), *GetName(), Duration);
}

void AEnemyDog::PlayStunMontageLoop()
{
	if (bIsDead || !bIsInAirStun) return;

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance && InAirStunMontage)
	{
		// ��Ʈ ��Ÿ�� �Ǵ� ��� ��Ÿ�ְ� ������� ��� ���� ��Ÿ�� �ݺ� ��� ����
		bool bIsHitPlaying = false;
		bool bIsDeathPlaying = false;

		// HitMontages �迭�� ���� �� ���̶� ��������� üũ
		for (auto* HitMontage : HitMontages)
		{
			if (AnimInstance->Montage_IsPlaying(HitMontage))
			{
				bIsHitPlaying = true;
				break;
			}
		}
		// ��� ��Ÿ�� ��� üũ
		if (AnimInstance->Montage_IsPlaying(InAirStunDeathMontage))
		{
			bIsDeathPlaying = true;
		}
		else
		{
			for (auto* DeathMontage : DeadMontages)
			{
				if (AnimInstance->Montage_IsPlaying(DeathMontage))
				{
					bIsDeathPlaying = true;
					break;
				}
			}
		}
		if (!bIsHitPlaying && !bIsDeathPlaying)
		{
			AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
		}
	}
}

void AEnemyDog::ExitInAirStunState()
{
	if (bIsDead) return;
	UE_LOG(LogTemp, Warning, TEXT("Exiting InAirStunState..."));

	bIsInAirStun = false;

	// Ÿ�̸� ����
	GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);

	// �߷� ���� �� ���� ���·� ����
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	GetCharacterMovement()->GravityScale = 1.5f; // ���� �� ������ ����
	ApplyBaseWalkSpeed();

	// AI �̵� �ٽ� Ȱ��ȭ
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reactivating AI movement..."));
		GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
		GetCharacterMovement()->SetDefaultMovementMode();

		// �ٽ� �̵� ����
		AICon->MoveToActor(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	}

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // �ִ��ν��Ͻ��� �ҷ���
	// �ִϸ��̼� ����
	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(0.1f);
	}

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s has recovered from stun and resumed AI behavior!"), *GetName());

	bIsInAirStun = false;
}

void AEnemyDog::ApplyGravityPull(FVector ExplosionCenter, float PullStrength)
{
	if (bIsDead) return; // ���� ���� �������� ����

	// ���� ���� ������ ���� ���Ƶ��̴� ����
	FVector Direction = ExplosionCenter - GetActorLocation();
	float Distance = Direction.Size();
	Direction.Normalize();  // ���� ���� ����ȭ

	// �Ÿ��� ���� �� ���� (�������� �� ���ϰ�)
	float DistanceFactor = FMath::Clamp(1.0f - (Distance / 500.0f), 0.1f, 1.0f);
	float AdjustedPullStrength = PullStrength * DistanceFactor;

	// ĳ���Ͱ� ���߿� �ִ� ���¶�� �� ���� �� ����
	if (GetCharacterMovement()->IsFalling())
	{
		AdjustedPullStrength *= 1.5f;
	}

	// ���ο� �ӵ� ����
	FVector NewVelocity = Direction * AdjustedPullStrength;
	GetCharacterMovement()->Velocity = NewVelocity;

	// ��� �׺���̼� �̵� ��Ȱ��ȭ
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	// ���� �ð� �� �׺���̼� �̵� �ٽ� Ȱ��ȭ
	FTimerHandle ResetMovementHandle;
	GetWorld()->GetTimerManager().SetTimer(
		ResetMovementHandle,
		[this]()
		{
			GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
		},
		0.5f, // 0.5�� �� ���� �̵� ���� ����
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s pulled toward explosion center with strength %f"),
		*GetName(), AdjustedPullStrength);
}

void AEnemyDog::RaycastAttack()
{
	if (bHasExecutedRaycast) return;
	bHasExecutedRaycast = true;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;

	FVector StartLocation = GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector Direction = (PlayerLocation - StartLocation).GetSafeNormal();
	FVector EndLocation = StartLocation + (Direction * 150.0f);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Pawn,
		CollisionParams
	);

	// �ð�ȭ �߰� - ��Ʈ ���ο� ���� ���� ����
	if (bHit && HitResult.GetActor() == PlayerPawn)
	{
		// ��Ʈ �� ������ ����
		DrawDebugLine(
			GetWorld(),
			StartLocation,
			HitResult.Location,
			FColor::Red,
			false,
			3.0f, // 3�ʰ� ǥ��
			0,
			5.0f // ����
		);

		// ��Ʈ ������ ��ü ǥ��
		DrawDebugSphere(
			GetWorld(),
			HitResult.Location,
			20.0f,
			12,
			FColor::Red,
			false,
			3.0f
		);

		UGameplayStatics::ApplyPointDamage(
			PlayerPawn, Damage, StartLocation, HitResult, nullptr, this, nullptr
		);

	}
	else
	{
		// �̽� �� �ʷϻ� ����
		DrawDebugLine(
			GetWorld(),
			StartLocation,
			EndLocation,
			FColor::Green,
			false,
			3.0f, // 3�ʰ� ǥ��
			0,
			5.0f // ����
		);
	}
}

void AEnemyDog::StartAttack()
{
	bIsAttacking = true;
	bHasExecutedRaycast = false;
	RaycastHitActors.Empty();
	DamagedActors.Empty();
	bCanAttack = false;
	RaycastAttack();
}

void AEnemyDog::EndAttack()
{
	bIsAttacking = false;
	bHasExecutedRaycast = false;
	RaycastHitActors.Empty();
	DamagedActors.Empty();
	bCanAttack = true;
}