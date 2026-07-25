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
  # ★ -NullRHI 금지: 그 모드에서 액터 스폰이 EXCEPTION_INT_DIVIDE_BY_ZERO로 죽는다. -RenderOffScreen 을 쓴다.
  RASEL_LEVEL=$i "$UE" "$PROJ" -ExecutePythonScript="$PY" -RenderOffScreen -unattended -nosplash -nopause > /dev/null 2>&1
  ok=$((ok+1))
  echo "[$i] done  (맵 누적: $(ls /c/Secret_Project/Content/Maps/Rasel/ 2>/dev/null | wc -l))"
done
echo "=== $ok회 실행 · 최종 맵 $(ls /c/Secret_Project/Content/Maps/Rasel/ 2>/dev/null | wc -l)개 ==="
