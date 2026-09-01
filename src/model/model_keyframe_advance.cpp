// ===========================================================================
// VA 0x004A31D0 - per-model keyframe application  (5754 bytes, 187 blocks)
// + helpers VA 0x004A05A0 (VMD interpolation easing) and 0x00499B50
// (bone physics-mode notify).
// ===========================================================================
// __thiscall on the model block, called once per model per playback tick by
// PlaybackPoseAdvance (0x4175A0, timeline_advance.cpp):
//
//   Sub4A31D0(model, cursor, physicsMode)
//     cursor       float seconds (app 0x9E64C physics cursor)
//     physicsMode  int app 0xA0CC4 (>=2 enables the IK-off bookkeeping and
//                  the rigid-body notifications)
//
// Three sections (original addresses):
//   1  0x4A3236  IK master tracks.  28-byte records at model+9960
//      {+0 frame(u32), +4 prevIdx, +8 nextIdx, +12 display byte ->
//       model+11661, +16 ptr per-IK display-flag bytes -> 24B structs at
//       model+9920 (+18 each), +24 ptr per-IK 8-byte {frame, selIdx} pairs},
//      cursor model+52, gate model+56, count model+11656 (IK chains).
//      Advance while frame < cursor; parking/refresh come in three variants:
//        A  terminal (0x4A3291)  pairs from the cursor record, backward walk
//           for sel+4, sel+8 = 0;
//        B  exact hit (0x4A3477) same, plus a forward walk for sel+8
//           (condition against the cursor record's pair - tautology);
//        C  between keys (0x4A36DF) pairs/flags from the PREVIOUS record,
//           forward condition against the cursor record's pair.
//      The 20-byte selector records at model+314596 (count model+314600,
//      referenced by &bone->slotIndex) hold the IK-off window {+4 start frame,
//      +8 end frame, +12/+16 the {frame, idx} pair} consumed by section 3.
//   2  0x4A391B  morph tracks.  20-byte key records at model+9956
//      {+0 frame, +4 prevIdx, +8 nextIdx, +12 float value}, per-morph cursors
//      int[] at model+44, active bytes at model+48, count model+11648;
//      current value stored to 136-byte structs at model+9924 (+48).
//      Linear interpolation only (VMD morphs have no curve).
//   3  0x4A3AA5  bone tracks.  60-byte key records at model+9952
//      {+0 frame, +4 prevIdx, +8 nextIdx, +12..27 bezier control bytes
//       x1[4] y1[4] x2[4] y2[4] (one byte per channel 0..3, channel-stride
//       1 inside each group), +28..38 position xyz, +40..55 quaternion
//       xyzw, +57 IK-off mode byte}, per-bone cursors int[] at model+36,
//      active bytes at model+40, count model+11652.  Bone structs (604
//       bytes, model+9916): +320 position, +332 quaternion, +392 position
//       backup, +404 quaternion backup, +420 armed start frame, +424/+436
//       working position/quaternion copies, +484 type byte, +492 notify
//       gate, +493 current mode, +600 selector index (-1 = none).
//
//      Quaternion blend is a hand-rolled slerp (0x4A43EF): dot clamp to
//      +-0.99999994f (0x3F7FFFFF), angle = acos (double), and when the angle
//      exceeds the original's pi/2 constant (double 0x3FF921FB00000000 -
//      pi/2 truncated to a 24-bit mantissa, NOT the exact double) while
//      dot < 0 the complement angle acos(-dot) with SUBTRACTED weights is
//      used; otherwise normal added weights.  sin/acos run in double on
//      float-rounded intermediates.
//
//      DECOMPILER TRAP (same class as the 0x41789E one, opposite direction):
//      Hex-Rays shows the slerp weights using the RAW fraction; live disasm
//      0x4A44F8 `fsub var_3C` / 0x4A4521 `fmul var_3C` proves the EASED
//      rotation fraction (sub_4A05A0 channel 3, stored float var_3C at
//      0x4A43A5) feeds both sin((1-t)*theta) and sin(t*theta).  The position
//      easing (0x4A4718/0x4A4778/0x4A47EE) passes the RAW fraction in and
//      uses the callee's return value directly (never stored, so no extra
//      rounding by the caller).
//
//      Mid-interpolation bookkeeping (physicsMode >= 2, &bone->f492): when the
//      current key's +57 byte is 1 and the previous key's is 0, the blend
//      runs from the ARMED working copies (+424/+436) with prevFrame taken
//      from &bone->rigidIdx instead of the previous record.  The arm is refreshed
//      (0x4A3FF1, unsigned: start < prevFrame || start >= curFrame) from
//      the backups (+392/+404); the exact-hit arm (0x4A3E54) additionally
//      copies the backups over the current pose (+320/+332).
//
//      The selector window (&bone->slotIndex -> 20B record) overrides blending
//      inside [selStart, selEnd): comparisons unsigned (0x4A41E9 sbb/ neg
//      idiom); inside the window the bone snaps to prev (selStart > cursor)
//      or cur (selStart <= cursor / selEnd <= cursor) verbatim.
//
// 0x004A05A0  VMD easing, __thiscall(model, channel, keyIdx, t).  Control
//      bytes at record+channel+{12,16,20,24} (x1,y1,x2,y2, signed), scaled
//      by the double at 0x531108 (0.02362200058996677 - float 0.023622
//      promoted, nominally 3/127).  X(u) = 3(1-u)^2 u x1 + 3(1-u) u^2 x2 +
//      u^3 is inverted by 12 halving bisection steps from u = 0.5 (step
//      halves before each update; exact double-equality early exit; u kept
//      as float); returns Y(u) rounded to float.  Linear early-out
//      x1 == y1 && x2 == y2 returns t unchanged.  Same shape as the camera
//      easing 0x410140 (timeline_advance.cpp) but with the 3x folded into
//      the scale constant and channel-interleaved control bytes.
//
// 0x00499B50  bone physics-mode notify, __thiscall(model, boneIdx, mode):
//      walks the 172-byte rigid-body records at model+12744 (count
//      model+12752); record +28 = bone index, +82 = skip gate, +104 =
//      object pointer.  mode != 0 makes the body kinematic and record+80 =
//      0; mode == 0 restores record+80 to (record+81 ? 2 : 1).  The original
//      writes the old Bullet collision-flag member directly; the port uses
//      Bullet's ABI-stable public collision-flag API for that same operation.
//      Also called from 0x4A2CD0/0x4B2210/0x4B38A0/0x4B4260 (future phases).
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

#include "btBulletDynamicsCommon.h"

#include "mikudancestudio/model.hpp"
#include "mikudancestudio/ported_funcs.hpp"

#include "keyframe_common.hpp"

namespace mikudancestudio {
namespace {

using kfa::CopyIkDisplayFlags;
using kfa::RefreshIkSelectors;
using kfa::CopyBoneKeyVerbatim;
using kfa::WriteBonePrevVerbatim;
using kfa::MirrorBackupsToWorking;
using kfa::MirrorBackupsToCurrent;

double PiHalfBits() {
    const std::uint64_t bits = 0x3FF921FB00000000ULL;
    double d;
    std::memcpy(&d, &bits, sizeof d);
    return d;
}

const float kQuatClamp = 0.99999994f;

const double kCpScale = 0.02362200058996677;

}  // namespace

// ---- VA 0x004A05A0 --------------------------------------------------------
float Sub4A05A0(unsigned char* model, int channel, int keyIdx, float t) {
    const mdl::BoneKey& rec = mdl::BoneKeys(model)[keyIdx];
    const unsigned char x1 = rec.interpolation[channel];
    const unsigned char y1 = rec.interpolation[channel + 4];
    const unsigned char x2 = rec.interpolation[channel + 8];
    const unsigned char y2 = rec.interpolation[channel + 12];
    if (x2 == y2 && x1 == y1) return t;  // linear curve (0x4A05DE)

    const double x1c = static_cast<double>(
                           static_cast<signed char>(x1)) * kCpScale;
    const double x2c = static_cast<double>(
                           static_cast<signed char>(x2)) * kCpScale;

    // 12 halving bisection steps on X(u) = t; u/step kept as float exactly
    // like the original stack slots (the step halves BEFORE the update).
    float u = 0.5f, step = 0.5f;
    for (int i = 0; i < 12; ++i) {
        const float om = 1.0f - u;
        const double xu = (double)om * (double)om * (double)u * x1c +
                          (double)om * (double)u * (double)u * x2c +
                          (double)u * (double)u * (double)u;
        if (xu == (double)t) break;
        step *= 0.5f;
        u = xu >= (double)t ? (float)(u - step) : (float)(u + step);
    }

    const float om = 1.0f - u;
    const double y1c = static_cast<double>(
                           static_cast<signed char>(y1)) * kCpScale;
    const double y2c = static_cast<double>(
                           static_cast<signed char>(y2)) * kCpScale;
    return static_cast<float>(
        (double)u * ((double)u * (double)u) +
        (double)om * (double)om * (double)u * y1c +
        (double)om * (double)u * (double)u * y2c);
}

// ---- VA 0x00499B50 --------------------------------------------------------
void Sub499B50(unsigned char* model, int boneIdx, unsigned char mode) {
    mikudancestudio::mdl::RigidRecord* const rigids = mikudancestudio::mdl::Rigids(model);
    const int cnt = static_cast<int>(mdl::Mdl(model)->rigidCount);
    for (int i = 0; i < cnt; ++i) {
        mikudancestudio::mdl::RigidRecord* const r = &rigids[i];
        if (r->staticFlag != 0 || r->boneIndex != boneIdx) continue;
        btRigidBody* const body = static_cast<btRigidBody*>(r->body);
        if (body == nullptr)
            continue;

        const int flags = body->getCollisionFlags();
        if (mode != 0) {
            body->setCollisionFlags(
                flags | btCollisionObject::CF_KINEMATIC_OBJECT);
            r->mode = 0;
        } else {
            body->setCollisionFlags(
                flags & ~btCollisionObject::CF_KINEMATIC_OBJECT);
            r->mode = r->kinematicFlag != 0 ? 2 : 1;
        }
        // Physics creation uses this activation state too.  Retaining it
        // across a temporary kinematic transition prevents a body from
        // silently sleeping after its authored mode is restored.
        body->setActivationState(DISABLE_DEACTIVATION);
    }
}

// ---- VA 0x004A2CD0 --------------------------------------------------------
// Initializes every per-model animation cursor at playback start.  Unlike
// Sub4B4260 this preserves the incremental-track state consumed by 0x4A31D0.
void Sub4A2CD0(unsigned char* model, float cursor, int physicsMode) {
    unsigned char* const m = model;
    const double frame = static_cast<double>(cursor) * 30.0;

    // IK/display master track.  This is a single linked list rooted at 0.
    mdl::DisplayKey* const ikKeys = mdl::DisplayKeys(m);
    mdl::ModelRecord& state = *mdl::Mdl(m);
    state.displayTrackActive = 1;
    state.displayKeyCursor = 0;
    if (static_cast<double>(ikKeys[0].frame) < frame) {
        for (;;) {
            const int cur = static_cast<int>(state.displayKeyCursor);
            const int next = static_cast<int>(ikKeys[cur].next);
            if (next == 0) {
                state.displayTrackActive = 0;
                const mdl::DisplayKey& rec = ikKeys[cur];
                mikudancestudio::mdl::Mdl(m)->loadComplete = rec.visible;
                kfa::CopyIkDisplayFlags(m, rec);
                kfa::RefreshIkSelectors(m, ikKeys, cur, cur, false, 0, 0);
                break;
            }
            state.displayKeyCursor = static_cast<std::uint32_t>(next);
            if (static_cast<double>(ikKeys[next].frame) >= frame)
                break;
        }
    }

    // Morph tracks are independently rooted at their morph index.
    const int morphCount = static_cast<int>(mdl::Mdl(m)->morphCount);
    mdl::MorphKey* const morphKeys = mdl::MorphKeys(m);
    mikudancestudio::mdl::MorphRecord* const morphValues = mikudancestudio::mdl::Morphs(m);
    unsigned char* const morphActive = state.morphTrackActive;
    std::uint32_t* const morphCursor = state.morphKeyCursors;
    for (int morph = 0; morph < morphCount; ++morph) {
        morphActive[morph] = 1;
        morphCursor[morph] = static_cast<std::uint32_t>(morph);
        for (;;) {
            const int cur = static_cast<int>(morphCursor[morph]);
            if (static_cast<double>(morphKeys[cur].frame) >= frame)
                break;
            const int next = static_cast<int>(morphKeys[cur].next);
            if (next != 0) {
                morphCursor[morph] = static_cast<std::uint32_t>(next);
                continue;
            }
            morphActive[morph] = 0;
            morphValues[morph].value = morphKeys[cur].value;
            break;
        }
    }

    // Bone tracks are independently rooted at their bone index.  The start
    // initializer excludes only type 7, exactly as 0x4A301F does.
    const int boneCount = static_cast<int>(mdl::Mdl(m)->boneCount);
    mdl::BoneKey* const boneKeys = mdl::BoneKeys(m);
    mikudancestudio::mdl::BoneRecord* const bones = mdl::Bones(m);
    unsigned char* const boneActive = state.boneTrackActive;
    std::uint32_t* const boneCursor = state.boneKeyCursors;
    for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex) {
        mikudancestudio::mdl::BoneRecord* const bone = &bones[boneIndex];
        boneActive[boneIndex] = 1;
        if (bone->type == 7) {
            boneActive[boneIndex] = 0;
            continue;
        }
        boneCursor[boneIndex] = static_cast<std::uint32_t>(boneIndex);
        for (;;) {
            const int cur = static_cast<int>(boneCursor[boneIndex]);
            const mdl::BoneKey& rec = boneKeys[cur];
            if (static_cast<double>(rec.frame) >= frame)
                break;
            const int next = static_cast<int>(rec.next);
            if (next != 0) {
                boneCursor[boneIndex] = static_cast<std::uint32_t>(next);
                continue;
            }
            boneActive[boneIndex] = 0;
            std::memcpy(bone->rotQuat, rec.rotation, sizeof rec.rotation);
            std::memcpy(bone->trans, rec.position, sizeof rec.position);
            break;
        }

        const int cur = static_cast<int>(boneCursor[boneIndex]);
        const unsigned char mode = boneKeys[cur].physicsDisabled;
        if (bone->f492 != 0) {
            if (physicsMode >= 2)
                Sub499B50(m, boneIndex, mode);
            bone->f493 = mode;
        }
    }
}

// ---- VA 0x004A31D0 --------------------------------------------------------
void Sub4A31D0(unsigned char* model, float cursor, int physicsMode) {
    unsigned char* const m = model;
    mdl::ModelRecord& state = *mdl::Mdl(m);

    // cursor(seconds) -> frame, 1/1000 round-up (0x4A31E1; same shape as
    // PlaybackPoseAdvance's rounding tail)
    double frame = static_cast<double>(cursor) * 30.0;
    {
        const double scaled = frame * 1000.0;
        if (scaled - static_cast<double>(static_cast<int>(scaled)) >= 0.5)
            frame = static_cast<double>(static_cast<int>(scaled) + 1) / 1000.0;
    }

    // ==== section 1: IK master tracks (0x4A3236) ==========================
    if (state.displayTrackActive != 0) {
        mdl::DisplayKey* const keys = mdl::DisplayKeys(m);
        while ((double)keys[state.displayKeyCursor].frame < frame &&
               state.displayTrackActive != 0) {
            const int cur = static_cast<int>(state.displayKeyCursor);
            const mdl::DisplayKey& rec = keys[cur];
            const int next = static_cast<int>(rec.next);
            if (next != 0) {
                state.displayKeyCursor = static_cast<std::uint32_t>(next);
                continue;
            }
            // park: track exhausted (variant A, 0x4A328B)
            state.displayTrackActive = 0;
            mikudancestudio::mdl::Mdl(m)->loadComplete = rec.visible;
            CopyIkDisplayFlags(m, rec);
            RefreshIkSelectors(m, keys, cur, cur, false, 0, 0);
        }
        if (state.displayTrackActive != 0) {
            const int cur = static_cast<int>(state.displayKeyCursor);
            const mdl::DisplayKey& rec = keys[cur];
            if (frame == (double)rec.frame) {
                // exact hit (variant B, 0x4A3477)
                mikudancestudio::mdl::Mdl(m)->loadComplete = rec.visible;
                CopyIkDisplayFlags(m, rec);
                RefreshIkSelectors(m, keys, cur, cur, true, cur, cur);
            } else {
                // between keys (variant C, 0x4A36DF): flags/pairs come from
                // the PREVIOUS record; forward condition uses the cursor
                // record's pair
                const int prevIdx = static_cast<int>(rec.previous);
                const mdl::DisplayKey& prec = keys[prevIdx];
                mikudancestudio::mdl::Mdl(m)->loadComplete = prec.visible;
                CopyIkDisplayFlags(m, prec);
                RefreshIkSelectors(m, keys, prevIdx, prevIdx, true, cur, cur);
            }
        }
    }

    // ==== section 2: morph tracks (0x4A391B) ==============================
    const int morphCnt = static_cast<int>(mdl::Mdl(m)->morphCount);
    if (morphCnt > 0) {
        mdl::MorphKey* const keys = mdl::MorphKeys(m);
        mikudancestudio::mdl::MorphRecord* const vals = mikudancestudio::mdl::Morphs(m);  // 136-byte structs
        unsigned char* const active = state.morphTrackActive;
        std::uint32_t* const cursors = state.morphKeyCursors;
        for (int k = 0; k < morphCnt; ++k) {
            if (active[k] == 0) continue;
            while ((double)keys[cursors[k]].frame < frame &&
                   active[k] != 0) {
                const mdl::MorphKey& rec = keys[cursors[k]];
                const int next = static_cast<int>(rec.next);
                if (next != 0)
                    cursors[k] = next;
                else {
                    active[k] = 0;
                    vals[k].value = rec.value;
                }
            }
            if (active[k] == 0) continue;
            const mdl::MorphKey& rec = keys[cursors[k]];
            float v;
            if (frame == (double)rec.frame) {
                v = rec.value;
            } else {
                const mdl::MorphKey& prev = keys[rec.previous];
                const float tF = (float)((frame - (double)prev.frame) /
                                         (double)(std::uint32_t)(
                                             rec.frame - prev.frame));
                const float delta = rec.value - prev.value;
                v = (float)((double)tF * (double)delta +
                            (double)prev.value);
            }
            vals[k].value = v;
        }
    }

    // ==== section 3: bone tracks (0x4A3AA5) ===============================
    const int boneCnt = static_cast<int>(mdl::Mdl(m)->boneCount);
    if (boneCnt > 0) {
        mdl::BoneKey* const keys = mdl::BoneKeys(m);
        mikudancestudio::mdl::BoneRecord* const bones =
            mdl::Bones(m);
        unsigned char* const active = state.boneTrackActive;
        std::uint32_t* const cursors = state.boneKeyCursors;
        for (int b = 0; b < boneCnt; ++b) {
            mikudancestudio::mdl::BoneRecord* const bone = &bones[b];
            if (active[b] == 0) continue;

            // advance (0x4A3ACA) - MIKUDANCESTUDIO_SPIN_CANARY guards a record-
            // link cycle (diagnostic; the original chain is acyclic)
            int spin = 0;
            while ((double)keys[cursors[b]].frame < frame &&
                   active[b] != 0) {
                if (++spin > 100000 && getenv("MIKUDANCESTUDIO_SPIN_CANARY")) {
                    FILE* sf = fopen(getenv("MIKUDANCESTUDIO_SPIN_CANARY"), "a");
                    if (sf) {
                        fprintf(sf,
                                "BONE spin b=%d cursor=%d frame=%.3f\n",
                                b, cursors[b], frame);
                        fclose(sf);
                    }
                    spin = 0;
                }
                const mdl::BoneKey& rec = keys[cursors[b]];
                const int next = static_cast<int>(rec.next);
                if (next != 0) {
                    cursors[b] = next;
                    continue;
                }
                // terminal key (0x4A3B1C)
                active[b] = 0;
                for (int c = 0; c < 4; ++c)
                    bone->rotQuat2[c] = rec.rotation[c];
                for (int c = 0; c < 3; ++c)
                    bone->trans[c] = rec.position[c];
                const unsigned char mode = rec.physicsDisabled;
                if (bone->f492 != 0 && bone->f493 != mode &&
                    physicsMode >= 2) {
                    Sub499B50(m, b, mode);  // 0x4A3BF9
                    bone->f493 = rec.physicsDisabled;
                }
                const int selIdx = bone->slotIndex;
                if (selIdx < 0) {
                    active[b] = 0;
                } else {
                    const mdl::BoneOrderEntry& window =
                        mdl::BoneOrder(m)[selIdx];
                    const std::uint32_t selStart = window.windowStart;
                    if (rec.frame < selStart &&
                        (double)selStart <= frame) {
                        // zero position + quaternion xyz (0x4A3C79) - the w
                        // component (+344) is deliberately NOT cleared
                        for (int off = 0; off < 6; ++off)
                            bone->trans[off] = 0.0f;
                    }
                    if (window.windowEnd == 0) active[b] = 0;
                }
                break;
            }
            if (active[b] == 0) continue;

            const int cursorIdx = cursors[b];
            const mdl::BoneKey& rec = keys[cursorIdx];
            const std::uint32_t curFrame = rec.frame;

            if (frame == (double)curFrame) {
                // exact hit (0x4A3D32)
                CopyBoneKeyVerbatim(bone, rec);
                if (physicsMode >= 2 && bone->f492 != 0) {  // 0x4A3DBE
                    const int nextIdx = static_cast<int>(rec.next);
                    const unsigned char cmode = rec.physicsDisabled;
                    if (nextIdx <= 0) {
                        if (bone->f493 != cmode) Sub499B50(m, b, cmode);
                        bone->f493 = cmode;
                        continue;
                    }
                    const mdl::BoneKey& nrec = keys[nextIdx];
                    if (nrec.physicsDisabled != 1 || cmode != 0) {
                        if (bone->f493 != cmode) Sub499B50(m, b, cmode);
                        bone->f493 = cmode;
                        continue;
                    }
                    // arm the interpolation for the upcoming segment
                    // (0x4A3E17): notify, remember the start frame, mirror
                    // the backups into the working copies AND the current
                    // pose
                    if (bone->f493 == 0) Sub499B50(m, b, 1);
                    bone->f493 = 1;
                    bone->rigidIdx = static_cast<std::int32_t>(curFrame);
                    for (int c = 0; c < 4; ++c)
                        bone->ikWorkingQuat[c] = bone->ikBackup[3 + c];
                    for (int c = 0; c < 3; ++c)
                        bone->ikWorkingPos[c] = bone->ikBackup[c];
                    for (int c = 0; c < 4; ++c)
                        bone->rotQuat2[c] = bone->ikBackup[3 + c];
                    for (int c = 0; c < 3; ++c)
                        bone->trans[c] = bone->ikBackup[c];
                }
                continue;
            }

            // between keys: previous-key blend inputs (0x4A3F94)
            const int prevIdx = static_cast<int>(rec.previous);
            const mdl::BoneKey& prevRec = keys[prevIdx];
            std::uint32_t prevFrame = prevRec.frame;
            float ppos[3] = {prevRec.position[0], prevRec.position[1],
                             prevRec.position[2]};
            float pq[4] = {prevRec.rotation[0], prevRec.rotation[1],
                           prevRec.rotation[2], prevRec.rotation[3]};

            if (physicsMode >= 2 && bone->f492 != 0) {  // 0x4A3FAE
                if (rec.physicsDisabled == 1 &&
                    prevRec.physicsDisabled == 0) {
                    // mid-interpolation segment (0x4A3FC3)
                    const std::uint32_t start =
                        static_cast<std::uint32_t>(bone->rigidIdx);
                    if (start < prevFrame || start >= curFrame) {
                        // stale arm -> re-arm from the backups (0x4A3FF1,
                        // unsigned; disasm 0x4A3FF1's [ecx+ebp] operand is
                        // the PREVIOUS record, var_34 the CURRENT frame)
                        if (bone->f493 == 0) Sub499B50(m, b, 1);
                        bone->f493 = 1;
                        bone->rigidIdx = static_cast<std::int32_t>(prevFrame);
                        std::memcpy(bone->ikWorkingQuat, bone->ikBackup + 3,
                                    sizeof(bone->ikWorkingQuat));
                        std::memcpy(bone->ikWorkingPos, bone->ikBackup,
                                    sizeof(bone->ikWorkingPos));
                    }
                    if (bone->f493 == 0) Sub499B50(m, b, 1);
                    bone->f493 = 1;
                    // blend from the armed working copies (0x4A40CD)
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
                    if (bone->f493 != pmode) Sub499B50(m, b, pmode);
                    bone->f493 = pmode;
                }
            }

            // selector window (0x4A41A0): snap inside [selStart, selEnd)
            bool fullInterp = true;
            const int selIdx = bone->slotIndex;
            if (selIdx >= 0) {
                const mdl::BoneOrderEntry& window =
                    mdl::BoneOrder(m)[selIdx];
                const std::uint32_t selStart = window.windowStart;
                const std::uint32_t selEnd = window.windowEnd;
                if (selStart < curFrame && prevFrame < selStart) {
                    if ((double)selStart <= frame) {
                        CopyBoneKeyVerbatim(bone, rec);
                        continue;
                    }
                    WriteBonePrevVerbatim(bone, pq, ppos);  // 0x4A421E
                    continue;
                }
                if (curFrame < selEnd || prevFrame >= selEnd) {
                    fullInterp = true;  // LABEL_122
                } else {
                    if ((double)selEnd <= frame) {
                        CopyBoneKeyVerbatim(bone, rec);
                        continue;
                    }
                    WriteBonePrevVerbatim(bone, pq, ppos);  // 0x4A430F
                    continue;
                }
            }
            (void)fullInterp;  // both remaining paths fall through

            // full interpolation (LABEL_122, 0x4A433D)
            const float cq[4] = {rec.rotation[0], rec.rotation[1],
                                 rec.rotation[2], rec.rotation[3]};
            const float tF = (float)(
                (frame - (double)prevFrame) /
                (double)(std::uint32_t)((std::int32_t)curFrame -
                                        (std::int32_t)prevFrame));
            // EASED rotation fraction - the slerp below uses this value
            // (see the file header's decompiler-trap note)
            const float eRot = Sub4A05A0(m, 3, cursorIdx, tF);

            const double dotD = (double)pq[0] * cq[0] +
                                (double)pq[1] * cq[1] +
                                (double)pq[2] * cq[2] +
                                (double)pq[3] * cq[3];
            const float dot = (float)dotD;
            if (static_cast<float>(1.0 - (double)dot * (double)dot) ==
                0.0f) {
                // parallel quaternions: previous key verbatim (0x4A4414)
                for (int c = 0; c < 4; ++c) bone->rotQuat[c] = pq[c];
            } else {
                float dc = dot;
                if (dc > 1.0f)
                    dc = kQuatClamp;
                else if (dc < -1.0f)
                    dc = -kQuatClamp;
                const float th = (float)std::acos((double)dc);
                if ((double)th > PiHalfBits() && (double)dc < 0.0) {
                    // long way around (0x4A44C6): complement angle, weights
                    // subtracted
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

            // position easing (0x4A46AF); the raw fraction goes in, the
            // callee eases per axis.  Type gate uses a SIGNED <= 6 compare
            // on the byte (setle), so negative type bytes pass too.
            const std::int8_t btype =
                static_cast<std::int8_t>(bone->type);
            if (btype <= 6 || bone->type == 8) {
                for (int axis = 0; axis < 3; ++axis) {
                    const float delta = rec.position[axis] - ppos[axis];
                    if (delta == 0.0f) {
                        bone->trans[axis] = ppos[axis];
                    } else {
                        const float e = Sub4A05A0(m, axis, cursorIdx, tF);
                        bone->trans[axis] =
                            (float)((double)e * (double)delta +
                                    (double)ppos[axis]);
                    }
                }
            }
        }
    }
}

}  // namespace mikudancestudio
