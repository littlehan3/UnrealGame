#include "EnemyDog.h"
#include "EnemyDogAnimInstance.h" // �ִ� �ν��Ͻ� Ŭ���� ����
#include "EnemyDogAIController.h" // AI ��Ʈ�ѷ� Ŭ���� ����
#include "Kismet/GameplayStatics.h" // �����÷��� ���� ��ƿ��Ƽ �Լ� ��� (UGameplayStatics)
#include "GameFramework/CharacterMovementComponent.h" // ĳ���� �����Ʈ ������Ʈ ����
#include "NiagaraFunctionLibrary.h" // ���̾ư��� ����Ʈ ���� �Լ� ���
#include "Engine/OverlapResult.h" // FOverlapResult ����ü ���
#include "MainGameModeBase.h" // ���� ���� ��� ���� (�� �ı� �˸���)

AEnemyDog::AEnemyDog()
{
	PrimaryActorTick.bCanEverTick = true; // �� ������ Tick �Լ��� ȣ���ϵ��� ����
	bCanAttack = true; // �⺻������ ���� ������ ���·� ����
	bIsAttacking = false; // ó������ ���� ���� �ƴ� ���·� ����

	AIControllerClass = AEnemyDogAIController::StaticClass(); // �� ĳ���Ͱ� ����� AI ��Ʈ�ѷ��� ����
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; // ���忡 ��ġ�ǰų� ������ �� AI ��Ʈ�ѷ��� �ڵ����� �����ϵ��� ����
	GetMesh()->SetAnimInstanceClass(UEnemyDogAnimInstance::StaticClass()); // ���̷�Ż �޽ÿ� ����� �ִ� �ν��Ͻ� Ŭ���� ����

	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // �⺻ �̵� �ӵ� ����
}

void AEnemyDog::BeginPlay()
{
	Super::BeginPlay(); // �θ� Ŭ������ BeginPlay �Լ� ȣ��
	SetCanBeDamaged(true); // �� ���Ͱ� �������� ���� �� �ֵ��� ����
	SetActorTickInterval(0.05f); // Tick �Լ� ȣ�� �ֱ⸦ 0.05�ʷ� �����Ͽ� ����ȭ (�ʴ� 20��)

	if (!GetController()) // AI ��Ʈ�ѷ��� �Ҵ���� �ʾҴٸ�
	{
		SpawnDefaultController(); // �⺻ ��Ʈ�ѷ��� �����Ͽ� �Ҵ�
		UE_LOG(LogTemp, Warning, TEXT("EnemyDog AI Controller manually spawned"));
	}

	AAIController* AICon = Cast<AAIController>(GetController()); // ���� �Ҵ�� ��Ʈ�ѷ��� AI ��Ʈ�ѷ��� ĳ����
	if (AICon) // ĳ������ �����ߴٸ�
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyDog AI Controller Assigend %s"), *AICon->GetName());
	}
	else // ĳ������ �����ߴٸ�
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyDog AI Controller is Null!"));
	}

	SetUpAI(); // AI �ʱ� ���� �Լ� ȣ��
	PlaySpawnIntroAnimation(); // ���� ��Ʈ�� �ִϸ��̼� ��� �Լ� ȣ��
}

void AEnemyDog::PostInitializeComponents()
{
	Super::PostInitializeComponents(); // �θ� Ŭ������ �Լ� ȣ��
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]() // ���� ƽ�� ����ǵ��� Ÿ�̸� ���� (��Ʈ�ѷ� �Ҵ� ����)
		{
			AAIController* AICon = Cast<AAIController>(GetController()); // AI ��Ʈ�ѷ� ��������
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
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // �ִ� �ν��Ͻ� ��������
	if (!AnimInstance) // �ִ� �ν��Ͻ��� ���ٸ�
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyDog AnimInstance not found"));
		return; // �Լ� ����
	}

	bIsPlayingIntro = true; // ��Ʈ�� ��� �� ���·� ����
	bCanAttack = false; // ���� �߿��� ���� �Ұ� ���·� ����

	// AI �̵� ����
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI ��Ʈ�ѷ� ��������
	if (AICon)
	{
		AICon->StopMovement(); // AI�� �̵��� ����
	}

	PlayAnimMontage(SpawnIntroMontage); // ���� ��Ʈ�� ��Ÿ�� ���

	// ��Ÿ�ְ� ������ OnIntroMontageEnded �Լ��� ȣ��ǵ��� ��������Ʈ ���ε�
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyDog::OnIntroMontageEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, SpawnIntroMontage);
}

void AEnemyDog::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("Dog Intro Montage Ended"));
	bIsPlayingIntro = false; // ��Ʈ�� ��� ���� ����
	bCanAttack = true; // ���� ���� ���·� ����

}

void AEnemyDog::ApplyBaseWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // �⺻ �̵� �ӵ��� 600���� ����
	GetCharacterMovement()->MaxAcceleration = 5000.0f; // ���ӵ��� ���� ��� �ִ� �ӵ��� �����ϰ� ����
	GetCharacterMovement()->BrakingFrictionFactor = 10.0f; // ���� �������� ���� ��� ���ߵ��� ����
}

void AEnemyDog::SetUpAI()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking); // �׺���̼� �޽ø� ���� �̵��ϴ� ���� ����
}

void AEnemyDog::PlayNormalAttackAnimation()
{
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // �ִ� �ν��Ͻ��� ������
	if (!AnimInstance || NormalAttackMontages.Num() == 0) return; // �ִ� �ν��Ͻ��� ���ų� ���� ��Ÿ�� �迭�� ����ٸ� ����
	if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return; // �ٸ� ��Ÿ�ְ� ��� ���̶�� ����

	int32 RandomIndex = FMath::RandRange(0, NormalAttackMontages.Num() - 1); // ���� ��Ÿ�� �迭���� ���� �ε��� ����
	UAnimMontage* SelectedMontage = NormalAttackMontages[RandomIndex]; // ���õ� ��Ÿ��

	if (SelectedMontage)
	{
		AnimInstance->Montage_Play(SelectedMontage, 1.5f); // ���õ� ��Ÿ�ָ� 1.5������� ���

		// ��Ÿ�� ���� �� OnAttackMontageEnded �Լ��� ȣ��ǵ��� ��������Ʈ ����
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyDog::OnAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI ��Ʈ�ѷ� ��������
	if (AICon)
	{
		AICon->StopMovement(); // ���� �ִϸ��̼� ���� �̵� ����
	}

	bCanAttack = false; // ���� �߿��� �ٽ� ������ �� ������ ����
	bIsAttacking = true; // ���� �� ���·� ����
}

void AEnemyDog::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bCanAttack = true; // ���� ���� ���·� ����
	bIsAttacking = false; // ���� �� ���� ����
	bHasExecutedRaycast = false; // ���� ������ ���� ����ĳ��Ʈ ���� ���� �ʱ�ȭ
}

float AEnemyDog::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || bIsPlayingIntro) return 0.0f; // �̹� �׾��ų� ��Ʈ�� ��� �߿��� �������� ���� ����

	float DamageApplied = FMath::Min(Health, DamageAmount); // ���� ����� ������ (���� ü�� �̻����� ������ �ʵ���)
	Health -= DamageApplied; // ü�� ����
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog took %f damage, Health remaining: %f"), DamageAmount, Health);

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance()); // �ִ� �ν��Ͻ� ��������
	if (!AnimInstance || HitMontages.Num() == 0) return 0.0f; // �ִ� �ν��Ͻ��� ���ų� �ǰ� ��Ÿ�ְ� ������ ����

	int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1); // �ǰ� ��Ÿ�� �迭���� ���� �ε��� ����
	UAnimMontage* SelectedMontage = HitMontages[RandomIndex]; // ���õ� �ǰ� ��Ÿ��

	if (SelectedMontage)
	{
		AnimInstance->Montage_Play(SelectedMontage, 1.0f); // ���õ� ��Ÿ�� ���

		// �ǰ� ��Ÿ�� ���� �� OnHitMontageEnded �Լ��� ȣ��ǵ��� ��������Ʈ ����
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyDog::OnHitMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedMontage);
	}

	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI ��Ʈ�ѷ� ��������
	if (AICon)
	{
		AICon->StopMovement(); // �ǰ� �ִϸ��̼� ���� �̵� ����
	}

	if (Health <= 0.0f) // ü���� 0 ���϶��
	{
		Die(); // ��� ó�� �Լ� ȣ��
	}

	return DamageApplied; // ���� ����� ������ �� ��ȯ
}

void AEnemyDog::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bIsDead) return; // �̹� ��� ���¸� �ƹ��͵� ���� ����

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (bIsInAirStun) // ���� ���� ���¿��� �ǰ� ��Ÿ�ְ� �����ٸ�
	{
		if (AnimInstance && InAirStunMontage)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit animation ended while airborne. Resuming InAirStunMontage."));
			GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle); // ���� ���� �ݺ� Ÿ�̸Ӹ� ���� (�ߺ� ����)

			AnimInstance->Montage_Play(InAirStunMontage, 1.0f); // �ٽ� ���� ���� ��Ÿ�� ���

			// ���� ��Ÿ�ָ� ��� �ݺ��ϱ� ���� Ÿ�̸Ӹ� �ٽ� ����
			GetWorld()->GetTimerManager().SetTimer(
				StunAnimRepeatTimerHandle,
				this,
				&AEnemyDog::PlayStunMontageLoop,
				0.4f,
				true
			);
		}
	}
	else // ���󿡼� �ǰ� ��Ÿ�ְ� �����ٸ�
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit animation ended on ground. No need to resume InAirStunMontage."));
	}
}

void AEnemyDog::Die()
{
	if (bIsDead) return; // �̹� ��� ó�� ���̶�� �ߺ� ���� ����
	bIsDead = true; // ��� ���·� ��ȯ

	// Ȱ��ȭ�� ���� ���� Ÿ�̸� ��� ����
	GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

	// ���� ���¿� �´� ��� ��Ÿ�� ���
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if ((bIsInAirStun || GetCharacterMovement()->IsFalling()) && InAirStunDeathMontage) // ���� ���� ���̰ų� ���� ���� ��
	{
		AnimInstance->Montage_Play(InAirStunDeathMontage, 1.0f); // ���� ��� ��Ÿ�� ���
	}
	else if (DeadMontages.Num() > 0) // ���� ���� ��
	{
		int32 Rand = FMath::RandRange(0, DeadMontages.Num() - 1);
		AnimInstance->Montage_Play(DeadMontages[Rand], 1.0f); // ���� ��� ��Ÿ�� �� �ϳ��� ���� ���
	}

	// 0.5�� �Ŀ� ���� �� ���� ó�� �Լ��� ȣ���ϵ��� Ÿ�̸� ����
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
		AICon->StopAI(); // AI ���� ��ü �ߴ�
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AIController is NULL! Can not stop AI."));
	}

	// �̵� ���� ������Ʈ ��Ȱ��ȭ
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	SetActorTickEnabled(false); // ������ Tick ��Ȱ��ȭ�� ���� �δ� ����

	UE_LOG(LogTemp, Warning, TEXT("Die() called: Setting HideEnemy timer"));
}

void AEnemyDog::OnDeadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == InAirStunDeathMontage) // ���� ��� ��Ÿ�ְ� �����ٸ�
	{
		UE_LOG(LogTemp, Warning, TEXT("InAir death montage ended, triggering explosion"));
		Explode(); // ���� �Լ� ȣ��
		HideEnemy(); // ���� �� ���� �Լ� ȣ��
	}
	else // ���� ��� ��Ÿ�ְ� �����ٸ�
	{
		Explode(); // ���� �Լ� ȣ��
		HideEnemy(); // ���� �� ���� �Լ� ȣ��
	}
}

void AEnemyDog::Explode()
{
	if (bIsDead == false) return; // ��� ���°� �ƴϸ� �������� ����

	// ���� ���� �� �÷��̾� ���� �� ������ ó��
	FVector ExplosionCenter = GetActorLocation(); // ���� �߽���
	float ExplosionRadiusTemp = ExplosionRadius; // ���� �ݰ�

	// ���� �ݰ� ���� �ִ� Pawn ä���� ���͵��� ����
	TArray<FOverlapResult> Overlaps;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ExplosionRadiusTemp);
	bool bHasOverlaps = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		ExplosionCenter,
		FQuat::Identity,
		ECC_Pawn,
		CollisionShape
	);

	if (bHasOverlaps) // ������ ���Ͱ� �ִٸ�
	{
		ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0); // �÷��̾� ĳ���� ��������
		for (auto& Overlap : Overlaps) // ��� ������ ���Ϳ� ���� �ݺ�
		{
			if (Overlap.GetActor() == PlayerCharacter) // ������ ���Ͱ� �÷��̾���
			{
				UGameplayStatics::ApplyDamage( // ������ ����
					PlayerCharacter,
					ExplosionDamage,
					GetInstigator() ? GetInstigator()->GetController() : nullptr,
					this,
					nullptr
				);
				break; // �÷��̾�� �� ���̹Ƿ� ã���� �ٷ� �ݺ� ����
			}
		}
	}

	// ����׿� ���� ���� �ð�ȭ
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
	if (!bIsDead) return; // ��� ���°� �ƴϸ� �������� ����

	UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyDog - Memory Cleanup"));

	// ���Ӹ�忡 ���� �ı��Ǿ����� �˸� (�� ī��Ʈ ���� ��)
	if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->OnEnemyDestroyed(this);
	}

	// 1. ��� Ÿ�̸� ����
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // �ִ� �ν��Ͻ� ����
	if (AnimInstance && IsValid(AnimInstance)) // �ִ� �ν��Ͻ��� ��ȿ�ϴٸ�
	{
		// ��� ��������Ʈ ���ε� ����
		AnimInstance->OnMontageEnded.RemoveAll(this);
		AnimInstance->OnMontageBlendingOut.RemoveAll(this);
		AnimInstance->OnMontageStarted.RemoveAll(this);
	}

	// 2. AI ��Ʈ�ѷ� ����
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController()); // AI ��Ʈ�ѷ� ����
	if (AICon && IsValid(AICon)) // AI ��Ʈ�ѷ��� ��ȿ�ϴٸ�
	{
		AICon->StopAI(); // AI ���� �ߴ�
		AICon->UnPossess(); // ��Ʈ�ѷ��� ���� ���� ����
		AICon->Destroy(); // AI ��Ʈ�ѷ� ���� ��ü�� �ı�
	}

	// 4. �����Ʈ ������Ʈ ����
	UCharacterMovementComponent* MovementComp = GetCharacterMovement(); // �����Ʈ ������Ʈ ����
	if (MovementComp && IsValid(MovementComp)) // ������Ʈ�� ��ȿ�ϴٸ�
	{
		MovementComp->DisableMovement(); // �̵� ��Ȱ��ȭ
		MovementComp->StopMovementImmediately(); // ���� �̵� ��� �ߴ�
		MovementComp->SetMovementMode(EMovementMode::MOVE_None); // �̵� ��带 '����'���� �����Ͽ� �׺���̼ǿ��� ����
		MovementComp->SetComponentTickEnabled(false); // ������Ʈ Tick ��Ȱ��ȭ
	}

	// 5. �޽� ������Ʈ ����
	USkeletalMeshComponent* MeshComp = GetMesh(); // �޽� ������Ʈ ����
	if (MeshComp && IsValid(MeshComp)) // ������Ʈ�� ��ȿ�ϴٸ�
	{
		MeshComp->SetVisibility(false); // ���������� �ʵ��� ���ü� ��Ȱ��ȭ
		MeshComp->SetHiddenInGame(true); // ���� ������ ���� ó��

		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �浹 �˻� ��Ȱ��ȭ
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // ��� ä�ο� ���� �浹 ���� ����

		MeshComp->SetComponentTickEnabled(false); // ������Ʈ Tick ��Ȱ��ȭ

		MeshComp->SetAnimInstanceClass(nullptr); // ����� �ִ� �ν��Ͻ� ���� ����
		MeshComp->SetSkeletalMesh(nullptr); // ����� ���̷�Ż �޽� ���� ����
	}

	// 6. ���� ��ü�� �ý��� ����
	SetActorHiddenInGame(true); // ���͸� ���� ������ ����
	SetActorEnableCollision(false); // ������ �浹 ��Ȱ��ȭ
	SetActorTickEnabled(false); // ������ Tick ��Ȱ��ȭ
	SetCanBeDamaged(false); // �������� ���� �� ������ ����

	// 7. ���� �����ӿ� �����ϰ� ���� ���� (ũ���� ����)
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AEnemyDog>(this)]() // ���� ����(Weak Ptr)�� ����Ͽ� �����ϰ� ���� ����
		{
			if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed()) // ������ ��ȿ�ϰ�, �̹� �ı� ���� �ƴ϶��
			{
				WeakThis->Destroy(); // ���͸� ���忡�� ������ ����
				UE_LOG(LogTemp, Warning, TEXT("EnemyDog Successfully Destroyed"));
			}
		});
}

void AEnemyDog::EnterInAirStunState(float Duration)
{
	if (bIsDead || bIsInAirStun) return; // �̹� �׾��ų� ���� ���¶�� �ߺ� ���� ����
	UE_LOG(LogTemp, Warning, TEXT("Entering InAirStunState..."));

	bIsInAirStun = true; // ���� ���� ���·� ��ȯ

	// AI �̵� ����
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Stopping AI manually..."));
		AICon->StopMovement();
	}

	// ĳ���͸� ���� ���
	FVector LaunchDirection = FVector(0.0f, 0.0f, 1.0f); // ��ġ ���� (��)
	float LaunchStrength = 600.0f; // ��ġ ����
	LaunchCharacter(LaunchDirection * LaunchStrength, true, true); // ĳ���� ��ġ ����
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s launched upwards! Current Location: %s"), *GetName(), *GetActorLocation().ToString());

	// 0.3�� �� �߷��� ��Ȱ��ȭ�Ͽ� ���߿� ���ְ� ����
	FTimerHandle GravityDisableHandle;
	GetWorld()->GetTimerManager().SetTimer(
		GravityDisableHandle,
		[this]()
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Flying); // ���� ���� ��ȯ
			GetCharacterMovement()->GravityScale = 0.0f; // �߷� ��Ȱ��ȭ
			GetCharacterMovement()->Velocity = FVector::ZeroVector; // �ӵ��� 0���� ����� ��ġ ����
			UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s gravity disabled, now floating!"), *GetName());
		},
		0.3f, // ���� �ð�
		false
	);

	// ���� �ִϸ��̼� ���
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance && InAirStunMontage)
	{
		AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
	}

	// ���� �ִϸ��̼��� �ݺ� ����ϱ� ���� Ÿ�̸� ����
	GetWorld()->GetTimerManager().SetTimer(
		StunAnimRepeatTimerHandle,
		this,
		&AEnemyDog::PlayStunMontageLoop,
		0.4f,    // ��Ÿ�� ���̺��� �ణ ª�� �����Ͽ� �ε巴�� ����
		true     // �ݺ�
	);

	// ���� ��ü ���ӽð��� ������ ���¸� �����ϴ� Ÿ�̸� ����
	GetWorld()->GetTimerManager().SetTimer(
		StunTimerHandle,
		this,
		&AEnemyDog::ExitInAirStunState,
		Duration,
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s is now stunned for %f seconds!"), *GetName(), Duration);
}

void AEnemyDog::PlayStunMontageLoop()
{
	if (bIsDead || !bIsInAirStun) return; // �׾��ų� ���� ���°� �ƴϸ� ����

	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance && InAirStunMontage)
	{
		// �ǰ� �Ǵ� ��� ��Ÿ�ְ� ��� ������ Ȯ��
		bool bIsHitPlaying = false;
		bool bIsDeathPlaying = false;

		// HitMontages �迭�� ���Ե� ��Ÿ�� �� �ϳ��� ��� ������ üũ
		for (auto* HitMontage : HitMontages)
		{
			if (AnimInstance->Montage_IsPlaying(HitMontage))
			{
				bIsHitPlaying = true;
				break;
			}
		}
		// ��� ��Ÿ�� ��� ������ üũ
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

		// �ٸ� ��Ÿ�ְ� ��� ���� �ƴ� ���� ���� ��Ÿ�ָ� �ݺ� ���
		if (!bIsHitPlaying && !bIsDeathPlaying)
		{
			AnimInstance->Montage_Play(InAirStunMontage, 1.0f);
		}
	}
}

void AEnemyDog::ExitInAirStunState()
{
	if (bIsDead) return; // �׾��ٸ� ���� ���� ���� ���� �� ��
	UE_LOG(LogTemp, Warning, TEXT("Exiting InAirStunState..."));

	bIsInAirStun = false; // ���� ���� ����

	// ���� �ִϸ��̼� �ݺ� Ÿ�̸� ����
	GetWorld()->GetTimerManager().ClearTimer(StunAnimRepeatTimerHandle);

	// �߷��� �ٽ� �����ϰ� ���� ���·� ����
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	GetCharacterMovement()->GravityScale = 1.5f; // �⺻ �߷º��� �ణ ���ϰ� �����Ͽ� ������ ����
	ApplyBaseWalkSpeed(); // �⺻ �̵� �ӵ� ������

	// AI �̵� ��Ȱ��ȭ
	AEnemyDogAIController* AICon = Cast<AEnemyDogAIController>(GetController());
	if (AICon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reactivating AI movement..."));
		GetCharacterMovement()->SetMovementMode(MOVE_NavWalking); // �׺���̼� ���� ���� ��ȯ
		GetCharacterMovement()->SetDefaultMovementMode(); // �⺻ �̵� ��� ����
		AICon->MoveToActor(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)); // �ٽ� �÷��̾ ���� �̵� ����
	}

	// ��� ���� ��Ÿ�� ����
	UEnemyDogAnimInstance* AnimInstance = Cast<UEnemyDogAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(0.1f); // �ε巴�� ����
	}

	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s has recovered from stun and resumed AI behavior!"), *GetName());

	bIsInAirStun = false; // ���� ���� ���� ����
}

void AEnemyDog::ApplyGravityPull(FVector ExplosionCenter, float PullStrength)
{
	if (bIsDead) return; // ���� ���� �������� ����

	// ���� �߽��� �������� ���� �����ϴ� ����
	FVector Direction = ExplosionCenter - GetActorLocation(); // ���� ��ġ���� �߽��������� ���� ����
	float Distance = Direction.Size(); // �߽��������� �Ÿ�
	Direction.Normalize();  // ���� ���� ����ȭ

	// �Ÿ��� ���� ���� ���� (�������� ���ϰ�)
	float DistanceFactor = FMath::Clamp(1.0f - (Distance / 500.0f), 0.1f, 1.0f);
	float AdjustedPullStrength = PullStrength * DistanceFactor; // ���� ����� ��

	// ĳ���Ͱ� ���߿� �ִٸ� �� ���� ���� ����
	if (GetCharacterMovement()->IsFalling())
	{
		AdjustedPullStrength *= 1.5f;
	}

	// ���� ������ �ӵ� ����
	FVector NewVelocity = Direction * AdjustedPullStrength;
	GetCharacterMovement()->Velocity = NewVelocity;

	// ��� ���� ���� ��ȯ�Ͽ� �����Ӱ� �����̰� ��
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	// 0.5�� �Ŀ� �ٽ� �׺���̼� ���� ���� ����
	FTimerHandle ResetMovementHandle;
	GetWorld()->GetTimerManager().SetTimer(
		ResetMovementHandle,
		[this]()
		{
			GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
		},
		0.5f,
		false
	);
	UE_LOG(LogTemp, Warning, TEXT("EnemyDog %s pulled toward explosion center with strength %f"),
		*GetName(), AdjustedPullStrength);
}

void AEnemyDog::RaycastAttack()
{
	if (bHasExecutedRaycast) return; // �̹� ���ݿ��� �̹� ������ �������� �ߺ� ���� ����
	bHasExecutedRaycast = true; // ���� ��������� ǥ��

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // �÷��̾� �� ��������
	if (!PlayerPawn) return; // �÷��̾ ������ ����

	FVector StartLocation = GetActorLocation(); // ���� ������ (�ڽ� ��ġ)
	FVector PlayerLocation = PlayerPawn->GetActorLocation(); // �÷��̾� ��ġ
	FVector Direction = (PlayerLocation - StartLocation).GetSafeNormal(); // �ڽſ��� �÷��̾�� ���ϴ� ���� ����
	FVector EndLocation = StartLocation + (Direction * 150.0f); // 150 ���� �ձ��� ���� �߻�

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); // �ڱ� �ڽ��� ���� �浹���� ����

	// Pawn ä�ο� ���� ���� Ʈ���̽�(����ĳ��Ʈ) ����
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Pawn,
		CollisionParams
	);

	if (bHit && HitResult.GetActor() == PlayerPawn) // ���̰� �÷��̾�� �¾Ҵٸ�
	{
		// ����׿� �ð�ȭ (������ ���ΰ� ��ü)
		DrawDebugLine(GetWorld(), StartLocation, HitResult.Location, FColor::Red, false, 3.0f, 0, 5.0f);
		DrawDebugSphere(GetWorld(), HitResult.Location, 20.0f, 12, FColor::Red, false, 3.0f);

		// �÷��̾�� ����Ʈ ������ ����
		UGameplayStatics::ApplyPointDamage(
			PlayerPawn, Damage, StartLocation, HitResult, nullptr, this, nullptr
		);
	}
	else // ���̰� �������ٸ�
	{
		// ����׿� �ð�ȭ (�ʷϻ� ����)
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Green, false, 3.0f, 0, 5.0f);
	}
}

void AEnemyDog::StartAttack()
{
	bIsAttacking = true; // ���� �� ���·� ����
	bHasExecutedRaycast = false; // ����ĳ��Ʈ ���� ���� �ʱ�ȭ
	RaycastHitActors.Empty(); // ��Ʈ ���� ��� �ʱ�ȭ
	DamagedActors.Empty(); // ������ ���� ���� ��� �ʱ�ȭ
	bCanAttack = false; // ���� ��Ÿ�� ���� ���� �Ұ�
	RaycastAttack(); // ����ĳ��Ʈ ���� ����
}

void AEnemyDog::EndAttack()
{
	bIsAttacking = false; // ���� �� ���� ����
	bHasExecutedRaycast = false; // ����ĳ��Ʈ ���� ���� �ʱ�ȭ
	RaycastHitActors.Empty(); // ��Ʈ ���� ��� �ʱ�ȭ
	DamagedActors.Empty(); // ������ ���� ���� ��� �ʱ�ȭ
	bCanAttack = true; // �ٽ� ���� ���� ���·� ����
}