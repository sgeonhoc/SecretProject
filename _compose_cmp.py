import sys; sys.path.insert(0, r"C:\Secret_Project\_pylibs")
from PIL import Image, ImageDraw
base = r"C:/Secret_Project/_tripo"
cols = [("ORIGINAL (VRoid)", base + "/our_chars/test7_orig_render"),
        ("SWAPPED (v3 outline)", base + "/our_chars/test7")]
rows = ["face_front.png", "face_34L.png", "eyes.png", "body_front.png"]
CW, CH = 600, 360
PADX, PADY, TOP = 14, 30, 38
def load_fit(p):
    try: im = Image.open(p).convert("RGB")
    except Exception: return Image.new("RGB", (CW, CH), (60,60,60))
    im.thumbnail((CW, CH), Image.LANCZOS)
    cell = Image.new("RGB", (CW, CH), (232,232,232))
    cell.paste(im, ((CW-im.width)//2, (CH-im.height)//2))
    return cell
W = PADX + len(cols)*(CW+PADX)
H = TOP + len(rows)*(CH+PADY)
canvas = Image.new("RGB", (W,H), (246,246,246))
d = ImageDraw.Draw(canvas)
for ci,(label,folder) in enumerate(cols):
    x = PADX + ci*(CW+PADX)
    d.text((x+8,12), label, fill=(15,15,15))
    for ri,fn in enumerate(rows):
        y = TOP + ri*(CH+PADY)
        canvas.paste(load_fit(folder+"/"+fn), (x,y))
        d.text((x+8, y+CH+7), fn.replace(".png",""), fill=(90,90,90))
out = base + "/grids/_compare_v3.png"
canvas.save(out); print("saved", out, canvas.size)
