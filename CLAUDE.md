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
5. 기본 세트의 애니메이션은 **무료이면서 공개 저장소에 원본 재배포가 허용된 것만** 쓴다. 현재는 CC0 에셋(Quaternius UAL2/UAL1 무료판, 검 메시는 LowPoly Medieval Weapons)만 쓴다. 깃 버전은 기능이 동작하는 것을 보여 주는 수준이고, 자연스러운 동작은 배포 빌드에서 보여 준다. 모든 외부 출처는 `Docs/Credits.md`에 적는다. Fab 무료 에셋, Epic 샘플(GASP, Lyra), Mixamo는 깃에 넣지 않는다.
6. Release 세트 선택은 `UPortfolioContentSettings`가 하고, 로드 실패 시 Default로 폴백한다.

### 5.1 소켓 / 노티파이 / 에셋 이름 규약 (초안, 스텝 진행 중 확정)

| 이름 | 용도 |
|---|---|
| `A_` 접두사 | 애니메이션 시퀀스. 외부 팩은 출처를 붙인다 (`A_UAL2_Sword_Regular_A`) |
| `AM_` 접두사 | 애니메이션 몽타주 |
| `IK_`, `RTG_` 접두사 | IK Rig, IK Retargeter (`IK_UAL2`, `IK_Manny`, `RTG_UAL2_Manny`) |
| `weapon_r` (캐릭터 `hand_r` 본에 추가) | 검 부착 위치 |
| `blade_start`, `blade_end` (검 메시 소켓) | 판정 스윕 소켓 쌍. 메시를 바꿔도 이름은 유지 |
| `Window.Combo`, `Window.Cancel`, `State.Invulnerable` | `ANS_TagWindow`가 붙이는 태그 |

---

## 6. 폴더 구조 (목표)

```
Source/FirstAction/
├── Public/
│   ├── Combat/
│   │   ├── CombatTags.h               // 네이티브 태그
│   │   ├── CombatCharacterBase.h      // ASC 부착, 플레이어/적 공통
│   │   ├── CombatAttributeSet.h       // Health, Poise
│   │   ├── CombatCharacterData.h      // 질량, 부위 반응 임계값, 반응 몽타주
│   │   ├── AttackDataAsset.h
│   │   ├── HitResolveSubsystem.h      // FHitResolution, 간격 분배, 상대 충격량
│   │   ├── PartialHitReactionComponent.h // 맞은 본 아래 물리 반응
│   │   ├── MeleeTraceComponent.h
│   │   ├── LockOnComponent.h
│   │   ├── Abilities/
│   │   │   ├── GA_MeleeAttack.h
│   │   │   ├── GA_Dodge.h
│   │   │   ├── GA_Guard.h
│   │   │   └── GA_HitReact.h
│   │   └── Animation/
│   │       ├── ANS_TagWindow.h
│   │       └── ANS_HitWindow.h
│   ├── AI/
│   │   ├── EnemyPatternDataAsset.h
│   │   ├── CombatTokenSubsystem.h
│   │   └── StateTree/                 // StateTree 태스크, 조건
│   └── Content/
│       ├── CombatContentSet.h
│       └── PortfolioContentSettings.h
└── Private/   (Public과 같은 구조로 .cpp)

Content/
├── Portfolio/
│   ├── Characters/   // 플레이어, 적 블루프린트
│   ├── Data/         // 공격, 패턴, Default 콘텐츠 세트
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
- 5-6. 저 프레임(`t.MaxFPS 15`)에서 판정 누락 재현 → 서브스텝 보간으로 해결
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

### 현재 위치
- **Step 1-1** 시작 전: 플러그인과 모듈 추가

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
- 2026-11-15: 저장소는 공개로 만든다. 공개 전에 이력에서 지원 회사 언급을 지우고, 커밋 이메일은 GitHub noreply로 바꿨다. 앞으로도 저장소에 회사·공고 이름을 쓰지 않는다
- 2026-11-15: Fab 후보 리타깃 검증은 9-3(유료 에셋 선정 시점)으로 미룬다. 유료 에셋이 미정이고, 후보 팩이 UE5 마네킹 기반이라 리타깃 위험이 낮다
