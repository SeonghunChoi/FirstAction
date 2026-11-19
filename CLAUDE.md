# CLAUDE.md — FirstAction (3인칭 격투 액션 포트폴리오)

이 파일은 Claude Code가 이 프로젝트에서 작업할 때 따르는 안내서다.
전체 계획과 설계 근거는 `Docs/PLAN.md`를 참고한다.

---

## 1. 사용자와 프로젝트

- 사용자는 **유니티 경력 약 19년**의 클라이언트 개발자다. C#, 컴포넌트 설계, DOTS/ECS, 전투 시스템, 애니메이션 최적화, Addressables, 빌드 자동화에 익숙하다.
- **언리얼은 아직 익숙하지 않다.** 자매 프로젝트 FirstVR에서 C++ 프로젝트 전환, 빌드, 데이터 에셋, 컴포넌트는 다뤘다. 이 프로젝트에서 처음 다루는 것(GAS, 몽타주, 노티파이, StateTree, 모션 워핑, Asset Manager)은 처음부터 설명한다.
- 목표: UE 클라이언트 리드 지원용 **3인칭 격투 액션** 포트폴리오.
- 핵심 메시지: **"플레이어와 적은 같은 전투 코어를 쓰고, 공격 하나는 데이터 한 장이다."**
- 깃에는 기본 에셋만, 배포 빌드에는 유료(Fab) 에셋을 쓴다. (5장)

---

## 2. 진행 방식: 튜토리얼 모드

### 2.1 기본 규칙

- **한 번에 한 스텝만** 진행한다. 스텝이 끝나면 사용자의 확인("됐어", "다음")을 기다린다.
- 각 스텝은 아래 순서로 안내한다.
  1. **목표**: 이번 스텝에서 만드는 것 (한두 줄)
  2. **왜**: `Docs/PLAN.md`의 어느 절과 연결되는지
  3. **에디터에서 할 일**: 메뉴 경로를 정확히 (`Tools > New C++ Class...`처럼)
  4. **코드**: 파일 경로와 전체 내용. 핵심 줄은 설명으로 짚는다.
  5. **빌드**: 이번 변경에 맞는 빌드 방법 (3.2)
  6. **확인 방법**: 무엇이 보이면 성공인지 (로그 문구, 화면 상태, 디버그 명령)
- 언리얼 개념은 **유니티 대응 개념과 비교**해서 설명한다. (3.3 표)
- 코드는 Claude가 작성하고, 에디터 조작(데이터 에셋 생성, 몽타주 편집, 노티파이 배치, 레벨 배치)은 사용자가 직접 한다.
- 사용자가 "직접 해볼게"라고 하면 코드를 주지 말고 **목표, 필요한 API 이름, 힌트**만 준다.

### 2.2 설계 판단이 필요할 때

- `Docs/PLAN.md` 9장 "결정 필요 목록"의 항목이나 구현 중 생긴 설계 판단은 **바로 답을 주지 말고 핵심을 짚는 질문을 하나씩** 던져 사용자가 결정하게 한다.
- 사용자가 "정리해줘" 또는 "결론 내줘"라고 하면 질문을 멈추고 정리한다.
- 결정된 내용은 `Docs/PLAN.md` 9장 표와 이 파일의 **9. 진행 상황 > 결정 기록**에 반영한다.

### 2.3 막혔을 때

- 빌드 에러는 에러 메시지 전체를 요청하고, 원인을 UHT, 모듈 의존성(`Build.cs`), 링크 관점에서 설명한 뒤 수정한다.
- 에디터 화면 문제는 스크린샷이나 메뉴 상태를 물어본다. **메뉴 경로나 API 이름을 추측으로 지어내지 않는다.** 엔진 버전에 따라 다를 수 있는 것은 확인을 요청한다.

---

## 3. 언리얼 기초 안내

### 3.1 프로젝트 생성 (Step 0에서 1회)

- UE 5.7에서는 C++ 버전에 변형 선택이 없고 모든 변형(`Variant_Combat`, `Variant_Platforming`, `Variant_SideScrolling`)의 코드와 콘텐츠가 함께 생성된다.
- 메인: `Games > Third Person`, 프로젝트 유형 **C++**. 생성 후 쓰지 않는 변형은 정리하고, `Variant_Combat`은 0-4 판정 후 필요한 것만 옮긴다.
- 참고용: 같은 템플릿, **C++**, 별도 폴더 `FirstActionRef` (저장소에 넣지 않음, 수정하지 않음)
- C++ 프로젝트로 시작하므로 FirstVR처럼 블루프린트 → C++ 전환 과정은 필요 없다.

### 3.2 빌드 방법 선택

| 변경 내용 | 방법 |
|---|---|
| `.cpp` 함수 본문만 수정 | 에디터의 Live Coding (`Ctrl+Alt+F11`) |
| 헤더 수정, `UPROPERTY`/`UFUNCTION` 추가·변경, 새 클래스 | **에디터를 닫고** IDE에서 빌드 후 재실행 |
| `Build.cs` 수정, 플러그인 추가 | 에디터 닫기 → `.uproject` 우클릭 `Generate Visual Studio project files` → 빌드 |

헤더 변경을 Live Coding으로 처리하면 블루프린트나 에셋이 깨질 수 있으므로 항상 위 표를 따른다.

### 3.3 유니티 대응 개념

| 언리얼 | 유니티에서 비슷한 것 | 이 프로젝트에서 |
|---|---|---|
| `UAbilitySystemComponent` | 스킬/버프를 관리하는 매니저 컴포넌트 | 플레이어와 적 모두 부착 |
| `UGameplayAbility` | 실행 가능한 스킬 클래스 (코루틴 흐름 포함) | `GA_MeleeAttack`, `GA_Dodge`, `GA_HitReact` |
| `UGameplayEffect` | 스탯 변경을 데이터로 정의한 명령 | 피해, 경직치, 무적 |
| `UAttributeSet` | 스탯 묶음 클래스 | `Health`, `Poise` |
| `FGameplayTag` | 계층형 문자열 enum | 상태, 구간, 입력, 이벤트 |
| `UAnimMontage` | 레이어 위에서 재생하는 클립 + 구간 정보 | 공격, 피격, 회피 |
| `UAnimNotifyState` | 시작/끝이 있는 Animation Event (`StateMachineBehaviour`의 Enter/Exit에 가까움) | 판정, 콤보, 캔슬, 무적 구간 |
| `UStateTree` | 계층형 FSM + 행동 트리 혼합 | 적 AI |
| `UWorldSubsystem` | 씬 단위 싱글톤 매니저 | 공격 토큰, 타격 해석 |
| `UPhysicalAnimationComponent` | 애니메이션을 따라가는 Active Ragdoll 블렌딩 | 부위 반응 |
| `UDeveloperSettings` | Project Settings에 뜨는 ScriptableObject 설정 | 콘텐츠 세트 선택 |
| Asset Manager, `UPrimaryDataAsset` | Addressables 그룹/라벨 | 콘텐츠 세트 쿡 포함 |
| `TSoftObjectPtr` | `AssetReference` | 유료 에셋 소프트 참조 |
| Motion Warping | 루트 모션 목표 위치 보정 (직접 구현하던 것) | 공격 시 대상에게 붙기 |

### 3.4 모듈 의존성

필요한 시점의 스텝에서 `Source/FirstAction/FirstAction.Build.cs`에 추가한다. 한꺼번에 넣지 않고, 왜 필요한지 스텝에서 설명한다.

- GAS: `GameplayAbilities`, `GameplayTags`, `GameplayTasks`
- 모션 워핑: `MotionWarping`
- AI: `AIModule`, `StateTreeModule`, `GameplayStateTreeModule`
- 설정: `DeveloperSettings`

### 3.5 자주 쓰는 디버그

- `showdebug abilitysystem`: ASC의 속성, 태그, 활성 어빌리티 표시
- 몽타주 에디터의 노티파이 트랙: 구간 위치 확인
- StateTree 디버거: 적 AI의 현재 상태 확인
- `stat unit`, Unreal Insights: 성능 측정

---

## 4. 설계 규칙 (반드시 지킬 것)

코드를 작성할 때 이 규칙을 어기면 사용자에게 먼저 알린다.

1. **입력과 AI는 어빌리티 활성화 요청만 보낸다.** 어빌리티는 요청자가 플레이어인지 AI인지 모른다.
2. **`GA_MeleeAttack`은 특정 공격을 모른다.** 공격의 성질은 모두 `UAttackDataAsset`에 있다. 공격별 어빌리티 클래스를 만들지 않는다.
3. **타이밍은 애니메이션, 의미는 태그.** C++에 프레임 번호나 초 단위 타이밍을 하드코딩하지 않는다. 튜닝값은 데이터 에셋에 둔다.
4. **콤보 분기 정보는 `UAttackDataAsset::ComboLinks`에만 있다.**
5. **체력과 경직치는 GameplayEffect로만 바꾼다.** 속성 값을 직접 대입하지 않는다.
6. **타격 결과는 `UHitResolveSubsystem`이 계산하고, 적용은 각자 한다.** 공격 어빌리티는 충격 벡터, 타격 후 간격, 맞은 본을 넘기기만 한다. 매니저는 양쪽 질량으로 `FHitResolution`을 만들고, 때린 쪽은 자기 이동량을, 맞는 쪽은 자기 이동량과 반응을 적용한다. 때린 쪽이 맞는 쪽을 직접 밀거나, 맞는 쪽이 때린 쪽을 직접 밀지 않는다.
6-1. **타격 후 간격은 공격 데이터(`PostHitSpacing`)에만 있다.** 질량 비율로 나눌 뿐, 간격 자체를 다른 곳에서 바꾸지 않는다.
6-2. **상대 충격량(충격 ÷ 맞는 쪽 질량)은 매니저에서 한 번만 계산한다.** 경직치, 반응 단계, 부위 반응 판단은 이 값을 받아 쓴다.
6-3. **질량은 플레이어와 적 모두 `UCombatCharacterData`에 둔다.**
6-4. **방어(가드, 패리, 저스트 회피)는 매니저 규칙이다.** 매니저가 맞는 쪽 상태 태그(`State.Guarding`, `Window.Parry`, `Window.JustDodge`)를 보고 `FHitResolution`을 바꾼다. 방어 전용 피해 경로나 별도 넉백 코드를 만들지 않는다. (PLAN 4.6 방어 규칙 표)
6-5. **패리 반응은 기존 부품의 재사용이다.** 충격 벡터 반전 → 공격자 `AttackBone` 아래 부위 반응 → 경직 태그.
7. **적 AI는 패턴을 선택만 한다.** 실행은 플레이어와 같은 어빌리티가 한다.
8. **동시 공격 제어는 `UCombatTokenSubsystem` 한 곳에서만** 한다.
9. 노출 범위:
   - 데이터 에셋의 설정값: `EditDefaultsOnly`, `BlueprintReadOnly`
   - 배치별 초기값: `EditInstanceOnly`
   - 실행 중 상태값: `VisibleInstanceOnly`, `BlueprintReadOnly`, 변경은 함수로만
10. 태그는 C++ 네이티브 태그(`UE_DECLARE_GAMEPLAY_TAG_EXTERN` / `UE_DEFINE_GAMEPLAY_TAG`)로 한 파일에 모아 정의한다.

---

## 5. 에셋 규칙

1. **`Content/Portfolio/` → `Content/ThirdParty/` 하드 참조 금지.** 깃을 클론한 사람이 그대로 열고 실행할 수 있어야 한다.
2. `Content/ThirdParty/`는 `.gitignore` 대상이다. 유료 원본, 이를 참조하는 몽타주, Release 콘텐츠 세트가 모두 여기에 있다.
3. 모든 애니메이션은 **UE5 마네킹 스켈레톤 기준**. 유료 애니메이션은 IK Retargeter로 리타깃한다.
4. **소켓과 노티파이 이름 규약**을 지킨다. 새 이름을 만들면 6.1 표에 추가한다.
5. 기본 세트의 애니메이션은 **무료이면서 공개 저장소에 원본 재배포가 허용된 것만** 쓴다. 현재는 CC0 에셋(Quaternius UAL2/UAL1 무료판)만 쓰고, 검 메시는 에디터에서 직접 만든 임시 메시를 쓴다. 깃 버전은 기능이 동작하는 것을 보여 주는 수준이고, 자연스러운 동작은 배포 빌드에서 보여 준다. 모든 외부 출처는 `Docs/Credits.md`에 적는다. Fab 무료 에셋, Epic 샘플(GASP, Lyra), Mixamo는 깃에 넣지 않는다.
6. Release 세트 선택은 `UPortfolioContentSettings`가 하고, 로드 실패 시 Default로 폴백한다.

### 5.1 소켓 / 노티파이 / 에셋 이름 규약 (초안, 스텝 진행 중 확정)

| 이름 | 용도 |
|---|---|
| `A_` 접두사 | 애니메이션 시퀀스. 외부 팩은 출처를 붙인다 (`A_UAL2_Sword_Regular_A`) |
| `AM_` 접두사 | 애니메이션 몽타주 |
| `BP_GA_` 접두사 | C++ 어빌리티의 블루프린트 자식. 에셋 참조(이펙트 등)만 기본값으로 담는다 (`BP_GA_MeleeAttack`) |
| `GE_`, `DA_` 접두사 | 게임플레이 이펙트, 데이터 에셋 |
| `GCN_` 접두사 | GameplayCue Notify. 태그 이름을 따른다 (`GameplayCue.Hit.Dealt` → `GCN_Hit_Dealt`). `Portfolio/GameplayCues/`에만 둔다 (Cue 검색 경로) |
| `Start`, `Loop`, `End`, `Parry` (몽타주 구간 이름) | 가드 몽타주: 올림(앞부분 `Window.Parry`) → 유지(반복) → 내림(뒷부분 `Window.Cancel`), 패리 성공 시 `Parry`(튕겨 낸 자세, `Window.Cancel`). `GA_Guard` 기본값과 같아야 한다 |
| `_Stagger` 접미사 | 경직 전용 반응 몽타주. 움찔 몽타주를 복제해 `Rate Scale`만 낮춘 것 (`AM_UAL1_Hit_Chest_Stagger`) |
| `IK_`, `RTG_` 접두사 | IK Rig, IK Retargeter (`IK_UAL2`, `IK_Manny`, `RTG_UAL2_Manny`) |
| `HandGrip_R` (마네킹 스켈레톤에 원래 있는 소켓) | 검 부착 위치. `weapon_r`은 마네킹에 이미 같은 이름이 있고 손을 따라 움직이지 않아 쓰지 않는다 |
| `blade_start`, `blade_end` (검 메시 소켓) | 판정 스윕 소켓 쌍. 메시를 바꿔도 이름은 유지 |
| `AttackTarget` (Motion Warping 목표 이름) | 공격 몽타주의 Motion Warping 구간 `Warp Target Name`. `GA_MeleeAttack`이 구간 시작마다 등록 |
| `Attack Reach` (`ANS_AttackReach`) | 워핑 구간 시작을 덮게 두는 휘두름별 멈출 간격(`StopGap` cm). 워핑 구간과 같은 몽타주에 둔다 |
| `Window.Combo`, `Window.Cancel`, `Window.Parry`, `Window.JustDodge`, `State.Invulnerable`, `State.Downed` | `ANS_TagWindow`가 붙이는 태그 (`Window.Parry`는 가드 시작 몽타주 앞부분, `Window.JustDodge`는 회피 몽타주 무적 구간 앞부분, `Window.Cancel`은 공격·회피·움찔 몽타주에서 다른 행동으로 끊을 수 있는 구간, `State.Invulnerable`은 회피 몽타주의 무적 구간, `State.Downed`는 넘어짐 몽타주의 누운 구간) |

---

## 6. 폴더 구조 (목표)

```
Source/FirstAction/
├── Public/
│   ├── Combat/
│   │   ├── CombatTags.h               // 네이티브 태그
│   │   ├── CombatCharacterBase.h      // ASC 부착, 플레이어/적 공통
│   │   ├── CombatAttributeSet.h       // Health, Poise
│   │   ├── TrainingDummy.h            // 훈련 더미 (0이 되면 자동 리필). 템플릿 CombatDummy와 이름 충돌을 피함
│   │   ├── CombatCharacterData.h      // 몸의 성질: 질량(6-3), 반응 몽타주(6-7), 부위 대표 본과 회복 시간(6-8)
│   │   ├── HitReactionTypes.h         // 반응 단계 열거형 (움찔/경직/넘어짐/그로기)
│   │   ├── AttackDataAsset.h
│   │   ├── HitResolveSubsystem.h      // FHitResolution, 간격 분배, 상대 충격량, 전달용 타깃 데이터
│   │   ├── CombatSettings.h           // 프로젝트 설정 "Combat": 세계 규칙 계수 (밀림, 경직치, 반응 단계 경계, 부위 반응)
│   │   ├── PartialHitReactionComponent.h // 맞은 본 아래 물리 반응
│   │   ├── MeleeTraceComponent.h
│   │   ├── LockOnComponent.h
│   │   ├── Abilities/
│   │   │   ├── CombatGameplayAbility.h // 행동 어빌리티 기반: 캔슬 구간 규칙 (6-9)
│   │   │   ├── GA_DebugLog.h          // 연결 확인용 테스트 어빌리티
│   │   │   ├── GA_MeleeAttack.h
│   │   │   ├── GA_Dodge.h
│   │   │   ├── GA_Guard.h
│   │   │   ├── GA_Death.h             // 사망: State.Dead, 몽타주 후 래그돌 (7-1)
│   │   │   ├── GA_Jump.h              // 점프: 회피와 같은 시작 규칙, 착지로 끝 (8-1 중 추가)
│   │   │   └── GA_HitReact.h
│   │   └── Animation/
│   │       ├── ANS_TagWindow.h
│   │       ├── ANS_AttackReach.h      // 휘두름별 워핑 멈출 간격 (데이터 전용)
│   │       └── ANS_HitWindow.h
│   ├── Input/
│   │   └── CombatInputConfig.h        // 입력 액션 → 입력 태그
│   ├── AI/
│   │   ├── EnemyCharacter.h           // 적: 같은 기반 클래스, 사망 후 제거, 공격 요청 (7-1)
│   │   ├── EnemyAIController.h        // 인지(시각, 피해)와 바라보기, 대상 (7-3)
│   │   ├── EnemyPatternDataAsset.h    // 적 종류 하나 = 한 장: 인지·반응 성향(7-3), 패턴 목록(7-4)
│   │   ├── CombatTokenSubsystem.h
│   │   └── StateTree/
│   │       └── EnemyStateTreeNodes.h  // 적 AI 조건(태그, 대상, 거리)과 태스크(접근, 맴돌기, 공격, 회복) (7-3)
│   └── Content/
│       ├── CombatContentSet.h
│       └── PortfolioContentSettings.h
└── Private/   (Public과 같은 구조로 .cpp)

Content/
├── Portfolio/
│   ├── Characters/   // 플레이어, 적 블루프린트
│   ├── Data/         // 공격, 패턴, Default 콘텐츠 세트
│   ├── GameplayCues/ // GCN_ 에셋 (프로젝트 설정의 GameplayCue 검색 경로)
│   ├── Animations/   // 기본 몽타주
│   ├── AI/           // StateTree 에셋
│   └── Maps/
└── ThirdParty/       // git 제외
```

---

## 7. 튜토리얼 커리큘럼

각 스텝의 세부 내용은 진행 시점에 2.1 형식으로 풀어서 안내한다.

### Step 0. 준비
- 0-1. 메인 프로젝트, 참고 프로젝트 생성(둘 다 C++), 메인에서 Platforming/SideScrolling 변형 정리 (3.1)
- 0-2. `git init`, Git LFS(`*.uasset`, `*.umap`), 언리얼용 `.gitignore` + `Content/ThirdParty/`
- 0-3. `Docs/PLAN.md`, 이 파일 배치, 첫 커밋
- 0-4. 참고 프로젝트의 전투 구현을 함께 훑어보고, 무엇을 새로 설계할지 사용자와 정리 (PDF 비교 페이지 소재)
- 0-5. **리타깃 검증**: Quaternius UAL2 검 애니메이션 하나(→ `Portfolio/`)를 마네킹에 리타깃해 재생 확인. Fab 후보 검증은 9-3으로 미룸
- ✅ 마일스톤: 클론 → 열기 → 실행이 되는 빈 저장소, 리타깃 가능성 확인

### Step 1. GAS 뼈대
- 1-1. 플러그인과 모듈 추가 (3.4)
- 1-2. `CombatTags.h`: 네이티브 태그 정의
- 1-3. `UCombatAttributeSet`: `Health`, `MaxHealth`, `Poise`, `MaxPoise`, 접근자 매크로
- 1-4. `ACombatCharacterBase`: ASC 부착, `IAbilitySystemInterface`, 시작 어빌리티/이펙트 부여
- 1-5. 템플릿 캐릭터 블루프린트의 부모를 교체
- ✅ 마일스톤: `showdebug abilitysystem`에 속성 값이 보인다

### Step 2. 입력 → 어빌리티
- 2-1. Enhanced Input 액션과 입력 태그 매핑 데이터
- 2-2. 입력 태그로 어빌리티를 찾아 활성화하는 경로
- 2-3. 로그만 찍는 테스트 어빌리티
- ✅ 마일스톤: 약 공격 키(패드 버튼) → 어빌리티 활성화 로그

### Step 3. 공격 데이터와 몽타주
- 3-1. `UAttackDataAsset`
- 3-2. `GA_MeleeAttack`: 활성화 시 공격 데이터를 받아 `PlayMontageAndWait`
- 3-3. 리타깃한 CC0 검 공격 애니메이션으로 몽타주 생성, 검 메시에 `blade_start`/`blade_end` 소켓 추가
- ✅ 마일스톤: 키 입력으로 데이터에 지정된 공격 몽타주가 재생된다

### Step 4. 애니메이션 구간과 콤보
- 4-1. `ANS_TagWindow`: 구간 동안 태그 부착/제거
- 4-2. 입력 버퍼 (보관 시간은 데이터값)
- 4-3. `ComboLinks`로 다음 공격 결정
- 4-4. **설계 판단**: 콤보 진행을 같은 어빌리티 안에서 이어갈지, 새 활성화로 넘길지 → 2.2 방식으로 질문
- ✅ 마일스톤: 약-약-강 분기 콤보, 구간 밖 입력은 버퍼 후 소비 또는 무시

### Step 5. 판정과 피해
- 5-1. `UMeleeTraceComponent`: 소켓 쌍 스윕, 한 휘두름 한 번 피격
- 5-2. `ANS_HitWindow`
- 5-3. `Event.Hit` → `GE_Damage`(SetByCaller) 적용
- 5-4. `PostGameplayEffectExecute`에서 경직치 처리
- 5-5. 훈련용 더미 배치
- 5-6. 저 프레임(`t.MaxFPS 15`)에서 판정 누락 재현 → 서브스텝 보간으로 해결 (측정 결과 누락 없음 → 보간 보류, 진단 로그만 추가)
- ✅ 마일스톤: 더미의 체력과 경직치가 줄고, 15fps에서도 판정이 누락되지 않는다

### Step 6. 조작감
- 6-1. 히트스톱 (`CustomTimeDilation`), 카메라 흔들림
- 6-2. 모션 워핑
- 6-3. `UCombatCharacterData`: 질량, 임계값. 플레이어와 더미에 지정
- 6-4. `UHitResolveSubsystem`, `FHitResolution`: 간격 분배와 상대 충격량 (PLAN 4.6)
- 6-5. 때린 쪽 이동 적용 (작은 적: 전진, 큰 적: 뒤로 밀림), 맞는 쪽 밀려남 적용
- 6-6. 작은 더미와 큰 더미를 나란히 두고 같은 콤보로 타격 후 간격 비교 → 분배 공식 튜닝
- 6-7. `GA_HitReact`: 상대 충격량 기반 반응 단계, 방향별 반응, 경직치 0이면 그로기
- 6-8. `UPartialHitReactionComponent`: 임계값 이상일 때 맞은 본 아래 물리 반응
- 6-9. `GA_Dodge`: 무적 구간, 공격 캔슬
- 6-10. **설계 판단**: `Window.Parry`, `Window.JustDodge` 구간을 어디에 둘지 → 2.2 방식으로 질문
- 6-11. `GA_Guard`와 가드 규칙 (간격 유지, 피해 감소)
- 6-12. 패리 규칙: 충격 반전, 공격자 `AttackBone` 부위 반응, 튕김 애니메이션, 경직
- 6-13. 저스트 회피 규칙: 공격자는 공격 데이터대로 이동, 방어자는 회피 방향, 피해 없음
- 6-14. `ULockOnComponent`
- 6-15. **공격 폴리싱** (최우선): 간격, 질량 분배, 히트스톱, 부위 반응 임계값, 마무리 타격 튜닝
- ✅ 마일스톤: 작은 적과 큰 적 모두 콤보 중간에는 간격이 유지되고, 마무리 타격에서 크게 벌어진다. 같은 공격이 가드/패리/저스트 회피에서 서로 다른 결과가 된다. 튜닝값 전후 비교 기록

### Step 7. 적 AI
- 7-1. 적 캐릭터: 같은 기반 클래스, 같은 어빌리티
- 7-2. **설계 판단**: StateTree / Behavior Tree → 2.2 방식으로 질문
- 7-3. 상태 구성: 접근, 측면 이동, 공격, 회복, 경직, 사망
- 7-4. `UEnemyPatternDataAsset`과 패턴 선택 태스크
- 7-5. `UCombatTokenSubsystem`
- ✅ 마일스톤: 일반 적 4체 중 정해진 수만 동시에 공격한다

### Step 8. 확장성 증명
- 8-1. 엘리트 적: 새 패턴 데이터와 공격 데이터만으로 제작
- 8-2. 플레이어 새 공격 1개를 데이터만으로 추가
- 8-3. 데이터 검증 (`IsDataValid`)
- ✅ 마일스톤: 두 작업 모두 C++ 변경 0줄

### Step 9. 콘텐츠 세트와 배포 빌드
- 9-1. `UCombatContentSet`, `UPortfolioContentSettings`, 폴백
- 9-2. Asset Manager 기본 에셋 타입 등록
- 9-3. 유료 에셋 선정과 리타깃 검증(0-5에서 이연), Release 세트 구성 (`ThirdParty/`)
- 9-4. RunUAT BuildCookRun 스크립트: 기본 빌드 / 배포 빌드
- ✅ 마일스톤: 같은 소스에서 두 빌드가 나오고, ThirdParty 없는 클론에서도 기본 빌드가 실행된다

---

## 8. 커밋 규칙

- 스텝(또는 하위 스텝)이 끝나 사용자가 확인하면 커밋을 제안한다. 커밋은 사용자 확인 후 실행한다.
- 메시지 형식: `[Step 3-2] GA_MeleeAttack 몽타주 재생 추가`
- 커밋 전 `git status`로 `Content/ThirdParty/`가 섞이지 않았는지 확인한다.

---

## 9. 진행 상황

Claude Code는 스텝이 끝날 때마다 이 절을 갱신한다.

### 환경 정보
- UE 버전: 5.7.4
- UE 설치 경로: `F:\Epic Games\Games\UE_5.7`
- 메인 프로젝트 경로: `D:\Projects\Unreal\FirstAction`
- 참고 프로젝트 경로: `D:\Projects\Unreal\FirstActionRef`
- IDE: JetBrains Rider
- 입력 장치: 게임패드 (조작감 튜닝 기준)

### 완료
- [x] 계획 문서 작성 (`Docs/PLAN.md`)
- [x] Step 0-1: 메인/참고 프로젝트 생성, 메인에서 Platforming/SideScrolling 변형 제거 후 빌드·실행 확인 (2026-11-15)
- [x] Step 0-2: `git init -b main`, Git LFS(`.uasset`, `.umap` 외), `.gitignore`(`Content/ThirdParty/`, `Content/Developers/` 포함) (2026-11-15)
- [x] Step 0-3: `Docs/PLAN.md` 배치, 첫 커밋 (2026-11-15)
- [x] Step 0-4: 템플릿 Combat 변형 파일별 판정, `Docs/TemplateReview.md` (2026-11-15)

- [x] Step 0-5 (UAL2 쪽): UAL2 Standard RM glb 임포트, 자동 IK Retargeter로 Manny에 `Sword_Regular_A`, `Sword_Dash` 리타깃, 루트 모션 유지 확인. 출처와 라이선스는 `Docs/Credits.md` (2026-11-15)
- [x] Step 0-5 (UAL1 쪽): UAL1 Standard RM glb 임포트, `Roll`을 Manny에 리타깃, 루트 모션 유지 확인. `Hit_Head`, `Hit_Chest`는 같은 `RTG_UAL1_Manny`로 6-7에서 리타깃 (2026-11-15)

- [x] Step 0-5 마무리: Uthana 시험 중단, Fab 후보 검증은 9-3으로 이연 (2026-11-15)

- [x] ✅ Step 0 마일스톤: 공개 저장소 https://github.com/SeonghunChoi/FirstAction 에 push(LFS 531개, 181MB), 새 폴더에 클론 → 명령줄 빌드 성공 → 에디터에서 Play와 리타깃 애니메이션 재생 확인 (2026-11-15)

- [x] Step 1-1: `GameplayAbilities` 플러그인 활성화, `Build.cs`에 `GameplayAbilities`, `GameplayTags`, `GameplayTasks` (Public) 추가, Rider 빌드와 에디터 실행 확인 (2026-11-15)

- [x] Step 1-2: `Public/Combat/CombatTags.h`, `Private/Combat/CombatTags.cpp`에 네이티브 태그 22개(`InputTag`, `State`, `Window`, `Event`, `SetByCaller`) 정의, 프로젝트 세팅 태그 목록에서 확인 (2026-11-15)

- [x] Step 1-3: `UCombatAttributeSet` (`Health`, `MaxHealth`, `Poise`, `MaxPoise`), Setter 없는 접근자 매크로, `PreAttributeBaseChange`/`PreAttributeChange`에서 0~최대값 제한. 초기값은 1-4 시작 이펙트로 (2026-11-15)

- [x] Step 1-4: `ACombatCharacterBase` (Abstract, ASC와 속성 세트를 서브오브젝트로, `IAbilitySystemInterface`, `BeginPlay`/`PossessedBy`에서 `InitAbilityActorInfo`, `StartupAbilities`/`StartupEffects` 부여). 부모 클래스 선택 창에서 확인 (2026-11-15)

- [x] Step 1-5: `AFirstActionCharacter`의 부모를 `ACombatCharacterBase`로 변경, `Portfolio/Data/Effects/GE_InitAttributes`(Instant, Override, 최대값 먼저)를 `BP_ThirdPersonCharacter`의 `Startup Effects`에 지정 (2026-11-15)
- [x] ✅ Step 1 마일스톤: `showdebug abilitysystem`에 `Health`/`MaxHealth`/`Poise`/`MaxPoise` 100 표시 (2026-11-15)

- [x] Step 2-1: `UCombatInputConfig`(입력 액션 → `InputTag`), `Portfolio/Input/`에 `IA_LightAttack`/`IA_HeavyAttack`/`IA_Dodge`/`IA_Guard`, `IMC_Combat`(Default Key Mappings, 패드 X/Y/B/RB + 키보드), `DA_CombatInputConfig`, `BP_ThirdPersonPlayerController`에 `IMC_Combat` 등록 (2026-11-15)

- [x] Step 2-2: `FCombatAbilityGrant {Ability, InputTag}`로 시작 어빌리티 목록 변경(입력 태그는 스펙 동적 태그), `ACombatCharacterBase::AbilityInputTagPressed/Released`, `AFirstActionCharacter`가 `DA_CombatInputConfig`로 Started/Completed 바인딩. 네 버튼 모두 "연결된 어빌리티 없음" 로그 1회 확인 (2026-11-15)

- [x] Step 2-3: `UGA_DebugLog`(InstancedPerActor, 스펙 입력 태그를 로그와 화면에 출력). `BP_ThirdPersonCharacter`에 `{GA_DebugLog, LightAttack}`, `{GA_DebugLog, HeavyAttack}` 부여 (2026-11-15)
- [x] ✅ Step 2 마일스톤: 패드 X/Y → 같은 `GA_DebugLog`가 각각 `InputTag.LightAttack`/`InputTag.HeavyAttack`으로 활성화 로그 (2026-11-15)

- [x] Step 3-1: `UAttackDataAsset : UPrimaryDataAsset`(현재 칸: `Montage`), `Portfolio/Data/Attacks/DA_Attack_Light_01`(임시 `AM_ComboAttack`), `DA_Attack_Heavy_01`(임시 `AM_ChargedAttack`) (2026-11-16)

- [x] Step 3-2: `UGA_MeleeAttack`(SourceObject의 공격 데이터 → `PlayMontageAndWait`, `State.Attacking` Owned/Blocked), 부여 목록에 `AbilityData` 추가. X/Y가 각각 다른 데이터로 몽타주 재생 확인. 임시 `AM_ChargedAttack`은 차지 구간이 무한 반복이라 어빌리티가 끝나지 않음(템플릿은 코드로 구간을 넘김) → 3-3에서 교체 (2026-11-16)

- [x] Step 3-3 (1/2 몽타주): `A_UAL2_Sword_Heavy_Combo` 리타깃(`RTG_UAL2_Manny` 재사용), `Portfolio/Animations/Montages/AM_UAL2_Sword_Regular_A`, `AM_UAL2_Sword_Heavy_Combo`(DefaultSlot, 루트 모션), 공격 데이터 두 장을 교체. C++ 변경 없이 공격이 바뀜 (2026-11-16)
- [x] ✅ Step 3 마일스톤: X/Y로 데이터에 지정된 UAL2 검 몽타주 재생, 둘 다 완료 종료 (2026-11-16)

- [x] Step 3-3 (2/2 검): Modeling Tools로 `Portfolio/Weapons/SM_Sword_Placeholder`(소켓 `blade_start`, `blade_end`), `BP_ThirdPersonCharacter`에 `Weapon` 스태틱 메시 컴포넌트(부모 소켓 `HandGrip_R`, NoCollision) (2026-11-16)

- [x] Step 4-1: `UANS_TagWindow`(구간 동안 ASC Loose 태그 부착/제거, 트랙에 태그 이름 표시, `Tag`는 노티파이 인스턴스 편집을 위해 `EditAnywhere`). `AM_UAL2_Sword_Regular_A` 후반에 `Window.Combo` 구간, showdebug로 구간에서만 태그 확인 (2026-11-16)

- [x] Step 4-2: `ACombatCharacterBase` 입력 버퍼(새 어빌리티를 시작하지 못한 입력만 기록, 최근 1개, `ConsumeBufferedInput`은 받는 태그만 꺼내고 나머지는 남김), `InputBufferDuration` 플레이어 0.3초 (2026-11-16)

- [x] Step 4-3/4-4: `UAttackDataAsset::ComboLinks {InputTag, NextAttack}`, 캐릭터 `OnInputBuffered` 알림, `GA_MeleeAttack`이 `Window.Combo` 열림/버퍼 기록 두 시점에서 `TryAdvanceCombo` → 같은 실행 안에서 몽타주 교체(이전 태스크 콜백 해제 후 `EndTask`). 짧은 UAL2 클립은 블렌드 아웃이 구간을 잡아먹어, 몽타주에 `_Rec` 회복 클립을 이어 붙이고 `Window.Combo`를 휘두르기 끝 ~ 회복 앞쪽에 둠. `Light_01/02` → `Light_02/03`, `Heavy_01` 분기 (2026-11-16)
- [x] ✅ Step 4 마일스톤: 약-약-약, 약-약-강 분기 콤보가 자연스럽게 이어지고, 콤보 전체에서 어빌리티 종료는 한 번 (2026-11-16)

- [x] Step 5-1: `UMeleeTraceComponent`(`BeginTrace`/`EndTrace`, `TG_PostUpdateWork` 틱, 칼날 위 N점의 이전→현재 구체 스윕, Pawn만, 한 휘두름 한 번 `OnHit`, 콘솔 `FirstAction.ShowMeleeTrace 1`), `ACombatCharacterBase`에 `WeaponMesh`(HandGrip_R, NoCollision)와 `MeleeTrace`. BP의 `Weapon`을 `WeaponMesh`로 이전. 실제 스윕 확인은 5-2 (2026-11-16)

- [x] Step 5-2: `UANS_HitWindow`(구간 동안 소유 액터의 `UMeleeTraceComponent`를 `FindComponentByClass`로 찾아 Begin/EndTrace, 구간 하나 = 한 휘두름). 공격 몽타주 4개에 배치(`Heavy_Combo`는 휘두를 때마다 하나씩), `FirstAction.ShowMeleeTrace 1`로 궤적이 칼을 따라감을 확인 (2026-11-16)

- [x] Step 5-3: 공격 데이터 `Damage` 칸, 캐릭터가 `MeleeTrace.OnHit` → `Event.Hit`(판정 결과를 타깃 데이터로 동봉), `GA_MeleeAttack`이 `WaitGameplayEvent`로 받아 `GE_Damage`(Health Add, `SetByCaller.Damage` 음수) 적용. `DamageEffect`는 BP 자식 `Portfolio/Abilities/BP_GA_MeleeAttack` 기본값, 부여 목록도 BP 자식으로 교체. 판정은 스켈레탈 메시 히트만 인정(캡슐 제외) → 맞은 본 이름 확인 (2026-11-16)

- [x] Step 5-4: 공격 데이터 임시 칸 `PoiseDamage`(6-4에서 삭제 예정) → `SetByCaller.PoiseDamage`, `GE_Damage`에 Poise 모디파이어 추가. `UCombatAttributeSet`이 `Pre/PostGameplayEffectExecute`로 경직치가 0보다 큰 값에서 0이 되는 순간에만 `Event.Groggy`(Instigator = 공격자) 1회 (2026-11-16)

- [x] Step 5-5: `ATrainingDummy`(템플릿 `ACombatDummy`와 이름 충돌을 피해 개명, AI 자동 빙의 끔, 체력이나 경직치가 0이 되면 `RefillDelay` 뒤 `ApplyStartupEffects`로 리필). 기반 클래스에서 `ApplyStartupEffects` 분리. `Portfolio/Characters/BP_TrainingDummy`(무기 없음, `GE_InitAttributes`, `RefillDelay` 2초), `Portfolio/Maps/Lvl_Training`에 배치, 판정·피해·Groggy·리필 확인 (2026-11-16)

- [x] Step 5-6: `UMeleeTraceComponent`에 휘두름마다 `판정 구간 끝: 스윕 N프레임, 맞힘 M` 진단 로그. 제한 없음/60/15/10fps 비교에서 누락 없음, `스윕 0프레임` 없음 → 서브스텝 보간은 보류 (2026-11-16)
- [x] ✅ Step 5 마일스톤: 더미의 체력과 경직치가 줄고(Groggy, 리필 포함), 10fps에서도 판정이 누락되지 않는다 (2026-11-16)

- [x] Step 6-1: 공격 데이터 `HitStopDuration`, `ACombatCharacterBase::ApplyHitStop`(캐릭터별 `HitStopTimeDilation`, 겹치면 긴 쪽), 맞은 쪽은 `Event.HitReact`(`EventMagnitude` = 시간)를 `GenericGameplayEventCallbacks`로 받아 스스로 적용. 연출은 GameplayCue: `GA_MeleeAttack`이 자기 ASC에 `GameplayCue.Hit.Dealt`, `GE_Damage`가 `GameplayCue.Hit.Taken`. `Portfolio/GameplayCues/GCN_Hit_Dealt`(흔들림), `GCN_Hit_Taken`(흔들림, `NS_Damage` 불꽃), Cue 검색 경로 `/Game/Portfolio/GameplayCues`, 템플릿 흔들림 에셋 `bSingleInstance`. 히트스톱·흔들림·맞은 위치 불꽃 확인. 맞은 쪽 흔들림은 적이 공격하는 Step 7에서 확인 (2026-11-16)

- [x] Step 6-2: Motion Warping 플러그인(`.uproject`)과 `Build.cs` 모듈, 공격 데이터 `MaxWarpDistance`, `ACombatCharacterBase`의 `MotionWarping` 컴포넌트와 `virtual FindAttackTarget(MaxDistance)`(스틱 입력, 없으면 정면 기준, 전체 60° 원뿔, 각도 우선, 몸 간격 기준), `GA_MeleeAttack::UpdateWarpTarget`이 콤보 타마다 `AttackTarget` 등록(캡슐 반지름 합까지, 최대 거리 제한). 몽타주 4개에 Motion Warping(Skew Warp) 구간, 콘솔 `FirstAction.ShowWarpTarget 1`. 원거리 추격, 스틱 방향 선택, 콤보 중 대상 변경 확인 (2026-11-16)

- [x] Step 6-3: `UCombatCharacterData : UPrimaryDataAsset`(`Mass` kg만), `ACombatCharacterBase::CharacterData`, `BeginPlay`에서 무브먼트 `Mass`로 복사. `Portfolio/Data/Characters/DA_Character_Player`, `DA_Character_TrainingDummy`(둘 다 80kg)를 각 블루프린트에 지정, 시작 로그 확인 (2026-11-16)

- [x] Step 6-4: `UCombatSettings`(프로젝트 설정 "Combat": `KnockbackDistancePerImpulse` 0.2s, `PoiseDamagePerImpulse` 0.07), `UHitResolveSubsystem::Resolve`(상대 충격량 → 맞은 쪽 밀림, 때린 쪽이 나머지), `FHitResolution`과 전달용 `FGameplayAbilityTargetData_HitResolution`. 공격 데이터 `ImpactForce`/`PostHitSpacing` 추가, 임시 `PoiseDamage` 삭제. 피해·경직치·양쪽 히트스톱이 해석 결과로 흐름. 로그로 공식 확인(간격 52→150, 밀림 120, 때린 쪽 전진 22 / 간격 0→150, 후퇴 30), Groggy 동작. 이동 적용은 6-5 (2026-11-16)

- [x] Step 6-5: 공격 데이터 `PostHitMoveDuration`, `ACombatCharacterBase::ApplyHitDisplacement`(루트 모션 몽타주 중이면 `ProcessRootMotionPostConvertToWorld`에서 애니메이션 루트 모션에 더하고, 아니면 Root Motion Source Override·Z 무시·끝나면 속도 0). 때린 쪽은 `GA_MeleeAttack`, 맞은 쪽은 `Event.HitReact` 수신에서 각자 적용. 컨트롤러 없는 더미는 `bRunPhysicsWithNoController`(엔진이 이동 계산을 건너뛰던 문제). 더미 밀림 확인 (2026-11-16)

- [x] Step 6-6: `DA_Character_Dummy_Small`(40kg), `DA_Character_Dummy_Large`(300kg), 자식 블루프린트 `BP_TrainingDummy_Small`/`_Large`를 `Lvl_Training`에 배치(스케일 0.75/1.4). 세 질량 모두 밀림 거리 적당 → 기준값 유지, `Docs/Tuning.md`에 기록. 발견한 문제: `Heavy_Combo`(워핑 구간 4개)가 2타부터 낡은 목표로 워핑 → `UMotionWarpingComponent::OnPreUpdate`에서 구간 시작마다 대상 재질의로 수정, 확인 (2026-11-17)

- [x] Step 6-7: `GA_HitReact`(`Event.HitReact`/`Event.Groggy` Ability Trigger, `ShouldAbilityRespondToEvent`로 공격 중 움찔·낮은 단계·누운 구간 거름, 같거나 높은 단계는 `bRetriggerInstancedAbility`로 처음부터, 경직 이상 `State.Staggered`, 그로기 `State.Groggy`, 그로기 끝에 `GE_RestorePoise`). `EHitReactionStage`, 매니저가 프로젝트 설정 경계값(250/700 cm/s)으로 `FHitResolution::ReactionStage`를 정함. `UCombatCharacterData`에 단계별 반응 몽타주와 `HeadRootBone`. `State.Downed` 태그, 공격은 `State.Staggered` 중 시작 불가, 더미 자동 리필은 체력만. 몽타주 `AM_UAL1_Hit_Chest`/`_Head`, `AM_UAL2_Hit_Knockback`(+`LayToIdle`, `State.Downed` 구간), `AM_UAL2_Idle_Shield_Break`(그로기), `BP_GA_HitReact`를 플레이어와 더미에 부여. 확인: 세 질량의 단계(Small 경직/넘어짐, 보통 경직/넘어짐, Large 움찔), 그로기 진입·차단·회복, 넘어짐 중 낮은 단계 유지. **미확인**: 누운 구간 반응 없음, 머리 몽타주(머리를 맞히지 못함), 공격 중 움찔 → Step 7에서 확인 (2026-11-17)

- [x] Step 6-8: `UPartialHitReactionComponent`(FirstVR 방식: `UPhysicalAnimationComponent` 월드 기준 당김, 부위 대표 본 아래 물리 + `bVelChange` 충격, 회복 시간 동안 블렌드 1→0, 누운 구간·가드에서 막음). 매니저가 `FHitResolution::PartialReactionVelocity` = 칼 방향 × (상대 충격량 − 200) × 0.5 계산. 캐릭터 데이터에 부위 대표 본(양팔, 양다리, `neck_01`, `spine_01`), 골반용 대체 본, 회복 시간 0.4초. 캐릭터가 C++로 두 컴포넌트를 만들고 메시 충돌만 `QueryAndPhysics`(프로필 유지). 부위별 반응 확인(pelvis→spine_01, hand_r→upperarm_r, head→neck_01). 발견한 문제: 너무 가까이 붙으면 `Heavy_Combo` 1타 궤적이 대상 뒤로 지나가 맞지 않음 (2026-11-17)

- [x] Step 6-8 후속: 휘두름별 워핑 멈출 간격 `ANS_AttackReach`(`StopGap`, 데이터 전용 노티파이), `GA_MeleeAttack::FindStopGap`이 워핑 구간 시작 시점을 덮는 노티파이를 찾아 이동 거리 = 몸 간격 − `StopGap`(물러나지 않음). `Heavy_Combo` 1타에 `StopGap` 40cm. 간격 36에서 이동 0, 95/71/79에서 40cm까지 붙고 모두 명중 확인 (2026-11-17)
- [x] Step 6-8 튜닝: 골반을 맞으면 시뮬레이션하지 않는 바디에 충격을 주던 빈틈 수정(부위 밖이면 대표 본에). 부위 반응이 보이지 않던 원인을 진단(충격은 들어가지만 FirstVR 당기는 힘이 3~4 물리 스텝 만에 지움) → 당기는 힘 0.2배(200/20/200/20), 계수 0.5 → 3.0. `Docs/Tuning.md`에 기록 (2026-11-17)

- [x] Step 6-9: `UCombatGameplayAbility`(`CancelWindowRequiredTags`: 이 태그가 있으면 `Window.Cancel`도 있어야 시작), `GA_Dodge`(스틱 방향으로 즉시 돌려 앞구르기, 없으면 정면, `State.Dodging`, 경직 중 막힘, 공격·회피·움찔 중엔 캔슬 구간에서만, 연속 구르기 `bRetriggerInstancedAbility`), `GA_MeleeAttack` 부모 교체·회피 중엔 캔슬 구간에서만. 매니저 `EHitOutcome::Evaded`(`State.Invulnerable`이면 결과 0, 때린 쪽 헛침). 캐릭터가 `Window.Cancel`이 붙는 순간 버퍼 입력 재시도, 실행 중 어빌리티도 재시작 시도. `State.Flinching`, 캐릭터 데이터 `DodgeMontage`. 몽타주: `AM_UAL1_Roll`(무적, 캔슬 구간), 공격 4개·움찔 2개에 `Window.Cancel`. 확인: 방향, 공격 캔슬, 선입력(캔슬 구간에서 소비), 연속 구르기, 회피 → 강공격. 알려진 현상: 연속 구르기 직후 12ms 만에 한 번 더 재시작된 경우 1회. 재시험(연타)에서는 재시작 45ms 뒤 입력이 버퍼로 가서 재현되지 않음 → 이전 몽타주의 `Window.Cancel`이 떨어지기까지 약 1프레임 틈으로 추정, 6-15에서 다시 확인 (2026-11-17)

- [x] Step 6-10 (설계 판단): 패리는 가드 시작 몽타주 앞부분 `Window.Parry`, 저스트 회피는 회피 무적 구간 앞부분 `Window.JustDodge`, 좁은 창 우선, 가드·패리는 정면 각도 안에서만(캐릭터 BP 기본값), 연타 패리는 가드 내리기 동작으로 막음. 코드 변경 없음 (2026-11-17)

- [x] Step 6-11: `GA_Guard`(누르는 동안 `State.Guarding`, 몽타주 `Start`/`Loop`/`End`, Start 중 떼면 Start를 마치고 End, End 캔슬 구간 전엔 다시 올릴 수 없음, 경직 중 막힘, 공격·회피·움찔·가드 중엔 캔슬 구간에서만). 매니저 `EHitOutcome::Guarded`(정면 `GuardAngle` 안이면 체력 0, 경직치·밀림·히트스톱 같음, 반응·부위 반응 없음). 맞은 쪽 Cue를 `GE_Damage`에서 빼고 공격 어빌리티가 결과별로 냄(`Hit.Taken`/`Hit.Guarded`). 캐릭터 `AddMovementInput` 재정의로 가드 중 회전만(`GuardTurnRate`), 눌린 입력 추적. `A_UAL2_Sword_Block`, `AM_UAL2_Sword_Block`, `GCN_Hit_Guarded`, 플레이어 `GuardAngle` 120·`GuardTurnRate` 360. 확인: 올림·유지·내림, 살짝 누르기, 연타 방지, 가드 중 회전, 가드에서 공격·회피 캔슬 (2026-11-17)

- [x] Step 6-12: 매니저 `EHitOutcome::Parried`(`Window.Parry` + 정면 각도, 가드보다 우선, 방어자 결과 0, 히트스톱 × `ParryHitStopMultiplier` 2.0)와 `ResolveParryRecoil`(되돌린 상대 충격량 = 충격 ÷ 공격자 질량, 최소 경직, 경직치, `AttackBone` 부위 반응). `GA_MeleeAttack`이 방어자와 자신에게 각각 `Event.HitReact`, `Hit.Parried` 양쪽, 자기 경직치는 `GE_Damage`로. `GA_Guard`가 `Parried`면 `Parry` 구간으로. 공격 데이터 `AttackBone`(기본 `hand_r`). 가드 몽타주에 `Sword_Regular_B_Rec`를 이어 붙인 `Parry` 구간(반격 `Window.Cancel`), `GCN_Hit_Parried`. 훈련 더미 시험용 `DefenseMode`(`HoldGuard`, `RepeatGuard`). 확인: 패리 3회(공격자 경직·`upperarm_r` 부위 반응·히트스톱 0.12초, 방어자 `Parry` 구간), 가드 11회(체력 0), 가드 중 경직치 0 → 그로기로 가드 깨짐 → 회복 후 다시 가드 (2026-11-17)

- [x] Step 6-13: 매니저 `EHitOutcome::JustDodged`(`Window.JustDodge`, 일반 회피보다 우선, 결과 0, 공격자 헛침). 회피한 쪽이 결과를 받아 자신을 뺀 모든 전투 캐릭터의 `ApplyTimeSlow`(배율 0.25, 1.5초, 세계 규칙). 캐릭터 시간 배율을 히트스톱·슬로모션 출처별로 관리(더 느린 쪽). `GameplayCue.Hit.JustDodged`(회피한 쪽만), `AM_UAL1_Roll`에 `Window.JustDodge`, 더미 `RepeatDodge`. 확인: 저스트 회피 2회(다른 캐릭터 5명 모두 슬로모션), 일반 회피(`Evaded`) 8회(6-9 미확인분). 미확인: 슬로모션 중 히트스톱이 겹쳤을 때 (2026-11-17)

- [x] Step 6-14: `ULockOnComponent`(R3 토글, 카메라 정면 각도 최소·`AcquireDistance` 1500, 오른쪽 스틱 튕김으로 좌우 다음 대상, 카메라 좌우만 `CameraYawSpeed` 360·실제 시간, `LoseDistance` 1800·사망 시 다음 대상, 디버그 표시 `FirstAction.ShowLockOn`). `AFirstActionCharacter`: `IA_LockOn`(R3) 직접 바인딩, 오른쪽 스틱은 락온 중 대상 바꾸기·마우스 시점 무시, `FindAttackTarget` 재정의로 락온 대상. 확인: 잡기·풀기, 스틱으로 연속 바꾸기, 콤보 중 대상을 바꾸면 다음 타가 새 대상으로 워핑, 범위 밖 락온 대상에 최대 거리까지 붙음(간격 286 → 이동 150). 미확인: 사망 시 다음 대상(Step 7) (2026-11-17)

- [x] Step 6-15: Small/보통/Large에 약-약-약, 약-약-강 점검. 간격 유지·마무리 벌어짐·히트스톱·반응 모두 적당 → 값 변경 없이 기준값 확정, `Docs/Tuning.md`에 반응·방어 값 표와 점검 기록. Heavy 휘두름별 차이는 불필요해 하지 않음(Step 8이나 배포 빌드에서 재검토), 판정 구간 변경이 없어 저 프레임 재측정 생략 (2026-11-17)
- [x] ✅ Step 6 마일스톤: 작은 적·큰 적 모두 콤보 중간 간격 유지, 마무리 타격에서 크게 벌어짐. 같은 공격이 일반 타격/가드/패리/일반 회피/저스트 회피에서 서로 다른 결과. 튜닝 전후 비교는 `Docs/Tuning.md` (2026-11-17)

- [x] Step 7-1: `AEnemyCharacter`(같은 `ACombatCharacterBase`, AI 컨트롤러 자동 빙의, `RequestAttack` → `Event.AttackRequest` 페이로드로 `GA_MeleeAttack` 시작, 시험용 배치별 `TestAttack`/`TestAttackInterval`, 사망 후 `DeathRemovalTime` 뒤 제거, `OnEnemyDied`). 편은 `IGenericTeamAgentInterface`(캐릭터 BP `TeamId`, AI 컨트롤러에 빙의 시 전달, 같은 편 판정 제외). `GA_Death`(`Event.Death` 트리거, `State.Dead`, 어빌리티 취소, 이동·충돌 끔, 사망 몽타주 후 래그돌), 속성 세트가 체력 0 전환 순간 `Event.Death`. 캐릭터 데이터 `DeathMontage`, `A_UAL1_Death01`/`AM_UAL1_Death01`, `DA_Character_Enemy`, `Portfolio/Characters/BP_Enemy`. 확인: 적 공격이 플레이어에게 일반 타격(Stagger)·가드·그로기로 가드 깨짐·패리(적 반동 경직)·저스트 회피(슬로모션)로 갈림, 같은 편 더미는 대상이 아님, 적 사망 → 래그돌(2.2초) → 제거(5초). 미확인: 플레이어 사망, 사망 시 락온 다음 대상 (2026-11-17)

- [x] Step 7-2 (설계 판단): 적 AI는 StateTree. 경직 중 공격 토큰은 토큰 관리자가 태그를 보고 반납. 코드 변경 없음 (2026-11-18)

- [x] Step 7-3 (1/2 인지): `UEnemyPatternDataAsset`(`SightRadius` 1500, `LoseSightRadius` 1800, `PeripheralVisionHalfAngle` 70, `TargetMemoryTime` 5, `ReactionTime` 0.6, `ProximityAwarenessRadius` 300), `AEnemyAIController`(시각은 적만, 피해 감각, 빙의할 때 패턴 데이터와 팀으로 인지 갱신, 반응 시간·근접 감지·기억 시간에 따른 Focus, 콘솔 `FirstAction.ShowEnemyAI`), 캐릭터가 `Event.HitReact`에서 `ReportDamageEvent`, `AEnemyCharacter`(`PatternData`, 컨트롤러 방향으로 회전 360°/초, `FindAttackTarget` = AI 대상). `Portfolio/Data/Enemies/DA_Pattern_Enemy`. 확인: 보임·놓침·반응 시간·기억 시간, 등 뒤에서 맞으면 즉시 잡고 돌아섬, 근접 감지 거리 안팎 전환과 돌아섬, 더미는 대상 아님, 시험 공격이 AI 대상으로 워핑 (2026-11-18)

- [x] Step 7-3 (2/2 StateTree): `EnemyStateTreeNodes`(조건 `Enemy Has Tag`/`Enemy Has Target`/`Enemy Target Farther Than`, 태스크 `Enemy Approach`(MoveTo)/`Enemy Strafe`(대상 둘레 옆걸음, 앞을 구로 검사해 막히기 전 방향 전환)/`Enemy Attack`(`MaxWarpDistance` 안까지 걸어 들어가 `RequestAttack`)/`Enemy Recover`), 패턴 데이터 `Engage` 값(`StrafeDistance` 250, `MaxStrafeDistance` 450, 맴돌기 1.5~3초, 속도 180/400, 검사 80, 회복 1초), `AEnemyAIController`에 `UStateTreeAIComponent`(빙의할 때 시작, 시작 실패 경고, 빙의 해제 전에 멈춤), 적 길찾기 이동에 가속도(`bUseAccelerationForPaths`, 애니메이션이 걷기로 바뀜). `Portfolio/AI/ST_Enemy`(Dead, Staggered, Idle, Engage > Approach/Strafe/Attack/Recover), `BP_EnemyAIController`, `Lvl_Training`에 Nav Mesh Bounds Volume. StateTree 노드 칸 Category `Input`은 바인딩 전용이라 값을 넣을 칸은 `Parameter`로. 확인: 다가감 → 맴돌기 → 걸어 들어가 공격 → 회복 반복, 멀어지면 다시 다가감, 더미 앞에서 방향 전환, 경직에서 맴돌기로 복귀, 플레이어 반격에 공격 끊김, 사망 → 래그돌 → 제거 (2026-11-19)

- [x] Step 7-4: `UEnemyPatternDataAsset`에 `Patterns`(`FEnemyAttackPattern`: 시작 공격, 이어 갈 입력 태그 순서, 가중치, 거리 범위, 반복 방지 시간), `ComboInputDelayMin/Max`, `ComboContinueRule`(`Always`/`OnHitOrGuard`). StateTree 태스크 `Enemy Select Pattern`(가중치 × 거리 × 반복 방지 점수에 비례해 무작위, 출력 `PatternIndex`), `Enemy Attack`이 `PatternIndex`를 받아 시작 공격 후 `Window.Combo`마다 지연 뒤 `AbilityInputTagPressed`로 입력. 컨트롤러가 패턴별 마지막 사용 시각, 캐릭터가 `AttackConnectCount`(맞힘·가드됨). `DA_Pattern_Enemy`에 `Light3`/`LightHeavy`/`Heavy`, `ST_Enemy` Attack 상태에 선택 태스크(앞)와 바인딩, `BP_Enemy` `InputBufferDuration` 0.3(0이면 콤보 입력이 버려짐). 확인: 비율 50/33/17%, 반복 점수 하락, 간격 379에서 `Heavy`만 후보, 약-약-약·약-약-강 콤보. 미확인: 엘리트 규칙(`OnHitOrGuard`)은 Step 8-1에서 (2026-11-19)

- [x] Step 7-5: `UCombatTokenSubsystem : UTickableWorldSubsystem`(대상마다 토큰, `RequestTurn`/`AcquireTokens`/`ReleaseTokens`, 매 틱 경직·사망·사라진 보유자 회수, 콘솔 `FirstAction.ShowAttackTokens`). `UCombatCharacterData::AttackTokens`(플레이어 2), 패턴 `TokenCost`(`Heavy` 2), 프로젝트 설정 `AttackTokenAnglePenalty` 3. StateTree: `Enemy Strafe`가 시간이 끝나면 차례를 받을 때까지 계속 맴돌고, `Enemy Select Pattern`이 살 수 있는 패턴만 골라 토큰을 쥐며(공격 상태가 실패·끊기면 반납), `Enemy Recover`가 끝날 때 반납. `Portfolio/Maps/Lvl_TestEnemies`(적 4체, `Lvl_Training`의 적은 이쪽으로 옮김). 확인: 4체 중 동시 공격 2체, `Heavy`는 혼자, 남은 토큰 1이면 `Heavy` 제외, 회복 끝 반납, 경직 회수 2회, 오래 기다린 먼 적보다 가까운 적이 먼저, 플레이어 사망 시 반납. 플레이어 사망(7-1 미확인분) 확인 (2026-11-19)
- [x] ✅ Step 7 마일스톤: 일반 적 4체 중 플레이어 토큰 수(2)만큼만 동시에 공격한다 (2026-11-19)

- [x] Step 8-1: 엘리트 적을 데이터만으로 제작(C++ 0줄). `DA_Character_Elite`(200kg), `GE_InitAttributes_Elite`(체력 300, 경직치 100), `AM_UAL2_Sword_Dash`(워핑, `Attack Reach` 40, 판정, `Window.Combo`, `Window.Cancel`), `DA_Attack_Elite_Dash`(충격 48000, 간격 120, 워핑 최대 450, `LightAttack` → `Light_02`), `DA_Pattern_Elite`(반응 0.3, 맴돌기 350/550·0.8~1.5초, 회복 0.6, 콤보 지연 0.02~0.08, `OnHitOrGuard`, 패턴 `DashCombo` 가중치 4·거리 250~600·토큰 2), `BP_Enemy_Elite`(`BP_Enemy` 자식, 스케일 1.2), `Lvl_TestEnemies`에 배치. 확인: 돌진 → Light_02 → Light_03, 헛치면 콤보 멈춤, 약·강공격에 공격 중 움찔(6-7 미확인분), 마무리(320)에만 경직, 그로기, 토큰 1개 남으면 돌진 제외 (2026-11-19)
- [x] Step 8-1 중 점프 수정: 템플릿이 `ACharacter::Jump`에 바로 묶어 공격 중에도 뛰던 문제. `GA_Jump`(`InputTag.Jump`, 경직·사망·공중 불가, 공격·회피·움찔·가드 중엔 캔슬 구간에서만, 시작 시 행동 몽타주 정지, 착지로 끝, 0.2초 안에 못 뜨면 끝), `State.InAir`(캐릭터 `OnMovementModeChanged`가 낙하 중 부착, 공격·회피·가드 막힘, 착지 순간 버퍼 입력 재시도), `DA_CombatInputConfig`에 `IA_Jump`, 플레이어 부여 목록에 `GA_Jump`. 확인: 캔슬 구간 전 점프는 버퍼로만, 캔슬 구간 점프로 `Sword_Regular_A`/`_C` 끊음, 공중 공격·회피는 착지 때 발동, 그로기 중·공중 재점프 불가, 공중 가드는 버퍼로만 가고 착지 순간 올림 (2026-11-19)

- [x] Step 8-2: 약1의 강 분기를 새 기술로 교체(C++ 0줄). `A_UAL1_Sword_Attack`(`RTG_UAL1_Manny`), `AM_UAL1_Sword_Attack`(워핑, 판정, `Window.Cancel`), `DA_Attack_LightHeavy`(피해 12, 충격 48000, 간격 60, 히트스톱 0.08, 끝 노드), `DA_Attack_Light_01`의 강 링크만 교체. 경직이 보이지 않아 경직 전용 몽타주 `AM_UAL1_Hit_Chest_Stagger`/`_Head_Stagger`(Rate Scale 0.5)를 만들어 모든 캐릭터 데이터의 경직 칸에 지정(경직 0.15 → 약 0.5초). 엘리트 질량 200 → 180kg. 절차 문서 `Docs/AddingAnAttack.md`. 확인: 약-강/약-약-강 분기, 일반 적 600 경직·경직치 42, 엘리트 267 경직·약공격 133 움찔, 엘리트 그로기, 패리 반동 267 (2026-11-20)
- [x] Step 8-2 중 크래시 수정: 락온한 채 같은 적에게 약-강을 거듭 넣다 엔진 `UPhysicalAnimationComponent::UpdateTargetActors`가 빈 뼈 배열을 읽어 크래시(본 11 = `upperarm_l`, 0개 배열). `UPartialHitReactionComponent`가 물리 애니메이션 자체 틱을 끄고 자기 틱(메시 틱 뒤)에서 뼈 배열이 다 찼을 때만 `TickComponent`를 부른다(비면 경고 로그). 확인: 크래시 재현 없음, 맞은 다음 프레임에 빈 배열(0/89) 1회를 건너뜀 (2026-11-20)

### 현재 위치
- **Step 8-3** 시작 전: 데이터 검증 (`IsDataValid`): 몽타주 누락, 판정 구간 없는 공격 등
- 알려진 현상: 점프 중에 맞으면 공중에서 경직 몽타주가 재생된다(공중 피격 반응은 따로 없음). 배포 빌드 후보
- Step 7 마무리 점검에서 확인됨 (2026-11-19): 머리 몽타주(`AM_UAL1_Hit_Head`, 맞은 본 head), 맞은 쪽 흔들림(6-1), 밀림 벽 정지, 경직·그로기 중 회피 금지(입력은 버퍼로만), 슬로모션 중 히트스톱 겹침(6-13, 히트스톱 뒤 슬로모션 유지. 슬로모션 1.5초가 짧게 느껴짐 → 배포 빌드 튜닝 후보)
- Step 8에서 다시 확인할 것 (Step 7 마무리에서 사용자 판단으로 이월, 적 2체로 시험하면 쉬움): 누운 구간 반응 없음(6-7), 6-11 등 뒤에서 때리면 가드 불성립(정면 각도), 6-12 패리 후 반격(`Parry` 구간 캔슬), 사망 시 락온 다음 대상(6-14)

### 결정 기록
- 2026-11-14: 역기획 레퍼런스는 데빌 메이 크라이의 타격 반응(밀려나는 적, 콤보 마무리 타격)
- 2026-11-14: 타격 후 간격은 공격 데이터가 가지고, 플레이어와 적의 질량 비율로 나눈다 (작은 적: 플레이어 전진, 큰 적: 플레이어 뒤로 밀림)
- 2026-11-14: 간격 계산은 외부 매니저(`UHitResolveSubsystem`)가 하고, 적용은 각자 한다
- 2026-11-14: 부위 반응은 적 크기 대비 충격이 임계값 이상일 때만 준다 (FirstVR 물리 반응 방식 재사용)
- 2026-11-14: 매니저 입력은 충격 벡터, 타격 후 간격, 양쪽 질량, 맞은 본/피격 위치
- 2026-11-14: 전투 형태는 근접 무기(검). 깃 버전 애니메이션은 Quaternius UAL2(CC0) 후보, 무료 범위 확인 후 확정. Fab/Mixamo는 깃 제외, 배포 빌드 전용
- 2026-11-14: 방어는 가드/패리/저스트 회피 3종 모두 구현하며, 매니저가 상태 태그로 결과를 바꾸는 규칙으로 처리한다. 가드: 간격 유지, 피해 감소 / 패리: 간격 이동 없음, 충격을 공격자에게 반대 방향으로 되돌려 공격 팔 부위 반응과 경직, 방어자가 먼저 공격할 기회 / 저스트 회피: 공격자는 공격대로, 방어자는 회피 방향으로, 피해 없음
- 2026-11-14: 공격 데이터에 `AttackBone` 추가 (패리 반응 위치)
- 2026-11-14: 폴리싱 우선순위는 공격 최우선, 방어 3종은 동작하고 구분되는 수준까지
- 2026-11-14: 템플릿 재사용 범위. 전투 코어(공격, 판정, 타격 해석, 반응, 방어)는 새로 설계하고, 코어 바깥(적 이동과 배회, 스포너, 체크포인트, UI)은 템플릿 Combat 변형을 고쳐 쓴다. 근거: 템플릿은 공격 값이 캐릭터당 하나이고 질량 개념이 없어 DMC식 간격 손맛을 표현할 수 없다. 파일 단위 판정은 `Docs/TemplateReview.md`
- 2026-11-15: 적이 플레이어 공격 예고를 보고 회피하는 반응(템플릿 `NotifyDanger` → `IsInDanger` → `EnvQuery_Evade`)은 제외. PLAN 3.5 범위 밖에 추가
- 2026-11-15: 깃 버전 검 애니메이션은 Quaternius UAL2 Standard(CC0)의 루트 모션 버전. 모션 워핑(6-2)이 루트 모션을 전제하므로 제자리 버전은 쓰지 않는다. 회피와 방향별 피격은 무료 범위에 없음 (PLAN 5.3)
- 2026-11-15: 깃 버전의 회피와 피격 반응도 보기에 자연스러워야 한다 (규칙 동작 확인만으로는 부족). 빈 곳을 채울 출처는 조사 후 결정
- 2026-11-15: 회피·피격 애니메이션의 동작 질감이 공격(UAL2)과 달라도 된다. 후보는 Quaternius UAL1(CC0)과 Uthana(AI 생성). 조사 결과는 PLAN 9장 표
- 2026-11-15: 깃 기본 세트 애니메이션은 무료로만. UAL1 무료판(구르기, 머리·가슴 피격)을 먼저 쓰고, 나머지(좌우 회피, 측면·후면 피격, 백스텝)는 Uthana 무료(출처 표기)로 생성. 에셋 규칙 5항을 "무료 + 재배포 허용"으로 변경
- 2026-11-15: **결정 변경**. 깃 버전 회피·피격은 "보기에 자연스러움"을 포기하고 기능 구현 수준으로 한다. Uthana 시험에서 아마추어 축척(0.0116)과 `root` 본 부재로 임포트·리타깃이 막혀 에셋 작업에 시간이 과하게 들었기 때문. 회피는 UAL1 `Roll`을 입력 방향으로 돌려 쓰고, 피격은 UAL1 `Hit_Head`/`Hit_Chest`, UAL2 `Hit_Knockback`과 부위 물리 반응(6-8)으로 방향을 구분한다. Uthana는 쓰지 않는다
- 2026-11-15: ASC는 캐릭터에 붙인다. 리스폰(템플릿 방식, 폰 재생성) 때 어빌리티 상태가 이어질 필요가 없다
- 2026-11-15: 플레이어 클래스는 템플릿 `AFirstActionCharacter`를 고쳐 쓴다. 부모만 `ACharacter` → `ACombatCharacterBase`로 바꾸고, `BP_ThirdPersonCharacter`는 그대로 쓴다. 플레이어 전용 코드(입력 → 어빌리티, 락온)는 이 클래스에 추가한다
- 2026-11-15: 어빌리티와 입력 태그의 짝은 부여 목록(`{어빌리티, 입력 태그}`)에 둔다. 같은 `GA_MeleeAttack`을 약공격·강공격 두 입력에 묶어야 하기 때문
- 2026-11-15: `UAttackDataAsset`의 칸은 쓰는 스텝에서 하나씩 추가한다. 이름·타입을 바꾸면 이미 입력한 값이 사라지므로, 의미가 정해지기 전의 칸은 미리 넣지 않는다
- 2026-11-16: 히트스톱은 각자 건다(6-1). 공격 어빌리티는 자기 캐릭터에만 걸고, 맞은 쪽에는 게임플레이 이벤트로 멈춤 시간을 넘겨 스스로 건다
- 2026-11-16: 타격 연출(카메라 흔들림, 타격음, 불꽃)은 GameplayCue로 한다(6-1). 게임 코드는 연출을 모르고, 연출 추가는 Cue 에셋만 고친다. 히트스톱은 Cue가 아니라 이벤트 경로
- 2026-11-17: 맞은 쪽 히트스톱과 밀림은 캐릭터가 적용한다(6-7). 반응이 막혀도(가드, 그로기, 누움) 몸은 밀린다. `GA_HitReact`는 반응 몽타주와 상태 태그만 맡는다
- 2026-11-17: 반응이 공격을 끊는지는 질량에 맡긴다(6-7). 움찔은 공격을 끊지 않고(몽타주 없이 히트스톱·밀림·Cue만), 경직 이상은 공격 몽타주를 끊는다. 큰 적의 슈퍼아머는 질량에서 저절로 생긴다
- 2026-11-17: 반응 중 다시 맞으면 같거나 높은 단계는 처음부터, 낮은 단계는 유지(6-7). 누운 구간(`State.Downed`)과 그로기 중에는 반응 없음. 단 누운 구간에 경직치가 0이 되면 그로기는 들어간다
- 2026-11-17: 반응 단계 경계값은 세계 규칙(프로젝트 설정 `Combat`), 매니저가 단계를 정해 `FHitResolution`에 싣는다. 반응 몽타주는 `UCombatCharacterData`, 머리/몸통은 맞은 본으로 고른다(6-7)
- 2026-11-17: 그로기는 몽타주 길이로 끝나고, 끝날 때 `GE_RestorePoise`로 경직치를 채운다. 그로기 중에는 피해만. 훈련 더미 자동 리필은 체력만 본다(6-7)
- 2026-11-17: 부위 반응 임계값과 세기 계수는 세계 규칙(프로젝트 설정 `Combat`, 반응 단계 경계와 별개)(6-8). 세기는 (상대 충격량 − 임계값) × 계수의 속도 변화, 방향은 칼이 지나간 방향. 매니저가 `FHitResolution::PartialReactionVelocity`로 싣는다
- 2026-11-17: 부위 반응은 맞은 본에서 위로 가장 가까운 부위 대표 본부터 흔든다(6-8). 대표 본 목록, 골반용 대체 본, 회복 시간은 몸마다 `UCombatCharacterData`에 둔다
- 2026-11-17: 부위 반응은 누운 구간과 가드에서만 막고 나머지(공격 중 움찔, 그로기)는 늘 준다(6-8). 캐릭터가 `Event.HitReact`에서 부르고, 막는 태그는 컴포넌트가 본다
- 2026-11-17: 워핑이 멈출 간격은 휘두름마다 몽타주에 둔다(6-8에서 발견). 워핑 구간 시작을 덮는 별도 노티파이 `ANS_AttackReach`(`StopGap`)를 `GA_MeleeAttack`이 구간 시작에서 찾아 읽고, 없으면 몸이 닿는 거리. 이미 더 가까우면 물러나지 않는다
- 2026-11-17: 회피는 공격 몽타주의 `Window.Cancel` 구간에서만 공격을 끊는다(6-9). 구간 밖 입력은 버퍼에 남았다가 구간이 열리는 순간 캐릭터가 다시 시도한다
- 2026-11-17: 회피는 스틱 방향으로 몸을 즉시 돌려 앞구르기, 스틱을 놓으면 정면(6-9). 회피 몽타주는 `UCombatCharacterData::DodgeMontage`
- 2026-11-17: 무적은 매니저 규칙(6-9). `State.Invulnerable`이면 결과 `Evaded`: 피해·반응 없음, 때린 쪽은 헛친 느낌(히트스톱·이동·Cue 없음). 방어 규칙도 `EHitOutcome`으로 같은 자리에 올린다
- 2026-11-17: 경직 중 회피는 막고, 공격·회피·움찔 중에는 그 몽타주의 캔슬 구간에서만 회피(와 회피 중 공격)를 허용한다(6-9). 공통 기반 `UCombatGameplayAbility::CancelWindowRequiredTags`, 움찔 표시 `State.Flinching`
- 2026-11-17: 패리는 가드를 올리는 순간 성립(6-10). 가드 시작 몽타주 앞부분에 `Window.Parry`, 그 뒤는 가드
- 2026-11-17: 저스트 회피 구간은 회피 몽타주 무적 구간의 앞부분(6-10). 겹치면 좁은 창이 이긴다(저스트 회피 > 일반 회피, 패리 > 가드)
- 2026-11-17: 가드와 패리는 맞는 쪽 정면 각도 안에서만 성립(6-10). 각도는 캐릭터 블루프린트 기본값
- 2026-11-17: 연타 패리는 가드 내리기 동작으로 막는다(6-10). RB를 떼면 가드 내리기 몽타주, 그 캔슬 구간 전에는 다시 가드 불가
- 2026-11-17: 가드로 막으면 체력 0, 경직치는 일반 타격과 같게(6-11). 경직치 0이면 그로기가 가드를 깬다. 히트스톱·밀림도 같고, 맞은 쪽 연출은 `GameplayCue.Hit.Guarded`, 막은 쪽은 자세 유지
- 2026-11-17: 맞은 쪽 Cue는 `GE_Damage`에서 빼서 공격 어빌리티가 결과에 따라 고른다(`Hit.Taken`/`Hit.Guarded`)(6-11). 피해 경로는 `GE_Damage` 하나
- 2026-11-17: 가드 중에는 제자리에서 방향 전환만(6-11). 몽타주 `Start`/`Loop`/`End` 구간, 회전은 캐릭터 `AddMovementInput`이 `State.Guarding`이면 처리, 회전 속도 `GuardTurnRate`(BP 기본값)
- 2026-11-17: 가드 중 공격·회피는 가드 몽타주의 캔슬 구간에서만(6-11)
- 2026-11-17: 패리당한 공격자는 되돌린 상대 충격량(충격 ÷ 공격자 질량)으로 반응하되 최소 경직, 경직치도 되돌린 충격량만큼 깎인다(6-12). 방어자는 경직치 0. 매니저 `ResolveParryRecoil`이 공격자 몫을 만들고 공격자가 자기 `Event.HitReact`로 적용
- 2026-11-17: 패리 순간 히트스톱은 `ParryHitStopMultiplier`(프로젝트 설정)배로 길게, 전용 `GameplayCue.Hit.Parried`를 양쪽에, 이동 없음(6-12)
- 2026-11-17: 패리 성공 시 방어자 가드 몽타주가 `Parry` 구간으로 넘어가고, 그 구간의 `Window.Cancel`부터 반격(6-12). `Parry` 구간은 다른 애니메이션(깃 버전 `Sword_Regular_B_Rec`)을 가드 몽타주에 이어 붙여 성공 동작을 구분하고, `Window.Cancel`로 구간 전체를 덮는다
- 2026-11-17: 적 AI 전 방어 시험용으로 훈련 더미에 배치별 `DefenseMode`(`HoldGuard`, `RepeatGuard`)(6-12)
- 2026-11-17: 저스트 회피 보상은 슬로모션(6-13). 패리는 빠른 반격, 저스트 회피는 시간 벌기로 갈라 둔다
- 2026-11-17: 슬로모션은 회피한 쪽을 뺀 모든 전투 캐릭터, 배율·시간은 세계 규칙(`JustDodgeTimeDilation`, `JustDodgeSlowDuration`). 각 캐릭터가 `ApplyTimeSlow`로 스스로 느려지고, 히트스톱과 겹치면 더 느린 쪽. 공격자는 헛친다(6-13)
- 2026-11-17: 저스트 회피 연출은 회피한 쪽에만 `GameplayCue.Hit.JustDodged`, 끝 표시 없음(6-13). 시험용 더미 `RepeatDodge`
- 2026-11-17: 락온은 R3 토글, 카메라 정면 각도가 가장 작은 적(15m 안), 락온 중 오른쪽 스틱 튕김으로 대상 바꾸기(6-14). 플레이어 컴포넌트라 입력 태그 없이 `IA_LockOn`을 캐릭터가 직접 받는다
- 2026-11-17: 락온 중 카메라는 좌우만 대상 쪽으로(속도 BP 기본값), 이동은 평소처럼, 공격은 `FindAttackTarget` 재정의로 락온 대상(6-14)
- 2026-11-17: 락온은 재입력·사망·범위 이탈(18m)로 풀리고, 대상이 죽으면 다음 대상으로(6-14). 표시는 디버그 구(`FirstAction.ShowLockOn`)
- 2026-11-18: 적 AI는 StateTree(7-2). 적은 "지금 어느 상태인가"를 기억하고 조건이 맞을 때만 옮긴다(맴돌기 방향·시간 유지, 경직이 끝나면 정해진 상태로 복귀). 템플릿 `CombatAIController`와 이동·보조 태스크를 고쳐 쓴다
- 2026-11-18: StateTree는 몸 상태를 조건으로 본다(7-3). 전이 조건이 적 ASC의 상태 태그를 확인한다(토큰 관리자와 같은 출처)
- 2026-11-18: 적 AI 대상은 AI Perception 시각 감지로 본 적(팀 태도 Hostile)(7-3). 보지도 맞지도 않은 채 기억 시간이 지나면 놓는다
- 2026-11-18: 시야에서 사라진 대상에는 반응 시간 뒤에 돌아서고, 맞으면 Damage 감각으로 즉시 돈다(7-3). 반응 시간은 적마다 데이터값
- 2026-11-18: 돌아선 뒤 근접 감지 거리(몸 간격) 안이면 대상의 지금 위치를, 밖이면 마지막으로 안 위치만 안다(7-3). 근접전에서는 찾아내고 멀리 빠져나가면 따돌릴 수 있다
- 2026-11-18: AI 성향 값(인지, 반응)은 `UEnemyPatternDataAsset` 한 장에 패턴 목록과 함께 둔다(7-3). 적 종류 하나 = 패턴 데이터 한 장
- 2026-11-19: 경직 상태는 `State.Staggered`·`Downed`·`Groggy` 묶음, 움찔은 빼서 AI가 하던 일을 계속한다(7-3)
- 2026-11-19: 유틸리티 점수 선택은 7-4 패턴 선택 태스크에서만 쓴다. 상태 흐름은 StateTree 전이(7-3)
- 2026-11-19: 맴돌기는 대상을 바라본 채 `StrafeDistance`를 유지하는 옆걸음, 앞을 검사해 막히기 전에 방향을 바꾼다(7-3)
- 2026-11-19: 공격은 그 공격의 `MaxWarpDistance` 안까지 걸어 들어간 뒤 요청한다. 나머지는 워핑(7-3)
- 2026-11-19: 적 공격 패턴 하나 = 시작 공격 + 이어 갈 입력 태그 순서(7-4). 다음 공격은 `ComboLinks`가 정하고, 적은 플레이어와 같은 입력 통로로 태그만 보낸다
- 2026-11-19: 적 콤보 입력은 구간이 열린 뒤 무작위 지연 뒤에, 지연 범위는 적마다 패턴 데이터(7-4)
- 2026-11-19: 콤보를 이어 가는 조건은 적마다(7-4). 일반형은 끝까지, 엘리트는 앞 타가 맞거나 막혔을 때만 잇고 헛치면 멈춘다
- 2026-11-19: 패턴 점수는 가중치 × 거리 × 반복 방지, 점수에 비례해 무작위로 고른다(7-4). 대상 상태 점수는 배포 빌드에서
- 2026-11-19: 공격 토큰은 공격받는 대상마다 따로, 수는 대상의 `UCombatCharacterData::AttackTokens`(7-5)
- 2026-11-19: 토큰은 걸어 들어가기 시작부터 회복이 끝날 때까지 쥔다(7-5). 공격 상태가 실패하거나 끊겨도 돌려준다
- 2026-11-19: 토큰은 기다리는 적 중 대상에게 가깝고 대상 시선 정면인 적부터 준다(7-5). 각도 벌점은 세계 규칙(`AttackTokenAnglePenalty`). 토큰이 없으면 맴돌기를 계속한다
- 2026-11-19: 패턴마다 토큰 비용(`TokenCost`), 남은 토큰보다 비싼 패턴은 고를 수 없다(7-5)
- 2026-11-19: 엘리트는 행동(패턴 데이터)·몸(캐릭터 데이터, 초기 속성 GE)·새 공격(돌진 베기) 세 축을 모두 데이터로 바꾼다(8-1)
- 2026-11-19: 엘리트 질량 200kg. 플레이어 콤보 마무리로만 끊기고 약·강공격에는 공격 중 움찔(8-1). **2026-11-20 변경**: 180kg, 강 계열(약-강·강·약3)로 끊긴다(8-2)
- 2026-11-19: 엘리트 체력 300, 경직치 100(8-1)
- 2026-11-19: 엘리트 돌진 베기는 콤보 시작기. 돌진 → Light_02 → Light_03, 돌진 공격 데이터의 `ComboLinks`에만 링크를 둬 플레이어 콤보는 그대로(8-1)
- 2026-11-19: 점프도 어빌리티(`GA_Jump`, `InputTag.Jump`)로 하고 시작 규칙은 회피와 같다(8-1 중 발견). 템플릿의 직접 바인딩은 지운다
- 2026-11-19: 공중에서는 공격·회피·가드·점프 모두 막는다. 캐릭터가 이동 모드로 `State.InAir`를 붙이고, 착지 순간 버퍼 입력을 다시 시도한다(8-1 중 발견)
- 2026-11-20: 플레이어 새 공격은 약1의 강 분기를 교체해 붙인다(8-2). 약-강과 약-약-강이 다른 기술. 새 버튼은 네이티브 태그가 필요해 데이터만으로는 하지 않는다
- 2026-11-20: 약-강 클립은 UAL1 `Sword_Attack`, 성격은 경직치 깎기(충격 48000, 간격 60, 끝 노드)(8-2)
- 2026-11-20: 경직은 움찔과 같은 클립을 `Rate Scale` 0.5로 재생하는 전용 몽타주(`_Stagger`)로 구분한다(8-2 중 발견). 캐릭터 데이터의 경직 칸만 바꾼다
- 2026-11-20: 물리 애니메이션 갱신은 부위 반응 컴포넌트가 메시 뼈 배열을 확인한 뒤 직접 부른다(8-2 중 크래시 수정)
- 2026-11-18: 경직되는 순간 공격 토큰을 반납한다(7-2). 반납은 `UCombatTokenSubsystem`이 토큰 보유자의 상태 태그를 보고 하고, AI는 반납을 모른다
- 2026-11-17: 편은 엔진 `IGenericTeamAgentInterface` 하나로 나눈다(7-1). 전투 코드와 AI 인지가 같은 값. 팀 번호는 캐릭터 BP(플레이어 0, 적·더미 1), AI 컨트롤러에는 빙의할 때 넘긴다. 같은 편끼리는 칼에 맞지 않는다
- 2026-11-17: 사망은 `Event.Death` → `GA_Death`(`State.Dead`, 어빌리티 취소, 이동·충돌 끔, 사망 몽타주 후 래그돌)(7-1). 적은 `DeathRemovalTime` 뒤 제거, 더미는 자동 리필 유지
- 2026-11-17: 적은 `Event.AttackRequest`(페이로드 = 공격 데이터)로 공격을 요청한다(7-1). AI 전 시험용 배치별 `TestAttack`
- 2026-11-17: 워핑 대상은 몽타주 시작이 아니라 Motion Warping 구간이 시작될 때마다 다시 묻는다(6-6에서 발견). 한 몽타주에 여러 번 휘두르는 `Heavy_Combo`가 2타부터 낡은 목표로 워핑되던 문제
- 2026-11-16: 타격 후 이동 시간은 공격 데이터에 둔다(6-5). 애니메이션 루트 모션이 재생 중이면 Root Motion Source가 무시되므로(엔진 소스 확인), 양쪽 모두 `ProcessRootMotionPostConvertToWorld`에서 루트 모션에 이동량을 더하고, 루트 모션 몽타주가 없을 때만 Root Motion Source(Override)로 옮긴다
- 2026-11-16: 간격 분배는 맞은 쪽 밀림 거리를 상대 충격량으로 먼저 정하고, 때린 쪽이 나머지(PostHitSpacing − 맞은 순간 간격 − 맞은 쪽 거리)를 맞춘다(6-4). 작은 적에서는 때린 쪽이 전진, 큰 적에서는 뒤로 밀린다. 밀림 거리는 상대 충격량에 단순 비례하고, 계수(밀림, 경직치)는 프로젝트 설정(`UDeveloperSettings`)에 둔다. `FHitResolution`은 전용 타깃 데이터로 `Event.HitReact`에 실어 맞은 쪽에 보내고, 매니저는 상태를 기억하지 않는다
- 2026-11-16: `UCombatCharacterData`에는 지금 `Mass`만 넣는다(6-3). 반응 관련 칸은 쓰는 스텝(6-7, 6-8)에서 추가하고, 조작감 값(입력 버퍼, 히트스톱 배율, 대상 원뿔)은 블루프린트 기본값에 둔다. `Mass`는 kg이고 정본이며, 시작할 때 캐릭터 무브먼트 `Mass`에 복사한다
- 2026-11-16: 모션 워핑 대상은 캐릭터가 제공하고, 어빌리티는 캐릭터에 물어본다(6-2). 플레이어와 AI는 고르는 방법이 달라 각자 구현한다. 콤보 타마다 묻고, 락온이 아니면 캐릭터가 그때 다시 고른다. 플레이어는 스틱 입력 방향 우선, 스틱을 놓으면 캐릭터 정면 기준. 탐색 범위는 어빌리티가 공격 데이터의 `MaxWarpDistance`를 넘겨 묻는다. 범위 밖 대상(락온, AI)은 방향만 돌리고 최대 거리까지 붙으며, 붙는 간격은 양쪽 캡슐 반지름 합. 후보가 여럿이면 각도 우선, 원뿔(전체 60°)은 캐릭터에 하나
- 2026-11-16: 연출 Cue는 맞힐 때마다 양쪽에서 발생한다(6-1). 때린 쪽 `GameplayCue.Hit.Dealt`(어빌리티가 자기 ASC에), 맞은 쪽 `GameplayCue.Hit.Taken`(`GE_Damage`). 흔들림 에셋은 `bSingleInstance`를 켜 여러 명을 맞혀도 쌓이지 않게 한다
- 2026-11-16: 저 프레임 판정은 측정 결과만 기록한다(5-6). 10fps까지 누락이 없어 서브스텝 보간은 넣지 않고, 진단 로그를 남겨 Hit Window를 줄이거나 빠른 공격을 추가할 때(6-15) 다시 측정한다
- 2026-11-16: 훈련 더미는 체력이나 경직치가 0이 되면 `RefillDelay` 뒤 시작 이펙트를 다시 적용해 자동으로 채운다
- 2026-11-16: Step 5의 경직치 감소량은 공격 데이터 임시 칸 `PoiseDamage`에서 가져온다. 6-4에서 상대 충격량 계산으로 바뀌면 칸을 삭제한다
- 2026-11-16: 피해는 `GE_Damage`가 `Health`에 SetByCaller 음수 값을 바로 더한다. 메타 속성(`IncomingDamage`)은 쓰지 않는다. 감소 계산은 매니저가 적용 전에 한다
- 2026-11-16: 무기 메시(`WeaponMesh`)와 판정 컴포넌트(`MeleeTrace`)는 `ACombatCharacterBase`가 C++로 가진다. 블루프린트는 검 메시 에셋만 고르고, 더미는 비워 둔다
- 2026-11-16: 콤보는 같은 `GA_MeleeAttack` 실행 안에서 `CurrentAttack`과 몽타주를 바꿔 이어 간다 (4-4). 새로 활성화 방식은 스펙 중복 반응과 `State.Attacking` 공백 문제로 쓰지 않는다
- 2026-11-16: 입력 버퍼는 캐릭터(입력 요청 입구 `AbilityInputTagPressed`)에 둔다. 실행 중인 어빌리티가 꺼내 간다. 다른 스펙의 입력과 회피 선입력도 같은 버퍼로 처리
- 2026-11-16: 입력 버퍼 보관 시간은 캐릭터마다 하나(`InputBufferDuration`). 공격 데이터에 두지 않는다
- 2026-11-16: 깃 버전 검 메시는 Modeling Tools로 만든 임시 검(`SM_Sword_Placeholder`). 기능 구현 수준 기준이고, 소켓 이름만 지키면 메시는 나중에 교체 가능
- 2026-11-16: `GA_MeleeAttack`의 시작 공격 데이터는 플레이어는 부여 목록(스펙 SourceObject), AI는 공격 이벤트 페이로드로 넘긴다. 페이로드 우선. 페이로드 경로는 Step 7에서 추가
- 2026-11-15: 저장소는 공개로 만든다. 공개 전에 이력에서 지원 회사 언급을 지우고, 커밋 이메일은 GitHub noreply로 바꿨다. 앞으로도 저장소에 회사·공고 이름을 쓰지 않는다
- 2026-11-15: Fab 후보 리타깃 검증은 9-3(유료 에셋 선정 시점)으로 미룬다. 유료 에셋이 미정이고, 후보 팩이 UE5 마네킹 기반이라 리타깃 위험이 낮다
