# LOCOMOTION (TPS Action Survival Game)

## 2025년 공주대학교 정보통신공학과 졸업 작품 전시회 시연작

- 본 프로젝트는 C++ 와 언리얼엔진 5.4 버전을 이용하여 제작된 실시간 액션 TPS 게임입니다.
- 졸업 전시회에서 성공적으로 시연 및 발표를 마쳤습니다.
- 졸업작품 전시 판넬: https://drive.google.com/file/d/1juNzVw-6LlI4VPA7kYJvu7l4knBZvY0F/view?usp=sharing

## 문재해결 및 개선 기록 노트

- 단순한 기능 구현을 넘어, 해당 프로젝트 개발 과정에서 마주한 36가지의 기술적 난관과 이를 데이터 기반으로 해결한 최적화 과정을 기록한 기술 백서입니다. 
- 성능 병목 지점 분석, 엔진 내부 구조 이해를 통한 트러블슈팅 등 엔지니어링적 고민의 흔적을 담았습니다.
- https://drive.google.com/file/d/1RFKRTb3lq0Ar7vT-4p2fAWsxuOvCUs_L/view?usp=sharing

## 2025-02-13 업데이트 내용

### 1. 레이캐스트 발사 위치 변경
- 기존: 총구(Muzzle)에서 발사  
- 변경: 카메라 중앙(ViewPoint)에서 발사되도록 수정
- 플레이어 조준 방향과 총알 궤적이 일치하도록 개선

### 2. 레이캐스트 적중 문제 해결
- 적(Enemy)이 레이캐스트에 맞지 않는 문제 해결
- `CapsuleComponent`와 `Mesh`의 Collision 설정 수정
 - `Visibility` 채널에서 `Block` 설정하여 레이캐스트가 적을 감지할 수 있도록 변경  

### 3. Enemy 시스템 구현
- `AEnemy` 클래스 추가  
- `TakeDamage()`를 통해 적이 총을 맞으면 체력이 차감됨  
- 체력이 0 이하가 되면 `Die()` 함수 실행하여 사망 처리 (Destroy 없이 사망 상태 유지)  

### 4. Rifle 사격 및 데미지 적용 구현
- `ARifle::Fire()`에서 적이 맞으면 데미지 적용  
- `ProcessHit()`에서 적중 대상이 Enemy인지 판별 후 데미지 적용
- 사망 시 사망 상태(`bIsDead`)를 설정하여 후속 데미지 적용 방지

## 2025-02-15 업데이트 내용

### 1. 콤보 공격 히트 판정
-Knife(좌/우)와 KickHitBox를 활용하여 적절한 콜리전 설정
-히트박스는 공격 타이밍에 맞춰 EnableHitBox(), DisableHitBox()로 제어

### 2. 콤보별 데미지 적용
-콤보별로 서로 다른 데미지 적용
1콤보: 20
2콤보: 25
3콤보: 30
4콤보(발차기): 35 (칼 데미지 적용 X)

### 3. 발차기 공격(4콤보) 소켓 적용
-변경: 발차기 전용 히트박스(KickHitBox)를 생성하고, 발에 부착된 FootSocket_R을 활용하여 처리
-KickHitBox는 EnableKickHitBox(), DisableKickHitBox()를 통해 활성화/비활성화

### 4. 히트 판정 최적화
-ComboAttack()에서 4콤보(발차기) 시 칼 히트박스 활성화 제외
-AKnife::EnableHitBox()에서 4콤보는 예외 처리하여 히트박스 활성화 방지
-KickHitBox는 OnKickHitBoxOverlap()을 통해 데미지 적용

## 2025-02-19 업데이트 내용

### 1. 락온 시스템 구현
- 근접공격이 가능한 상태일때만 카메라가 자동으로 적을 바라보도록함.
- UpdateLockOnRotation()을 통해 플레이어의 회전을 타겟 방향으로 보간하여 유지
- 락온 후 일정 거리 이상 멀어지면 자동 해제 TickComponent()에서 LockOnRadius * 1.2f 범위를 벗어나면 락온 해제

### 2. EnemyAI 기본 이동 및 플레이어 추적 구현
- AEnemyAIController 추가
- DetectionRadius 내에서 플레이어 감지 시 추적
- StopChasingRadius를 벗어나면 추적 중단
- MoveToActor(PlayerPawn, 5.0f)를 활용하여 네비게이션 시스템을 통한 이동 구현
- AI 이동 최적화
Tick()에서 불필요한 연산을 줄이고, 거리 조건을 만족할 때만 MoveToActor() 호출
NavMesh가 존재하지 않는 경우 경로 탐색을 중단하도록 설정
StopMovement()를 통해 불필요한 이동을 최소화

## 2025-02-23 업데이트 내용

### 1. EnemyAI 일반 공격 구현
- 플레이어가 공격 범위 내에 있을 때, NormalAttack 함수를 호출하여 일반 공격 애니메이션 실행
- 공격 시 공격 쿨타임을 적용하고, 일반 공격 횟수를 증가시킴

### 2. EnemyAI 강 공격 구현
-일반 공격을 3회 수행한 후 StrongAttack 함수를 통해 강 공격 실행
-강 공격 시 강 공격 애니메이션을 재생하고, 일반 공격 횟수를 초기화하며 쿨타임을 적용

### 3. EnemyAI 닷지 로직 구현
-일정 확률(DodgeChance)에 따라 좌/우 닷지를 선택하여 TryDodge 함수를 호출
-닷지 실행 후 지정된 닷지 쿨다운을 통해 연속 닷지를 방지

### 4. EnemyAI 점프 공격 구현
- 플레이어가 감지 범위에 진입하면 JumpAttack 함수를 통해 점프 공격 실행
- 점프 공격 애니메이션 실행 및 루트 모션 기반 이동 중지 처리
- 공격 후 쿨타임을 적용하고, 감지 범위를 벗어나면 점프 공격 재사용 가능하도록 상태를 리셋

## 2025-02-26 업데이트 내용

### 1. EnemyAI 오류 수정
- 닷지 문제: 공격(일반/강공격) 애니메이션이 재생 중임에도 불구하고, AttackCooldown 타이머 만료로 인해 bCanAttack이 true로 전환되어 닷지가 실행되는 현상
- 일반 공격 카운트 문제: 전투 초반에는 일반 공격 3회 후 강공격이 정상적으로 발동되었으나, 이후 전투에서는 이전 전투의 일반 공격 카운트가 누적되어 1~2회 공격 후에도 강공격이 발동되는 현상

- 공격이 진행 중임을 명확히 나타내기 위해 bIsAttacking 플래그를 도입
- 닷지 로직 조건 수정: 공격 애니메이션이 진행 중(bIsAttacking true)일 때 닷지 실행을 방지
- Tick() 함수 전투 종료 조건에 감지범위 밖으로 대상이 나갔을 때 공격 카운트를 초기화

### 2. EnemyAI 히트 및 사망에 따른 AI 중지기능
- 총기(레이케스트) 및 칼(폰 히트박스 오버랩) 공격 시 히트 애니메이션을 호출하며 사망시에도 애니메이션을 호출
- 사망 시엔 사망 애니메이션을 끝으로 AI가 어떤 행동과 움직임, 회전을 하지 않음

### 3. LockOnComponent 개선
- 사망한 EnemyAI는 더이상 락온이 되지 않음

## 2025-03-01 업데이트 내용

### 1. Knife 히트 오류 수정
- 뒤에 있는 적도 히트되는 문제
- 공격을 하지 않아도 히트박스에 충돌하면 데미지가 적용되던 문제

- 근접공격 즉시 캐릭터 전방에 래이캐스트를 발사
- 래이케스트에 히트되고 히트박스에도 충돌한다면 데미지를 적용하게 수정. 둘 중 하나만 충족한다면 히트되지 않음.
- 발차기 공격 또한 동일하게 적용

## 2025-03-08 업데이트 내용

### 1. 대쉬 시스템 구현
- WASD 방향 입력을 기반으로 대쉬하도록 변경
- 카메라 방향과 관계없이 직관적인 방향으로 대쉬 가능
- 대쉬 중 근접 공격 불가, 사격 가능
- 각 방향에 맞는 대쉬 애니메이션 적용

### 2. 전방, 후방, 좌측, 우측, 대각선 방향 대쉬 애니메이션 재생
- DotProduct를 활용하여 입력 방향을 정확히 판별하여 적절한 애니메이션 적용

### 3. 대쉬 관련 오류 수정 및 최적화 (4월1일에 업데이트 될 문제해결 노트 참고)
- 대쉬 방향이 의도와 다르게 실행되는 문제 해결
- SetActorRotation()을 올바른 타이밍에 적용하여 캐릭터 회전 오류 수정
- 애니메이션 실행 속도 조정 (1.2배속)

### 4. 대쉬 시스템 최종 코드 정리 및 개선
- Dash() 함수의  불필요한 연산 제거
- PlayDashMontage()에서 애니메이션 실행 여부를 철저히 체크하여 예외 처리 추가
- ResetDash()와 ResetDashCooldown()을 활용하여 대쉬 종료 후 상태를 명확히 관리

### 5. 에임모드 시 애니메이션 방향 개선
- 에임 모드 시 캐릭터 회전 방식 개선
- 기존: bUseControllerRotationYaw = true;로 인해 조준 시 캐릭터가 즉각적으로 회전
- 변경: Tick()에서 카메라 방향과 이동 방향을 동적으로 조절하도록 개선

### 6. 대쉬, 점프 시 애니메이션 방향 수정
- 대쉬 또는 점프할 때 카메라 방향을 기준으로 애니메이션 방향이 틀어지는 문제 수정
- 대쉬 중 캐릭터의 LastInputVector를 유지하여 올바른 방향을 유지하도록 처리
- 점프 중 회전이 불필요하게 변경되지 않도록 bOrientRotationToMovement를 일시적으로 비활성화

### 7. 에임모드 중 이동 방향이 자연스럽게 반영되도록 개선
- 기존: 카메라 방향과 이동 방향이 어긋나는 경우 애니메이션 방향이 부자연스러움
- 개선: bIsAiming 상태에서 FRotator ControlRotation을 기반으로 이동 방향을 조절
- AimPitch 값을 활용하여 상하 조준 시에도 부드러운 애니메이션 적용

## [긴급 패치] 에임 모드 회전 문제 수정 + 추가 개선 (2025-03-08)

### 1. 에임 모드 회전 문제 (긴급 패치)
- 이동하지 않은 상태에서 에임 모드로 전환 시, 카메라만 회전하고 캐릭터는 회전하지 않는 문제

- 대쉬중 에임모드로 전환했을때 원하는 방향으로 대쉬하게 수정하는 과정중 이동하지 않은 상태에서 캐릭터의 회전을 관리하는 부분이 누락됨
- Tick() 함수에서 이동입력이 없을때도 카메라 방향을 따라 캐릭터가 회전하도록 추가하여 해결

### 2. 콤보 리셋 기능 추가
- 1.5초 동안 콤보 공격을 이어서 하지 않으면 1타로 초기화

### 3. 에임 모드가 아닌 상태에서 줌인/줌아웃 기능 추가
- 기본 모드에서 마우스 휠을 사용해 줌인/줌아웃 가능
- 줌 변경 시 FMath::FInterpTo(보간)를 이용하여 부드럽게 적용
- 에임 모드에서는 줌 조정이 불가능하도록 설정

### 4. 에임 모드 전환 시 부드러운 화면 전환 적용
- 기존: 에임 모드 전환 시 카메라 위치가 즉시 변경됨
- 변경: 카메라 전환 시 보간을 적용하여 부드럽게 화면 전환

### 5. 대쉬 중 점프 불가하도록 수정
- 대쉬 중 점프를 사용할 수 없도록 설정

### 6. 점프 중 대쉬 불가하도록 수정
- 점프 상태에서 대쉬를 사용할 수 없도록 설정

### 7. 점프 중 콤보 공격 불가능하도록 하는 조건 최적화
- 기존: bCanPerformAction 변수를 사용하여 제어
- 변경: 불필요한 변수를 사용하지 않고 콤보 공격 함수 내에서 초기화 조건을 추가하는 방식으로 최적화

### 8. 에임 모드 중 대쉬 후 방향 유지 문제 수정
- 에임 모드 상태에서 대쉬 후, 대쉬 방향을 바라보는 문제 

- ResetDash() 함수에서 bIsAiming 상태를 체크하여 캐릭터 방향을 조정
- 에임 모드 유지 시 대쉬 후에도 원래 조준 방향을 바라보도록 수정
- SetActorRotation()을 사용하여 대쉬 종료 후 조준 방향으로 복귀하도록 보정

## 3월 15일 업데이트 내용

### 1. 락온 기능 제거
- 적이 여럿 몰려오는 게임의 특성상 락온 기능이 적절하지 않다고 판단하여 제거
- 락온 모드 시 회전 방향의 기준이 바뀌어 대쉬나 점프 시 의도되지 않은 방향으로 이동하는 문제가 지속적으로 발생하였음

### 2. 근접 공격 보정 기능 추가
- 락온이 제거됨에 따라 근접 공격 시 목표 대상과의 공격 방향이 어긋나는 문제를 해결하기 위해 보정 기능을 추가
- 공격 시 자동으로 가장 가까운 적을 감지하여 방향을 조정하는 기능을 구현
- AdjustComboAttackDirection() 함수에서 FVector::Dist()를 활용하여 가장 가까운 적을 찾아 FRotationMatrix::MakeFromX()를 사용해 공격 방향을 조정

### 3. 근접 모드 스킬 추가 (스킬1)
- 주변의 모든 적을 일정 시간 동안 공중에 띄우며, 해당 적들은 일정 시간 동안 행동 불가 상태가 되며 스턴 애니메이션을 재생
- 띄워진 적들은 사격이나 추가로 개발될 스킬과 연계하여 사용할 수 있도록 설계됨
- AMainCharacter::Skill1()을 통해 실행되며, EnterInAirStunState()를 호출하여 적을 띄우고 일정 시간 후 착지하게 함
- Skill1CooldownTimerHandle을 통해 쿨다운을 적용하고, bIsUsingSkill1과 bCanUseSkill1 변수를 활용하여 연속 사용을 방지

### 4. 상황에 따라 다른 적 사망 애니메이션 적용
- 기존에는 적이 사망 시 단일 애니메이션을 재생하였으나 사망 상황에 따라 다른 애니메이션을 적용하도록 개선예정
- 공중에서 사망한 경우 InAirStunDeathMontage를 재생하여 일반적인 사망과 다른 애니메이션 재생

### 5. 적 사망 시 일정 시간 후 사라지는 기능 추가
- 사망 애니메이션이 끝난 후 일정 시간이 지나면 적이 사라지도록 설정하여 씬을 깔끔하게 정리
- AEnemy::Die()에서 GetWorld()->GetTimerManager().SetTimer()를 사용하여 특정 시간 후 HideEnemy()를 호출
- 적이 사망하면 무기도 함께 사라지도록 AEnemyKatana::HideKatana() 함수를 호출하여 일정 시간 후 가비지 컬렉션을 유도

## 2025-03-25 업데이트 내용

### 1. 근접 모드 스킬 추가 (스킬2)
- 캐릭터가 도약 후 광역 데미지를 가하는 스킬
- 스킬 시전 후 일정 시간 뒤 범위 내에 있는 적들에게 피해를 줌
- SphereTrace를 사용하여 피해 범위 내 적 감지 및 데미지 적용
- 스킬1(공중 스턴)과의 연계를 고려하여 설계
- Skill2CooldownTimerHandle을 통해 쿨다운을 적용하고, bIsUsingSkill2과 bCanUseSkill2 변수를 활용하여 연속 사용을 방지

### 2. 근접 모드 스킬 추가(스킬3) 구현
- 캐릭터가 바라보는 방향으로 형태의 투사체를 발사하여 광역 피해를 입힘
- 적과 충돌 시 폭발 이펙트 및 사운드 재생
- 벽과 충돌 시에도 폭발하여 피해 발생
- Skill3CooldownTimerHandle을 통해 쿨다운을 적용하고, bIsUsingSkill3과 bCanUseSkill3 변수를 활용하여 연속 사용을 방지

### 3. Skill3Projectile 클래스 추가
- USphereComponent로 충돌 감지 OnComponentHit에서 충돌 이벤트 처리
- 충돌한 액터가 시전자 또는 자기 자신이 아닐 경우 폭발 로직 실행
- UGameplayStatics::ApplyDamage와 UKismetSystemLibrary::SphereTraceMultiForObjects를 활용해 폭발 반경 내 적에게 광역 피해 적용
- 적을 한 번만 타격하기 위해 TSet<AActor*>를 통해 중복 감지 방지 처리
- UProjectileMovementComponent를 통한 이동 구현
- UNiagaraSystem을 활용한 폭발 이펙트 연출

### 4. 기타 사운드 시스템 추가
- 스킬 사운드, 기본 공격 사운드, 발걸음 사운드 추가
- 사운드 큐의 어테뉴에이션 오버라이드를 활용하여 거리 기반 볼륨 감소 적용
- 채워지지 않은 사운드 또한 추가 예정

## 4월 22일 업데이트 내용

### 1. 메인캐릭터 클래스 리펙토링
- 기존의 AMainCharacter 클래스는 캐릭터의 이동, 점프, 대쉬, 무기 관리, 스킬 사용, 콤보 공격 등 모든 게임플레이 로직을 한 클래스에서 직접 처리하기에 다음과 같은 문제 발생
- 코드가 2천 줄에 가까워지며 한눈에 파악하기 어려움
- 서로 다른 기능들이 뒤섞여 있어 가독성 및 유지보수성 저하
- 새로운 기능 추가나 버그 수정 시, 의도치 않은 추가적인 문제 발생
- 각 기능별로 테스트를하기 어렵고 확장이 불가능.
- 이러한 문제를 해결하기 위해 단일 책임 원칙(SRP)에 따라 기능별로 클래스를 분리하여 각자의 역할과 책임을 명확히 함

### 1-1. AMainCharacter
- 역할
- 플레이어 캐릭터의 전체적인 상태와 동작, 입력 처리, 무기/스킬/애니메이션 등 주요 시스템의 중심
- 책임
- 이동, 점프, 대쉬, 에임 등 캐릭터의 기본 조작 처리
- 입력 바인딩 및 입력 이벤트 분기 처리
- 무기(총, 칼 등) 생성, 장착 및 위치 변환 관리
- 캐릭터의 상태(점프, 대쉬, 에임 등) 관리
- 각종 애니메이션 몽타주 및 변수 관리
- 전투/스킬 기능은 별도의 컴포넌트에 위임(MeleeCombatComponent, SkillComponent)
- 콤보 공격, 스킬 사용 등은 해당 컴포넌트의 메서드를 호출하여 처리

### 1-2. USkillComponent
- 역할
- 캐릭터의 다양한 스킬(액티브 스킬, 에임 스킬 등) 사용 및 쿨타임, 효과, 애니메이션, 무기 연동 등 스킬 관련 로직 집중 관리
- 책임
- 스킬1, 스킬2, 스킬3, 에임스킬1, 에임스킬2 등 각종 스킬의 사용 조건, 쿨타임, 효과, 애니메이션 실행 및 상태 관리
- 스킬별로 필요한 무기(머신건, 칼, 캐논 등)와의 연동 및 위치/상태 제어
- 스킬 효과 적용(범위 내 적 상태이상, 투사체 발사, 무기 발사 등)
- 스킬 사용 중 캐릭터의 회전, 애니메이션 섹션 분기, 타이머 관리 등 세부 로직 분리
- 스킬 사용 가능 여부, 사용 중 여부 등 상태값 제공
- 캐릭터의 상태(대쉬, 점프 등)와 연동하여 스킬 사용 제한
- 스킬 관련 모든 책임을 MainCharacter에서 분리하여 독립적으로 관리

### 1-3. UMeleeCombatComponent
- 역할
- 근접 콤보 공격, 발차기, 칼 공격 등 근접 전투 시스템의 로직 전담
- 책임
- 콤보 공격(연속 입력, 몽타주 실행, 콤보 인덱스 관리, 타이머 관리)
- 콤보별 애니메이션 몽타주 실행 및 종료 처리
- 칼/발차기 히트박스 활성화 및 비활성화, 충돌 감지 및 데미지 적용
- 콤보 공격 방향 자동 조정(적 자동 타겟팅 및 캐릭터 회전)
- 콤보 공격 중 이동(LaunchCharacter 등), 콤보 리셋 타이머 관리
- 콤보 공격 상태(공격 중 여부, 다음 입력 큐잉 등) 제공

- MainCharacter는 "플레이어의 상태와 입력, 무기 장착 등 전반적인 관리"만 담당
- SkillComponent는 "스킬 사용 및 관련 로직"만 담당
- MeleeCombatComponent는 "근접 전투 및 콤보 공격"에만 집중

- 각 컴포넌트가 명확한 책임을 가지며, 코드의 유지보수성, 확장성, 테스트 용이성 향상
- 이 구조는 "단일 책임 원칙(SRP)"에 충실하며 각 기능별로 독립적인 관리가 가능
- 각 컴포넌트가 자신만의 명확한 책임을 가지며 서로의 역할을 침범하지 않음

### 2. 에임 모드 스킬 추가 (에임모드 스킬1)
- 기관총으로 제자리에 선채로 고속으로 연사하는 스킬
- 플레이어 카메라 피치 각도를 AimPitch 변수로 갱신
- ABP와 연동하여 해당 스킬의 몽타주 실행시 상체는 Aimpitch 값을 따라 위 아래로 회전
- Start->Loop->Loop 무한반복
- (AimSkill1PlayInterval) 에 설정한 시간 간격으로 몽타주 재시작
- 쿨타임 적용 (AimSkill1Cooldown)
- 스킬 사용 시 자동으로 칼/라이플 수납
- 사용 중 이동/점프/다른 스킬 사용 차단

### 3. MachineGun 클래스 추가
- StartFire()를 호출하면 일정 간격(FireRate)마다 Fire() 함수가 반복 실행되어 자동으로 총알을 발사
- StopFire()를 호출하면 연사가 중단
- 탄퍼짐: 각 발사마다 GetFireDirectionWithSpread()를 통해 Pitch/Yaw 축 모두 무작위 탄퍼짐 으로 현실적인 사격 구현
- Fire() 함수에서 레이캐스트로 탄환 궤적을 계산하고, 적중 시 UGameplayStatics::ApplyPointDamage()로 데미지를 적용
- 디버그용 선(라인)으로 탄환 궤적을 시각화
- 파라미터 설정: SetFireParams()로 발사 속도(FireRate), 데미지(BulletDamage), 탄퍼짐(SpreadAngle) 등을 외부에서 조정가능

### 4. 에임 모드 스킬 추가 (에임모드 스킬2)
- 캐논으로 유도형/비유도형 투사체를 발사하는 스킬
- 준비 동작: AimSkill2StartMontage (팔 위치 조정), 발사 동작: AimSkill2Montage (캐논 발사) 로 2가지 몽타주가 순서대로 실행되며 AimSkill2Montage가 실행될때 발사
- 플레이어 카메라 피치 각도를 AimPitch 변수로 갱신
- ABP와 연동하여 해당 스킬의 몽타주 실행시 상체는 Aimpitch 값을 따라 위 아래로 회전
- 고각발사시: 투사체가 지정된 고도에 도달한 뒤 멈추고 적을 탐색
- 저각발사시: 유도 기능이 없는 일반적인 폭발형 투사체기능 수행
- 폭발한 투사체는 ApplyGravityPull을 사용하여 적을 당김
- 쿨타임 적용 (AimSkill2Cooldown)
- 스킬 사용 시 자동으로 칼/라이플 수납
- 사용 중 점프/다른 스킬 사용 차단

### 5. Cannon 클래스 추가
- 에임스킬2 투사체 발사 관리 클래스
- 투사체 발사 위치 보정 FVector SpawnLocation = GetActorLocation() + ShootDirection * 200.f + FVector(0.f, 0.f, -150.f);  Z축 -150.f 추가 보정 (고각/저각 발사 시 위치 차이 보정)
- 디버그 시각화 (DrawDebugSphere, DrawDebugDirectionalArrow)
- SetProjectileClass(TSubclassOf<AAimSkill2Projectile> InClass) 로 투사체 클래스 설정

### 6. AimSkill2Projectile 클래스 추가
- FindClosetEnemy()로 지정된 반경 내 가장 가까운 적 탐색
- 지정된 반경 내 적이 없을 시 일정시간 후 AutoExplodeIfNoTarget() 자동 폭발
- UGameplayStatics::ApplyDamage와 UKismetSystemLibrary::SphereTraceMultiForObjects를 활용해 폭발 반경 내 적에게 광역 피해 적용
- 적을 한 번만 타격하기 위해 TSet<AActor*>를 통해 중복 감지 방지 처리
- UProjectileMovementComponent를 통한 이동 구현
- UNiagaraSystem을 활용한 폭발 이펙트 연출

### 7. 근접공격 시각효과 개선
- 나이아가라 에셋 수정 및 PlaySlashEffectForwad 애님 노티파이 클래스 추가
- 나이아가라 에셋의 기존 360도 원형 이펙트가 실제 칼 공격 범위(전방)와 불일치
- 메쉬 형태 변경: Blender를 이용해 원형 메시 → 반원 메쉬로 재모델링
- 나이아가라 시스템 설정 변경: Mesh Orientation 모듈에 User_ForwardVector, User_RightVector 2개의 새로운 파라메터를 생성하여 바인딩, Local Space True설정으로 캐릭터의 방향 기준으로 회전값 반영
- 나이아가라에 생성한 파라미터에 User_ForwardVector, User_RightVector를 전달하여 나이아가라 내부 방향을 제어
- 애님몽타주에서 위치 오프셋(LocationOffset)을 설정가능
- 오브젝트 풀링으로 (ENCPoolMethod::AutoRelease) 성능 최적화  

### 8. 스킬 사용시 스폰되고 사라지는 MachineGun과 Cannon
- Destroy 대신 SetActorHiddenInGame 사용
- 필요할 때만 SetActorHiddenInGame(false)로 표시
- 사용이 끝나면 SetActorHiddenInGame(true)로 숨김
- 필요한 오브젝트인 머신건/캐논 1회 생성하고 월드에 남김
- 메모리 할당/해제 오버헤드 감소: 반복적인 Spawn/Destroy를 피해 GC(가비지 컬렉션) 부하를 최소화
- 반응 속도 향상: 숨김 처리된 오브젝트를 즉시 재활용하므로 스킬 사용 시 지연이 없음

## 6월 1일 업데이트 내용

### 1. 일반공격 히트판정 개선
- 일반공격 이펙트는 전방 180도로 표현되지만 실제 히트는 LineTraceSingleByChannel로 인해 단일 적만 타격
- SweepMultiByChannel 로 레이캐스트 방식을 변경하여 구체를 스윕시켜 모든 충돌한 적들에게 데미지를 가할 수 있게 수정
- 9개의 스윕을 부채꼴 형태로 발사
- 각 스윕마다 반지름 20의 구체가 경로를 따라 이동
- 180도 범위를 완전히 커버
- 여러 스윕에서 같은 적이 감지 되어도 Tset을 통해 한 번만 데미지 적용

### 2. 에임스킬2 투사체 로직 개선
- 투사체가 폭발 후 즉시 사라지는 것이 아니라, 일정 시간 동안 폭발 영역을 유지하는 시스템이 추가
- SpawnPersistentExplosionArea(): 폭발 위치에 지속적인 영역 효과를 생성
- ApplyPersistentEffects(): 0.1초마다 적들을 끌어당기는 효과 적용
- ApplyPeriodicDamage(): 설정된 간격마다 영역 내 적들에게 지속 데미지 적용
- 투사체 생명 주기 변경 충돌시 기존에는 즉시 Destroy 했으나 충돌 시 액터를 숨기고 콜리전 비활성화 후 폭발 지속시간이 끝나면 SetLifeSpan()으로 파괴
- 메모리 누수 방지를 위해 액터가 파괴될 때 모든 타이머와 오디오 컴포넌트를 정리하는 EndPlay() 함수가 추가

### 3. 점프 공격(실험적) 추가 
- 점프 중 공격과 더블점프 중 공격이 가능
- 이 기능을 추가한 이후 점프공격 후 일반공격 시 간헐적으로 일반공격의 몽타주가 빠르게 스킵되는 현상이 발생하여 해결중

## 7월 24일 업데이트 내용

### 1. 점프공격 몽타주 스킵현상 미해결
- 현재까지도 동일한 증상이 나타나며 해결을 위해 노력중

### 2. 카메라 줌 개선
- PreviousZoom을 추가로 선언하여 에임모드해제 시 DefaultZoom 값으로 돌아가지 않고 마지막 Zoom값으로 돌아감

### 3. 에임스킬3 추가
- 클래스 구조
- 메인 클래스: USkillComponent - 에임스킬3의 핵심 로직을 관리하는 컴포넌트
- 투사체 클래스: AAimSkill3Projectile - 하늘에서 떨어지는 투사체

- 핵심 기능
- 타겟 지점 설정: 캐릭터 앞방향으로 1500 단위 거리까지 라인 트레이스하여 타겟 위치 결정
- 다중 투사체: 5개의 투사체를 캐릭터 앞쪽부터 일정 간격으로 배치
- 지연 낙하: 스킬 시전 후 3초 뒤에 투사체들이 하늘에서 낙하
- 범위 표시: 하늘색 원으로 투사체 낙하 예정 지점을 6초간 표시 (반지름 300)
- 지면 높이 보정: 각 투사체 낙하 지점의 지면 높이를 자동으로 계산하여 Z좌표 10으로 보정

- 스킬 실행 순서
- 즉시 실행: 몽타주 재생, 타겟 지점 계산, 범위 표시
- 3초 후: SpawnAimSkill3Projectiles() 함수로 투사체들을 하늘 높이(+2000)에서 생성
- 투사체 생성: 각 투사체는 수직 아래방향(-90도)으로 발사되어 지면으로 낙하

- 피해적용
- 충돌 트리거: OnHit() 함수에서 적이나 지면과 충돌 시 폭발 로직 실행
- 광역 피해 범위: 충돌 지점 중심으로 반지름 150 단위 내 모든 적 탐지
- 타겟 검출: SphereTraceMultiForObjects()로 ECC_Pawn 타입 액터들을 다중 탐지
- 중복 피해 방지: TSet<AActor*> DamagedActors를 통해 같은 적이 여러 번 피해받지 않도록 보장
- 데미지 적용: UGameplayStatics::ApplyDamage()로 기본 60의 피해량 적용
- 시전자 제외: IgnoredActors에 Shooter를 추가하여 플레이어는 피해받지 않
- 충돌 감지: 투사체가 지면이나 적과 충돌 시 폭발
- 광역 피해: 폭발 지점 중심으로 설정된 반지름 내 모든 적에게 피해 적용

### 4. Enemy 공격 판정 추가 및 기타 변화
- 클래스 구조
- 메인 클래스: AEnemy - 적 캐릭터 본체, 공격 애니메이션과 AI 행동 관리
- 무기 클래스: AEnemyKatana - 물리적 공격 판정과 레이캐스트 시스템 담당
- 노티파이 클래스: UAnimNotify_EnemyStartAttack, UAnimNotify_EnemyEndAttack - 애니메이션 타이밍 제어

- 공격 몽타주 변경
- NormalAttackMontages 배열 (일반공격 + 연속공격, 발차기 삭제)
- StrongAttackMontage (강공격)
- JumpAttackMontages 배열 (점프공격)

- 플레이어 데미지 적용
- 공격 타입별 데미지: EAttackType 열거형으로 Normal(20/30), Strong(50/60), Jump(30/40) 구분
- 엘리트 적 보너스: bIsEliteEnemy가 true일 경우 모든 공격 데미지 1.5배 증가
- 타겟 검증: 플레이어(AMainCharacter)만 데미지 적용, 다른 적들은 무시
- 중복 피해 방지: TSet<AActor*> DamagedActors로 같은 공격에서 한 번만 피해 적용

- 노티파이 기반 타이밍 제어 시스템
- 공격 시작: UAnimNotify_EnemyStartAttack에서 AttackType 설정 후 Enemy->StartAttack() 호출
- 판정 활성화: AEnemyKatana::EnableAttackHitDetection()로 무기 히트 판정 시작
- 실시간 검사: 카타나의 Tick() 함수에서 bIsAttacking이 true일 동안 지속적으로 PerformRaycastAttack() 실행
- 공격 종료: UAnimNotify_EnemyEndAttack에서 Enemy->EndAttack() 호출하여 판정 비활성화

- 레이캐스트 공격 판정 시스템
- 다중 스윕: 카타나 전방 120 단위를 5단계로 나누어 구체 스윕 (반지름 30) 실행
- 충돌 필터링: 자신과 다른 적들은 IgnoredActor로 설정하여 플레이어만 타격
- 시각적 피드백: 공격 경로는 초록색 선, 플레이어 히트 시 빨간색 구체로 표시
- 즉시 중단: 플레이어 타격 시 즉시 스윕 중단
- 무기 휘두르기 타이밍 동기화
- 연속 공격은 애니메이션 몽타주의 무기 휘두르는 순간마다 노티파이가 트리거되어
몽타주 재생 → 2. StartAttack 노티파이 → 3. 실시간 히트 판정 → 4. EndAttack 노티파이 → 5. 판정 종료
이 과정이 각 콤보 동작마다 반복되어 정확한 타이밍에 피해를 입힘

- 기타 변화
- 공격도중에 피격당하거나 스턴상태일 경우 레이캐스트나 히트박스가 활성화되는 시점이더라도 꺼짐
- 강공격 몽타주중에 히트했을땐 히트몽타주 재생안하고 체력만 차감됌. 근데 강공격 재생중에 체력이 0이하가 되면 사망 애니메이션 호출
- 스폰 즉시 인트로 몽타주가 실행되며 재생 중엔 피해를 입지 않음.

### 5. 엘리트 Enemy 추가
- 확률 기반 생성: BeginPlay()에서 10% 확률로 엘리트 적 생성

- 시각적 차별화
- 등장 애니메이션: EliteSpawnIntroMontage와 SpawnIntroMontage로 구분
- ABP 연동: bIsEliteEnemy 변수를 애니메이션 블루프린트로 전달하여 아이들/이동 애니메이션 분기
- 이동속도 기반 블렌드스페이스: 엘리트(500)와 일반(300) 이동속도에 맞는 서로 다른 BS 사용
- 이동속도 증가: ApplyBaseWalkSpeed()에서 엘리트는 500, 일반은 300으로 설정
- 애니메이션 재생속도 가속: 모든 공격 몽타주를 1.5배 속도로 재생

- 스탯 차별화
- 체력 증가: ApplyEliteSettings()에서 일반(100) → 엘리트(200)로 2배 증가
- 데미지 증가: EnemyKatana에서 모든 공격 타입별로 데미지 상승
일반공격: 20 → 30
강공격: 50 → 60
점프공격: 30 → 40 

### 6. Enemy ai 컨트롤러 조건 강화
- 플래그 조건 강화로 일반공격, 점프공격, 강공격, 닷지 등 현재 상태에서 실행하는 몽타주 도중에 다른 상태의 몽타주를 재생하는 것을 방지하고 완전히 몽타주가 끝나야 다음 행동으로 넘어감

### 7. Enemy AI 포위로직 추가 및 개선  
- 다수의 적들이 플레이어를 자연스럽게 포위하면서도 조직적인 전투 행동을 수행

- AI 상태머신 시스템
- 메인 클래스: AEnemyAIController - AI 행동 제어 및 상태 관리
- 상태 열거형: EEnemyAIState - Idle, MoveToCircle, ChasePlayer 3가지 상태
- 상태 전환: UpdateAIState()에서 플레이어와의 거리에 따른 자동 상태 변경

- 포위 로직 시스템
- 원형 포위 전략: 플레이어 중심으로 반지름 200 단위의 원 위에 배치
- 각도 분산: StaticAngleOffset 변수로 각 AI마다 고유한 각도 할당하여 겹침 방지
- 랜덤 변화: 기본 각도에서 ±30도 랜덤 오프셋으로 자연스러운 움직임 연출
- 거리 변화: 반지름을 ±50 단위로 랜덤 조정하여 일직선 배치 방지

- AI 작동 순서
- 1단계: 거리 측정
FVector::DistSquared() 사용으로 제곱근 계산 최적화
플레이어와의 거리에 따른 행동 결정
- 2단계: 상태 판정
3000 단위 초과: Idle 상태 (추적 중단)
320~3000 단위: MoveToCircle 상태 (포위 이동)
320 단위 이하: ChasePlayer 상태 (직접 추격)
- 3단계: 행동 실행

- 포위 위치 계산 로직
- 각도 계산: StaticAngleOffset을 1도씩 증가시켜 360도 순환
- 위치 생성: FVector(Cos(각도), Sin(각도), 0) * 반지름으로 원형 좌표 생성
- 내비게이션 보정: ProjectPointToNavigation()으로 이동 가능한 지형으로 위치 조정
- 캐싱 시스템: CachedTargetLocation으로 목표 지점 저장하여 불필요한 재계산 방지

- 성능 최적화
- 틱 빈도 제한: AI 업데이트를 60fps → 20fps로 감소 (SetActorTickInterval(0.05f))
- 회전 보간 최적화: 플레이어 바라보기를 60fps → 10fps로 제한
- 타이머 기반 갱신: 원형 위치를 0.7초 → 2초 간격으로 재계산

- 전투 시스템
ChasePlayer 상태에서의 행동:
점프공격: 거리 관계없이 우선 실행 (JumpAttack())
일반공격: 200 단위 내에서 실행, 3회 후 강공격으로 자동 전환
회피 시스템: 30% 확률로 공격 대신 회피 선택
쿨다운 관리: 각 행동마다 독립적인 타이머로 스팸 방지

- 상태별 동작
- Idle: 모든 이동 중단, 플레이어 추적 대기
- MoveToCircle: 포위 위치로 이동, 도달 시 Idle로 전환하여 대기
- ChasePlayer: 직접 추격하며 공격 패턴 실행

### 8. Enemy 닷지 거리 증가
- 커스텀 노티파이를 이용하여 원하는 순간에 루트모션을 해제하고 end 포인트에서 루트모션을 재활성화
- 런치 시작: UAnimNotify_EnemyDodgeLaunch에서 Enemy->OnDodgeLaunchNotify(bDodgeLeft) 호출
- 런치 종료: UAnimNotify_EnemyDodgeLaunchEnd에서 Enemy->OnDodgeLaunchEndNotify() 호출
- 타이밍 제어: 애니메이션 몽타주의 특정 프레임에 노티파이 배치하여 정확한 타이밍에 루트모션 해제 및 목구
- 루트모션 + 런치 혼합하여 사용

- 루트모션과 런치를 동시에 사용하면 안 되는 이유
- 제어권 충돌
- 루트모션: 애니메이션 데이터가 캐릭터 이동을 직접 제어
- LaunchCharacter: 물리 시스템이 Velocity를 통해 캐릭터를 강제 이동
- 결과: 두 시스템이 서로 다른 방향으로 캐릭터를 움직이려 해서 예측 불가능한 움직임이 발생
- 우선순위 문제
- 언리얼 엔진에서 루트모션이 활성화되면 물리적 힘(Launch)보다 우선권을 가짐
- LaunchCharacter 호출해도 루트모션이 이를 무시하거나 상쇄시킴
- 최종적으로 원하는 거리만큼 이동하지 못함

- 루트모션을 유지한 이유
- 루트모션으로 닷지 시작과 끝부분의 자연스러운 애니메이션 유지
- 캐릭터의 회전과 미세한 위치 조정이 애니메이션과 완벽히 동기화
- 닷지 시작 시 관성과 가속도가 자연스럽게 표현됨
- 닷지 종료 시 감속과 착지 모션이 부드럽게 연결됨
- 일관성 유지: 다른 적 행동들(공격, 이동)도 루트모션 기반이므로 시스템 통일성 유지

- 단계별 제어권 양도
- 1단계: 루트모션 시작: 닷지 애니메이션의 준비 동작
- 2단계: 물리 제어: 루트모션 해제 후 LaunchCharacter로 강력한 이동력 적용
- 3단계: 루트모션 복귀: 착지 및 마무리 동작을 자연스럽게 처리

### 9. Enemy, EnemyBoss 사망 시 메모리 최적화 개선
- 문제해결 및 개선노트 32번 참고

### 10. 웨이브 시스템 및 웨이브 시스템 프레임 드랍 현상 최적화
- 프레임 드랍 현상 최적화 세부 내용은 문제해결 및 개선노트 33번 참고
- 작동 원리
- 메인 클래스: AMainGameModeBase - 전체 웨이브 시스템 제어 및 스폰 관리
- 구조체: FEnemySpawnInfo, FBossSpawnInfo - 스폰 정보 데이터 구조화
- 타이머 시스템: MainSpawnTimer로 10초 간격 스폰 단계 진행

- 웨이브 패턴 시스템
- 스폰 패턴 (ProcessSpawnStep 기반)
- 지정된 시간 단위로 Enemy 및 Boss 스폰 Enemy는 최대 5명씩 소환
- ex) 10초 1명 20초 2명 30초 3명 40초 4명 50초 5명
- 보스 등장 시  (Enemy가 남아있는 경우 보스와의 1대1 구도를 위하여 모두 즉시 사망 및 웨이브 레벨1 모든 메모리 정리)
- 보스 사망 후 (다음 웨이브 레벨의 적 스폰 증가된 체력, 이동속도)
- ex)와 동일한 패턴으로 계속해서 반복

- 시스템 흐름
- 시작: StartWaveSystem() → StartWaveLevel() → ProcessSpawnStep() 타이머 시작
- 스폰 진행: 10초마다 ProcessSpawnStep() 호출하여 SpawnStepCount 증가
- 보스 단계: 6단계 도달 시 모든 적 제거 후 보스 스폰
- 웨이브 전환: 보스 사망 시 10초 대기 후 CurrentWaveLevel++
- 반복: 새로운 웨이브 레벨에서 1단계부터 다시 시작

### 11. EnemyBoss 추가
- 전신공격 (정지공격), 상반신공격(움직이면서공격), 후퇴텔레포트(플레이어로부터 멀어짐), 원거리공격, 플레이어뒤로 즉시 텔레포트 후 공격, 스텔스 모드 등의 다양한 공격 패턴을 가짐.

```
- 공격 우선순위 시스템
NormalAttack 상태 진입
    ↓
1순위: 스텔스 공격 조건 체크 (최우선)
    ├─ 300~600 거리 + bCanUseStealthAttack = true
    ├─ 50% 확률로 스텔스 공격 실행
    └─ 실패 시 → 2순위로 진행
    ↓
2순위: 거리별 일반 공격 패턴
    ├─ 0~200: 근거리 패턴
    ├─ 201~250: 중거리 패턴  
    └─ 251+: 추적 재개

- 스텔스 공격 단계별 진행
스텔스 공격 시작 (300~600 거리에서 50% 확률)
    ↓
1단계: AI 완전 비활성화
    ├─ bIsAIDisabledForStealth = true
    ├─ bCanBossAttack = false
    └─ 모든 이동 정지
    ↓
3단계: 투명화 + 추적 해제
    ├─ SetFocus(nullptr)
    └─ 플레이어가 보스를 못 봄
    ↓
5단계: 킥 공격
    ├─ SetFocus(Player) - 플레이어 재포커싱
    └─ 공격 실행
    ↓
6단계: 피니쉬 공격
    └─ 강력한 마무리 공격
    ↓
0단계: 스텔스 완전 종료
    ├─ bIsAIDisabledForStealth = false
    ├─ bCanBossAttack = true  
    ├─ SetFocus(Player) - 플레이어 재추적
    └─ 강제 MoveToPlayer 상태 전환

- 근거리 전신 공격 패턴 (0~200 범위)
근거리 진입
    ↓
50% 확률 분기
    ├─ 후퇴 텔레포트 (50%)
    │   ↓
    │   플레이어와 멀어지는 위치로 텔레포트
    │   ↓
    │   텔레포트 완료 후 행동 결정
    │   ├─ bShouldUseRangedAfterTeleport = true
    │   │   ↓
    │   │   투사체 공격 실행
    │   │   ↓
    │   │   투사체 공격 완료 후 걸어서 추격 재개
    │   │   └─ OnBossRangedAttackEnded() → bCanBossAttack = true
    │   │
    │   └─ bShouldUseRangedAfterTeleport = false
    │       ↓
    │       즉시 걸어서 추격 재개
    │
    └─ 전신 정지 공격 (50%)
        ├─ PlayBossNormalAttackAnimation()
        ├─ 제자리에서 강력한 근거리 공격
        └─ 공격 완료 후 일반 AI 재개

- 투사체 공격 완료 후 행동
투사체 공격 종료
    ↓
OnBossRangedAttackEnded() 호출
    ├─ bCanBossAttack = true 설정
    └─ bIsBossRangedAttacking = false
    ↓
다음 틱에서 거리 재측정
    ├─ 플레이어와 거리 계산
    └─ 거리별 상태 전환
    ↓
거리에 따른 행동
    ├─ 251+ 거리 → MoveToPlayer (걸어서 추격)
    ├─ 201~250 → 상체 분리 이동 공격
    └─ 0~200 → 근거리 공격 패턴 재시작

- 통합 워크플로우
보스 AI 틱 시작
    ↓
특수 상황 체크
    ├─ 등장 애니메이션 중 → 행동 중지
    ├─ 스텔스 공격 중 → AI 비활성화
    └─ 텔레포트/전신공격 중 → 행동 제한
    ↓
일반 상황일 때 거리 측정
    ↓
NormalAttack 상태에서 공격 패턴 결정
    ↓
1순위: 스텔스 공격 체크 (300~600)
    ├─ 조건 만족 + 50% 확률 → 스텔스 공격 실행
    └─ 실패 → 거리별 일반 공격으로 진행
    ↓
2순위: 거리별 공격 패턴
    ├─ 0~200: 후퇴텔레포트(50%) vs 전신공격(50%)
    │   └─ 후퇴텔레포트 시 → 투사체공격 or 즉시추격
    ├─ 201~250: 상체분리 이동공격
    └─ 251+: MoveToPlayer (걸어서 추격)
    ↓
공격 완료 후
    ├─ 쿨타임 적용 (1초)
    ├─ 상태 플래그 초기화  
    └─ 다음 틱에서 새로운 행동 결정
    ↓
투사체 공격 완료 시
    ├─ OnBossRangedAttackEnded() 호출
    ├─ 즉시 공격 가능 상태 복구
    └─ 걸어서 플레이어에게 다가가기 시작
```
- 스텔스 공격의 레이캐스트시 캐릭터를 FVector LaunchVelocity 띄우고 즉시 ExecuteStealthFinish 함수를 호출하여 StealthFinish 몽타주를 재생하며Raycast로 즉시공격. 레이캐스트에 맞지 않았을 시 ExecuteStealthFinish로 넘어가지 않고 즉시 플레이어 추격 모드로 전환. 
- 현재 계속해서 작업중이며 기능 추가 및 개선 예정
- 정리된 보스에 대한 자세한 설명이 추가 될 예정

### 12. 맵 적용
- 신규 맵에서 해당 게임을 즐길 수 있도록 변경
- 해당 맵의 블룸과 라이팅 설정으로 인한 특정 이펙트의 과도한 눈부심 현상 발생
- 시각적 편의성을 위한 적절한 설정값 조정 작업 진행중

## 10월 18일 업데이트

### 1. 보스 (Boss) AI
- 다양한 특수 패턴과 예측 불가능한 움직임을 결합하여 플레이어를 압박하는 복합형 AI

- 1. 주요 시스템 및 로직
- 메인 클래스: ABossEnemy, ABossEnemyAIController, AEnemyBossKatana, ABossProjectile
- 상하체 분리 시스템: UBossEnemyAnimInstance의 bUseUpperBodyBlend 플래그를 통해, 이동 중에도 상체는 독립적으로 공격 애니메이션을 재생할 수 있어 지속적인 압박 가능
- 텔레포트 시스템: 두 가지 종류의 텔레포트를 전술적으로 사용
- 후퇴 텔레포트: 플레이어로부터 멀어지는 방향으로 이동하며, 텔레포트 후 잠시 멈춘 뒤 원거리 공격 또는 공격형 텔레포트를 연계
- 공격 텔레포트: 플레이어의 등 뒤 사각지대로 빠르고 기습적으로 이동하여 즉시 공격을 시작
- 무적(Invincibility) 시스템: 텔레포트 및 스텔스 공격의 특정 구간에서는 bIsInvincible 플래그가 활성화되어 모든 피해를 무시
- 메모리 관리: 사망 시 HideBossEnemy() 함수를 통해 장착된 무기, AI 컨트롤러, 타이머, 델리게이트 등 모든 종속 객체를 안전하게 정리하고 SetTimerForNextTick을 이용해 다음 프레임에 액터를 파괴

- 2. AI 상태머신 시스템
- 메인 클래스: ABossEnemyAIController - 보스의 모든 행동을 제어하고 상태를 관리
- 상태 열거형: EBossEnemyAIState - Idle, MoveToPlayer, NormalAttack 3가지 핵심 상태를 가짐
- 상태 전환: UpdateBossAIState() 함수에서 플레이어와의 거리에 따라 상태를 자동으로 변경
- 상태 잠금: bIsFullBodyAttacking, bIsBossTeleporting 등 보스의 전신 애니메이션이 재생되는 동안에는 AI 상태가 NormalAttack으로 고정되어 다른 행동으로 전환되지 않음

- 3. 스텔스(Stealth) 공격 시스템
- 6단계 시퀀스: 보스의 가장 강력한 특수 패턴으로, 6개의 정해진 단계에 따라 순차적으로 진행
- 1단계 (시작): StealthStartMontage를 재생하며 스텔스에 진입
- 2단계 (돌진): StealthDiveMontage를 재생하며 지면으로 파고드는 듯한 모션을 취함
- 3단계 (은신 및 추적): SetActorHiddenInGame(true)로 완전히 투명해진 뒤, 타이머(StealthWaitTimer)를 이용해 5초 동안 주기적으로 플레이어 주변의 텔레포트 위치를 갱신하며 추적
- 4단계 (텔레포트): 5초 후, 마지막으로 갱신된 위치(플레이어의 전후좌우 중 랜덤)로 즉시 텔레포트
- 5단계 (킥 공격): 모습을 드러내며 StealthKickMontage를 재생하고, LineTrace 판정으로 플레이어를 타격
- 6단계 (피니시): 킥 공격에 맞은 플레이어는 LaunchCharacter에 의해 공중에 뜨고 중력이 0이 되어 무력화되며, 보스는 StealthFinishMontage를 재생하여 LineTrace 판정의 연계 마무리 공격 수행
- 쿨다운 관리: 스텔스 공격이 완전히 종료되면 StealthCooldown 타이머가 작동하여 연속 사용을 방지

- 4. AI 작동 순서
- 1단계: 특수 상태 확인: Tick 함수에서 가장 먼저 보스가 등장 중(bIsPlayingBossIntro)이거나 스텔스 공격 중(IsExecutingStealthAttack())인지 확인하고, 해당될 경우 다른 모든 AI 로직을 중단
- 2단계: 거리 측정: 플레이어와의 거리를 계산하여 UpdateBossAIState()를 호출하고, 현재 상태를 갱신
- 3단계: 행동 실행: 현재 CurrentState에 따라 BossMoveToPlayer()(이동) 또는 BossNormalAttack()(공격 결정) 함수를 호출
- 4단계: 공격 패턴 결정: BossNormalAttack() 함수 내에서 현재 거리, 각종 쿨타임, 확률(50%)을 종합적으로 판단하여 근접 공격, 상체 공격, 텔레포트, 스텔스 등 가장 적절한 공격 패턴을 최종적으로 선택하고 실행

- 5. 전투 시스템
- 카타나 공격: AEnemyBossKatana는 공격 애니메이션이 재생되는 동안 Tick 함수에서 SweepMultiByChannel 판정을 지속적으로 발생시켜, 빠르고 넓은 범위의 공격 판정을 생성
- 스텔스 킥 & 피니시: 킥 공격이 명중하면 LaunchCharacter를 통해 플레이어를 공중에 띄우고 GravityScale을 0으로 만들어 무력화시킵니다. 이후 이어지는 피니시 공격이 명중하거나 빗나가면 플레이어의 중력은 다시 1.0으로 복구
- 투사체 공격: ABossProjectile은 발사 후 직선으로 날아가며, 충돌 시 ApplyAreaDamage()를 호출하여 DamageRadius 범위 내의 모든 플레이어에게 광역 피해를 입힘

### 2. EnemyDog AI
- 다수의 적들이 플레이어를 자연스럽게 포위하면서도 조직적인 전투 행동을 수행
- AI 상태머신 시스템
- 메인 클래스: AEnemyDogAIController - EnemyDog의 이동, 추적, 공격 등 모든 행동을 제어
- 상태 열거형: EEnemyDogAIState - Idle, ChasePlayer 2가지 상태를 가짐
- 상태 전환: UpdateAIState() 함수에서 플레이어와의 거리에 따라 상태를 자동으로 변경

- 1. 포위 로직 시스템
- 원형 포위 전략: ChasePlayer 상태일 때, 플레이어를 중심으로 반지름 100 단위(SurroundRadius)의 원 위에 자신의 위치를 계산하여 이동
- 각도 분산: 월드에 존재하는 모든 EnemyDog 수와 각자의 고유한 인덱스를 기반으로 360도를 균등하게 분배하여 목표 각도를 할당함으로써 서로 겹치지 않고 포위 진형을 형성
- 동적 위치 선정: TActorIterator를 사용해 실시간으로 아군 수를 파악하고 자신의 인덱스를 찾아내므로, 아군이 죽거나 새로 스폰되어도 유동적으로 포위 위치를 재조정

- 2. AI 작동 순서
- 1단계: 거리 측정: Tick 함수 내에서 FVector::Dist()를 사용해 플레이어와의 거리를 실시간으로 계산
- 2단계: 상태 판정: 2000 단위 초과: Idle 상태 (추적 중단 및 제자리 대기)
- 150 ~ 2000 단위: ChasePlayer 상태 (포위 이동)
- 150 단위 이하: NormalAttack 상태 (포위 이동 중단 및 공격 실행)
- 3단계: 행동 실행: 결정된 상태에 따라 ChasePlayer() 또는 NormalAttack() 함수를 호출하여 실제 행동을 수행

- 3. 포위 위치 계산 로직
- 각도 계산: 360.0f / (전체 EnemyDog 수)로 기본 각도를 계산하고, 자신의 인덱스를 곱하여 고유 각도를 할당

- 4. 위치 생성
- FVector(Cos(각도), Sin(각도), 0) * SurroundRadius 공식을 통해 플레이어 중심의 원형 좌표를 생성합니다. 

- 5. 내비게이션 이동
- MoveToLocation() 함수를 사용해 계산된 포위 위치로 이동합니다.

- 6. 성능 최적화
- 틱 빈도 제한: AI 업데이트 주기를 0.05초 (초당 20회)로 제한하여 Tick 함수의 과도한 호출을 방지
- 회전 보간 최적화: 플레이어를 바라보는 회전을 FMath::RInterpTo 함수를 통해 프레임에 독립적인 부드러운 움직임으로 처리

- 7. 전투 시스템
- NormalAttack 상태에서의 행동:
공격 실행: 150 단위 내에서 bCanAttack이 true일 때 NormalAttack() 함수를 호출합니다.
- 애니메이션 연계: AEnemyDog의 PlayNormalAttackAnimation() 함수를 호출하여 실제 공격 애니메이션과 피해 판정을 실행
- 쿨다운 관리: 공격 후 AttackCooldown 타이머가 작동하여 ResetAttack() 함수가 호출되기 전까지는 연속 공격을 방지
- 상태별 동작
- Idle: 모든 이동을 중단하고 플레이어가 탐지 범위에 들어오기를 기다림
- ChasePlayer: 계산된 포위 위치로 이동하며 플레이어와의 거리를 유지
- NormalAttack: 이동을 멈추고 플레이어를 향해 즉시 회전한 후, EnemyDog에게 공격 명령

### 3. EnemyDrone AI
- 공중에서 플레이어 주위를 맴돌며 장애물을 회피하고 지속적으로 사격하는 비행형 AI

- 1. AI 상태머신 시스템
메인 클래스: AEnemyDroneAIController - EnemyDrone의 궤도 비행, 장애물 회피, 고도 조절 등 복잡한 공중 기동을 제어

- 2. 상태 열거형: 별도의 상태 열거형 대신 bRising(긴급 상승 중), bTriedReverse(장애물 충돌 후 방향 전환 시도) 등 Boolean 플래그를 조합하여 상태를 관리

- 3. 상태 전환: Tick 함수 내에서 플레이어와의 거리, 장애물 충돌 여부, 끼임 상태 등을 종합적으로 판단하여 각 플래그를 동적으로 변경

- 4. 핵심 행동 로직 시스템 
- 궤도 비행 전략: 플레이어를 중심으로 반지름 500 단위(OrbitRadius)와 높이 300 단위(HeightOffset)를 유지하며 원형으로 비행하는 것을 기본 행동으로 함
- 장애물 회피: LineTrace를 이용해 자신의 이동 경로에 장애물이 있는지 지속적으로 확인함
- 아군 드론 충돌 시: HeightOffset을 랜덤하게 변경하여 상하로 회피함
- 지형지물 충돌 시: bClockwise 플래그를 반전시켜 궤도 방향을 바꿈
- 끼임 탈출 로직: 방향 전환 후에도 계속 장애물에 막힐 경우(MaxStuckTime 초과), 고도를 급격히 높여 해당 지역을 완전히 벗어남
- 궤도 이탈 복귀: 설정된 궤도 반경을 일정 시간 이상 벗어날 경우, 끼임 탈출 로직과 동일하게 고도를 급상승하여 궤도로 복귀를 시도함

- 5. AI 작동 순서
- 1단계: 상태 및 거리 측정: 플레이어와의 2D 거리, 궤도 이탈 여부, 장애물 충돌 여부를 Tick마다 계산함
- 2단계: 긴급 행동 판정: 궤도를 이탈했거나 장애물에 끼인 상태가 감지되면, 다른 모든 로직을 무시하고 긴급 상승(bRising = true) 상태로 전환함
- 3단계: 일반 행동 실행: 긴급 상황이 아닐 경우, 계산된 궤도 위치로 이동하고 플레이어를 향해 지속적으로 회전함

- 6. 궤도 위치 계산 로직
- 각도 계산: CurrentAngle 변수를 OrbitSpeed에 따라 매 틱마다 증감시켜 현재 목표 각도를 계산 
- 위치 생성: FVector(Cos(각도), Sin(각도), HeightOffset) 공식을 통해 플레이어 중심의 3D 원형 좌표를 생성
- 이동 보간: FMath::VInterpTo를 사용해 현재 위치에서 목표 위치로 부드럽게 이동

- 7. 성능 최적화
- 틱 빈도 제한: EnemyDrone 액터 자체의 틱 주기를 0.2초로 제한하여 비싼 연산을 줄임
- 전투 시스템
- 독립적인 발사 시스템: AI는 이동만 제어하고, 공격은 AEnemyDrone 액터가 자체적으로 MissileCooldown 타이머에 따라 독립적으로 수행함
- 지속적인 조준: 이동 중에도 Tick 함수에서 LookAtPlayerWithConstraints를 통해 항상 플레이어를 정확히 조준하여 미사일 발사에 대비함

- 8. 상태별 동작
- 궤도 비행: 기본 상태. 플레이어 주위를 돌며 거리를 유지
- 장애물 회피: LineTrace에 장애물이 감지되면 궤도 방향을 바꾸거나 고도를 변경함
- 긴급 상승: 장애물에 끼었거나 궤도를 너무 많이 이탈했을 때, 고도를 급격히 높여 위기를 탈출함

### 4. EnemyShooter AI
- 아군과 진형을 유지하고 엄폐물을 활용하며, 시야가 막히면 수류탄을 사용하는 전략적인 원거리 AI

- 1. AI 상태머신 시스템
- 메인 클래스: AEnemyShooterAIController - EnemyShooter의 복잡한 전투 행동과 상태를 관리 

- 2. 상태 열거형
- EEnemyShooterAIState - Idle, Detecting, Moving, Shooting, Retreating 5가지의 명확한 상태를 가짐

- 3. 상태 전환
- UpdateAIState()를 통해 현재 상태에서 다른 상태로 전환할 조건을 매번 검사하고, 상황에 맞는 상태로 변경

- 4. 핵심 행동 로직 시스템
- 포메이션(진형) 전략: 플레이어를 중심으로 반지름 600 단위(FormationRadius)의 원형 진형을 유지하려고 시도

- 5. 아군 이격
- 다른 아군과 최소 180 단위(MinAllyDistance) 이상 거리를 유지하여 서로 뭉치거나 시야를 가리는 것을 방지

- 6. 사선 확보시스템
- LineTrace를 통해 플레이어 사이에 다른 아군이나 지형지물이 있는지 주기적으로 확인

- 7. 전술적 판단
- 사선이 다른 EnemyShooter에 의해 막히면, 자리를 바꾸기 위해 Moving 상태로 전환함

- 8. 사선이 EnemyGuardian(방패병)에 의해 막히면, 제자리에 서서 수류탄 투척을 시도하는 특수 패턴으로 전환함

- 9. 작동 순서
- 1단계: 데이터 캐싱: Tick 함수에서 매번 비싼 연산을 하는 것을 막기 위해 타이머를 사용함
- 1초 주기: 주변 아군 목록(CachedAllies) 갱신
- 0.3초 주기: 사선 확보 여부(bCachedHasClearShot) 갱신
- 1초 주기: 자신의 포메이션 위치(AssignedPosition) 갱신
- 2단계: 상태 판정: 캐시된 데이터와 플레이어와의 거리를 기반으로 UpdateAIState()에서 다음에 전환할 상태를 결정
- 3단계: 행동 실행: 현재 상태에 맞는 Handle...State() 함수가 호출되어 이동, 사격, 후퇴 등의 행동을 실행

- 10. 포메이션 위치 계산 로직
각도 계산: 모든 아군 EnemyShooter 중 자신의 고유 인덱스를 찾아 360도를 균등하게 나눈 각도를 할당받음

- 11. 위치 생성: 플레이어 위치에 FVector(Cos(각도), Sin(각도), 0) * FormationRadius를 더해 이상적인 위치를 계산함

- 12. 위치 보정: 계산된 위치에 이미 다른 아군이 있다면, 자리가 비워질 때까지 45도씩 회전하며 새로운 위치를 재탐색함

- 13. 성능 최적화: 틱 빈도 제한: EnemyShooter 액터의 틱 주기를 0.2초로 제한함

- 14. 타이머 기반 갱신: 아군 탐색, 사선 확인, 진형 위치 계산 등 주요 로직을 각각 1초, 0.3초, 1초 간격으로 나누어서 실행하여 Tick의 부하를 분산

- 15. 전투 시스템
- AEnemyShooterGun은 플레이어의 움직임을 예측하고 사전에 경고하는 지능형 무기를 사용
- 주요 시스템 및 로직
- 지연 발사 메커니즘: FireGun()이 호출되면 즉시 발사되지 않고, 설정된 AimWarningTime만큼 지연된 후 ExecuteDelayedShot() 함수가 실행되는 2단계 발사 구조, 이를 통해 플레이어에게 대응할 시간을 주는 조준 경고 기능을 구현
- 플레이어 예측 시스템: CalculatePredictedPlayerPosition 함수를 통해 플레이어의 현재 속도(CurrentVelocity)를 기반으로 PredictionTime 후의 미래 위치를 계산하여 조준. 이로 인해 가만히 서 있는 플레이어는 정확히 맞추지만, 움직이는 플레이어는 이동 방향으로 예측하여 사격
- 조준 경고 시스템: bShowAimWarning이 활성화된 경우, 발사 전 예측된 위치까지 AimingLaserMesh라는 UStaticMeshComponent를 사용. SetupLaserTransform 함수는 총구와 목표 지점 사이의 거리를 계산하여 메쉬의 X축 스케일을 동적으로 조절하고, 목표 방향으로 회전시켜 레이저 조준선을 시각적으로 완벽하게 표현
- 명중률 시스템: Accuracy 값(0.0~1.0)에 따라 명중 여부를 확률적으로 결정. 빗나갈 경우 MaxSpreadRadius 내에서, 명중하더라도 약간의 오차 범위 내에서 ApplyAccuracySpread 함수가 최종 탄착군을 형성

- AEnemyGrenade: EnemyGuardian에 의해 사선이 막혔을 때, SuggestProjectileVelocity를 이용해 포물선 궤도를 계산하여 수류탄을 투척
- 물리 기반 투사체 시스템: UProjectileMovementComponent를 사용하여 중력(ProjectileGravityScale)의 영향을 받는 현실적인 포물선 궤적을 그림
- 환경 상호작용: bShouldBounce가 활성화되어 있으며, Bounciness(탄성)와 Friction(마찰) 값을 통해 지형이나 벽에 튕기고 구르는 움직임을 구현
- 시간차 폭발 (Time-Based Fuse): BeginPlay 시점에 FuseTime(기본 3초)으로 설정된 타이머가 시작됩니다. 수류탄은 지형과의 충돌 여부와 관계없이, 이 시간이 지나면 Explode() 함수를 호출하여 폭발
- 발사 로직: AEnemyShooterAIController가 LaunchGrenade 함수를 호출하여 계산된 발사 속도(LaunchVelocity)를 전달하면, ProjectileMovement 컴포넌트가 활성화되어 물리 시뮬레이션을 시작
- 광역 피해 (Area-of-Effect Damage): Explode() 함수는 OverlapMultiByChannel을 사용하여 폭발 지점(ExplosionCenter)을 중심으로 ExplosionRadius 범위 내의 모든 Pawn을 감지합니다. 감지된 플레이어 캐릭터에게 ExplosionDamage 만큼의 피해를 적용
- 쿨다운 관리: 사격과 수류탄은 각각 독립적인 쿨다운 타이머(ShootCooldown, GrenadeCooldown)를 가짐

- 16. 상태별 동작
- Idle: 플레이어가 탐지 범위 밖에 있을 때 대기
- Moving: 사격 또는 진형 유지를 위해 계산된 목표 위치로 이동
- Shooting: 사격 범위 내에 있고 사선이 확보되면 이동을 멈추고 사격 및 수류탄 투척 패턴을 실행
- Retreating: 플레이어가 너무 가까워지면(MinShootDistance 이하) 뒷걸음질치며 거리를 벌림

### 5. EnemyGuardian AI
- 방패(EnemyGuardianShield)의 체력 여부에 따라 아군 슈터 보호와 적 포위의 두 가지 역할을 수행하는 하이브리드형 AI

- 1. AI 상태머신 시스템
- 메인 클래스: AEnemyGuardianAIController - EnemyGuardian의 모든 행동을 제어
- 상태 열거형: 별도의 상태 열거형 대신, AEnemyGuardian의 bIsShieldDestroyed 플래그를 핵심적인 상태 전환 조건으로 사용
- 상태 전환: Tick 함수에서 매번 bIsShieldDestroyed 값을 확인하여, true이면 포위 모드로 false이면 보호 모드로 행동 패턴을 즉시 변경

- 2. 핵심 행동 로직 시스템 (상태 기반)
- 슈터 보호 전략 (방패가 파괴되지 않았을 시)
- 자신에게 가장 가까운 아군 EnemyShooter를 찾아 그 정면으로 이동하여 방패 역할을 수행
- 자신과 같은 슈터를 보호하는 다른 가디언들을 찾아내어, 슈터 정면을 기준으로 좌우로 대칭되게 방어선을 구축.  이 과정에서 각 가디언은 고유 ID 기반의 정렬을 통해 자신의 위치를 스스로 찾아감

- 원형 포위 전략 (방패 파괴 시):
- 슈터 보호를 포기하고 공격적인 EnemyDog처럼 플레이어를 직접 공격하기 위해 포위 진형에 합류
- 각도 분산: 활동 가능한 모든 EnemyGuardian의 수와 자신의 인덱스를 기반으로 360도를 균등 분배하여 포위 위치를 계산

- 3. AI 작동 순서
- 1단계: 공격 가능 여부 확인: Tick의 최우선 순위로 플레이어가 ShieldAttackRadius 또는 BatonAttackRadius 내에 있는지 확인
- 2단계: 상태 판정: 공격 범위 내가 아닐 경우, Guardian->bIsShieldDestroyed 플래그를 확인하여 '보호 모드' 또는 '포위 모드'를 결정
- 3단계:행동실행: 결정된 모드에 따라 PerformShooterProtection() 또는 PerformSurroundMovement() 함수를 호출하여 이동 로직을 수행

- 4. 전투 시스템
- 방패 공격: 방패가 파괴되지 않은 상태에서 플레이어가 150 단위 내로 접근하면 발동
- AEnemyGuardianShield의 Tick 함수가 지속적인 LineTrace를 발생시켜 전방에 피해 영역을 생성
- 진압봉 공격: 방패가 파괴된 후 플레이어가 200 단위 내로 접근하면 발동 
- AEnemyGuardianBaton의 Tick 함수가 지속적인 Sweep 판정을 발생시켜 넓은 범위에 피해를 입힘
- 방패 파괴 및 스턴: 방패가 파괴되면 가디언은 잠시동안 Stun() 상태에 빠져 무방비 상태에 돌입

- 5. 성능 최적화
- 캐싱 시스템: 매 Tick마다 주변 아군을 검색하는 대신, 주기적으로 아군 목록을 갱신하는 캐싱 시스템을 도입하여 성능 부하를 크게 줄임
- 주기적 갱신: BeginPlay 시점에 타이머(AllyCacheUpdateTimerHandle)를 설정하여, AllyCacheUpdateInterval(기본 2초)마다 한 번씩만 UpdateAllyCaches() 함수를 호출
- 데이터 재사용: UpdateAllyCaches() 함수는 월드의 모든 EnemyShooter와 EnemyGuardian을 검색하여 CachedShooters와 CachedGuardians 배열에 저장. Tick 함수 내의 AI 로직은 매번 검색을 수행하는 대신 이 캐시된 데이터를 사용
- 안전한 참조: 캐시된 목록은 TWeakObjectPtr를 사용하여 아군을 참조. 이를 통해 아군이 사망하여 파괴되더라도 포인터가 자동으로 무효화되어 크래쉬를 방지하는 안정성을 확보

- 6. 상태별 동작
- 보호 기동 (Protection): 계산된 보호 위치로 이동하며, 항상 몸은 플레이어를 향해 회전하여 방패로 막을 준비를 함
- 포위 기동 (Surround): 계산된 포위 위치로 이동하며, 항상 플레이어를 향해 회전하여 공격 기회를 엿봄 
- 공격 (Attack): 모든 이동을 멈추고 제자리에서 플레이어를 향해 즉시 회전한 후, 조건에 맞는 공격 애니메이션을 재생

### 6. 라이플 리코일 및 화면 흔들림 시스템
- 사격 시 시각적인 반동과 화면 흔들림을 생성하여 타격감을 강화했습니다.

- 1. 시스템 구조
- 사격 시(FireWeapon) ApplyCameraRecoil() 함수가 호출됩니다.
- ApplyCameraRecoil()은 수직/수평 반동의 목표 값(TargetRecoil)을 랜덤하게 설정하고 , bIsRecoiling 상태를 활성화함과 동시에 ApplyCameraShake()를 호출하여 즉각적인 화면 흔들림을 유발
- Tick 함수는 bIsRecoiling 상태일 때 매 프레임 UpdateRecoil()을 호출
- UpdateRecoil()은 FMath::Vector2DInterpTo를 사용해 현재 반동 값을 목표 값까지 부드럽게 보간하며 AddControllerPitchInput과 AddControllerYawInput으로 카메라를 지속적으로 움직임

- 2. 라이플 화면 흔들림
- ApplyCameraShake() 함수는 Camera 컴포넌트의 상대 위치에 ShakeIntensity를 기반으로 한 랜덤 오프셋을 직접 더함
- ShakeDuration 시간이 지난 후 타이머를 통해 카메라 위치를 원래대로 복구시켜 짧고 강렬한 흔들림을 구현

- 3. 재장전 시 비활성화
- FireWeapon() 함수는 실제 발사 로직 이전에 Rifle->IsReloading()을 확인
- 재장전 중일 경우 함수가 즉시 반환되므로, ApplyCameraRecoil()과 ApplyCameraShake()가 호출되지 않아 반동 및 화면 흔들림이 발생하지 않음

### 7. 다이나믹 크로스헤어 시스템
- 플레이어의 상태에 따라 크로스헤어의 크기와 색상이 실시간으로 변경되어 현재 탄착군 분산도를 시각적으로 표현

- 1. 주요 기능
- 이동 번짐: AMainCharacter의 Tick 함수에서 현재 이동 속도(MovementSpeed)를 계산하여 CrosshairComponent->SetMovementSpread()를 호출함으로써 크로스헤어가 벌어짐
- 격발 번짐: FireWeapon() 함수에서 CrosshairComponent->StartExpansion()을 호출하여 크로스헤어를 즉시 확장
- 연속 사격 번짐: StartExpansion() 함수는 ConsecutiveShotWindow 내의 연속적인 호출을 감지하여 ConsecutiveShots 카운트를 증가시키고, 이를 통해 추가적인 분산도 배율을 적용

- 2. 실제 발사 연관성
- 현재까지는 총알은 항상 카메라 정면(CameraRotation.Vector())으로 발사되며, 시각적인 크로스헤어 번짐과 실제 탄도는 서로 영향을 주지 않는 상태입니다.
크로스헤어 번짐에 따라 실제 탄착군도 동일하게 번질 수 있도록 연구중에 있습니다.

### 8. 라이플 비주얼 이펙트 (VFX)
- 사격 시 총구 섬광과 피격 지점의 임팩트 효과를 통해 시각적 피드백을 강화

- 머즐 플래시
- Fire() 함수에서 MuzzleSocket 위치에 나이아가라 시스템(MuzzleFlash)을 부착하여 스폰
- 생성된 이펙트는 MuzzleFlashDuration 시간이 지난 후 타이머에 의해 StopMuzzleFlash() 함수가 호출되어 안전하게 제거

- 히트 임팩트:
- 총알이 목표에 명중했을 때, ProcessHit 함수 내에서 해당 위치(HitResult.ImpactPoint)에 나이아가라 시스템(ImpactEffect)을 스폰
- 이펙트는 ImpactEffectDuration 시간이 지난 후 타이머를 통해 StopImpactEffect() 함수가 호출되어 정리

### 9. 라이플 헤드샷 데미지 시스템
- 헤드샷 데미지 배율
- ProcessHit 함수는 총알이 맞은 부위의 본(Bone) 이름을 확인
- 본 이름이 "Head" 또는 "CC_Base_Head"일 경우, 기본 데미지(Damage)의 100배에 해당하는 피해를 ApplyPointDamage를 통해 적용

### 10. 이동 속도 제어 시스템
- 기본 이동 속도 설정: 생성자에서 CharacterMovementComponent의 MaxWalkSpeed를 DefaultWalkSpeed(700.0f)로 초기화
- 에임 모드 시 이동 속도 저하:
- UpdateMovementSpeed() 함수는 캐릭터가 조준 중(bIsAiming)이거나 특정 조준 스킬을 사용하는지 확인
- 해당 조건이 참일 경우, MaxWalkSpeed를 AimWalkSpeed(500.0f)로 변경하여 이동 속도를 감소
- 이 함수는 Tick, EnterAimMode, ExitAimMode에서 각각 호출되어 상태가 변경될 때마다 속도를 갱신

### 11. BS 의 이동 애니메이션 에셋 교체
- 기본 이동 애니메이션, 에임모드 이동 애니메이션 전면 교체
- 기존 애니메이션은 지나치게 역동적이여서 상하체 연동 애니메이션 재생 시 부자연스럽게 튀는 현상이 있었음

### 12. 근접 전투 시스템 개선: 자동 조준 및 텔레포트 돌진
- UMeleeCombatComponent에 근접 공격 시 주변의 적을 자동으로 조준하고, 특정 조건 만족 시 적에게 순간이동하여 공격을 이어가는 기능 추가
- 통합 타겟팅 시스템: AdjustAttackDirection 함수는 GetAllActorsOfClass를 통해 월드에 존재하는 모든 종류의 적(Enemy, EnemyDog, EnemyShooter, EnemyGuardian, BossEnemy)을 단일 배열로 통합하여 탐색
- 타겟 선별:
- 거리 우선: MaxAutoAimDistance 범위 내에서 가장 가까운 적을 우선 타겟으로 선정
- 유효성 검사: 각 적 클래스에 맞는 상태 변수(bIsDead, bIsInAirStun 등)를 확인하여, 공격 불가능한 상태의 적은 타겟에서 제외
- 사선 확보: 가장 가까운 적을 찾더라도, 플레이어와 적 사이에 장애물이 있는지 LineTrace를 통해 확인하며 장애물에 가려진 적은 최종 타겟으로 선정되지 않음
- 텔레포트 돌진:
- 조건부 발동: AdjustAttackDirection 함수는 선정된 타겟과의 거리를 측정한 후, ShouldTeleportToTarget 함수를 호출하여 텔레포트 가능 여부를 판단
- 거리 조건: 타겟이 MinTeleportDistance보다는 멀고, TeleportDistance보다는 가까이 있을 때만 텔레포트를 시도
- 상태 조건: 플레이어가 공중에 있거나, bCanTeleport 플래그가 false(쿨다운 상태)일 경우 텔레포트는 발동하지 않음
- 텔레포트 로직:
- TeleportToTarget 함수는 타겟의 정면 TeleportOffset 만큼 떨어진 위치로 즉시 이동(SetActorLocation)함
- 이동 후에는 즉시 타겟을 바라보도록 캐릭터의 회전(SetActorRotation)을 보정하여 다음 공격이 빗나가지 않도록 함
- 방향 보정: 텔레포트 조건이 만족되지 않거나 타겟이 너무 가까울 경우, 텔레포트 대신 캐릭터의 방향만 타겟을 향하도록 즉시 회전시켜주는 기존의 공격 방향 보정만 수행

### 13. 기타 벨런싱 및 버그 수정
- 에임스킬3 투사체 떨어지는 시간 감소 
- 스킬3 투사체가 간헐적으로 앞에서 멈추는 현상 수정: 메쉬를 사용하지 않기에 메쉬 관련 코드 전부 삭제 및 투사체 발사 시작 위치를 더 앞으로 수정하고 투사체 비행속도 증가
- 점프 짧게 연타 시 점프 애니메이션 밀리는 현상 수정: 문제해결노트 35번 참고

## 최종 업데이트 (시연 버전)

### 1. 카메라 충돌
- 카메라와 캐릭터 사이의 지형지물을 감지하여 카메라가 벽을 투과하지 않도록 Spring Arm의 Do Collision Test를 활성화
- 이를 통해 캐릭터가 벽에 밀착한 상태여도 카메라 시작지점(ViewPoint)에서 발생하는 래이캐스트 판정 오류가 없도록 함

### 2. 포스트 프로세싱 기반 사망 연출
- UPostProcessComponent를 직접제어하여 캐릭터 사망 시 화면을 즉각적으로 흑백으로 전환하는 연출을 구현
- 공개 변수인 BlendWeight에 접근하여 효과를 즉시 1.0f (100%) 적용
- Settings.bOverride_ColorSaturation 플래그를 활성화하고 FVector(0.0f, 0.0f, 1.0f) 값을 할당하여 채도를 조정하여 흑백 화면 구현
- Settings.bOverride_VignetteIntensity 플래그를 활성화하고 강도를 0.5f로 설정하여 화면 가장자리를 어둡게 처리하는 비네팅효과를 적용

### 3. 시스템 UI 및 환경 설정(Options) 구축
- GameInstance 기반 전역 설정 관리:
- USettingsGameInstance를 통해 게임 시작부터 종료까지 유지되는 전역 설정 시스템을 구축
- 그래픽, 사운드, 마우스 감도 설정을 개별적으로 적용할 수 있는 기능을 구현
- 상세 그래픽 Scalability 제어:
- UGameUserSettings를 활용하여 그래픽 품질, 안티앨리어싱, 수직 동기화(V-Sync), 창 모드 및 해상도를 C++에서 직접 제어
- 사용자 모니터에서 지원하는 해상도 목록을 동적으로 가져와 UI에 표시하는 기능을 구현
- 사운드 믹스 시스템:
- USoundMix와 USoundClass를 활용하여 마스터 볼륨을 실시간으로 제어하고, 설정값을 파일에 저장하여 재시작 시에도 유지

### 4. 메인 메뉴 및 UX 연출
- C++ 위젯 바인딩 (BindWidget):
- UMainMenuWidget에서 meta = (BindWidget) 매크로를 사용하여 WBP(블루프린트)의 버튼과 C++ 로직을 안전하게 연결
- 로딩 스크린 및 입력 모드 제어:
- 레벨 전환 시 LoadingScreenWidget을 동적으로 생성하고, FInputModeUIOnly를 사용하여 로딩 중 불필요한 입력을 차단
- 일시정지(Pause) 시스템: 
- 게임 도중 ESC를 통해 RESUME(재개), RESTART(재시작), OPTIONS(환경 설정) 메뉴를 사용할 수 있도록 구현
- 이벤트 디스패처 연동:
- 옵션 위젯에서 메인 메뉴로 돌아올 때 자동으로 설정을 저장하고 UI를 갱신하도록 델리게이트 시스템을 구축

### 5. 타격 피드백 및 넉백(Knockback) 시스템
- LaunchCharacter 기반의 수평 넉백 로직:
- 총기 및 스킬 히트 시 LaunchCharacter를 호출하여 피격 대상을 발사 방향으로 밀어내는 물리적 효과를 추가
- 속도 오버라이드(bXYOverride) 적용: 
- LaunchCharacter의 두 번째 매개변수를 true로 설정하여, 대상의 기존 수평 이동 속도를 무시하고 넉백 속도를 즉시 덮어씌움으로써 즉각적인 타격 피드백을 구현
- Z축 고정: 
- 넉백 방향의 Z값을 0으로 정규화하여 공중으로 뜨는 현상을 방지하고 지면을 따라 수평으로만 밀려나도록 설계

### 6. 웨이브 최고 기록 저장 시스템 (SaveGame)
- 웨이브 데이터 영속성 확보:
- UWaveRecordSaveGame 클래스를 설계하여 클리어한 최고 웨이브 인덱스와 해당 웨이브 이름을 로컬 슬롯에 저장
- 기록 자동 업데이트
- 게임 종료 또는 웨이브 클리어 시 현재 기록과 저장된 최고 기록을 비교하여 최신화하는 저장 로직을 구축

### 7. 게임 폴리싱 및 피드백 강화 
- 사운드 시스템 통합 및 설정 연동:
- 게임 내 모든 사운드(공격, 스킬, 발걸음, UI 등)를 Sound Cue로 제작하고 Sound Class에 할당
- 이를 통해 사용자가 옵션 메뉴에서 조절한 마스터 볼륨값이 모든 인게임 사운드에 실시간으로 반영되도록 시스템을 연동
- 거리 기반 사운드 감쇄(Attenuation)를 적용하여 공간감 있는 오디오 환경을 구축
- 시각 효과(VFX) 보완:
- 총구 섬광(Muzzle Flash), 피격 임팩트, 스킬 효과 등 모든 이펙트를 최종 검수하고 안정적으로 노출되도록 최적화
- 맵의 라이팅 설정에 맞춰 이펙트의 과도한 눈부심을 방지하도록 설정값을 조정. 
- 밸런싱 및 마이너 버그 수정:
- 적들의 체력, 공격력, 스폰 간격 등 웨이브 시스템 전반의 수치를 조정하여 긴장감 있는 난이도를 유지
- 시스템 안정화 및 예외 처리 로직 보완 엔진 파라미터 최적화 및 에셋 설정 보정

## 프로젝트 최적화 및 리펙토링

### 1. 런타임 퍼포먼스 개선
- 캐싱 시스템: 빈번한 Cast<T> 연산 오버헤드를 방지하기 위해 참조가 잦은 클래스는 멤버 변수로 캐싱하고 BeginPlay 시점에 초기화하여 런타임 CPU 비용을 최소화
- Tick 최적화: 상시 업데이트가 불필요한 클래스의 PrimaryActorTick.bCanEverTick을 false로 수정하고 불필요한 프레임 소모 최소화
- 가상 함수 호출 최적화: 함수 내 GetWorld() 반복호출을 지양하고 지역 변수 (UWorld* World = GetWorld();) 를 활용하여 연산 효율 증가
- 물리 쿼리 최적화: 래이캐이스트 호출 빈도를 로직에 맞게 조절하여 물리 엔진의 부하를 최소화

### 2. 코드 안정성 및 컴파일 구조 개선
- Early Return 패턴 적용: 함수 구현부 전반에 도입하여 불필요한 else 구문과 if 중첩을 제거하여 코드 가독성을 최대화
- 의존성관리: 헤더 파일의 불필요한 #include를 제거하고 전방 선언을 적용하여 컴파일 속도를 개선하고 순환 참조 오류를 최소화
- SafeTimer: this 를 직접 캡처하는 타이머 람다 식을 모두 약참조 방식으로 전환하여 객체 소멸 후 타이머 실행으로 인한 런타임 크래쉬 최소화
- 리플렉션 시스템 재정비: UPROPERTY 및 UFUNCTION 지정자를 전수 조사하여 누락을 수정하고 블루프린트 노출 범위를 최적화

### 3. 고도화 및 OOP 설계 연구 (진행중)
- TObjectPtr 마이그레이션: 현재는 일부 클래스에만 적용되어 있으며 언리얼 표준에 맞게 로우 포인터를 TObjectPtr로 순차 전환하여 GC 안정성과 디버그 용이성을 확보중
- 데이터 주도 설계: 하드코딩된 수치들을 추출하여 변수화 하고 있으며 UDataAsset을 이용하여 분리하기 위한 목표로 연구중
- 인터페이스 기반 설계: 클래스 간 결합도를 낮추고 SRP를 준수하기 위해 UInterface를 적극적으로 활용 하는 방안을 연구중

### 4. 성능 검증 결과 (stat memory)
- 테스트 환경: 모든 그래픽 설정, 웨이브 설정을 동일하게 하고 5분간 다양한 스킬 연계 및 대규모 AI 웨이브 스폰 및 처치

| 측정항목 | 시연빌드 | 최적화빌드 | 개선결과 |
| --- | --- | --- | --- |
| Proxy Total (Avg) | 3806.00 | 166.00 | 평균부하 95% 감소 |
| Proxy Total (Max) | 7704.00 | 3312.00 | 최대부하 57% 감소 |
| MemStack Large Block | 2.50 MB | 1.25 MB | 메모리 점유 50% 감소 |
| Used Streaming Pool | 187.68 MB (100%) | 159.49MB (100%) | 텍스처 요구량 15% 감소 |

- 1) 프록시 부하 절감 (Proxy Total Avg -95% / Max -57%)
- 리팩토링 전 불규칙하게 치솟는 프록시 부하(Max 7,704)로 인해 CPU-GPU 간 통신 오버헤드가 발생하였으며 고사양 설정에서 GPU 드라이버 크래시 (D3D Device Removed)의 직접적 원인이 되었음
- 빈번한 Cast<T> 연산을 제거하고 멤버 변수 캐싱(Caching) 및 약참조(WeakPtr) 를 적용하여 참조 구조를 단선화
- 결과: 평균 부하를 3,806에서 166으로 95% 절감하여 시스템 생존성을 확보 및 프레임 안정성 확보

- 2) 함수 실행 구조 효율화 (MemStack -50%)
- 복잡한 조건문 중첩으로 인해 함수 실행 시 스택 메모리에 불필요한 데이터가 장시간 점유
- 54개 클래스 전체에 얼리 리턴(Early Return) 패턴을 도입하여 로직의 흐름을 최적화
- 결과: 임시 메모리 스택 점유율을 2.50MB에서 1.25MB로 절감하여 CPU 연산 효율을 극대화

- 3) 리소스 스트리밍 효율 향상 (Streaming Pool -15%)
- 동일 그래픽 사양임에도 필요한 스트리밍 풀 자원을 약 28MB 추가 확보
- C++ 로직 최적화가 가비지 컬렉션(GC)의 효율을 높여 엔진 수준의 리소스 관리가 더 원활하게 작동함을 확인

- 위 수치는 현재 최적화의 중간 결과이며 TObjectPtr 마이그레이션, 데이터 주도 설계·인터페이스 기반 설계가 완료되면 추가적인 개선이 가능할 것으로 보임
- 지속적인 프로파일링과 최적화, 리팩토링을 통해 더 높은 품질의 코드를 목표로 연구 중

### 5. 저사양 PC 대응 (그래픽 기본값 최적화)

- 그래픽 설정 기본값이 Epic(3)으로 설정된 상태로 저사양 환경에서 실행 시 GPU 드라이버 크래시 (D3D Device Removed)가 발생함을 확인
- 가용 VRAM 4160.29MB 환경에서 Lumen Epic 품질의 VRAM 요구량이 가용 예산을 초과하여 GPU 타임아웃 발생
- SettingsSaveGame 생성자의 기본값을 Epic → Medium로 조정하고 창모드로 설정하여 다양한 사양의 PC에서 안정적으로 실행되도록 대응
- 게임 환경이 고사양으로 한정되지 않도록 저사양환경에서도 안정적인 실행을 보장을 목표로 함

- 그래픽 설정 변경 후 크래시 발생 시:
- [게임경로]\Locomotion\Saved\Config\Windows\GameUserSettings.ini
- 파일 삭제 후 재실행하면 기본값으로 복원되어 플레이 가능