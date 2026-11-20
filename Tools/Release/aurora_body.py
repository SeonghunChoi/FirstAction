# 배포판 플레이어 Aurora: 반응 몽타주, 캐릭터 데이터, 플레이어 BP, 배포 GameMode (9-3)
# aurora.py(공격) 다음에 실행한다. 가드·패리 없음(9-3 결정 C), 회피 몽타주는 GASP 리타깃 단계에서 채운다

SRC = "/Game/ThirdParty/ParagonAurora/Characters/Heroes/Aurora/"
OUT = "/Game/ThirdParty/Release/Aurora/"
GIT_PLAYER_BP = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"
GIT_GAMEMODE_BP = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode"
saved = []

# 반응: 움찔은 빠르게(1.6배), 경직은 원래 속도. 둘 다 뒷부분부터 캔슬 가능 (6-9 규칙)
hit = make_montage(OUT + "Montages/AM_Aurora_Hit_Front", [SRC + "Animations/HitReact_Front"], rates=[1.6], blend_out=0.15)
add_window(hit, 0.30, hit.get_play_length(), "Window.Cancel")
stagger = make_montage(OUT + "Montages/AM_Aurora_Hit_Front_Stagger", [SRC + "Animations/HitReact_Front"], rates=[1.0], blend_out=0.2)
add_window(stagger, 0.70, stagger.get_play_length(), "Window.Cancel")
# 넘어짐: 밀려 넘어져 누웠다가 일어난다. 누운 구간은 칼끝·몸 움직임이 거의 없는 0.95~1.65초
knock = make_montage(OUT + "Montages/AM_Aurora_Knockback", [SRC + "Animations/Knockback_Bwd"], blend_out=0.2)
add_window(knock, 0.95, 1.65, "State.Downed")
groggy = make_montage(OUT + "Montages/AM_Aurora_Groggy", [SRC + "Animations/Stun_Start", SRC + "Animations/Stun_Loop"], blend_out=0.25)
death = make_montage(OUT + "Montages/AM_Aurora_Death", [SRC + "Animations/Death"], blend_out=0.0)
saved += [OUT + "Montages/" + n for n in ["AM_Aurora_Hit_Front", "AM_Aurora_Hit_Front_Stagger", "AM_Aurora_Knockback", "AM_Aurora_Groggy", "AM_Aurora_Death"]]

# 캐릭터 데이터: 깃 플레이어 데이터를 틀로
cdata = make_data(OUT + "Data/DA_Character_Aurora", "/Game/Portfolio/Data/Characters/DA_Character_Player", dict(
    mass=80.0,
    flinch_montage=hit, flinch_head_montage=hit,
    stagger_montage=stagger, stagger_head_montage=stagger,
    knockdown_montage=knock, groggy_montage=groggy, death_montage=death,
    dodge_montage=None, guard_montage=None,
))
saved.append(OUT + "Data/DA_Character_Aurora")
save_all(saved)

# 플레이어 BP: 깃 플레이어 BP의 자식 (입력, 카메라, 락온을 그대로 물려받는다)
def make_child_bp(path, parent_path):
    parent = unreal.load_asset(parent_path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent.generated_class())
    return load_or_create(path, unreal.Blueprint, factory)

player = make_child_bp(OUT + "BP_Player_Aurora", GIT_PLAYER_BP)
cdo = unreal.get_default_object(player.generated_class())
mesh = cdo.get_editor_property("mesh")
mesh.set_editor_property("skeletal_mesh_asset", unreal.load_asset(SRC + "Meshes/Aurora"))
mesh.set_editor_property("anim_class", unreal.load_asset(OUT + "ABP_Aurora").generated_class())
cdo.get_editor_property("weapon_mesh").set_editor_property("static_mesh", None)
cdo.set_editor_property("character_data", cdata)

attack = lambda n: unreal.load_asset(OUT + "Data/DA_Attack_Aurora_" + n)
grants = []
# 기준은 늘 부모(깃) BP의 부여 목록. 자식 목록을 고쳐 쓰면 한 번 비어 버린 칸을 복구하지 못한다
parent_cdo = unreal.get_default_object(unreal.load_asset(GIT_PLAYER_BP).generated_class())
for g in parent_cdo.get_editor_property("startup_abilities"):
    text = g.export_text()
    if "InputTag.Guard" in text:
        continue  # 가드·패리 없음 (9-3 결정 C)
    if "InputTag.LightAttack" in text:
        text = text.replace("/Game/Portfolio/Data/Attacks/DA_Attack_Light_01.DA_Attack_Light_01", attack("Fast_A").get_path_name())
    if "InputTag.HeavyAttack" in text:
        text = text.replace("/Game/Portfolio/Data/Attacks/DA_Attack_Heavy_01.DA_Attack_Heavy_01", attack("Primary_B").get_path_name())  # 강 단독 시작 (9-3 사용자 콤보표)
    n = type(g)()
    n.import_text(text)
    grants.append(n)
cdo.set_editor_property("startup_abilities", grants)
unreal.BlueprintEditorLibrary.compile_blueprint(player)
saved.append(OUT + "BP_Player_Aurora")

gamemode = make_child_bp("/Game/ThirdParty/Release/BP_GameMode_Release", GIT_GAMEMODE_BP)
unreal.get_default_object(gamemode.generated_class()).set_editor_property("default_pawn_class", player.generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(gamemode)
saved.append("/Game/ThirdParty/Release/BP_GameMode_Release")
save_all(saved)

# 확인 출력
cdo = unreal.get_default_object(unreal.load_asset(OUT + "BP_Player_Aurora").generated_class())
log("player mesh", cdo.get_editor_property("mesh").get_editor_property("skeletal_mesh_asset").get_name(),
    "anim", cdo.get_editor_property("mesh").get_editor_property("anim_class").get_name(),
    "data", cdo.get_editor_property("character_data").get_name())
for g in cdo.get_editor_property("startup_abilities"):
    log("grant", g.export_text())
log("pawn", unreal.get_default_object(unreal.load_asset("/Game/ThirdParty/Release/BP_GameMode_Release").generated_class()).get_editor_property("default_pawn_class"))
