# 배포 맵 동선 (게임 흐름, 9-4 이후): 시작 광장 → 원형 극장 → 윗 광장 → 신전, 4구간.
# 구간마다 AStageEncounter(들어오면 시작, 적을 한 마리씩), 구간 사이 가로선에 문(AStageDoor, 다 잡으면 바닥으로 내려감)과 벽.
# 얕은 물과 낮은 바닥이 거의 다 이어져 있어(내비 확인) 구간 사이는 동선 폭 전체를 가로로 막는다:
#   보이지 않는 막이(캐릭터만 막음) + 낮은 바닥 위에만 보이는 폐허 벽(JungleWall_02A). 높은 바위 위는 바위가 벽 역할.
# release_map.py 다음에 실행한다. 다시 돌리면 "Stage_"로 시작하는 액터를 지우고 새로 놓는다
#   cat Tools/Release/common.py Tools/Release/stages.py > tmp && python Tools/ue_exec.py tmp

MAP = "/Game/ThirdParty/Release/Maps/Lvl_Release_Agora"
KWANG = "/Game/ThirdParty/Release/Kwang/BP_Enemy_Kwang"
GREYSTONE = "/Game/ThirdParty/Release/Greystone/BP_Enemy_Greystone"
WALL_MESH = "/Game/ThirdParty/ParagonProps/Monolith/Ruins/Meshes/JungleWall_02A"        # 800 x 102 x 403, 길이 X축
PILLAR_MESH = "/Game/ThirdParty/ParagonProps/Monolith/Ruins/Meshes/JunglePillarBlock_01A"  # 259 x 259 x 250

# 동선 전체 (x, y 범위). 내비 범위와 바깥 경계 벽
X0, X1, Y0, Y1 = -2400.0, 2400.0, -8800.0, 2950.0
NAV_Z0, NAV_Z1 = -600.0, 2400.0

# 구간 사이 가로선: (y, 문 가운데 x). 문 폭은 벽 메시 한 장(800)
DIVIDERS = [(-2700.0, 675.0), (-5250.0, 600.0), (-7100.0, -125.0)]
DOOR_W = 800.0

K, G = "K", "G"
# 구간: 번호, 상자(가운데, 반지름), 웨이브(한 웨이브는 한꺼번에, 다 쓰러지면 다음), 적 나오는 곳, 다시 하기 위치(구간 상자 밖 입구 앞)
STAGES = [
    (1, (0, 1100, 300), (1500, 1700, 900), [[K], [K]], [(700, 1500), (-700, 1500), (0, 2200)], (-30, 400)),
    (2, (300, -4150, 400), (1900, 950, 1000), [[K], [K], [K]], [(-450, -4350), (0, -4500), (450, -3750)], (700, -2350)),
    (3, (0, -6300, 300), (2000, 700, 1000), [[K, K]], [(-300, -6450), (600, -6300), (300, -6750)], (600, -4950)),
    (4, (-100, -7900, 300), (1500, 600, 1000), [[K], [G, K]], [(-500, -8100), (500, -7900), (0, -7650)], (-125, -6850)),
]
# 메뉴 배경 카메라: 시작 광장을 내려다본다
MENU_CAM = ((1300, 2700, 600), (0, 600, 50))

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not ues.get_editor_world().get_path_name().startswith(MAP):
    les.load_level(MAP)
world = ues.get_editor_world()

# 다시 돌려도 쌓이지 않게. 맵에 직접 둔 적과 옛 경계 벽·내비 범위도 지운다(구간이 적을 생성한다)
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if label.startswith("Stage_") or label.startswith("Boundary_") or a.get_class().get_name() in ("BP_Enemy_Kwang_C", "BP_Enemy_Greystone_C", "NavMeshBoundsVolume"):
        eas.destroy_actor(a)

placed = []


def ground(x, y, ignore=None):
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 4000), unreal.Vector(x, y, -3000),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, ignore or [], unreal.DrawDebugTrace.NONE, True).to_tuple()
    return hit[4].z if hit[0] else -71.0


def mesh_bottom_offset(mesh, scale):
    """메시 바닥 가운데를 액터 원점에 맞추는 상대 위치"""
    b = mesh.get_bounds()
    o, e = b.origin, b.box_extent
    return unreal.Vector(-o.x * scale.x, -o.y * scale.y, -(o.z - e.z) * scale.z)


def blocker(label, cx, cy, sx, sy, z0=-300.0, z1=2600.0):
    """보이지 않는 막이: 캐릭터만 막고 카메라·시야선(Visibility)은 통과한다 (9-3 경계 벽과 같은 설정)"""
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx, cy, (z0 + z1) / 2))
    a.set_actor_label(label)
    a.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, (z1 - z0) / 100.0))
    c = a.static_mesh_component
    c.modify()
    c.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    c.set_collision_profile_name("BlockAll")
    c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA, unreal.CollisionResponseType.ECR_IGNORE)
    c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY, unreal.CollisionResponseType.ECR_IGNORE)
    c.set_editor_property("visible", False)
    c.set_editor_property("can_ever_affect_navigation", False)
    a.set_actor_hidden_in_game(True)
    placed.append(label)
    return a


def deco_mesh(label, mesh_path, x, y, z, yaw=0.0, scale=unreal.Vector(1, 1, 1), sink=40.0):
    mesh = unreal.load_asset(mesh_path)
    off = mesh_bottom_offset(mesh, scale)
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x + off.x, y + off.y, z + off.z - sink), unreal.Rotator(0, 0, yaw))
    a.set_actor_label(label)
    a.set_actor_scale3d(scale)
    c = a.static_mesh_component
    c.modify()
    c.set_static_mesh(mesh)
    c.set_collision_profile_name("BlockAll")
    c.set_editor_property("can_ever_affect_navigation", False)
    placed.append(label)
    return a


# ---------------------------------------------------------------- 내비 범위, 바깥 경계
nav = eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector((X0 + X1) / 2, (Y0 + Y1) / 2, (NAV_Z0 + NAV_Z1) / 2))
nav.set_actor_label("Stage_NavBounds")
nav.set_actor_scale3d(unreal.Vector((X1 - X0) / 200.0, (Y1 - Y0) / 200.0, (NAV_Z1 - NAV_Z0) / 200.0))   # 기본 브러시 반지름 100
T = 100.0
blocker("Stage_Boundary_XMin", X0 - T / 2, (Y0 + Y1) / 2, T, Y1 - Y0 + 2 * T)
blocker("Stage_Boundary_XMax", X1 + T / 2, (Y0 + Y1) / 2, T, Y1 - Y0 + 2 * T)
blocker("Stage_Boundary_YMin", (X0 + X1) / 2, Y0 - T / 2, X1 - X0 + 2 * T, T)
blocker("Stage_Boundary_YMax", (X0 + X1) / 2, Y1 + T / 2, X1 - X0 + 2 * T, T)

# ---------------------------------------------------------------- 구간 사이: 막이, 벽, 문
wall_mesh = unreal.load_asset(WALL_MESH)
doors = []
for n, (y, dx) in enumerate(DIVIDERS, start=1):
    l, r = dx - DOOR_W / 2, dx + DOOR_W / 2
    # 바닥 높이는 벽을 놓기 전에 잰다 (벽 윗면에 걸리지 않게)
    gz = min(ground(dx + o, y) for o in (-350, 0, 350))
    pillar_z = {"L": ground(l - 130.0, y), "R": ground(r + 130.0, y)}
    blocker("Stage_Divider_%d_L" % n, (X0 + l) / 2, y, l - X0, T)
    blocker("Stage_Divider_%d_R" % n, (r + X1) / 2, y, X1 - r, T)

    # 보이는 벽: 낮은 바닥(물, 길) 위에만 800 간격으로. 문 양쪽부터 바깥으로
    k = 0
    for side, start, stop, step in (("L", l, X0, -1), ("R", r, X1, 1)):
        x = start
        while (x - stop) * step < 0:
            cx = x + step * 400.0
            zs = [ground(cx + o, y) for o in (-350, 0, 350)]
            if max(zs) < 300.0:
                k += 1
                deco_mesh("Stage_Wall_%d_%s%d" % (n, side, k), WALL_MESH, cx, y, min(zs))
            x += step * 800.0

    # 문: 벽 메시 한 장, 열리면 바닥 아래로 내려간다. 양옆 기둥
    door = eas.spawn_actor_from_class(unreal.StageDoor, unreal.Vector(dx, y, gz))
    door.set_actor_label("Stage_Door_%d" % n)
    dm = door.get_editor_property("door_mesh")
    dm.modify()
    dm.set_static_mesh(wall_mesh)
    off = mesh_bottom_offset(wall_mesh, unreal.Vector(1, 1, 1))
    dm.set_relative_location(unreal.Vector(off.x, off.y, off.z - 40.0), False, False)
    door.set_editor_property("open_offset", unreal.Vector(0, 0, -520))
    placed.append("Stage_Door_%d" % n)
    doors.append(door)
    for side, px in (("L", l - 130.0), ("R", r + 130.0)):
        deco_mesh("Stage_DoorPillar_%d_%s" % (n, side), PILLAR_MESH, px, y, pillar_z[side], scale=unreal.Vector(0.8, 0.8, 2.0))

# ---------------------------------------------------------------- 구간
classes = {K: unreal.load_asset(KWANG).generated_class(), G: unreal.load_asset(GREYSTONE).generated_class()}


def walkable_z(x, y):
    """걸을 수 있는 면의 높이: 내비 메시에 투영한다. 아래로 쏜 선은 나무 꼭대기에 걸리거나, 가시성 충돌이 없는 바닥을 뚫고
    그 아래 물 바닥에 닿아(윗 광장) 적이 바닥 밑에 생겼다. 내비가 없으면 선으로"""
    for extent in (300.0, 1500.0):   # 바닥 높이(약 -70~200) 근처를 먼저, 없으면 넓게
        p = unreal.NavigationSystemV1.project_point_to_navigation(world, unreal.Vector(x, y, 100.0), None, None, unreal.Vector(60, 60, extent))
        if p is not None:
            return p.z
    return ground(x, y)


def point(label, x, y, yaw):
    p = eas.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(x, y, walkable_z(x, y) + 100.0), unreal.Rotator(0, 0, yaw))
    p.set_actor_label(label)
    placed.append(label)
    return p


def make_wave(enemy_classes):
    w = unreal.StageWave()
    w.set_editor_property("enemies", enemy_classes)
    return w


for num, center, extent, enemies, spawns, respawn in STAGES:
    enc = eas.spawn_actor_from_class(unreal.StageEncounter, unreal.Vector(*center))
    enc.set_actor_label("Stage_Encounter_%d" % num)
    area = enc.get_editor_property("area")
    area.modify()
    area.set_box_extent(unreal.Vector(*extent), False)
    enc.set_editor_property("stage_number", num)
    enc.set_editor_property("waves", [make_wave([classes[e] for e in wave]) for wave in enemies])
    enc.set_editor_property("spawn_points", [point("Stage_Spawn_%d_%d" % (num, i + 1), x, y, 0) for i, (x, y) in enumerate(spawns)])
    enc.set_editor_property("respawn_point", point("Stage_Respawn_%d" % num, respawn[0], respawn[1], -90.0 if num > 1 else 90.0))
    enc.set_editor_property("doors_to_close_on_start", [doors[num - 2]] if num > 1 else [])
    enc.set_editor_property("doors_to_open_on_clear", [doors[num - 1]] if num <= len(doors) else [])
    placed.append("Stage_Encounter_%d" % num)
    log("stage", num, "enemies", enemies, "spawns", len(spawns))

# ---------------------------------------------------------------- 메뉴 카메라
(cx, cy, cz), (tx, ty, tz) = MENU_CAM
rot = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(cx, cy, cz), unreal.Vector(tx, ty, tz))
cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(cx, cy, cz), rot)
cam.set_actor_label("Stage_MenuCamera")
cam.set_editor_property("tags", [unreal.Name("MenuCamera")])
placed.append("Stage_MenuCamera")

unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
les.save_current_level()
log("placed", len(placed))
