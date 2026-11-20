# 배포판 세 캐릭터의 래그돌 물리 에셋 (9-3). Paragon 물리 에셋은 골반·가슴·머리·손·발 몇 덩어리뿐인 판정용이라
# 쓰러지면 다리가 막대처럼 뻣뻣했고, Kwang은 물리 에셋이 아예 없어 칼 판정(메시 바디)에 맞지 않았다.
# 마네킹과 같은 뼈 20개에만 캡슐 바디를 만들고(FirstActionEditorLibrary.create_ragdoll_physics_asset), 원본 메시는 두고
# 캐릭터 BP 메시 컴포넌트의 Physics Asset Override로 지정한다. 칼 판정, 부위 반응, 래그돌이 모두 이 바디를 쓴다
# aurora_body.py, enemies.py 다음에 실행한다

BONES = ["pelvis", "spine_01", "spine_02", "spine_03", "neck_01", "head",
         "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l", "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r",
         "thigh_l", "calf_l", "foot_l", "thigh_r", "calf_r", "foot_r"]
CHARACTERS = [
    ("/Game/ThirdParty/ParagonAurora/Characters/Heroes/Aurora/Meshes/Aurora", "/Game/ThirdParty/Release/Aurora/PA_Aurora_Ragdoll", "/Game/ThirdParty/Release/Aurora/BP_Player_Aurora"),
    ("/Game/ThirdParty/ParagonKwang/Characters/Heroes/Kwang/Meshes/Kwang_GDC", "/Game/ThirdParty/Release/Kwang/PA_Kwang_Ragdoll", "/Game/ThirdParty/Release/Kwang/BP_Enemy_Kwang"),
    ("/Game/ThirdParty/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone", "/Game/ThirdParty/Release/Greystone/PA_Greystone_Ragdoll", "/Game/ThirdParty/Release/Greystone/BP_Enemy_Greystone"),
]
CIL = unreal.ConstraintInstanceBlueprintLibrary
saved = []
for mesh_path, pa_path, bp_path in CHARACTERS:
    pa = LIB.create_ragdoll_physics_asset(unreal.load_asset(mesh_path), BONES, pa_path)
    pairs = []
    for c in pa.get_constraints(False):
        names = [str(x) for x in CIL.get_attached_body_names(c) if isinstance(x, (str, unreal.Name))]
        pairs.append("-".join(names))
    log("ragdoll", pa_path.rsplit("/", 1)[1], len(pa.get_constraints(False)), pairs)
    bp = unreal.load_asset(bp_path)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.get_editor_property("mesh").set_editor_property("physics_asset_override", pa)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    saved += [pa_path, bp_path]
save_all(saved)
for _, _, bp_path in CHARACTERS:
    cdo = unreal.get_default_object(unreal.load_asset(bp_path).generated_class())
    log("override", bp_path.rsplit("/", 1)[1], cdo.get_editor_property("mesh").get_editor_property("physics_asset_override").get_name())
