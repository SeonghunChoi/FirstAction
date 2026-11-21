# 배포 맵 얕은 물 연출 (9-3): 반투명 물 표면, 물 바닥 물리 재질, 발 디딤 물결·물방울(UFootstepEffectComponent) 설정.
# 샘플의 "물"은 물 바닥 판(SM_Plane_5x5m, -71cm)에 물처럼 보이는 불투명 재질을 입힌 것이라 표면이 따로 없다.
# 바닥 위 WATER_DEPTH에 반투명 표면을 깔아 발이 잠겨 보이게 하고, 바닥에 Paragon PM_ShallowWater를 지정해 물을 판정한다.
# release_map.py 다음에 실행한다 (common.py + 이 파일)

MAP = "/Game/ThirdParty/Release/Maps/Lvl_Release_Agora"
OUT = "/Game/ThirdParty/Release/Environment/Water/"
WATER_NORMAL = "/Game/ThirdParty/ParagonAgora/KiteDemo/Environments/Textures/T_Water_N"
WATER_PHYSMAT = "/Game/ThirdParty/ParagonProps/Agora/Props/MaterialFunctions/PM_ShallowWater"
WATER_DEPTH = 25.0
CHARACTERS = {   # BP, Idle 동작 (서 있을 때 발 뼈 높이를 잰다)
    "/Game/ThirdParty/Release/Aurora/BP_Player_Aurora": "/Game/ThirdParty/ParagonAurora/Characters/Heroes/Aurora/Animations/Idle",
    "/Game/ThirdParty/Release/Kwang/BP_Enemy_Kwang": "/Game/ThirdParty/ParagonKwang/Characters/Heroes/Kwang/Animations/Idle",
    "/Game/ThirdParty/Release/Greystone/BP_Enemy_Greystone": "/Game/ThirdParty/ParagonGreystone/Characters/Heroes/Greystone/Animations/Idle",
}
MEL = unreal.MaterialEditingLibrary
saved = []


def node(mat, cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def new_material(name):
    ensure_dir(OUT[:-1])
    mat = load_or_create(OUT + name, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(mat)
    return mat


# 1. 물 표면: 반투명(탁하게), 표면 픽셀 조명(반사 하이라이트), 두 방향으로 흐르는 노멀, 가장자리 프레넬, 물체와 닿는 곳만 살짝 부드럽게(DepthFade 3cm)
water = new_material("M_Release_ShallowWater")
water.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
water.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
wp = node(water, unreal.MaterialExpressionWorldPosition, -1400, 0)
mask = node(water, unreal.MaterialExpressionComponentMask, -1250, 0)
mask.set_editor_property("r", True)
mask.set_editor_property("g", True)
MEL.connect_material_expressions(wp, "", mask, "")
div = node(water, unreal.MaterialExpressionDivide, -1100, 0)
div.set_editor_property("const_b", 400.0)
MEL.connect_material_expressions(mask, "", div, "")
normals = []
for i, (sx, sy, sc) in enumerate([(0.02, 0.012, 1.0), (-0.016, 0.022, 0.63)]):
    uv = node(water, unreal.MaterialExpressionMultiply, -950, i * 250)
    uv.set_editor_property("const_b", sc)
    MEL.connect_material_expressions(div, "", uv, "")
    pan = node(water, unreal.MaterialExpressionPanner, -800, i * 250)
    pan.set_editor_property("speed_x", sx)
    pan.set_editor_property("speed_y", sy)
    MEL.connect_material_expressions(uv, "", pan, "Coordinate")
    tex = node(water, unreal.MaterialExpressionTextureSample, -600, i * 250)
    tex.set_editor_property("texture", unreal.load_asset(WATER_NORMAL))
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    MEL.connect_material_expressions(pan, "", tex, "UVs")
    normals.append(tex)
nadd = node(water, unreal.MaterialExpressionAdd, -350, 100)
MEL.connect_material_expressions(normals[0], "RGB", nadd, "A")
MEL.connect_material_expressions(normals[1], "RGB", nadd, "B")
flat = node(water, unreal.MaterialExpressionConstant3Vector, -350, 250)
flat.set_editor_property("constant", unreal.LinearColor(0, 0, 1, 1))
nlerp = node(water, unreal.MaterialExpressionLinearInterpolate, -200, 150)
nlerp.set_editor_property("const_alpha", 0.55)
MEL.connect_material_expressions(nadd, "", nlerp, "A")
MEL.connect_material_expressions(flat, "", nlerp, "B")
MEL.connect_material_property(nlerp, "", unreal.MaterialProperty.MP_NORMAL)
col = node(water, unreal.MaterialExpressionConstant3Vector, -200, -300)
col.set_editor_property("constant", unreal.LinearColor(0.012, 0.045, 0.055, 1))
MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = node(water, unreal.MaterialExpressionConstant, -200, -200)
rough.set_editor_property("r", 0.04)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
spec = node(water, unreal.MaterialExpressionConstant, -200, -150)
spec.set_editor_property("r", 1.0)
MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
fres = node(water, unreal.MaterialExpressionFresnel, -500, 500)
fres.set_editor_property("exponent", 4.0)
fmul = node(water, unreal.MaterialExpressionMultiply, -350, 500)
fmul.set_editor_property("const_b", 0.35)
MEL.connect_material_expressions(fres, "", fmul, "")
fadd = node(water, unreal.MaterialExpressionAdd, -250, 500)
fadd.set_editor_property("const_b", 0.55)   # 탁한 물: 잠긴 발이 흐려 보이게
MEL.connect_material_expressions(fmul, "", fadd, "")
fade = node(water, unreal.MaterialExpressionDepthFade, -100, 500)
fade.set_editor_property("fade_distance_default", 3.0)   # 길면 물속 다리 앞이 투명해져 잠긴 것처럼 안 보인다
MEL.connect_material_expressions(fadd, "", fade, "Opacity")
MEL.connect_material_property(fade, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(water)
saved.append(OUT + "M_Release_ShallowWater")

# 2. 물결 고리: 흰색을 칠하면 페인트처럼 보여(9-3 시험) 물 표면과 같은 반사 재질로 하고, 고리는 노멀(표면 기울기)로 넣는다.
# 반사 하이라이트가 고리를 따라 휘어 물결로 보인다. 흰 기운은 아주 옅게만. Age(0~1)에 따라 고리가 퍼지며 사라진다
ripple = new_material("M_Release_WaterRipple")
ripple.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
ripple.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
ripple.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
NL = chr(10)
RING = NL.join([
    "float2 d = UV - 0.5;",
    "float r = length(d) * 2;",
    "float2 dir = d / max(length(d), 1e-4);",
    "float R = lerp(0.12, 0.95, Age);",
    "float w = lerp(0.09, 0.05, Age);",
    "float x1 = (r - R) / w;",
    "float x2 = (r - R * 0.62) / (w * 0.8);",
    "float fade = (1 - Age) * (1 - Age) * saturate((1 - r) * 4);",
])


def custom(name, out_type, body, x, y):
    c = node(ripple, unreal.MaterialExpressionCustom, x, y)
    c.set_editor_property("description", name)
    c.set_editor_property("output_type", out_type)
    c.set_editor_property("code", RING + NL + body)
    ins = []
    for n in ("UV", "Age"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        ins.append(ci)
    c.set_editor_property("inputs", ins)
    # 입력 노드를 Custom 노드마다 따로 둔다 (한 파라미터 노드를 두 Custom에 이으면 한쪽 입력이 빠져 컴파일이 실패했다)
    t = node(ripple, unreal.MaterialExpressionTextureCoordinate, x - 300, y)
    a = node(ripple, unreal.MaterialExpressionScalarParameter, x - 300, y + 100)
    a.set_editor_property("parameter_name", "Age")
    a.set_editor_property("default_value", 0.3)
    MEL.connect_material_expressions(t, "", c, "UV")
    MEL.connect_material_expressions(a, "", c, "Age")
    return c


# 고리 단면은 가우스, 기울기는 그 도함수. Custom 노드 하나가 노멀(xyz)과 불투명도(w)를 함께 낸다
# (Custom 노드를 둘 두면 두 번째 노드의 입력 연결이 빠져 컴파일이 실패했다)
ring = custom("Ripple", unreal.CustomMaterialOutputType.CMOT_FLOAT4,
              NL.join(["float s = -2 * x1 * exp(-x1 * x1) + (-2 * x2 * exp(-x2 * x2)) * 0.5;",
                       "float2 n = dir * s * 0.9 * fade;",
                       "float a = (exp(-x1 * x1) + exp(-x2 * x2) * 0.5) * fade * 0.14;",
                       "return float4(normalize(float3(n, 1)), a);"]), -400, 0)
nmask = node(ripple, unreal.MaterialExpressionComponentMask, -200, 0)
for ch in ("r", "g", "b"):
    nmask.set_editor_property(ch, True)
# Custom 노드에서 나가는 연결은 하나만 잡혀(두 번째가 빠져 컴파일 실패) 곱하기(x1) 노드를 거쳐 나눈다
relay = node(ripple, unreal.MaterialExpressionMultiply, -300, 75)
relay.set_editor_property("const_b", 1.0)
MEL.connect_material_expressions(ring, "", relay, "A")
MEL.connect_material_expressions(relay, "", nmask, "")
MEL.connect_material_property(nmask, "", unreal.MaterialProperty.MP_NORMAL)
amask = node(ripple, unreal.MaterialExpressionComponentMask, -200, 150)
amask.set_editor_property("a", True)
MEL.connect_material_expressions(relay, "", amask, "")
MEL.connect_material_property(amask, "", unreal.MaterialProperty.MP_OPACITY)
rcol = node(ripple, unreal.MaterialExpressionConstant3Vector, -400, -200)
rcol.set_editor_property("constant", unreal.LinearColor(0.05, 0.09, 0.1, 1))
MEL.connect_material_property(rcol, "", unreal.MaterialProperty.MP_BASE_COLOR)
rr = node(ripple, unreal.MaterialExpressionConstant, -400, -100)
rr.set_editor_property("r", 0.03)
MEL.connect_material_property(rr, "", unreal.MaterialProperty.MP_ROUGHNESS)
rs = node(ripple, unreal.MaterialExpressionConstant, -400, -50)
rs.set_editor_property("r", 1.0)
MEL.connect_material_property(rs, "", unreal.MaterialProperty.MP_SPECULAR)
MEL.recompile_material(ripple)
saved.append(OUT + "M_Release_WaterRipple")

# 3. 물방울: 반투명 반사 재질. FootstepEffectComponent가 작은 구 메시 인스턴스로 그리고 포물선으로 움직인다
# Niagara 물보라(NS_Release_WaterSplash)는 사용자가 Niagara 편집기에서 만든다(Python으로 모듈 값을 고칠 수 없다).
# 있으면 아래 5에서 캐릭터에 끼운다. 이 스크립트는 그 에셋을 만들거나 지우지 않는다
SPLASH = OUT + "NS_Release_WaterSplash"
drop = new_material("M_Release_WaterDroplet")
drop.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
drop.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
for prop, value, y in [(unreal.MaterialProperty.MP_OPACITY, 0.55, 0), (unreal.MaterialProperty.MP_ROUGHNESS, 0.05, 100), (unreal.MaterialProperty.MP_SPECULAR, 1.0, 200)]:
    k = node(drop, unreal.MaterialExpressionConstant, -300, y)
    k.set_editor_property("r", value)
    MEL.connect_material_property(k, "", prop)
dcol = node(drop, unreal.MaterialExpressionConstant3Vector, -300, -150)
dcol.set_editor_property("constant", unreal.LinearColor(0.5, 0.65, 0.7, 1))
MEL.connect_material_property(dcol, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.recompile_material(drop)
saved.append(OUT + "M_Release_WaterDroplet")
save_all(saved)

# 4. 맵: 물 바닥 판에 얕은 물 물리 재질, 그 위에 반투명 물 표면
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(MAP)
physmat = unreal.load_asset(WATER_PHYSMAT)
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "WaterSurface":
        eas.destroy_actor(a)
floors = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh
          and a.static_mesh_component.static_mesh.get_name() == "SM_Plane_5x5m"]
for f in floors:
    c = f.static_mesh_component
    c.modify()
    c.set_phys_material_override(physmat)
    surf = eas.spawn_actor_from_class(unreal.StaticMeshActor, f.get_actor_location() + unreal.Vector(0, 0, WATER_DEPTH), f.get_actor_rotation())
    surf.set_actor_label("WaterSurface")
    surf.set_actor_scale3d(f.get_actor_scale3d())
    sc = surf.static_mesh_component
    sc.modify()
    sc.set_static_mesh(c.static_mesh)
    sc.set_collision_profile_name("NoCollision")
    sc.set_editor_property("can_ever_affect_navigation", False)
    sc.set_editor_property("cast_shadow", False)
    sc.set_material(0, water)
    log("water floor", f.get_actor_label(), f.get_package().get_name(), "surface at", round(f.get_actor_location().z + WATER_DEPTH))
les.save_current_level()
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)

# 5. 캐릭터: 발 디딤 연출 설정. 서 있을 때 발 뼈 높이를 Idle 첫 프레임에서 잰다
A = unreal.AnimationLibrary
plane = unreal.load_asset("/Engine/BasicShapes/Plane")
sphere = unreal.load_asset("/Engine/BasicShapes/Sphere")
splash = unreal.load_asset(SPLASH) if EAL.does_asset_exist(SPLASH) else None
for bp_path, idle_path in CHARACTERS.items():
    idle = unreal.load_asset(idle_path)
    heights = []
    for foot in ("foot_l", "foot_r"):
        x = unreal.Transform()
        side = foot[-1]
        for p in reversed(A.get_bone_poses_for_time(idle, ["root", "pelvis", "thigh_" + side, "calf_" + side, foot], 0.0, False)):
            x = unreal.MathLibrary.compose_transforms(x, p)
        heights.append(x.translation.z)
    bp = unreal.load_asset(bp_path)
    cdo = unreal.get_default_object(bp.generated_class())
    fx = cdo.get_editor_property("footstep_effect")
    fx.set_editor_property("water_materials", [physmat])
    fx.set_editor_property("foot_ground_offset", round(min(heights), 1))
    fx.set_editor_property("water_depth", WATER_DEPTH)
    fx.set_editor_property("ripple_mesh", plane)
    fx.set_editor_property("ripple_material", ripple)
    fx.set_editor_property("droplet_mesh", sphere)
    fx.set_editor_property("droplet_material", drop)
    # 사용자 Niagara 물보라: 발 디딤에만 쓴다. 계산 물방울(DropletsPerStep)과 함께 나오고, 하나만 쓰려면 그쪽 개수를 0으로.
    # 물 가르기(대시 등)는 물결만 남긴다: Niagara도, 양옆 계산 물보라도 끈다(9-3 시험 후 사용자 판단)
    fx.set_editor_property("splash_system", splash)
    # 계산 물방울은 Niagara 물보라가 있으면 끈다(하얀 구슬처럼 보여 사용자 판단으로 끔). Niagara가 없으면 기본 7개
    fx.set_editor_property("droplets_per_step", 0 if splash else 7)
    fx.set_editor_property("wake_splash_system", None)
    fx.set_editor_property("wake_droplets_per_side", 0)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    EAL.save_loaded_asset(bp, False)
    log("footstep", bp_path.rsplit("/", 1)[1], "foot height", [round(h, 1) for h in heights])
