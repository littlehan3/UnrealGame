#include "BossEnemy.h"
#include "BossEnemyAnimInstance.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "BossEnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"

ABossEnemy::ABossEnemy()
{
	PrimaryActorTick.bCanEverTick = true; // tick Ȱ��ȭ
	bCanBossAttack = true; // ���� ���� ���� true
	AIControllerClass = ABossEnemyAIController::StaticClass(); // AI ��Ʈ�ѷ� ����
	GetMesh()->SetAnimInstanceClass(UBossEnemyAnimInstance::StaticClass()); // �ִ� �ν��Ͻ� ����
	if (!AIControllerClass) // ��Ʈ�ѷ��� ���ٸ�
	{
		UE_LOG(LogTemp, Error, TEXT("Boss AI Controller NULL"));
	}
	//GetCharacterMovement()->MaxWalkSpeed = 300.0f; // �⺻ �̵��ӵ� ����
}

// Called when the game starts or when spawned
void ABossEnemy::BeginPlay()
{
	Super::BeginPlay();
	SetCanBeDamaged(true); // ���ظ� ���� �� �ִ��� ���� true
	AAIController* AICon = Cast<AAIController>(GetController()); // AI ��Ʈ�ѷ��� �ҷ��� ĳ��Ʈ
	if (AICon) // ��Ʈ�ѷ��� �����Ѵٸ�
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss AI Controller Assigend")); // ��Ʈ�ѷ� �Ҵ��
	}
	else // �ƴѰ��
	{
		UE_LOG(LogTemp, Error, TEXT("Boss AI Controller NULL")); // ��Ʈ�ѷ� NULL
	}
	SetUpBossAI(); // AI �����Լ� ȣ��
	BossEnemyAnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance());
}

void ABossEnemy::SetUpBossAI()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_NavWalking); // ĳ���� �����Ʈ�� ������ �����忡 �ִ� �׺� ��ŷ���� ����
}

void ABossEnemy::PostInitializeComponents()
{
	Super::PostInitializeComponents(); // ������Ʈ �ʱ�ȭ ���� �߰� �۾��� ���� �θ��Լ� ȣ��

	GetWorld()->GetTimerManager().SetTimerForNextTick([this]() // ���� Tick���� AI ��Ʈ�ѷ� �Ҵ� ���θ� Ȯ���ϴ� ���� ���
		{
			AAIController* AICon = Cast<AAIController>(GetController()); // �ѹ��� AI��Ʈ�ѷ��� ĳ�����ؼ� �޾ƿ�
			if (AICon) // ��Ʈ�ѷ��� �����Ѵٸ�
			{
				UE_LOG(LogTemp, Warning, TEXT("Boss AICon Assigned Later")); // ��Ʈ�ѷ� �Ҵ��
			}
			else // �ƴѰ��
			{
				UE_LOG(LogTemp, Error, TEXT("Boss AICon Still NULL")); // ��Ʈ�ѷ� NULL
			}
		});
}

void ABossEnemy::PlayBossNormalAttackAnimation()
{
	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance()); // �ֽ� �ִ��ν��Ͻ��� ĳ�����ؼ� �޾ƿ�
	if (AnimInstance || BossNormalAttackMontages.Num() == 0) return; // �ִ��ν��Ͻ��� ���ų� �Ϲݰ��� ��Ÿ�ְ� ���ٸ� ���� 
	if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return; // �ִ��ν��Ͻ��� �ְ� �ִ��ν��Ͻ��� ��Ÿ�ְ� �������̶�� ����

	int32 RandomIndex = FMath::RandRange(0, BossNormalAttackMontages.Num() - 1); // �Ϲݰ��� ��Ÿ�ֹ迭�߿� �������� �����ϴ� �����ε��� ����
	UAnimMontage* SelectedMontage = BossNormalAttackMontages[RandomIndex]; // �������ؽ����� ������ ��Ÿ�ָ� ������
	if (SelectedMontage) // ��Ÿ�ְ� ���� �Ǿ��ٸ�
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectedMontage: %s is playing"), *SelectedMontage->GetName());
		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // �÷��̰���� �ִ��ν����� ��Ÿ�� �÷��̸� 1.0������� ���
		if (PlayResult == 0.0f) // �÷��� ����� 0�̶��
		{
			UE_LOG(LogTemp, Warning, TEXT("Montage Play Failed")); // ��Ÿ�� ��� ����
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Montage Succesfully Playing")); // ��Ÿ�� �����
		}

		ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController()); // ��Ʈ�ѷ��� ĳ��Ʈ�ؼ� ������
		if (AICon) // ��Ʈ�ѷ��� ������ ���
		{
			AICon->StopMovement(); // ��Ʈ�ѷ��� �̵����� �Լ��� ȣ��
			UE_LOG(LogTemp, Warning, TEXT("Enemy Stopped Moving to Attack"));
		}

		bCanBossAttack = false; // ���� ���̹Ƿ� ���� �Ұ� false
	}
	if (BossNormalAttackSound) // ���尡 �ִٸ�
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossNormalAttackSound, GetActorLocation()); // �ش� ������ ��ġ���� �Ҹ� ���
	}
}

//void ABossEnemy::PlayBossStrongAttackAnimation()
//{
//	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance()); // �ֽ� �ִ��ν��Ͻ��� ĳ�����ؼ� �޾ƿ�
//	if (AnimInstance || BossStrongAttackMontages.Num() == 0) return; // �ִ��ν��Ͻ��� ���ų� �Ϲݰ��� ��Ÿ�ְ� ���ٸ� ���� 
//	bIsBossStrongAttacking = true; // ������ ���̹Ƿ� true
//	if (AnimInstance && AnimInstance->IsAnyMontagePlaying()) return; // �ִ��ν��Ͻ��� �ְ� �ִ��ν��Ͻ��� ��Ÿ�ְ� �������̶�� ����
//
//	int32 RandomIndex = FMath::RandRange(0, BossStrongAttackMontages.Num() - 1); // �Ϲݰ��� ��Ÿ�ֹ迭�߿� �������� �����ϴ� �����ε��� ����
//	UAnimMontage* SelectedMontage = BossStrongAttackMontages[RandomIndex]; // �������ؽ����� ������ ��Ÿ�ָ� ������
//	if (SelectedMontage) // ��Ÿ�ְ� ���� �Ǿ��ٸ�
//	{
//		UE_LOG(LogTemp, Warning, TEXT("SelectedMontage: %s is playing"), *SelectedMontage->GetName());
//		float PlayResult = AnimInstance->Montage_Play(SelectedMontage, 1.0f); // �÷��̰���� �ִ��ν����� ��Ÿ�� �÷��̸� 1.0������� ���
//		if (PlayResult == 0.0f) // �÷��� ����� 0�̶��
//		{
//			UE_LOG(LogTemp, Warning, TEXT("Montage Play Failed")); // ��Ÿ�� ��� ����
//		}
//		else
//		{
//			UE_LOG(LogTemp, Warning, TEXT("Montage Succesfully Playing")); // ��Ÿ�� �����
//		}
//
//		ABossEnemyAIController* AICon = Cast<ABossEnemyAIController>(GetController()); // ��Ʈ�ѷ��� ĳ��Ʈ�ؼ� ������
//		if (AICon) // ��Ʈ�ѷ��� ������ ���
//		{
//			AICon->StopMovement(); // ��Ʈ�ѷ��� �̵����� �Լ��� ȣ��
//			UE_LOG(LogTemp, Warning, TEXT("Enemy Stopped Moving to Attack"));
//		}
//	}
//	if (BossStrongAttackSound) // ���尡 �ִٸ�
//	{
//		UGameplayStatics::PlaySoundAtLocation(this, BossStrongAttackSound, GetActorLocation()); // �ش� ������ ��ġ���� �Ҹ����
//	}
//}

float ABossEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsBossDead) return 0.0f; // �̹� ����ߴٸ� ������ ����
	float DamageApplied = FMath::Min(BossHealth, DamageAmount); // ������ ü�°� �������� �ҷ���
	BossHealth -= DamageApplied; // ü�¿��� ��������ŭ ����
	UE_LOG(LogTemp, Warning, TEXT("Boss took %f damage, Health remaining: %f"), DamageAmount, BossHealth);

	if (BossHitSound) // ���尡 �ִٸ�
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossHitSound, GetActorLocation()); // �ش� ������ ��ġ���� �Ҹ����
	}

	if (BossHitReactionMontages.Num() > 0) // ��Ʈ�� �ִϸ��̼� ��� (�������߿� ��Ʈ �������)
	{
		int32 RandomIndex = FMath::RandRange(0, BossHitReactionMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossHitReactionMontages[RandomIndex];
		if (SelectedMontage)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				AnimInstance->Montage_Play(SelectedMontage, 1.0f);
			}
		}
	}

	if (BossHealth <= 0.0f) // ü���� 0������ ���
	{
		BossDie(); // ����Լ� ȣ��
	}
	
	return DamageApplied; // ���� ���� �ʱ�ȭ
}

void ABossEnemy::BossDie()
{
	if (bIsBossDead) return; // �̹� ����� ��� ����
	bIsBossDead = true; // ������� Ʈ��
	StopBossActions(); // �ൿ���� �Լ��� ȣ��
	
	if (BossDeadMontages.Num() > 0) // ��� �ִϸ��̼� ���
	{
		int32 RandomIndex = FMath::RandRange(0, BossDeadMontages.Num() - 1);
		UAnimMontage* SelectedMontage = BossDeadMontages[RandomIndex];
		if (SelectedMontage)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				AnimInstance->Montage_Play(SelectedMontage, 1.0f);
			}

			// ��Ÿ�� ���̸�ŭ Ÿ�̸� ����
			float MontageLength = SelectedMontage->GetPlayLength();
			GetWorld()->GetTimerManager().SetTimer(
				BossDeathHideTimerHandle, 
				this,
				&ABossEnemy::HideBossEnemy,
				MontageLength * 0.8f, // 80�ۼ�Ʈ���� ��� ��
				false
			);
			return;
		}
	}
	// ��Ÿ�ְ� ���ų� ��� ���н�
	HideBossEnemy(); // ������ ����� �Լ� ȣ��
}

void ABossEnemy::StopBossActions()
{
	GetCharacterMovement()->DisableMovement(); // ��� �̵� ����
	GetCharacterMovement()->StopMovementImmediately(); // ��� �̵� ��� ����

	UBossEnemyAnimInstance* AnimInstance = Cast<UBossEnemyAnimInstance>(GetMesh()->GetAnimInstance()); // �ֽ� �ִ��ν��Ͻ��� ĳ�����ؼ� �޾ƿ�
	if (AnimInstance) // �ִ� �ν��Ͻ���
	{
		AnimInstance->Montage_Stop(0.1f); // ��� ��Ÿ�� ����
	}
}

void ABossEnemy::HideBossEnemy()
{
	if (!bIsBossDead) return;
	SetActorHiddenInGame(true); // ������ ����
	SetActorEnableCollision(false); // �浹 ����
	SetActorTickEnabled(false); // AI ��ü Tick ��Ȱ��ȭ
}
