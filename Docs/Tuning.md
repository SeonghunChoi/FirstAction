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

### 부위 반응 (Step 6-8)

```
맞은 바디의 속도 변화 = 칼 방향 × (상대 충격량 − PartialReactionImpulseThreshold) × PartialReactionVelocityPerImpulse
```

| 항목 | 값 |
|---|---|
| Partial Reaction Impulse Threshold | 200 cm/s |
| Partial Reaction Velocity Per Impulse | 3.0 |
| Physical Animation Data (Orientation/AngularVelocity/Position/Velocity) | 200 / 20 / 200 / 20 |
| 회복 시간 (캐릭터 데이터) | 0.4 s |

## 변경 이력

| 날짜 | 바꾼 값 | 이유 | 결과 |
|---|---|---|---|
| 2026-11-17 | 부위 반응 `Physical Animation Data` 당기는 힘 1000/100/1000/100 → **200/20/200/20** (FirstVR 값의 0.2배, 컴포넌트 C++ 기본값) | 6-8 첫 값(FirstVR)에서 부위 반응이 보이지 않음. 진단 결과 충격은 들어가지만(물리 스텝 1번 뒤 570~760 cm/s) 당기는 힘이 3~4 스텝 만에 50 cm/s 아래로 지워, 흔들림이 히트스톱 프레임 안에서 생겼다 사라졌다 | 당김 0에서도 젖혀졌다 돌아옴, 0.2배가 적당 (눈으로 판단) |
| 2026-11-17 | 부위 반응 계수 `PartialReactionVelocityPerImpulse` 0.5 → **3.0** (출발점), 임계값 200 유지 | 당김이 약해진 뒤 다시 맞춘다. 확인용으로 임계값 0·계수 10을 잠시 썼다 | 약공격은 살짝, 마무리 타격은 크게 흔들려 적당 |
