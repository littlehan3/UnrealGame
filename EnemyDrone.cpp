#include "EnemyDrone.h"
#include "Kismet/GameplayStatics.h" // 게임플레이 유틸리티 함수 사용
#include "GameFramework/CharacterMovementComponent.h" // 캐릭터 무브먼트 컴포넌트 제어
#include "NiagaraFunctionLibrary.h" // 나이아가라 이펙트 스폰 함수 사용
#include "GameFramework/ProjectileMovementComponent.h" // 미사일의 투사체 무브먼트 제어
#include "EnemyDroneAIController.h" // 이 드론이 사용할 AI 컨트롤러
#include "MainGameModeBase.h" // 게임모드에 적 사망을 알리기 위해 포함

AEnemyDrone::AEnemyDrone()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화
    GetCharacterMovement()->SetMovementMode(MOVE_Flying); // 이동 모드를 '비행'으로 설정
    GetCharacterMovement()->GravityScale = 0.0f; // 비행 캐릭터이므로 중력 비활성화
    AIControllerClass = AEnemyDroneAIController::StaticClass(); // 이 캐릭터가 사용할 AI 컨트롤러 클래스 지정
    FlightLoopAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FlightLoopAudio"));
    FlightLoopAudio->SetupAttachment(RootComponent);
    FlightLoopAudio->bAutoActivate = false; // BeginPlay에서 수동으로 시작
}

void AEnemyDrone::BeginPlay()
{
    Super::BeginPlay(); // 부모 클래스 BeginPlay 호출
    
    if (FlightLoopSound)
    {
        FlightLoopAudio->SetSound(FlightLoopSound);
        FlightLoopAudio->Play();
    }

    // 최대 체력으로 현재 체력 초기화
    Health = MaxHealth;

    if (!GetController()) // AI 컨트롤러가 할당되지 않았다면
    {
        UE_LOG(LogTemp, Warning, TEXT("No AI Controller found, spawning one"));
        AEnemyDroneAIController* NewController = GetWorld()->SpawnActor<AEnemyDroneAIController>(); // 새 컨트롤러 스폰
        if (NewController)
        {
            NewController->Possess(this); // 스폰된 컨트롤러가 이 드론에 빙의하도록 설정
        }
    }

    PlayerActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0); // 플레이어 캐릭터 찾아 저장

    // 오브젝트 풀링: 게임 시작 시 미사일 10개를 미리 생성하여 풀에 넣어둠
    for (int32 i = 0; i < 10; ++i)
    {
        if (MissileClass) // 미사일 클래스가 유효하다면
        {
            // 미사일을 월드에 스폰하지만, 바로 사용하지는 않음
            AEnemyDroneMissile* Missile = GetWorld()->SpawnActor<AEnemyDroneMissile>(MissileClass, FVector::ZeroVector, FRotator::ZeroRotator);
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
    MissileTimer += DeltaTime; // 매 틱마다 쿨타임 타이머를 증가시킴

    if (MissileTimer >= MissileCooldown) // 타이머가 쿨타임을 초과하면
    {
        ShootMissile(); // 미사일 발사
        MissileTimer = 0.0f; // 타이머 초기화
    }
}

void AEnemyDrone::ShootMissile()
{
    AEnemyDroneMissile* Missile = GetAvailableMissileFromPool(); // 풀에서 사용 가능한 미사일을 가져옴
    if (Missile && PlayerActor) // 미사일과 플레이어가 모두 유효하다면
    {
        FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 100.f; // 드론의 약간 앞에서 미사일 스폰
        FRotator SpawnRot = (PlayerActor->GetActorLocation() - SpawnLoc).Rotation(); // 스폰 시 플레이어를 바라보는 방향 설정
        Missile->ResetMissile(SpawnLoc, PlayerActor); // 가져온 미사일을 초기화하고 발사 준비
        Missile->SetActorRotation(SpawnRot); // 미사일 방향 설정
    }
}

AEnemyDroneMissile* AEnemyDrone::GetAvailableMissileFromPool()
{
    for (AEnemyDroneMissile* Missile : MissilePool) // 풀에 있는 모든 미사일을 순회
    {
        if (Missile && Missile->IsHidden()) // 미사일이 유효하고, 현재 숨겨져 있다면(사용 중이 아니라면)
        {
            return Missile; // 해당 미사일 반환
        }
    }
    return nullptr; // 사용 가능한 미사일이 없으면 null 반환
}

float AEnemyDrone::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f;

    float DamageApplied = FMath::Min(Health, DamageAmount);
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
    if (bIsDead) return; // 중복 사망 처리 방지
    bIsDead = true; // 사망 상태로 전환

    // 사망 이펙트 재생
    if (DeathEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathEffect, GetActorLocation(), GetActorRotation());
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
        if (Missile && !Missile->IsHidden()) // 풀에서 활성화된 미사일을 찾아
        {
            Missile->Explode(); // 즉시 폭발시킴
        }
    }

    HideEnemy(); // 액터 정리 함수 호출
}

void AEnemyDrone::HideEnemy()
{
    UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyDrone - Cleanup"));
    // 게임모드에 적이 파괴되었음을 알림
    if (AMainGameModeBase* GameMode = Cast<AMainGameModeBase>(GetWorld()->GetAuthGameMode()))
    {
        GameMode->OnEnemyDestroyed(this);
    }

    GetWorld()->GetTimerManager().ClearAllTimersForObject(this); // 모든 타이머 정리

    // AI 컨트롤러 정리
    AController* AICon = GetController();
    if (AICon && IsValid(AICon))
    {
        AICon->UnPossess(); // 빙의 해제
        AICon->Destroy(); // 컨트롤러 파괴
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
    GetWorld()->GetTimerManager().SetTimerForNextTick(
        [WeakThis = TWeakObjectPtr<AEnemyDrone>(this)]()
        {
            if (WeakThis.IsValid() && !WeakThis->IsActorBeingDestroyed())
            {
                WeakThis->Destroy();
                UE_LOG(LogTemp, Warning, TEXT("EnemyDrone Successfully Destroyed"));
            }
        }
    );
}