# -*- coding: utf-8 -*-
"""네르한 저택 텍스처 발생기 — 전부 우리가 저작. 외부 에셋·외부 라이브러리 0.
표준 zlib/struct만으로 PNG를 직접 인코딩하고, 절차 패턴으로 색/높이를 만든다.
높이맵→노멀맵, 거칠기맵도 같이 뽑는다.  (_rasel_tex.py 와 같은 결의 인프라)

산출: C:/Secret_Project/_ArtSource/Textures/T_Manor_*.png
실행: "<UE>/Engine/Binaries/ThirdParty/Python3/Win64/python.exe" _manor_tex.py
      (또는 그냥 시스템 python — 표준 라이브러리만 씀)

설계 원칙(2026-07-25 사용자): 재료 베이스색은 다양하되 밝고 깨끗하게 — 통일된 '분위기'는
레벨의 색광(푸른 새벽/달빛)이 씌운다. 그러니 텍스처는 유치하지 않게 '결·무늬·명암'만 넣고
색은 은은한 중간톤으로 둔다(빛이 물들일 여지를 남긴다).
"""
import math, os, struct, zlib

OUT = "C:/Secret_Project/_ArtSource/Textures"
SIZE = 512


# ── PNG 인코딩 ─────────────────────────────────────────────────────────
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
    rows = []
    for y in range(h):
        b = bytearray()
        for (r, g, bl) in pix[y]:
            b += bytes((clamp8(r), clamp8(g), clamp8(bl)))
        rows.append(b)
    n = write_png(os.path.join(OUT, name + ".png"), w, h, rows)
    print("  %-30s %6.1f KB" % (name + ".png", n / 1024.0))


def save_gray(name, w, h, val):
    pix = [[(val[y][x] * 255.0,) * 3 for x in range(w)] for y in range(h)]
    save_rgb(name, w, h, pix)


# ── 타일링 값잡음 ──────────────────────────────────────────────────────
def _hash2(ix, iy, seed):
    n = (ix * 374761393 + iy * 668265263 + seed * 1442695040888963407) & 0xFFFFFFFF
    n = (n ^ (n >> 13)) * 1274126177 & 0xFFFFFFFF
    return ((n ^ (n >> 16)) & 0xFFFFFF) / 16777215.0


def _smooth(t):
    return t * t * (3.0 - 2.0 * t)


def vnoise(x, y, period, seed):
    ix, iy = int(math.floor(x)), int(math.floor(y))
    fx, fy = x - ix, y - iy
    sx, sy = _smooth(fx), _smooth(fy)
    x0, x1 = ix % period, (ix + 1) % period
    y0, y1 = iy % period, (iy + 1) % period
    a = _hash2(x0, y0, seed); b = _hash2(x1, y0, seed)
    c = _hash2(x0, y1, seed); d = _hash2(x1, y1, seed)
    return (a + (b - a) * sx) * (1 - sy) + (c + (d - c) * sx) * sy


def fbm(u, v, base, octaves, seed, gain=0.5):
    total, amp, norm, per = 0.0, 1.0, 0.0, base
    for o in range(octaves):
        total += amp * vnoise(u * per, v * per, per, seed + o * 101)
        norm += amp
        amp *= gain
        per *= 2
    return total / norm


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


# ── ① 대리석 — 밝은(홀 바닥·기둥) ──────────────────────────────────────
def tex_marble(name, base, vein, gold_vein, size=SIZE, seed=13, freq=3.0):
    """휘어 흐르는 결 위에 얇은 정맥. base/vein 은 (r,g,b) 0~255.
    turbulence 로 좌표를 흔들어 sin 밴드를 접으면 대리석 결이 난다."""
    print("[대리석 %s]" % name)
    col = [[None] * size for _ in range(size)]
    hgt = blank(size, size)
    rgh = blank(size, size)
    for y in range(size):
        vv = y / float(size)
        for x in range(size):
            uu = x / float(size)
            turb = (fbm(uu, vv, 6, 5, seed) - 0.5) * 2.4
            turb2 = (fbm(uu * 2.1 + 0.3, vv * 2.1, 10, 4, seed + 40) - 0.5) * 1.1
            band = math.sin((uu * freq + vv * freq * 0.4 + turb) * math.pi)
            m = abs(band)
            # 얇고 날카로운 정맥 — band 가 0을 지나는 골
            vein_t = max(0.0, 1.0 - m / 0.09) ** 1.4
            # 두 번째 갈래(가는 곁정맥)
            band2 = math.sin((uu * freq * 2.3 - vv * freq * 0.7 + turb2) * math.pi)
            vein2 = max(0.0, 1.0 - abs(band2) / 0.05) ** 1.6 * 0.6
            gold = max(0.0, 1.0 - abs(band2 - 0.35) / 0.03) ** 1.6 * 0.5
            grain = (fbm(uu, vv, 40, 3, seed + 7) - 0.5) * 10
            base_r = base[0] + grain
            base_g = base[1] + grain
            base_b = base[2] + grain
            v = min(1.0, vein_t + vein2)
            r = base_r * (1 - v) + vein[0] * v
            g = base_g * (1 - v) + vein[1] * v
            b = base_b * (1 - v) + vein[2] * v
            r = r * (1 - gold) + gold_vein[0] * gold
            g = g * (1 - gold) + gold_vein[1] * gold
            b = b * (1 - gold) + gold_vein[2] * gold
            col[y][x] = (r, g, b)
            hgt[y][x] = 0.5 + (fbm(uu, vv, 24, 2, seed + 3) - 0.5) * 0.3 - v * 0.12
            rgh[y][x] = 0.18 + (fbm(uu, vv, 30, 2, seed + 9) - 0.5) * 0.08 + v * 0.10
    save_rgb("T_Manor_%s_C" % name, size, size, col)
    save_rgb("T_Manor_%s_N" % name, size, size, height_to_normal(hgt, size, size, 0.7))
    save_gray("T_Manor_%s_R" % name, size, size, rgh)


# ── ② 헤링본/바스켓위브 마루 — 그랜드홀 나무 바닥 ──────────────────────
def tex_parquet(size=SIZE, seed=27, blocks=4):
    """바스켓위브: 정사각 블록마다 널의 방향이 가로/세로로 번갈아 간다.
    블록 경계에 이음매 골, 널 방향으로 나뭇결."""
    print("[마루 바스켓위브]")
    col = [[None] * size for _ in range(size)]
    hgt = blank(size, size)
    rgh = blank(size, size)
    planks = 4  # 한 블록당 널 수
    for y in range(size):
        vv = y / float(size)
        for x in range(size):
            uu = x / float(size)
            bx = int(uu * blocks); by = int(vv * blocks)
            fu = uu * blocks - bx; fv = vv * blocks - by
            horizontal = (bx + by) % 2 == 0
            # 블록 경계 골
            bseam = 1.0 - min(1.0, min(fu, 1 - fu, fv, 1 - fv) / 0.03)
            if horizontal:
                along, across = fu, fv
            else:
                along, across = fv, fu
            # 널 나누기(across 방향)
            pl = across * planks
            pid = int(pl); pt = pl - pid
            pseam = 1.0 - min(1.0, min(pt, 1 - pt) / 0.05)
            # 결(along 방향으로 흐르는 줄)
            gu = along if horizontal else along
            warp = (fbm(uu * 5, vv * 5, 16, 3, seed) - 0.5) * 0.5
            grain = math.sin((along * 34.0 + warp * 6 + (bx * 7 + by * 3 + pid)) * math.pi) * 0.5 + 0.5
            grain = grain ** 1.4
            long_ = fbm(uu * 6, vv * 6, 30, 3, seed + 12)
            tone = 0.84 + _hash2(bx * 13 + pid, by * 7, seed + 5) * 0.30
            base = (104 + grain * 24 + (long_ - 0.5) * 26) * tone
            base -= max(bseam, pseam) * 46
            r = base * 1.16; g = base * 0.88; b = base * 0.60
            col[y][x] = (r, g, b)
            hgt[y][x] = 0.4 + grain * 0.2 + long_ * 0.2 - max(bseam, pseam) * 0.6
            rgh[y][x] = 0.34 + (1 - grain) * 0.12 + max(bseam, pseam) * 0.2
    save_rgb("T_Manor_Parquet_C", size, size, col)
    save_rgb("T_Manor_Parquet_N", size, size, height_to_normal(hgt, size, size, 1.1))
    save_gray("T_Manor_Parquet_R", size, size, rgh)


# ── ③ 다마스크 벽지 — 크림 바탕에 은은한 오지 격자 무늬 ────────────────
def tex_damask(size=SIZE, seed=51):
    """오지(ogee) 격자 = |접힌 좌표|로 뾰족아치 마름모를 만들고, 그 안에 이파리 무늬.
    바탕과 도안의 명도차만 은은히(같은 크림, 도안이 조금 밝고 광택 다름)."""
    print("[다마스크 벽지]")
    base = (206, 190, 150)   # 크림
    col = [[None] * size for _ in range(size)]
    hgt = blank(size, size)
    rgh = blank(size, size)
    cells = 3.0
    for y in range(size):
        vv = y / float(size)
        for x in range(size):
            uu = x / float(size)
            # 격자 셀 좌표(반 칸 엇갈림으로 다마스크 특유의 벽돌엇갈림)
            cy = vv * cells
            row = int(cy)
            cu = uu * cells + 0.5 * (row % 2)
            fu = cu - int(cu); fv = cy - row
            # 마름모(오지) 경계까지의 거리 — 뾰족아치 느낌으로 위아래 곡선
            dx = abs(fu - 0.5)
            dy = abs(fv - 0.5)
            ogee = dx + dy * 0.9 + 0.12 * math.sin(fu * math.pi * 2)
            ring = max(0.0, 1.0 - abs(ogee - 0.42) / 0.06)     # 격자 테두리
            # 중앙 이파리(작은 fbm 방울)
            leaf = max(0.0, fbm(fu * 2, fv * 2, 8, 3, seed + row * 3 + int(cu) * 5) - 0.52) * 2.2
            centre = max(0.0, 1.0 - (dx + dy) / 0.22)
            motif = min(1.0, ring * 0.8 + leaf * centre)
            grain = (fbm(uu, vv, 48, 2, seed + 3) - 0.5) * 8
            # 도안은 바탕보다 살짝 밝고 결이 다름(명도차 ~7%)
            k = 1.0 + motif * 0.10
            r = base[0] * k + grain
            g = base[1] * k + grain
            b = base[2] * k + grain
            col[y][x] = (r, g, b)
            hgt[y][x] = 0.5 + motif * 0.22
            rgh[y][x] = 0.7 - motif * 0.18     # 도안은 살짝 광택
    save_rgb("T_Manor_Damask_C", size, size, col)
    save_rgb("T_Manor_Damask_N", size, size, height_to_normal(hgt, size, size, 0.5))
    save_gray("T_Manor_Damask_R", size, size, rgh)


# ── ④ 붉은 카펫 — 결·보풀 + 테두리 문양 ────────────────────────────────
def tex_carpet(size=SIZE, seed=63):
    print("[카펫]")
    field = (120, 22, 26)    # 진홍
    figure = (150, 40, 44)   # 밝은 붉은 도안
    col = [[None] * size for _ in range(size)]
    hgt = blank(size, size)
    rgh = blank(size, size)
    for y in range(size):
        vv = y / float(size)
        for x in range(size):
            uu = x / float(size)
            # 보풀(고주파 잡음)
            pile = fbm(uu, vv, 120, 2, seed)
            # 은은한 마름모 격자 도안
            cu = (uu * 4) % 1.0; cv = (vv * 4) % 1.0
            diamond = max(0.0, 1.0 - (abs(cu - 0.5) + abs(cv - 0.5)) / 0.28)
            motif = diamond * (0.4 + 0.6 * (fbm(uu * 3, vv * 3, 10, 2, seed + 5)))
            k = 0.9 + pile * 0.2
            r = (field[0] * (1 - motif) + figure[0] * motif) * k
            g = (field[1] * (1 - motif) + figure[1] * motif) * k
            b = (field[2] * (1 - motif) + figure[2] * motif) * k
            col[y][x] = (r, g, b)
            hgt[y][x] = pile
            rgh[y][x] = 0.92 + (pile - 0.5) * 0.08
    save_rgb("T_Manor_Carpet_C", size, size, col)
    save_rgb("T_Manor_Carpet_N", size, size, height_to_normal(hgt, size, size, 0.6))
    save_gray("T_Manor_Carpet_R", size, size, rgh)


# ── ⑤ 금장 — 결 있는 놋/금 몰딩(빗질 자국) ─────────────────────────────
def tex_gilt(size=SIZE, seed=71):
    print("[금장]")
    gold = (196, 150, 66)
    col = [[None] * size for _ in range(size)]
    hgt = blank(size, size)
    rgh = blank(size, size)
    for y in range(size):
        vv = y / float(size)
        for x in range(size):
            uu = x / float(size)
            # 세로로 빗질된 광택 줄
            streak = math.sin((uu * 220.0) * math.pi) * 0.5 + 0.5
            streak = 0.6 + 0.4 * streak
            patina = fbm(uu, vv, 10, 3, seed)      # 얼룩덜룩한 오래된 광
            k = streak * (0.82 + patina * 0.4)
            r = gold[0] * k; g = gold[1] * k; b = gold[2] * k
            col[y][x] = (r, g, b)
            hgt[y][x] = 0.5 + (streak - 0.75) * 0.4
            rgh[y][x] = 0.28 + (1 - streak) * 0.18 + (patina - 0.5) * 0.1
    save_rgb("T_Manor_Gilt_C", size, size, col)
    save_rgb("T_Manor_Gilt_N", size, size, height_to_normal(hgt, size, size, 0.5))
    save_gray("T_Manor_Gilt_R", size, size, rgh)


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    print("=== 네르한 저택 텍스처 저작 (우리 것만) ===")
    # 밝은 대리석: 따뜻한 흰 바탕 + 회금 정맥
    tex_marble("MarbleW", (226, 220, 208), (150, 142, 128), (196, 168, 110),
               seed=13, freq=3.0)
    # 짙은 대리석: 검푸른 바탕 + 흰정맥 (체커 무늬 짝)
    tex_marble("MarbleD", (26, 28, 44), (150, 160, 190), (120, 130, 170),
               seed=29, freq=2.6)
    tex_parquet()
    tex_damask()
    tex_carpet()
    tex_gilt()
    print("=== 완료: %s ===" % OUT)
