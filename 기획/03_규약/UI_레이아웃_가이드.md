# 🖼️ UI 레이아웃 가이드 — 따라 그리기용 도면 (A 작성)

> **워크플로우:** `build_all_ui.py` 실행 → 위젯들이 **올바른 이름의 버튼/텍스트/패널까지 들어찬 채로** 생성됨.
> 당신은 그걸 **보기 좋게 배치·스타일만** 하면 됨 (요소를 0부터 만들 필요 없음. 이름은 이미 정확).
> 아래는 각 위젯의 **권장 배치 도면 + 요소 이름표.** 이름 안 바꾸면 C++가 자동 연결.
> ※ 이름이 정확하면 위치는 자유. 도면은 "이렇게 두면 자연스럽다" 제안.

범례: `[버튼]`  `「텍스트」`  `▢패널(컨테이너)`  `◳이미지`  `▭슬라이더`  `☑체크박스`

---

## 1) WBP_MainMenu (타이틀 메인 화면)
```
        ┌────────────────────────────┐
        │        「Txt_Title」        │   ← 게임 타이틀(예 "PERSONA")
        │                            │
        │       [Btn_NewGame]        │   새 게임
        │       [Btn_Continue]       │   이어하기(세이브 없으면 자동 비활성)
        │       [Btn_Settings]       │   설정
        │       [Btn_Quit]           │   종료
        └────────────────────────────┘
```

## 2) WBP_Settings (설정 — Btn_Settings로 열림)
```
┌──────────────────── 「Txt_Title」(설정) ────────────────────┐
│ 마스터  ▭Slider_Master      「Txt_Master」(80%)            │
│ 배경음  ▭Slider_Bgm         「Txt_Bgm」                    │
│ 효과음  ▭Slider_Sfx         「Txt_Sfx」                    │
│                                                            │
│ 화면모드 [Btn_WindowMode] 「Txt_WindowMode」  ☑Chk_VSync    │
│ 해상도  [Btn_ResPrev]◀ 「Txt_Res」 ▶[Btn_ResNext]          │
│ 글자속도[Btn_TextPrev]◀ 「Txt_TextSpeed」 ▶[Btn_TextNext]   │
│ 언어    [Btn_Language] 「Txt_Language」                     │
│                                                            │
│        [Btn_Apply]  [Btn_Reset]  [Btn_Close]               │
└────────────────────────────────────────────────────────────┘
```

## 3) WBP_BattleHUD (전투 — 가장 복잡, 핵심)
```
┌──────────────────────────────────────────────────────────┐
│ 「Txt_Status」  (HP/SP/상성 등 실시간)                      │
│ 「Txt_TurnOrder」(턴: ▶아군→적1→…)                         │
│                                                            │
│  [Btn_Attack][Btn_Skill][Btn_Guard][Btn_Charge]           │  ← 메인 액션바
│  [Btn_Item][Btn_Escape][Btn_AllOut][Btn_Baton]            │     (AllOut/Baton은 조건부 자동표시)
│                                                            │
│  ▢Panel_Skills  (평소 숨김, 스킬 누르면 표시)              │
│    [Btn_SkillOpt0]「Txt_SkillOpt0」 … 0~5                  │
│  ▢Panel_Items   (아이템 누르면)                            │
│    [Btn_ItemOpt0]「Txt_ItemOpt0」 … 0~5                    │
│  ▢Panel_Targets (적 2+일 때)                               │
│    [Btn_TargetOpt0]「Txt_TargetOpt0」 … 0~3                │
│  ▢Panel_Allies  (아군 2+일 때)                             │
│    [Btn_AllyOpt0]「Txt_AllyOpt0」 … 0~3                    │
└──────────────────────────────────────────────────────────┘
```
> 4개 패널은 평소 Collapsed(C++가 자동 토글). 디자이너는 화면 하단에 겹쳐 두면 됨.
> 연출(WEAK!/CRITICAL! 팝업)은 OnFlair 이벤트로 BP에서 애니 — 나중(STEP 연출).

## 4) WBP_SystemMenu (ESC/M로 열림 — 허브)
```
┌──────────── 「Txt_Status」 / 「Txt_Info」(Lv·골드) ─────────┐
│ [Btn_Save] [Btn_Load] [Btn_NewGame] [Btn_Resume]          │  저장/불러오기/새게임/계속
│ ── 하위 메뉴 ──                                            │
│ [Btn_Inventory][Btn_Quests][Btn_Status][Btn_Equipment]    │
│ [Btn_Discovery][Btn_Bestiary][Btn_Bond][Btn_Social]       │
│ [Btn_Travel][Btn_Bank][Btn_Craft][Btn_Achievements]       │
│ [Btn_Help]   [Btn_Journal]?(B가 추가 시)                  │
└────────────────────────────────────────────────────────────┘
```
> 각 버튼 → 해당 위젯 자동 오픈. 버튼 이름만 맞으면 됨.

## 5) WBP_Story (스토리 대사창 — 자동 표시)
```
┌──────────────────────────────────────────────────────────┐
│  ◳Img_Illust  (장면 일러스트, 배경 전체에 깔아도 됨)       │
│                                                            │
│  ◳Img_Portrait   「Txt_Speaker」(화자)   「Txt_Progress」(3/8)│
│  ┌──────────────────────────────────────────────┐         │
│  │ 「Txt_Line」 (대사 — 타자기 효과로 출력)        │         │
│  └──────────────────────────────────────────────┘         │
│                       [Btn_Next] ▶   [Btn_Skip]            │
└──────────────────────────────────────────────────────────┘
```
> 화면 아무데나 클릭해도 진행됨. Img_Illust/Portrait는 파일 있으면 자동 표시.

## 6) WBP_StoryJournal (회상 저널)
```
┌──────── 「Txt_Title」(회상 N/M) ────────┐
│ 「Txt_Entries」 (본 장면 목록)           │
│ [Btn_Beat0]「Txt_Beat0」 … 0~7 (다시보기)│
│ [Btn_Close]                             │
└──────────────────────────────────────────┘
```

## 7) WBP_Shop (상점)
```
┌──── 「Txt_Gold」(골드) / 「Txt_Message」(상인 대사) ────┐
│ [Btn_Buy0]「Txt_Item0」 … 0~5  (상품 6칸)              │
│ [Btn_Close]                                            │
└────────────────────────────────────────────────────────┘
```

## 8) WBP_Inventory (가방)
```
┌──── 「Txt_Title」(가방) ────┐
│ 「Txt_Item0」 … 0~5         │  (보유 아이템+개수)
│ [Btn_Close]                │
└────────────────────────────┘
```

## 9) WBP_QuestLog
```
┌──「Txt_Title」(퀘스트)──┐
│ 「Txt_Quest0」 … 0~3    │
│ [Btn_Close]            │
└────────────────────────┘
```

## 10) WBP_Status (상태 — 5스탯 표시)
```
┌──────────────────────┐
│ 「Txt_Stats」          │  ← Lv/HP/SP/공방/힘마력체력민첩운/골드/날짜 (C++가 한 텍스트로)
│ [Btn_Close]           │
└──────────────────────┘
```

## 11) WBP_Equipment (장비)
```
┌──── 「Txt_Equipped」(현재 장착 요약) ────┐
│ [Btn_Item0]「Txt_Item0」 … 0~7 (카탈로그)│
│ [Btn_Close]                             │
└──────────────────────────────────────────┘
```

## 12) WBP_Bank (은행)
```
┌── 「Txt_OnHand」(소지금) / 「Txt_Stored」(예치금) ──┐
│ [Btn_Deposit100][Btn_DepositAll]                  │
│ [Btn_Withdraw100][Btn_WithdrawAll]                │
│ [Btn_Close]                                       │
└────────────────────────────────────────────────────┘
```

## 13) WBP_FastTravel (빠른 이동)
```
┌── 「Txt_Title」(빠른 이동) ──┐
│ [Btn_Dest0]「Txt_Dest0」 … 0~5│
│ [Btn_Close]                 │
└──────────────────────────────┘
```

## 14) WBP_Crafting (제작)
```
┌── 「Txt_Title」(제작) ──┐
│ [Btn_Craft0]「Txt_Craft0」 … 0~5│
│ [Btn_Close]            │
└────────────────────────┘
```

## 15) WBP_WorldHUD (상시 월드 HUD — 화면 구석)
```
┌─────────────────────────┐
│ 「Txt_DayTime」 「Txt_Gold」 「Txt_Weather」 │   ← 항상 표시(작게, 모서리)
└─────────────────────────┘
```

## 16) 공통 "목록형" 위젯 — 같은 패턴
**WBP_Bond / WBP_Bestiary / WBP_Discovery / WBP_Achievements / WBP_Social / WBP_Help**
```
┌── 「Txt_Title」(제목) ──┐
│ 「Txt_Entries」(목록)   │   (Help는 Txt_Help, Discovery는 Txt_Regions)
│ [Btn_Close]            │
└────────────────────────┘
```
| 위젯 | 제목 | 목록 텍스트 | 닫기 |
|---|---|---|---|
| Bond | Txt_Title | Txt_Entries | Btn_Close |
| Bestiary | Txt_Title | Txt_Entries | Btn_Close |
| Discovery | Txt_Title | **Txt_Regions** | Btn_Close |
| Achievements | Txt_Title | Txt_Entries | Btn_Close |
| Social | Txt_Title | Txt_Entries | Btn_Close |
| Help | — | **Txt_Help** | Btn_Close |

## 17) WBP_TalkUserWidget (대화창 — 기존, 이미 있음)
```
┌── 「CharacterNameText」(NPC 이름) ──┐
│ 「DialogueText」(대사)              │   ← 이 둘은 필수(이름 정확)
│ [Btn_PersonaBattle][Btn_Shop]      │   (선택: 전투/상점 진입 버튼)
└────────────────────────────────────┘
```

---

## ✅ 따라하기 순서
1. `build_all_ui.py` 실행 → /Game/UI 에 위젯들 생성(이름 박힌 요소 포함).
2. 각 WBP 더블클릭 → 디자이너에서 위 도면처럼 **드래그로 위치만** 잡기(이름 그대로).
3. 안 보이거나 이상하면 그 위젯/요소 이름과 증상을 A에게.
4. (나중) 효과음/그림/애니 파일은 `에셋_이름규칙.md` 보고 폴더에 드롭.
