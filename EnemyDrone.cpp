#include "EnemyDrone.h"
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수 사용
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트 제어
#include "NiagaraFunctionLibrary.h" // 나이아가라 이펙트 스폰 함수 사용
#include "GameFramework/ProjectileMovementComponent.h" // 미사일의 투사체 무브먼트 제어
#include "EnemyDroneAIController.h" // 이 드론이 사용할 AI 컨트롤러
#include "Components/AudioComponent.h"
#include "MainGameModeBase.h" // 게임모드에 적 사망을 알리기 위해 포함
#include "EnemyDroneMissile.h"

AEnemyDrone::AEnemyDrone()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화
	UCharacterMovementComponent* MoveComp = GetCharacterMovement(); // 캐릭터 무브먼트 컴포넌트 참조
    MoveComp->SetMovementMode(MOVE_Flying); // 이동 모드를 '비행'으로 설정
    MoveComp->GravityScale = DroneGravityScale; // 비행 캐릭터이므로 중력 비활성화
    AIControllerClass = AEnemyDroneAIController::StaticClass(); // 이 캐릭터가 사용할 AI 컨트롤러 클래스 지정
    FlightLoopAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FlightLoopAudio"));
    FlightLoopAudio->SetupAttachment(RootComponent);
    FlightLoopAudio->bAutoActivate = false; // BeginPlay에서 수동으로 시작
}

void AEnemyDrone::BeginPlay()
{
    UWorld* World = GetWorld();
    if (!World) return;

    Super::BeginPlay(); // 부모 클래스 BeginPlay 호출
    
    if (FlightLoopSound)
    {
        FlightLoopAudio->SetSound(FlightLoopSound);
        FlightLoopAudio->Play();
    }

    // 최대 체력으로 현재 체력 초기화
    Health = MaxHealth;

    AICon = Cast<AEnemyDroneAIController>(GetController()); // AI 컨트롤러 가져옴
    if (!AICon) // AI 컨트롤러가 없다면
    {
        SpawnDefaultController(); // AI 컨트롤러 스폰
        AICon = Cast<AEnemyDroneAIController>(GetController());  // AI 컨트롤러 다시 가져옴
    }

    PlayerActor = UGameplayStatics::GetPlayerCharacter(World, 0); // 플레이어 캐릭터 찾아 저장

    // 오브젝트 풀링: 게임 시작 시 미사일을 미리 생성하여 풀에 넣어둠
	if (MissileClass) // 미사일 클래스가 유효하다면
    {
		for (int32 i = 0; i < InitialMissilePoolSize; i++) // 초기 생성 수 만큼 미사일 생성
        {
            // 미사일을 월드에 스폰하지만, 바로 사용하지는 않음
            AEnemyDroneMissile* Missile = World->SpawnActor<AEnemyDroneMissile>(MissileClass, FVector::ZeroVector, FRotator::ZeroRotator);
            if (Missile)
            {
                Missile->SetActorHiddenInGame(true); // 보이지 않게 숨김
                Missile->SetActorEnableCollision(false); // 충돌 비활성화
                MissilePool.Add(Missile); // 배열(풀)에 추가하여 보관
            }
        }
    }
}

void AEnemyDrone::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // 부모 클래스 Tick 호출
    if (bIsDead) return; // 사망했거나 인트로 중이면 로직 중단

    MissileFireInterval += DeltaTime; // 매 틱마다 발사간격 타이머를 증가시킴

    if (MissileFireInterval >= MissileCooldown) // 타이머가 쿨타임을 초과하면
    {
        ShootMissile(); // 미사일 발사
		MissileFireInterval = 0.0f; // 타이머 초기화
    }
}

void AEnemyDrone::ShootMissile()
{
	AEnemyDroneMissile* AvailableMissile = GetAvailableMissileFromPool(); // 풀에서 사용 가능한 미사일 가져오기

	if (!IsValid(AvailableMissile) || !IsValid(PlayerActor)) return; // 미사일이나 플레이어가 유효하지 않으면 중단

    // 한 번 계산한 값은 const 변수에 고정해서 재사용
	const FVector DroneLocation = GetActorLocation(); // 드론의 현재 위치
	const FVector DroneForward = GetActorForwardVector(); // 드론의 정면 방향 벡터
	const FVector TargetLocation = PlayerActor->GetActorLocation(); // 플레이어의 현재 위치

    // 발사 위치 및 방향 계산
	const FVector SpawnLocation = DroneLocation + (DroneForward * MissileSpawnForwardOffset); // 드론 앞쪽으로 오프셋만큼 이동한 위치
	const FRotator SpawnRotation = (TargetLocation - SpawnLocation).Rotation(); // 플레이어를 향하는 방향으로 회전값 계산

    // 미사일 세팅 및 발사
	AvailableMissile->SetActorRotation(SpawnRotation); // 미사일의 회전값 설정
	AvailableMissile->ResetMissile(SpawnLocation, PlayerActor); // 미사일 초기화 및 타겟 설정
}

AEnemyDroneMissile* AEnemyDrone::GetAvailableMissileFromPool()
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

    for (AEnemyDroneMissile* Missile : MissilePool) // 풀에 있는 모든 미사일을 순회
    {
        if (IsValid(Missile) && Missile->IsHidden()) // 미사일이 유효하고, 현재 숨겨져 있다면(사용 중이 아니라면)
        {
            return Missile; // 해당 미사일 반환
        }
    }

    // 풀링한 갯수보다 더 필요하다면 새로 하나 구워서 풀에 넣고 반환!
	if (MissileClass) // 미사일 클래스가 유효하다면
    {
		AEnemyDroneMissile* NewMissile = World->SpawnActor<AEnemyDroneMissile>(MissileClass); // 새 미사일 스폰
		if (NewMissile) // 스폰에 성공했다면
        {
			MissilePool.Add(NewMissile); // 풀에 추가
			return NewMissile; // 새로 생성한 미사일 반환
        }
    }

    return nullptr; // 사용 가능한 미사일이 없으면 null 반환
}

float AEnemyDrone::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f;

    const float DamageApplied = FMath::Min(Health, DamageAmount);
    Health -= DamageApplied;
    UE_LOG(LogTemp, Warning, TEXT("Drone took damage: %f, Health: %f"), DamageApplied, Health);

    if (Health <= 0.0f) // 체력이 0 이하라면
    {
        Die();
    }

    // 모든 로직이 끝난 후 Super::TakeDamage 호출
    Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    return DamageApplied;
}

float AEnemyDrone::GetHealthPercent_Implementation() const
{
    if (bIsDead || Health <= 0.0f)
    {
        return 0.0f;
    }
    if (MaxHealth <= 0.0f)
    {
        return 0.0f;
    }
    return FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);
}

bool AEnemyDrone::IsEnemyDead_Implementation() const
{
    return bIsDead;
}

void AEnemyDrone::Die()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (bIsDead) return; // 중복 사망 처리 방지
    bIsDead = true; // 사망 상태로 전환

    // 사망 이펙트 재생
    if (DeathEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, DeathEffect, GetActorLocation(), GetActorRotation());
    }

    // 사망 사운드 재생
    if (DeathSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
    }

    if (FlightLoopAudio)
    {
        FlightLoopAudio->Stop();
    }

    // 현재 활성화된 모든 미사일을 강제로 폭발시킴
    for (AEnemyDroneMissile* Missile : MissilePool)
    {
        if (IsValid(Missile) && !Missile->IsHidden()) // 풀에서 활성화된 미사일을 찾아
        {
            Missile->Explode(); // 즉시 폭발시킴
        }
    }

    HideEnemy(); // 액터 정리 함수 호출
}

void AEnemyDrone::HideEnemy()
{
    UWorld* World = GetWorld();
    if (!World) return;
    UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyDrone - Cleanup"));
    // 게임모드에 적이 파괴되었음을 알림
    if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(World->GetAuthGameMode()))
    {
        GameMode->OnEnemyDestroyed(this);
    }

    World->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 정리

    // AI 컨트롤러 정리
    AController* EnemyDroneAICon = GetController();
    if (EnemyDroneAICon && IsValid(EnemyDroneAICon))
    {
        EnemyDroneAICon->UnPossess(); // 빙의 해제
        EnemyDroneAICon->Destroy(); // 컨트롤러 파괴
    }

    // 무브먼트 컴포넌트 정리
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp && IsValid(MoveComp))
    {
        MoveComp->DisableMovement(); // 이동 비활성화
        MoveComp->StopMovementImmediately(); // 즉시 정지
        MoveComp->SetComponentTickEnabled(false); // 컴포넌트 틱 비활성화
    }

    if (FlightLoopAudio)
    {
        FlightLoopAudio->Stop();
        FlightLoopAudio->SetComponentTickEnabled(false);
    }

    // 메쉬 정리
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetVisibility(false); // 보이지 않게 설정
        MeshComp->SetHiddenInGame(true); // 게임 내에서 숨김 처리
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 비활성화
        MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // 모든 충돌 응답 무시
        MeshComp->SetComponentTickEnabled(false); // 컴포넌트 틱 비활성화
    }

    // 미사일 풀 정리 (파괴가 아닌 비활성화)
    for (AEnemyDroneMissile* Missile : MissilePool)
    {
        if (Missile && !Missile->IsHidden()) // 아직 활성화된 미사일이 남아있다면
        {
            // 오브젝트 풀링 방식이므로 Destroy 대신 숨김 처리하여 재사용 가능하게 남겨둠
            Missile->SetActorHiddenInGame(true);
            Missile->SetActorEnableCollision(false);
            Missile->ProjectileMovement->StopMovementImmediately();
        }
    }

    // 액터 자체 비활성화
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    SetCanBeDamaged(false);

    // 다음 프레임에 안전하게 액터 제거
	TWeakObjectPtr<AEnemyDrone> WeakThis(this);
    World->GetTimerManager().SetTimerForNextTick(
        [WeakThis]()
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
            {
                // 풀링된 미사일들도 함께 제거 (드론이 월드에서 완전히 사라질 때)
                for (AEnemyDroneMissile* Missile : WeakThis->MissilePool)
                {
                    if (IsValid(Missile))
                    {
                        Missile->Destroy();
                    }
                }
                WeakThis->Destroy();
                UE_LOG(LogTemp, Warning, TEXT("EnemyDrone Successfully Destroyed"));
            }
        }
    );
}