# =====================================================================
#  Tripo 고퀄 카툰 캐릭터 → 3D → (리깅) → FBX 자동 파이프라인  (단일 스크립트)
#  - 공식 tripo3d SDK 기반(업로드/폴링/다운로드/리깅/변환 전부 처리)
#  - 실행: Blender 번들 파이썬으로 (아래 RUN 주석 참고). 키만 넣고 트리거하면 끝.
#  - 작성: 우리가 분석한 "셀 룩 = UE 툰셰이더" 전략을 3단계 가이드에 반영.
# =====================================================================
import sys, os, asyncio, shutil, glob
sys.path.insert(0, r"C:\Secret_Project\_pylibs")          # SDK 설치 위치
from tripo3d import TripoClient, TaskStatus
try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass

# ===================== CONFIG (여기만 만지면 됨) =====================
KEY_FILE   = r"C:\Secret_Project\_secrets\tripo_key.txt"   # 여기에 tsk_... 키 한 줄 (또는 환경변수 TRIPO_API_KEY)

# 입력 모드: "multiview"(앞/뒤/좌/우 제공·추천) | "single"(정면 1장) | "text"(프롬프트)
INPUT_MODE = "multiview"
REF_DIR    = r"C:\Secret_Project\_blender\refs"
_MD = r"C:\Secret_Project\_tripo\maid_input"
MULTIVIEW_IMAGES = [                                       # 순서 고정: 앞(필수)/뒤/좌/우. 없으면 그 줄 비워둠("")
    os.path.join(_MD, "front.png"),
    os.path.join(_MD, "back.png"),
    os.path.join(_MD, "left.png"),
    os.path.join(_MD, "right.png"),
]
SINGLE_IMAGE = os.path.join(REF_DIR, "portrait_front.png")  # 얼굴-우세 입력(얼굴 벽 테스트)
AUTO_MULTIVIEW_FROM_SINGLE = False                          # 직접 image_to_model(포트레이트→3D)
TEXT_PROMPT  = "masterpiece, anime cel shaded character, full body, A-pose, clean white background, turnaround reference sheet"

# 품질(애니 셀 지향 — 내 판단 기본값)
MODEL_VERSION    = "P1-20260311"      # 최고 퀄. 크레딧 더 듦. 절약하려면 "v3.1-20260211"
TEXTURE_QUALITY  = "detailed"
GEOMETRY_QUALITY = "detailed"
PBR              = False               # 셀=플랫 알베도. 실사 PBR 반사 끔
QUAD             = True                # 쿼드 토폴로지(애니/리깅 유리)
FACE_LIMIT       = None                # None=자동. 정하려면 정수(예 60000)

# 리깅/출력
DO_RIG        = False                  # 1차 테스트=메시 퀄만 평가(크레딧 절약). 좋으면 True로 리깅.
RIG_OUT_FORMAT= "fbx"                  # 'fbx' | 'glb'
ALSO_CONVERT_FBX = False               # 1차는 GLB 그대로(UE도 GLB 임포트 됨). 나중에 True로 FBX 변환.
OUTPUT_DIR    = r"C:\UE_Import_Zone"
OUTPUT_NAME   = "maid_tripo"           # 고퀄 일러 4면 → 멀티뷰 메시
# ====================================================================

RUN_LOG = r"C:\Secret_Project\기획/99_보관/구로그/tripo_run_log.txt"
LAST_TASK = r"C:\Secret_Project\_blender\last_task.txt"
_logbuf = []
def log(s):
    line = "PIPE> %s" % s
    _logbuf.append(line)
    try: print(line, flush=True)
    except Exception: pass
    try:
        with open(RUN_LOG, "w", encoding="utf-8") as f: f.write("\n".join(_logbuf))
    except Exception: pass
def save_task(tid):
    try: open(LAST_TASK, "w", encoding="utf-8").write(str(tid))
    except Exception: pass

def get_key():
    k = os.environ.get("TRIPO_API_KEY")
    if k and k.strip(): return k.strip()
    if os.path.exists(KEY_FILE):
        v = open(KEY_FILE, encoding="utf-8").read().strip()
        if v: return v
    return None

UE_GUIDE = """
============================================================
💡 [언리얼 임포트 후 셀 퀄리티 가이드 — 우리가 분석한 결론]
  ※ 제미나이가 말한 'Unlit + Emissive에 텍스처 연결'은 *플랫 스티커*가 됨
    (셀 음영도 외곽선도 없음). 우리가 배운 진짜 답은:
  1) 캐릭터 머티리얼 = '툰/셀 셰이더'(VRM4U MToon 또는 CiciToon 계열).
     - Tripo 텍스처(알베도)는 BaseColor/ShadeTexture에 연결.
     - ShadeToony↑(하드 2톤) + ShadeColor(그림자색) + 외곽선(Outline).
  2) 라이팅/노출 제어가 셀의 8할:
     - 디렉셔널 라이트는 부드럽게 + 스카이라이트로 필.
     - 캐릭터 노출 고정(PostProcess Manual exposure 또는 캐릭터 노출 PP).
       → 안 그러면 언라이트/툰 캐릭이 검게 크러시됨(우리가 겪은 그 문제).
  3) Tripo 메시는 토폴로지/노멀이 거칠 수 있음 → 얼굴 노멀 구면화(우리 Blender 스크립트)
     로 한 번 정리하면 셀 얼굴이 깨끗해짐.
  4) 임포트 시 'Import Normals'(노멀 보존)로 들여올 것. Compute로 덮으면 셀 깨짐.
============================================================
"""

async def download_and_finalize(client, task, tag):
    files = await client.download_task_models(task, OUTPUT_DIR)
    log("%s 다운로드: %s" % (tag, files))
    # 받은 것 중 fbx 우선, 없으면 glb
    picked = None
    for k, p in (files or {}).items():
        if p and p.lower().endswith(".fbx"): picked = p; break
    if not picked:
        for k, p in (files or {}).items():
            if p and p.lower().endswith(".glb"): picked = p; break
    if not picked:
        # 폴더에서 직접 탐색
        cands = glob.glob(os.path.join(OUTPUT_DIR, "*.fbx")) + glob.glob(os.path.join(OUTPUT_DIR, "*.glb"))
        picked = max(cands, key=os.path.getmtime) if cands else None
    if picked:
        ext = os.path.splitext(picked)[1]
        dst = os.path.join(OUTPUT_DIR, OUTPUT_NAME + ext)
        if os.path.abspath(picked) != os.path.abspath(dst):
            shutil.copyfile(picked, dst)
        log("✅ 최종 파일: %s" % dst)
    else:
        log("!! 다운로드 파일을 못 찾음")

async def run():
    key = get_key()
    if not key:
        log("API 키 없음 → 환경변수 TRIPO_API_KEY 설정하거나 %s 에 tsk_... 한 줄 넣어줘." % KEY_FILE); return
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    client = TripoClient(api_key=key)
    try:
        try:
            bal = await client.get_balance()
            log("Tripo 잔액(credits): %s" % getattr(bal, "balance", bal))
        except Exception as e:
            log("잔액 조회 실패(무시): %s" % e)

        if "P1" in MODEL_VERSION:
            # P1은 프리미엄이라 품질/quad/geometry 파라미터 미지원 — 핵심만
            common = dict(model_version=MODEL_VERSION, texture=True, pbr=PBR)
        else:
            common = dict(model_version=MODEL_VERSION, texture=True, pbr=PBR,
                          texture_quality=TEXTURE_QUALITY, geometry_quality=GEOMETRY_QUALITY,
                          auto_size=True, orientation="align_image")
            if QUAD: common["quad"] = True
        if FACE_LIMIT: common["face_limit"] = FACE_LIMIT

        model_task_id = None

        if INPUT_MODE == "multiview":
            imgs = [p for p in MULTIVIEW_IMAGES if p and os.path.exists(p)]
            if not imgs:
                log("!! 멀티뷰 이미지가 없음. %s 에 front.png 등 넣어줘." % REF_DIR); return
            log("multiview_to_model: %d장 → %s" % (len(imgs), [os.path.basename(x) for x in imgs]))
            model_task_id = await client.multiview_to_model(images=imgs, **common)

        elif INPUT_MODE == "single":
            if not os.path.exists(SINGLE_IMAGE):
                log("!! 정면 이미지 없음: %s" % SINGLE_IMAGE); return
            if AUTO_MULTIVIEW_FROM_SINGLE:
                log("generate_multiview_image (정면→4면도 자동)...")
                mv_task_id = await client.generate_multiview_image(image=SINGLE_IMAGE)
                mv = await client.wait_for_task(mv_task_id, polling_interval=5, verbose=True)
                if mv.status != TaskStatus.SUCCESS:
                    log("멀티뷰 이미지 생성 실패(%s) → 단일 image_to_model로 진행" % mv.status)
                    model_task_id = await client.image_to_model(image=SINGLE_IMAGE, **common)
                else:
                    gen = await client.download_task_models(mv, REF_DIR)  # 생성된 뷰 받기
                    log("생성 멀티뷰: %s" % gen)
                    views = [p for p in (gen or {}).values() if p and p.lower().endswith((".png", ".jpg", ".jpeg", ".webp"))]
                    if views:
                        model_task_id = await client.multiview_to_model(images=views, **common)
                    else:
                        model_task_id = await client.image_to_model(image=SINGLE_IMAGE, **common)
            else:
                log("image_to_model (단일)")
                model_task_id = await client.image_to_model(image=SINGLE_IMAGE, **common)

        elif INPUT_MODE == "text":
            log("text_to_image: %s" % TEXT_PROMPT[:70])
            it = await client.text_to_image(prompt=TEXT_PROMPT)
            itk = await client.wait_for_task(it, polling_interval=5, verbose=True)
            imgs = [p for p in (await client.download_task_models(itk, REF_DIR)).values()
                    if p and p.lower().endswith((".png", ".jpg", ".jpeg", ".webp"))]
            if not imgs:
                log("!! 텍스트→이미지 결과 없음"); return
            model_task_id = await client.image_to_model(image=imgs[0], **common)
        else:
            log("!! INPUT_MODE 잘못됨"); return

        save_task(model_task_id)
        log("3D 생성 태스크: %s - 대기(폴링)..." % model_task_id)
        task = await client.wait_for_task(model_task_id, polling_interval=15, verbose=True)
        if task.status != TaskStatus.SUCCESS:
            log("!! 3D 생성 실패: status=%s" % task.status); return
        log("✅ 3D 생성 성공")

        final_task = task
        if DO_RIG:
            log("rig_model (자동 리깅, out=%s)..." % RIG_OUT_FORMAT)
            try:
                rig_id = await client.rig_model(original_model_task_id=model_task_id, out_format=RIG_OUT_FORMAT)
                rt = await client.wait_for_task(rig_id, polling_interval=15, verbose=True)
                if rt.status == TaskStatus.SUCCESS:
                    final_task = rt; log("✅ 리깅 성공")
                else:
                    log("리깅 실패(%s) → 리깅 없는 모델로 진행" % rt.status)
            except Exception as e:
                log("리깅 예외(%s) → 모델만 진행" % e)
        elif ALSO_CONVERT_FBX:
            log("convert_model → FBX...")
            try:
                cv = await client.convert_model(original_model_task_id=model_task_id, format="FBX", fbx_preset="blender")
                ct = await client.wait_for_task(cv, polling_interval=15, verbose=True)
                if ct.status == TaskStatus.SUCCESS: final_task = ct; log("✅ FBX 변환 성공")
            except Exception as e:
                log("convert 예외(무시): %s" % e)

        await download_and_finalize(client, final_task, "최종")
        for _ln in UE_GUIDE.splitlines(): log(_ln)
    finally:
        try: await client.close()
        except Exception: pass

if __name__ == "__main__":
    asyncio.run(run())

# ---- RUN (Blender 번들 파이썬으로) ----
# & "C:\Program Files\Blender Foundation\Blender 5.1\5.1\python\bin\python.exe" "C:\Secret_Project\tripo_pipeline.py"
