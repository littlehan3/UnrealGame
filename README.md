# LOCOMOTION (TPS Action Survival Game)
![C++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=c%2B%2B&logoColor=white)
![Unreal Engine 5.4](https://img.shields.io/badge/Unreal%20Engine-5.4-313131?style=flat-square&logo=unrealengine&logoColor=white)
> **2025년 공주대학교 정보통신공학과 졸업 작품 전시회 시연작**
> 
> C++ 와 언리얼엔진 5.4 버전을 이용하여 제작된 실시간 액션 TPS 게임입니다.
> 졸업 전시회에서 성공적으로 시연 및 발표를 마쳤습니다.
>
> 더블 점프와 역동적인 기동 시스템, 6종의 특화 스킬로 근·원거리를 넘나들며 한계를 시험하는 기록 갱신형 TPS 서바이벌 게임입니다.

---

## 관련 문서

| 문서 | 설명 |
| --- | --- |
| [플레이 영상(4분)](https://www.youtube.com/watch?v=pneKAQoV7Fg) | 자막 및 설명이 없는 플레이 영상
| [기능 설명 영상(11분)](https://youtu.be/nkGwWj_yPy4) | 게임의 기능에 대한 설명이 담긴 영상
| [졸업작품 전시 판넬](https://drive.google.com/file/d/1juNzVw-6LlI4VPA7kYJvu7l4knBZvY0F/view?usp=sharing) | 졸업 전시회에서 사용된 프로젝트 소개 판넬 |
| [문제해결 및 개선 기록 노트](https://drive.google.com/file/d/1xz6mUJ4xct0AHBG25TouJpRpg-kXSh8e/view?usp=sharing) | 졸업 작품 출품까지의 개발 기간 동안 발생한 36가지 문제 해결 및 개선 기록 노트 |
| [최적화 및 리펙토링 보고서](https://drive.google.com/file/d/19-7WQJbWdH-FLJ1KRGbyTvw9noyQnH2h/view?usp=sharing) | 시연 이후 코드 품질 향상과 게임의 안정성을 확보한 대표 사례 및 성과 보고서 |

---

## 시스템 아키텍처

> 게임 클래스 전체의 흐름과 관계를 보여주는 다이어그램

[시스템 아키텍처](https://mermaid.live/view#pako:eNqVV1tr40YU_itCsJCFbIgvSRxDCpbsVYyrEKwuhspLmbXG9jTSjJHGbd0Q2LYp9KGPW9qHZelPcEtp9zfZ3v_QM7p6dDG7erDmfOebc5uLju_VCXOw2lZnPlrMlS-0MVXgCZavIsBAHtaZj-2xKoaKGI_VlxFLPIap2SYiVGhNMKWhAO-pLaNvW5hzQmeBoPRpwBGdJBRMnTHNubTQN9haBRx74FQISiRJbkdDy7BHoBziCfMdQRPm9z0DIfGcU5d6hbAc5DKKX_SF31RUXvQlz6Y5ChM2MV2OiDPD_IDVWxetsJ9mE4ll-Zh6aFSfIx9NOPZfJqZKzOnMW2TGhARhUh5IBsWjX-u27rMguEbET3kF0igjSQllsUFw2MWwEbxXiFcZsnTbuiOuW-mo07f1ZcCZ16HEy22E5LnWdPsaI5fPNVQa8l6VS0ozwghmBFl1YqBQmqE9JFM3731gDyiZ5lHTgLWZzAnFxpLmk7J1RCmjHx3hrc--xhNOXLwX5R5YiLRj1W_tDvHC2tYzZoHWyGiNSppgHaCEsVdt5h7F3irdy6EUb2XliHgLF3tiFyrR8vUp7OIpmuCnUkaZMbgoyATshO_IdiH3nh3ieXQQwQMEOwjllbDLQm2nrzPKfea64WmSOTQhle_EqiXsaXBORMzwqghZ69lCWxq3FgcuCKXBayL6dPqBDLQwhYxZfaC025BWvdplaXbZDLKE36p16UaJAKOgSeoPukNL0E3XQBA_cRm6PlwLIkLxroxxGNsXpKIyjVOoD0Zq7hFNEgQfWURjiXyHICo-nfGwKlQj8pDQCmpN1muIsyLJkknWnGDXKbCStBPaocyNdI1S9iculDVnDO4B8UGNRlUViIOPWQUtXLwSoXgRAwe6FInjY4qcwspbSQVi2qECWGkBEvKB_KuuTTHnhnEyJeGVL0TECaNKAkrV6NzU7HTG6itxdp8TH5ee385NfZ8bBcqRzzuco8mdxGwUmD3qlPCaBV6XQU_wOVrSyVxinh1ignGJfL5PFl89y0XBvDedQlLPmf8tbK9iG_XkiXJ1daXsHt9u3_-6-e_99t0fH948Kpv1o7L7fb35e60cbd_8pTQ367dPBTOaBR2p8uzZZ9C2yHJPFkUHGSHQo4aIJQDJc_x53v74z_bf18rR7s9H8Za6OGX32y97zk09NAWdVyTDIAZGEgF6Kkm2ZBF6JUmGtigJLUaGkjSQjRuysWSqFcmio8gBDRkIZakS0dc-KcRm_YPy4afXu3c_FxZHLkcvqvxAkpLcEpH2E2daDGkDSdbSKRlAc0gacK8b2eimblIgcwT3fwwOM1oCmSnLiBBDy8lWTk5tpMCeKyvCxB2WR-C_lIykhlKA9tVj-HNGHLXN_SU-Vj3se0iI6r3gjlU-h8ZrrLZh6CD_bqyO6QPMWSD6JWNeMs1ny9lcbU-RG4C0XDiI4y5BcEVlFDh64t_FknK1fRZaUNv36ndqu3Vx0qjVm5et2vlls3bWahyrK7XduDg9qTdOW7Va6-LyvNU4bz4cq9-HPk9PWhdnD_8DYDVSzg)

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
<summary><b>캐릭터 기본 시스템 (AMainCharacter)</b></summary>

**클래스 구조**: `AMainCharacter` + `UMeleeCombatComponent` + `USkillComponent` + `UCrossHairComponent` (SRP 기반 컴포넌트 분리)

**기본 스탯**
| 항목 | 값 | 변수명 |
|------|-----|--------|
| 최대 체력 | `1000` | `MaxHealth` |
| 기본 이동속도 | `700` | `DefaultWalkSpeed` |
| 에임 이동속도 | `500` | `AimWalkSpeed` |
| 대쉬 쿨타임 | `1.0초` | `DashCooldown` |

**Enhanced Input System (UE5)**
- 13개 `UInputAction` 바인딩: `Move`, `Look`, `Jump`, `Aim`, `Fire`, `Reload`, `Dash`, `ZoomIn`, `ZoomOut`, `Skill1~3`, `Pause`
- `UInputMappingContext` 기반 컨텍스트 매핑

**이동 시스템**
- 더블 점프: `bCanDoubleJump` 플래그 + 중력 마찰력 조절 (`HandleDoubleJump()`)
- 8방향 대쉬: `DotProduct` 기반 전/후/좌/우/대각선 방향 판별, 방향별 전용 몽타주
- 대쉬 제약: 대쉬 중 근접 공격 불가, 사격 가능 / 점프 중 대쉬 불가 / 대쉬 중 점프 불가
- 에임 모드 중 대쉬 후 원래 조준 방향 자동 복귀
- `UpdateMovementSpeed()`: 에임/비에임 상태 동적 속도 전환

**무기 장착 시스템**
- `ARifle` × 1, `AKnife` × 2 (좌/우): 소켓 기반 등/손 탈부착
- `AMachineGun`, `ACannon`: 스킬 전용 무기, 1회 생성 후 `SetActorHiddenInGame`으로 재활용
- 모든 소켓 위치/회전값을 `EditAnywhere`로 에디터 커스터마이징 가능

</details>

<details>
<summary><b>사격 및 레이캐스트 시스템 (ARifle)</b></summary>

**라이플 스탯**
| 항목 | 값 | 변수명 |
|------|-----|--------|
| 탄창 용량 | `30발` | `MaxAmmo` |
| 예비 탄약 | `90발` | `TotalAmmo` |
| 연사 간격 | `0.15초` | `FireRate` (~6.67발/초) |
| 재장전 시간 | `2.0초` | `ReloadTime` |
| 기본 데미지 | `15` | `Damage` |
| 헤드샷 배율 | `10배` | `HeadshotDamageMultiplier` |
| 유효 사거리 | `5000` | `Range` |

**레이캐스트 방식**
- 발사 기점: 카메라 중앙(`ViewPoint`) — 조준 방향과 탄도 일치
- 적중 판별: 본(Bone) 이름이 `"Head"` 또는 `"CC_Base_Head"`일 경우 헤드샷 데미지 적용
- `CapsuleComponent` + `Mesh`의 Collision `Visibility:Block` 설정으로 적 감지

**정확도 보정**
| 상태 | 배율 | 변수명 |
|------|------|--------|
| 이동 중 | ×`1.5` | `MovementSpreadMultiplier` |
| 에임 모드 | ×`0.3` | `AimSpreadReduction` |

</details>

<details>
<summary><b>카메라 반동 및 화면 흔들림</b></summary>

**반동 시스템 (`ApplyCameraRecoil()`)**
| 항목 | 값 |
|------|-----|
| 수직 반동 범위 | `1.0` ~ `2.0` |
| 수평 반동 범위 | `-7.0` ~ `7.0` |
| 반동 지속시간 | `0.25초` |
| 회복 속도 | `5.0` |

- `Tick`에서 `FMath::Vector2DInterpTo`로 부드러운 보간 반동 적용
- 재장전 중 반동 비활성화

**화면 흔들림 (`ApplyCameraShake()`)**
| 항목 | 값 |
|------|-----|
| 흔들림 강도 | `0.8` |
| 지속시간 | `0.15초` |

- 카메라 위치에 랜덤 오프셋 적용 → 타이머로 원위치 복구

</details>

<details>
<summary><b>다이나믹 크로스헤어 (UCrossHairComponent)</b></summary>

**크로스헤어 수치**
| 항목 | 값 |
|------|-----|
| 기본 스프레드 | `10.0` |
| 최소 스프레드 | `5.0` |
| 최대 스프레드 | `50.0` |
| 확장 속도 | `8.0` |
| 회복 속도 | `4.0` |

**탄퍼짐 연동**
| 항목 | 값 |
|------|-----|
| 기본 탄퍼짐 각도 | `1.0도` |
| 최대 탄퍼짐 각도 | `5.0도` |

**동적 반응**
- 이동 번짐: 현재 이동 속도에 따라 실시간 확장
- 격발 번짐: `StartExpansion()` 호출로 즉시 확장
- 연속 사격 번짐: `ConsecutiveShotWindow`(`0.5초`) 내 연속 호출 감지 시 추가 분산
- 색상 변화: 기본 `White` → 확장 시 `Orange`(`1.0, 0.5, 0.0`)

</details>

<details>
<summary><b>근접 전투 (UMeleeCombatComponent)</b></summary>

**5단 콤보 시스템**
| 콤보 | 데미지 | 무기 | 설명 |
|------|--------|------|------|
| 1타 | `20` | 칼 | 약공격 |
| 2타 | `25` | 칼 | 약공격 |
| 3타 | `30` | 칼 | 약공격 |
| 4타 | `35` | 발차기 | `KickHitBox` + `FootSocket_R` |
| 5타 | `50` | 칼 | 강공격 |

- 콤보 리셋: `1.5초` 동안 입력 없으면 1타로 초기화 (`ComboResetTime`)
- 콤보 쿨타임: `2.0초` (`ComboCooldownTime`)

**히트 판정 (`SweepMultiByChannel`)**
- 9개 구체 스윕을 부채꼴 형태로 발사하여 180도 범위 커버
- 구체 반지름: `20.0` (`TraceSphereRadius`), 공격 사거리: `180.0` (`AttackRadius`)
- `TSet<AActor*>` 중복 데미지 방지

**자동 조준 및 텔레포트 돌진**
| 항목 | 값 |
|------|-----|
| 자동 조준 거리 | `300` |
| 텔레포트 최소 거리 | `150` |
| 텔레포트 최대 거리 | `1200` |
| 텔레포트 쿨타임 | `5.0초` |
| 텔레포트 도착 오프셋 | `50` |

- `AdjustAttackDirection()`: 모든 적 클래스를 단일 배열로 탐색, 거리 우선 타겟 선정 + `LineTrace` 사선 검증
- 넉백: 칼 `1000`, 킥 `1500` (`KnifeKnockbackStrength`, `KickKnockbackStrength`)

</details>

<details>
<summary><b>에임 모드 / 카메라 시스템</b></summary>

**줌 제어**
| 항목 | 값 |
|------|-----|
| 기본 줌 | `250` |
| 에임 줌 | `70` |
| 최소 줌 | `125` |
| 최대 줌 | `500` |
| 줌 단계 | `20` |
| 줌 보간 속도 | `10.0` |

- 마우스 휠 줌인/줌아웃 (`FMath::FInterpTo` 보간)
- `PreviousZoom` 저장: 에임 모드 해제 시 마지막 줌 값 유지
- Spring Arm `Do Collision Test` 활성화: 카메라 벽 투과 방지
- 에임 모드 시 `FRotator ControlRotation` 기반 이동 방향 조절
- `AimPitch` 값을 ABP와 공유하여 부드러운 상하 조준 애니메이션

</details>

<details>
<summary><b>웨이브 시스템 (AMainGameModeBase)</b></summary>

**데이터 주도 설계**
- `FWaveConfiguration` + `FWaveSpawnEntry` 구조체: BP 에디터에서 웨이브별 적 구성 편집
- 6종 AI 클래스 등록: `AEnemy`, `ABossEnemy`, `AEnemyDog`, `AEnemyDrone`, `AEnemyShooter`, `AEnemyGuardian`

**스폰 시스템**
| 항목 | 값 |
|------|-----|
| 사전계산 스폰 위치 | `200개` |
| 스폰 반경 | `1000` |
| 드론 높이 오프셋 | `200` |
| 위치 갱신 주기 | `300초` (5분) |
| 갱신 시 교체 수 | `50개` |

- NavMesh 기반 사전계산: `PreCalculateSpawnLocations()`에서 게임 시작 시 5단계 반경으로 200개 위치 계산
- 타이머 기반 분할 갱신: `RefillSpawnLocationsPerFrame()` → 게임스레드에서 프레임당 1개씩 NavMesh 쿼리 (AsyncTask 제거, 스레드 안전성 확보)

**웨이브 진행**
- 웨이브 클리어 조건: 현재 웨이브의 모든 적 처치
- 웨이브 간 준비시간: `PrepareTime` (기본 `7.0초`)
- 클리어 대기시간: `DefaultWaveClearDelay` (`5.0초`)
- 클리어 보상: 체력 `200`, 탄약 `60` (`HealthRewardOnClear`, `AmmoRewardOnClear`)
- `UWaveRecordSaveGame` 기반 최고 기록 저장

</details>

<details>
<summary><b>넉백 및 피격 시스템</b></summary>

**넉백**
- `LaunchCharacter` 기반 수평 넉백 (`bXYOverride = true`로 즉각 피드백)
- Z축 0 정규화: 공중 부양 방지, 지면 수평 이동만 허용

**피격 반응**
- 일반 피격: `NormalHitMontages` 배열에서 랜덤 재생
- 빅 히트: `BigHitMontage` → `BigHitRecoverMontage` 순차 재생
- 사망: `DieMontages` 배열에서 랜덤 선택, 포스트프로세싱 사망 연출

**포스트프로세싱 사망 연출**
- `UPostProcessComponent` 직접 제어
- `ColorSaturation = FVector(0, 0, 1)` → 흑백 전환
- `VignetteIntensity = 0.5f` → 화면 가장자리 어둡게 처리

</details>

---

## AI 시스템

<details>
<summary><b>Enemy AI (기본 적) — AEnemy + AEnemyAIController</b></summary>

**기본 스탯**
| 항목 | 일반 | 엘리트 |
|------|------|--------|
| 체력 | `100` | `400` |
| 이동속도 | `300` | `500` |
| 일반공격 데미지 | `20` | `30` |
| 강공격 데미지 | `50` | `60` |
| 점프공격 데미지 | `30` | `40` |
| 엘리트 확률 | — | `30%` (`EliteChance`) |

**상태 머신 (`EEnemyAIState`)**
```
Idle → MoveToCircle (포위 위치 이동) → ChasePlayer (추격)
```

**공격 패턴**
| 공격 | 조건 | 설명 |
|------|------|------|
| 일반 공격 | `AttackRange`(`200`) 이내 | 쿨타임 `2.0초` |
| 강 공격 | 일반 공격 3회 후 | 자동 전환 |
| 점프 공격 | 감지 범위 진입 시 | 루트 모션 기반 이동 |
| 닷지 | 공격 쿨타임 중 `30%` 확률 | 좌/우 랜덤, 쿨다운 `5.0초` |

**포위 시스템**
- 플레이어 중심 반지름 `500` 원형 포위 (`CircleRadius`)
- `StaticAngleOffset`: 적 수에 따라 360° 균등 분배로 겹침 방지
- `ChaseStartDistance`(`3000`) 이내 진입 시 추격 개시
- `StopChasingRadius`(`6000`) 초과 시 추격 중단

**카타나 히트판정 (`AEnemyKatana`)**
- 5단계 구체 스윕 (`NumSteps: 5`, `HitRadius: 30`, `TotalDistance: 150`)
- `TraceInterval: 0.016초` (~60FPS 동기화)
- `TSet<AActor*>` 중복 피격 방지

**엘리트 Enemy**
- `30%` 확률 생성, 전용 등장 애니메이션
- 모든 공격 몽타주 1.5배 속도 재생
- ABP 연동 블렌드스페이스 분기

</details>

<details>
<summary><b>Boss AI — ABossEnemy + ABossEnemyAIController</b></summary>

**기본 스탯**
| 항목 | 값 |
|------|-----|
| 최대 체력 | `2000` |
| 기본 이동속도 | `300` |
| 최대 가속도 | `5000` |
| 텔레포트 거리 | `500` |
| 텔레포트 쿨다운 | `10초` |
| 스텔스 쿨다운 | `30초` |
| 원거리 공격 쿨다운 | `10초` |

**상태 Enum (`EBossState`) — 11개 상태**
```
Idle → Intro → Combat → Teleporting / AttackTeleport / RangedAttack / NormalAttack / UpperBodyAttack / StealthAttack / HitReaction → Dead
```

**AI 상태 머신 (`EBossEnemyAIState`)**
| 상태 | 설명 |
|------|------|
| `Idle` | 대기 |
| `MoveToPlayer` | 접근 (`BossMoveRadius: 600`) |
| `NormalAttack` | 근접 공격 (`BossStandingAttackRange: 200`, `BossMovingAttackRange: 250`) |

**공격 패턴 우선순위**
```
1순위: 스텔스 공격 (200~250 거리, 쿨다운 미적용 시)
2순위: 거리별 분기
  ├─ 0~200: 후퇴텔레포트(50%) vs 근접공격(50%)
  ├─ 201~250: 상체분리 이동공격 (bUseUpperBodyBlend)
  └─ 251+: 걸어서 추격
```

**스텔스 공격 시퀀스 (`EStealthPhase`) — 6단계**
| 단계 | 설명 |
|------|------|
| `Starting` | 시작 몽타주 재생 |
| `Diving` | 지면 파고들기 |
| `Invisible` | `SetActorHiddenInGame(true)` 투명화, 플레이어 주변 위치 갱신 추적 |
| `Kicking` | 텔레포트 후 킥 공격 (`LineTrace` 판정, `LaunchCharacter`로 공중 발사) |
| `Finishing` | 킥 적중 시 피니시 연계 공격 |
| `None` | 종료, AI 복구 |

**텔레포트 시스템**
- 후퇴 텔레포트: 플레이어 반대 방향 → 원거리 공격 또는 공격 텔레포트 연계
- 공격 텔레포트: 플레이어 등 뒤 사각지대(`AttackTeleportRange: 150`)로 기습 이동

**상하체 분리**: `bUseUpperBodyBlend` 플래그로 이동 중 독립적 상체 공격

**무적 시스템**: 텔레포트/스텔스 특정 구간에서 `bIsInvincible` 활성화

**투사체 공격 (`ABossProjectile`)**: 직선 비행 → 충돌 시 `ApplyAreaDamage()` 광역 피해

**메모리 관리**
- 사망 시 `HideBossEnemy()`: 무기/AI/타이머/델리게이트 안전 정리
- `SetSafeTimerForNextTick`으로 다음 프레임에 액터 파괴

</details>

<details>
<summary><b>EnemyDog AI (돌격형) — AEnemyDog + AEnemyDogAIController</b></summary>

**기본 스탯**
| 항목 | 값 |
|------|-----|
| 체력 | `50` |
| 이동속도 | `600` |
| 일반공격 데미지 | `10` |
| 자폭 데미지 | `40` |
| 자폭 반경 | `100` |

**상태 머신 (`EEnemyDogAIState`)**
```
Idle → ChasePlayer
```

**포위 시스템**
- 플레이어 중심 반지름 `100` 원형 포위 (`SurroundRadius`)
- `TActorIterator` 실시간 아군 수 파악, 360° 균등 분배 각도 할당
- `AttackRange`(`130`) 이내 접근 시 공격 실행

**자폭 메카닉**
- 사망 시 `Explode()` 호출 → 반경 `100` 내 `40` 데미지
- 나이아가라 폭발 이펙트 + 폭발 사운드

**성능 최적화**
- `AIUpdateInterval: 0.05초` 틱 제한
- `RotationInterpSpeed: 10.0`, `MoveAcceptanceRadius: 10.0`

</details>

<details>
<summary><b>EnemyDrone AI (비행형) — AEnemyDrone + AEnemyDroneAIController</b></summary>

**기본 스탯**
| 항목 | 값 |
|------|-----|
| 체력 | `40` |
| 궤도 반경 | `500` |
| 비행 높이 | `300` |
| 궤도 속도 | `45°/초` |
| 미사일 쿨다운 | `3.0초` |

**궤도 비행**
- 플레이어 중심 반지름 `500`, 높이 `300` 궤도 비행
- `bClockwise` 기반 시계/반시계 방향 회전
- `FMath::VInterpTo` 보간 이동 (`MoveInterpSpeed: 2.0`)

**장애물 회피**
| 상황 | 대응 |
|------|------|
| 아군 드론 충돌 | 고도 `150` 단위 변경 (`DroneAvoidanceStep`) |
| 지형 충돌 | 궤도 방향 반전 |
| 끼임 (`MaxStuckTime: 1.5초` 초과) | 고도 `500`으로 급상승 (`EscapeHight`) |
| 궤도 이탈 (`OutOfRadiusLimit: 1.5초`) | 궤도 복귀 |

**미사일 시스템 (오브젝트 풀링)**
- `InitialMissilePoolSize: 10` — 게임 시작 시 10개 사전 생성
- `GetAvailableMissileFromPool()`: 비활성 미사일 재활용
- `AEnemyDroneMissile`: 유도 호밍 미사일

**비행 제한**: 최소 `150` ~ 최대 `1000`, 중력 비활성화 (`DroneGravityScale = 0.0`)

</details>

<details>
<summary><b>EnemyShooter AI (원거리) — AEnemyShooter + AEnemyShooterAIController</b></summary>

**기본 스탯**
| 항목 | 값 |
|------|-----|
| 체력 | `200` |
| 사격 사거리 | `350` ~ `700` |
| 사격 쿨다운 | `2.0초` |
| 감지 반경 | `3000` |
| 수류탄 쿨다운 | `3.0초` |

**상태 머신 (`EEnemyShooterAIState`)**
```
Idle → Detecting → Moving → Shooting ↔ Retreating
```

**진형 시스템**
- 플레이어 중심 반지름 `600` 원형 진형 유지 (`FormationRadius`)
- 아군 최소 `180` 단위 이격 (`MinAllyDistance`)
- `PositionUpdateInterval: 1.0초` 간격 재계산

**사선 확보 시스템**
- `LineTrace` 기반 장애물 감지
- 아군에 의해 사선 차단 시 → 자리 변경
- `EnemyGuardian`에 의해 차단 시 → 수류탄 투척

**지능형 무기 (`AEnemyShooterGun`)**
- 2단계 발사: `AimWarningTime` 지연 → `ExecuteDelayedShot()` 실행
- 예측 조준: 플레이어 현재 속도 기반 미래 위치 계산
- `AimingLaserMesh` 기반 시각적 조준 경고선
- `Accuracy` 값(0.0~1.0)에 따른 확률적 명중/탄퍼짐

**수류탄 (`AEnemyShooterGrenade`)**
- `SuggestProjectileVelocity` 포물선 궤도 계산 (`GrenadeLaunchSpeed: 700`)
- 물리 기반 투사체 (중력, 탄성, 마찰), 시간차 폭발
- 타겟 오프셋: `250` (플레이어 약간 전방 조준)

**성능 최적화**
| 캐싱 항목 | 주기 |
|----------|------|
| 아군 목록 | `1.0초` |
| 사선 확인 | `0.3초` |
| 진형 위치 | `1.0초` |
| AI 로직 | `0.1초` |

</details>

<details>
<summary><b>EnemyGuardian AI (방패병) — AEnemyGuardian + AEnemyGuardianAIController</b></summary>

**기본 스탯**
| 항목 | 값 |
|------|-----|
| 체력 | `600` |
| 방패 체력 | `500` |
| 방패 공격 범위 | `150` |
| 진압봉 공격 범위 | `200` |

**이중 역할 시스템**
| 단계 | 조건 | 행동 |
|------|------|------|
| 수호자 모드 | 방패 건재 | 가장 가까운 `EnemyShooter` 정면에서 방패 역할 (`ProtectionDistance: 150`) |
| 포위 모드 | 방패 파괴 | 플레이어 직접 공격 (`SurroundRadius: 150`) |

**방패 시스템 (`AEnemyGuardianShield`)**
- 방패 체력: `500` — 독립 데미지 처리
- 방패 공격: `LineTrace` 판정 + 넉백 `1000` (`KnockbackStrength`)
- 5단계 구체 스윕 (`NumSteps: 5`, `SweepRadius: 50`, `TotalDistance: 60`)
- 파괴 시 → `Stun()` 상태 → 전투 모드 전환

**공격 무기**
| 무기 | 범위 | 데미지 | 판정 |
|------|------|--------|------|
| 방패 | `150` | `10` | `LineTrace` |
| 진압봉 | `200` | — | `Sweep` |

**성능 최적화**
- `TWeakObjectPtr` 기반 안전한 캐시 참조
- `AllyCacheUpdateInterval: 2.0초` 주기 아군 목록 갱신
- 다수 가디언 시 좌우 대칭 방어선 자동 구축

</details>

<details>
<summary><b>AI 공통 시스템</b></summary>

**IHealthInterface (UInterface)**
- `GetHealthPercent()`: 현재 체력 비율 (0.0~1.0) 반환
- `IsEnemyDead()`: 사망 여부 반환
- 6종 적 클래스 공통 구현 → 체력바/UI 표준화

**체력바 (`UHealthBarComponent`)**
- `UWidgetComponent` 상속, 빌보드 방식 월드 위젯
- `OnTakeAnyDamage` 이벤트 바인딩으로 피격 시 자동 표시
- 표시 지속: `3.0초`, 페이드아웃: `0.5초`

**공통 행동**
- 스폰 즉시 인트로 몽타주 실행 (재생 중 피해 면역)
- 사망 후 일정 시간 뒤 `HideEnemy()` → 메모리 정리
- `FVector::DistSquared()` 거리 비교 최적화 (제곱근 생략)
- `ProjectPointToNavigation()` NavMesh 보정
- 공중 스턴 (`EnterInAirStunState`), 중력장 (`EnableGravityPull`) 공통 지원

</details>

---

## 스킬 시스템

<details>
<summary><b>근접 모드 스킬 (USkillComponent)</b></summary>

**스킬1 — 공중 스턴**
| 항목 | 값 |
|------|-----|
| 쿨타임 | `7.0초` |
| 범위 | `400` |
| 스턴 지속시간 | `5.0초` |
| 몽타주 배속 | `1.3배` |
| 효과 지연 | `0.5초` |

- 주변 모든 적을 공중에 띄우고 행동 불가 상태로 만듦
- `EnterInAirStunState()` 호출, 사격/추가 스킬 연계 가능

**스킬2 — 도약 광역 타격**
| 항목 | 값 |
|------|-----|
| 쿨타임 | `5.0초` |
| 데미지 | `50` |
| 범위 | `200` |
| 도약 거리 | `300` |
| 효과 지연 | `0.5초` |
| 몽타주 배속 | `1.5배` |

- 도약 후 `SphereTrace`로 범위 내 적 감지 및 광역 데미지
- 스킬1과 연계하여 띄운 적에게 추가 타격

**스킬3 — 투사체 발사**
| 항목 | 값 |
|------|-----|
| 쿨타임 | `6.0초` |
| 데미지 | `60` |
| 스폰 전방 오프셋 | `200` |
| 스폰 수직 오프셋 | `30` |

- `ASkill3Projectile`: 전방 투사체 발사, 적/벽 충돌 시 폭발
- `TSet<AActor*>` 중복 타격 방지, 나이아가라 폭발 이펙트

</details>

<details>
<summary><b>에임 모드 스킬 (USkillComponent)</b></summary>

**에임스킬1 — 기관총 연사**
| 항목 | 값 |
|------|-----|
| 쿨타임 | `7.0초` |
| 연사 지속시간 | `5.0초` |
| 몽타주 반복 간격 | `0.85초` |

- `AMachineGun` 클래스: `GetFireDirectionWithSpread()` 무작위 탄퍼짐
- ABP 연동 상체 `AimPitch` 회전, Start→Loop 무한반복
- 스킬 사용 시 자동 무기 수납, 이동/점프/다른 스킬 차단
- 대쉬로 강제 취소 가능 (`CancelAimSkill1ByDash()`)

**에임스킬2 — 캐논 투사체**
| 항목 | 값 |
|------|-----|
| 쿨타임 | `6.0초` |
| 지속시간 | `5.0초` |
| 시작 몽타주 배속 | `0.5배` |
| 전환 임계값 | `0.9` |

- `ACannon` + `AAimSkill2Projectile` 클래스
- 고각 발사: 지정 고도 도달 후 `FindClosestEnemy()`로 유도
- 저각 발사: 비유도 폭발형 투사체
- 폭발 후 지속 영역: `ApplyPersistentEffects()` (0.1초마다 끌어당기기) + `ApplyPeriodicDamage()` (지속 데미지)
- `EndPlay()`에서 모든 타이머/오디오 정리로 메모리 누수 방지

**에임스킬3 — 공중 폭격**
| 항목 | 값 |
|------|-----|
| 쿨타임 | `10.0초` |
| 투사체 수 | `5개` |
| 낙하 지연 | `3.0초` |
| 최대 사거리 | `1500` |
| 폭격 반경 | `300` |
| 스폰 높이 | `2000` |

- `AAimSkill3Projectile`: 5개 투사체를 일정 간격 배치
- `3.0초` 후 고도 `2000`에서 하강
- 하늘색 원으로 낙하 예정 지점 표시 (`AimSkill3Radius: 300`)
- 지면 높이 자동 보정 (`AimSKill3TraceHeight: 500`)

</details>

<details>
<summary><b>스킬 무기 관리</b></summary>

- `AMachineGun`, `ACannon`: 게임 시작 시 1회 생성 후 월드에 유지
- `SetActorHiddenInGame(true/false)`: 표시/숨김 전환 (Spawn/Destroy 반복 회피)
- GC 부하 최소화, 스킬 사용 시 지연 없는 즉시 재활용

</details>

---

## UI / UX / 사운드 / VFX

<details>
<summary><b>시스템 UI 및 환경 설정 (USettingsGameInstance)</b></summary>

**아키텍처**
- `UGameInstance` 상속: 게임 시작~종료까지 전역 설정 유지
- `USettingsSaveGame`: 설정 데이터 영속화 (로컬 파일)
- `LoadOrCreate()` 패턴: 저장 데이터 존재 시 로드, 없으면 기본값 생성

**그래픽 설정**
- `UGameUserSettings` 활용 Scalability 제어
- 그래픽 품질, 안티앨리어싱, V-Sync, 창 모드, 해상도 C++ 직접 제어
- `GetSupportedResolutions()`: 모니터 지원 해상도 동적 목록 표시
- `SetScreenResolutionFromString()`: 문자열 기반 해상도 설정

**사운드 설정**
- `USoundMix` + `USoundClass` 기반 마스터 볼륨 실시간 제어
- 설정 변경 즉시 적용 + 파일 저장

**마우스 감도**
- `ApplyMouseSensitivityOnly()`: 감도 배율 실시간 적용

</details>

<details>
<summary><b>메인 메뉴 및 UX 연출 (UMainMenuWidget)</b></summary>

- `BindWidget` 매크로: WBP 버튼(`PlayButton`, `OptionButton`, `ExitButton`)과 C++ 로직 안전 연결
- 레벨 전환 시 `LoadingScreenWidget` 동적 생성 (`LoadingDisplayTime: 2.0초`)
- `FInputModeUIOnly`로 로딩 중 입력 차단
- ESC 일시정지 메뉴: RESUME, RESTART, OPTIONS
- 이벤트 디스패처 연동: 옵션에서 메인 메뉴 복귀 시 자동 설정 저장 및 UI 갱신

</details>

<details>
<summary><b>HUD 시스템</b></summary>

**크로스헤어 (`UCrossHairComponent` + `UCrossHairWidget`)**
- C++ 컴포넌트 + UMG 위젯 분리 설계
- `GetBulletSpreadAngle()`: 현재 크로스헤어 상태를 라이플 탄퍼짐에 반영
- 상/하/좌/우 라인 위치 실시간 계산
- 에임 스킬 사용 시 강제 표시/숨김 제어

**체력바 (`UHealthBarComponent`)**
- `UWidgetComponent` 상속 빌보드 위젯
- `IHealthInterface` 캐싱으로 체력 퍼센트 실시간 갱신
- 피격 시 `3.0초` 동안 표시 → `0.5초` 페이드아웃
- `OnTakeAnyDamage` 델리게이트 바인딩

**플레이어 HUD**
- 체력 비율: `GetHealthPercent()`
- 탄약 정보: `GetRifleCurrentAmmo()`, `GetRifleMaxAmmo()`, `GetRifleTotalAmmo()`
- 스킬 쿨타임: 6개 스킬 + 텔레포트 쿨타임 진행률 BP 바인딩

</details>

<details>
<summary><b>사운드 시스템</b></summary>

- 모든 인게임 사운드를 `Sound Cue` → `Sound Class`로 체계화
- 마스터 볼륨 옵션 실시간 연동
- 거리 기반 사운드 감쇄(`Attenuation`) 적용
- 스킬 쿨타임 완료 알림 사운드 (`Skill1~3ReadySound`, `AimSkill1~3ReadySound`)
- 쿨타임 미충족 사운드 (`SkillCooldownSound`) — `UAudioComponent` 기반 중복 재생 방지
- 드론 비행 루프 사운드 (`FlightLoopAudio`)

</details>

<details>
<summary><b>VFX (시각 효과)</b></summary>

**라이플 VFX**
| 이펙트 | 위치 | 지속시간 |
|--------|------|----------|
| 총구 섬광 | `MuzzleSocket` | `0.15초` |
| 히트 임팩트 | `HitResult.ImpactPoint` | `0.2초` |

- 나이아가라 시스템, 타이머 기반 정리
- `MuzzleFlashScale: 0.1`, `MuzzleFlashPlayRate: 3.0`

**근접공격 이펙트**
- Blender로 원형 → 반원 메쉬 재모델링 (전방 180도 범위 일치)
- 나이아가라 시스템에 `User_ForwardVector`, `User_RightVector` 파라미터 바인딩
- `ENCPoolMethod::AutoRelease` 오브젝트 풀링 성능 최적화
- 킥/나이프 이펙트 전방 오프셋: `80` / `120`

**적 이펙트**
- 무기 히트 나이아가라 이펙트 (`WeaponHitNiagaraEffect`) — 모든 적 공통
- EnemyDog 자폭 폭발 이펙트
- EnemyDrone 사망 이펙트
- Boss 스텔스 피니시 이펙트 + 사운드

</details>

---

## 프로젝트 최적화 및 리펙토링

<details>
<summary><b>런타임 퍼포먼스 개선</b></summary>

- **캐싱 시스템**: 빈번한 `Cast<T>` 연산 오버헤드 방지 → 멤버 변수 캐싱, `BeginPlay` 시점 초기화
  - 예: `AEnemyAIController`, `UAnimInstance`, `UCharacterMovementComponent` 등 모든 적 클래스에 적용
- **Tick 최적화**: `PrimaryActorTick.bCanEverTick = false` 설정으로 불필요한 프레임 소모 최소화
  - AI 틱 주기 제한: Dog `0.05초`, Shooter `0.1초`
- **가상 함수 호출 최적화**: `GetWorld()` 반복호출 → 지역 변수(`UWorld* World`) 활용
- **물리 쿼리 최적화**: 레이캐스트 호출 빈도를 로직에 맞게 조절
- **FORCEINLINE**: 빈번히 호출되는 Getter 함수에 인라인 적용 — 호출 오버헤드 제거

</details>

<details>
<summary><b>코드 안정성 및 컴파일 구조 개선</b></summary>

- **Early Return 패턴**: 54개 클래스 전체에 도입, 불필요한 else/if 중첩 제거
- **의존성 관리**: 불필요한 `#include` 제거 + 전방 선언 적용 → 컴파일 속도 개선, 순환 참조 방지
- **SafeTimer**: 모든 타이머 람다에서 `TWeakObjectPtr` 약참조 캡처 통일 → 객체 소멸 후 크래시 방지
- **AsyncTask 완전 제거**: NavMesh 쿼리 스레드 안전성 확보를 위해 게임스레드 타이머 기반 분할 실행으로 전환
- **리플렉션 시스템 재정비**: `UPROPERTY` / `UFUNCTION` 지정자 전수 조사, 블루프린트 노출 범위 최적화
- **TObjectPtr 마이그레이션**: `AEnemyShooter` 등 로우 포인터 → `TObjectPtr` 순차 전환 (GC 안정성, 디버그 용이성)

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

- **프록시 부하 절감**: 빈번한 `Cast<T>` 제거 + 멤버 변수 캐싱 + 약참조(`WeakPtr`) 적용 → 평균 부하 95% 절감
- **함수 실행 구조 효율화**: Early Return 패턴 도입 → 스택 메모리 점유 50% 절감
- **리소스 스트리밍 효율 향상**: C++ 로직 최적화로 GC 효율 증가 → 약 28MB 추가 스트리밍 풀 확보

</details>

<details>
<summary><b>아키텍처 설계</b></summary>

**SRP 기반 컴포넌트 분리**

기존 `AMainCharacter` 클래스가 2천 줄에 달하던 구조를 단일 책임 원칙에 따라 분리:

| 클래스 | 역할 |
| --- | --- |
| `AMainCharacter` | 이동/점프/대쉬/에임, 입력 바인딩, 무기 관리, 상태 관리, 카메라 반동 |
| `USkillComponent` | 6종 스킬(근접 3, 에임 3) 사용 조건, 쿨타임, 효과, 무기 연동 |
| `UMeleeCombatComponent` | 5단 콤보, 히트박스 제어, 데미지 적용, 자동 조준, 텔레포트 돌진 |
| `UCrossHairComponent` | 다이나믹 크로스헤어, 탄퍼짐 연동, 연속 사격 감지 |

**인터페이스 기반 설계**
- `IHealthInterface` (`UInterface`): 6종 적 클래스 공통 체력 인터페이스
- `GetHealthPercent()`, `IsEnemyDead()` 표준화 → 체력바/UI 결합도 최소화

**스레드 안전성 확보**
- `AsyncTask(BackgroundThread)` → 게임스레드 `SetTimer` 기반 분할 실행으로 전환
- NavMesh 쿼리(`GetRandomPointInNavigableRadius`)의 게임스레드 전용 제약 준수
- 모든 타이머 람다에서 `TWeakObjectPtr` 캡처 컨벤션 통일

</details>

<details>
<summary><b>저사양 PC 대응</b></summary>

- 그래픽 기본값 Epic → Medium 조정, 창모드 설정
- "GPU Crashed or D3D Device Removed"
- 일부 저사양 PC에서 그래픽 설정을 과도하게 높게 설정 할 시 크래시 발생 확인
- 게임 강제 종료 후 `GameUserSettings.ini` 삭제로 기본값 복원 가능
- 복합적인 원인이 존재 (그래픽 드라이버문제, PC의 사양문제 등)

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

### 1. 카메라 충돌
- 카메라와 캐릭터 사이의 지형지물을 감지하여 카메라가 벽을 투과하지 않도록 Spring Arm의 Do Collision Test를 활성화
- 캐릭터가 벽에 밀착한 상태여도 카메라 시작지점(ViewPoint)에서 발생하는 레이캐스트 판정 오류가 없도록 함

### 2. 포스트 프로세싱 기반 사망 연출
- `UPostProcessComponent`를 직접 제어하여 캐릭터 사망 시 화면을 즉각적으로 흑백으로 전환
- `ColorSaturation` = `FVector(0, 0, 1)` → 흑백 전환
- `VignetteIntensity` = 0.5f → 화면 가장자리 어둡게 처리 (비네팅)

### 3. 시스템 UI 및 환경 설정(Options) 구축
- `USettingsGameInstance`를 통해 게임 시작부터 종료까지 유지되는 전역 설정 시스템 구축
- `UGameUserSettings`를 활용하여 그래픽 품질, 안티앨리어싱, V-Sync, 창 모드 및 해상도를 C++에서 직접 제어
- 사용자 모니터 지원 해상도 목록을 동적으로 가져와 UI에 표시
- `USoundMix`와 `USoundClass`를 활용하여 마스터 볼륨 실시간 제어 및 파일 저장

### 4. 메인 메뉴 및 UX 연출
- `UMainMenuWidget`에서 `BindWidget` 매크로를 사용하여 WBP 버튼과 C++ 로직을 안전하게 연결
- 레벨 전환 시 `LoadingScreenWidget`을 동적으로 생성하고, `FInputModeUIOnly`로 로딩 중 입력 차단
- ESC를 통해 RESUME, RESTART, OPTIONS 메뉴를 사용할 수 있는 일시정지 시스템
- 이벤트 디스패처 연동: 옵션에서 메인 메뉴 복귀 시 자동 설정 저장 및 UI 갱신

### 5. 타격 피드백 및 넉백(Knockback) 시스템
- `LaunchCharacter` 기반의 수평 넉백 로직 (총기 및 스킬 히트 시)
- `bXYOverride = true` 적용: 기존 수평 이동 속도를 무시하고 넉백 속도를 즉시 덮어씌움
- Z축 0 정규화로 공중 부양 방지, 지면 수평 이동만 허용

### 6. 웨이브 최고 기록 저장 시스템 (SaveGame)
- `UWaveRecordSaveGame` 클래스로 최고 웨이브 인덱스와 웨이브 이름을 로컬 슬롯에 저장
- 게임 종료 또는 웨이브 클리어 시 현재 기록과 저장된 최고 기록을 비교하여 최신화

### 7. 게임 폴리싱 및 피드백 강화
- 모든 인게임 사운드를 Sound Cue → Sound Class로 체계화, 마스터 볼륨 옵션 실시간 연동
- 거리 기반 사운드 감쇄(Attenuation) 적용
- 총구 섬광, 피격 임팩트, 스킬 효과 등 모든 이펙트 최종 검수 및 눈부심 방지 조정
- 적 체력, 공격력, 스폰 간격 등 웨이브 전반의 수치 밸런싱

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

<details>
<summary><b>AsyncTask 스레드 안전성 이슈 수정</b></summary>

`MainGameModeBase`에서 `AsyncTask`를 사용하던 두 함수에서 스레드 안전성 문제를 발견하고, 모든 비동기 작업을 게임스레드 기반 타이머 방식으로 전환했습니다.

### 1. NavMesh 쿼리의 스레드 안전성 문제

**문제 코드 (`RefillSpawnLocationsAsync`)**
```cpp
// Before — 백그라운드 스레드에서 NavMesh 쿼리 호출 (위험)
AsyncTask(ENamedThreads::BackgroundThreadPriority, [this, CachedCenter, Radius]()
    {
        for (int32 i = 0; i < 50; i++)
        {
            FindRandomLocationOnNavMesh(CachedCenter, Radius, SpawnLocation);
            // ↑ 내부에서 GetWorld(), UNavigationSystemV1, GetRandomPointInNavigableRadius 호출
            // 모두 게임스레드 전용 API
        }
        AsyncTask(ENamedThreads::GameThread, [...] { /* 결과 적용 */ });
    });
```

**문제 사항 3가지**

| # | 문제 | 설명 |
|---|------|------|
| 1 | `GetWorld()` | UObject 접근 — 게임스레드 전용 |
| 2 | `UNavigationSystemV1::GetCurrent()` | 엔진 서브시스템 접근 — 게임스레드 전용 |
| 3 | `GetRandomPointInNavigableRadius()` | NavMesh 데이터 읽기 — 리빌드 중 동시 접근 시 레이스 컨디션 → 크래시 |

**크래시 없이 동작했던 이유**
- NavMesh가 정적 레벨이라 런타임 리빌드가 발생하지 않아 읽기 충돌 타이밍이 없었습니다.
- 읽기 전용 연산이고 빠르게 완료되어 충돌 윈도우가 짧았습니다.
- 레이스 컨디션은 확률적이므로 항상 발생하지는 않습니다.

**수정 코드 (`RefillSpawnLocationsPerFrame` + `RefillOneSpawnLocation`)**
```cpp
// After — 게임스레드에서 프레임당 1개씩 분할 실행 
void AMainGameModeBase::RefillSpawnLocationsPerFrame()
{
    RefillCenter = bSpawnAroundPlayer ? GetPlayerLocation() : SpawnCenterLocation;
    RefillRadius = DefaultSpawnRadius;
    RefillCount = 0;

    TWeakObjectPtr<AMainGameModeBase> WeakThis(this);
    GetWorldTimerManager().SetTimer(RefillTimer, [WeakThis]()
        {
            if (WeakThis.IsValid())
                WeakThis->RefillOneSpawnLocation(); // 게임스레드에서 실행 → 안전
        }, 0.0f, true); // 매 프레임 1개씩, 50프레임(약 0.8초)에 완료
}

void AMainGameModeBase::RefillOneSpawnLocation()
{
    if (RefillCount >= RefillTargetCount) { ClearTimer(RefillTimer); return; }
    FindRandomLocationOnNavMesh(RefillCenter, RefillRadius, SpawnLocation);
    RefillCount++;
}
```

**해결 원리:** `SetTimer` 콜백은 게임스레드에서 실행되므로 NavMesh 쿼리가 게임스레드에서 호출되어 레이스 컨디션을 방지합니다. 50개를 한번에 처리하던 것을 프레임당 1개씩 분할하여 성능 영향도 무시 할 수 있습니다.

---

### 2. `[this]` 직접 캡처

**문제:** 프로젝트 전체에서 타이머 람다에 `TWeakObjectPtr`를 쓰는 컨벤션을 정했으나, `RefillSpawnLocationsAsync`의 외부 람다만 `[this]`를 직접 캡처하고 있었다. 같은 함수의 내부 람다는 `TWeakObjectPtr`를 사용하여 한 함수 안에서도 불일치가 발생했습니다.

**수정:** 모든 타이머 람다에서 `TWeakObjectPtr` 사용으로 통일했습니다.

---

### 3. 불필요한 AsyncTask 사용 제거

**문제 코드 (`PerformAsyncCleanup`)**
```cpp
// Before — 0.1초 대기를 위해 불필요하게 백그라운드 스레드 사용
AsyncTask(BackgroundThread, [WeakThis]()
    {
        FPlatformProcess::Sleep(0.1f);  // 백그라운드에서 대기만 함
        AsyncTask(GameThread, [WeakThis]()
            {
                StrongThis->SpawnedEnemies.RemoveAll(...);
                StrongThis->SpawnedEnemies.Shrink();
            });
    });
```

스레드 안전성 위반은 아니었으나 실제 UObject 작업은 게임스레드에서 수행하며, 0.1초 대기를 위해 불필요하게 백그라운드 스레드를 사용하고 있었습니다.

**수정 코드 (`PerformDeferredCleanup`)**
```cpp
// After — 타이머 한 줄로 동일한 동작
TWeakObjectPtr<AMainGameModeBase> WeakThis(this);
GetWorldTimerManager().SetTimer(CleanupTimer, [WeakThis]()
    {
        if (WeakThis.IsValid())
        {
            WeakThis->SpawnedEnemies.RemoveAll([](const TWeakObjectPtr<APawn>& EnemyPtr)
                { return !EnemyPtr.IsValid(); });
            WeakThis->SpawnedEnemies.Shrink();
        }
    }, 0.1f, false); // 0.1초 후 게임스레드에서 1회 실행
```

---

### 4. 전체 변경 요약

| 항목 | Before | After |
|------|--------|-------|
| 함수명 | `RefillSpawnLocationsAsync()` | `RefillSpawnLocationsPerFrame()` |
| 함수명 | `PerformAsyncCleanup()` | `PerformDeferredCleanup()` |
| 변수명 | `bEnableAsyncCleanup` | `bEnableDeferredCleanup` |
| include | `#include "Async/Async.h"` | 제거 |
| NavMesh 쿼리 스레드 | BackgroundThread | GameThread |
| `this` 캡처 방식 | `[this]` 직접 캡처 | `TWeakObjectPtr` 통일 |
| 메모리 정리 방식 | AsyncTask 이중 호출 | 타이머 0.1초 지연 |
| `StopAllSystems()` | RefillTimer 미포함 | RefillTimer 정리 추가 |

**동작 결과:** 기존과 동일하게 동작하며. 5분마다 스폰 위치 50개를 갱신하고, 웨이브 클리어 후 0.1초 뒤 무효 참조를 정리합니다.

---

### 5. 참고 자료

| 내용 | 출처 |
|------|------|
| 레이스 컨디션은 일반 버그의 10배 수정 비용 발생 | [Threaded Rendering in Unreal Engine (공식)](https://dev.epicgames.com/documentation/en-us/unreal-engine/threaded-rendering-in-unreal-engine) |
| UObject는 게임스레드 소유, 다른 스레드에서 직접 참조 금지 | [Threaded Rendering in Unreal Engine (공식)](https://dev.epicgames.com/documentation/en-us/unreal-engine/threaded-rendering-in-unreal-engine) |
| NavMesh 클린업 시 워커 스레드 접근 레이스 컨디션  발생 | [NavMesh Race Condition (UE-229415)](https://issues.unrealengine.com/issue/UE-229415) |
| 백그라운드에서 UObject 전달 시 GC가 조용히 삭제할 위험 | [How to properly work with UObjects in background threads](https://georgy.dev/posts/avoid-gc-async/) |
| AsyncTask 사용법 및 스레드 지정 방법 | [How to use AsyncTask in Unreal Engine](https://georgy.dev/posts/async-task/) |
| UE5 Task Graph 시스템 공식 레퍼런스 | [Tasks Systems in Unreal Engine (공식)](https://dev.epicgames.com/documentation/en-us/unreal-engine/tasks-systems-in-unreal-engine) |

</details>