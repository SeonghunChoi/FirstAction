# 템플릿 Combat 변형 검토 (Step 0-4)

- 대상: UE 5.7 Third Person C++ 템플릿의 `Variant_Combat` (참고 프로젝트 `FirstActionRef`)
- 목적: 파일별로 **그대로 / 고쳐 씀 / 새로 설계 / 제외**를 판정하고, PDF 비교 페이지 소재를 모은다.
- 판정 기준: PLAN 9장 "템플릿 코드 재사용 범위" 결정. 전투 코어(공격, 판정, 타격 해석, 반응, 방어)는 새로 설계하고, 코어 바깥은 고쳐 쓴다.
- 상태: **확정** (2026-11-15)

---

## 1. 템플릿 전투 구조 요약

| 항목 | 템플릿 방식 | 위치 |
|---|---|---|
| 체력 | `float CurrentHP`를 `TakeDamage`에서 직접 감소 | `CombatCharacter.cpp:439`, `CombatEnemy` |
| 공격 값 | `MeleeDamage`, `MeleeKnockbackImpulse`, `MeleeLaunchImpulse`가 **캐릭터당 하나** | `CombatCharacter.h:122-132` |
| 콤보 | 몽타주 하나 + `ComboSectionNames` 배열. 순서 고정, 분기 없음 | `CombatCharacter.cpp:300` |
| 입력 버퍼 | `CachedAttackInputTime` + 허용 시간(캐릭터 프로퍼티) | `CombatCharacter.cpp:125, 233` |
| 판정 | 노티파이 **한 프레임**에 소켓 위치에서 캐릭터 정면으로 구체 스윕 | `CombatCharacter.cpp:255` |
| 피해 전달 | `ICombatDamageable::ApplyDamage` 인터페이스 직접 호출 | `Interfaces/CombatDamageable.h` |
| 넉백 | 때린 쪽이 계산한 임펄스를 맞는 쪽 `CharacterMovement`에 그대로 적용. 질량 개념 없음 | `CombatCharacter.cpp:288, 390` |
| 피격 반응 | 골반 제외 전신 물리 블렌드 0.5, 착지 시 해제 | `CombatCharacter.cpp:461` |
| 플레이어와 적 | `ACombatCharacter`와 `ACombatEnemy`가 **각각** 같은 공격/판정/피해 코드를 복제 | `CombatEnemy.cpp:45-149` |
| AI → 공격 | StateTree 태스크가 `ACombatEnemy::DoAIComboAttack()`을 직접 호출. 콤보 길이는 적 클래스 안에서 랜덤 | `CombatEnemy.cpp:57` |
| 공격 예고 | 공격 시작 시 정면 스윕으로 적에게 `NotifyDanger` 전달 → StateTree 조건 `IsInDanger` → 회피 EQS | `CombatCharacter.cpp:342` |
| 훈련 더미 | 캐릭터가 아닌 스태틱 메시 + 물리 컨스트레인트 액터 | `Gameplay/CombatDummy.h` |

---

## 2. 소스 판정

| 파일 | 하는 일 | 판정 | 비고 |
|---|---|---|---|
| `CombatCharacter` | 플레이어: 카메라, 이동, 콤보, 차지, 판정, 체력, 사망 | **분해** | 아래 세 줄로 나눔 |
| ├ 카메라 붐, 이동/시점 입력 | | 고쳐 씀 | `ACombatCharacterBase` 파생 플레이어 클래스로 이동 |
| ├ 공격, 콤보, 입력 버퍼, 판정, 피해, 넉백 | | **새로 설계** | `GA_MeleeAttack`, `UAttackDataAsset`, `UMeleeTraceComponent`, `GE_Damage`, `UHitResolveSubsystem` |
| └ 사망 후 리스폰 | | 고쳐 씀 | 체력 0 판정만 GAS 속성으로 교체 |
| `CombatEnemy` | 적: 위와 같은 전투 코드 복제 + AI용 공격 함수 | **새로 설계** | 플레이어와 같은 기반 클래스. `OnEnemyDied`, 사망 후 제거 타이머는 가져옴 |
| `CombatPlayerController` | 입력 매핑 등록, 폰 파괴 시 리스폰 | 고쳐 씀 | 터치 입력 부분 제거 |
| `CombatGameMode` | 기본 클래스 지정 | 그대로 | |
| `Interfaces/CombatAttacker` | 판정/콤보 노티파이 콜백 | 제외 | 어빌리티와 `ANS_*`로 대체 |
| `Interfaces/CombatDamageable` | 피해, 사망, 공격 예고 수신 | 제외 | GAS 이벤트/이펙트로 대체. 공격 예고는 결정 1에 따라 제외 |
| `Interfaces/CombatActivatable` | 레벨 장치 켜기/끄기 | 그대로 | |
| `Animation/AnimNotify_DoAttackTrace` | 한 프레임 판정 | 새로 설계 | `ANS_HitWindow` (구간 스윕, 5-6 저프레임 대응) |
| `Animation/AnimNotify_CheckCombo` | 콤보 입력 확인 시점 | 새로 설계 | `ANS_TagWindow` + `Window.Combo` |
| `Animation/AnimNotify_CheckChargedAttack` | 차지 루프 확인 | 제외 | 차지 공격은 PLAN 3.2 범위에 없음 |
| `AI/CombatAIController` | StateTree 실행 | 그대로 | |
| `AI/CombatStateTreeUtility` 이동/보조 태스크 | 착지 조건, 대상 바라보기, 속도 설정, 플레이어 정보 | 고쳐 씀 | `ACombatEnemy` 타입 참조를 기반 클래스로 |
| `AI/CombatStateTreeUtility` 공격 태스크 | `DoAIComboAttack` 직접 호출 | **새로 설계** | 패턴 선택 → 어빌리티 활성화 요청 (설계 규칙 1, 7) |
| `AI/CombatStateTreeUtility` `IsInDanger` 조건 | 공격 예고 반응 | 제외 | 결정 1 |
| `AI/EnvQueryContext_Player` | EQS 기준점: 플레이어 | 그대로 | |
| `AI/EnvQueryContext_Danger` | EQS 기준점: 공격 예고 위치 | 제외 | 결정 1 |
| `AI/CombatEnemySpawner` | 적 생성, 전멸 시 장치 활성화 | 고쳐 씀 | 생성 클래스 타입만 교체 |
| `Gameplay/CombatActivationVolume` | 진입 시 장치 활성화 | 그대로 | |
| `Gameplay/CombatCheckpointVolume` | 리스폰 위치 갱신 | 그대로 | |
| `Gameplay/CombatDummy` | 물리 컨스트레인트 더미 | 새로 설계 | 질량과 반응이 있어야 하므로 기반 캐릭터 클래스로 (5-5, 6-6) |
| `Gameplay/CombatDamageableBox` | 부서지는 상자 | 제외 | 전투 검증과 무관 |
| `Gameplay/CombatLavaFloor` | 닿으면 피해 | 제외 | 범위 밖 |
| `UI/CombatLifeBar` | 체력바 위젯 | 고쳐 씀 | GAS 속성 변경 이벤트로 갱신, 경직치 바 추가 |

## 3. 콘텐츠 판정

| 에셋 | 판정 | 비고 |
|---|---|---|
| `ABP_Manny_Combat` | 고쳐 씀 | 몽타주 슬롯 구성 참고 |
| `AM_ComboAttack`, `AM_ChargedAttack` | 임시 사용 후 교체 | Step 3-3에서 CC0 검 애니메이션 몽타주로 교체 |
| `BP_CameraShake_Hit_Enemy`, `_Player` | 그대로 | Step 6-1 카메라 흔들림 |
| `EnvQuery_Flank`, `EnvQuery_Fallback` | 그대로 | 측면 이동, 거리 유지 (PLAN 3.4 일반형) |
| `EnvQuery_Evade` | 제외 | 결정 1 |
| `ST_CombatEnemy` | 고쳐 씀 | 상태 구성은 PLAN 4.7 기준으로 다시 짠다 |
| `BP_CombatActivationVolume`, `BP_CombatCheckpointVolume` | 그대로 | |
| `BP_CombatDummy`, `BP_CombatDamageableBox`, `BP_CombatLavaFloor` | 제외 | |
| `IMC_Combat`, `IA_ComboAttack`, `IA_ChargedAttack` | 새로 설계 | Step 2-1 입력 태그 매핑 |
| `IA_ToggleCameraSide` | 그대로 | |
| 터치 입력 (`BPI_/UI_TouchInterface_Combat`) | 제외 | |
| `NS_Damage` | 그대로 | 임시 피격 이펙트 |
| `UI_LifeBar` | 고쳐 씀 | |
| `Lvl_Combat` | 참고만 | 맵은 `Portfolio/Maps`에 새로 |

---

## 4. PDF 비교 포인트 (템플릿 → 이 프로젝트)

1. **공격 값의 위치**: 캐릭터당 한 벌 → 공격마다 데이터 한 장. 마무리 타격만 다르게 튜닝할 수 있다.
2. **플레이어와 적**: 같은 전투 코드를 두 클래스에 복제 → 같은 기반 클래스와 같은 어빌리티.
3. **AI와 공격의 경계**: AI가 적 클래스의 공격 함수를 직접 호출 → AI는 패턴 선택과 활성화 요청만.
4. **판정**: 한 프레임 구체 스윕 → 노티파이 구간 동안 검 소켓 쌍 스윕 + 서브스텝 보간 (15fps 재현과 해결).
5. **넉백**: 때린 쪽이 맞는 쪽을 직접 밈, 질량 없음 → 매니저가 질량 비율로 간격을 나누고 각자 적용.
6. **피격 반응**: 전신 물리 블렌드 고정값 → 상대 충격량이 임계값을 넘을 때 맞은 본 아래만.
7. **체력**: float 직접 감소 → GameplayEffect로만 변경.

---

## 5. 결정

- **결정 1** (2026-11-15): 적이 플레이어 공격 예고를 보고 회피하는 반응(`NotifyDanger` → `IsInDanger` → `EnvQuery_Evade`)은 가져가지 않는다. PLAN 3.4 적 행동 범위에 없고, 적 AI는 폴리싱 우선순위 3위다.
