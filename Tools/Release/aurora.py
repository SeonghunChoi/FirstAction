# 배포판 플레이어 Aurora: 공격 몽타주·데이터 9개(콤보 8 + 돌진 후보 Ability_Q) (9-3)
# 콤보(9-3, 시험 후 사용자 콤보표): 약 Fast_A→B→C→D, C에서 강→Slow_D, D에서 강→Ability_E, 강 단독 Primary_B→C→Slow_D
# 판정 구간은 칼끝 속도와 칼이 정면 공간(앞 40~170, 높이 20~180cm)을 지나는 시간을 함께 보고 앞뒤 여유를 붙였다
# (Ability_R은 칼이 머리 위로 돌아 정면을 거의 지나지 않아 Ability_Q로 교체, 9-3 시험)

SRC = "/Game/ThirdParty/ParagonAurora/Characters/Heroes/Aurora/Animations/"
OUT = "/Game/ThirdParty/Release/Aurora/"
GIT = "/Game/Portfolio/Data/Attacks/"

# 이름, 시퀀스, 블렌드 아웃, 판정 구간들, 워핑 끝, 콤보 구간(없으면 None), 캔슬 시작
# 판정 구간: 칼끝 속도 + 정면 공간 통과 측정값(Primary_Attack_B/C는 1.0초, 정면 0.22~0.32 / Slow_D 2.53초, 0.23~0.34)
ATTACKS = [
    ("Fast_A", "Primary_Attack_Fast_A", 0.10, [(0.14, 0.29)], 0.14, (0.24, 0.50), 0.30),
    ("Fast_B", "Primary_Attack_Fast_B", 0.10, [(0.12, 0.25)], 0.12, (0.22, 0.50), 0.28),
    ("Fast_C", "Primary_Attack_Fast_C", 0.10, [(0.10, 0.25)], 0.10, (0.22, 0.50), 0.28),
    ("Fast_D", "Primary_Attack_Fast_D", 0.25, [(0.19, 0.32)], 0.19, (0.30, 0.90), 0.60),
    ("Primary_B", "Primary_Attack_B", 0.15, [(0.16, 0.34)], 0.16, (0.30, 0.85), 0.40),
    ("Primary_C", "Primary_Attack_C", 0.15, [(0.17, 0.36)], 0.17, (0.30, 0.85), 0.40),
    ("Slow_D", "Primary_Attack_Slow_D", 0.25, [(0.20, 0.38)], 0.20, None, 0.90),
    ("Ability_E", "Ability_E", 0.25, [(0.32, 0.45)], 0.32, None, 0.80),
    # Ability_Q: 콤보에서 뺐다. 코드로 위치를 옮기는 돌진 기술 후보로 남겨 둔다 (9-3 시험)
    ("Ability_Q", "Ability_Q", 0.25, [(0.30, 0.46)], 0.30, None, 0.90),
]

# 깃 데이터를 틀로 쓰고 값만 덮어쓴다 (수치는 깃 기준값에서 시작해 튜닝)
VALUES = {
    "Fast_A": (GIT + "DA_Attack_Light_01", dict(damage=8.0, impact_force=24000.0, post_hit_spacing=60.0, hit_stop_duration=0.05, max_warp_distance=250.0)),
    "Fast_B": (GIT + "DA_Attack_Light_02", dict(damage=8.0, impact_force=24000.0, post_hit_spacing=60.0, hit_stop_duration=0.05, max_warp_distance=250.0)),
    "Fast_C": (GIT + "DA_Attack_Light_02", dict(damage=10.0, impact_force=30000.0, post_hit_spacing=70.0, hit_stop_duration=0.06, max_warp_distance=250.0)),
    "Fast_D": (GIT + "DA_Attack_Light_03", dict(damage=15.0, impact_force=64000.0, post_hit_spacing=200.0, hit_stop_duration=0.10, max_warp_distance=250.0)),
    "Primary_B": (GIT + "DA_Attack_Heavy_01", dict(damage=14.0, impact_force=40000.0, post_hit_spacing=100.0, post_hit_move_duration=0.2, hit_stop_duration=0.08, max_warp_distance=250.0)),
    "Primary_C": (GIT + "DA_Attack_Heavy_01", dict(damage=16.0, impact_force=44000.0, post_hit_spacing=110.0, post_hit_move_duration=0.2, hit_stop_duration=0.09, max_warp_distance=250.0)),
    "Slow_D": (GIT + "DA_Attack_Heavy_01", dict(damage=25.0, impact_force=64000.0, post_hit_spacing=220.0, post_hit_move_duration=0.35, hit_stop_duration=0.12, max_warp_distance=250.0)),
    "Ability_E": (GIT + "DA_Attack_Heavy_01", dict(damage=25.0, impact_force=56000.0, post_hit_spacing=170.0, hit_stop_duration=0.12, max_warp_distance=300.0)),
    "Ability_Q": (GIT + "DA_Attack_Heavy_01", dict(damage=20.0, impact_force=56000.0, post_hit_spacing=180.0, hit_stop_duration=0.12, max_warp_distance=250.0)),
}

# 교체 전 에셋 정리
for old in [OUT + "Montages/AM_Aurora_Ability_R", OUT + "Data/DA_Attack_Aurora_Ability_R"]:
    if EAL.does_asset_exist(old):
        EAL.delete_asset(old)

saved = []
montages = {}
for name, seq, blend_out, hits, warp_end, combo, cancel_start in ATTACKS:
    path = OUT + "Montages/AM_Aurora_" + name
    m = make_montage(path, [SRC + seq], blend_out=blend_out)
    length = m.get_play_length()
    add_warp(m, 0.0, warp_end)
    for start, end in hits:
        add_hit(m, start, end)
    if combo:
        add_window(m, combo[0], min(combo[1], length - blend_out), "Window.Combo")
    add_window(m, cancel_start, length, "Window.Cancel")
    montages[name] = m
    saved.append(path)
    log("montage", name, round(length, 3), "hits", len(hits))

data = {}
for name, (template, values) in VALUES.items():
    path = OUT + "Data/DA_Attack_Aurora_" + name
    d = make_data(path, template, values, montages[name])
    d.set_editor_property("combo_links", [])
    data[name] = d
    saved.append(path)

# 콤보 링크는 ComboLinks에만 (규칙 4)
# 9-3 사용자 콤보표: 약 A→B→C→D, C에서 강→Slow_D, D에서 강→Ability_E, 강 단독 Primary_B→C→Slow_D
L, H = "InputTag.LightAttack", "InputTag.HeavyAttack"
LINKS = {
    "Fast_A": [(L, "Fast_B")],
    "Fast_B": [(L, "Fast_C")],
    "Fast_C": [(L, "Fast_D"), (H, "Slow_D")],
    "Fast_D": [(H, "Ability_E")],
    "Primary_B": [(H, "Primary_C")],
    "Primary_C": [(H, "Slow_D")],
}
for a, links in LINKS.items():
    data[a].set_editor_property("combo_links", [link(t, data[b]) for t, b in links])

save_all(saved)
for name in montages:
    log(LIB.describe_montage(montages[name]).replace("\n", " | "))
log("done", len(saved))
