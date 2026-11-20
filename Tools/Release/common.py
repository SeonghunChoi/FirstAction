# 배포판 에셋 생성 공통 함수 (9-3, CLAUDE.md 2.4). 에디터 안에서 실행된다:
#   cat Tools/Release/common.py Tools/Release/<캐릭터>.py > tmp.py && python Tools/ue_exec.py tmp.py
# 경로는 모두 /Game/ThirdParty/Release/ 아래(git 제외)만 만든다

import unreal

EAL = unreal.EditorAssetLibrary
ANIM = unreal.AnimationLibrary
LIB = unreal.FirstActionEditorLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

WARP_TARGET = "AttackTarget"   # CLAUDE.md 5.1
ANIM_SLOT = "UpperBody"        # Paragon 애님 BP의 슬롯 이름


def log(*args):
    print(*args)


def tag(name):
    t = unreal.GameplayTag()
    t.import_text('(TagName="%s")' % name)
    return t


def alpha_blend(seconds):
    b = unreal.AlphaBlend()
    b.import_text("(BlendTime=%f)" % seconds)
    return b


def ensure_dir(path):
    if not EAL.does_directory_exist(path):
        EAL.make_directory(path)


def load_or_create(path, asset_class, factory):
    """있으면 그 에셋을 그대로 쓰고(다른 에셋의 참조가 끊기지 않게), 없으면 만든다.
    지우고 다시 만들면 그 에셋을 가리키던 참조(BP 부여 목록 등)가 비어 버린다"""
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    folder, name = path.rsplit("/", 1)
    ensure_dir(folder)
    return TOOLS.create_asset(name, folder, asset_class, factory)


def make_montage(path, anims, rates=None, sections=None, blend_in=0.1, blend_out=0.15, slot=ANIM_SLOT):
    """anims: 시퀀스 경로 목록(이어 붙임). sections: [(이름, 시작 시간, 다음 이름)]"""
    seqs = [unreal.load_asset(a) for a in anims]
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", seqs[0])
    m = load_or_create(path, unreal.AnimMontage, factory)
    ANIM.remove_all_animation_notify_tracks(m)   # 다시 돌릴 때 노티파이를 처음부터
    LIB.set_montage_slot(m, slot)
    LIB.set_montage_segments(m, seqs, rates or [])
    if sections:
        LIB.set_montage_sections(m, [s[0] for s in sections], [s[1] for s in sections], [s[2] or "None" for s in sections])
    m.set_editor_property("blend_in", alpha_blend(blend_in))
    m.set_editor_property("blend_out", alpha_blend(blend_out))
    return m


def _track(m, name):
    if not ANIM.is_valid_anim_notify_track_name(m, name):
        ANIM.add_animation_notify_track(m, name)
    return name


def add_window(m, start, end, tag_name, track="Window"):
    ns = ANIM.add_animation_notify_state_event(m, _track(m, track), start, end - start, unreal.ANS_TagWindow)
    ns.set_editor_property("tag", tag(tag_name))
    return ns


def add_hit(m, start, end):
    return ANIM.add_animation_notify_state_event(m, _track(m, "Hit"), start, end - start, unreal.ANS_HitWindow)


def add_warp(m, start, end, translation=True):
    """공격 접근 구간. 제자리 동작(루트 모션 없음)은 GA_MeleeAttack이 이 구간 동안 Root Motion Source로 옮긴다 (9-3)"""
    ns = ANIM.add_animation_notify_state_event(m, _track(m, "MotionWarping"), start, end - start, unreal.AnimNotifyState_MotionWarping)
    mod = unreal.new_object(unreal.RootMotionModifier_SkewWarp, outer=ns)
    mod.set_editor_property("warp_target_name", WARP_TARGET)
    mod.set_editor_property("warp_translation", translation)
    mod.set_editor_property("warp_rotation", True)
    ns.set_editor_property("root_motion_modifier", mod)
    return ns


def make_data(path, template, values, montage=None):
    """template: 깃 데이터 에셋을 복제해 값만 바꾼다 (칸 구조를 그대로 물려받는다)"""
    if EAL.does_asset_exist(path):
        d = unreal.load_asset(path)
    else:
        ensure_dir(path.rsplit("/", 1)[0])
        d = EAL.duplicate_asset(template, path)
    for k, v in values.items():
        d.set_editor_property(k, v)
    if montage is not None:
        d.set_editor_property("montage", montage)
    return d


def link(input_tag, next_attack):
    # 구조체 칸이 EditDefaultsOnly라 set_editor_property로는 못 바꾼다. 텍스트로 만든다
    l = unreal.AttackComboLink()
    l.import_text('(InputTag=(TagName="%s"),NextAttack="%s")' % (input_tag, next_attack.get_path_name()))
    return l


def save_all(paths):
    for p in paths:
        EAL.save_asset(p, only_if_is_dirty=False)
