// ===========================================================================
// VA 0x004B4260 - direct frame seek / pose rebuild  (0x14F8 bytes)
// ===========================================================================
// __thiscall on the model block:
//
//   Sub4B4260(model, frame, physicsMode)
//     frame        unsigned frame number (NOT seconds - no x30 rounding)
//     physicsMode  0/1 = no rigid notifications, 2 = notify on mode byte
//                  changes, 3 = full IK-off arming bookkeeping
//
// The stateless sibling of 0x4A31D0 (model_keyframe_advance.cpp): instead
// of advancing per-track cursors it walks each track from its first record
// every call, which is what the 30 call sites want - timeline scrub, play
// start, key registration/deletion, model reload and the VMD load tail
// (0x434B60) all snap the pose to an arbitrary frame.
//
// Key structural facts (live disasm 0x4B4A79..0x4B4ABE):
//   * every walk starts at the track's FIRST record, and the first record
//     of bone b / morph k is record number b / k itself (var_40 = 60*b,
//     var_5C = b; the loader places each track's first key at its own
//     index; record 0 is the shared frame-0 sentinel whose prev chain the
//     walks never follow here);
//   * bones whose type byte (+484) is > 6 (signed setle, negative bytes
//     pass) and != 8 are skipped ENTIRELY (0x4B4A5F), unlike 0x4A31D0
//     which processes them but skips position interpolation;
//   * the terminal zero-fill sets quat w (+344) to 1.0 (fld1), unlike
//     0x4A31D0's terminal which leaves w untouched;
//   * the exact-hit arming and the mid-interpolation re-arm only run for
//     physicsMode == 3; physicsMode == 2 performs plain mode-byte notifies;
//   * the selector window uses an unsigned jae against a SHORT-CIRCUIT
//     carried reference (0x4B51AC `cmp arg_0, ecx; jnb`): when the start
//     window test fails, the end-window condition never evaluates and the
//     reference stays selStart; otherwise it becomes selEnd - semantics
//     identical to 0x4A31D0's two-window structure;
//   * the eased rotation fraction feeds the slerp weights exactly like
//     0x4A31D0 (v175 = sub_4A05A0(...,3,...) at 0x4B52D1), and position
//     easing passes the raw fraction per axis (0x4B5640/0x4B569D/0x4B570A);
//   * return value is the last loop counter (bone count when bones were
//     processed) - the original leaves it in eax and no caller uses it.
//
// Reference: IDA live disassembly of MikuMikuDance.exe v932 (sole source of
// truth; ../translated/ reference files deviate).
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstring>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/model.hpp"

#include "keyframe_common.hpp"

namespace mikudancestudio {
namespace {

using kfa::CopyIkDisplayFlags;
using kfa::RefreshIkSelectors;
using kfa::CopyBoneKeyVerbatim;
using kfa::WriteBonePrevVerbatim;
using kfa::MirrorBackupsToWorking;
using kfa::MirrorBackupsToCurrent;

// PI/2 as the original double constant at 0x530F48 (0x3FF921FB00000000).
double PiHalfBits() {
    const std::uint64_t bits = 0x3FF921FB00000000ULL;
    double d;
    std::memcpy(&d, &bits, sizeof d);
    return d;
}
const float kQuatClamp = 0.99999994f;  // 0x3F7FFFFF

}  // namespace

// ---- VA 0x004B4260 --------------------------------------------------------
int Sub4B4260(unsigned char* model, int frameArg, int physicsMode) {
    unsigned char* const m = model;
    const std::uint32_t a2 = static_cast<std::uint32_t>(frameArg);
    int result = 0;

    // ==== section 1: IK master track (0x4B426D) ===========================
    {
        mdl::DisplayKey* const keys = mdl::DisplayKeys(m);
        int found = 0;                                 // v5 / v157
        bool resolved = false;
        if (keys[0].frame < a2) {
            // walk the master chain (0x4B4281)
            for (;;) {
                const int next = static_cast<int>(keys[found].next);
                if (next == 0) break;
                found = next;
                if (keys[found].frame >= a2) {
                    resolved = true;
                    break;
                }
            }
            if (!resolved) {
                // chain exhausted past a2: terminal variant A (0x4B43A0)
                mikudancestudio::mdl::Mdl(m)->loadComplete = keys[found].visible;
                CopyIkDisplayFlags(m, keys[found]);
                RefreshIkSelectors(m, keys, found, found, false, 0, 0);
            }
        } else {
            resolved = true;  // a2 is at/before the first record
        }
        if (resolved) {
            if (keys[found].frame == a2) {
                // exact hit: variant B (0x4B42BF)
                mikudancestudio::mdl::Mdl(m)->loadComplete = keys[found].visible;
                CopyIkDisplayFlags(m, keys[found]);
                RefreshIkSelectors(m, keys, found, found, true, found, found);
            } else {
                // between keys: variant C (0x4B46BA) - flags/pairs from the
                // PREVIOUS record, forward condition against the found one
                const int prevIdx = static_cast<int>(keys[found].previous);
                mikudancestudio::mdl::Mdl(m)->loadComplete = keys[prevIdx].visible;
                CopyIkDisplayFlags(m, keys[prevIdx]);
                RefreshIkSelectors(m, keys, prevIdx, prevIdx, true, found,
                                   found);
            }
        }
    }

    // ==== section 2: morph tracks (0x4B4917) ==============================
    const int morphCnt = static_cast<int>(mdl::Mdl(m)->morphCount);
    if (morphCnt > 0) {
        mdl::MorphKey* const keys = mdl::MorphKeys(m);
        mikudancestudio::mdl::MorphRecord* const vals = mikudancestudio::mdl::Morphs(m);  // 136-byte structs
        for (int k = 0; k < morphCnt; ++k) {
            int found = k;              // morph k's first record is #k
            if (keys[found].frame < a2) {
                bool resolved = false;
                for (;;) {
                    const int next = static_cast<int>(keys[found].next);
                    if (next == 0) break;
                    found = next;
                    if (keys[found].frame >= a2) {
                        resolved = true;
                        break;
                    }
                }
                if (!resolved) {
                    // chain exhausted: copy the final value (0x4B49AA)
                    vals[k].value = keys[found].value;
                    result = k + 1;
                    continue;
                }
            }
            const mdl::MorphKey& rec = keys[found];
            float v;
            if (rec.frame == a2) {
                v = rec.value;
            } else {
                const mdl::MorphKey& prev = keys[rec.previous];
                const float tF = (float)((double)(a2 - prev.frame) /
                                         (double)(std::uint32_t)(
                                             rec.frame - prev.frame));
                const float delta =
                    rec.value - prev.value;
                v = (float)((double)tF * (double)delta +
                            (double)prev.value);
            }
            vals[k].value = v;
            result = k + 1;
        }
    }

    // ==== section 3: bone tracks (0x4B4A42) ===============================
    const int boneCnt = static_cast<int>(mdl::Mdl(m)->boneCount);
    if (boneCnt > 0) {
        mdl::BoneKey* const keys = mdl::BoneKeys(m);
        mikudancestudio::mdl::BoneRecord* const bones = mdl::Bones(m);
        for (int b = 0; b < boneCnt; ++b) {
            mikudancestudio::mdl::BoneRecord* const bone = &bones[b];
            // type gate (0x4B4A5F): signed setle <= 6 or == 8
            const std::int8_t btype = static_cast<std::int8_t>(bone->type);
            if (!(btype <= 6 || bone->type == 8)) continue;

            int found = b;              // bone b's first record is #b
            bool resolved = false;
            if (keys[found].frame < a2) {
                for (;;) {
                    const int next = static_cast<int>(keys[found].next);
                    if (next == 0) break;
                    found = next;
                    if (keys[found].frame >= a2) {
                        resolved = true;
                        break;
                    }
                }
                if (!resolved) {
                    // terminal key (0x4B4C8E): copy verbatim
                    const mdl::BoneKey& rec = keys[found];
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat[c] = rec.rotation[c];
                    for (int c = 0; c < 3; ++c)
                        bone->trans[c] = rec.position[c];
                    const unsigned char mode = rec.physicsDisabled;
                    if (bone->f492 != 0 && bone->f493 != mode &&
                        physicsMode >= 2) {
                        Sub499B50(m, b, mode);  // 0x4B4D23
                        bone->f493 = rec.physicsDisabled;
                    }
                    const int selIdx = bone->slotIndex;
                    if (selIdx >= 0) {
                        const mdl::BoneOrderEntry& window =
                            mdl::BoneOrder(m)[selIdx];
                        const std::uint32_t selStart = window.windowStart;
                        if (rec.frame < selStart && selStart <= a2) {
                            // zero position + quaternion xyz, w = 1.0
                            // (0x4B4D79..0x4B4DCB)
                            for (int o = 0; o < 6; ++o)
                                bone->trans[o] = 0.0f;
                            bone->rotQuat[3] = 1.0f;
                        }
                    }
                    result = b + 1;
                    continue;
                }
            } else {
                resolved = true;
            }

            const mdl::BoneKey& rec = keys[found];
            const std::uint32_t curFrame = rec.frame;

            if (curFrame == a2) {
                // exact hit (0x4B4AD8): copy verbatim
                CopyBoneKeyVerbatim(bone, rec);
                if (bone->f492 != 0) {  // 0x4B4B37
                    if (physicsMode != 3) {
                        const unsigned char mode = rec.physicsDisabled;
                        if (bone->f493 != mode && physicsMode == 2) {
                            Sub499B50(m, b, mode);  // 0x4B4E67
                            bone->f493 = rec.physicsDisabled;
                        }
                        result = b + 1;
                        continue;
                    }
                    const int nextIdx = static_cast<int>(rec.next);
                    const unsigned char mode = rec.physicsDisabled;
                    if (nextIdx <= 0) {
                        if (bone->f493 != mode)
                            Sub499B50(m, b, mode);  // 0x4B4E1D
                        bone->f493 = rec.physicsDisabled;
                        result = b + 1;
                        continue;
                    }
                    const mdl::BoneKey& nrec = keys[nextIdx];
                    if (nrec.physicsDisabled != 1 || mode != 0) {
                        if (bone->f493 != mode)
                            Sub499B50(m, b, mode);  // 0x4B4DE7
                        bone->f493 = rec.physicsDisabled;
                        result = b + 1;
                        continue;
                    }
                    // arm the interpolation for the upcoming segment
                    // (0x4B4B91): working copies AND current pose from the
                    // backups
                    if (bone->f493 == 0) Sub499B50(m, b, 1);
                    bone->f493 = 1;
                    bone->rigidIdx = static_cast<std::int32_t>(curFrame);
                    MirrorBackupsToWorking(bone);
                    MirrorBackupsToCurrent(bone);
                }
                result = b + 1;
                continue;
            }

            // between keys: previous-key blend inputs (0x4B4E8F)
            const int prevIdx = static_cast<int>(rec.previous);
            const mdl::BoneKey& prevRec = keys[prevIdx];
            std::uint32_t prevFrame = prevRec.frame;
            float ppos[3] = {prevRec.position[0], prevRec.position[1],
                             prevRec.position[2]};
            float pq[4] = {prevRec.rotation[0], prevRec.rotation[1],
                           prevRec.rotation[2], prevRec.rotation[3]};

            if (bone->f492 != 0) {  // 0x4B4E88
                if (physicsMode == 3) {
                    if (rec.physicsDisabled == 1 &&
                        prevRec.physicsDisabled == 0) {
                        // mid-interpolation segment (0x4B4EBF)
                        const std::uint32_t start =
                            static_cast<std::uint32_t>(bone->rigidIdx);
                        if (start >= curFrame || start < prevFrame) {
                            // stale arm -> re-arm from the backups (0x4B4EDA)
                            if (bone->f493 == 0) Sub499B50(m, b, 1);
                            bone->f493 = 1;
                            bone->rigidIdx = static_cast<std::int32_t>(prevFrame);
                            MirrorBackupsToWorking(bone);
                        }
                        if (bone->f493 == 0) Sub499B50(m, b, 1);
                        bone->f493 = 1;
                        // blend from the armed working copies (0x4B4FC4)
                        pq[0] = bone->ikWorkingQuat[0];
                        pq[1] = bone->ikWorkingQuat[1];
                        pq[2] = bone->ikWorkingQuat[2];
                        pq[3] = bone->ikWorkingQuat[3];
                        ppos[0] = bone->ikWorkingPos[0];
                        ppos[1] = bone->ikWorkingPos[1];
                        ppos[2] = bone->ikWorkingPos[2];
                        prevFrame = static_cast<std::uint32_t>(
                            bone->rigidIdx);
                    } else {
                        const unsigned char pmode = prevRec.physicsDisabled;
                        if (bone->f493 != pmode)
                            Sub499B50(m, b, pmode);  // 0x4B5022
                        bone->f493 = prevRec.physicsDisabled;
                    }
                } else {
                    const unsigned char pmode = prevRec.physicsDisabled;
                    if (bone->f493 != pmode && physicsMode == 2) {
                        Sub499B50(m, b, pmode);  // 0x4B50B0
                        bone->f493 = prevRec.physicsDisabled;
                    }
                }
            }

            // selector window (0x4B5190): the snap reference is carried by
            // the short-circuit - selStart when the start-window test fails,
            // selEnd when both conditions evaluate (disasm 0x4B51AC jnb)
            bool doSnap = false;
            std::uint32_t snapRef = 0;
            const int selIdx = bone->slotIndex;
            if (selIdx >= 0) {
                const mdl::BoneOrderEntry& window =
                    mdl::BoneOrder(m)[selIdx];
                const std::uint32_t selStart = window.windowStart;
                const std::uint32_t selEnd = window.windowEnd;
                if (selStart < curFrame && prevFrame < selStart) {
                    doSnap = true;
                    snapRef = selStart;
                } else if (!(curFrame < selEnd || prevFrame >= selEnd)) {
                    doSnap = true;
                    snapRef = selEnd;
                }
            }
            if (doSnap) {
                if (a2 >= snapRef)
                    CopyBoneKeyVerbatim(bone, rec);  // 0x4B51FC
                else
                    WriteBonePrevVerbatim(bone, pq, ppos);  // 0x4B51BA
                result = b + 1;
                continue;
            }

            // full interpolation (0x4B5284): eased rotation fraction drives
            // the slerp, raw fraction eases each position axis
            const float cq[4] = {rec.rotation[0], rec.rotation[1],
                                 rec.rotation[2], rec.rotation[3]};
            const float tF = (float)(
                (double)(a2 - prevFrame) /
                (double)(std::uint32_t)((std::int32_t)curFrame -
                                        (std::int32_t)prevFrame));
            const float eRot = Sub4A05A0(m, 3, found, tF);

            const double dotD = (double)pq[0] * cq[0] +
                                (double)pq[1] * cq[1] +
                                (double)pq[2] * cq[2] +
                                (double)pq[3] * cq[3];
            const float dot = (float)dotD;
            if (static_cast<float>(1.0 - (double)dot * (double)dot) ==
                0.0f) {
                // parallel quaternions: previous key verbatim (0x4B534A)
                for (int c = 0; c < 4; ++c) bone->rotQuat[c] = pq[c];
            } else {
                float dc = dot;
                if (dc > 1.0f)
                    dc = kQuatClamp;
                else if (dc < -1.0f)
                    dc = -kQuatClamp;
                const float th = (float)std::acos((double)dc);
                if ((double)th > PiHalfBits() && (double)dc < 0.0) {
                    // long way around (0x4B53F7)
                    const float th2 = (float)std::acos(-(double)dc);
                    const float s = (float)std::sin((double)th2);
                    const float a0 =
                        (float)((1.0 - (double)eRot) * (double)th2);
                    const float sin0 =
                        (float)std::sin((double)a0);
                    const float w0 =
                        (float)((double)sin0 / (double)s);
                    const float a1 = (float)((double)eRot * (double)th2);
                    const float sin1 =
                        (float)std::sin((double)a1);
                    const float w1 =
                        (float)((double)sin1 / (double)s);
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat[c] =
                            (float)((double)pq[c] * (double)w0 -
                                    (double)w1 * (double)cq[c]);
                } else {
                    const float s = (float)std::sin((double)th);
                    const float a0 =
                        (float)((1.0 - (double)eRot) * (double)th);
                    const float sin0 =
                        (float)std::sin((double)a0);
                    const float w0 =
                        (float)((double)sin0 / (double)s);
                    const float a1 = (float)((double)eRot * (double)th);
                    const float sin1 =
                        (float)std::sin((double)a1);
                    const float w1 =
                        (float)((double)sin1 / (double)s);
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat[c] =
                            (float)((double)pq[c] * (double)w0 +
                                    (double)w1 * (double)cq[c]);
                }
            }

            // position easing (0x4B55E1) - same type gate again (always
            // true here because of the bone-level gate, kept for fidelity)
            if (btype <= 6 || bone->type == 8) {
                for (int axis = 0; axis < 3; ++axis) {
                    const float delta = rec.position[axis] - ppos[axis];
                    if (delta == 0.0f) {
                        bone->trans[axis] = ppos[axis];
                    } else {
                        const float e = Sub4A05A0(m, axis, found, tF);
                        bone->trans[axis] =
                            (float)((double)e * (double)delta +
                                    (double)ppos[axis]);
                    }
                }
            }
            result = b + 1;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// VA 0x004220C0 - Sub4220C0(app): seek the selected model to the current
// frame and set the re-eval byte.  Model slot = app+0x780[byte 0x910],
// frame = app+0x980, third arg app+0xA0CC4 (same convention as the 30
// other Sub4B4260 call sites), then byte 0x9ED95 = 1.
// ---------------------------------------------------------------------------
void Sub4220C0(MMDApp* app) {
    unsigned char* model = app->SelectedModel();
    Sub4B4260(model, app->state.currentFrame,
              app->PlaybackPhysicsMode());                         // 0x4220DF
    app->F9ed94Byte1() = 1;                                        // 0x4220E4
}

}  // namespace mikudancestudio
