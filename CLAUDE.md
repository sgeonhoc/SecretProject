# Secret_Project — Claude 행동 지침

## 세션 시작 시 필수 행동
1. **반드시 `C:\Secret_Project\DevLog.md` 를 먼저 읽을 것**
2. 읽기 전까지 어떤 작업 지시도 하지 말 것
3. 현재 진행 상태, 잘못된 작업 내역, 이전 지시 내용을 파악한 후 작업 시작

## 폴더 구조 (2026-07-25 정리 — 루트에 파일을 새로 흩뿌리지 말 것)

```
C:\Secret_Project\
├ 기획/          설정·설계 정본 md  ← 글은 전부 여기
│  ├ 00_지시/      사용자 지시 원문 (훅이 절대경로로 읽음 — 옮기지 말 것)
│  ├ 01_세계관/    0_색인 · 1_고대사 · 3_현대 · 4_체계 · 9_구작업
│  ├ 02_게임설계/  1_전투 · 2_레벨 …
│  ├ 03_규약/      작업 규약·가이드 (이 구조 문서 포함)
│  ├ 04_외부대화/  노트북LM·제미나이 등 외부 산출물 (정본 아님)
│  └ 99_보관/      끝난 것
├ 웹/            열람용 html 49 + html 이 읽는 데이터 js 4  ← 빌드 산출물, 직접 고치지 말 것
├ 도구/
│  ├ 웹빌더/       md → html 생성기 (node)
│  ├ 언리얼/       UE 에디터 파이썬 (레벨·에셋·위젯 생성)
│  ├ 진단/         검수·촬영·집계 (읽기 위주)
│  └ 보관/         끝난 일회용 (citoon 셀룩·Tripo·test7)
├ Content/ Config/ Source/ Plugins/ Binaries/ Saved/
├ 열람.html      ← 웹 페이지 49장 진입점 (node 도구/웹빌더/_build_열람index.js 로 재생성)
├ CLAUDE.md  DevLog.md  Secret_Project.uproject
```

**규칙**
- 새 md 는 `기획/` 아래 알맞은 갈래에. 루트에 두지 말 것.
- 새 스크립트는 `도구/` 아래 갈래에. node 빌더는 `도구/웹빌더/`.
- **빌더는 프로젝트 루트에서 실행한다** — `node 도구/웹빌더/_build_살란_전서.js`
- html 은 전부 빌드 산출물이다. `웹/*.html` 을 손으로 고치면 다음 빌드에 날아간다. 정본 md 를 고치고 빌더를 돌릴 것.

## 프로젝트 개요
- **엔진:** Unreal Engine **5.7** (`.uproject` EngineAssociation "5.7")
- **구현:** **로직은 C++**(`Source/Secret_Project/`), Blueprint 는 UI·연출만
- **장르:** 페르소나 구조를 뼈대로 한 싱글플레이 턴제 RPG (자유탐험 허브 + 에피소드)
- **무대:** 라셀 — 끊기지 않은 한 공간(`/Game/Maps/Rasel/Rasel_City`)에 자리 22곳
- **스켈레톤:** `/Game/Characters/Mannequins/Meshes/SK_Mannequin`
- **애니메이션:** `/Game/ABP`(MIXAMO) · `/Game/paragonanimRTGtoCharacters`
  → 둘 다 디스크에는 있으나 **깃 추적 제외**(재다운로드 가능). 지우면 BP_PlayerCharacter 와
     BP_ANPCCharacter 30개의 애니메이션이 깨진다.

## 에셋 규율
- **외부 샘플 팩을 게임에 쓰지 않는다** — 저작권 미확인. 엔진 기본(`/Engine/BasicShapes`)이나 우리가 만든 것만.
- 팀원이 올린 `.uasset` 은 받은 그대로 쓴다(수정 금지).

## 작업 원칙
- 사용자는 Claude 지시를 그대로 따르므로 **지시가 틀리면 작업도 틀림**
- 지시 전에 논리적으로 맞는지 스스로 검토할 것
- 방향을 바꿀 때는 반드시 이유를 먼저 설명하고 동의 받을 것
- 매 작업 지시 후 DevLog에 기록할 것 (지시 내용 + 결과)
- 완료 확인은 반드시 실기(스크린샷)로 할 것 — 추측으로 ✓ 표시 금지
- 커밋·푸시는 실행 전 사용자 승인을 받을 것

## 현재 진행 상태 요약
→ DevLog.md 참조
