# 배포 맵에서 플래너 리플렉션만 지운다 (9-4 이후, release_map.py와 같은 처리를 맵 전체 재생성 없이). 에디터 안에서:
#   python Tools/ue_exec.py Tools/Release/remove_planar.py
import unreal

MAP = "/Game/ThirdParty/Release/Maps/Lvl_Release_Agora"
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
if ues.get_game_world():
    raise RuntimeError("에디터가 플레이 중이다. 플레이를 멈추고 다시 실행한다")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not ues.get_editor_world().get_path_name().startswith(MAP):
    les.load_level(MAP)
removed = [a.get_actor_label() for a in eas.get_all_level_actors() if isinstance(a, unreal.PlanarReflection)]
for a in [a for a in eas.get_all_level_actors() if isinstance(a, unreal.PlanarReflection)]:
    eas.destroy_actor(a)
les.save_current_level()
print("removed", removed)
