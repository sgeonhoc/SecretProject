#!/usr/bin/env python
# -*- coding: utf-8 -*-
# =====================================================================
#  grid_tool.py  —  파츠 이미지 → 통짜 그리드 1장(pack) / AI 처리 후 분할복원(split)
#  - SeaArt 등 img2img에 한 번에 올리려고 여러 파츠를 빈틈 적게 한 장으로 합침.
#  - 좌표 JSON 저장 → AI 돌린 결과 그리드를 다시 원래 파츠들로 정확히 자름.
#  - 원본 비율 보존(왜곡 X), 투명도(알파) 보존, AI가 전체 크기 바꿔도 좌표 비율로 복원.
#
#  실행(UE 번들 파이썬 권장 — Pillow가 _pylibs에 설치돼 있음):
#   합치기: "<UE>\python.exe" grid_tool.py pack  --in <폴더> --out grid.png --json grid.json [--cell 512] [--pad 0]
#   분할 :  "<UE>\python.exe" grid_tool.py split --grid <AI결과.png> --json grid.json --out <복원폴더>
#   UE python 경로 예:
#   "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\ThirdParty\Python3\Win64\python.exe"
# =====================================================================
import sys, os, json, argparse, math
sys.path.insert(0, r"C:\Secret_Project\_pylibs")   # Pillow 설치 위치
from PIL import Image


def list_pngs(d):
    return sorted([f for f in os.listdir(d) if f.lower().endswith(".png") and not f.startswith("_")])


def _fit(w, h, cell):
    """cell>0 이면 (cell x cell) 안에 들어가게 비율 유지 축소. 확대는 안 함."""
    if cell <= 0 or (w <= cell and h <= cell):
        return w, h
    s = min(cell / float(w), cell / float(h))
    return max(1, int(round(w * s))), max(1, int(round(h * s)))


def pack(indir, outimg, outjson, maxw, pad, bg, cell):
    names = list_pngs(indir)
    if not names:
        print("[pack] PNG 없음:", indir); return
    # (이름, 배치폭, 배치높이, 원본폭, 원본높이)
    items = []
    for n in names:
        with Image.open(os.path.join(indir, n)) as im:
            ow, oh = im.width, im.height
        w, h = _fit(ow, oh, cell)
        items.append((n, w, h, ow, oh))

    # 타깃 그리드 폭: 대략 정사각형이 되게 자동 산출(미지정 시)
    total_area = sum(w * h for _, w, h, _, _ in items)
    if maxw <= 0:
        maxw = int(math.sqrt(total_area) * 1.35)
    maxw = max(maxw, max(w for _, w, h, _, _ in items) + 2 * pad)

    # 높이 내림차순 셸프(행) 패킹 → 빈틈 최소화 + 바둑판 행 모양
    order = sorted(items, key=lambda t: t[2], reverse=True)
    placed = []          # (name, x, y, w, h, ow, oh)
    x = pad; y = pad; rowh = 0; used_w = pad
    for n, w, h, ow, oh in order:
        if x + w + pad > maxw and x > pad:     # 다음 행으로
            y += rowh + pad
            x = pad; rowh = 0
        placed.append((n, x, y, w, h, ow, oh))
        x += w + pad
        rowh = max(rowh, h)
        used_w = max(used_w, x)
    canvas_w = used_w + pad
    canvas_h = y + rowh + pad

    canvas = Image.new("RGB", (canvas_w, canvas_h), tuple(bg))
    manifest = {"source_dir": os.path.abspath(indir),
                "canvas": [canvas_w, canvas_h], "pad": pad, "bg": list(bg),
                "cell": cell, "parts": []}
    for n, x, y, w, h, ow, oh in placed:
        with Image.open(os.path.join(indir, n)) as im:
            has_alpha = (im.mode in ("RGBA", "LA")) or (im.mode == "P" and "transparency" in im.info)
            if (w, h) != (ow, oh):
                im = im.resize((w, h), Image.LANCZOS)
            if has_alpha:
                rgba = im.convert("RGBA")
                canvas.paste(rgba, (x, y), rgba)     # 알파로 합성
            else:
                canvas.paste(im.convert("RGB"), (x, y))
        manifest["parts"].append({"name": n, "x": x, "y": y, "w": w, "h": h,
                                  "orig_w": ow, "orig_h": oh, "alpha": bool(has_alpha)})

    os.makedirs(os.path.dirname(outimg) or ".", exist_ok=True)
    canvas.save(outimg)
    with open(outjson, "w", encoding="utf-8") as f:
        json.dump(manifest, f, ensure_ascii=False, indent=1)
    print("[pack] %d개 파츠 → %s  (%dx%d)" % (len(placed), outimg, canvas_w, canvas_h))
    print("[pack] 좌표 JSON → %s" % outjson)
    print("[pack] 이제 이 PNG를 SeaArt img2img에 올리고, 결과를 받아 split 하세요.")


def split(gridimg, injson, outdir, restore_orig_size, reapply_alpha):
    with open(injson, encoding="utf-8") as f:
        man = json.load(f)
    cw, ch = man["canvas"]
    src = man["source_dir"]
    g = Image.open(gridimg).convert("RGB")
    sx = g.width / float(cw); sy = g.height / float(ch)   # AI가 전체 크기 바꿨을 때 보정
    if abs(sx - 1) > 0.001 or abs(sy - 1) > 0.001:
        print("[split] 주의: AI 결과 크기가 원본 그리드와 다름 → 좌표 비율 보정 (%.3f, %.3f)" % (sx, sy))
    os.makedirs(outdir, exist_ok=True)
    for p in man["parts"]:
        x = int(round(p["x"] * sx)); y = int(round(p["y"] * sy))
        w = int(round(p["w"] * sx)); h = int(round(p["h"] * sy))
        piece = g.crop((x, y, x + w, y + h))
        # 출력 크기: 기본은 패킹 크기, --restore-orig 면 원본 해상도로 복원
        target = (p["orig_w"], p["orig_h"]) if restore_orig_size else (p["w"], p["h"])
        if piece.size != target:
            piece = piece.resize(target, Image.LANCZOS)
        if reapply_alpha and p.get("alpha"):
            with Image.open(os.path.join(src, p["name"])) as orig:
                a = orig.convert("RGBA").split()[3].resize(target, Image.LANCZOS)
            piece = piece.convert("RGBA"); piece.putalpha(a)
        piece.save(os.path.join(outdir, p["name"]))
    print("[split] %d개 파츠 복원 → %s" % (len(man["parts"]), outdir))


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="파츠↔그리드 합치기/분할 (img2img 배치용)")
    sub = ap.add_subparsers(dest="cmd", required=True)

    pp = sub.add_parser("pack", help="파츠 폴더 → 그리드 PNG 1장 + 좌표 JSON")
    pp.add_argument("--in",  dest="indir",  default=r"C:\Secret_Project\_tripo\our_chars\test7")
    pp.add_argument("--out", dest="outimg", default=r"C:\Secret_Project\_tripo\grids\test7_grid.png")
    pp.add_argument("--json", dest="outjson", default=r"C:\Secret_Project\_tripo\grids\test7_grid.json")
    pp.add_argument("--maxw", type=int, default=0, help="그리드 최대 폭(px). 0=자동(정사각형 근사)")
    pp.add_argument("--pad",  type=int, default=0, help="파츠 간격(px). 0=빈틈없이")
    pp.add_argument("--cell", type=int, default=512, help="파츠당 최대 변(px). SeaArt 과대 업로드 방지. 0=원본해상도")
    pp.add_argument("--bg",   default="190,190,190", help="빈 공간 배경색 R,G,B")

    sp = sub.add_parser("split", help="AI 처리한 그리드 + JSON → 파츠 복원")
    sp.add_argument("--grid", required=True, help="AI img2img 결과 그리드 PNG")
    sp.add_argument("--json", dest="injson", default=r"C:\Secret_Project\_tripo\grids\test7_grid.json")
    sp.add_argument("--out",  dest="outdir", default=r"C:\Secret_Project\_tripo\our_chars\test7_styled")
    sp.add_argument("--restore-orig", dest="restore", action="store_true", help="원본 해상도로 복원(텍스처용)")
    sp.add_argument("--no-alpha", dest="noalpha", action="store_true", help="원본 알파 재적용 끔")

    a = ap.parse_args()
    if a.cmd == "pack":
        bg = tuple(int(v) for v in a.bg.split(","))
        pack(a.indir, a.outimg, a.outjson, a.maxw, a.pad, bg, a.cell)
    else:
        split(a.grid, a.injson, a.outdir, a.restore, not a.noalpha)
