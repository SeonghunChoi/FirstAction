# 튜닝 기록

Step 6 마일스톤의 "튜닝값 전후 비교 기록". 값을 바꿀 때마다 **바꾼 값, 이유, 관찰 결과**를 아래에 한 줄씩 추가한다.
수치의 의미와 공식은 `PLAN.md` 4.6, 결정 근거는 `PLAN.md` 9장.

## 공식 (Step 6-4)

```
상대 충격량  I   = ImpactForce / 맞은 쪽 Mass                       (cm/s)
맞은 쪽 밀림 d_v = KnockbackDistancePerImpulse × I                 (cm)
때린 쪽 후퇴 d_a = (PostHitSpacing − 맞은 순간 몸 간격) − d_v      (cm, 음수면 전진)
경직치 감소      = PoiseDamagePerImpulse × I
```

## 기준값 (2026-11-17, Step 6-6)

### 프로젝트 설정 `Combat > Hit Resolve`

| 항목 | 값 |
|---|---|
| Knockback Distance Per Impulse | 0.2 s |
| Poise Damage Per Impulse | 0.07 |

### 공격 데이터

| 데이터 | Damage | Impact Force | Post Hit Spacing | Post Hit Move Duration | Hit Stop | Max Warp |
|---|---|---|---|---|---|---|
| Light_01 | 10 | 24000 | 60 | 0.15 | 0.06 | 150 |
| Light_02 | 10 | 24000 | 60 | 0.15 | 0.06 | 150 |
| Light_03 (마무리) | 15 | 64000 | 200 | 0.35 | 0.08 | 200 |
| Heavy_01 (4연타 한 몽타주) | 25 | 48000 | 150 | 0.25 | 0.12 | 250 |

### 캐릭터 데이터

| 데이터 | Mass | 레벨 배치 스케일 |
|---|---|---|
| Player | 80 | 1.0 |
| Dummy_Small | 40 | 0.75 |
| TrainingDummy (보통) | 80 | 1.0 |
| Dummy_Large | 300 | 1.4 |

### 관찰

- 작은 더미, 보통 더미, 큰 더미에 같은 콤보를 넣었을 때 밀려나는 거리가 모두 적당하다. 작은 적은 멀리 밀리고 플레이어가 따라 들어가며, 큰 적에게는 플레이어가 물러난다.
- 이 기준값을 유지한다. 휘두름마다 다른 간격·충격(예: Heavy 4연타)이 필요하면 6-15에서 다룬다.

## 변경 이력

| 날짜 | 바꾼 값 | 이유 | 결과 |
|---|---|---|---|
| | | | |
