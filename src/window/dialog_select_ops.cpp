// ===========================================================================
// Select-dialog family (original sub_47A3F0 SelectNavDlgProc helpers)
// ===========================================================================
// VA 0x00466630  Sub466630(app, hDlg)  - WM_INITDIALOG list fill
// VA 0x00461C20  Sub461C20(app, hDlg)  - selection refresh (walks combo 669)
// VA 0x004256C0  Sub4256C0(app)        - "all select / reset" apply (id 632)
// VA 0x0043D2E0  Sub43D2E0(app, hDlg, keep) - bone list rebuild (combo 677)
// VA 0x0043D560  Sub43D560(app, hDlg)  - commit bone pick (combo 677 -> rec[4])
// VA 0x0043D610  Sub43D610(app, hDlg)  - apply selection to the model (id 630)
//
// All six are __thiscall in the original with this = Block (g_Block); the
// caller sub_47A3F0 loads ECX from the Block global before each call
// (0x47A462/0x47A4C4/0x47A4DF/0x47A4FD/0x47A536/0x47A556/0x47A57C/0x47A594).
// The former stub signatures in dialog_procs.cpp (HWND-only) were therefore
// missing the MMDApp* first parameter and have been corrected here.
//
// Dialog controls (SelectNavDlg template):
//   669 = bone combo (root entry 0 + bone-list entries from model+0x4CCE4)
//   673 = attach combo (0 = root, 1 = ground, 2.. = models by display order)
//   677 = target bone combo (per-model selectable-bone list)
//   436 = main-window model combo (used only for its item count)
//
// Working state:
//   app+0xA0668 -> array of 20-byte SelectAttachRecord values, one per
//     combo-669 item.
//   app+0xA0B1C -> int[] mapping combo-673 item-2 to model slot
//   app+0xA0B24 -> int[] mapping combo-677 item to bone id
//   app+0xA0664 = "select state changed" byte set after each apply
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "mikudancestudio/d3dx_dyn.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/model.hpp"

namespace mikudancestudio {

// ---- display refresh --------------------------------------------------------
// VA 0x004250C0 - select-dialog value display refresh (thiscall app, hDlg);
// full port at the bottom of this file (local matrix + euler decomposition
// into edits 688..693).
void Sub4250C0(MMDApp* app, HWND hDlg);                // VA 0x004250C0

// ---- intra-family forward declarations (mutual recursion) ------------------
void Sub461C20(MMDApp* app, HWND hDlg);                // VA 0x00461C20
void Sub43D2E0(MMDApp* app, HWND hDlg, int keepSelection); // VA 0x0043D2E0

namespace {

// ---- app offsets not yet registered in offsets.hpp --------------------------
constexpr std::size_t kByteA0664 = 656996;   // 0xA0664 select-state-changed flag
constexpr std::size_t kPtrA0B1C = 658204;    // 0xA0B1C model-order int[] (slot per combo-673 item)
constexpr std::size_t kPtrA0B24 = 658212;    // 0xA0B24 bone-id int[] (per combo-677 item)

// ---- model-object offsets (on the unsigned char* model pointer) -------------
constexpr std::size_t kMdlVertSel = 0x26E0;  // vertex select-buffer ptr (stride 0x168)
constexpr std::size_t kMdlSel2    = 0x26E4;  // select-buffer 2 ptr (stride 0x64)
constexpr std::size_t kMdlSel3    = 0x26E8;  // select-buffer 3 ptr (stride 0x8C)
constexpr std::size_t kMdlNameJp  = 0x2248;  // JP name char[]
constexpr std::size_t kMdlNameEn  = 0x227A;  // EN name char[] (+0x14 past JP)
constexpr std::size_t kMdlOrder   = 0x2D7D;  // display-order byte (0..99)
constexpr std::size_t kMdlSelList = 0x4CCE4; // select-record source array ptr (20-byte recs)
constexpr std::size_t kMdlSelCnt  = 0x4CCE8; // select-record count (combo-669 items)

// ---- .rdata literals --------------------------------------------------------
// JP 0x52D354 "ルート" (root)
const char kJpRoot[] = "\x83\x8b\x81\x5b\x83\x67";
// JP 0x52E7DC "なし" (none) - combo 673 entry 0
const char kJpNone[] = "\x82\xc8\x82\xb5";
// JP 0x52E7D4 "地面" (ground) - combo 673 entry 1
const char kJpGround[] = "\x92\x6e\x96\xca";

unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

unsigned char* ModelAt(MMDApp* app, int slot) {
    return app->ModelSlot(slot);
}

bool English(MMDApp* app) {
    return app->state.englishUI != 0;  // 0xA0B4C
}

struct SelectAttachRecord {
    std::int32_t boneIndex;
    std::int32_t reserved[2];
    std::int32_t targetModelSlot;  // -1 root, -2 ground
    std::int32_t targetBoneIndex;
};
static_assert(sizeof(SelectAttachRecord) == 20, "select dialog record ABI");

// Model-owned selection tracks.  Their payload is not decoded yet, but the
// original only mutates the selection bytes listed below.
struct VertexSelectionTrack {
    std::uint8_t reserved0[0x38];
    std::uint8_t selected0;
    std::uint8_t reserved1[0x3B];
    std::uint8_t selected1;
    std::uint8_t reserved2[0x3B];
    std::uint8_t selected2;
    std::uint8_t reserved3[0x3B];
    std::uint8_t selected3;
    std::uint8_t reserved4[0x3B];
    std::uint8_t selected4;
    std::uint8_t reserved5[0x3B];
    std::uint8_t selected5;
    std::uint8_t reserved6[3];
};
static_assert(sizeof(VertexSelectionTrack) == 0x168,
              "vertex selection track ABI");

struct SelectionTrack100 {
    std::uint8_t reserved0[0x10];
    std::uint8_t selected0;
    std::uint8_t reserved1[0x13];
    std::uint8_t selected1;
    std::uint8_t reserved2[0x13];
    std::uint8_t selected2;
    std::uint8_t reserved3[0x13];
    std::uint8_t selected3;
    std::uint8_t reserved4[0x13];
    std::uint8_t selected4;
    std::uint8_t reserved5[3];
};
static_assert(sizeof(SelectionTrack100) == 0x64, "0x64 selection track ABI");

struct SelectionTrack140 {
    std::uint8_t reserved0[0x14];
    std::uint8_t selected0;
    std::uint8_t reserved1[0x1B];
    std::uint8_t selected1;
    std::uint8_t reserved2[0x1B];
    std::uint8_t selected2;
    std::uint8_t reserved3[0x1B];
    std::uint8_t selected3;
    std::uint8_t reserved4[0x1B];
    std::uint8_t selected4;
    std::uint8_t reserved5[7];
};
static_assert(sizeof(SelectionTrack140) == 0x8C, "0x8c selection track ABI");

// Combo-669 selection record for item index i.
SelectAttachRecord* SelRecord(MMDApp* app, int item) {
    return static_cast<SelectAttachRecord*>(
               app->raw<void*>(offsets::kDwordA0668)) + item;
}

LPARAM StrParam(const void* s) {
    return reinterpret_cast<LPARAM>(const_cast<void*>(s));
}

}  // namespace

// ===========================================================================
// VA 0x00466630 - Sub466630: fill the select dialog on WM_INITDIALOG
// ===========================================================================
void Sub466630(MMDApp* app, HWND hDlg) {
    app->raw<unsigned char>(kByteA0664) = 0;                     // 0x46665D
    if (app->raw<void*>(offsets::kDwordA0668) != nullptr) {      // 0x466664
        free(app->raw<void*>(offsets::kDwordA0668));             // j_j__free_0
        app->raw<void*>(offsets::kDwordA0668) = nullptr;
    }

    unsigned char* model = ActiveModel(app);
    const int count = *reinterpret_cast<int*>(model + kMdlSelCnt);   // 0x4CCE8
    auto* const records = static_cast<SelectAttachRecord*>(
        operator new(sizeof(SelectAttachRecord) * static_cast<std::size_t>(count)));
    app->raw<void*>(offsets::kDwordA0668) = records;
    if (count > 0) {                                                 // 0x4666B4
        memcpy(records, *reinterpret_cast<void**>(model + kMdlSelList),
               sizeof(SelectAttachRecord) * static_cast<std::size_t>(count));
    }

    const HWND combo669 = GetDlgItem(hDlg, 669);                    // 0x46671F
    const HWND combo673 = GetDlgItem(hDlg, 673);                    // 0x466733
    SendMessageA(combo669, 0x14B /*CB_RESETCONTENT*/, 0, 0);        // 0x466737
    SendMessageA(combo673, 0x14B, 0, 0);                            // 0x466747
    SendMessageA(combo669, 0x143 /*CB_ADDSTRING*/, 0,               // 0x46675E
                 English(app) ? StrParam("root")          // 0x52E7E4
                              : StrParam(kJpRoot));      // 0x52D354

    int selItem = 0;                                                 // 0x46676B
    int item = 1;                                                    // 0x466773
    if (count > 1) {
        mdl::BoneRecord* const bones = mdl::Bones(model);
        do {
            const int boneIdx = records[item].boneIndex;             // 0x46679D
            const char* name =
                English(app) ? bones[boneIdx].nameEn : bones[boneIdx].name;
            SendMessageA(combo669, 0x143, 0, StrParam(name));        // 0x4667C1
            if (mdl::Mdl(model)->selectedBone == boneIdx)  // 0x4667F1
                selItem = item;
            ++item;
        } while (item < count);
    }
    const LRESULT cnt669 =
        SendMessageA(combo669, 0x146 /*CB_GETCOUNT*/, 0, 0);         // 0x466819
    if (selItem <= 0)
        SendMessageA(combo669, 0x14E /*CB_SETCURSEL*/, cnt669 > 0, 0); // 0x466838
    else
        SendMessageA(combo669, 0x14E, selItem, 0);                   // 0x466826

    if (English(app)) {                                              // 0x46683A
        SendMessageA(combo673, 0x143, 0, StrParam("non"));     // 0x52D38C (sic)
        SendMessageA(combo673, 0x143, 0, StrParam("ground"));   // 0x52D2A4
    } else {
        SendMessageA(combo673, 0x143, 0, StrParam(kJpNone));    // 0x52E7DC
        SendMessageA(combo673, 0x143, 0, StrParam(kJpGround));  // 0x52E7D4
    }

    // Fill combo 673 item 2.. with the loaded models in display order,
    // mirroring the main-window model combo (item count - 1).
    const HWND mainCombo = GetDlgItem(
        static_cast<HWND>(app->state.hwnd), 436); // 0x4668A0
    const int modelCount =
        static_cast<int>(SendMessageA(mainCombo, 0x146, 0, 0)) - 1;  // 0x4668B1
    if (app->raw<void*>(kPtrA0B1C) != nullptr) {                    // 0x4668AB
        free(app->raw<void*>(kPtrA0B1C));
        app->raw<void*>(kPtrA0B1C) = nullptr;
    }
    int* const order = static_cast<int*>(
        operator new(static_cast<std::size_t>(4 * modelCount)));     // 0x4668DE
    app->raw<void*>(kPtrA0B1C) = order;

    char Buffer[256];                                                // 0x466906
    int nOrder = 0;
    for (int ord = 0; ord < 100; ++ord) {                            // 0x4669DE
        int slot = 0;
        unsigned char* m = nullptr;
        for (; slot < 100; ++slot) {                                 // 0x466911
            unsigned char* cand = ModelAt(app, slot);
            if (cand != nullptr &&
                *reinterpret_cast<unsigned char*>(cand + kMdlOrder) ==
                    static_cast<unsigned char>(ord)) {
                m = cand;
                break;
            }
        }
        if (m == nullptr)
            continue;                                                // LABEL_40
        const char* name = reinterpret_cast<const char*>(
            m + (English(app) ? kMdlNameEn : kMdlNameJp));
        if (slot == app->SelectedModelSlot()) {                      // 0x46692A
            sprintf_s(Buffer, 0x100, "(%s)", name);                  // 0x466951
            SendMessageA(combo673, 0x143, 0, StrParam(Buffer));
        } else {
            SendMessageA(combo673, 0x143, 0, StrParam(name));
        }
        order[nOrder++] = slot;                                      // 0x4669CA
    }

    Sub461C20(app, hDlg);                                            // 0x4669F0
}

// ===========================================================================
// VA 0x00461C20 - Sub461C20: sync combo 673 with the combo-669 record
// ===========================================================================
void Sub461C20(MMDApp* app, HWND hDlg) {
    const HWND combo673 = GetDlgItem(hDlg, 673);                     // 0x461C47
    const HWND combo669 = GetDlgItem(hDlg, 669);                     // 0x461C49
    const int attach = SelRecord(
        app,
        static_cast<int>(SendMessageA(combo669, 0x147 /*CB_GETCURSEL*/, 0, 0)))
        ->targetModelSlot;

    if (attach == -1) {                                              // 0x461C66
        SendMessageA(combo673, 0x14E, 0, 0);
    } else if (attach == -2) {                                       // 0x461C6F
        SendMessageA(combo673, 0x14E, 1, 0);
    } else {
        const int n = static_cast<int>(
                         SendMessageA(combo673, 0x146 /*CB_GETCOUNT*/, 0, 0)) - 2;
        if (n > 0) {                                                 // 0x461C86
            const int* order = static_cast<int*>(app->raw<void*>(kPtrA0B1C));
            int idx = 0;
            while (attach != order[idx]) {
                ++idx;
                if (idx >= n) {
                    Sub43D2E0(app, hDlg, 1);                         // 0x461C9C
                    return;
                }
            }
            SendMessageA(combo673, 0x14E, idx + 2, 0);               // 0x461CBF
        }
    }
    Sub43D2E0(app, hDlg, 1);                                         // 0x461CAC
}

// ===========================================================================
// VA 0x0043D2E0 - Sub43D2E0: rebuild the combo-677 bone list for the
// attach target selected in combo 673
// ===========================================================================
void Sub43D2E0(MMDApp* app, HWND hDlg, int keepSelection) {
    const HWND combo677 = GetDlgItem(hDlg, 677);                     // 0x43D30A
    const int boneSel = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, 669), 0x147 /*CB_GETCURSEL*/, 0, 0));       // 0x43D320
    const int attach = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, 673), 0x147, 0, 0));                        // 0x43D333
    SelectAttachRecord* const rec = SelRecord(app, boneSel);

    if (attach == 0 || attach == 1) {                                // 0x43D348
        rec->targetModelSlot = attach == 0 ? -1 : -2;                // 0x43D350 / 0x43D365
        // LABEL_5: empty target-bone list                          // 0x43D36D
        rec->targetBoneIndex = 0;
        SendMessageA(combo677, 0x14B /*CB_RESETCONTENT*/, 0, 0);
        SendMessageA(combo677, 0x143, 0, StrParam("------"));        // 0x52C0E8
        SendMessageA(combo677, 0x14E, 0, 0);
        Sub4250C0(app, hDlg);                                        // 0x43D394
        return;
    }

    const int modelSlot = static_cast<int*>(app->raw<void*>(kPtrA0B1C))[attach - 2];
    rec->targetModelSlot = modelSlot;                                // 0x43D3B1
    SendMessageA(combo677, 0x14B, 0, 0);                             // 0x43D3B5

    unsigned char* const model = ModelAt(app, modelSlot);
    mdl::BoneRecord* const bones = mdl::Bones(model);
    const int boneCount =
        static_cast<int>(mdl::Mdl(model)->boneCount);                 // 0x2D84

    int nSelectable = 0;                                             // 0x43D3CA
    for (int i = 0; i < boneCount; ++i) {                            // 0x43D3E8
        const unsigned char type = bones[i].type;
        if (type < 7 || type == 8)
            ++nSelectable;
    }

    if (app->raw<void*>(kPtrA0B24) != nullptr) {                     // 0x43D40A
        free(app->raw<void*>(kPtrA0B24));
        app->raw<void*>(kPtrA0B24) = nullptr;
    }
    int* const boneIds = static_cast<int*>(
        operator new(static_cast<std::size_t>(4 * nSelectable)));    // 0x43D441
    app->raw<void*>(kPtrA0B24) = boneIds;

    int n = 0;                                                       // 0x43D454
    for (int i = 0; i < boneCount; ++i) {                            // 0x43D4EA
        const unsigned char type = bones[i].type;
        if (type < 7 || type == 8) {
            const char* name = English(app) ? bones[i].nameEn : bones[i].name;
            SendMessageA(combo677, 0x143, 0, StrParam(name));        // 0x43D4AC
            boneIds[n++] = i;                                        // 0x43D4BC
        }
    }

    if (keepSelection) {                                             // 0x43D4F7
        if (n > 0) {
            int idx = 0;
            while (rec->targetBoneIndex != boneIds[idx]) {
                ++idx;
                if (idx >= n) {
                    Sub4250C0(app, hDlg);                            // 0x43D51F
                    return;
                }
            }
            SendMessageA(combo677, 0x14E, idx, 0);                   // 0x43D52C
        }
    } else {
        SendMessageA(combo677, 0x14E, 0, 0);                         // 0x43D538
        rec->targetBoneIndex = 0;                                    // 0x43D544
    }
    Sub4250C0(app, hDlg);                                            // 0x43D554
}

// ===========================================================================
// VA 0x0043D560 - Sub43D560: store the combo-677 pick into rec[4]
// ===========================================================================
void Sub43D560(MMDApp* app, HWND hDlg) {
    const int boneSel = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, 669), 0x147 /*CB_GETCURSEL*/, 0, 0));       // 0x43D597
    const int attach = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, 673), 0x147, 0, 0));                        // 0x43D5A2
    SelectAttachRecord* const rec = SelRecord(app, boneSel);
    if (attach >= 2) {                                               // 0x43D5A5
        const int pick = static_cast<int>(SendMessageA(
            GetDlgItem(hDlg, 677), 0x147, 0, 0));                    // 0x43D5DF
        rec->targetBoneIndex =
            static_cast<int*>(app->raw<void*>(kPtrA0B24))[pick];
    } else {
        rec->targetBoneIndex = 0;                                    // 0x43D5B0
    }
    Sub4250C0(app, hDlg);                                            // 0x43D5C0
}

// ===========================================================================
// VA 0x004256C0 - Sub4256C0: clear all per-vertex/per-morph selection state
// and restore the select records (button id 632)
// ===========================================================================
void Sub4256C0(MMDApp* app) {
    app->SceneModified() = 1;                                       // 0x4256C5

    unsigned char* const model = ActiveModel(app);

    // Vertex selection tracks: six selection lanes for each of 50,000 slots.
    auto* const verts = static_cast<VertexSelectionTrack*>(
        *reinterpret_cast<void**>(model + kMdlVertSel));
    for (std::size_t i = 0; i < 50000; ++i) {                        // 0x4256CC
        verts[i].selected0 = 0;
        verts[i].selected1 = 0;
        verts[i].selected2 = 0;
        verts[i].selected3 = 0;
        verts[i].selected4 = 0;
        verts[i].selected5 = 0;
    }
    // Five selection lanes for each of 4,000 0x64-byte tracks.
    auto* const sel2 = static_cast<SelectionTrack100*>(
        *reinterpret_cast<void**>(model + kMdlSel2));
    for (std::size_t j = 0; j < 4000; ++j) {                         // 0x42577C
        sel2[j].selected0 = 0;
        sel2[j].selected1 = 0;
        sel2[j].selected2 = 0;
        sel2[j].selected3 = 0;
        sel2[j].selected4 = 0;
    }
    // Five selection lanes for each of 200 0x8c-byte tracks.
    auto* const sel3 = static_cast<SelectionTrack140*>(
        *reinterpret_cast<void**>(model + kMdlSel3));
    for (std::size_t k = 0; k < 200; ++k) {                          // 0x425806
        sel3[k].selected0 = 0;
        sel3[k].selected1 = 0;
        sel3[k].selected2 = 0;
        sel3[k].selected3 = 0;
        sel3[k].selected4 = 0;
    }

    // restore the saved 20-byte select records into the model
    const int count = *reinterpret_cast<int*>(model + kMdlSelCnt);   // 0x4258B2
    if (count > 0) {
        memcpy(*reinterpret_cast<void**>(model + kMdlSelList),
               app->raw<void*>(offsets::kDwordA0668),
               sizeof(SelectAttachRecord) * static_cast<std::size_t>(count));
    }

    Sub49F480(model, app->state.currentFrame);    // 0x42592E
    const auto registeredFrames = mdl::Mdl(model)->maxFrame;         // 0x31B0
    if (app->LastRegisteredFrame() < registeredFrames)                // 0x42594E
        app->LastRegisteredFrame() = registeredFrames;

    PanelPaint(app);                                                 // 0x414610
    app->raw<unsigned char>(kByteA0664) = 1;                         // 0x42595D
}

// ===========================================================================
// VA 0x0043D610 - Sub43D610: apply the selected attach to the current bone
// (button id 630) - matrix bake + selection flag + refresh
// ===========================================================================
void Sub43D610(MMDApp* app, HWND hDlg) {
    const int boneSel = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, 669), 0x147 /*CB_GETCURSEL*/, 0, 0));       // 0x43D650
    const int attach = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, 673), 0x147, 0, 0));                        // 0x43D655
    const SelectAttachRecord* const rec = SelRecord(app, boneSel);

    unsigned char* const model = ActiveModel(app);
    mdl::BoneRecord* const bones = mdl::Bones(model);                // 0x26BC
    mdl::BoneRecord& bone = bones[rec->boneIndex];

    auto* d3dx = &d3dx::Get();
    if (d3dx->Load()) {                                              // D3DX imports
        d3dx::D3DXMATRIXF X = {};                                    // var_90 matrix
        d3dx::D3DXMATRIXF T = {};                                    // var_40 matrix
        if (attach == 0) {
            // root: relative to the parent bone
            const int parent = bone.parent;                           // 0x43D69B
            if (parent < 0) {
                X = *reinterpret_cast<d3dx::D3DXMATRIXF*>(
                    bone.matInit);                                   // 0x43D6F8
            } else {
                d3dx->inverse(
                    &T, nullptr, reinterpret_cast<d3dx::D3DXMATRIXF*>(
                                     bones[parent].matInit));         // 0x43D6BE
                d3dx->multiply(
                    &X, reinterpret_cast<d3dx::D3DXMATRIXF*>(bone.matInit),
                    &T);                                             // 0x43D884
            }
        } else if (attach == 1) {
            // ground: the bone's own local matrix
            X = *reinterpret_cast<d3dx::D3DXMATRIXF*>(bone.matInit); // 0x43D734
        } else {
            // attach to a bone of another model: bake the target bone's
            // world placement relative to the current bone's position
            unsigned char* const tmodel =
                ModelAt(app, rec->targetModelSlot);                  // 0x43D73B
            mdl::BoneRecord& target =
                mdl::Bones(tmodel)[rec->targetBoneIndex];
            const float* const tpos = target.position;
            X = *reinterpret_cast<d3dx::D3DXMATRIXF*>(
                target.matExtra);                                    // 0x43D764
            d3dx->translation(&T, tpos[0], tpos[1], tpos[2]);        // 0x43D7A9
            d3dx->multiply(&X, &T, &X);                              // 0x43D7BE
            const float* const cpos = bone.position;
            d3dx->translation(&T, -cpos[0], -cpos[1], -cpos[2]);     // 0x43D817
            d3dx->multiply(&X, &T, &X);                              // 0x43D82C
            d3dx->inverse(&X, nullptr, &X);                          // 0x43D83B
            T = *reinterpret_cast<d3dx::D3DXMATRIXF*>(bone.matInit); // 0x43D870
            d3dx->multiply(&X, &T, &X);                              // 0x43D884
        }

        // offset of the bone position through the composed matrix
        const float* const cpos2 = bone.position;                    // 0x43D8AC
        float vin[3] = {cpos2[0], cpos2[1], cpos2[2]};
        float vout[4];                                               // var_50
        d3dx->vec3Transform(vout, vin, &X);                          // 0x43D8FB
        bone.trans[0] = vout[0] - vin[0];                            // 0x43D933
        bone.trans[1] = vout[1] - vin[1];                            // 0x43D96D
        bone.trans[2] = vout[2] - vin[2];                            // 0x43D9AB

        // pure-rotation matrix -> bone quaternion
        X.m[3][0] = 0.0f;                                            // 0x43D9BB
        X.m[3][1] = 0.0f;                                            // 0x43D9BF
        X.m[3][2] = 0.0f;                                            // 0x43D9CA
        d3dx->quatFromMatrix(bone.rotQuat, &X);                      // 0x43D9EC
    }

    app->SceneModified() = 1;                                       // 0x43D9F8

    // Single-bone selection state.
    unsigned char* const selFlags = mdl::Mdl(model)->boneSelection;
    const int boneCount = static_cast<int>(mdl::Mdl(model)->boneCount);
    for (int i = 0; i < boneCount; ++i)                              // 0x43DA3F
        selFlags[i] = 0;
    selFlags[rec->boneIndex] = 1;                                    // 0x43DA5E

    Sub4C2080(model, app->state.currentFrame,     // 0x43DA7E
              app->PlaybackPhysicsMode());
    const auto registeredFrames = mdl::Mdl(model)->maxFrame;         // 0x31B0
    if (app->LastRegisteredFrame() < registeredFrames)                // 0x43DA9D
        app->LastRegisteredFrame() = registeredFrames;

    PanelPaint(app);                                                 // 0x414610
    SelectionReeval(app);                                            // 0x430510
    app->raw<unsigned char>(kByteA0664) = 1;                         // 0x43DAB5
}

// ---------------------------------------------------------------------------
// VA 0x004250C0 - select-dialog value display refresh (thiscall app, hDlg;
// called from 0x43D394/0x43D51F/0x43D554/0x43D5C0).  Reads combo 669 (bone
// list entry, 0 = none) and combo 673 (attach kind: 0 root / 1 ground /
// >=2 attached-to-model), computes the bone's local matrix, decomposes it
// into position offset + euler angles and shows the six "%f" values in
// edits 688..693; no selection shows "------" x6 and disables button 630.
//
// Matrix assembly (all via d3dx9_32.dll):
//   kind 1 (ground): local = bone matrix (&bones[rec.bone].matInit[0])
//   kind 0 (root):   parent id at &bone->parent: <0 -> plain bone matrix, else
//                    boneMat * Inverse(parentBoneMat)
//   kind >=2:        T(srcBone.pos) * srcBoneMat, then
//                    T(-parentBone.pos) * that, Inverse, then parentBoneMat
//                    * result (cross-model attach chain through the record's
//                      target-model and target-bone fields)
// Euler extraction (floats, double intermediates, the original's two-tier
// epsilon cleanups and the 3.141592025756836 degree constant):
//   pitch = asin(-m[9]); yaw = atan(m[1]); roll = atan(m[8]); gimbal patch
//   when |cos(pitch)| < 1e-6 (±3.141592 on yaw/roll by matrix signs).
// ---------------------------------------------------------------------------
void Sub4250C0(MMDApp* app, HWND hDlg) {
    auto& s = *app;
    const int sel = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, 669), CB_GETCURSEL, 0, 0));             // 0x425111
    const int kind = static_cast<int>(SendMessageA(
        GetDlgItem(hDlg, 673), CB_GETCURSEL, 0, 0));             // 0x42511A
    if (sel == 0) {
        for (int i = 0; i < 6; ++i)                              // 0x42513D
            SetWindowTextA(GetDlgItem(hDlg, 688 + i), "------");
        EnableWindow(GetDlgItem(hDlg, 630), FALSE);              // 0x42569A
        return;
    }

    unsigned char* model = s.SelectedModel();
    mikudancestudio::mdl::BoneRecord* bones =
        mikudancestudio::mdl::Bones(model);
    unsigned char* recs = s.raw<unsigned char*>(657000);         // 0xA0668
    unsigned char* rec = recs + 20 * sel;
    const std::int32_t boneId =
        *reinterpret_cast<std::int32_t*>(rec);
    mikudancestudio::mdl::BoneRecord* bone = &bones[boneId];

    auto& api = d3dx::Get();
    if (!api.Load())                                             // d3dx absent
        return;
    d3dx::D3DXMATRIXF m{}, t{}, t2{};
    if (kind == 1) {                                             // 0x42521A
        std::memcpy(&m, bone->matInit, sizeof m);
    } else if (kind >= 2) {                                      // 0x425286
        const std::int32_t srcSlot =
            *reinterpret_cast<std::int32_t*>(rec + 12);
        const std::int32_t srcBone =
            *reinterpret_cast<std::int32_t*>(rec + 16);
        unsigned char* srcModel = s.ModelSlot(srcSlot);
        mikudancestudio::mdl::BoneRecord* srcBones =
            mikudancestudio::mdl::Bones(srcModel);
        std::memcpy(&m, &srcBones[srcBone].matInit[0], sizeof m);
        float p[3] = {
            *reinterpret_cast<float*>(srcBones + sizeof(mikudancestudio::mdl::BoneRecord) * srcBone + 308),
            *reinterpret_cast<float*>(srcBones + sizeof(mikudancestudio::mdl::BoneRecord) * srcBone + 312),
            *reinterpret_cast<float*>(srcBones + sizeof(mikudancestudio::mdl::BoneRecord) * srcBone + 316)};
        api.translation(&t, p[0], p[1], p[2]);
        api.multiply(&m, &t, &m);
        api.translation(&t,
                        -*reinterpret_cast<float*>(bone->position),
                        -*reinterpret_cast<float*>(bone + 312),
                        -*reinterpret_cast<float*>(bone + 316));
        api.multiply(&m, &t, &m);
        api.inverse(&m, nullptr, &m);
        std::memcpy(&t2, bone->matInit, sizeof t2);
        api.multiply(&m, &t2, &m);
    } else {                                                     // kind 0
        const std::int32_t parent =
            *reinterpret_cast<std::int32_t*>(&bone->parent);
        if (parent < 0) {                                        // 0x425192
            std::memcpy(&m, bone->matInit, sizeof m);
        } else {
            api.inverse(&t, nullptr,
                        reinterpret_cast<d3dx::D3DXMATRIXF*>(
                            &bones[parent].matInit[0]));
            api.multiply(&m,
                         reinterpret_cast<d3dx::D3DXMATRIXF*>(bone->matInit),
                         &t);
        }
    }

    // ---- offset + euler decomposition ------------------------------------
    float pos[3] = {
        *reinterpret_cast<float*>(bone->position),
        *reinterpret_cast<float*>(bone + 312),
        *reinterpret_cast<float*>(bone + 316)};
    float out[4]{};
    float world[3] = {pos[0], pos[1], pos[2]};
    api.vec3Transform(out, world, &m);                           // 0x42543B
    float values[6];
    values[0] = out[0] - pos[0];
    values[1] = out[1] - pos[1];
    values[2] = out[2] - pos[2];

    float& pitch = values[3];                                    // v54
    float& roll = values[4];                                     // v55
    float& yaw = values[5];                                      // v56
    yaw = static_cast<float>(atan(static_cast<double>(m.m[0][1])));
    pitch = static_cast<float>(asin(-static_cast<double>(m.m[2][1])));
    roll = static_cast<float>(atan(static_cast<double>(m.m[2][0])));
    const double c = cos(static_cast<double>(pitch));
    if (fabs(c) < 0.000001) {                                    // gimbal lock
        yaw = static_cast<float>(
            static_cast<double>(yaw) +
            (static_cast<double>(m.m[0][1]) <= 0.0 ? -3.141592
                                                   : 3.141592));
        roll = static_cast<float>(
            static_cast<double>(roll) +
            (static_cast<double>(m.m[2][0]) <= 0.0 ? -3.141592
                                                   : 3.141592));
    }
    if (fabs(static_cast<double>(pitch)) < 0.000001) pitch = 0.0f;
    if (fabs(static_cast<double>(roll)) < 0.000001) roll = 0.0f;
    if (fabs(static_cast<double>(yaw)) < 0.000001) yaw = 0.0f;
    constexpr float kPiDeg = 3.141592025756836f;
    pitch = static_cast<float>(static_cast<double>(pitch) /
                               kPiDeg * 180.0);
    roll = static_cast<float>(-static_cast<double>(roll) /
                              kPiDeg * 180.0);
    yaw = static_cast<float>(180.0 *
                             (-static_cast<double>(yaw) / kPiDeg));
    const double eps = 0.0000001000000011686097;
    if (fabs(static_cast<double>(pitch)) < eps) pitch = 0.0f;
    if (fabs(static_cast<double>(roll)) < eps) roll = 0.0f;
    if (fabs(static_cast<double>(yaw)) < eps) yaw = 0.0f;

    for (int i = 0; i < 6; ++i) {                                // 0x42563A
        char text[0x100];
        sprintf_s(text, 0x100, "%f", values[i]);
        SetWindowTextA(GetDlgItem(hDlg, 688 + i), text);
    }
    EnableWindow(GetDlgItem(hDlg, 630), TRUE);                   // 0x425685
}

}  // namespace mikudancestudio
