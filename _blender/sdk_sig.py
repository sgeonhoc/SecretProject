import sys; sys.path.insert(0, r"C:\Secret_Project\_pylibs")
import inspect, tripo3d
from tripo3d import TripoClient as C
print("tripo3d", getattr(tripo3d, "__version__", "?"))
for m in ['image_to_model','multiview_to_model','generate_image','generate_multiview_image',
          'text_to_image','rig_model','convert_model','wait_for_task','download_task_models',
          'get_balance','retarget_animation','stylize_model','refine_model','texture_model',
          'create_task','upload_file','import_model']:
    try: print(m, str(inspect.signature(getattr(C, m))))
    except Exception as e: print(m, "ERR", e)
try:
    from tripo3d import TaskStatus
    print("TaskStatus:", [x for x in dir(TaskStatus) if not x.startswith('_')])
except Exception as e: print("TaskStatus err", e)
# Task 객체 결과 접근 필드
try:
    import tripo3d as t
    print("module attrs:", [x for x in dir(t) if not x.startswith('_')])
except Exception as e: print("mod err", e)
