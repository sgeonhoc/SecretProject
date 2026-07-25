#!/usr/bin/env bash
# UE 5.7 -> 5.8 전환 + Revvy 설치. 프로젝트 루트에서 실행.
#   확인만:  bash 도구/_upgrade_to_5.8.sh
#   실제로:  bash 도구/_upgrade_to_5.8.sh --go
#
# 준비물 (2026-07-25 미리 받아둠): C:\Secret_Project_5.8_stage\
#   KawaiiPhysics_5.8_1.21.0.zip  (EngineVersion 5.8.0)
#   VRM4U_5_8_20260722.zip        (EngineVersion 5.8.0)
set -u
GO=0; [ "${1:-}" = "--go" ] && GO=1
ROOT=/c/Secret_Project
STAGE=/c/Secret_Project_5.8_stage
UE58="/c/Program Files/Epic Games/UE_5.8"
say() { printf '%s\n' "$*"; }
run() { if [ $GO -eq 1 ]; then eval "$@"; else say "   [dry] $*"; fi; }

say "=== 0. 전제 확인 ==="
# ★폴더만 보면 안 된다 — 다운로드 도중에도 폴더는 생긴다. 에디터 바이너리로 판정할 것.
UE58_EDITOR="$UE58/Engine/Binaries/Win64/UnrealEditor.exe"
if [ -f "$UE58_EDITOR" ]; then
  say "   UE 5.8 : 설치 완료 ($(du -sh "$UE58" 2>/dev/null | cut -f1))"
else
  say "   UE 5.8 : ★아직 아님 — 폴더는 있으나 UnrealEditor.exe 가 없다 (다운로드 중, 현재 $(du -sh "$UE58" 2>/dev/null | cut -f1))"
  [ $GO -eq 1 ] && exit 1
fi
[ -f "$STAGE/KawaiiPhysics_5.8_1.21.0.zip" ] && say "   KP 5.8 : 있음" || say "   KP 5.8 : ★없음"
[ -f "$STAGE/VRM4U_5_8_20260722.zip" ]      && say "   VRM 5.8: 있음" || say "   VRM 5.8: ★없음"
[ -d /c/Revvy ] && say "   Revvy  : 있음" || say "   Revvy  : ★없음"
cd "$ROOT" || exit 1
dirty=$(git status --porcelain | wc -l)
say "   미커밋 : $dirty (0이어야 안전 — 5.8로 열면 5.7로 못 돌아옴)"
[ "$dirty" -ne 0 ] && [ $GO -eq 1 ] && { say "   ★미커밋이 있어 중단. 먼저 커밋할 것."; exit 1; }

say ""
say "=== 1. 현재 플러그인 백업 ==="
run "mkdir -p '$STAGE/backup_5.7'"
for p in KawaiiPhysics VRM4U; do
  run "cp -r '$ROOT/Plugins/$p' '$STAGE/backup_5.7/$p'"
  say "   backup: Plugins/$p"
done

say ""
say "=== 2. 5.8 플러그인으로 교체 ==="
run "rm -rf '$ROOT/Plugins/KawaiiPhysics' '$ROOT/Plugins/VRM4U'"
run "unzip -q -o '$STAGE/KawaiiPhysics_5.8_1.21.0.zip' -d '$ROOT/Plugins'"        # zip 안 = KawaiiPhysics/
run "unzip -q -o '$STAGE/VRM4U_5_8_20260722.zip' -d '$STAGE/_vrm_tmp'"            # zip 안 = Plugins/VRM4U/
run "cp -r '$STAGE/_vrm_tmp/Plugins/VRM4U' '$ROOT/Plugins/VRM4U'"
run "rm -rf '$STAGE/_vrm_tmp'"

say ""
say "=== 3. Revvy 설치 (Binaries/Intermediate 제외) ==="
run "rm -rf '$ROOT/Plugins/Revvy'"
run "mkdir -p '$ROOT/Plugins/Revvy'"
run "rsync -a --exclude Binaries --exclude Intermediate --exclude .git /c/Revvy/ '$ROOT/Plugins/Revvy/'"

say ""
say "=== 4. EngineAssociation 5.7 -> 5.8 ==="
run "sed -i 's/\"EngineAssociation\": \"5.7\"/\"EngineAssociation\": \"5.8\"/' '$ROOT/Secret_Project.uproject'"

say ""
say "=== 5. UE_5.7 경로가 박힌 파일 치환 (DevLog.md 는 기록이라 건드리지 않음) ==="
for f in Build_Editor.bat PLAY.bat \
         "기획/03_규약/캐릭터_셀룩_파이프라인.md" \
         "도구/언리얼/grid_tool.py" "도구/언리얼/setup_test_battle.py" \
         "도구/언리얼/_make_rasel_maps.sh" "도구/언리얼/_wire_rasel_maps.sh"; do
  if [ -f "$ROOT/$f" ]; then
    n=$(grep -c "UE_5\.7" "$ROOT/$f" 2>/dev/null || echo 0)
    say "   $f  ($n 곳)"
    run "sed -i 's|UE_5\\.7|UE_5.8|g' '$ROOT/$f'"
  fi
done

say ""
say "=== 6. 빌드 산출물 청소 (5.7 바이너리는 5.8에서 못 씀) ==="
for d in Binaries Intermediate .vs DerivedDataCache; do
  [ -e "$ROOT/$d" ] && { say "   rm $d"; run "rm -rf '$ROOT/$d'"; }
done
run "rm -rf '$ROOT/Plugins/'*/Binaries '$ROOT/Plugins/'*/Intermediate"

say ""
say "=== 7. 다음 (수동/확인) ==="
say "   a) .uproject 우클릭 → Generate Visual Studio project files"
say "   b) Build_Editor.bat 로 컴파일 → 5.8 API 변경으로 우리 C++ 이 깨지면 그때 고친다"
say "   c) 에디터 실행 → Edit > Plugins 에서 '레비 (Revvy)' 체크 → 재시작"
say "   d) ★Revvy 외부 앱/MCP 는 시스템 python 이 필요하나 이 PC 것은 WindowsApps 스텁이다."
say "      에디터 내장 채팅만 쓸 거면 무관. 외부 앱까지 쓰려면 python.org 판 설치 필요."
[ $GO -eq 0 ] && { say ""; say "※ 확인용으로만 돌았다. 실제 적용은 --go"; }
