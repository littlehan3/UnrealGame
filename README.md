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


