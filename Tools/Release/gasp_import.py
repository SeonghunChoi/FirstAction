# GASP 회피 옆걸음 FBX를 마네킹 스켈레톤으로 임포트 (9-3)
# 원본: D:/Projects/Unreal/Temp/Animations/Avoidance (Fab "Game Animation Sample Animations Retargeted to ue5 mannequin", 애니메이션만 든 FBX)
# 애니메이션만 든 FBX는 스켈레톤을 지정해야 임포트된다. Fab "프로젝트에 추가"는 이걸 하지 않아 전부 실패했다

import os

TEMP = r"D:/Projects/Unreal/Temp/Animations/"
SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
# (원본 폴더, 대상 폴더, 파일 이름에 들어 있어야 하는 문자열들)
SOURCES = [
    ("Avoidance", "/Game/ThirdParty/Release/GASP/Avoid", [""]),
    # 슬라이드: 정지·제자리에서 쓸 수 있는 진입·유지·일어서기만 (9-3 회피 후보 비교)
    ("Slide", "/Game/ThirdParty/Release/GASP/Slide",
     ["Neutral_Slide_FootOut_Into_Rfoot", "Neutral_Slide_FootOut_Loop", "Neutral_Slide_FootOut_Out_Idle_Stand",
      "Neutral_Slide_KneesOut_Into_Rfoot", "Neutral_Slide_KneesOut_Loop", "Neutral_Slide_KneesOut_Out_Idle_Stand"]),
]

skeleton = unreal.load_asset(SKELETON)
tasks = []
for folder, DEST, filters in SOURCES:
  ensure_dir(DEST)
  SRC_DIR = TEMP + folder
  for name in sorted(os.listdir(SRC_DIR)):
    if not name.lower().endswith(".fbx") or not any(f in name for f in filters):
        continue
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", False)
    ui.set_editor_property("import_animations", True)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("skeleton", skeleton)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    ui.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SRC_DIR + "/" + name)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", "A_GASP_" + os.path.splitext(name)[0].replace("M_Neutral_", ""))
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    tasks.append(task)

TOOLS.import_asset_tasks(tasks)
for t in tasks:
    log("imported", [str(p) for p in t.get_editor_property("imported_object_paths")])
