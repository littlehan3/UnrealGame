### 2025-02-11 업데이트내용

### 1. 2025년 1월 1일 부터 2월 11일까지의 문제해결 및 개선 기록 노트입니다.
-https://drive.google.com/file/d/1RFKRTb3lq0Ar7vT-4p2fAWsxuOvCUs_L/view
-계속해서 유지중이며 README 에는 결과적으로 구현한 기능들만 정리했습니다.

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

### 2025-02-15 업데이트 내용

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


