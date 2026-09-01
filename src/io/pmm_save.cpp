// ===========================================================================
// VA 0x0041B080 - SaveSceneFile  (original: sub_41B080, 0x3746 bytes)
// ===========================================================================
// Serialises the whole scene to the .pmm at app+0xA0900 (the caller fills
// that field from the save dialog first; the original ignores any argument
// and reads the path from the app object - it is a __thiscall member).
//
// Layout (write order is the original's, block VAs from the return-address
// landmarks left at every __write thunk call):
//   0x41B1B3  sprintf_s "Polygon Movie maker 0002" -> W(30)  [the 4 bytes
//             past the NUL are stack garbage in the original too]
//   0x41B1C4+ W4 render w/h (A08D4/A08D8), edit flag (A06C8 or A0D3C,
//             selected by the alt-dialog hwnd at A0D38), W4 fov (9E1E8),
//             W1 x6 option flags 0x2F8..0x2FD (bool), W1 slot index 0x910,
//             W1 (CB_GETCOUNT(item 0x1B4) - 1)
//   0x41B381  per existing model slot (0..99, byte = 0-based slot index):
//             name1 (len+bytes, model+0x2248), name2 (model+0x227A),
//             0x100-byte Shift-JIS path (WideToSjis of model+0x24BC),
//             W1 bone count byte (0x26D4), display-frame name table
//             (count 0x2D84, 0x25C-stride names at *0x26BC), morph name
//             table (0x2D80, 0x88 stride at *0x26C4), IK frame table
//             (0x2D88, W4 per 0x18-stride record at *0x26C0), rigid frame
//             table (0x4CCE8, W4 per 0x14-stride record at *0x4CCE4),
//             W1 0x2D7C, W1 bool 0x2D8D, W4 0x2D90, W4x5 0x2D9C..0x2DAC,
//             W1 bone-count byte again + per-bone bools (0x65-stride
//             records at *0x26D0, flag byte at +100), W4 0x31AC/0x31B0,
//             dense display-frame keys [0, count) (0x3C-stride records at
//             *0x26E0: W4 +0/+4/+8, W1 x4 (+0xC..+0x18), W4 x7
//             (+0x1C..+0x34), W1 bool +0x38/+0x39 - the dense loop counter
//             is 16-bit in the original and its record offset wraps with
//             it), then the sparse half: signed (count*0x3C <= off <
//             18000000) scan counting frame!=0 records, W4 count, then the
//             same records prefixed by W4(frame index); morph keys
//             (0x14-stride at *0x26E4, dense + sparse to 20000), rigid/IK
//             keys (0x1C-stride at *0x26E8: W4 +0/+4/+8 + W1 bool +0xC
//             base record; per record IK bool table at +0x10, float pairs
//             at +0x18, W1 bool +0x14; sparse scan 1..1000 re-emits the
//             per-record sub tables), then the current pose dump: per
//             display frame (0x25C records at *0x26BC) W4x7 +0x140..+0x158,
//             W1 bool +0x1ED, W1 bool *0x2D98[i], W1 bool *0x2D94[i]; per
//             morph W4 (+0x30 of the 0x88 records); per IK W1 bool +0x12;
//             per rigid W4 +4/+8/+0xC/+0x10 of the 0x14 records; finally
//             W1 bool 0x31BE, W4 0x31C0, W1 bool 0x37C0, W1 0x2D7D.
//   0x41C9AE  camera track (*0x374): W4x12 record 0 (+0..+0x24,+0x4C,+0x50),
//             W1 x24 (+0x28/+0x2E/+0x34/+0x3A + j, j=0..5), W1 bool +0x40,
//             W4 +0x44, W1 bool +0x48; sparse scan 1..9999 (0x54 stride)
//             each non-empty record prefixed by W4(frame index).
//   0x41CE85  camera misc W4s (0x334/0x338/0x33C/0x308/0x30C/0xA08DC/
//             0x310/0x314/0x318) + W1 bool 0x31C.
//   0x41CF51  light track (*0x378): W4 +0/+4/+8/+0x18/+0x1C/+0x20/+0xC/
//             +0x10/+0x14, W1 bool +0x24; sparse scan 1..9999 (0x28 stride).
//   0x41D207  light misc W4s (9E1A4/9E1A8/9E1AC/9E174/9E178/9E17C/9E170/
//             9DA48).
//   0x41D2AF  shadow list: W1 (u8)CB_GETCOUNT(item 0x1D7); per entry
//             CB_GETLBTEXT into the scratch buffer -> W(100).
//   0x41D310  accessory slots 0..254 (byte = slot index): W(acc+0x238,100),
//             0x100-byte SJIS path of acc+0x29C, W1 acc+0x49D, track
//             (*0x384[slot]) W4 +0/+4/+8, transparency byte
//             ((-0x1C - (int)(float(at +0x38) * 100.0)) * 2, +1 if the
//             byte at +0xC is set), W4 x9 (+0x10..+0x34 skipping +0x18),
//             W1 bool +0xD, W1 bool +0x18, sparse scan 1..9999 (0x3C
//             stride, transparency byte recomputed per record), then the
//             accessory-scale byte from acc+0x4A0 (+1 if acc+0x210),
//             W4 acc+0x230/+0x234/+0x220/+0x224/+0x228/+0x22C/+0x214/
//             +0x218/+0x21C, W1 bool acc+0x49C, W1 bool acc+0x49E.
//   0x41DB42  config: W4 0x980/0x97C/0x9E16C/0x914, W1 0x340 (raw),
//             W1 bool 0x341/0x342/0x9ED99, W4 atol(GetWindowText(item
//             0x199,8)) then W4 atol(item 0x19A,8), W1 bool 0xA06CC,
//             0x100-byte SJIS of 0xD0, if 0x9E400==0 swprintf_s(0x9E1EC,
//             0x100, L"%s") [see deviation note], W4 0x9E414/18/1C, 0x100
//             SJIS of 0x9E1EC, W4 0x91C, same quirk for 0x9E448 when
//             0x9E42C==0, W4 0x9E434/38/3C, 0x100 SJIS of 0x9E448,
//             W1 bool 0x9E428/0x31E/0x31D/0x918, W4 0xA08E0/0x9EB84/
//             0xA0B20/0xA0CF0, W1 bool 0x9ED9A, W1 0xA0CC4 (raw), W4
//             gravity 0x9EDC4/C8/B8/BC/C0, W1 bool 0xA0CD4.
//   0x41DF7C  selection/self-shadow track (*0x380): W4 +0/+4/+8, W1 bool
//             +0x20, W4 +0x1C/+0xC/+0x10/+0x14/+0x18, W1 bool +0x21;
//             sparse scan 1..9999 (0x24 stride).
//   0x41E25E  W1 bool 0xA0188, W4 0xA0D2C.
//   0x41E284  self-shadow track (*0x37C): W4 +0/+4/+8, W1 raw +0xC, W4
//             +0x10, W1 bool +0x14; sparse scan 1..9999 (0x18 stride).
//   0x41E46F  config2: W4 0xA0198/0xA019C/0xA01A0, W1 bool 0xA0194, W4
//             0xA0430..0xA0474 (18 dwords), W1 bool 0x9ED98/0xA0478/
//             0xA0197, W4 max(0, atol(GetWindowText(GetDlgItem(alt-or-
//             main, 0x22A), 10))), W1 constant 1.
//   0x41E70D  per existing model slot: W1 slot index, W4 model+0x4CCF0.
//   0x41E747  success tail: _close, swprintf_s title "MikuMikuDance [%s]"
//             with app+0xA0900, SetWindowTextW, MessageBeep(0x40),
//             app+0xA442C = 1.
// Error paths: path[0]=='\\' -> MessageBoxA JP/EN "cannot open save file"
// caption "save"; _wsopen_s errno -> sprintf_s JP/EN "%d" message with
// JP/EN caption.  0xA0B4C (EnglishUI) selects the branch; the JP texts are
// embedded here as raw Shift-JIS bytes from .rdata 0x52BCE4/0x52BD64/
// 0x52BD80.
//
// Deviations (docs/ARCHITECTURE.md section 8):
//   * swprintf_s(buf, 0x100, L"%s") is called with NO vararg in the
//     original (0x41DCC1/0x41DD7B - the CRT then reads a stack-garbage
//     pointer).  We pass buf itself so the call stays a no-op on the
//     already-valid string; the original's garbage cannot be reproduced.
//   * The 4 stack-garbage bytes after the header NUL and the garbage tails
//     of the 0x100-byte path writes are unspecified stack contents in the
//     original as well.  Our scratch buffer is zero-initialized once at
//     declaration (see below), so our tails are deterministic zeros on the
//     first conversion and previous-path residue afterwards - the
//     original's own garbage values are unreproducible and not attempted.
//   * Saving requires the model keyframe tracks (model+0x26BC/26C0/26C4/
//     26D0/26E0/26E4/26E8/4CCE4 and the count fields) to be allocated by
//     the model constructor; until that constructor is ported the counts
//     are 0 and the sparse scan would dereference a null track exactly
//     like the original would on such a model.
// ===========================================================================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <io.h>

#include "mikudancestudio/accessory_layout.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/ported_funcs.hpp"

namespace mikudancestudio {

// VA 0x00407910, defined in src/window/ui_dropfiles.cpp.
void WideToSjisPath(char* dst, const wchar_t* src, std::size_t size);

namespace {

// fcn.005089ba thunk shape: _write(fd, buf, count), cdecl, result ignored.
inline void W(int fd, const void* buf, unsigned int count) {
    _write(fd, buf, static_cast<unsigned int>(count));
}

void WritePmmBoneKey(int fd, const mdl::BoneKey& key) {
    W(fd, &key.frame, 4); W(fd, &key.previous, 4); W(fd, &key.next, 4);
    for (int lane = 0; lane < 4; ++lane) {
        W(fd, &key.interpolation[lane], 1);
        W(fd, &key.interpolation[lane + 4], 1);
        W(fd, &key.interpolation[lane + 8], 1);
        W(fd, &key.interpolation[lane + 12], 1);
    }
    W(fd, key.position, sizeof key.position);
    W(fd, key.rotation, sizeof key.rotation);
    const unsigned char allocated = key.allocated != 0;
    const unsigned char physicsDisabled = key.physicsDisabled != 0;
    W(fd, &allocated, 1); W(fd, &physicsDisabled, 1);
}

void WritePmmMorphKey(int fd, const mdl::MorphKey& key) {
    W(fd, &key.frame, 4); W(fd, &key.previous, 4); W(fd, &key.next, 4);
    W(fd, &key.value, 4);
    const unsigned char allocated = key.allocated != 0;
    W(fd, &allocated, 1);
}

void WritePmmDisplayKey(int fd, const mdl::DisplayKey& key,
                        int ikCount, int rigidCount) {
    W(fd, &key.frame, 4); W(fd, &key.previous, 4); W(fd, &key.next, 4);
    const unsigned char visible = key.visible != 0;
    W(fd, &visible, 1);
    const auto* flags = mdl::IkStates(key);
    for (int i = 0; i < ikCount; ++i) {
        const unsigned char enabled = flags[i] != 0;
        W(fd, &enabled, 1);
    }
    const auto* pairs = mdl::SelectorStates(key);
    for (int i = 0; i < rigidCount; ++i)
        W(fd, &pairs[i], sizeof(pairs[i]));
    const unsigned char allocated = key.allocated != 0;
    W(fd, &allocated, 1);
}

// strlen walked to the NUL as the original does, then truncated to the
// byte that the length slot holds (asm 0x41B3A8: mov [Buf], al / movzx).
inline unsigned char NameLen(const unsigned char* s) {
    const unsigned char* p = s;
    while (*p != 0) ++p;
    return static_cast<unsigned char>(p - s);
}

// Transparency/scale byte idiom (0x41D44E etc.): FPU control word | 0xC00
// (truncate), fistp of (float * 100.0 double), then
// byte = (char)(2 * (-0x1C - v)), +1 when the paired flag byte is set.
inline unsigned char ScaleQuirkByte(float scale01, unsigned char additive) {
    const int v = static_cast<int>(static_cast<double>(scale01) * 100.0);
    unsigned char b = static_cast<unsigned char>((-0x1C - v) * 2);
    if (additive != 0) b = static_cast<unsigned char>(b + 1);
    return b;
}

void WritePmmAccessoryKey(int fd, const mdl::AccessoryKey& key) {
    W(fd, &key.frame, 4);
    W(fd, &key.previous, 4);
    W(fd, &key.next, 4);
    const unsigned char visibleAndOpacity =
        ScaleQuirkByte(key.opacity, key.visible);
    W(fd, &visibleAndOpacity, 1);
    W(fd, &key.parentModel, 4);
    W(fd, &key.parentBone, 4);
    W(fd, key.position, sizeof key.position);
    W(fd, key.rotation, sizeof key.rotation);
    W(fd, &key.scale, 4);
    const unsigned char shadowEnabled = key.shadowEnabled != 0;
    const unsigned char selected = key.selected != 0;
    W(fd, &shadowEnabled, 1);
    W(fd, &selected, 1);
}

// Shift-JIS texts from the original .rdata (embedded byte-exact).
const char kJpSaveFailFmt[] =         // 0x52BCE4
    "\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xaa\x95\xdb\x91\xb6\x82\xc5"
    "\x82\xab\x82\xdc\x82\xb9\x82\xf1:%d";
const char kJpSaveFailCaption[] =     // 0x52BD64
    "\x83\x74\x83\x40\x83\x43\x83\x8b\x95\xdb\x91\xb6";
const char kJpCannotOpenText[] =      // 0x52BD80
    "\x82\xbb\x82\xcc\x83\x74\x83\x48\x83\x8b\x83\x5f\x82\xc9\x82\xcd"
    "\x83\x5a\x81\x5b\x83\x75\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82\xf1";

}  // namespace

void SaveSceneFile(MMDApp* app) {
    auto* s = app;
    namespace off = offsets;

    // The scratch CHAR buffer (stack "Text").  The original leaves this
    // uninitialized, so every 0x100 fixed-width path field it feeds carries
    // whatever stack history preceded the save - deterministic in the
    // original's codegen, run-to-run garbage in ours.  Zero-initialize the
    // buffer once: first conversion gets zero tails (a documented deviation
    // - the original's garbage is unreproducible by design), later
    // conversions still carry the previous path's bytes exactly like the
    // original's buffer reuse.  This makes our own back-to-back saves
    // byte-identical, matching the original's determinism property.
    char text[0x100] = {};
    char hdr[0x100];    // sprintf buffer at stack -0x34
    wchar_t title[0x100];

    unsigned char** const slots = s->ModelSlots();
    mdl::AccessoryRecord** const accessories = s->AccessorySlots();
    mdl::AccessoryKey** const accTracks = s->AccessoryKeyTracks();
    HWND const main = reinterpret_cast<HWND>(s->Hwnd());

    if (s->EnvFileName()[0] == L'\\') {                        // 0x41B097
        MessageBoxA(main,
                    s->EnglishUI() != 0 ? "Cannot open save file"
                                        : kJpCannotOpenText,
                    "save", 0);
        return;
    }

    s->SceneModified() = 0;                                    // 0x41B0C6
    int fd = -1;
    const errno_t err =
        _wsopen_s(&fd, s->EnvFileName(), 0x8301, 0x40, 0x80);  // 0x41B125
    if (err != 0) {
        if (s->EnglishUI() == 0)
            sprintf_s(hdr, 0x100, kJpSaveFailFmt, err);
        else
            sprintf_s(hdr, 0x100, "Cannot save file:%d", err);
        MessageBoxA(main, hdr,
                    s->EnglishUI() == 0 ? kJpSaveFailCaption : "save file",
                    0);
        return;
    }

    // ---- 1. file header and global block (0x41B1B3..0x41B355) -----------
    s->raw<std::uint32_t>(off::kByteA442C) = 0;
    sprintf_s(hdr, 0x100, "Polygon Movie maker 0002");
    // Bytes 25..29 after the 24-char magic + NUL are unspecified stack
    // residue in the original (its sprintf leaves the buffer's prior bytes
    // there); observed original saves carry zeros, so pin them to zero for
    // deterministic output (same deviation class as the text buffer below).
    hdr[25] = hdr[26] = hdr[27] = hdr[28] = hdr[29] = 0;
    W(fd, hdr, 0x1E);                                          // 0x41B1C4

    W(fd, &s->raw<std::uint32_t>(off::kDwordRenderw), 4);      // 0x41B1D7
    W(fd, &s->raw<std::uint32_t>(off::kDwordRenderh), 4);      // 0x41B1EA
    {
        const std::int32_t v = s->raw<std::int32_t>(off::kDwordA0d38);
        if (v == 0)
            W(fd, &s->raw<std::uint32_t>(off::kDwordSidebar), 4);
        else
            W(fd, &s->raw<std::uint32_t>(off::kDwordV658748), 4);
    }
    W(fd, &s->raw<std::uint32_t>(off::kFloat9e1e8), 4);        // 0x41B22E

    for (int f = 0; f < 7; ++f) {                              // 0x41B22E..0x41B303
        const unsigned char b =
            s->raw<unsigned char>(760 + f) != 0;               // 0x2F8..0x2FE
        W(fd, &b, 1);
    }
    W(fd, &s->SelectedModelSlot(), 1);                         // 0x41B31A

    {                                                          // 0x41B33F
        const unsigned char b = static_cast<unsigned char>(
            SendMessageA(GetDlgItem(main, 0x1B4), 0x146, 0, 0) - 1);
        W(fd, &b, 1);
    }

    // ---- 2. per-model block (0x41B355..0x41C981) ------------------------
    for (unsigned char slot = 0; slot < 100; ++slot) {         // loc_41B360
        if (slots[slot] == 0) continue;
        W(fd, &slot, 1);                                       // 0x41B381
        unsigned char* const model = slots[slot];

        {  // name1 at model+0x2248
            const unsigned char len = NameLen(model + 0x2248);
            W(fd, &len, 1);                                    // 0x41B3B7
            W(fd, model + 0x2248, len);                        // 0x41B3D9
        }
        {  // name2 at model+0x227A
            const unsigned char len = NameLen(model + 0x227A);
            W(fd, &len, 1);                                    // 0x41B40F
            W(fd, model + 0x227A, len);                        // 0x41B432
        }

        WideToSjisPath(text,                                  // 0x41B45D
                       mdl::Mdl(model)->path, 0x100);
        W(fd, text, 0x100);                                    // 0x41B471

        const mdl::ModelRecord* const record = mdl::Mdl(model);
        const std::int32_t dispCount =
            static_cast<std::int32_t>(record->boneCount);
        const std::int32_t morphCount =
            static_cast<std::int32_t>(record->morphCount);
        const std::int32_t ikCount =
            static_cast<std::int32_t>(record->ikChainCount);
        const std::int32_t rigidCount =
            *reinterpret_cast<const std::int32_t*>(model + 0x4CCE8);
        mdl::BoneRecord* const bones = mdl::Bones(model);
        mdl::MorphRecord* const morphs = mdl::Morphs(model);
        mdl::IkChain* const ikChains = mdl::IkChains(model);
        unsigned char* const rigidFrames =
            *reinterpret_cast<unsigned char**>(model + 0x4CCE4);
        mdl::BoneKey* const dispKeys = mdl::BoneKeys(model);
        mdl::MorphKey* const morphKeys = mdl::MorphKeys(model);
        mdl::DisplayKey* const physKeys = mdl::DisplayKeys(model);

        W(fd, model + 0x26D4, 1);                              // 0x41B48E
        W(fd, &record->boneCount, sizeof(record->boneCount));  // 0x41B4AC
        if (dispCount != 0 && dispCount > 0) {
            for (std::int32_t i = 0; i < dispCount; ++i) {
                const unsigned char len = NameLen(
                    reinterpret_cast<const unsigned char*>(bones[i].name));
                W(fd, &len, 1);                                // 0x41B4FF
                W(fd, bones[i].name, len);                     // 0x41B524
            }
        }
        W(fd, &record->morphCount, sizeof(record->morphCount)); // 0x41B562
        if (morphCount != 0 && morphCount > 0) {
            for (std::int32_t i = 0; i < morphCount; ++i) {
                const unsigned char len = NameLen(
                    reinterpret_cast<const unsigned char*>(morphs[i].name));
                W(fd, &len, 1);                                // 0x41B5AF
                W(fd, morphs[i].name, len);                    // 0x41B5D4
            }
        }
        W(fd, &record->ikChainCount, sizeof(record->ikChainCount)); // 0x41B612
        if (ikCount != 0 && ikCount > 0) {
            for (std::int32_t i = 0; i < ikCount; ++i)
                W(fd, &ikChains[i].boneIndex, sizeof(ikChains[i].boneIndex));
        }
        W(fd, model + 0x4CCE8, 4);                             // 0x41B680
        if (rigidCount != 0 && rigidCount > 0) {
            for (std::int32_t i = 0, o = 0; i < rigidCount; ++i, o += 0x14)
                W(fd, rigidFrames + o, 4);                     // 0x41B6B5
        }
        W(fd, model + 0x2D7C, 1);                              // 0x41B6EF
        {
            const unsigned char b =
                *reinterpret_cast<const unsigned char*>(model + 0x2D8D) != 0;
            W(fd, &b, 1);                                      // 0x41B719
        }
        W(fd, &record->selectedBone, sizeof(record->selectedBone)); // 0x41B737
        for (const std::int32_t selectedMorph :
             mdl::Mdl(model)->selectedMorphs)
            W(fd, &selectedMorph, sizeof(selectedMorph));      // 0x41B75B

        const unsigned char boneCount =
            *reinterpret_cast<const unsigned char*>(model + 0x26D4);
        W(fd, model + 0x26D4, 1);                              // 0x41B790
        if (boneCount != 0) {
            unsigned char* const bones =
                *reinterpret_cast<unsigned char**>(model + 0x26D0);
            for (unsigned char i = 0; i < boneCount; ++i) {    // 0x41B7D5
                const unsigned char b = bones[100 + i * 0x65] != 0;
                W(fd, &b, 1);
            }
        }
        W(fd, &record->boneListPos, sizeof(record->boneListPos)); // 0x41B800
        W(fd, &record->maxFrame, sizeof(record->maxFrame));    // 0x41B81F

        // dense display-frame key records (0x41B860..0x41BAA4); the
        // original walks both the counter and the record offset through
        // a 16-bit register (movzx eax, bx at 0x41BABA), so the pair
        // wraps together past 65535.
        if (dispCount != 0 && dispCount > 0) {
            for (std::uint16_t i = 0;
                 static_cast<std::int32_t>(i) < dispCount; ++i) {
                WritePmmBoneKey(fd, dispKeys[i]);
            }
        }
        // sparse half: signed scan count*0x3C <= off < 18000000
        {                                                      // 0x41BACC
            std::int32_t cnt = 0;
            if (dispCount < 300000) {
                for (std::int32_t i = dispCount; i < 300000; ++i)
                    if (dispKeys[i].frame != 0)
                        ++cnt;
            }
            W(fd, &cnt, 4);                                    // 0x41BB2D
            if (dispCount < 300000) {                          // 0x41BB44
                for (std::int32_t i = dispCount; i < 300000; ++i) {
                    if (dispKeys[i].frame == 0)
                        continue;
                    W(fd, &i, 4);                              // 0x41BB92
                    WritePmmBoneKey(fd, dispKeys[i]);
                }
            }
        }

        // morph keys: dense then sparse (0x41BEE8..0x41C13A)
        if (morphCount != 0 && morphCount > 0) {
            for (std::int32_t i = 0; i < morphCount; ++i)
                WritePmmMorphKey(fd, morphKeys[i]);
        }
        {                                                      // 0x41BFFD
            std::int32_t cnt = 0;
            if (morphCount < 20000) {
                for (std::int32_t i = morphCount; i < 20000; ++i)
                    if (morphKeys[i].frame != 0)
                        ++cnt;
            }
            W(fd, &cnt, 4);
            if (morphCount < 20000) {
                for (std::int32_t i = morphCount; i < 20000; ++i) {
                    if (morphKeys[i].frame == 0)
                        continue;
                    W(fd, &i, 4);                              // 0x41C05E
                    WritePmmMorphKey(fd, morphKeys[i]);
                }
            }
        }

        // rigid/IK base record (0x41C173..)
        WritePmmDisplayKey(fd, physKeys[0], ikCount, rigidCount);
        {  // sparse rigid/IK scan, 1000 records of 0x1C (0x41C353..)
            std::int32_t cnt = 0;
            for (std::int32_t i = 1; i < 1000; ++i)
                if (physKeys[i].frame != 0) ++cnt;
            W(fd, &cnt, 4);
            for (std::int32_t i = 1; i < 1000; ++i) {          // 0x41C394
                if (physKeys[i].frame == 0)
                    continue;
                W(fd, &i, 4);
                WritePmmDisplayKey(fd, physKeys[i], ikCount, rigidCount);
            }
        }

        // current pose dump (0x41C5EB..0x41C8AD)
        if (dispCount != 0 && dispCount > 0) {
            unsigned char* const physicsState = record->bonePhysicsState;
            unsigned char* const selection = record->boneSelection;
            for (std::int32_t i = 0; i < dispCount; ++i) {
                W(fd, &bones[i].trans[0], 4);                   // 0x41C5EB
                W(fd, &bones[i].trans[1], 4);                   // 0x41C611
                W(fd, &bones[i].trans[2], 4);                   // 0x41C637
                W(fd, &bones[i].rotQuat[0], 4);                 // 0x41C65D
                W(fd, &bones[i].rotQuat[1], 4);                 // 0x41C683
                W(fd, &bones[i].rotQuat[2], 4);                 // 0x41C6A9
                W(fd, &bones[i].rotQuat[3], 4);                 // 0x41C6D2
                {
                    const unsigned char b = bones[i].f493 != 0;
                    W(fd, &b, 1);                              // 0x41C703
                }
                {
                    const unsigned char b = physicsState[i] != 0; // 0x41C730
                    W(fd, &b, 1);
                }
                {
                    const unsigned char b = selection[i] != 0; // 0x41C75D
                    W(fd, &b, 1);
                }
            }
        }
        if (morphCount != 0 && morphCount > 0) {
            for (std::int32_t i = 0; i < morphCount; ++i)
                W(fd, &morphs[i].value, sizeof(morphs[i].value));
        }
        if (ikCount != 0 && ikCount > 0) {
            for (std::int32_t i = 0; i < ikCount; ++i) {
                const unsigned char b = ikChains[i].enabled != 0;
                W(fd, &b, 1);                                  // 0x41C812
            }
        }
        if (rigidCount != 0 && rigidCount > 0) {
            for (std::int32_t i = 0, o = 0; i < rigidCount; ++i, o += 0x14) {
                W(fd, rigidFrames + o + 4, 4);                 // 0x41C867
                W(fd, rigidFrames + o + 8, 4);                 // 0x41C88A
                W(fd, rigidFrames + o + 0xC, 4);               // 0x41C8AD
                W(fd, rigidFrames + o + 0x10, 4);              // 0x41C8D0
            }
        }
        {
            const unsigned char b =                            // 0x41C91A
                *reinterpret_cast<const unsigned char*>(model + 0x31BE) != 0;
            W(fd, &b, 1);
        }
        W(fd, model + 0x31C0, 4);                              // 0x41C939
        {
            const unsigned char b =                            // 0x41C963
                *reinterpret_cast<const unsigned char*>(model + 0x37C0) != 0;
            W(fd, &b, 1);
        }
        const unsigned char displayOrder = mdl::Mdl(model)->comboSelIndex2;
        W(fd, &displayOrder, sizeof(displayOrder));             // 0x41C981
    }

    // ---- 3. camera track (0x41C9AE..0x41CE85) ---------------------------
    unsigned char* const cam =
        *reinterpret_cast<unsigned char**>(&s->raw<std::uint32_t>(off::kDword374));
    W(fd, cam + 0x00, 4);                                      // 0x41C9AE
    W(fd, cam + 0x04, 4);                                      // 0x41C9C4
    W(fd, cam + 0x08, 4);                                      // 0x41C9DA
    W(fd, cam + 0x0C, 4);                                      // 0x41C9F0
    W(fd, cam + 0x10, 4);                                      // 0x41CA06
    W(fd, cam + 0x14, 4);                                      // 0x41CA1C
    W(fd, cam + 0x18, 4);                                      // 0x41CA35
    W(fd, cam + 0x1C, 4);                                      // 0x41CA4B
    W(fd, cam + 0x20, 4);                                      // 0x41CA61
    W(fd, cam + 0x24, 4);                                      // 0x41CA77
    W(fd, cam + 0x4C, 4);                                      // 0x41CA8D
    W(fd, cam + 0x50, 4);                                      // 0x41CAA3
    for (int j = 0; j < 6; ++j) {                              // 0x41CAC6..
        W(fd, cam + 0x28 + j, 1);
        W(fd, cam + 0x2E + j, 1);
        W(fd, cam + 0x34 + j, 1);
        W(fd, cam + 0x3A + j, 1);
    }
    {
        const unsigned char b = cam[0x40] != 0;                // 0x41CB33
        W(fd, &b, 1);
    }
    W(fd, cam + 0x44, 4);                                      // 0x41CB49
    {
        const unsigned char b = cam[0x48] != 0;                // 0x41CB6A
        W(fd, &b, 1);
    }
    {  // sparse scan over camera keys 1..9999 (0x54 stride)
        std::int32_t cnt = 0;
        const std::int32_t* p = reinterpret_cast<const std::int32_t*>(cam);
        for (int k = 0; k < 3333; ++k, p += 0x3F) {
            if (p[0x15] != 0) ++cnt;                           // 0x41CBB8..
            if (p[0x2A] != 0) ++cnt;
            if (p[0x3F] != 0) ++cnt;
        }
        W(fd, &cnt, 4);
        for (std::int32_t i = 1; i < 10000; ++i) {             // 0x41CBE6
            const std::size_t o = static_cast<std::size_t>(i) * 0x54;
            if (*reinterpret_cast<const std::int32_t*>(cam + o) == 0)
                continue;
            W(fd, &i, 4);
            W(fd, cam + o + 0x00, 4);                          // 0x41CC00
            W(fd, cam + o + 0x04, 4);                          // 0x41CC1E
            W(fd, cam + o + 0x08, 4);                          // 0x41CC3C
            W(fd, cam + o + 0x0C, 4);                          // 0x41CC5A
            W(fd, cam + o + 0x10, 4);                          // 0x41CC78
            W(fd, cam + o + 0x14, 4);                          // 0x41CC99
            W(fd, cam + o + 0x18, 4);                          // 0x41CCB7
            W(fd, cam + o + 0x1C, 4);                          // 0x41CCD5
            W(fd, cam + o + 0x20, 4);                          // 0x41CCF3
            W(fd, cam + o + 0x24, 4);                          // 0x41CD11
            W(fd, cam + o + 0x4C, 4);                          // 0x41CD2F
            W(fd, cam + o + 0x50, 4);                          // 0x41CD50
            for (int j = 0; j < 6; ++j) {                      // 0x41CD7F..
                W(fd, cam + o + 0x28 + j, 1);
                W(fd, cam + o + 0x2E + j, 1);
                W(fd, cam + o + 0x34 + j, 1);
                W(fd, cam + o + 0x3A + j, 1);
            }
            {
                const unsigned char b = cam[o + 0x40] != 0;    // 0x41CE13
                W(fd, &b, 1);
            }
            W(fd, cam + o + 0x44, 4);                          // 0x41CE31
            {
                const unsigned char b = cam[o + 0x48] != 0;    // 0x41CE5A
                W(fd, &b, 1);
            }
        }
    }

    // camera misc (0x41CE85..0x41CF3E)
    W(fd, &s->raw<std::uint32_t>(off::kFloatPosx), 4);         // 0x334
    W(fd, &s->raw<std::uint32_t>(off::kFloatPosy), 4);         // 0x338
    W(fd, &s->raw<std::uint32_t>(off::kFloatPosz), 4);         // 0x33C
    W(fd, &s->raw<std::uint32_t>(off::kFloatCam0), 4);         // 0x308
    W(fd, &s->raw<std::uint32_t>(off::kFloatCam1), 4);         // 0x30C
    W(fd, &s->raw<std::uint32_t>(off::kFloatCamangle), 4);     // 0xA08DC
    W(fd, &s->raw<std::uint32_t>(off::kFloatCam2), 4);         // 0x310
    W(fd, &s->raw<std::uint32_t>(off::kFloatCam3), 4);         // 0x314
    W(fd, &s->raw<std::uint32_t>(off::kFloatCam4), 4);         // 0x318
    {
        const unsigned char b =                                // 0x31C
            s->raw<unsigned char>(off::kByte31C) != 0;
        W(fd, &b, 1);
    }

    // ---- 4. light track (0x41CF51..0x41D207) ----------------------------
    unsigned char* const light =
        *reinterpret_cast<unsigned char**>(&s->raw<std::uint32_t>(off::kDword378));
    W(fd, light + 0x00, 4);                                    // 0x41CF51
    W(fd, light + 0x04, 4);                                    // 0x41CF67
    W(fd, light + 0x08, 4);                                    // 0x41CF80
    W(fd, light + 0x18, 4);                                    // 0x41CF96
    W(fd, light + 0x1C, 4);                                    // 0x41CFAC
    W(fd, light + 0x20, 4);                                    // 0x41CFC2
    W(fd, light + 0x0C, 4);                                    // 0x41CFD8
    W(fd, light + 0x10, 4);                                    // 0x41CFEE
    W(fd, light + 0x14, 4);                                    // 0x41D007
    {
        const unsigned char b = light[0x24] != 0;              // 0x41D028
        W(fd, &b, 1);
    }
    {  // sparse scan over light keys 1..9999 (0x28 stride)
        std::int32_t cnt = 0;
        const std::int32_t* p = reinterpret_cast<const std::int32_t*>(light);
        for (int k = 0; k < 3333; ++k, p += 0x1E) {
            if (p[10] != 0) ++cnt;                             // 0x41D072..
            if (p[0x14] != 0) ++cnt;
            if (p[0x1E] != 0) ++cnt;
        }
        W(fd, &cnt, 4);
        for (std::int32_t i = 1; i < 10000; ++i) {             // 0x41D0A3
            const std::size_t o = static_cast<std::size_t>(i) * 0x28;
            if (*reinterpret_cast<const std::int32_t*>(light + o) == 0)
                continue;
            W(fd, &i, 4);
            W(fd, light + o + 0x00, 4);                        // 0x41D0C0
            W(fd, light + o + 0x04, 4);                        // 0x41D0DE
            W(fd, light + o + 0x08, 4);                        // 0x41D0FC
            W(fd, light + o + 0x18, 4);                        // 0x41D11A
            W(fd, light + o + 0x1C, 4);                        // 0x41D138
            W(fd, light + o + 0x20, 4);                        // 0x41D159
            W(fd, light + o + 0x0C, 4);                        // 0x41D177
            W(fd, light + o + 0x10, 4);                        // 0x41D195
            W(fd, light + o + 0x14, 4);                        // 0x41D1B3
            {
                const unsigned char b = light[o + 0x24] != 0;  // 0x41D1DC
                W(fd, &b, 1);
            }
        }
    }

    // light misc (0x41D207..0x41D28F)
    W(fd, s->LightColor() + 0, sizeof(float));
    W(fd, s->LightColor() + 1, sizeof(float));
    W(fd, s->LightColor() + 2, sizeof(float));
    W(fd, s->LightDirection() + 0, sizeof(float));
    W(fd, s->LightDirection() + 1, sizeof(float));
    W(fd, s->LightDirection() + 2, sizeof(float));
    W(fd, &s->SelectedAccessorySlot(), 1);                      // 0x41D26D 1 byte
    W(fd, &s->DisplayObjectListScrollPosition(), 4);

    // ---- 5. accessory-shadow list (0x41D2AF..0x41D310) ------------------
    {
        const unsigned char cnt = static_cast<unsigned char>(
            SendMessageA(GetDlgItem(main, 0x1D7), 0x146, 0, 0));
        W(fd, &cnt, 1);                                        // 0x41D2CD
        for (unsigned int i = 0; i < cnt; ++i) {               // 0x41D2FC
            SendMessageA(GetDlgItem(main, 0x1D7), 0x148, i,
                         reinterpret_cast<LPARAM>(text));
            W(fd, text, 100);                                  // 0x41D310
        }
    }

    // ---- 6. accessory block (0x41D310..0x41DB42) ------------------------
    for (unsigned char slot = 0; slot != 0xFF; ++slot) {       // 0x41D351
        if (accessories[slot] == 0) continue;
        W(fd, &slot, 1);
        const mdl::AccessoryRecord& accessory =
            *mdl::Accessory(accessories[slot]);
        const auto* const track = reinterpret_cast<const mdl::AccessoryKey*>(
            accTracks[slot]);

        W(fd, accessory.name, sizeof accessory.name);          // 0x41D370
        WideToSjisPath(text,                                   // 0x41D39B
                       accessory.sourcePath, 0x100);
        W(fd, text, 0x100);                                    // 0x41D3AF
        W(fd, &accessory.order, 1);                             // 0x41D3CD

        WritePmmAccessoryKey(fd, track[0]);
        {  // sparse scan over accessory keys 1..9999 (0x3C stride)
            std::int32_t cnt = 0;
            for (std::int32_t i = 1; i < 10000; ++i)
                if (track[i].frame != 0) ++cnt;
            W(fd, &cnt, 4);
            for (std::int32_t i = 1; i < 10000; ++i) {         // 0x41D66E
                if (track[i].frame == 0)
                    continue;
                W(fd, &i, 4);
                WritePmmAccessoryKey(fd, track[i]);
            }
        }
        {
            const unsigned char b = ScaleQuirkByte(            // 0x41D9AC
                accessory.opacity, accessory.visible);
            W(fd, &b, 1);
        }
        W(fd, &accessory.parentModel, 4);                       // 0x41D9CB
        W(fd, &accessory.parentBone, 4);                        // 0x41D9EA
        W(fd, &accessory.rotation[0], 4);                       // 0x41DA09
        W(fd, &accessory.rotation[1], 4);                       // 0x41DA28
        W(fd, &accessory.rotation[2], 4);                       // 0x41DA47
        W(fd, &accessory.scale, 4);                             // 0x41DA69
        W(fd, &accessory.position[0], 4);                       // 0x41DA88
        W(fd, &accessory.position[1], 4);                       // 0x41DAA7
        W(fd, &accessory.position[2], 4);                       // 0x41DAC6
        {
            const unsigned char b = accessory.shadowEnabled != 0;
            W(fd, &b, 1);
        }
        {
            const unsigned char b = accessory.additiveBlend != 0;
            W(fd, &b, 1);
        }
    }

    // ---- 7. config block (0x41DB42..0x41DF7C) ---------------------------
    W(fd, &s->raw<std::uint32_t>(off::kDword980), 4);
    W(fd, &s->raw<std::uint32_t>(off::kDword97C), 4);
    W(fd, &s->raw<std::uint32_t>(off::kDword9E16C), 4);
    const std::int32_t savedEditMode = static_cast<std::int32_t>(s->EditMode());
    W(fd, &savedEditMode, 4);
    W(fd, &s->raw<unsigned char>(off::kByte340), 1);           // raw byte
    {
        const unsigned char b = s->raw<unsigned char>(off::kByte341) != 0;
        W(fd, &b, 1);
    }
    {
        const unsigned char b = s->raw<unsigned char>(off::kByte342) != 0;
        W(fd, &b, 1);
    }
    {
        const unsigned char b = s->raw<unsigned char>(off::kByteB9ed99) != 0;
        W(fd, &b, 1);
    }
    {  // frame edit readbacks 0x199 then 0x19A (asm 0x41DBEA..0x41DC5F)
        GetWindowTextA(GetDlgItem(main, 0x199), text, 8);
        const std::int32_t v1 = atol(text);
        GetWindowTextA(GetDlgItem(main, 0x19A), text, 8);
        const std::int32_t v2 = atol(text);
        W(fd, &v1, 4);
        W(fd, &v2, 4);
    }
    {
        const unsigned char b = s->raw<unsigned char>(off::kByteA06CC) != 0;
        W(fd, &b, 1);
    }
    WideToSjisPath(text,                                       // 0x41DC85
                   reinterpret_cast<const wchar_t*>(
                       &s->raw<wchar_t>(off::kWcsWavpath)),
                   0x100);
    W(fd, text, 0x100);                                        // 0x41DCB5
    if (s->AviStream() == nullptr) {                            // 0x41DCC1
        swprintf_s(s->AviBackgroundPath(), 0x100, L"%s",
                   s->AviBackgroundPath());
    }
    W(fd, &s->AviOffsetX(), 4);
    W(fd, &s->AviOffsetY(), 4);
    W(fd, &s->AviScale(), 4);
    WideToSjisPath(text, s->AviBackgroundPath(), 0x100);        // 0x41DD32
    W(fd, text, 0x100);                                        // 0x41DD46
    W(fd, &s->AviBackgroundEnabled(), 4);
    if (s->PictureBackgroundTexture() == nullptr) {             // 0x41DD7B
        swprintf_s(s->PictureBackgroundPath(), 0x100, L"%s",
                   s->PictureBackgroundPath());
    }
    W(fd, &s->PictureOffsetX(), 4);
    W(fd, &s->PictureOffsetY(), 4);
    W(fd, &s->PictureScale(), 4);
    WideToSjisPath(text, s->PictureBackgroundPath(), 0x100);    // 0x41DDD6
    W(fd, text, 0x100);                                        // 0x41DDEA
    {
        const unsigned char b = s->PictureBackgroundEnabled() != 0;
        W(fd, &b, 1);
    }
    {
        const unsigned char b = s->raw<unsigned char>(off::kByte31E) != 0;
        W(fd, &b, 1);
    }
    {
        const unsigned char b = s->raw<unsigned char>(off::kByte31D) != 0;
        W(fd, &b, 1);
    }
    {
        const unsigned char b = s->raw<unsigned char>(off::kByte918) != 0;
        W(fd, &b, 1);
    }
    W(fd, &s->raw<std::uint32_t>(off::kFloatFpslimit), 4);     // 0xA08E0
    W(fd, &s->raw<std::uint32_t>(off::kDword9EB84), 4);
    W(fd, &s->raw<std::uint32_t>(off::kDwordA0B20), 4);
    W(fd, &s->ProjectedShadowAmbientIntensity(), 4);
    {
        const unsigned char b = s->raw<unsigned char>(off::kByteB9ed9a) != 0;
        W(fd, &b, 1);
    }
    W(fd, reinterpret_cast<const unsigned char*>(
              &s->PlaybackPhysicsMode()), 1);                  // raw byte
    W(fd, &s->raw<std::uint32_t>(off::kFloatGravmag), 4);      // 0x9EDC4
    W(fd, &s->raw<std::uint32_t>(off::kDword9EDC8), 4);
    W(fd, &s->raw<std::uint32_t>(off::kFloatGravx), 4);        // 0x9EDB8
    W(fd, &s->raw<std::uint32_t>(off::kFloatGravy), 4);        // 0x9EDBC
    W(fd, &s->raw<std::uint32_t>(off::kFloatGravz), 4);        // 0x9EDC0
    {
        const unsigned char b = s->raw<unsigned char>(off::kByteA0CD4) != 0;
        W(fd, &b, 1);
    }

    // ---- 8. selection/self-shadow track (0x41DF7C..0x41E25E) ------------
    unsigned char* const sel =
        *reinterpret_cast<unsigned char**>(&s->raw<std::uint32_t>(off::kDword380));
    W(fd, sel + 0x00, 4);                                      // 0x41DF7C
    W(fd, sel + 0x04, 4);                                      // 0x41DF95
    W(fd, sel + 0x08, 4);                                      // 0x41DFAB
    {
        const unsigned char b = sel[0x20] != 0;                // 0x41DFCC
        W(fd, &b, 1);
    }
    W(fd, sel + 0x1C, 4);                                      // 0x41DFE2
    W(fd, sel + 0x0C, 4);                                      // 0x41DFF8
    W(fd, sel + 0x10, 4);                                      // 0x41E00E
    W(fd, sel + 0x14, 4);                                      // 0x41E027
    W(fd, sel + 0x18, 4);                                      // 0x41E03D
    {
        const unsigned char b = sel[0x21] != 0;                // 0x41E05E
        W(fd, &b, 1);
    }
    {  // sparse scan over selection keys 1..9999 (0x24 stride)
        std::int32_t cnt = 0;
        const std::int32_t* p = reinterpret_cast<const std::int32_t*>(sel);
        for (int k = 0; k < 3333; ++k, p += 0x1B) {
            if (p[9] != 0) ++cnt;                              // 0x41E0AC..
            if (p[0x12] != 0) ++cnt;
            if (p[0x1B] != 0) ++cnt;
        }
        W(fd, &cnt, 4);
        for (std::int32_t i = 1; i < 10000; ++i) {             // 0x41E0E4
            const std::size_t o = static_cast<std::size_t>(i) * 0x24;
            if (*reinterpret_cast<const std::int32_t*>(sel + o) == 0)
                continue;
            W(fd, &i, 4);
            W(fd, sel + o + 0x00, 4);                          // 0x41E101
            W(fd, sel + o + 0x04, 4);                          // 0x41E11F
            W(fd, sel + o + 0x08, 4);                          // 0x41E13D
            {
                const unsigned char b = sel[o + 0x20] != 0;    // 0x41E166
                W(fd, &b, 1);
            }
            W(fd, sel + o + 0x1C, 4);                          // 0x41E184
            W(fd, sel + o + 0x0C, 4);                          // 0x41E1A5
            W(fd, sel + o + 0x10, 4);                          // 0x41E1C3
            W(fd, sel + o + 0x14, 4);                          // 0x41E1E1
            W(fd, sel + o + 0x18, 4);                          // 0x41E1FF
            {
                const unsigned char b = sel[o + 0x21] != 0;    // 0x41E228
                W(fd, &b, 1);
            }
        }
    }

    {
        const unsigned char b = s->raw<unsigned char>(off::kByteA0188) != 0;
        W(fd, &b, 1);                                          // 0x41E25E
    }
    W(fd, &s->raw<std::uint32_t>(off::kByteBa0d2c), 4);        // 0xA0D2C

    // ---- 9. self-shadow track (0x41E284..0x41E46F) ----------------------
    unsigned char* const shadow =
        *reinterpret_cast<unsigned char**>(&s->raw<std::uint32_t>(off::kDword37C));
    W(fd, shadow + 0x00, 4);                                   // 0x41E284
    W(fd, shadow + 0x04, 4);                                   // 0x41E29A
    W(fd, shadow + 0x08, 4);                                   // 0x41E2B0
    W(fd, shadow + 0x0C, 1);                                   // raw byte
    W(fd, shadow + 0x10, 4);                                   // 0x41E2DE
    {
        const unsigned char b = shadow[0x14] != 0;             // 0x41E2FF
        W(fd, &b, 1);
    }
    {  // sparse scan over self-shadow keys 1..9999 (0x18 stride)
        std::int32_t cnt = 0;
        const std::int32_t* p = reinterpret_cast<const std::int32_t*>(shadow);
        for (int k = 0; k < 3333; ++k, p += 0x12) {
            if (p[6] != 0) ++cnt;                              // 0x41E34D..
            if (p[0xC] != 0) ++cnt;
            if (p[0x12] != 0) ++cnt;
        }
        W(fd, &cnt, 4);
        for (std::int32_t i = 1; i < 10000; ++i) {             // 0x41E384
            const std::size_t o = static_cast<std::size_t>(i) * 0x18;
            if (*reinterpret_cast<const std::int32_t*>(shadow + o) == 0)
                continue;
            W(fd, &i, 4);
            W(fd, shadow + o + 0x00, 4);                       // 0x41E3A1
            W(fd, shadow + o + 0x04, 4);                       // 0x41E3BF
            W(fd, shadow + o + 0x08, 4);                       // 0x41E3DD
            W(fd, shadow + o + 0x0C, 1);                       // raw byte
            W(fd, shadow + o + 0x10, 4);                       // 0x41E418
            {
                const unsigned char b = shadow[o + 0x14] != 0; // 0x41E444
                W(fd, &b, 1);
            }
        }
    }

    // ---- 10. config2 (0x41E46F..0x41E6E5) --------------------------------
    W(fd, &s->raw<std::uint32_t>(off::kDwordA0198), 4);
    W(fd, &s->raw<std::uint32_t>(off::kDwordA019C), 4);
    W(fd, &s->raw<std::uint32_t>(off::kDwordA01A0), 4);
    {
        const unsigned char b = s->raw<unsigned char>(off::kByteA0194) != 0;
        W(fd, &b, 1);
    }
    W(fd, &s->raw<std::uint32_t>(off::kDwordA0430), 4);
    W(fd, &s->raw<std::uint32_t>(off::kDwordA0434), 4);
    W(fd, &s->raw<std::uint32_t>(off::kFloatColor16), 4);      // 0xA0438
    for (int i = 0; i < 15; ++i)                               // 0xA043C..A0474
        W(fd, &s->raw<std::uint32_t>(offsets::kDwordA043c0 +
                                     static_cast<std::size_t>(i) * 4),
          4);
    {
        const unsigned char b = s->raw<unsigned char>(off::kDwordF9ed98) != 0;
        W(fd, &b, 1);                                          // 0x9ED98
    }
    {
        const unsigned char b = s->raw<unsigned char>(off::kByteA0478) != 0;
        W(fd, &b, 1);                                          // 0xA0478
    }
    {
        const unsigned char b = s->raw<unsigned char>(off::kByteA0197) != 0;
        W(fd, &b, 1);
    }
    {  // 0x22A edit readback on the alt dialog when present (0x41E69E)
        const HWND alt = s->raw<HWND>(off::kDwordA0d38);
        GetWindowTextA(GetDlgItem(alt != nullptr ? alt : main, 0x22A), text, 10);
        std::int32_t v = atol(text);
        if (v < 0) v = 0;
        W(fd, &v, 4);                                          // 0x41E6D1
    }
    {
        const unsigned char one = 1;                           // 0x41E6E5
        W(fd, &one, 1);
    }

    // ---- 11. per-model tail (0x41E70D) -----------------------------------
    for (unsigned char slot = 0; slot < 100; ++slot) {         // loc_41E6F0
        if (slots[slot] == 0) continue;
        W(fd, &slot, 1);
        W(fd, slots[slot] + 0x4CCF0, 4);                       // 0x41E72C
    }

    // ---- 12. success tail (0x41E747) --------------------------------------
    _close(fd);
    swprintf_s(title, 0x100, L"MikuDanceStudio [%s]", s->EnvFileName());
    SetWindowTextW(main, title);
    MessageBeep(0x40);
    s->raw<std::uint32_t>(off::kByteA442C) = 1;
}

}  // namespace mikudancestudio
