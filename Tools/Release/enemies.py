# 배포판 적: 일반 Kwang, 엘리트 Greystone (9-3). 공격 몽타주·데이터, 반응 몽타주, 캐릭터·패턴 데이터, 적 BP
# 콤보(9-3 결정): Kwang Combo3 A→B→C, Combo4 A→B→C→D(마무리 넘어뜨림), Flurry Ability_R(토큰 2)
#                 Greystone QuickJab Fast A→B→C, HeavyCombo PrimaryA→B→C(토큰 2), Slam Ability_R(토큰 2), Mix Fast_A→PrimaryB
# 판정 구간은 measure_swings.py 측정값(칼끝 속도 + 정면 공간 통과)에 앞뒤 여유를 붙였다. 모두 제자리 동작이라
# 워핑 구간 동안 GA_MeleeAttack이 대상 쪽으로 옮긴다(9-3 접근 이동)
#
# Kwang 공격 클립은 0.4초 뒤부터 자세를 오래 유지하고 회복 클립이 따로 있다. 0.8초에서 잘라 회복 클립을 잇는다
# (자른 자리와 회복 시작의 칼끝 차이 9~22cm로 가장 가깝다). Kwang Ability_R은 측정해 보니 2연격(회전 베기, 내려찍기)이다.
# Greystone PrimaryA의 0.65초 무렵 자세가 PrimaryB 시작, PrimaryB의 0.7초 무렵이 PrimaryC 시작과 같아 콤보 구간을 그 근처에 둔다.
# Paragon 두 캐릭터 모두 바닥에 눕는 동작이 없다. 넘어짐 칸은 크게 밀리는 반응(Kwang Knockback_Bwd, Greystone 경직)으로 채우고
# State.Downed 구간은 두지 않는다(검증 경고는 의도)

GIT = "/Game/Portfolio/"
GA = GIT + "Data/Attacks/"
L, H = "InputTag.LightAttack", "InputTag.HeavyAttack"
saved = []


def body(out, prefix, src, flinch, knock, groggy, death):
    """반응 몽타주: 움찔(1.6배), 경직(1.0배), 넘어짐, 그로기, 사망"""
    ms = {}
    m = make_montage(out + "Montages/AM_%s_Hit_Front" % prefix, [src + flinch], rates=[1.6], blend_out=0.15)
    add_window(m, 0.30, m.get_play_length(), "Window.Cancel")
    ms["flinch"] = m
    m = make_montage(out + "Montages/AM_%s_Hit_Front_Stagger" % prefix, [src + flinch], rates=[1.0], blend_out=0.2)
    add_window(m, 0.60, m.get_play_length(), "Window.Cancel")
    ms["stagger"] = m
    ms["knock"] = make_montage(out + "Montages/AM_%s_Knockback" % prefix, [src + knock[0]], rates=[knock[1]], blend_out=0.25) if knock else m
    ms["groggy"] = make_montage(out + "Montages/AM_%s_Groggy" % prefix, [src + s for s, _, _ in groggy],
                                ranges=[(a, b if b is not None else unreal.load_asset(src + s).get_play_length()) for s, a, b in groggy], blend_out=0.25)
    ms["death"] = make_montage(out + "Montages/AM_%s_Death" % prefix, [src + death], blend_out=0.0)
    save_all([mm.get_path_name().split(".")[0] for mm in set(ms.values())])
    return ms


TELEGRAPH = (0.55, 0.8, 0.5)   # 구간 없는 몽타주의 첫 타·콤보 배율, 구간 있는 몽타주의 콤보 적용 비율


def character_data(path, template, mass, ms):
    d = make_data(path, template, dict(
        mass=mass,
        flinch_montage=ms["flinch"], flinch_head_montage=ms["flinch"],
        stagger_montage=ms["stagger"], stagger_head_montage=ms["stagger"],
        knockdown_montage=ms["knock"], groggy_montage=ms["groggy"], death_montage=ms["death"],
        dodge_montage=None, guard_montage=None,   # 적은 회피·가드를 쓰지 않는다 (틀의 마네킹 몽타주를 남기지 않는다)
        # 공격 예비 동작 감속 (9-4 이후): 첫 타 크게, 콤보로 이어진 타는 조금
        telegraph_time_scale=TELEGRAPH[0], telegraph_combo_time_scale=TELEGRAPH[1], telegraph_combo_strength=TELEGRAPH[2],
    ))
    saved.append(path)
    return d


def pattern_data(path, template, patterns):
    d = make_data(path, template, {})
    d.set_editor_property("patterns", patterns)
    saved.append(path)
    return d


def enemy_bp(path, parent, mesh, abp, cdata, pdata, scale):
    bp = make_child_bp(path, parent)
    cdo = unreal.get_default_object(bp.generated_class())
    m = cdo.get_editor_property("mesh")
    m.set_editor_property("skeletal_mesh_asset", unreal.load_asset(mesh))
    m.set_editor_property("anim_class", unreal.load_asset(abp).generated_class())
    cdo.get_editor_property("weapon_mesh").set_editor_property("static_mesh", None)   # 검은 몸 메시에 붙어 있다(몸 메시 판정 소켓)
    cdo.set_editor_property("character_data", cdata)
    cdo.set_editor_property("pattern_data", pdata)
    cdo.get_editor_property("root_component").set_editor_property("relative_scale3d", unreal.Vector(scale, scale, scale))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    saved.append(path)
    return bp


# ---------------------------------------------------------------- Kwang (일반)
KS = "/Game/ThirdParty/ParagonKwang/Characters/Heroes/Kwang/"
KA = KS + "Animations/"
KO = "/Game/ThirdParty/Release/Kwang/"
kwang_attacks = [
    # 이름, [(시퀀스, 시작, 끝)], 블렌드 아웃, 판정, 워핑, 콤보, 캔슬 시작
    ("A", [(KA + "PrimaryAttack_A_Slow", 0.0, 0.8), (KA + "PrimaryAttack_A_Slow_Recovery", 0.0, None)], 0.2, [(0.15, 0.27)], (0.0, 0.17), (0.28, 0.95), 1.0),
    ("B", [(KA + "PrimaryAttack_B_Slow", 0.0, 0.8), (KA + "PrimaryAttack_B_Slow_Recovery", 0.0, None)], 0.2, [(0.15, 0.28)], (0.0, 0.17), (0.28, 0.95), 1.0),
    ("C", [(KA + "PrimaryAttack_C_Slow", 0.0, 0.8), (KA + "PrimaryAttack_C_Slow_Recovery", 0.0, None)], 0.2, [(0.15, 0.26)], (0.0, 0.17), (0.28, 0.95), 1.0),
    ("D", [(KA + "PrimaryAttack_D_Slow", 0.0, None)], 0.25, [(0.16, 0.29)], (0.0, 0.18), None, 0.9),
    ("Flurry", [(KA + "Ability_R", 0.0, None)], 0.25, [(0.09, 0.18), (0.41, 0.53)], (0.0, 0.11), None, 1.0),
]
kwang_values = {
    "A": (GA + "DA_Attack_Light_01", dict(damage=10.0, impact_force=24000.0, post_hit_spacing=60.0, hit_stop_duration=0.06, max_warp_distance=200.0)),
    "B": (GA + "DA_Attack_Light_02", dict(damage=10.0, impact_force=24000.0, post_hit_spacing=60.0, hit_stop_duration=0.06, max_warp_distance=200.0)),
    "C": (GA + "DA_Attack_Light_02", dict(damage=12.0, impact_force=30000.0, post_hit_spacing=70.0, hit_stop_duration=0.06, max_warp_distance=200.0)),
    "D": (GA + "DA_Attack_Light_03", dict(damage=15.0, impact_force=64000.0, post_hit_spacing=200.0, post_hit_move_duration=0.35, hit_stop_duration=0.08, max_warp_distance=200.0)),
    "Flurry": (GA + "DA_Attack_Heavy_01", dict(damage=12.0, impact_force=36000.0, post_hit_spacing=90.0, post_hit_move_duration=0.2, hit_stop_duration=0.08, max_warp_distance=250.0)),
}
# 예비 동작 구간 (9-4 이후 2차): Kwang 공격 클립은 첫 프레임이 이미 칼을 치켜든 정점이고 0.03초 뒤 내려친다(칼끝 속도 측정).
# 맨 앞 몇 프레임을 아주 느리게 해 "들고 멈칫했다가 내리침"으로 만든다. (시작, 끝, 배율), 몽타주 시간
KWANG_TELEGRAPH = {"A": [(0.005, 0.05, 0.12)], "B": [(0.005, 0.05, 0.12)], "C": [(0.005, 0.06, 0.15)],
                   "D": [(0.005, 0.07, 0.12)], "Flurry": [(0.005, 0.04, 0.12)]}
kd = build_attacks(KO, "Kwang", kwang_attacks, kwang_values, {"A": [(L, "B")], "B": [(L, "C")], "C": [(L, "D")]}, KWANG_TELEGRAPH)
kms = body(KO, "Kwang", KA, "Hitreact_Fwd", ("Knockback_Bwd", 1.3), [("Stun_Start", 0.0, None), ("Stun_Loop", 0.0, None)], "Death_Bwd")
kc = character_data(KO + "Data/DA_Character_Kwang", GIT + "Data/Characters/DA_Character_Enemy", 80.0, kms)
kp = pattern_data(KO + "Data/DA_Pattern_Kwang", GIT + "Data/Enemies/DA_Pattern_Enemy", [
    pattern("Combo3", kd["A"], [L, L], 3.0, 0.0, 350.0, 4.0, 1),
    pattern("Combo4", kd["A"], [L, L, L], 2.0, 0.0, 350.0, 6.0, 1),
    pattern("Flurry", kd["Flurry"], [], 1.0, 100.0, 450.0, 8.0, 2),
])
save_all(saved)
enemy_bp(KO + "BP_Enemy_Kwang", GIT + "Characters/BP_Enemy", KS + "Meshes/Kwang_GDC", KO + "ABP_Kwang", kc, kp, 1.0)

# ---------------------------------------------------------------- Greystone (엘리트)
GS = "/Game/ThirdParty/ParagonGreystone/Characters/Heroes/Greystone/"
GAN = GS + "Animations/"
GO = "/Game/ThirdParty/Release/Greystone/"
grey_attacks = [
    ("Fast_A", [(GAN + "Attack_A_Fast", 0.0, None)], 0.1, [(0.12, 0.21)], (0.0, 0.13), (0.22, 0.50), 0.30),
    ("Fast_B", [(GAN + "Attack_B_Fast", 0.0, None)], 0.1, [(0.10, 0.19)], (0.0, 0.11), (0.22, 0.50), 0.30),
    ("Fast_C", [(GAN + "Attack_C_Fast", 0.0, None)], 0.2, [(0.13, 0.21)], (0.0, 0.14), None, 0.35),
    ("PrimaryA", [(GAN + "Attack_PrimaryA", 0.0, None)], 0.25, [(0.35, 0.47)], (0.0, 0.36), (0.48, 0.80), 0.90),
    ("PrimaryB", [(GAN + "Attack_PrimaryB", 0.0, None)], 0.25, [(0.36, 0.47)], (0.0, 0.37), (0.48, 0.85), 0.90),
    ("PrimaryC", [(GAN + "Attack_PrimaryC", 0.0, None)], 0.25, [(0.59, 0.71)], (0.0, 0.60), None, 1.10),
    ("Slam", [(GAN + "Ability_R", 0.0, None)], 0.25, [(0.85, 0.95)], (0.55, 0.86), None, 1.30),
]
grey_values = {
    "Fast_A": (GA + "DA_Attack_Light_01", dict(damage=8.0, impact_force=30000.0, post_hit_spacing=50.0, hit_stop_duration=0.05, max_warp_distance=200.0)),
    "Fast_B": (GA + "DA_Attack_Light_02", dict(damage=8.0, impact_force=30000.0, post_hit_spacing=50.0, hit_stop_duration=0.05, max_warp_distance=200.0)),
    "Fast_C": (GA + "DA_Attack_Light_02", dict(damage=12.0, impact_force=48000.0, post_hit_spacing=120.0, hit_stop_duration=0.08, max_warp_distance=200.0)),
    "PrimaryA": (GA + "DA_Attack_Heavy_01", dict(damage=15.0, impact_force=40000.0, post_hit_spacing=80.0, post_hit_move_duration=0.2, hit_stop_duration=0.08, max_warp_distance=250.0)),
    "PrimaryB": (GA + "DA_Attack_Heavy_01", dict(damage=18.0, impact_force=44000.0, post_hit_spacing=90.0, post_hit_move_duration=0.2, hit_stop_duration=0.09, max_warp_distance=250.0)),
    "PrimaryC": (GA + "DA_Attack_Heavy_01", dict(damage=25.0, impact_force=64000.0, post_hit_spacing=200.0, post_hit_move_duration=0.35, hit_stop_duration=0.12, max_warp_distance=250.0)),
    "Slam": (GA + "DA_Attack_Heavy_01", dict(damage=30.0, impact_force=72000.0, post_hit_spacing=220.0, post_hit_move_duration=0.35, hit_stop_duration=0.14, max_warp_distance=300.0)),
}
# 예비 동작 구간: Fast는 Kwang처럼 맨 앞, Primary·Slam은 칼끝이 가장 느린 정점(측정 0.27 / 0.18 / 0.47 / 0.33~0.63초) 부근
GREY_TELEGRAPH = {"Fast_A": [(0.005, 0.04, 0.12)], "Fast_B": [(0.005, 0.04, 0.12)], "Fast_C": [(0.005, 0.04, 0.12)],
                  "PrimaryA": [(0.22, 0.30, 0.25)], "PrimaryB": [(0.14, 0.24, 0.3)], "PrimaryC": [(0.42, 0.50, 0.3)],
                  "Slam": [(0.60, 0.68, 0.35)]}
gd = build_attacks(GO, "Greystone", grey_attacks, grey_values, {
    "Fast_A": [(L, "Fast_B"), (H, "PrimaryB")], "Fast_B": [(L, "Fast_C")],
    "PrimaryA": [(H, "PrimaryB")], "PrimaryB": [(H, "PrimaryC")],
}, GREY_TELEGRAPH)
gms = body(GO, "Greystone", GAN, "HitReact_Front", None, [("Stun_Start", 0.0, None), ("Stun_Idle", 0.0, 1.0)], "Death")
gc = character_data(GO + "Data/DA_Character_Greystone", GIT + "Data/Characters/DA_Character_Elite", 180.0, gms)
gp = pattern_data(GO + "Data/DA_Pattern_Greystone", GIT + "Data/Enemies/DA_Pattern_Elite", [
    pattern("QuickJab", gd["Fast_A"], [L, L], 3.0, 0.0, 350.0, 4.0, 1),
    pattern("HeavyCombo", gd["PrimaryA"], [H, H], 2.0, 0.0, 400.0, 6.0, 2),
    pattern("Slam", gd["Slam"], [], 1.0, 150.0, 500.0, 8.0, 2),
    pattern("Mix", gd["Fast_A"], [H], 2.0, 0.0, 350.0, 5.0, 1),
])
save_all(saved)
enemy_bp(GO + "BP_Enemy_Greystone", GIT + "Characters/BP_Enemy_Elite", GS + "Meshes/Greystone", GO + "ABP_Greystone", gc, gp, 1.0)
save_all(saved)

# 확인 출력: 데이터 검증(8-3)
for p in saved:
    obj = unreal.load_asset(p)
    if isinstance(obj, unreal.Blueprint):
        c = unreal.get_default_object(obj.generated_class())
        log("bp", p.rsplit("/", 1)[1], c.get_editor_property("mesh").get_editor_property("skeletal_mesh_asset").get_name(),
            c.get_editor_property("character_data").get_name(), c.get_editor_property("pattern_data").get_name())
log("done", len(saved))
