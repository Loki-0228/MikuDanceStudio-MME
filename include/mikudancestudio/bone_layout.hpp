// ===========================================================================
// MikuDanceStudio - the per-bone record (GENERATED - do not edit)
// ===========================================================================
// Regenerate: python scripts/gen_bone_layout.py
// x86 layout pinned byte-exact (604, the port's hardcoded stride);
// x64 layout is the compiler's natural regrowth (624 = 0x270, the
// stride mined from the x64 original), anchored at 11 twin-verified
// offsets.  Placeholder names (f<off>/pad*) are promoted to real
// names as semantics are recovered - never guessed.
// ===========================================================================
#pragma once

#include <cstddef>
#include <cstdint>

namespace mikudancestudio::mdl {

#ifndef MIKUDANCESTUDIO_MDL_RAWPAD
#define MIKUDANCESTUDIO_MDL_RAWPAD
template <std::size_t N>
struct RawPad { unsigned char b[N]; };
#endif

// Loader-local PMX IK-link data. This table is converted into the compact
// IkChain link-index list after all PMX bones are read.
struct PmxIkLinkRecord {
    std::int32_t boneIndex;
    std::uint8_t hasLimits;
    unsigned char padding[3];
    float minimum[3];
    float maximum[3];
};
static_assert(sizeof(PmxIkLinkRecord) == 32, "PMX IK link ABI");

struct BoneRecord {
    char name[20];  // 0  (SJIS)
    char nameEn[20];  // 20  (x64 anchor 20 verified (sub_14008CE20))
    wchar_t* jpText;  // 40  (read-text buffer (pmx ReadTextBuf JP); freed after conversion)
    wchar_t* enText;  // 44  (read-text buffer (EN))
    std::int32_t parent;  // 48  (parent bone index)
    float matInit[16];  // 52  (source matrix (debug_geometry walks elements))
    float matLocal[16];  // 116  (skinning/local matrix (bone_transform, vpd 0x74 region))
    float matWorld[16];  // 180  (world matrix; loaders write identity (f%5==0 -> 1.0))
    float matExtra[16];  // 244  (extension matrix (memcpy 0x40 src; dialog_select_ops reads [0]))
    float position[3];  // 308  (model-space position)
    float trans[3];  // 320  (vpd translation (0x140))
    float rotQuat[4];  // 332  (vpd quat (0x14C); w@344=1.0f init)
    float rotQuat2[4];  // 348  (w@360=1.0f init)
    float f364[3];  // 364  (physics_create/bone_transform)
    float f376[4];  // 376  (376..391 bone_transform)
    float ikBackup[7];  // 392  (keyframe backup block 392..419 (kfa mirrors; [3..6]=quat))
    std::int32_t rigidIdx;  // 420  (-296 sentinel = unlinked (x64 anchor 428))
    float ikWorkingPos[3];  // 424  (kfa working-copy position 424..435)
    float ikWorkingQuat[4];  // 436  (kfa working-copy quaternion 436..447)
    std::int32_t selState;  // 452  (frame_modes / sprite_overlay)
    std::int32_t selState2;  // 456
    std::int32_t tailBone;  // 460  (tail-is-bone index)
    float tailOffset[3];  // 464  (tail-is-offset vector)
    std::int32_t f476;  // 476  (sprite_overlay)
    std::int32_t f480;  // 480  (sprite_overlay)
    unsigned char type;  // 484  (bone type byte (x64 anchor 492: <7 or ==8 name filter))
    RawPad<3> gap0;  // 485..488 (unrecovered)
    std::int32_t tailIdx;  // 488  (inheritance source index)
    unsigned char f492;  // 492  (sprite_overlay)
    unsigned char f493;  // 493  (sprite_overlay)
    RawPad<2> gap1;  // 494..496 (unrecovered)
    std::int32_t layer;  // 496  (transform layer)
    std::uint16_t flags;  // 500  (PMD/PMX bone flag bits (x64 anchor 508))
    RawPad<2> gap2;  // 502..504 (unrecovered)
    float inheritRatio;  // 504  (0x100/0x200 inheritance rate)
    float axis[3];  // 508  (fixed axis (0x400), normalized)
    float localAxes[6];  // 520  (local axes (0x800))
    std::int32_t extParent;  // 544  (external parent (0x2000))
    std::int32_t ikTarget;  // 548  (IK target bone)
    std::int32_t ikLoop;  // 552  (IK loop count)
    float ikAngle;  // 556  (IK angle limit)
    std::int32_t ikLinkCount;  // 560
#ifdef _M_X64
    RawPad<4> gap3;  // x64 572..576 (align slide)
#endif
    PmxIkLinkRecord* ikLinks;  // 564  (x64 anchor 576)
    unsigned char twistEnable;  // 568  (pmx2 twist-limit solver gate (link[568], x64 584))
    RawPad<3> gap4;  // 569..572 (unrecovered)
    float ikLimitMin[3];  // 572  (ClampEuler lower bounds (x64 588))
    float ikLimitMax[3];  // 584  (ClampEuler upper bounds)
    unsigned char hasFlag;  // 596  (x64 anchor 612)
    RawPad<3> gap5;  // 597..600 (unrecovered)
    std::int32_t slotIndex;  // 600  (-1 init; x64 anchor 616 (sub_1400A9AC0 [r14+rax-8]))
#ifdef _M_X64
    RawPad<4> gapTail;  // 620..624
#else
#endif
};

#ifndef _M_X64
static_assert(offsetof(BoneRecord, name) == 0,
              "name x86");
static_assert(offsetof(BoneRecord, nameEn) == 20,
              "nameEn x86");
static_assert(offsetof(BoneRecord, jpText) == 40,
              "jpText x86");
static_assert(offsetof(BoneRecord, enText) == 44,
              "enText x86");
static_assert(offsetof(BoneRecord, parent) == 48,
              "parent x86");
static_assert(offsetof(BoneRecord, matInit) == 52,
              "matInit x86");
static_assert(offsetof(BoneRecord, matLocal) == 116,
              "matLocal x86");
static_assert(offsetof(BoneRecord, matWorld) == 180,
              "matWorld x86");
static_assert(offsetof(BoneRecord, matExtra) == 244,
              "matExtra x86");
static_assert(offsetof(BoneRecord, position) == 308,
              "position x86");
static_assert(offsetof(BoneRecord, trans) == 320,
              "trans x86");
static_assert(offsetof(BoneRecord, rotQuat) == 332,
              "rotQuat x86");
static_assert(offsetof(BoneRecord, rotQuat2) == 348,
              "rotQuat2 x86");
static_assert(offsetof(BoneRecord, f364) == 364,
              "f364 x86");
static_assert(offsetof(BoneRecord, f376) == 376,
              "f376 x86");
static_assert(offsetof(BoneRecord, ikBackup) == 392,
              "ikBackup x86");
static_assert(offsetof(BoneRecord, rigidIdx) == 420,
              "rigidIdx x86");
static_assert(offsetof(BoneRecord, ikWorkingPos) == 424,
              "ikWorkingPos x86");
static_assert(offsetof(BoneRecord, ikWorkingQuat) == 436,
              "ikWorkingQuat x86");
static_assert(offsetof(BoneRecord, selState) == 452,
              "selState x86");
static_assert(offsetof(BoneRecord, selState2) == 456,
              "selState2 x86");
static_assert(offsetof(BoneRecord, tailBone) == 460,
              "tailBone x86");
static_assert(offsetof(BoneRecord, tailOffset) == 464,
              "tailOffset x86");
static_assert(offsetof(BoneRecord, f476) == 476,
              "f476 x86");
static_assert(offsetof(BoneRecord, f480) == 480,
              "f480 x86");
static_assert(offsetof(BoneRecord, type) == 484,
              "type x86");
static_assert(offsetof(BoneRecord, tailIdx) == 488,
              "tailIdx x86");
static_assert(offsetof(BoneRecord, f492) == 492,
              "f492 x86");
static_assert(offsetof(BoneRecord, f493) == 493,
              "f493 x86");
static_assert(offsetof(BoneRecord, layer) == 496,
              "layer x86");
static_assert(offsetof(BoneRecord, flags) == 500,
              "flags x86");
static_assert(offsetof(BoneRecord, inheritRatio) == 504,
              "inheritRatio x86");
static_assert(offsetof(BoneRecord, axis) == 508,
              "axis x86");
static_assert(offsetof(BoneRecord, localAxes) == 520,
              "localAxes x86");
static_assert(offsetof(BoneRecord, extParent) == 544,
              "extParent x86");
static_assert(offsetof(BoneRecord, ikTarget) == 548,
              "ikTarget x86");
static_assert(offsetof(BoneRecord, ikLoop) == 552,
              "ikLoop x86");
static_assert(offsetof(BoneRecord, ikAngle) == 556,
              "ikAngle x86");
static_assert(offsetof(BoneRecord, ikLinkCount) == 560,
              "ikLinkCount x86");
static_assert(offsetof(BoneRecord, ikLinks) == 564,
              "ikLinks x86");
static_assert(offsetof(BoneRecord, twistEnable) == 568,
              "twistEnable x86");
static_assert(offsetof(BoneRecord, ikLimitMin) == 572,
              "ikLimitMin x86");
static_assert(offsetof(BoneRecord, ikLimitMax) == 584,
              "ikLimitMax x86");
static_assert(offsetof(BoneRecord, hasFlag) == 596,
              "hasFlag x86");
static_assert(offsetof(BoneRecord, slotIndex) == 600,
              "slotIndex x86");
static_assert(sizeof(BoneRecord) == 604,
              "bone record x86 size");
#else
static_assert(offsetof(BoneRecord, nameEn) == 20,
              "nameEn x64");
static_assert(offsetof(BoneRecord, jpText) == 40,
              "jpText x64");
static_assert(offsetof(BoneRecord, matInit) == 60,
              "matInit x64 (PMX skinning worker)");
static_assert(offsetof(BoneRecord, matWorld) == 188,
              "matWorld x64");
static_assert(offsetof(BoneRecord, position) == 316,
              "position x64");
static_assert(offsetof(BoneRecord, trans) == 328,
              "trans x64");
static_assert(offsetof(BoneRecord, rotQuat) == 340,
              "rotQuat x64");
static_assert(offsetof(BoneRecord, rigidIdx) == 428,
              "rigidIdx x64");
static_assert(offsetof(BoneRecord, type) == 492,
              "type x64");
static_assert(offsetof(BoneRecord, flags) == 508,
              "flags x64");
static_assert(offsetof(BoneRecord, ikLinks) == 576,
              "ikLinks x64");
static_assert(offsetof(BoneRecord, hasFlag) == 612,
              "hasFlag x64");
static_assert(offsetof(BoneRecord, slotIndex) == 616,
              "slotIndex x64");
static_assert(sizeof(BoneRecord) == 624,
              "bone record x64 size");
#endif

}  // namespace mikudancestudio::mdl
