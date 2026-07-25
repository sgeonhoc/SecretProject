# -*- coding: utf-8 -*-
"""라셀 텍스처 발생기 — 전부 우리가 저작. 외부 에셋·외부 라이브러리 0.
표준 zlib/struct만으로 PNG를 직접 인코딩하고, 절차 패턴으로 색/높이를 만든다.
높이맵에서 노멀맵을 유도하고, 거칠기맵도 같이 뽑는다.

실행: "<UE>/Engine/Binaries/ThirdParty/Python3/Win64/python.exe" _rasel_tex.py
산출: C:/Secret_Project/_ArtSource/Textures/*.png   (임포트는 _rasel_import_tex.py)
"""
import math, os, struct, zlib, random

OUT = "C:/Secret_Project/_ArtSource/Textures"
SIZE = 1024          # 벽·바닥처럼 코앞에서 보게 되는 것은 1024, 천·종이는 아래에서 512로 낮춰 부름


# ── PNG 인코딩 (우리가 직접) ───────────────────────────────────────────
def write_png(path, w, h, rgb_rows):
    raw = bytearray()
    for row in rgb_rows:
        raw.append(0)
        raw += row

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(raw), 6))
           + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)
    return len(png)


def clamp8(v):
    return 0 if v < 0 else (255 if v > 255 else int(v))


def save_rgb(name, w, h, pix):
    """pix: [ [ (r,g,b) ... ] ... ] 0~255 float 허용"""
    rows = []
    for y in range(h):
        b = bytearray()
        for (r, g, bl) in pix[y]:
            b += bytes((clamp8(r), clamp8(g), clamp8(bl)))
        rows.append(b)
    n = write_png(os.path.join(OUT, name + ".png"), w, h, rows)
    print("  %-28s %6.1f KB" % (name + ".png", n / 1024.0))


def save_gray(name, w, h, val):
    pix = [[(val[y][x] * 255.0,) * 3 for x in range(w)] for y in range(h)]
    save_rgb(name, w, h, pix)


# ── 이어붙어도 티 안 나는(타일링) 값잡음 ────────────────────────────────
def _hash2(ix, iy, seed):
    n = (ix * 374761393 + iy * 668265263 + seed * 1442695040888963407) & 0xFFFFFFFF
    n = (n ^ (n >> 13)) * 1274126177 & 0xFFFFFFFF
    return ((n ^ (n >> 16)) & 0xFFFFFF) / 16777215.0


def _smooth(t):
    return t * t * (3.0 - 2.0 * t)


def vnoise(x, y, period, seed):
    """격자를 period로 감아 이어붙였을 때 이음매가 안 보이는 값잡음."""
    ix, iy = int(math.floor(x)), int(math.floor(y))
    fx, fy = x - ix, y - iy
    sx, sy = _smooth(fx), _smooth(fy)
    x0, x1 = ix % period, (ix + 1) % period
    y0, y1 = iy % period, (iy + 1) % period
    a = _hash2(x0, y0, seed); b = _hash2(x1, y0, seed)
    c = _hash2(x0, y1, seed); d = _hash2(x1, y1, seed)
    return (a + (b - a) * sx) * (1 - sy) + (c + (d - c) * sx) * sy


def fbm(u, v, base, octaves, seed, gain=0.5):
    """u,v = 0~1. base = 첫 옥타브 격자 수(=타일 주기)."""
    total, amp, norm, per = 0.0, 1.0, 0.0, base
    for o in range(octaves):
        total += amp * vnoise(u * per, v * per, per, seed + o * 101)
        norm += amp
        amp *= gain
        per *= 2
    return total / norm


# ── 높이맵 → 노멀맵 (중앙차분) ─────────────────────────────────────────
def height_to_normal(h, w, hh, strength=2.0):
    pix = []
    for y in range(hh):
        row = []
        ym, yp = (y - 1) % hh, (y + 1) % hh
        for x in range(w):
            xm, xp = (x - 1) % w, (x + 1) % w
            dx = (h[y][xm] - h[y][xp]) * strength
            dy = (h[ym][x] - h[yp][x]) * strength
            nz = 1.0
            l = math.sqrt(dx * dx + dy * dy + nz * nz)
            row.append(((dx / l * 0.5 + 0.5) * 255,
                        (dy / l * 0.5 + 0.5) * 255,
                        (nz / l * 0.5 + 0.5) * 255))
        pix.append(row)
    return pix


def blank(w, h, v=0.0):
    return [[v] * w for _ in range(h)]


# ── ① 돌 포장 — 큰길 바닥 ──────────────────────────────────────────────
def tex_cobble(size=SIZE, cells=9, seed=7):
    """자리 흐트러진 격자 특징점으로 돌 낱장을 나누고, 사이는 파인 줄눈."""
    print("[돌 포장]")
    pts = []
    for cy in range(cells):
        for cx in range(cells):
            jx = _hash2(cx, cy, seed) - 0.5
            jy = _hash2(cx, cy, seed + 55) - 0.5
            pts.append((cx + 0.5 + jx * 0.75, cy + 0.5 + jy * 0.75, cx, cy))
    grid = {}
    for p in pts:
        grid[(p[2], p[3])] = p

    col = [[None] * size for _ in range(size)]
    hgt = blank(size, size)
    rgh = blank(size, size)
    for y in range(size):
        v = y / float(size) * cells
        for x in range(size):
            u = x / float(size) * cells
            gx, gy = int(u), int(v)
            d1, d2, owner = 9e9, 9e9, None
            for oy in (-1, 0, 1):
                for ox in (-1, 0, 1):
                    p = grid[((gx + ox) % cells, (gy + oy) % cells)]
                    px = p[0] + ox * 0 + ((gx + ox) - ((gx + ox) % cells))
                    py = p[1] + ((gy + oy) - ((gy + oy) % cells))
                    dx, dy = u - px, v - py
                    d = dx * dx + dy * dy
                    if d < d1:
                        d2, d1, owner = d1, d, p
                    elif d < d2:
                        d2 = d
            edge = math.sqrt(d2) - math.sqrt(d1)          # 낱장 경계까지
            mortar = 1.0 - min(1.0, edge / 0.16)           # 줄눈 골
            n = fbm(x / float(size), y / float(size), 32, 4, seed + 3)
            tone = _hash2(owner[2], owner[3], seed + 9)
            base = 96 + tone * 34 + (n - 0.5) * 26         # 회갈색 돌
            base -= mortar * 46                            # 줄눈은 어둡게
            r = base * 1.03; g = base * 0.99; b = base * 0.93
            col[y][x] = (r, g, b)
            hgt[y][x] = (1.0 - mortar) * 0.8 + n * 0.2
            rgh[y][x] = 0.62 + mortar * 0.22 + (n - 0.5) * 0.1
    save_rgb("T_Rasel_Cobble_C", size, size, col)
    save_rgb("T_Rasel_Cobble_N", size, size, height_to_normal(hgt, size, size, 3.0))
    save_gray("T_Rasel_Cobble_R", size, size, rgh)


# ── ② 회벽 — 때 타고 갈라진 구시가 벽 ──────────────────────────────────
def tex_plaster(size=SIZE, seed=21):
    print("[회벽]")
    col = [[None] * size for _ in range(size)]
    hgt = blank(size, size)
    rgh = blank(size, size)
    for y in range(size):
        vv = y / float(size)
        for x in range(size):
            uu = x / float(size)
            n = fbm(uu, vv, 8, 5, seed)
            fine = fbm(uu, vv, 64, 3, seed + 40)
            stain = fbm(uu * 0.6, vv * 2.2, 6, 4, seed + 80)
            wet = max(0.0, (vv - 0.62) / 0.38) ** 1.6      # 아랫도리 물때

            # 실금 — 머리카락 굵기. 벽 전체가 아니라 두어 군데에만 몰린다
            zone = min(1.0, max(0.0, (fbm(uu, vv, 3, 3, seed + 300) - 0.46) / 0.2))
            cr = fbm(uu, vv, 4, 5, seed + 120)
            crack = (1.0 - min(1.0, abs(cr - 0.5) / 0.0055)) * zone
            cr2 = fbm(uu * 1.6 + 0.31, vv * 1.6, 7, 4, seed + 160)
            crack = max(crack, (1.0 - min(1.0, abs(cr2 - 0.5) / 0.0035)) * zone * 0.8)

            # 회가 떨어져 나가 아래 벽돌 켜가 드러난 자리
            sp = fbm(uu, vv, 5, 4, seed + 200)
            spall = min(1.0, max(0.0, (sp - 0.685) / 0.045))
            rim = min(1.0, max(0.0, (sp - 0.66) / 0.025)) * (1.0 - spall)

            base = 176 + (n - 0.5) * 40 + (fine - 0.5) * 14
            base -= wet * (26 + stain * 34)
            base -= crack * 46
            base -= rim * 30                               # 떨어진 자리 가장자리 그늘
            r = base * 1.0; g = base * 0.985; b = base * 0.945
            if spall > 0.0:                                # 드러난 벽돌 = 붉은 회갈 + 켜
                bx = uu * 11.0
                by = vv * 26.0
                brow = int(by)
                bxs = bx + 0.5 * (brow % 2)
                jx = min(bxs - int(bxs), 1.0 - (bxs - int(bxs)))
                jy = min(by - brow, 1.0 - (by - brow))
                bjoint = 1.0 - min(1.0, min(jx / 0.055, jy / 0.13))
                bn = fbm(uu * 3.0, vv * 3.0, 48, 3, seed + 240)
                br = 128 + _hash2(int(bxs), brow, seed + 7) * 26 + (bn - 0.5) * 20
                br -= bjoint * 40
                r = r * (1 - spall) + br * 1.00 * spall     # 오래 삭아 바랜 벽돌
                g = g * (1 - spall) + br * 0.84 * spall
                b = b * (1 - spall) + br * 0.76 * spall
            col[y][x] = (r, g, b)
            hgt[y][x] = (n * 0.55 + fine * 0.45) - crack * 0.6 - spall * 0.45
            rgh[y][x] = 0.78 + (fine - 0.5) * 0.14 + wet * 0.08 + spall * 0.1
    save_rgb("T_Rasel_Plaster_C", size, size, col)
    save_rgb("T_Rasel_Plaster_N", size, size, height_to_normal(hgt, size, size, 1.1))
    save_gray("T_Rasel_Plaster_R", size, size, rgh)


# ── ③ 나무 — 문틀·좌판·기둥 ────────────────────────────────────────────
def tex_wood(size=SIZE, seed=33):
    """켠 판재 — 결이 길이 방향으로 곧게 흐르고, 옹이가 드문드문, 판과 판 사이에 이음매."""
    print("[나무]")
    col = [[None] * size for _ in range(size)]
    hgt = blank(size, size)
    rgh = blank(size, size)

    knots = [(_hash2(i, 3, seed + 71), _hash2(i, 9, seed + 73),
              0.020 + _hash2(i, 17, seed + 79) * 0.022) for i in range(5)]

    for y in range(size):
        vv = y / float(size)
        for x in range(size):
            uu = x / float(size)
            # 판 나누기 — 세로로 켠 널 다섯 장
            plank = uu * 5.0
            pid = int(plank)
            pt = plank - pid
            seam = 1.0 - min(1.0, min(pt, 1.0 - pt) / 0.018)

            # 결 — 길이(세로) 방향으로 흐르는 촘촘한 줄
            warp = (fbm(uu * 6.0, vv * 0.35, 16, 4, seed) - 0.5) * 0.9
            fine = math.sin((uu * 46.0 + warp * 7.0 + pid * 3.1) * math.pi) * 0.5 + 0.5
            fine = fine ** 1.4
            long_ = fbm(uu * 8.0, vv * 0.30, 40, 3, seed + 12)

            # 옹이 — 몇 군데만, 둘레에 결이 휘어 감김
            knot = 0.0
            for (kx, ky, kr) in knots:
                dx, dy = (uu - kx), (vv - ky) * 0.55
                d = math.sqrt(dx * dx + dy * dy)
                if d < kr * 3.6:
                    # 둘레를 잡음으로 흔들어 동심원 과녁처럼 안 보이게
                    wob = (fbm(uu * 5.0, vv * 5.0, 20, 3, seed + 91) - 0.5) * kr * 1.1
                    ring = math.sin((d + wob) / kr * 8.0) * 0.5 + 0.5
                    knot = max(knot, (1.0 - min(1.0, d / (kr * 3.6))) ** 1.3
                               * (0.4 + ring * 0.6))

            tone = 0.86 + _hash2(pid, 0, seed + 5) * 0.28      # 널마다 색이 조금씩 다름
            base = (96 + fine * 26 + (long_ - 0.5) * 30) * tone
            base -= knot * 46
            base -= seam * 40
            r = base * 1.16; g = base * 0.90; b = base * 0.66
            col[y][x] = (r, g, b)
            hgt[y][x] = fine * 0.34 + long_ * 0.34 - seam * 0.6 - knot * 0.25 + 0.3
            rgh[y][x] = 0.68 + (1.0 - fine) * 0.14 + knot * 0.1
    save_rgb("T_Rasel_Wood_C", size, size, col)
    save_rgb("T_Rasel_Wood_N", size, size, height_to_normal(hgt, size, size, 1.4))
    save_gray("T_Rasel_Wood_R", size, size, rgh)


# ── ④ 기와 — 청록 지붕 ─────────────────────────────────────────────────
def tex_roof(size=SIZE, cols=7, rows=9, seed=44):
    """처마 쪽으로 흘러내리는 골 — 엎은 기와(볼록)와 받는 기와(오목)가 번갈아 가고,
    가로로는 한 단씩 겹쳐 물린 자리에 그늘이 진다."""
    print("[기와]")
    col = [[None] * size for _ in range(size)]
    hgt = blank(size, size)
    rgh = blank(size, size)
    for y in range(size):
        vv = y / float(size)
        fy = vv * rows
        ry = int(fy)
        ty = fy - ry
        lap = 1.0 - min(1.0, ty / 0.16)                    # 윗단이 덮은 그늘
        step = min(1.0, (1.0 - ty) / 0.1)                  # 단 끝 두께 턱
        for x in range(size):
            uu = x / float(size)
            fx = uu * cols
            cx = int(fx)
            tx = fx - cx
            half = (tx * 2.0) % 1.0
            convex = (tx < 0.5)
            prof = math.sin(half * math.pi)
            bump = prof if convex else -prof * 0.75        # 볼록 / 오목
            n = fbm(uu, vv, 40, 3, seed)
            moss = max(0.0, fbm(uu * 1.4, vv * 1.4, 7, 4, seed + 30) - 0.52) * 2.0
            shade = _hash2(cx, ry, seed + 5)
            base = 62 + bump * 30 + shade * 12 + (n - 0.5) * 13
            base -= lap * 26
            base += (1.0 - step) * 10
            r = base * 0.70; g = base * 1.04; b = base * 0.95   # 청록
            r += moss * 6; g += moss * 16; b -= moss * 4        # 골에 낀 이끼
            col[y][x] = (r, g, b)
            hgt[y][x] = (bump * 0.5 + 0.5) * 0.78 + (1.0 - lap) * 0.14 + n * 0.08
            rgh[y][x] = 0.55 + (n - 0.5) * 0.16 + moss * 0.18
    save_rgb("T_Rasel_Roof_C", size, size, col)
    save_rgb("T_Rasel_Roof_N", size, size, height_to_normal(hgt, size, size, 2.6))
    save_gray("T_Rasel_Roof_R", size, size, rgh)


# ── ⑤ 옛 돌 — 벽돌 밑에서 드러나는 아래층 석축 (§7-3) ───────────────────
def tex_oldstone(size=SIZE, seed=61):
    print("[옛 돌]")
    bw, bh = 4, 8                                          # 큼직한 켜쌓기
    col = [[None] * size for _ in range(size)]
    hgt = blank(size, size)
    rgh = blank(size, size)
    for y in range(size):
        fy = y / float(size) * bh
        ry = int(fy)
        ty = fy - ry
        shift = 0.5 * (ry % 2)
        for x in range(size):
            fx = x / float(size) * bw + shift
            cx = int(fx)
            tx = fx - cx
            jx = min(tx, 1.0 - tx)
            jy = min(ty, 1.0 - ty)
            joint = 1.0 - min(1.0, min(jx / 0.045, jy / 0.09))
            rough = fbm(x / float(size) * 2, y / float(size) * 2, 56, 4, seed)
            tone = _hash2(cx, ry, seed + 3)
            base = 118 + tone * 26 + (rough - 0.5) * 30
            base -= joint * 52
            r = base * 1.0; g = base * 0.97; b = base * 0.9
            col[y][x] = (r, g, b)
            hgt[y][x] = (1.0 - joint) * 0.7 + rough * 0.3
            rgh[y][x] = 0.84 + (rough - 0.5) * 0.12
    save_rgb("T_Rasel_OldStone_C", size, size, col)
    save_rgb("T_Rasel_OldStone_N", size, size, height_to_normal(hgt, size, size, 2.2))
    save_gray("T_Rasel_OldStone_R", size, size, rgh)


# ── ⑥ 천 — 좌판 차양 (줄무늬 + 올) ─────────────────────────────────────
def tex_canvas(name, rgb_a, rgb_b, size=SIZE, stripes=6, seed=77):
    print("[천 %s]" % name)
    col = [[None] * size for _ in range(size)]
    hgt = blank(size, size)
    for y in range(size):
        vv = y / float(size)
        for x in range(size):
            uu = x / float(size)
            s = 1.0 if (int(uu * stripes) % 2) else 0.0
            weave = (math.sin(uu * size * 0.5) * math.sin(vv * size * 0.5)) * 0.5 + 0.5
            n = fbm(uu, vv, 24, 3, seed)
            a = [rgb_a, rgb_b][int(s)]
            k = 0.86 + weave * 0.16 + (n - 0.5) * 0.13
            col[y][x] = (a[0] * k, a[1] * k, a[2] * k)
            hgt[y][x] = weave * 0.6 + n * 0.4
    save_rgb("T_Rasel_%s_C" % name, size, size, col)
    save_rgb("T_Rasel_%s_N" % name, size, size, height_to_normal(hgt, size, size, 0.9))


# ── ⑦ 종이 — 벽보·낱장 ────────────────────────────────────────────────
def tex_paper(size=256, seed=91):
    print("[종이]")
    col = [[None] * size for _ in range(size)]
    for y in range(size):
        vv = y / float(size)
        for x in range(size):
            uu = x / float(size)
            fiber = fbm(uu * 2.0, vv * 0.5, 64, 3, seed)
            age = fbm(uu, vv, 5, 4, seed + 20)
            base = 214 + (fiber - 0.5) * 16 - age * 26
            col[y][x] = (base * 1.0, base * 0.965, base * 0.885)
    save_rgb("T_Rasel_Paper_C", size, size, col)


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    print("=== 라셀 텍스처 저작 (우리 것만) ===")
    tex_cobble()
    tex_plaster()
    tex_wood()
    tex_roof()
    tex_oldstone()
    tex_canvas("ClothA", (168, 62, 48), (196, 104, 74), size=512)   # 붉은 차양
    tex_canvas("ClothB", (54, 84, 112), (86, 118, 148), size=512)   # 푸른 차양
    tex_paper()
    print("=== 완료: %s ===" % OUT)
