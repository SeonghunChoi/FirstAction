# 배포 맵 (9-3): Paragon: Agora 샘플 맵(ParagonSample)을 복제하고 시작 광장에 전투를 배치한다.
# 원형 단상(높이 약 50cm, 원래 플레이어 시작점) 둘레의 걸을 수 있는 구역(약 20x28m)에서 싸운다.
# 맵 대부분은 물 높이(-71cm)의 바닥판이라 내비 메시는 두 광장만 덮는다. common.py 다음에 실행한다

SRC_MAP = "/Game/ThirdParty/ParagonAgora/SampleMap/ParagonSample"
MAP = "/Game/ThirdParty/Release/Maps/Lvl_Release_Agora"
GAMEMODE = "/Game/ThirdParty/Release/BP_GameMode_Release"
KWANG = "/Game/ThirdParty/Release/Kwang/BP_Enemy_Kwang"
GREYSTONE = "/Game/ThirdParty/Release/Greystone/BP_Enemy_Greystone"
NAV_CENTER, NAV_EXTENT = unreal.Vector(0, 1000, 0), unreal.Vector(1800, 2000, 600)
# 적은 맵에 두지 않는다. 구간(AStageEncounter)이 하나씩 생성한다 (stages.py, 게임 흐름). 내비 범위와 경계 벽도 stages.py가 동선 전체로 넓힌다
ENEMIES = []

# 단순 충돌이 없는 바닥 메시: 캐릭터 이동과 내비 메시가 무시해 떨어지거나 길이 끊긴다(연못 광장 바닥, 물 표면 판 -71cm, 계단 등).
# 샘플은 맵 전체를 덮는 SM_SkyFloor의 거대한 볼록 충돌이 대신 떠받치고 있었는데, 이 충돌이 적 시야선도 막아 끈다.
# 바닥 메시는 정밀 충돌을 단순 충돌로 쓰게 바꾼다(에셋 수정, ThirdParty 원본). 얕은 물은 물 표면 판 위를 걷는다
COMPLEX_FLOORS = ["JungleCamp_Green", "JungleCamp_White", "SM_Plane_5x5m", "Dawn_TerminusCore_Stairs_Mid_B",
                  "Ruins_Player_Pedestal_Inner", "Dino_Nest_Piece_C"]

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not EAL.does_asset_exist(MAP):
    les.load_level(SRC_MAP)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    ensure_dir(MAP.rsplit("/", 1)[0])
    unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
les.load_level(MAP)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_asset(GAMEMODE).generated_class())

# 다시 돌려도 쌓이지 않게: 이 스크립트가 놓은 것과 샘플의 전시용 받침대를 지운다
for a in eas.get_all_level_actors():
    n = a.get_class().get_name()
    if n in ("BP_Enemy_Kwang_C", "BP_Enemy_Greystone_C", "NavMeshBoundsVolume", "BP_CharacterPedestal_C"):
        eas.destroy_actor(a)

# 맵 아래 바닥판 SM_SkyFloor는 단순 충돌 상자가 맵 전체를 감싸 적 시야선(AI Sight)과 높이 측정 선이 시작하자마자 걸린다.
# 원본 메시 에셋은 두고 액터만 충돌을 끈다 (보이는 바닥판일 뿐 걸을 곳이 아니다). 이 액터는 샘플 조명 하위 레벨에 있다.
# 켜 두면 캐릭터가 그 거대한 덩어리 안에서 시작해 물 위에서 바닥을 찾지 못하고 떨어진다
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_name() == "SM_SkyFloor":
        # set_collision_enabled는 맵에 저장되지 않는다(다시 열면 켜져 있어 캐릭터가 물 위에서 떨어졌다). 충돌 프로필로 바꿔 저장한다
        a.static_mesh_component.modify()
        a.static_mesh_component.set_collision_profile_name("NoCollision")
        log("sky floor collision off", a.get_actor_label())

# 샘플의 플래너 리플렉션(호수 반사)은 지운다(9-4 이후 결정). 프로젝트 설정의 글로벌 클립 플레인이 꺼져 있어 제대로 동작하지 않고
# ("플레이너 리플렉션의 정상 작동을 위해서는 글로벌 클립 플레인..." 경고), 켜면 모든 셰이더 재컴파일과 그리기 비용이 든다. 물 반사는 Lumen이 맡는다
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.PlanarReflection):
        log("planar reflection removed", a.get_actor_label())
        eas.destroy_actor(a)

floor_meshes = {}
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_name() in COMPLEX_FLOORS:
        floor_meshes[a.static_mesh_component.static_mesh.get_path_name()] = a.static_mesh_component.static_mesh
for path, mesh in floor_meshes.items():
    LIB.set_use_complex_as_simple_collision(mesh, True)
    EAL.save_loaded_asset(mesh, False)
log("complex floors", sorted(m.get_name() for m in floor_meshes.values()))

nav = eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume, NAV_CENTER)
nav.set_actor_scale3d(unreal.Vector(NAV_EXTENT.x / 100.0, NAV_EXTENT.y / 100.0, NAV_EXTENT.z / 100.0))   # 기본 브러시 반지름 100

# 샘플의 시작점은 단상 가장자리 무너진 건물 옆이라 카메라가 덤불에 갇힌다. 단상 가운데로 옮기고 적들 쪽을 보게 한다
START_XY = (-30, 400)
ps = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.PlayerStart)][0]
g = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(START_XY[0], START_XY[1], 4000), unreal.Vector(START_XY[0], START_XY[1], -500), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [], unreal.DrawDebugTrace.NONE, True).to_tuple()
cx, cy = (sum(e[2] for e in ENEMIES) / len(ENEMIES), sum(e[3] for e in ENEMIES) / len(ENEMIES)) if ENEMIES else (0, 1500)   # 광장 안쪽을 본다
ps.set_actor_location_and_rotation(unreal.Vector(START_XY[0], START_XY[1], g[4].z + 100),
                                   unreal.Rotator(0, 0, unreal.MathLibrary.find_look_at_rotation(unreal.Vector(START_XY[0], START_XY[1], 0), unreal.Vector(cx, cy, 0)).yaw), False, True)
start = ps.get_actor_location()
for path, label, x, y in ENEMIES:
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 4000), unreal.Vector(x, y, -500), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [], unreal.DrawDebugTrace.NONE, True).to_tuple()  # 정밀 충돌: 맵을 감싸는 메시의 단순 충돌에 걸리지 않게
    z = hit[4].z + 110 if hit[0] else 100
    yaw = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(x, y, 0), unreal.Vector(start.x, start.y, 0)).yaw
    a = eas.spawn_actor_from_class(unreal.load_asset(path).generated_class(), unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw))
    a.set_actor_label(label)
    log("enemy", label, round(z), round(yaw))

# 전투 구역 경계: 물 표면 판이 맵 거의 전체를 덮어 플레이어가 내비 범위 밖 물 위로 나가면 적이 길을 못 찾는다(9-3 결정: 보이지 않는 벽).
# 내비 범위 둘레에 안 보이는 벽 4개. 캐릭터만 막고 카메라와 시야선(Visibility)은 통과한다
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("Boundary_"):
        eas.destroy_actor(a)
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
WALL_H, WALL_T = 1000.0, 100.0
x0, x1 = NAV_CENTER.x - NAV_EXTENT.x, NAV_CENTER.x + NAV_EXTENT.x
y0, y1 = NAV_CENTER.y - NAV_EXTENT.y, NAV_CENTER.y + NAV_EXTENT.y
walls = [("Boundary_XMin", (x0 - WALL_T / 2, (y0 + y1) / 2), (WALL_T, y1 - y0 + 2 * WALL_T)),
         ("Boundary_XMax", (x1 + WALL_T / 2, (y0 + y1) / 2), (WALL_T, y1 - y0 + 2 * WALL_T)),
         ("Boundary_YMin", ((x0 + x1) / 2, y0 - WALL_T / 2), (x1 - x0 + 2 * WALL_T, WALL_T)),
         ("Boundary_YMax", ((x0 + x1) / 2, y1 + WALL_T / 2), (x1 - x0 + 2 * WALL_T, WALL_T))]
for label, (cx_, cy_), (sx, sy) in walls:
    wa = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx_, cy_, WALL_H / 2 - 200))   # 물 표면(-71) 아래부터
    wa.set_actor_label(label)
    wa.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, WALL_H / 100.0))
    c = wa.static_mesh_component
    c.modify()
    c.set_static_mesh(cube)
    c.set_collision_profile_name("BlockAll")
    c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA, unreal.CollisionResponseType.ECR_IGNORE)
    c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY, unreal.CollisionResponseType.ECR_IGNORE)
    c.set_editor_property("visible", False)
    c.set_editor_property("can_ever_affect_navigation", False)   # 내비 범위 가장자리에 있어 내비를 깎지 않게
    wa.set_actor_hidden_in_game(True)
log("boundary walls", [w[0] for w in walls])

unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
les.save_current_level()
# SM_SkyFloor는 함께 불러오는 샘플 조명 하위 레벨(ParagonSample_Daylight)에 있어 그 레벨도 저장한다
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
log("map", MAP, "start", start, "navmesh", [a.get_actor_label() for a in eas.get_all_level_actors() if a.get_class().get_name() == "RecastNavMesh"])
