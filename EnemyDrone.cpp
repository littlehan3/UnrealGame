#include "EnemyDrone.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"

AEnemyDrone::AEnemyDrone()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    GetCharacterMovement()->GravityScale = 0.0f;
}

void AEnemyDrone::BeginPlay()
{
    Super::BeginPlay();
    PlayerActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

    // 미리 10개씩 생성하여 풀에 넣음
    for (int32 i = 0; i < 10; ++i)
    {
        // 미사일
        if (MissileClass)
        {
            AEnemyDroneMissile* Missile = GetWorld()->SpawnActor<AEnemyDroneMissile>(MissileClass, FVector::ZeroVector, FRotator::ZeroRotator);
            if (Missile)
            {
                Missile->SetActorHiddenInGame(true);
                Missile->SetActorEnableCollision(false);
                MissilePool.Add(Missile);
            }
        }
    }
}

void AEnemyDrone::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    MissileTimer += DeltaTime;

    if (MissileTimer >= MissileCooldown)
    {
        ShootMissile();
        MissileTimer = 0.0f;
    }
}

void AEnemyDrone::ShootMissile()
{
    AEnemyDroneMissile* Missile = GetAvailableMissileFromPool();
    if (Missile && PlayerActor)
    {
        FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 100.f;
        FRotator SpawnRot = (PlayerActor->GetActorLocation() - SpawnLoc).Rotation();
        Missile->ResetMissile(SpawnLoc, PlayerActor);
        Missile->SetActorRotation(SpawnRot);
    }
}

AEnemyDroneMissile* AEnemyDrone::GetAvailableMissileFromPool()
{
    for (AEnemyDroneMissile* Missile : MissilePool)
    {
        if (Missile && Missile->IsHidden())
        {
            return Missile;
        }
    }
    return nullptr;
}

float AEnemyDrone::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f;

    float DamageApplied = FMath::Min(Health, DamageAmount);
    Health -= DamageApplied;

    UE_LOG(LogTemp, Warning, TEXT("Drone took damage: %f, Health: %f"), DamageApplied, Health);

    if (Health <= 0.0f)
    {
        Die();
    }
    return DamageApplied;
}

void AEnemyDrone::Die()
{
    if (bIsDead) return;
    bIsDead = true;

    // 나이아가라 이펙트 재생
    if (DeathEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            DeathEffect,
            GetActorLocation(),
            GetActorRotation()
        );
    }

    // 사운드 재생 (선택)
    if (DeathSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
    }

    // 이미 발사되어 플레이어를 추적 중인 미사일 강제 폭발
    for (AEnemyDroneMissile* Missile : MissilePool)
    {
        if (Missile && !Missile->IsHidden()) // 현재 활성 상태
        {
            Missile->Explode();  // 체력 무시하고 바로 폭발하게 하려면 이 방법
        }
    }

    // 정리 함수 호출
    HideEnemy();
}

void AEnemyDrone::HideEnemy()
{
    UE_LOG(LogTemp, Warning, TEXT("Hiding EnemyDrone - Cleanup"));

    // 1. 타이머 제거
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

    // 2. AI 컨트롤러 해제
    AController* AICon = GetController();
    if (AICon && IsValid(AICon))
    {
        AICon->UnPossess();
        AICon->Destroy();
    }

    // 3. 무브먼트 중지 및 비활성화
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp && IsValid(MoveComp))
    {
        MoveComp->DisableMovement();
        MoveComp->StopMovementImmediately();
        MoveComp->SetComponentTickEnabled(false);
    }

    // 4. 메쉬 숨김 & 콜리전 해제
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetVisibility(false);
        MeshComp->SetHiddenInGame(true);
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
        MeshComp->SetComponentTickEnabled(false);
    }

    // 미사일 정리 (혹시 아직 있는 경우)
    for (AEnemyDroneMissile* Missile : MissilePool)
    {
        if (Missile && !Missile->IsHidden())
        {
            // 풀링이기에 Destroy 대신 히튼, 콜리전 해제로 재사용 가능하게 남김
            Missile->SetActorHiddenInGame(true); 
            Missile->SetActorEnableCollision(false);
            Missile->ProjectileMovement->StopMovementImmediately();
        }
    }

    // 5. 액터 자체 비활성화
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    SetCanBeDamaged(false);

    // 6. 다음 프레임에 안전하게 제거
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
