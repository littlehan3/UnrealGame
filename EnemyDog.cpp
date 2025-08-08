#include "EnemyDog.h"
#include "EnemyDogAnimInstance.h"
#include "EnemyDogAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyDog::AEnemyDog()
{
	PrimaryActorTick.bCanEverTick = true;
	bCanAttack = true;

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
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // ���õ� ��Ÿ�ָ� ���
	}

	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // ���� ���� ��
	if (AICon)
	{
		AICon->StopMovement(); // �̵� ����
	}
	
	bCanAttack = false;
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

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // �ִ��ν��Ͻ��� �ҷ���
	// ���� ���� ������ ���� �ٽ� ���� �ִϸ��̼� ����
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

void AEnemyDog::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	StopActions();

	float HideTime = 3.0f;

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // �ִ��ν��Ͻ��� �ҷ���
	if (bIsInAirStun && InAirStunDeathMontage) // ���߿��� ��� ��
	{
		float AirDeathDuration = InAirStunDeathMontage->GetPlayLength();
		AnimInstance->Montage_Play(InAirStunDeathMontage, 1.0f); // �ִϸ��̼� ����ӵ� ����
		HideTime = AirDeathDuration * 0.9f; // �ִϸ��̼� ��� �ð��� ������ % ��ŭ ��� �� �����
	}
	else if (!AnimInstance || DeadMontages.Num() == 0) return; // �ִ��ν��Ͻ��� ���ų� ��Ÿ�ֹ迭�� ����ٸ� ����
	int32 RandomIndex = FMath::RandRange(0, DeadMontages.Num() - 1); // ��� ��Ÿ�� ������� ����
	UAnimMontage* SelectedMontage = DeadMontages[RandomIndex]; // �ش� ���� �ȿ��� ��Ÿ�ָ� ����

	if (SelectedMontage)
	{
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // ���õ� ��Ÿ�ָ� ���
	}
	else
	{
		// ��� �ִϸ��̼��� ���� ��� ��� ������� ��
		HideEnemy();
		return;
	}

	// ���� �ð� �� ��������� ����
	GetWorld()->GetTimerManager().SetTimer(DeathTimerHandle, this, &AEnemyDog::HideEnemy, HideTime, false);

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

	Explode();
	UE_LOG(LogTemp, Warning, TEXT("Die() called: Setting HideEnemy timer"));
}

void AEnemyDog::Explode()
{
	if (bIsDead == false) return; // ������� ���� ���¿��� ���� ����

	FVector ExplosionCenter = GetActorLocation();
	float ExplosionRadius = 300.0f; // �ʿ�� UPROPERTY�� ����
	float ExplosionDamage = 40.0f;

	// ���� ���� ���� ���͵鿡�� �������� ����
	UGameplayStatics::ApplyRadialDamage(
		GetWorld(),
		ExplosionDamage,
		ExplosionCenter,
		ExplosionRadius,
		nullptr, // DamageTypeClass (�⺻ ������ Ÿ��)
		TArray<AActor*>(), // IgnoreActors (����)
		this, // ������ ���� ����
		GetController(), // Instigator(��Ʈ�ѷ�)
		true // DoFullDamage
	);

	// ���� ���� �ð�ȭ (����׿�)
	DrawDebugSphere(GetWorld(), ExplosionCenter, ExplosionRadius, 32, FColor::Red, false, 3.0f, 0, 2.0f);

	// ���� ��ƼŬ, ���� �� ȿ��

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog exploded at location: %s"), *ExplosionCenter.ToString());
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

	// ���� �ð��� ������ ���� ���·� ����
	GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, this, &AEnemyDog::ExitInAirStunState, Duration, false);
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s is now stunned for %f seconds!"), *GetName(), Duration);
}

void AEnemyDog::ExitInAirStunState()
{
	if (bIsDead) return;
	UE_LOG(LogTemp, Warning, TEXT("Exiting InAirStunState..."));

	bIsInAirStun = false;

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

void AEnemyDog::StartAttack()
{
}

void AEnemyDog::EndAttack()
{
}

void AEnemyDog::StopActions()
{
}
