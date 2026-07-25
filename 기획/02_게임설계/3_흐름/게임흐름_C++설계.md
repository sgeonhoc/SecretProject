# 🎮 게임 전체 흐름 — C++ 시뮬레이션 & 설계 (A 작성, 2026-06-03)

> 사용자 지시: "게임 켜면 메인 화면 나오지? 메인에 뭐 나오고, 버튼 누르면 뭐가 되고, 스토리 진행하며 생기는 모든 상호작용을 미리 구상해 코드로 구현. 에디터 설정은 최대한 C++로 옮기고, 나는 애니메이션/에셋 이름만 바꾸게."
>
> 이 문서 = **혼자 게임을 켜서 끝까지 돌리는 시뮬레이션** + 각 단계가 **어느 C++ 클래스/함수**로 구현됐는지 + **사용자가 채울 것(에셋 슬롯)만** 표시. 로직은 전부 C++, 사용자는 이름/에셋만.

---

## 🟢 0. 게임 부팅 → 타이틀 → 메인 화면
**시뮬레이션:** 게임 실행 → 타이틀 레벨 로드 → **메인 메뉴가 자동으로 뜬다.**
- **C++:** `ATitleGameMode`(신규). 타이틀 레벨의 GameMode로 지정하면 `BeginPlay`에서 자동으로 `UMainMenuWidget::ShowMainMenu` 호출. 메뉴 클래스는 생성자에서 `/Game/UI/WBP_MainMenu`를 **FClassFinder로 자동 연결**(에디터 할당 불필요).
- **메인 메뉴 버튼 4개 (`UMainMenuWidget`, 전부 C++ 동작):**
  | 버튼 | 동작 | 구현 |
  |---|---|---|
  | `Btn_NewGame` | (옵션) 세이브 삭제 후 본편 레벨로 | `OnNewGameClicked`→`StartGame`(OpenLevel) |
  | `Btn_Continue` | 본편 레벨로(세이브는 BeginPlay 자동로드). 세이브 없으면 자동 비활성 | `OnContinueClicked` |
  | `Btn_Settings` | 설정 화면 | `OpenSettings`(BP 연출) |
  | `Btn_Quit` | 게임 종료 | `OnQuitClicked` |
- **사용자가 할 최소 작업:** ①빈 타이틀 레벨 1개 생성(.umap은 에디터로만 가능) ②그 레벨 World Settings → GameMode Override = `ATitleGameMode` ③Project Settings → Game Default Map = 타이틀 레벨 (또는 A가 `DefaultEngine.ini`에 설정). `WBP_MainMenu`의 `GameplayLevelName`을 본편 레벨 이름으로.
- **연출만 BP:** 등장 애니(`PlayIntro`), 배경/타이틀 아트.

## 🟢 1. New Game/Continue → 본편 진입
**시뮬레이션:** 본편 레벨 로드 → 플레이어 캐릭터 스폰 → 상시 월드 HUD(날짜/시간/골드/날씨)·스토리 디렉터 작동 시작 → 세이브 있으면 진행 복원.
- **C++:** `ASecretProjectGameMode`(DefaultPawn=APlayerCharacter, Controller=ASecretProjectPlayerController). 플레이어 `BeginPlay`에서 세이브 로드 + 월드 HUD 표시(`WorldHUDClass` 지정 시) + 컴포넌트(인벤/퀘스트/시간/인연/캘린더/날씨/은행/도전과제/사회/장비/스토리디렉터) 작동.
- **사용자:** 본편 레벨에 `BP_BattleManager` 배치, NPC/탐험 액터 배치(레벨 디자인은 에디터 불가피). 위젯 클래스 연결은 `build_all_ui.py`가 자동.

## 🟢 2. 탐험 중 상호작용 (월드)
| 입력 | 동작 | 구현 |
|---|---|---|
| 이동/점프/스프린트/시점 | 캐릭터 조작 | `APlayerCharacter` (Enhanced Input) |
| **Interact (E)** | 앞의 대상과 상호작용 — NPC 대화/상점/전투진입/퀘스트수령, 보물상자/픽업/표지판/잠긴문/세이브포인트/포탈/채집/소원샘/점프패드/지역트리거 | `APlayerCharacter::Interact` + 각 Actor |
| **시스템 메뉴 (M/ESC)** | 시스템 메뉴 허브 열기 | `APlayerCharacter::OpenSystemMenu` |
- ⚠️ **남은 에디터 데이터:** Interact/SystemMenu 키는 InputAction 에셋+IMC 매핑 필요(이동/E는 이미 됨). → **A 전환 후보:** C++ 레거시 키바인드 폴백(IMC 없이 M/E 직접 바인드) — APlayerCharacter(B 경계)라 B와 조율.

## 🟢 3. 시스템 메뉴 허브 (`USystemMenuWidget`, 전부 C++)
버튼 클릭 → 해당 하위 위젯 `Open*` 호출(전부 C++ 바인딩). 위젯 클래스는 `build_all_ui.py`가 배선.
- 저장 / 불러오기 / 새 게임 / 계속
- 가방(인벤) · 퀘스트 · 상태 · 지역발견 · 적 도감 · 인연 · 빠른이동 · 은행 · 제작 · 도전과제 · 사회 스탯 · **장비**(B 추가) · **도움말**(B 추가) · **회상 저널**(Btn_Journal — B 추가 대기)
- 각 하위 위젯: 데이터 읽어 목록/버튼 표시 + 닫기 시 입력 복원. 전부 C++.

## 🟢 4. 전투 (페르소나 턴제, `ABattleManager` + `UBattleHUDWidget`)
**시뮬레이션:** NPC 전투 진입 → 턴 순서 → 플레이어 턴에 HUD 버튼:
| 버튼 | 동작 |
|---|---|
| 공격/방어/차지/도망 | 기본 행동 |
| 스킬 → 서브메뉴(0~5) | `PlayerUseSkill` (속성/연타/상태이상/즉사/회복/버프) |
| 아이템 → 서브메뉴 | `PlayerUseItem` (효과 라벨 표시, KeyItem 제외) |
| 타겟/아군 선택 | `SetEnemyTarget`/`SetAllyTarget` |
| 총공격(전멸시)/바톤(원모어시) | 조건부 표시 |
- **★ 스킬별 애니메이션(신규):** `FSkillDef.SkillMontage` — 스킬마다 몽타주 에셋 지정 시 시전자가 **그 애니메이션 재생**(C++ `PerformAttack`이 자동). 비우면 기본 공격 모션 폴백. 적/아군 대칭. **→ 사용자는 캐릭터 BP의 Skills 배열에서 스킬별 SkillMontage에 애니메이션만 넣으면 됨.**
- **연출 신호(Flair):** `OnBattleFlair` 델리게이트가 WEAK!/CRITICAL!/TECHNICAL!/1 MORE!/COUNTER! 등을 방송 → HUD `OnFlair`(BP가 팝업/사운드 구현). 타이밍/판정은 C++, 연출 에셋만 BP.
- **기본/피격 모션:** `BasicAttackMontages`/`HitReactionMontages` 배열(사용자가 몽타주 채움).

## 🟢 5. 스토리 진행 (전부 C++ 자동)
**시뮬레이션:** 일상(취침/시간대 변화)으로 날이 흐르면 → 스토리 비트가 **알아서 떠오른다**(프롤로그→각성→동료→괴담/악몽→보스→결말, 인연 에피소드 40+).
- **C++:** `UStoryDirectorComponent`(플레이어 부착, build_all_ui.py가 SCS 추가) → 시간 변화 구독 → `UStoryManagerSubsystem.GetNextAvailableBeat` → **`UStoryWidget` 자동 표시(타자기 효과, Next/Skip/클릭)** → 다 보면 보상/플래그/저장+다음 비트 연쇄. BP 그래프 0.
- **게이팅:** 보스 처치(`SetFlag`)·인연 랭크(`GetRank`)·날짜(`MinDay`)·단서(`Clue_*`)로 비트 해금. 전투/인연/탐험이 스토리를 연다.
- **회상 저널(`UStoryJournalWidget`):** 본 비트 다시보기(Director=null로 표시만).
- **사용자:** 대사는 전부 C++(StoryManager 카탈로그). 표시 위젯 레이아웃/연출만 BP. 비트 추가는 A.

## 🟢 6. 세이브/로드
- `USecretSaveGame` 슬롯0: 레벨/스탯/골드/인벤/퀘스트/시간/인연/날씨/은행/도감/장비(EquippedIds) 등. 스토리는 자체 슬롯("StorySave"). 사회스탯 자체 슬롯.
- 저장: 시스템메뉴·세이브포인트·전투승리. 로드: BeginPlay 자동.

---

## 📊 에디터 → C++ 전환 현황표
| 항목 | 상태 | 비고 |
|---|---|---|
| 게임 로직(전투/스토리/월드/메뉴) | ✅ 전부 C++ | |
| UI 버튼 바인딩·표시·동작 | ✅ 전부 C++ | BindWidgetOptional |
| 위젯 생성+클래스 배선 | ✅ `build_all_ui.py` 1회 | BP 에셋은 런타임 C++ 생성 불가 → 스크립트 |
| 타이틀→메인메뉴 자동 표시 | ✅ `ATitleGameMode`(신규) | 빈 타이틀 레벨만 사용자 생성 |
| 메인메뉴 클래스 연결 | ✅ C++ FClassFinder 자동 | 에디터 할당 불필요 |
| 스킬별 애니메이션 | ✅ `FSkillDef.SkillMontage` C++ 재생 | **사용자는 몽타주 에셋만 지정** |
| 기본/피격 모션 | ✅ C++ 재생 | 사용자는 몽타주 배열만 |
| 전투 연출(WEAK!/팝업/사운드) | ⚙️ C++ 신호 + BP 연출 | 타이밍 C++, 아트/사운드 BP |
| 입력 키(시스템메뉴/Interact) | ⚠️ IA/IMC 에셋 | **A 전환 후보: C++ 레거시 키바인드 폴백** (B 조율) |
| Game Default Map / GameMode | ⚠️ Project Settings 또는 `DefaultEngine.ini` | A가 config 편집 가능(레벨명 확정 후) |
| 레벨 디자인/메시/애니 에셋 | 🚫 에디터 불가피 | 사용자가 배치·에셋 제작 |

## 🔜 A 다음 전환 후보 (코드로 더 옮길 것)
1. **입력 키 C++ 폴백** — IMC 없이도 M(시스템메뉴)/E(Interact)/Tab(저널) 동작하게 레거시 `BindKey`(APlayerCharacter, B 조율).
2. **`DefaultEngine.ini`** — GlobalDefaultGameMode/GameDefaultMap 설정(타이틀 레벨명 확정 시).
3. **비-데미지 스킬(회복/버프) 애니메이션** — 현재 데미지 스킬만 SkillMontage 재생. 회복/버프도 시전 모션 추가.
4. **연출 기본값** — Flair 종류별 기본 사운드/색을 C++ 데이터로(사용자는 에셋만 교체).
