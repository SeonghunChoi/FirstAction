# 공격 클립의 판정 구간 후보를 잰다 (9-3). common.py 뒤에 붙여 실행하고, 맨 위에서 MEASURE = [(메시 경로, [시퀀스 경로...])]를 정한다.
# 칼날 소켓(blade_start/blade_end) 위의 5점을 루트 기준 위치로 구해, 칼끝 속도와 정면 공간(Paragon 정면 +Y: 앞 40~170, 높이 20~180, 좌우 ±90cm)
# 통과를 프레임마다 본다. 둘 다 만족하는 시간대를 판정 구간 후보로 출력한다
A = unreal.AnimationLibrary
STEP = 1.0 / 60.0
SPEED_MIN = 400.0


def _chain(mesh, bone):
    sm = unreal.SkeletonModifier()
    sm.set_skeletal_mesh(mesh)
    ch = [bone]
    while True:
        p = str(sm.get_parent_name(ch[0]))
        if p in ("None", ""):
            return ch
        ch.insert(0, p)


def _world(seq, chain, t, local):
    x = unreal.Transform(location=local)
    for p in reversed(A.get_bone_poses_for_time(seq, chain, t, False)):
        x = unreal.MathLibrary.compose_transforms(x, p)
    return x.translation


def measure(mesh_path, seq_path):
    mesh = unreal.load_asset(mesh_path)
    seq = unreal.load_asset(seq_path)
    s0, s1 = mesh.find_socket("blade_start"), mesh.find_socket("blade_end")
    c0, c1 = _chain(mesh, str(s0.bone_name)), _chain(mesh, str(s1.bone_name))
    L = seq.get_play_length()
    rows, prev, t = [], None, 0.0
    while t <= L + 1e-4:
        a, b = _world(seq, c0, t, s0.relative_location), _world(seq, c1, t, s1.relative_location)
        pts = [a + (b - a) * (k / 4.0) for k in range(5)]
        front = any(40 <= p.y <= 170 and 20 <= p.z <= 180 and abs(p.x) <= 90 for p in pts)
        speed = 0.0 if prev is None else (b - prev).length() / STEP
        rows.append((t, speed, front, b))
        prev, t = b, t + STEP
    wins, cur = [], None
    for t, sp, fr, _ in rows:
        on = fr and sp >= SPEED_MIN
        if on and cur is None:
            cur = [t, t]
        elif on:
            cur[1] = t
        elif cur:
            wins.append(tuple(round(v, 2) for v in cur)); cur = None
    if cur:
        wins.append(tuple(round(v, 2) for v in cur))
    if seq_path.rsplit("/", 1)[1] in globals().get("VERBOSE", []):
        for i in range(0, len(rows), 2):
            t, sp, fr, b = rows[i]
            log("SV %.2f %6.0f %s tip(%d,%d,%d)" % (t, sp, "F" if fr else ".", b.x, b.y, b.z))
    peak = max(rows, key=lambda r: r[1])
    log("SW %-40s len %.2f peak %.0f@%.2f hits %s" % (seq_path.rsplit("/", 1)[1], L, peak[1], peak[0], wins))


for mesh_path, seqs in MEASURE:
    for s in seqs:
        measure(mesh_path, s)
