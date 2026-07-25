#!/bin/bash
# 라셀 플레이 레벨 23개 생성 — 레벨당 에디터 프로세스 하나 (한 프로세스에 여러 개는 크래시)
# 실행: bash _make_rasel_maps.sh          (전체)
#       bash _make_rasel_maps.sh 3 7      (3~7번만)
UE="/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
PROJ="C:/Secret_Project/Secret_Project.uproject"
PY="C:/Secret_Project/도구/언리얼/_make_rasel_maps.py"
FROM=${1:-0}
TO=${2:-22}
ok=0
for i in $(seq "$FROM" "$TO"); do
  # ★2026-07-25 UE 5.8 — -RenderOffScreen 을 쓰면 StylusInputWintab(Wacom Wintab Coordinator)에서
  #   EXCEPTION_ACCESS_VIOLATION 으로 에디터가 뜨지도 못한다. 그래서 -nullrhi 로 바꿨다.
  #   5.7 시절 적어 둔 "NullRHI 는 액터 스폰이 죽는다"는 5.8 에서 재현되지 않는다(L01 액터 1834개 정상).
  #   ※ 화면을 찍는 스크립트(_shot_*.py)는 렌더가 필요하므로 -nullrhi 를 쓰면 안 된다.
  RASEL_LEVEL=$i "$UE" "$PROJ" -ExecutePythonScript="$PY" -nullrhi -unattended -nosplash -nopause > /dev/null 2>&1
  ok=$((ok+1))
  echo "[$i] done  (맵 누적: $(ls /c/Secret_Project/Content/Maps/Rasel/ 2>/dev/null | wc -l))"
done
echo "=== $ok회 실행 · 최종 맵 $(ls /c/Secret_Project/Content/Maps/Rasel/ 2>/dev/null | wc -l)개 ==="
