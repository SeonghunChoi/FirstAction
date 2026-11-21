# 깃 버전 에셋: 게임 흐름 UI·연출 (9-4 이후). 에디터 안에서 실행한다:
#   python Tools/ue_exec.py Tools/Portfolio/game_ui_assets.py
# 있으면 지우지 않고 그 자리에서 갱신한다(참조가 끊기지 않게).
#   - M_OverlayFlash, GCN_Attack_Telegraph: 적 공격 예비 동작 번쩍임 (GameplayCue.Attack.Telegraph)
#   - M_GuideGlow, MI_GuideDot, MI_GuidePillar, BP_RouteGuide: 다음 구간 길 안내 (플레이어 컨트롤러 RouteGuideClass)
#   - 입력 액션 Action Description: 조작법 화면·힌트 줄의 동작 이름
#   - 적 캐릭터 데이터 예비 동작 배율
import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("에디터가 플레이 중이다. 플레이를 멈추고 다시 실행한다")

MAT_DIR = "/Game/Portfolio/Materials"
FX_DIR = "/Game/Portfolio/Effects"
CUE_DIR = "/Game/Portfolio/GameplayCues"
TELEGRAPH = (0.55, 0.8)   # 적 예비 동작 시간 배율: 첫 타, 콤보로 이어진 타
saved = []


def load_or_create(path, cls, factory):
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    folder, name = path.rsplit("/", 1)
    if not EAL.does_directory_exist(folder):
        EAL.make_directory(folder)
    return TOOLS.create_asset(name, folder, cls, factory)


def new_material(path, blend, unlit=True):
    m = load_or_create(path, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(m)
    m.set_editor_property("blend_mode", blend)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT if unlit else unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    return m


def expr(m, cls, x, y, **props):
    e = MEL.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def mul(m, a, b, x, y, const_b=None):
    e = expr(m, unreal.MaterialExpressionMultiply, x, y)
    MEL.connect_material_expressions(a, "", e, "A")
    if b is not None:
        MEL.connect_material_expressions(b, "", e, "B")
    else:
        e.set_editor_property("const_b", const_b)
    return e


# ---------------------------------------------------------------- 번쩍임 오버레이
m = new_material(MAT_DIR + "/M_OverlayFlash", unreal.BlendMode.BLEND_TRANSLUCENT)
color = expr(m, unreal.MaterialExpressionVectorParameter, -900, -100, parameter_name="FlashColor", default_value=unreal.LinearColor(1, 0.25, 0.05, 1))
inten = expr(m, unreal.MaterialExpressionScalarParameter, -900, 100, parameter_name="Intensity", default_value=0.0)
fres = expr(m, unreal.MaterialExpressionFresnel, -900, 250, exponent=2.0)
rim = expr(m, unreal.MaterialExpressionLinearInterpolate, -650, 250, const_a=0.3, const_b=1.0)
MEL.connect_material_expressions(fres, "", rim, "Alpha")
glow = mul(m, color, inten, -650, -50)
emissive = mul(m, glow, rim, -400, 0)
alpha = mul(m, inten, None, -650, 120, const_b=0.15)
alpha_rim = mul(m, alpha, rim, -400, 150)
sat = expr(m, unreal.MaterialExpressionSaturate, -200, 150)
MEL.connect_material_expressions(alpha_rim, "", sat, "")
MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.connect_material_property(sat, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(m)
saved.append(m)

# ---------------------------------------------------------------- 공격 예비 동작 Cue
factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", unreal.GCN_OverlayFlash.static_class())
cue = load_or_create(CUE_DIR + "/GCN_Attack_Telegraph", unreal.Blueprint, factory)
cdo = unreal.get_default_object(cue.generated_class())
t = unreal.GameplayTag(); t.import_text('(TagName="GameplayCue.Attack.Telegraph")')
cdo.set_editor_property("gameplay_cue_tag", t)
cdo.set_editor_property("overlay_material", m)
cdo.set_editor_property("flash_color", unreal.LinearColor(1.0, 0.22, 0.04, 1.0))
cdo.set_editor_property("duration", 0.5)
cdo.set_editor_property("intensity", 5.0)
unreal.BlueprintEditorLibrary.compile_blueprint(cue)
saved.append(cue)

# ---------------------------------------------------------------- 길 안내
g = new_material(MAT_DIR + "/M_GuideGlow", unreal.BlendMode.BLEND_ADDITIVE)
gcol = expr(g, unreal.MaterialExpressionVectorParameter, -900, -100, parameter_name="Color", default_value=unreal.LinearColor(0.35, 0.85, 1.0, 1))
gint = expr(g, unreal.MaterialExpressionScalarParameter, -900, 100, parameter_name="Intensity", default_value=4.0)
gfres = expr(g, unreal.MaterialExpressionFresnel, -900, 250, exponent=1.5)
core = expr(g, unreal.MaterialExpressionLinearInterpolate, -650, 250, const_a=1.0, const_b=0.15)   # 가장자리는 흐리게
MEL.connect_material_expressions(gfres, "", core, "Alpha")
gglow = mul(g, gcol, gint, -650, -50)
gem = mul(g, gglow, core, -400, 0)
MEL.connect_material_property(gem, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(g)
saved.append(g)


def instance(path, parent, intensity):
    mi = load_or_create(path, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", parent)
    MEL.set_material_instance_scalar_parameter_value(mi, "Intensity", intensity)
    MEL.update_material_instance(mi)
    saved.append(mi)
    return mi


dot_mi = instance(MAT_DIR + "/MI_GuideDot", g, 6.0)
pillar_mi = instance(MAT_DIR + "/MI_GuidePillar", g, 1.2)

factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", unreal.RouteGuide.static_class())
guide = load_or_create(FX_DIR + "/BP_RouteGuide", unreal.Blueprint, factory)
gcdo = unreal.get_default_object(guide.generated_class())
cyl = unreal.load_asset("/Engine/BasicShapes/Cylinder")
gcdo.set_editor_property("dot_mesh", cyl)
gcdo.set_editor_property("dot_material", dot_mi)
gcdo.set_editor_property("pillar_mesh", cyl)
gcdo.set_editor_property("pillar_material", pillar_mi)
unreal.BlueprintEditorLibrary.compile_blueprint(guide)
saved.append(guide)

pc_bp = unreal.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController")
unreal.get_default_object(pc_bp.generated_class()).set_editor_property("route_guide_class", guide.generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(pc_bp)
saved.append(pc_bp)

# ---------------------------------------------------------------- 조작법 동작 이름
DESCRIPTIONS = {
    "/Game/Input/Actions/IA_Move": "Move",
    "/Game/Input/Actions/IA_Look": "Camera",
    "/Game/Input/Actions/IA_MouseLook": "Camera",
    "/Game/Input/Actions/IA_Jump": "Jump",
    "/Game/Portfolio/Input/IA_LightAttack": "Light Attack",
    "/Game/Portfolio/Input/IA_HeavyAttack": "Heavy Attack",
    "/Game/Portfolio/Input/IA_Dodge": "Dodge",
    "/Game/Portfolio/Input/IA_Guard": "Guard",
    "/Game/Portfolio/Input/IA_LockOn": "Lock-on",
    "/Game/Portfolio/Input/IA_Pause": "Pause",
}
for path, text in DESCRIPTIONS.items():
    ia = unreal.load_asset(path)
    ia.set_editor_property("action_description", text)
    saved.append(ia)

# ---------------------------------------------------------------- 적 예비 동작 배율
for path in ("/Game/Portfolio/Data/Characters/DA_Character_Enemy", "/Game/Portfolio/Data/Characters/DA_Character_Elite"):
    d = unreal.load_asset(path)
    d.set_editor_property("telegraph_time_scale", TELEGRAPH[0])
    d.set_editor_property("telegraph_combo_time_scale", TELEGRAPH[1])
    saved.append(d)

for a in saved:
    EAL.save_loaded_asset(a, False)
print("saved", len(saved), [a.get_name() for a in saved])
