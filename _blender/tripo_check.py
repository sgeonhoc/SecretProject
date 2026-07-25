import sys, asyncio
sys.path.insert(0, r"C:\Secret_Project\_pylibs")
from tripo3d import TripoClient

async def main():
    key = open(r"C:\Secret_Project\_secrets\tripo_key.txt", encoding="utf-8").read().strip()
    print("key prefix:", key[:8], "len:", len(key))
    c = TripoClient(api_key=key)
    try:
        b = await c.get_balance()
        print("BALANCE OBJ:", b)
        for attr in ("balance", "frozen", "credits"):
            if hasattr(b, attr):
                print("  ", attr, "=", getattr(b, attr))
    except Exception as e:
        print("ERROR:", type(e).__name__, e)
    finally:
        try: await c.close()
        except Exception: pass

asyncio.run(main())
