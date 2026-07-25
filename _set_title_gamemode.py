# -*- coding: utf-8 -*-
"""새 시작 화면 맵의 게임모드를 TitleGameMode로 박는다.

전역 기본 게임모드는 본편(진행)용이라, 시작 화면 맵이 그걸 물면 진행 로직이 헛돈다.
World Settings의 GameMode Override만 이 맵에 지정한다.
실행: UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="C:/Secret_Project/_set_title_gamemode.py"
"""
import unreal

MAP = "/Game/Maps/Title_Rasel"
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
LES.load_level(MAP)

ws = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_world_settings()
cls = unreal.load_class(None, "/Script/Secret_Project.TitleGameMode")
if cls is None:
    raise RuntimeError("TitleGameMode 클래스를 못 찾았다 — 모듈이 빌드돼 있어야 한다.")
ws.set_editor_property("default_game_mode", cls)
unreal.log("[title] 게임모드 박음: %s" % cls.get_name())

ok = LES.save_current_level()
unreal.log("[title] 저장 -> %s" % ok)
if not ok:
    raise RuntimeError("맵 저장 실패")
