#include "EnemyDroneMissile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "EnemyDrone.h"
#include "Engine/OverlapResult.h"

AEnemyDroneMissile::AEnemyDroneMissile()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick 함수 호출 활성화

    // 충돌 컴포넌트 생성 및 설정
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->InitSphereRadius(14.f);
    CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic")); // 동적인 모든 것과 충돌
    CollisionComponent->OnComponentHit.AddDynamic(this, &AEnemyDroneMissile::OnHit); // OnHit 함수와 델리게이트 바인딩
    RootComponent = CollisionComponent; // 루트 컴포넌트로 설정

    // 메쉬 컴포넌트 생성 및 설정
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(CollisionComponent); // 충돌 컴포넌트에 부착
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 메쉬 자체의 충돌은 사용 안함

    // 투사체 이동 컴포넌트 생성 및 설정
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 600.f; // 초기 속도
    ProjectileMovement->MaxSpeed = 600.f; // 최대 속도
    ProjectileMovement->bRotationFollowsVelocity = true; // 이동 방향에 따라 자동으로 회전
    ProjectileMovement->ProjectileGravityScale = 0.0f; // 중력 영향 없음

    SetActorTickInterval(0.05f); // Tick 주기 0.05초로 최적화
}

void AEnemyDroneMissile::SetTarget(AActor* Target)
{
    TargetActor = Target; // 타겟 설정
}

void AEnemyDroneMissile::BeginPlay()
{
    Super::BeginPlay();
}

// 오브젝트 풀링을 위한 재활용 함수
void AEnemyDroneMissile::ResetMissile(FVector SpawnLocation, AActor* NewTarget)
{
    ProjectileMovement->StopMovementImmediately(); // 기존 움직임 즉시 중지
    ProjectileMovement->SetUpdatedComponent(CollisionComponent); // 이동 기준 컴포넌트 재설정
    ProjectileMovement->Activate(true); // 이동 컴포넌트 재활성화

    // 위치 및 상태 초기화
    SetActorLocation(SpawnLocation); // 새로운 위치로 이동
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 충돌 재활성화
    SetActorEnableCollision(true); // 액터 충돌 재활성화
    SetActorHiddenInGame(false); // 다시 보이게 설정
    MeshComp->SetHiddenInGame(false);

    bExploded = false; // 폭발 상태 초기화
    TargetActor = NewTarget; // 새로운 타겟 설정
    LastMoveDirection = FVector::ZeroVector; // 마지막 이동 방향 초기화

    // 풀링 시 발생할 수 있는 충돌 문제를 방지하기 위해 무시 액터 목록을 초기화하고 다시 설정
    CollisionComponent->MoveIgnoreActors.Empty();
    // 모든 아군 드론을 충돌에서 무시하도록 설정
    TArray<AActor*> AllDrones;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDrone::StaticClass(), AllDrones);
    for (AActor* Drone : AllDrones)
        CollisionComponent->IgnoreActorWhenMoving(Drone, true);
    // 자기 자신을 제외한 다른 모든 미사일을 충돌에서 무시하도록 설정
    TArray<AActor*> AllMissiles;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyDroneMissile::StaticClass(), AllMissiles);
    for (AActor* Missile : AllMissiles)
    {
        if (Missile != this)
            CollisionComponent->IgnoreActorWhenMoving(Missile, true);
    }

    // 초기 발사 방향 계산
    FVector Dir = FVector::ZeroVector;
    if (IsValid(TargetActor)) // 타겟이 유효하다면
    {
        Dir = (TargetActor->GetActorLocation() - SpawnLocation).GetSafeNormal(); // 타겟을 향하는 방향 벡터
    }
    SetActorRotation(Dir.Rotation()); // 방향에 맞게 회전
    ProjectileMovement->Velocity = Dir * ProjectileMovement->InitialSpeed; // 해당 방향으로 속도 설정
    LastMoveDirection = Dir; // 마지막 이동 방향 저장
}

void AEnemyDroneMissile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bExploded) return; // 폭발했다면 업데이트 중지

    // 타겟 추적 (유도 기능)
    FVector Dir = FVector::ZeroVector;
    if (IsValid(TargetActor)) // 타겟이 유효하다면
    {
        Dir = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal(); // 현재 위치에서 타겟을 향하는 방향을 다시 계산
    }

    if (!Dir.IsNearlyZero()) // 방향이 유효하다면
    {
        FRotator NewRot = Dir.Rotation(); // 새로운 방향으로의 회전값
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), NewRot, DeltaTime, 5.f)); // 부드럽게 방향 전환
        ProjectileMovement->Velocity = Dir * ProjectileMovement->InitialSpeed; // 새로운 방향으로 속도 업데이트
        LastMoveDirection = Dir; // 마지막 이동 방향 갱신
    }
    else // 타겟을 놓쳤다면
    {
        if (LastMoveDirection.IsNearlyZero())
            LastMoveDirection = GetActorForwardVector(); // 마지막 이동 방향으로 직진
        ProjectileMovement->Velocity = LastMoveDirection * ProjectileMovement->InitialSpeed;
    }
}

void AEnemyDroneMissile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (bExploded) return; // 이미 폭발했다면 중복 실행 방지
    if (!OtherActor || OtherActor == this) return; // 부딪힌 대상이 없거나 자기 자신이면 무시

    Explode(); // 무언가에 부딪히면 폭발
}

void AEnemyDroneMissile::Explode()
{
    if (bExploded) return; // 중복 폭발 방지
    bExploded = true; // 폭발 상태로 전환

    // 폭발 이펙트 생성
    if (ExplosionEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), ExplosionEffect, GetActorLocation(), FRotator::ZeroRotator);
    }

    // 폭발 범위 내에서 플레이어를 찾아 데미지 적용 (범위 공격)
    FVector ExplosionCenter = GetActorLocation();
    float ExplosionRadiusTemp = ExplosionRadius;
    TArray<FOverlapResult> Overlaps;
    FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ExplosionRadiusTemp);
    // Pawn 채널에 대해서만 오버랩(범위) 검사 수행
    bool bHasOverlaps = GetWorld()->OverlapMultiByChannel(Overlaps, ExplosionCenter, FQuat::Identity, ECC_Pawn, CollisionShape);
    if (bHasOverlaps) // 범위 내에 Pawn이 있다면
    {
        ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
        for (auto& Overlap : Overlaps) // 모든 오버랩된 액터에 대해
        {
            if (Overlap.GetActor() == PlayerCharacter) // 만약 플레이어라면
            {
                UGameplayStatics::ApplyDamage(PlayerCharacter, Damage, GetInstigator() ? GetInstigator()->GetController() : nullptr, this, nullptr);
                break; // 플레이어는 한 명이므로 찾으면 바로 중단
            }
        }
    }

    // 폭발 후 미사일을 풀에 반환하기 위해 비활성화
    MeshComp->SetHiddenInGame(true);
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    ProjectileMovement->StopMovementImmediately();
}

float AEnemyDroneMissile::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    Health -= DamageAmount; // 체력 감소
    if (Health <= 0.f && !bExploded) // 체력이 0 이하가 되고 아직 폭발하지 않았다면
        Explode(); // 폭발 (요격 성공)
    return DamageAmount;
}