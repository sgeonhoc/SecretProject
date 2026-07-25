#!/bin/bash
# 라셀 플레이 레벨 배선 — 블록아웃 위에 연결·짚을 것·사는 사람 얹기. 레벨당 프로세스 하나.
# 실행: bash _wire_rasel_maps.sh          (전체 0~22)
#       bash _wire_rasel_maps.sh 3 3      (L04만)  /  bash _wire_rasel_maps.sh 0 6 (L01~L07)
UE="/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
PROJ="C:/Secret_Project/Secret_Project.uproject"
PY="C:/Secret_Project/도구/언리얼/_wire_rasel_maps.py"
FROM=${1:-0}
TO=${2:-22}
> /c/Secret_Project/Saved/rasel_wire.log 2>/dev/null
for i in $(seq "$FROM" "$TO"); do
  # ★2026-07-25 UE 5.8 — -RenderOffScreen 은 StylusInputWintab(Wacom)에서 죽는다. -nullrhi 사용.
  #   렌더가 필요한 촬영 스크립트에는 -nullrhi 를 쓰면 안 된다.
  RASEL_LEVEL=$i "$UE" "$PROJ" -ExecutePythonScript="$PY" -nullrhi -unattended -nosplash -nopause > /dev/null 2>&1
  echo "[$i] done"
done
echo "=== 배선 로그 ==="
cat /c/Secret_Project/Saved/rasel_wire.log 2>/dev/null
