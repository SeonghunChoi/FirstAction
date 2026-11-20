# GASP 회피 옆걸음(마네킹) → Aurora 리타깃 (9-3, 에셋 규칙 3: 다른 출처 동작은 캐릭터 스켈레톤으로 리타깃)
# IK Rig는 엔진 자동 생성(리타깃 체인 + 전신 IK), 리타기터는 기본 오퍼레이션 + 체인 자동 연결 + 대상 자세 자동 정렬

RIGS = "/Game/ThirdParty/Release/Retarget/"
MANNY_MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
AURORA_MESH = "/Game/ThirdParty/ParagonAurora/Characters/Heroes/Aurora/Meshes/Aurora"
SRC_ANIMS = ["/Game/ThirdParty/Release/GASP/Avoid", "/Game/ThirdParty/Release/GASP/Slide"]
OUT_ANIMS = "/Game/ThirdParty/Release/Aurora/Animations"
SIDE = unreal.RetargetSourceOrTarget


def make_rig(path, mesh_path):
    rig = load_or_create(path, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    c.set_skeletal_mesh(unreal.load_asset(mesh_path))
    c.apply_auto_generated_retarget_definition()
    c.apply_auto_fbik()
    log("rig", path.rsplit("/", 1)[1], "chains", [str(x.get_editor_property("chain_name")) for x in c.get_retarget_chains()])
    return rig


manny_rig = make_rig(RIGS + "IK_Manny_Release", MANNY_MESH)
aurora_rig = make_rig(RIGS + "IK_Aurora", AURORA_MESH)

rtg = load_or_create(RIGS + "RTG_Manny_Aurora", unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
rc.set_ik_rig(SIDE.SOURCE, manny_rig)
rc.set_ik_rig(SIDE.TARGET, aurora_rig)
rc.remove_all_ops()  # 다시 실행해도 오퍼레이션이 쌓이지 않게 비우고 새로 만든다
rc.add_default_ops()
rc.assign_ik_rig_to_all_ops(SIDE.SOURCE, manny_rig)
rc.assign_ik_rig_to_all_ops(SIDE.TARGET, aurora_rig)
rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)

# IK 뼈(ik_foot_l/r, ik_hand_l/r)는 체인에 없어 리타깃되지 않고 기준 자세에 남는다.
# Paragon 애님 BP의 다리 IK가 발을 이 뼈로 끌어당기므로, 남겨 두면 두 발이 한자리에 붙은 채 끌려간다(9-3 시험에서 발견).
# Pin Bone 오퍼레이션으로 대상 스켈레톤의 실제 발·손 위치를 IK 뼈에 복사한다
PIN_PAIRS = [("foot_l", "ik_foot_l"), ("foot_r", "ik_foot_r"), ("hand_l", "ik_hand_l"), ("hand_r", "ik_hand_r")]
pin = rc.get_op_controller(rc.add_retarget_op("/Script/IKRig.IKRetargetPinBoneOp"))
for src, dst in PIN_PAIRS:
    pin.set_bone_pair(src, dst)
log("ops", [rc.get_op_controller(i).get_class().get_name() for i in range(rc.get_num_retarget_ops())])
log("pin bones (target: source)", pin.get_all_bone_pairs())
rc.auto_align_all_bones(SIDE.TARGET)
save_all([RIGS + "IK_Manny_Release", RIGS + "IK_Aurora", RIGS + "RTG_Manny_Aurora"])

reg = unreal.AssetRegistryHelpers.get_asset_registry()
# 방금 임포트한 폴더를 레지스트리가 모르면 결과가 /Game 바로 아래로 떨어진다(9-3에서 겪음). 먼저 스캔한다
reg.scan_paths_synchronous(SRC_ANIMS + [OUT_ANIMS], True, False)
# RETARGET_ALL: 리타기터를 고친 뒤 기존 결과를 지우고 모두 다시 만든다 (몽타주는 aurora_body.py를 다시 돌려 새 시퀀스에 연결한다)
RETARGET_ALL = True
if RETARGET_ALL:
    for n in EAL.list_assets(OUT_ANIMS, recursive=False):
        if "/A_GASP_Aurora_" in n:
            EAL.delete_asset(n.split(".")[0])
done = set(n.rsplit("/", 1)[1] for n in EAL.list_assets(OUT_ANIMS, recursive=False))
sources = [a for folder in SRC_ANIMS for a in reg.get_assets_by_path(folder, recursive=False)
           if str(a.asset_class_path.asset_name) == "AnimSequence"
           and str(a.asset_name).replace("A_GASP_", "A_GASP_Aurora_") + "." + str(a.asset_name).replace("A_GASP_", "A_GASP_Aurora_") not in done]
ensure_dir(OUT_ANIMS)
results = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
    sources, unreal.load_asset(MANNY_MESH), unreal.load_asset(AURORA_MESH), rtg,
    search="A_GASP_", replace="A_GASP_Aurora_", include_referenced_assets=False, overwrite_existing_files=True)

# 결과를 Aurora 폴더로 옮긴다. 레지스트리를 거치지 않고 메모리의 에셋을 바로 옮긴다(rename_loaded_asset)
moved = []
for r in results:
    obj = r.get_asset()
    dst = OUT_ANIMS + "/" + obj.get_name()
    if obj.get_path_name().split(".")[0] != dst:
        EAL.rename_loaded_asset(obj, dst)
    EAL.save_loaded_asset(obj, False)
    moved.append(obj.get_path_name())
log("retargeted", len(moved), moved)
