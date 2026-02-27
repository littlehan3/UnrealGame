# LOCOMOTION (TPS Action Survival Game)
![C++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=c%2B%2B&logoColor=white)
![Unreal Engine 5.4](https://img.shields.io/badge/Unreal%20Engine-5.4-313131?style=flat-square&logo=unrealengine&logoColor=white)
> **2025년 공주대학교 정보통신공학과 졸업 작품 전시회 시연작**
>
> C++ 와 언리얼엔진 5.4 버전을 이용하여 제작된 실시간 액션 TPS 게임입니다.
> 졸업 전시회에서 성공적으로 시연 및 발표를 마쳤습니다.

---

## 관련 문서

| 문서 | 설명 |
| --- | --- |
| [졸업작품 전시 판넬](https://drive.google.com/file/d/1juNzVw-6LlI4VPA7kYJvu7l4knBZvY0F/view?usp=sharing) | 졸업 전시회에서 사용된 프로젝트 소개 판넬 |
| [문제해결 및 개선 기록 노트](https://drive.google.com/file/d/1RFKRTb3lq0Ar7vT-4p2fAWsxuOvCUs_L/view?usp=sharing) | 36가지 기술적 난관과 데이터 기반 해결 과정을 기록한 기술 백서 |

---

## 목차

- [핵심 시스템](#핵심-시스템)
- [AI 시스템](#ai-시스템)
- [스킬 시스템](#스킬-시스템)
- [UI / UX / 사운드 / VFX](#ui--ux--사운드--vfx)
- [프로젝트 최적화 및 리펙토링](#프로젝트-최적화-및-리펙토링)
- [날짜별 업데이트 기록](#날짜별-업데이트-기록)

---

## 핵심 시스템

<details>
<summary><b>사격 및 레이캐스트 시스템</b></summary>

- 레이캐스트 발사 위치를 총구(Muzzle)에서 카메라 중앙(ViewPoint)으로 변경하여 조준 방향과 탄도를 일치
- `CapsuleComponent`와 `Mesh`의 Collision 설정 수정으로 적 감지 문제 해결
- `ARifle::Fire()`에서 적중 대상이 Enemy인지 판별 후 데미지 적용 (`ProcessHit()`)
- 헤드샷 데미지 시스템: 본(Bone) 이름이 "Head" 또는 "CC_Base_Head"일 경우 기본 데미지의 100배 적용

</details>

<details>
<summary><b>리코일 및 화면 흔들림 시스템</b></summary>

- 사격 시 `ApplyCameraRecoil()` 호출로 수직/수평 반동 목표 값 설정
- `Tick` 함수에서 `FMath::Vector2DInterpTo`를 사용한 부드러운 보간 반동 적용
- `ApplyCameraShake()` 함수를 통한 카메라 위치 랜덤 오프셋 기반 흔들림
- `ShakeDuration` 후 타이머로 카메라 위치 원상 복구
- 재장전 중에는 반동 및 화면 흔들림 비활성화

</details>

<details>
<summary><b>다이나믹 크로스헤어 시스템</b></summary>

- 이동 번짐: 현재 이동 속도에 따라 크로스헤어 실시간 확장
- 격발 번짐: `StartExpansion()` 호출을 통한 즉시 확장
- 연속 사격 번짐: `ConsecutiveShotWindow` 내 연속 호출 감지 및 추가 분산도 배율 적용

</details>

<details>
<summary><b>근접 전투 (콤보 / 히트판정 / 자동조준)</b></summary>

**콤보 공격 시스템**
- 콤보별 차등 데미지 (1콤보: 20, 2콤보: 25, 3콤보: 30, 4콤보 발차기: 35)
- 1.5초 동안 콤보를 이어서 하지 않으면 1타로 초기화
- 4콤보(발차기)는 전용 히트박스(`KickHitBox`)를 `FootSocket_R`에 부착하여 처리

**히트판정 개선**
- 근접공격 시 캐릭터 전방에 레이캐스트를 발사, 히트박스 충돌과 동시 충족 시에만 데미지 적용
- `SweepMultiByChannel`로 변경: 9개의 구체 스윕을 부채꼴 형태로 발사하여 180도 범위 커버
- `TSet<AActor*>`를 통한 중복 데미지 방지

**자동 조준 및 텔레포트 돌진**
- `AdjustAttackDirection` 함수에서 모든 종류의 적을 단일 배열로 통합 탐색
- 거리 우선 타겟 선정 + 사선 확보(LineTrace) 검증
- 조건부 텔레포트: `MinTeleportDistance` ~ `TeleportDistance` 범위 내 적에게 즉시 이동

</details>

<details>
<summary><b>대쉬 시스템</b></summary>

- WASD 방향 입력 기반, 카메라 방향과 독립적인 직관적 대쉬
- `DotProduct`를 활용한 전/후/좌/우/대각선 방향 판별 및 애니메이션 적용
- 대쉬 중 근접 공격 불가, 사격 가능 / 점프 중 대쉬 불가 / 대쉬 중 점프 불가
- 에임 모드 중 대쉬 후 원래 조준 방향으로 자동 복귀

</details>

<details>
<summary><b>에임 모드 / 카메라 시스템</b></summary>

- 에임 모드 시 `FRotator ControlRotation` 기반 이동 방향 조절
- `AimPitch` 값을 활용한 부드러운 상하 조준 애니메이션
- 에임 모드 전환 시 보간을 적용한 부드러운 카메라 전환
- 마우스 휠 줌인/줌아웃 (`FMath::FInterpTo` 보간), 에임 모드 해제 시 마지막 줌 값 유지
- Spring Arm의 `Do Collision Test` 활성화로 카메라 벽 투과 방지
- 에임 모드 시 이동 속도 저하 (700 → 500)

</details>

<details>
<summary><b>웨이브 시스템</b></summary>

- `AMainGameModeBase` 기반 웨이브 제어 및 스폰 관리
- 10초 간격 `ProcessSpawnStep` 기반 점진적 스폰 패턴 (1명 → 2명 → ... → 5명)
- 보스 등장 시 일반 적 즉시 처리 및 메모리 정리 후 1:1 구도 진입
- 보스 사망 후 다음 웨이브 레벨 (증가된 체력, 이동속도)
- `UWaveRecordSaveGame` 기반 최고 기록 저장 시스템

</details>

<details>
<summary><b>이동 속도 제어 / 넉백 시스템</b></summary>

**이동 속도 제어**
- `UpdateMovementSpeed()` 함수로 상태에 따른 동적 속도 변경 (기본 700, 에임 500)

**넉백 시스템**
- `LaunchCharacter` 기반 수평 넉백 (`bXYOverride = true`로 즉각적 피드백)
- Z축 0 정규화로 공중 부양 방지, 지면 수평 이동만 허용

</details>

---

## AI 시스템

<details>
<summary><b>Enemy AI (기본 적)</b></summary>

**기본 행동**
- `AEnemyAIController`에서 `DetectionRadius` 내 플레이어 감지 시 추적
- `MoveToActor`를 활용한 네비게이션 시스템 기반 이동

**공격 패턴**
- 일반 공격 → 3회 후 강공격 자동 전환
- 점프 공격: 감지 범위 진입 시 실행, 루트 모션 기반 이동
- 30% 확률 좌/우 닷지 (쿨다운 적용)

**히트 및 사망 처리**
- 총기/칼 공격 시 히트 애니메이션 호출, 사망 시 AI 완전 중지
- 사망 상황별 다른 애니메이션 (공중 사망, 일반 사망)
- 사망 후 일정 시간 뒤 무기와 함께 사라짐

**노티파이 기반 타이밍 제어**
- `UAnimNotify_EnemyStartAttack` / `UAnimNotify_EnemyEndAttack`로 정확한 공격 타이밍 제어
- 카타나 `Tick`에서 `PerformRaycastAttack()` 실행 (5단계 구체 스윕, 반지름 30)

**포위 로직**
- 3가지 상태: `Idle`, `MoveToCircle`, `ChasePlayer`
- 플레이어 중심 반지름 200 원형 포위, `StaticAngleOffset`으로 겹침 방지
- 성능 최적화: AI 업데이트 60fps → 20fps, 회전 보간 10fps, 원형 위치 2초 간격 재계산

**엘리트 Enemy**
- 10% 확률 생성, 체력 2배(200), 이동속도 증가(500), 공격 데미지 상승
- 모든 공격 몽타주 1.5배 속도 재생
- 전용 등장 애니메이션, ABP 연동 블렌드스페이스 분기

</details>

<details>
<summary><b>Boss AI</b></summary>

**핵심 시스템**
- 클래스 구조: `ABossEnemy`, `ABossEnemyAIController`, `AEnemyBossKatana`, `ABossProjectile`
- 상하체 분리 시스템: `bUseUpperBodyBlend` 플래그로 이동 중 독립적 상체 공격
- 무적 시스템: 텔레포트 및 스텔스 특정 구간에서 `bIsInvincible` 활성화
- 상태: `Idle`, `MoveToPlayer`, `NormalAttack` 3가지 핵심 상태

**텔레포트 시스템**
- 후퇴 텔레포트: 플레이어 반대 방향 이동 → 원거리 공격 또는 공격형 텔레포트 연계
- 공격 텔레포트: 플레이어 등 뒤 사각지대로 기습 이동 후 즉시 공격

**스텔스 공격 (6단계 시퀀스)**
1. `StealthStartMontage` 재생
2. `StealthDiveMontage` 재생 (지면 파고들기)
3. `SetActorHiddenInGame(true)` 투명화 + 5초간 플레이어 주변 위치 갱신 추적
4. 마지막 갱신 위치로 즉시 텔레포트
5. 킥 공격 (`LineTrace` 판정, `LaunchCharacter`로 플레이어 공중 부양 + 중력 0)
6. 피니시 연계 공격

**공격 패턴 우선순위**
```
1순위: 스텔스 공격 (300~600 거리, 50% 확률)
2순위: 거리별 일반 공격
  ├─ 0~200: 후퇴텔레포트(50%) vs 전신공격(50%)
  ├─ 201~250: 상체분리 이동공격
  └─ 251+: 걸어서 추격
```

**투사체 공격**
- `ABossProjectile`: 직선 비행 → 충돌 시 `ApplyAreaDamage()` 광역 피해

**메모리 관리**
- 사망 시 `HideBossEnemy()`로 무기, AI 컨트롤러, 타이머, 델리게이트 안전 정리
- `SetTimerForNextTick`으로 다음 프레임에 액터 파괴

</details>

<details>
<summary><b>EnemyDog AI</b></summary>

- 2가지 상태: `Idle`, `ChasePlayer`
- 플레이어 중심 반지름 100 원형 포위
- `TActorIterator`로 실시간 아군 수 파악, 360도 균등 분배 각도 할당
- 150 단위 이하 접근 시 `NormalAttack` 실행
- 틱 빈도 0.05초 제한, `FMath::RInterpTo` 회전 보간

</details>

<details>
<summary><b>EnemyDrone AI (비행형)</b></summary>

- 플레이어 중심 반지름 500, 높이 300 궤도 비행
- `LineTrace` 기반 장애물 회피 (아군 드론 충돌 시 고도 변경, 지형 충돌 시 궤도 방향 반전)
- 끼임 탈출: `MaxStuckTime` 초과 시 고도 급상승
- `FMath::VInterpTo` 보간 이동, 틱 주기 0.2초 제한
- 독립적인 미사일 발사 시스템 (`MissileCooldown` 타이머)

</details>

<details>
<summary><b>EnemyShooter AI (원거리)</b></summary>

- 5가지 상태: `Idle`, `Detecting`, `Moving`, `Shooting`, `Retreating`
- 반지름 600 원형 진형 유지, 아군 최소 180 단위 이격
- `LineTrace` 사선 확보 시스템: 아군에 의해 막히면 자리 변경, `EnemyGuardian`에 의해 막히면 수류탄 투척

**지능형 무기 (`AEnemyShooterGun`)**
- 2단계 발사: `AimWarningTime` 지연 후 `ExecuteDelayedShot()` 실행
- 플레이어 예측 시스템: 현재 속도 기반 미래 위치 계산 조준
- `AimingLaserMesh` 기반 시각적 조준 경고선
- `Accuracy` 값(0.0~1.0)에 따른 확률적 명중/탄퍼짐

**수류탄 (`AEnemyGrenade`)**
- `SuggestProjectileVelocity` 포물선 궤도 계산
- 물리 기반 투사체 (중력, 탄성, 마찰), 3초 시간차 폭발

**성능 최적화**
- 캐싱: 아군 목록 1초, 사선 확인 0.3초, 진형 위치 1초 주기 갱신
- 틱 주기 0.2초 제한

</details>

<details>
<summary><b>EnemyGuardian AI (방패병)</b></summary>

**이중 역할 시스템**
- 방패 건재 시: 가장 가까운 `EnemyShooter` 정면에서 방패 역할 수행, 다수 가디언 좌우 대칭 방어선 구축
- 방패 파괴 시: 포위 모드 전환, `EnemyDog`처럼 플레이어 직접 공격

**전투**
- 방패 공격: 150 단위 내 접근 시 `LineTrace` 판정
- 진압봉 공격: 방패 파괴 후 200 단위 내 접근 시 `Sweep` 판정
- 방패 파괴 시 `Stun()` 상태 돌입

**성능 최적화**
- `TWeakObjectPtr` 기반 안전한 캐시 참조
- `AllyCacheUpdateInterval` (2초) 주기 아군 목록 갱신

</details>

<details>
<summary><b>Enemy AI 포위 / 상태 관리 공통</b></summary>

- `FVector::DistSquared()` 사용으로 제곱근 계산 최적화
- `ProjectPointToNavigation()` 네비게이션 보정
- `CachedTargetLocation` 캐싱으로 불필요한 재계산 방지
- `bIsAttacking` 플래그로 공격 중 닷지 방지
- 전투 종료 시 공격 카운트 초기화
- 스폰 즉시 인트로 몽타주 실행 (재생 중 피해 면역)

</details>

---

## 스킬 시스템

<details>
<summary><b>근접 모드 스킬</b></summary>

**스킬1 - 공중 스턴**
- 주변 모든 적을 일정 시간 동안 공중에 띄움 (행동 불가, 스턴 애니메이션)
- `EnterInAirStunState()` 호출, 사격/추가 스킬과 연계 가능

**스킬2 - 도약 광역 타격**
- 도약 후 `SphereTrace`로 범위 내 적 감지 및 광역 데미지
- 스킬1과의 연계를 고려한 설계

**스킬3 - 투사체 발사**
- `Skill3Projectile` 클래스: 전방 투사체 발사, 적/벽 충돌 시 폭발
- `TSet<AActor*>` 중복 타격 방지, 나이아가라 폭발 이펙트

</details>

<details>
<summary><b>에임 모드 스킬</b></summary>

**에임스킬1 - 기관총 연사**
- `MachineGun` 클래스: `FireRate` 간격 자동 연사, `GetFireDirectionWithSpread()` 무작위 탄퍼짐
- ABP 연동 상체 `AimPitch` 회전, Start→Loop 무한반복
- 스킬 사용 시 자동 무기 수납, 이동/점프/다른 스킬 차단

**에임스킬2 - 캐논 투사체**
- `Cannon` 클래스 + `AimSkill2Projectile` 클래스
- 고각발사: 지정 고도 도달 후 `FindClosetEnemy()`로 유도
- 저각발사: 비유도 폭발형 투사체
- 폭발 후 지속 영역 효과: `ApplyPersistentEffects()` (0.1초마다 끌어당기기), `ApplyPeriodicDamage()` (지속 데미지)
- `EndPlay()`에서 모든 타이머/오디오 정리로 메모리 누수 방지

**에임스킬3 - 공중 폭격**
- `AAimSkill3Projectile`: 5개 투사체를 일정 간격 배치, 3초 후 하늘(+2000)에서 낙하
- 하늘색 원으로 6초간 낙하 예정 지점 표시 (반지름 300)
- 지면 높이 자동 보정, 충돌 지점 반지름 150 내 광역 피해 (기본 60 데미지)

</details>

<details>
<summary><b>스킬 관련 무기 관리</b></summary>

- `MachineGun` / `Cannon`은 1회 생성 후 월드에 유지
- `SetActorHiddenInGame(true/false)`로 표시/숨김 전환 (Spawn/Destroy 반복 회피)
- GC 부하 최소화, 스킬 사용 시 지연 없는 즉시 재활용

</details>

---

## UI / UX / 사운드 / VFX

<details>
<summary><b>시스템 UI 및 환경 설정</b></summary>

- `USettingsGameInstance` 기반 전역 설정 (그래픽, 사운드, 마우스 감도)
- `UGameUserSettings` 활용 Scalability 제어 (그래픽 품질, AA, V-Sync, 창 모드, 해상도)
- 모니터 지원 해상도 동적 목록 표시
- `USoundMix` / `USoundClass` 기반 마스터 볼륨 실시간 제어 및 파일 저장

</details>

<details>
<summary><b>메인 메뉴 및 UX 연출</b></summary>

- `BindWidget` 매크로로 WBP 버튼과 C++ 로직 안전 연결
- 레벨 전환 시 `LoadingScreenWidget` 동적 생성, `FInputModeUIOnly`로 입력 차단
- ESC 일시정지 메뉴 (RESUME, RESTART, OPTIONS)
- 이벤트 디스패처 연동: 옵션 → 메인 메뉴 복귀 시 자동 설정 저장 및 UI 갱신

</details>

<details>
<summary><b>포스트 프로세싱 사망 연출</b></summary>

- `UPostProcessComponent` 직접 제어
- `ColorSaturation` = `FVector(0, 0, 1)` → 흑백 전환
- `VignetteIntensity` = 0.5f → 화면 가장자리 어둡게 처리

</details>

<details>
<summary><b>사운드 시스템</b></summary>

- 모든 인게임 사운드를 Sound Cue → Sound Class로 체계화
- 마스터 볼륨 옵션 연동 (실시간 반영)
- 거리 기반 사운드 감쇄(Attenuation) 적용

</details>

<details>
<summary><b>VFX (시각 효과)</b></summary>

**라이플 VFX**
- 총구 섬광: `MuzzleSocket` 위치에 나이아가라 시스템 부착, `MuzzleFlashDuration` 후 제거
- 히트 임팩트: `HitResult.ImpactPoint`에 나이아가라 시스템 스폰, 타이머 기반 정리

**근접공격 이펙트**
- Blender로 원형 → 반원 메쉬 재모델링 (전방 180도 범위 일치)
- 나이아가라 시스템에 `User_ForwardVector`, `User_RightVector` 파라미터 바인딩
- 오브젝트 풀링(`ENCPoolMethod::AutoRelease`) 성능 최적화

</details>

---

## 프로젝트 최적화 및 리펙토링

<details>
<summary><b>런타임 퍼포먼스 개선</b></summary>

- **캐싱 시스템**: 빈번한 `Cast<T>` 연산 오버헤드를 방지하기 위해 멤버 변수로 캐싱, `BeginPlay` 시점 초기화
- **Tick 최적화**: `PrimaryActorTick.bCanEverTick = false` 설정으로 불필요한 프레임 소모 최소화
- **가상 함수 호출 최적화**: `GetWorld()` 반복호출 → 지역 변수(`UWorld* World`) 활용
- **물리 쿼리 최적화**: 레이캐스트 호출 빈도를 로직에 맞게 조절

</details>

<details>
<summary><b>코드 안정성 및 컴파일 구조 개선</b></summary>

- **Early Return 패턴**: 불필요한 else 구문과 if 중첩 제거, 코드 가독성 극대화
- **의존성 관리**: 불필요한 `#include` 제거 + 전방 선언 적용 → 컴파일 속도 개선, 순환 참조 방지
- **SafeTimer**: `this` 직접 캡처 타이머 람다를 약참조 방식으로 전환 → 객체 소멸 후 크래시 방지
- **리플렉션 시스템 재정비**: `UPROPERTY` / `UFUNCTION` 지정자 전수 조사, 블루프린트 노출 범위 최적화

</details>

<details>
<summary><b>성능 검증 결과 (stat memory)</b></summary>

> 테스트 환경: 동일 그래픽/웨이브 설정, 5분간 스킬 연계 및 대규모 AI 웨이브 스폰·처치

| 측정항목 | 시연빌드 | 최적화빌드 | 개선결과 |
| --- | --- | --- | --- |
| Proxy Total (Avg) | 3806.00 | 166.00 | **평균부하 95% 감소** |
| Proxy Total (Max) | 7704.00 | 3312.00 | **최대부하 57% 감소** |
| MemStack Large Block | 2.50 MB | 1.25 MB | **메모리 점유 50% 감소** |
| Used Streaming Pool | 187.68 MB (100%) | 159.49 MB (100%) | **텍스처 요구량 15% 감소** |

- **프록시 부하 절감**: 빈번한 `Cast<T>` 제거 + 멤버 변수 캐싱 + 약참조(`WeakPtr`) 적용 → 평균 부하 95% 절감, GPU 드라이버 크래시 원인 제거
- **함수 실행 구조 효율화**: 54개 클래스 전체에 Early Return 패턴 도입 → 스택 메모리 점유 50% 절감
- **리소스 스트리밍 효율 향상**: C++ 로직 최적화로 GC 효율 증가 → 약 28MB 추가 스트리밍 풀 확보

</details>

<details>
<summary><b>고도화 및 OOP 설계 연구 (진행중)</b></summary>

- **TObjectPtr 마이그레이션**: 로우 포인터 → `TObjectPtr` 순차 전환 (GC 안정성, 디버그 용이성)
- **데이터 주도 설계**: 하드코딩 수치 변수화, `UDataAsset` 분리 연구중
- **인터페이스 기반 설계**: `UInterface` 활용으로 클래스 간 결합도 최소화, SRP 준수 연구중

</details>

<details>
<summary><b>저사양 PC 대응</b></summary>

- 그래픽 기본값 Epic → Medium 조정, 창모드 설정
- VRAM 부족 환경에서의 GPU 드라이버 크래시 방지
- 크래시 발생 시 `GameUserSettings.ini` 삭제로 기본값 복원 가능

</details>

<details>
<summary><b>메인캐릭터 클래스 리펙토링 (SRP)</b></summary>

기존 `AMainCharacter` 클래스가 2천 줄에 달하며 모든 로직을 담당하던 구조를 단일 책임 원칙(SRP)에 따라 분리:

| 클래스 | 역할 |
| --- | --- |
| `AMainCharacter` | 이동/점프/대쉬/에임 등 기본 조작, 입력 바인딩, 무기 관리, 상태 관리 |
| `USkillComponent` | 모든 스킬(액티브/에임) 사용 조건, 쿨타임, 효과, 애니메이션, 무기 연동 |
| `UMeleeCombatComponent` | 콤보 공격, 히트박스 제어, 데미지 적용, 공격 방향 보정, 텔레포트 돌진 |

- 각 컴포넌트가 독립적 책임 → 유지보수성, 확장성, 테스트 용이성 향상

</details>

---

## 날짜별 업데이트 기록

<details>
<summary><b>2025-02-13 업데이트</b></summary>

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

</details>

<details>
<summary><b>2025-02-15 업데이트</b></summary>

### 1. 콤보 공격 히트 판정
- Knife(좌/우)와 KickHitBox를 활용하여 적절한 콜리전 설정
- 히트박스는 공격 타이밍에 맞춰 `EnableHitBox()`, `DisableHitBox()`로 제어

### 2. 콤보별 데미지 적용
- 콤보별로 서로 다른 데미지 적용 (1콤보: 20, 2콤보: 25, 3콤보: 30, 4콤보 발차기: 35)

### 3. 발차기 공격(4콤보) 소켓 적용
- 발차기 전용 히트박스(KickHitBox)를 생성하고, `FootSocket_R`을 활용하여 처리
- `EnableKickHitBox()`, `DisableKickHitBox()`를 통해 활성화/비활성화

### 4. 히트 판정 최적화
- `ComboAttack()`에서 4콤보(발차기) 시 칼 히트박스 활성화 제외
- `AKnife::EnableHitBox()`에서 4콤보는 예외 처리하여 히트박스 활성화 방지
- KickHitBox는 `OnKickHitBoxOverlap()`을 통해 데미지 적용

</details>

<details>
<summary><b>2025-02-19 업데이트</b></summary>

### 1. 락온 시스템 구현
- 근접공격이 가능한 상태일때만 카메라가 자동으로 적을 바라보도록함
- `UpdateLockOnRotation()`을 통해 플레이어의 회전을 타겟 방향으로 보간
- 락온 후 일정 거리 이상 멀어지면 자동 해제

### 2. EnemyAI 기본 이동 및 플레이어 추적 구현
- `AEnemyAIController` 추가
- DetectionRadius 내에서 플레이어 감지 시 추적, StopChasingRadius를 벗어나면 추적 중단
- `MoveToActor(PlayerPawn, 5.0f)`를 활용한 네비게이션 시스템 이동
- 불필요한 연산 최소화, NavMesh 미존재 시 경로 탐색 중단

</details>

<details>
<summary><b>2025-02-23 업데이트</b></summary>

### 1. EnemyAI 일반 공격 구현
- 공격 범위 내 NormalAttack 함수 호출, 공격 쿨타임 적용

### 2. EnemyAI 강 공격 구현
- 일반 공격 3회 후 StrongAttack 실행, 일반 공격 횟수 초기화

### 3. EnemyAI 닷지 로직 구현
- 일정 확률(DodgeChance)에 따라 좌/우 닷지, 쿨다운 적용

### 4. EnemyAI 점프 공격 구현
- 감지 범위 진입 시 JumpAttack 실행, 루트 모션 기반 이동
- 범위 이탈 시 재사용 가능 상태 리셋

</details>

<details>
<summary><b>2025-02-26 업데이트</b></summary>

### 1. EnemyAI 오류 수정
- 공격 중 닷지 실행 방지: `bIsAttacking` 플래그 도입
- 일반 공격 카운트 누적 문제: 감지범위 밖 이탈 시 카운트 초기화

### 2. EnemyAI 히트 및 사망에 따른 AI 중지기능
- 총기/칼 공격 시 히트 애니메이션 호출
- 사망 시 사망 애니메이션 후 AI 행동/움직임/회전 완전 중지

### 3. LockOnComponent 개선
- 사망한 EnemyAI는 락온 대상에서 제외

</details>

<details>
<summary><b>2025-03-01 업데이트</b></summary>

### 1. Knife 히트 오류 수정
- 뒤에 있는 적 히트 / 공격 없이 히트박스 충돌 시 데미지 적용 문제 해결
- 근접공격 즉시 전방 레이캐스트 발사, 레이캐스트 + 히트박스 동시 충족 시에만 데미지 적용
- 발차기 공격도 동일하게 적용

</details>

<details>
<summary><b>2025-03-08 업데이트</b></summary>

### 1. 대쉬 시스템 구현
- WASD 방향 입력 기반 대쉬, DotProduct 기반 방향 판별

### 2. 대쉬 관련 오류 수정 및 최적화
- 대쉬 방향 오류, `SetActorRotation()` 타이밍 수정, 애니메이션 속도 조정 (1.2배속)

### 3. 에임모드 시 애니메이션 방향 개선
- Tick()에서 카메라 방향과 이동 방향 동적 조절

### 4. [긴급 패치] 에임 모드 회전 문제 수정
- 이동하지 않은 상태에서 에임 모드 전환 시 캐릭터 미회전 문제 해결

### 5. 콤보 리셋 / 줌인·줌아웃 / 에임 전환 보간 / 대쉬·점프 상호 제한

</details>

<details>
<summary><b>2025-03-15 업데이트</b></summary>

### 1. 락온 기능 제거
- 다수 적 게임 특성상 부적합, 대쉬/점프 방향 오류 지속 발생

### 2. 근접 공격 보정 기능 추가
- `AdjustComboAttackDirection()`에서 가장 가까운 적 감지 후 방향 조정

### 3. 근접 모드 스킬1 (공중 스턴) 추가

### 4. 상황별 적 사망 애니메이션 적용

### 5. 적 사망 시 일정 시간 후 사라지는 기능 추가

</details>

<details>
<summary><b>2025-03-25 업데이트</b></summary>

### 1. 근접 모드 스킬2 (도약 광역 타격) 추가

### 2. 근접 모드 스킬3 (투사체 발사) 추가
- `Skill3Projectile` 클래스: 전방 투사체 발사, 충돌 시 폭발

### 3. 기타 사운드 시스템 추가
- 스킬/기본 공격/발걸음 사운드, 어테뉴에이션 오버라이드 거리 기반 볼륨 감소

</details>

<details>
<summary><b>2025-04-22 업데이트</b></summary>

### 1. 메인캐릭터 클래스 리펙토링 (SRP 적용)
- `AMainCharacter` → `USkillComponent` + `UMeleeCombatComponent` 분리

### 2. 에임모드 스킬1 (기관총 연사) + MachineGun 클래스 추가

### 3. 에임모드 스킬2 (캐논 투사체) + Cannon / AimSkill2Projectile 클래스 추가

### 4. 근접공격 시각효과 개선
- Blender 반원 메쉬 재모델링, 나이아가라 방향 파라미터 바인딩, 오브젝트 풀링

### 5. 스킬 무기 관리 최적화
- Destroy 대신 `SetActorHiddenInGame` 사용으로 메모리 효율 향상

</details>

<details>
<summary><b>2025-06-01 업데이트</b></summary>

### 1. 일반공격 히트판정 개선
- `SweepMultiByChannel`로 변경: 9개 구체 스윕, 부채꼴 180도 범위

### 2. 에임스킬2 투사체 로직 개선
- 폭발 후 지속 영역 효과 (끌어당기기 + 지속 데미지)
- `EndPlay()` 메모리 누수 방지

### 3. 점프 공격(실험적) 추가

</details>

<details>
<summary><b>2025-07-24 업데이트</b></summary>

### 1. 점프공격 몽타주 스킵현상 (미해결)

### 2. 카메라 줌 개선
- PreviousZoom 추가: 에임 해제 시 마지막 줌 값으로 복귀

### 3. 에임스킬3 (공중 폭격) + AimSkill3Projectile 클래스 추가

### 4. Enemy 공격 판정 추가
- 노티파이 기반 타이밍 제어, 다중 스윕 레이캐스트, 공격 타입별 차등 데미지

### 5. 엘리트 Enemy 추가 (10% 확률, 2배 체력, 1.5배 데미지)

### 6. Enemy AI 포위로직 추가 (원형 포위, 각도 분산, 성능 최적화)

### 7. Enemy 닷지 거리 증가 (루트모션 + LaunchCharacter 혼합)

### 8. Enemy/EnemyBoss 사망 시 메모리 최적화

### 9. 웨이브 시스템 구현 및 프레임 드랍 최적화

### 10. EnemyBoss 추가 (다양한 공격 패턴, 스텔스, 텔레포트)

### 11. 맵 적용

</details>

<details>
<summary><b>2025-10-18 업데이트</b></summary>

### 1. 보스 (Boss) AI 완성
- 상하체 분리, 텔레포트(후퇴/공격), 무적 시스템, 스텔스 6단계 시퀀스
- 투사체 공격, 카타나 스윕 판정

### 2. EnemyDog AI
- 플레이어 포위 + 근접 공격, TActorIterator 실시간 포위 위치 조정

### 3. EnemyDrone AI (비행형)
- 궤도 비행, 장애물 회피, 끼임 탈출, 독립적 미사일 시스템

### 4. EnemyShooter AI (원거리)
- 진형 유지, 사선 확보, 지능형 무기 (예측 조준, 레이저 경고, 명중률), 수류탄

### 5. EnemyGuardian AI (방패병)
- 이중 역할: 슈터 보호 / 포위 공격, 방패 파괴 시 스턴 → 전투 모드 전환

### 6. 라이플 리코일 및 화면 흔들림 시스템

### 7. 다이나믹 크로스헤어 시스템

### 8. 라이플 VFX (머즐 플래시, 히트 임팩트)

### 9. 라이플 헤드샷 데미지 시스템

### 10. 이동 속도 제어 시스템

### 11. BS 이동 애니메이션 에셋 교체

### 12. 근접 전투 시스템 개선 (자동 조준, 텔레포트 돌진)

</details>

<details>
<summary><b>최종 업데이트 (시연 버전)</b></summary>

### 1. 카메라 충돌 (Spring Arm Do Collision Test)
### 2. 포스트 프로세싱 기반 사망 연출 (흑백 전환 + 비네팅)
### 3. 시스템 UI 및 환경 설정 (그래픽, 사운드, 해상도)
### 4. 메인 메뉴 및 UX 연출 (로딩 스크린, 일시정지, 이벤트 디스패처)
### 5. 타격 피드백 및 넉백 시스템
### 6. 웨이브 최고 기록 저장 시스템 (SaveGame)
### 7. 게임 폴리싱 및 피드백 강화 (사운드, VFX, 밸런싱)

</details>

<details>
<summary><b>프로젝트 최적화 및 리펙토링 업데이트 (시연 이후)</b></summary>

### 1. 런타임 퍼포먼스 개선
- 빈번한 `Cast<T>` 연산 → 멤버 변수 캐싱 + `BeginPlay` 초기화
- `PrimaryActorTick.bCanEverTick = false` 설정으로 불필요한 Tick 제거
- `GetWorld()` 반복호출 → 지역 변수(`UWorld* World`) 활용
- 레이캐스트 호출 빈도를 로직에 맞게 조절

### 2. 코드 안정성 및 컴파일 구조 개선
- 54개 클래스 전체에 Early Return 패턴 도입
- 불필요한 `#include` 제거 + 전방 선언 적용 → 컴파일 속도 개선, 순환 참조 방지
- `this` 직접 캡처 타이머 람다를 약참조 방식으로 전환 (SafeTimer)
- `UPROPERTY` / `UFUNCTION` 지정자 전수 조사 및 재정비

### 3. 성능 검증 결과 (stat memory)
| 측정항목 | 시연빌드 | 최적화빌드 | 개선결과 |
| --- | --- | --- | --- |
| Proxy Total (Avg) | 3806.00 | 166.00 | 평균부하 95% 감소 |
| Proxy Total (Max) | 7704.00 | 3312.00 | 최대부하 57% 감소 |
| MemStack Large Block | 2.50 MB | 1.25 MB | 메모리 점유 50% 감소 |
| Used Streaming Pool | 187.68 MB | 159.49 MB | 텍스처 요구량 15% 감소 |

### 4. 메인캐릭터 클래스 리펙토링 (SRP)
- `AMainCharacter` → `USkillComponent` + `UMeleeCombatComponent` 분리
- 2천 줄 단일 클래스 → 단일 책임 원칙에 따른 컴포넌트 분리

### 5. 고도화 및 OOP 설계 연구 (진행중)
- `TObjectPtr` 마이그레이션, `UDataAsset` 데이터 주도 설계, `UInterface` 기반 설계

### 6. 저사양 PC 대응
- 그래픽 기본값 Epic → Medium 조정, VRAM 부족 환경 GPU 크래시 방지

</details>
