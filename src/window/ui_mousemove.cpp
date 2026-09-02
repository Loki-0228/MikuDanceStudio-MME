// ===========================================================================
// VA 0x00444CC0 - HandleMouseMove  (original: sub_444CC0)
// ===========================================================================
// WM_MOUSEMOVE handler for the editor panel (~0x1DA6 bytes, 441 basic
// blocks; called from WndProc 0x4C3A10, case 512).  Original signature:
//   HCURSOR __thiscall sub_444CC0(int this, unsigned __int16 X, unsigned
//   __int16 Y)
// The caller pushes lParam (mouse X in the low word) then lParam >> 16
// (mouse Y), i.e. X = lParam & 0xFFFF and Y = mouseY here.  The HCURSOR
// return value is discarded by the wndproc, so the port is void, but the
// early returns are preserved because they gate the hide-rect flag clears.
//
// Flow (original instruction ranges):
//   1. 0x444CC6  three hide-rect gates (bytes this+0xA06B4/0xA06B5/0xA06B6):
//      when the mouse sits inside the stored rect this+0xA0D40..0xA0D4C the
//      gate flag is cleared so later moves can re-enter; otherwise the
//      handler returns with the flag still set.
//   2. 0x444D5B  on a real move: store X/Y into this+4/this+8 (with the
//      0xEA60 -> X-0x10000 wrap), maintain the >50px jump flag
//      (byte this+0xA05D3, prev this+0xC/0x10, gate byte this+0x9F12C),
//      GetClientRect, then cursor switching:
//        IDC_SIZEWE (0x7F84): while the pointer hugs the sidebar edge
//          (sidebar <= X <= sidebar+6 && Y <= bottom-158 && 0xA0D38 == 0)
//        IDC_HAND   (0x7F89): while interaction mode (dword this+0x344) == 1
//   3. 0x444E54  sidebar drag (byte this+0xC8): sidebar this+0xA06C8 =
//      clamp(X, 250, clientRight-5), ratio float this+0xA4428, then
//      Sub442EB0 + PanelPaint and three local InvalidateRect passes
//      (right strip / timeline strip / top strip).
//   4. 0x444F46  if byte this+0xA03EB != 0: hover row = (X - 0xA018C - 6)
//      / 13, byte this+0xA0B0D = 1, then the drag-frame remap over the
//      hovered row's list(s): display mode (optflag0 @0x2F8 != 0) walks
//      the four fixed band lists (84/40/24/36-byte records via this+0x374/
//      0x378/0x37C/0x380) and the accessory lists (this+0x384, 60-byte
//      records) driven by the 16-byte offset records at this+0xA03EC..
//      0xA0410, then PanelPaint, Sub42E640, Sub411070, Sub411B90,
//      Sub412330, Sub413120 x255 and Sub4134E0; bone-edit mode walks the
//      model bone/morph/IK lists (28/20/60-byte records, offset records at
//      this+0xA0414..0xA0428) then PanelPaint + Sub4B4260.
//   5. 0x44606F  else, if byte this+0xA0189 != 0: hover-highlight pass over
//      the five fixed-band regions (window-y bands 160..174 / 174..188 /
//      188..202 / 202..216, i.e. the four fixed bands + the accessory
//      grid; row pitch 13 starting at x = 94, column pitch 14 starting at
//      y = 209).  The visibility flags of the band records are first
//      cleared for all 200 rows (0x4461CE display / 0x44653B bone-edit,
//      gated on mode word this+0x24 != 3) and then re-set for the hovered
//      rows to (this+0xC0 != 3); the hover rect lands in
//      this+0xA05C0..0xA05CC.  Ends with PanelPaint and the rect zeroed.
//   6. 0x446A4C  if byte this+0x9DA09 != 0: Sub416280.
//
// The magic-number divisions (/13 = 0x4EC4EC4F<<2, /14 = 0x92492493<<3 with
// add) were verified numerically against the original instruction stream -
// both are plain C truncating division, so ordinary `/` is used.
//
// Reference: ../translated/MikuMikuDance/fcn_00444cc0.cpp (machine
//            translation, incomplete; IDA IDB is authoritative)
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "mikudancestudio/accessory_layout.hpp"
#include "mikudancestudio/global_key_layout.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/model.hpp"

namespace mikudancestudio {

// VA 0x00414610 - ported (ui_panel_paint.cpp).
void PanelPaint(MMDApp* app);

// Not-yet-ported refresh helpers (src/unported/stubs.cpp).
void Sub4134E0(MMDApp* a);              // 0x4134E0
void Sub411070(MMDApp* a);              // 0x411070
void Sub412330(MMDApp* a);              // 0x412330
void Sub411B90(MMDApp* a);              // 0x411B90
void Sub416280(MMDApp* a);              // 0x416280
void Sub42E640(MMDApp* a);              // 0x42E640
void Sub442EB0(MMDApp* a);              // 0x442EB0
int Sub4B4260(unsigned char* model, int frame, int a3);  // 0x4B4260
void Sub413120(MMDApp* a, int idx);     // 0x413120

namespace {

// --- app offsets not yet named in offsets.hpp (0x literals) ----------------
constexpr std::size_t kMapIk = 0x984;        // IK row map (bone-edit)
constexpr std::size_t kMapMorph = 0x27A84;   // morph row map (bone-edit)
constexpr std::size_t kMapBone = 0x4EB84;    // bone row map (bone-edit)
constexpr std::size_t kMapBand0 = 0x75C84;   // band0 row map (200 ints)
constexpr std::size_t kMapBand1 = 0x75FA4;   // band1 row map
constexpr std::size_t kMapBand2 = 0x762C4;   // band2 row map
constexpr std::size_t kMapBand3 = 0x765E4;   // band3 row map
constexpr std::size_t kMapAcc = 0x76904;     // accessory map [row][band]
constexpr std::size_t kSlotArr = 0x9DA50;    // accessory slot array (200 ints)
constexpr std::size_t kCtlMorphCnt = 0xA041C;
constexpr std::size_t kCtlMorphPtr = 0xA0420;
constexpr std::size_t kCtlBoneCnt = 0xA0424;
constexpr std::size_t kCtlBonePtr = 0xA0428;

// --- layout constants -------------------------------------------------------
constexpr int kRowPitch = 13;     // 0x0D  hover row pitch
constexpr int kColPitch = 14;     // 0x0E  hover column pitch (bands)
constexpr int kGridX0 = 94;       // 0x5E  first hover row x
constexpr int kColY0 = 209;       // 0xD1  first hover column y (display)
constexpr int kColY0Bone = 153;   // 0x99  first hover column y (bone-edit)
constexpr int kSidebarMin = 250;  // 0xFA  sidebar drag minimum

// Offsets that exist in offsets.hpp under their generated names.
using offsets::kByte9F12C;
using offsets::kByteA06B4;
using offsets::kByteA06B5;
using offsets::kByteA06B6;
using offsets::kByteB6568483;
using offsets::kByteOptflag0;
using offsets::kDword980;
using offsets::kDwordHideBottom;
using offsets::kDwordHideLeft;
using offsets::kDwordHideRight;
using offsets::kPtrHwnd;

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

unsigned char* CurrentModel(MMDApp* app) {
    return app->SelectedModel();
}

// Frame-remap walk over 16-byte offset records {idx, parent, slot, frame}
// referenced by the band lists (display mode 0x444F8C..0x445833), or by the
// model bone/morph/IK lists (bone-edit mode 0x445889..0x446040).  Every
// record that references list entry `idx` rewrites the entry's frame so the
// dragged frames land on the hovered row:
//   row == 0: frame = rec.frame                       (direct copy)
//   row <  0: parent chain via record field +4        (order-preserving)
//   row >  0: reverse walk, parent chain via field +8 (clamped, and the
//             entry frame feeds the this+0x9E16C / model+0x31B0 maxima)
// `limit` >= 0 selects the `idx >= limit` validity test (morph/IK), -1 the
// `idx > 0` test (bands/bone).  `maxModel` is model+0x31B0 for the model
// lists, nullptr for the band lists.
template <typename Key>
void RemapList(MMDApp* app, const int* recs, int count, Key* keys,
               int row, int limit, std::uint32_t* maxModel,
               int capacity = 10000) {
    auto valid = [limit](int idx) {
        return limit >= 0 ? idx >= limit : idx > 0;
    };
    // TEMP(debug, keyframe-drag crash): validate the record buffer before
    // the remap walks it - rec[0] must be a list index within capacity.
    // Logs and skips the whole remap instead of dereferencing a wild index.
    for (int i = 0; i < count; ++i) {
        const int idx = recs[4 * i];
        if (idx < 0 || idx >= capacity) {
            std::fprintf(stderr,
                         "REMAP-GUARD: rec[%d/%d]={%d,%d,%d,%d} row=%d "
                         "limit=%d capacity=%d keys=%p recs=%p\n",
                         i, count, idx, recs[4 * i + 1], recs[4 * i + 2],
                         recs[4 * i + 3], row, limit, capacity,
                         static_cast<void*>(keys), static_cast<const void*>(recs));
            for (int d = 0; d < 8 && d < count; ++d)
                std::fprintf(stderr, "  rec[%d]={%d,%d,%d,%d}\n", d,
                             recs[4 * d], recs[4 * d + 1], recs[4 * d + 2],
                             recs[4 * d + 3]);
            std::fflush(stderr);
            return;
        }
    }
    if (row == 0) {
        for (int i = 0; i < count; ++i) {
            const int* rec = recs + 4 * i;
            if (valid(rec[0]))
                keys[rec[0]].frame = rec[3];
        }
    } else if (row < 0) {
        for (int i = 0; i < count; ++i) {
            const int* rec = recs + 4 * i;
            const int idx = rec[0];
            if (valid(idx)) {
                Key& key = keys[idx];
                const std::int32_t pf = static_cast<std::int32_t>(
                    keys[key.previous].frame);
                if (pf + 1 < row + rec[3]) {
                    const std::int32_t v = row + rec[3];
                    if (v > 0)
                        key.frame = v;
                } else {
                    key.frame = pf + 1;
                }
            }
        }
    } else {
        for (int i = count - 1; i >= 0; --i) {
            const int* rec = recs + 4 * i;
            const int idx = rec[0];
            if (valid(idx)) {
                Key& key = keys[idx];
                if (key.next == 0) {
                    key.frame = row + rec[3];
                } else {
                    const std::int32_t nextFrame =
                        static_cast<std::int32_t>(keys[key.next].frame);
                    if (nextFrame - 1 > row + rec[3])
                        key.frame = row + rec[3];
                    else
                        key.frame = nextFrame - 1;
                }
                const std::uint32_t frame = key.frame;
                if (maxModel != nullptr) {
                    if (*maxModel < frame)
                        *maxModel = frame;
                    if (app->LastRegisteredFrame() < *maxModel)
                        app->LastRegisteredFrame() = *maxModel;
                } else if (app->LastRegisteredFrame() < frame) {
                    app->LastRegisteredFrame() = frame;
                }
            }
        }
    }
}

// Accessory-list variant of RemapList (0x445621..0x445833): the offset
// records carry the index at +4 and the accessory slot at +8; the list base
// is this+0x384[slot] and the records are 60 bytes.
void RemapAccList(MMDApp* app, const int* recs, int count, int row) {
    auto listOf = [app](int slot) {
        return app->AccessoryKeys(slot);
    };
    // TEMP(debug, keyframe-drag crash): same validation as RemapList - the
    // accessory records carry the key index at +4 and the slot at +8.
    for (int i = 0; i < count; ++i) {
        const int idx = recs[4 * i + 1];
        const int slot = recs[4 * i + 2];
        if (idx < 0 || idx >= static_cast<int>(mdl::kTimelineKeyCapacity) ||
            slot < 0 || slot >= 255 || listOf(slot) == nullptr) {
            std::fprintf(stderr,
                         "REMAPACC-GUARD: rec[%d/%d]={%d,%d,%d,%d} row=%d\n",
                         i, count, recs[4 * i], idx, slot, recs[4 * i + 3],
                         row);
            for (int d = 0; d < 8 && d < count; ++d)
                std::fprintf(stderr, "  rec[%d]={%d,%d,%d,%d}\n", d,
                             recs[4 * d], recs[4 * d + 1], recs[4 * d + 2],
                             recs[4 * d + 3]);
            std::fflush(stderr);
            return;
        }
    }
    if (row == 0) {
        for (int i = 0; i < count; ++i) {
            const int* rec = recs + 4 * i;
            const int idx = rec[1];
            if (idx > 0)
                listOf(rec[2])[idx].frame = rec[3];
        }
    } else if (row < 0) {
        for (int i = 0; i < count; ++i) {
            const int* rec = recs + 4 * i;
            const int idx = rec[1];
            if (idx > 0) {
                mdl::AccessoryKey* list = listOf(rec[2]);
                mdl::AccessoryKey& key = list[idx];
                const std::int32_t pf = static_cast<std::int32_t>(
                    list[key.previous].frame);
                if (pf + 1 < row + rec[3]) {
                    const std::int32_t v = row + rec[3];
                    if (v > 0)
                        key.frame = v;
                } else {
                    key.frame = pf + 1;
                }
            }
        }
    } else {
        for (int i = count - 1; i >= 0; --i) {
            const int* rec = recs + 4 * i;
            const int idx = rec[1];
            if (idx > 0) {
                mdl::AccessoryKey* list = listOf(rec[2]);
                mdl::AccessoryKey& key = list[idx];
                if (key.next == 0) {
                    key.frame = row + rec[3];
                } else {
                    const std::int32_t nextFrame =
                        static_cast<std::int32_t>(list[key.next].frame);
                    if (nextFrame - 1 > row + rec[3])
                        key.frame = row + rec[3];
                    else
                        key.frame = nextFrame - 1;
                }
                const std::uint32_t frame = key.frame;
                if (app->LastRegisteredFrame() < frame)
                    app->LastRegisteredFrame() = frame;
            }
        }
    }
}

// Write the hover flag value (Ctrl not active) into a band record flag byte.
inline unsigned char HoverFlag(MMDApp* app) {
    return static_cast<unsigned char>(!app->CtrlModifierActive());
}

void SetAccessorySelected(MMDApp* app, int slot, int keyIndex,
                          std::uint8_t selected) {
    auto* keys = app->AccessoryKeys(slot);
    keys[keyIndex].selected = selected;
}

}  // namespace

// ===========================================================================
// WM_MOUSEMOVE handler (see header comment for the flow map).
// ===========================================================================
void HandleMouseMove(std::uint32_t lParam, int mouseY) {
    MMDApp* app = g_Block;
    const int X = static_cast<int>(lParam & 0xFFFFu);  // a2 (low word)
    const int Y = mouseY;                              // a3 (high word)

    // --- 1. hide-rect gates (0x444CC6) -------------------------------------
    if (app->raw<std::uint8_t>(kByteA06B5) != 0) {
        D3DRenderer* sub = app->Renderer();
        const int ratio = static_cast<int>(
            sub->viewScale * 200.0);  // viewScale = sub+0x1D4F0 float ratio
        if (X > static_cast<int>(app->raw<std::int32_t>(kDwordHideLeft)) - ratio)
            return;
        app->raw<std::uint8_t>(kByteA06B5) = 0;
    }
    if (app->raw<std::uint8_t>(kByteA06B4) != 0) {
        if (Y > app->raw<std::int32_t>(kDwordHideBottom))
            return;
        app->raw<std::uint8_t>(kByteA06B4) = 0;
    }
    if (app->raw<std::uint8_t>(kByteA06B6) != 0) {
        if (X > app->raw<std::int32_t>(kDwordHideLeft) ||
            X < app->raw<std::int32_t>(kDwordHideRight))
            return;
        // Original stores (X < 0xA0D40), which is false on this path (0x444D55).
        app->raw<std::uint8_t>(kByteA06B6) = 0;
    }

    // --- 2. position store + cursor switching (0x444D5B) -------------------
    if (app->MouseY() != Y || app->MouseX() != X) {
        app->MouseX() = X;
        app->MouseY() = Y;
        if (X > 0xEA60)
            app->MouseX() = X - 0x10000;
        if (Y > 0xEA60)
            app->MouseY() = Y - 0x10000;
        if (app->raw<std::uint8_t>(kByte9F12C) != 0) {
            const int x0 = app->MouseX();
            const int y0 = app->MouseY();
            if (std::abs(app->PreviousMouseX() - x0) > 50 ||
                std::abs(app->PreviousMouseY() - y0) > 50)
                app->raw<std::uint8_t>(kByteB6568483) = 1;
            app->PreviousMouseX() = x0;
            app->PreviousMouseY() = y0;
            app->raw<std::uint8_t>(kByte9F12C) = 0;
        }
        RECT rc;
        GetClientRect(static_cast<HWND>(app->Hwnd()), &rc);
        if ((app->SidebarWidth() <=
                 app->MouseX() && app->MouseY() <= rc.bottom - 158 &&
             app->MouseX() <=
                 app->SidebarWidth() + 6) &
            (app->FloatingWindow() == nullptr)) {
            SetCursor(LoadCursorA(nullptr,
                                  reinterpret_cast<LPCSTR>(
                                      static_cast<INT_PTR>(0x7F84))));  // IDC_SIZEWE
        }

        // --- 3. sidebar drag (0x444E54) ------------------------------------
        if (app->SidebarResizeDragging() != 0) {
            const int x = app->MouseX();
            app->SidebarWidth() = x;
            if (x < kSidebarMin)
                app->SidebarWidth() = kSidebarMin;
            if (rc.right - 5 < app->SidebarWidth())
                app->SidebarWidth() = rc.right - 5;
            app->SidebarRatio() = static_cast<float>(
                static_cast<double>(app->SidebarWidth()) /
                static_cast<double>(rc.right));
            Sub442EB0(app);
            PanelPaint(app);
            const int sidebar = app->SidebarWidth();
            HWND hwnd = static_cast<HWND>(app->Hwnd());
            rc.bottom -= 158;
            rc.left = sidebar - 19;
            InvalidateRect(hwnd, &rc, FALSE);
            rc.right = sidebar;
            rc.left = 0;
            rc.top = rc.bottom - 90;
            InvalidateRect(hwnd, &rc, FALSE);
            rc.left = 0;
            rc.top = 0;
            rc.right = sidebar;
            rc.bottom = 145;
            InvalidateRect(hwnd, &rc, FALSE);
        }
        if (app->ViewportToolHovered() == 1) {
            SetCursor(LoadCursorA(nullptr,
                                  reinterpret_cast<LPCSTR>(
                                      static_cast<INT_PTR>(0x7F89))));  // IDC_HAND
        }

        // --- 4. drag-frame remap (0x444F46) --------------------------------
        if (app->TimelineSelectionChanged() != 0) {
            const int row =
                (app->MouseX() -
                 app->SelectionBoxAnchorX() - 6) /
                kRowPitch;  // v15 == v257 (magic /13 = truncating)
            app->SceneModified() = 1;
            if (app->raw<std::uint8_t>(kByteOptflag0) == 0) {
                // ---- bone-edit mode (0x445887..0x446065) ------------------
                unsigned char* model = CurrentModel(app);
                mdl::ModelRecord& record = *mdl::Mdl(model);
                // bone list: 28-byte records, idx > 0 (0x44588F..0x445B30)
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::ModelBone)),
                          app->TimelineSelectionCount(TimelineSelectionBand::ModelBone),
                          mdl::DisplayKeys(model), row, -1,
                          &record.maxFrame,
                          static_cast<int>(mdl::kDisplayKeyCapacity));
                // morph list: 20-byte records, idx >= model count
                // (0x445B38..0x445D90)
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::ModelMorph)),
                          app->TimelineSelectionCount(
                              TimelineSelectionBand::ModelMorph),
                          mdl::MorphKeys(model), row,
                          static_cast<std::int32_t>(record.morphCount),
                          &record.maxFrame,
                          static_cast<int>(mdl::kMorphKeyCapacity));
                // IK list: 60-byte records, idx >= model count
                // (0x445D9E..0x446040)
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::ModelIk)),
                          app->TimelineSelectionCount(TimelineSelectionBand::ModelIk),
                          mdl::BoneKeys(model), row,
                          static_cast<std::int32_t>(record.boneCount),
                          &record.maxFrame,
                          static_cast<int>(mdl::kBoneKeyCapacity));
                PanelPaint(app);  // 0x446044
                // 0x446065: original __thiscall(this=model, dword0x980,
                // dword0xA0CC4).
                Sub4B4260(model,
                          app->raw<std::int32_t>(kDword980),
                          app->raw<std::int32_t>(0xA0CC4));
            } else {
                // ---- display mode (0x444F8A..0x44587D) --------------------
                // four fixed band lists (84/40/24/36-byte records)
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::Camera)),
                          app->TimelineSelectionCount(TimelineSelectionBand::Camera),
                          app->CameraKeys(),
                          row, -1, nullptr);
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::Light)),
                          app->TimelineSelectionCount(TimelineSelectionBand::Light),
                          app->LightKeys(),
                          row, -1, nullptr);
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::SelfShadow)),
                          app->TimelineSelectionCount(TimelineSelectionBand::SelfShadow),
                          app->ShadowKeys(),
                          row, -1, nullptr);
                RemapList(app,
                          reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                              TimelineSelectionBand::Gravity)),
                          app->TimelineSelectionCount(TimelineSelectionBand::Gravity),
                          app->GravityKeys(),
                          row, -1, nullptr);
                RemapAccList(app,
                             reinterpret_cast<const int*>(app->TimelineSelectionRecords(
                                 TimelineSelectionBand::Accessory)),
                             app->TimelineSelectionCount(
                                 TimelineSelectionBand::Accessory), row);
                PanelPaint(app);   // 0x445837
                Sub42E640(app);    // 0x44583E
                Sub411070(app);    // 0x445845
                Sub411B90(app);    // 0x44584C
                Sub412330(app);    // 0x445853
                // per-slot refresh for every set flag (0x445858..0x445879)
                for (int i = 0; i < 255; ++i) {
                    if (app->ObjectSlot(i) != nullptr)
                        Sub413120(app, i);
                }
                Sub4134E0(app);    // 0x44587D
            }
        } else if (app->SelectionBoxDragging() != 0) {
            // ---- 5. hover highlight (0x44606F..0x446A2C) ------------------
            // hover row/column range from the anchor (this+0xA018C/0xA0190)
            int xEnd = app->MouseX();
            int xStart;
            if (xEnd <= app->SelectionBoxAnchorX() + 6) {
                xStart = xEnd;
                xEnd = app->SelectionBoxAnchorX() + 6;
            } else {
                xStart = app->SelectionBoxAnchorX() + 6;
            }
            int yTop, yBottom;
            if (app->MouseY() <=
                app->SelectionBoxAnchorY() + 145) {
                yTop = app->MouseY();
                yBottom = app->SelectionBoxAnchorY() + 145;
            } else {
                yTop = app->SelectionBoxAnchorY() + 145;
                yBottom = app->MouseY();
            }
            int rowStart = (xStart - kGridX0) / kRowPitch;  // 0x4460BC
            if (xStart - kRowPitch * rowStart - kGridX0 <= 6)
                rowStart -= 1;
            if (rowStart < 0)
                rowStart = 0;
            int rowEnd = (xEnd - kGridX0) / kRowPitch;      // 0x4460F8
            if (xEnd - kRowPitch * rowEnd - kGridX0 > 6)
                rowEnd += 1;
            if (rowEnd > app->SidebarWidth() - 22)
                rowEnd = app->SidebarWidth() - 22;
            int colStart = (yTop - kColY0) / kColPitch;     // 0x446135
            if (yTop - kColPitch * colStart - kColY0 <= 7)
                colStart -= 1;
            if (colStart < 0)
                colStart = 0;
            int colEnd = (yBottom - kColY0) / kColPitch;    // 0x44617B
            if (yBottom - kColPitch * colEnd - kColY0 > 7)
                colEnd += 1;

            const unsigned char flag = HoverFlag(app);
            if (app->raw<std::uint8_t>(kByteOptflag0) != 0) {
                // ---- display-mode hover (0x4461B3) ------------------------
                if (!app->ShiftModifierActive()) {
                    // clear band visibility flags for all 200 rows
                    // (0x4461CE..0x446312)
                    auto* cameraKeys = app->CameraKeys();
                    auto* lightKeys = app->LightKeys();
                    auto* shadowKeys = app->ShadowKeys();
                    auto* gravityKeys = app->GravityKeys();
                    int* bandMaps =
                        reinterpret_cast<int*>(app->at(kMapBand1));
                    int* accWalk =
                        reinterpret_cast<int*>(app->at(kMapAcc) + 4);
                    for (int row_i = 0; row_i < 200; ++row_i) {
                        const int b0 = bandMaps[-200];  // band0 map
                        if (b0 >= 0)
                            cameraKeys[b0].selected = 0;
                        const int b1 = bandMaps[0];     // band1 map
                        if (b1 >= 0)
                            lightKeys[b1].selected = 0;
                        const int b2 = bandMaps[200];   // band2 map
                        if (b2 >= 0)
                            shadowKeys[b2].selected = 0;
                        const int b3 = bandMaps[400];   // band3 map
                        if (b3 >= 0)
                            gravityKeys[b3].selected = 0;
                        // accessory map + slot array, 40 groups of 5
                        // (0x446242..0x4462FC); accWalk keeps walking the
                        // whole map across rows, slotWalk restarts per row.
                        int* slotWalk =
                            reinterpret_cast<int*>(app->at(kSlotArr) + 4);
                        for (int g = 0; g < 40; ++g, slotWalk += 5, accWalk += 5) {
                            const int a0 = accWalk[-1];
                            if (a0 >= 0) {
                                const int s0 = slotWalk[-1];
                                if (s0 >= 0)
                                    SetAccessorySelected(app, s0, a0, 0);
                            }
                            const int a1 = accWalk[0];
                            if (a1 >= 0 && slotWalk[0] >= 0)
                                SetAccessorySelected(app, slotWalk[0], a1, 0);
                            const int a2 = accWalk[1];
                            if (a2 >= 0) {
                                const int s2 = slotWalk[1];
                                if (s2 >= 0)
                                    SetAccessorySelected(app, s2, a2, 0);
                            }
                            const int a3 = accWalk[2];
                            if (a3 >= 0) {
                                const int s3 = slotWalk[2];
                                if (s3 >= 0)
                                    SetAccessorySelected(app, s3, a3, 0);
                            }
                            const int a4 = accWalk[3];
                            if (a4 >= 0) {
                                const int s4 = slotWalk[3];
                                if (s4 >= 0)
                                    SetAccessorySelected(app, s4, a4, 0);
                            }
                        }
                        ++bandMaps;
                    }
                }
                // hovered-row highlight for the four fixed bands
                // (0x446342..0x44649F)
                if (yBottom >= 160 && yTop <= 174 && rowStart < rowEnd) {
                    auto* keys = app->CameraKeys();
                    int* m = reinterpret_cast<int*>(app->at(kMapBand0)) + rowStart;
                    int n = rowEnd - rowStart;
                    do {
                        if (*m >= 0)
                            keys[*m].selected = flag;
                        ++m;
                    } while (--n);
                }
                if (yBottom >= 174 && yTop <= 188 && rowStart < rowEnd) {
                    auto* keys = app->LightKeys();
                    int* m = reinterpret_cast<int*>(app->at(kMapBand1)) + rowStart;
                    int n = rowEnd - rowStart;
                    do {
                        if (*m >= 0)
                            keys[*m].selected = flag;
                        ++m;
                    } while (--n);
                }
                if (yBottom >= 188 && yTop <= 202 && rowStart < rowEnd) {
                    auto* keys = app->ShadowKeys();
                    int* m = reinterpret_cast<int*>(app->at(kMapBand2)) + rowStart;
                    int n = rowEnd - rowStart;
                    do {
                        if (*m >= 0)
                            keys[*m].selected = flag;
                        ++m;
                    } while (--n);
                }
                if (yBottom >= 202 && yTop <= 216 && rowStart < rowEnd) {
                    auto* keys = app->GravityKeys();
                    int* m = reinterpret_cast<int*>(app->at(kMapBand3)) + rowStart;
                    int n = rowEnd - rowStart;
                    do {
                        if (*m >= 0)
                            keys[*m].selected = flag;
                        ++m;
                    } while (--n);
                }
                // accessory grid rows x columns (0x4464A1..0x446534)
                if (rowStart < rowEnd) {
                    int* acc = reinterpret_cast<int*>(app->at(kMapAcc)) +
                               (colStart + 200 * rowStart);
                    int rows = rowEnd - rowStart;
                    do {
                        if (colStart < colEnd) {
                            int* slots =
                                reinterpret_cast<int*>(app->at(kSlotArr)) + colStart;
                            int cols = colEnd - colStart;
                            do {
                                if (*acc >= 0 && *slots >= 0)
                                    SetAccessorySelected(
                                        app, *slots, *acc, flag);
                                ++acc;
                                ++slots;
                            } while (--cols);
                        }
                        acc += 200;
                    } while (--rows);
                }
            } else {
                // ---- bone-edit mode hover (0x44653B) ----------------------
                unsigned char* model = CurrentModel(app);
                mdl::DisplayKey* displayKeys = mdl::DisplayKeys(model);
                mdl::MorphKey* morphKeys = mdl::MorphKeys(model);
                mdl::BoneKey* boneKeys = mdl::BoneKeys(model);
                if (!app->ShiftModifierActive()) {
                    // clear bone/morph/IK visibility flags for all 200 rows
                    // (0x44653F..0x4467F0)
                    displayKeys[0].allocated = 0;
                    boneKeys[0].allocated = 0;
                    int* map = reinterpret_cast<int*>(app->at(kMapMorph));
                    for (int row_i = 0; row_i < 200; ++row_i) {
                        for (int g = 0; g < 40; ++g, map += 5) {
                            const int b0 = map[40000];  // bone map
                            if (b0 > 0)
                                displayKeys[b0].allocated = 0;
                            const int m0 = map[0];      // morph map
                            if (m0 > 0)
                                morphKeys[m0].allocated = 0;
                            const int i0 = map[-40000]; // IK map
                            if (i0 > 0)
                                boneKeys[i0].allocated = 0;
                            const int b1 = map[40001];
                            if (b1 > 0)
                                displayKeys[b1].allocated = 0;
                            const int m1 = map[1];
                            if (m1 > 0)
                                morphKeys[m1].allocated = 0;
                            const int i1 = map[-39999];
                            if (i1 > 0)
                                boneKeys[i1].allocated = 0;
                            const int b2 = map[40002];
                            if (b2 > 0)
                                displayKeys[b2].allocated = 0;
                            const int m2 = map[2];
                            if (m2 > 0)
                                morphKeys[m2].allocated = 0;
                            const int i2 = map[-39998];
                            if (i2 > 0)
                                boneKeys[i2].allocated = 0;
                            const int b3 = map[40003];
                            if (b3 > 0)
                                displayKeys[b3].allocated = 0;
                            const int m3 = map[3];
                            if (m3 > 0)
                                morphKeys[m3].allocated = 0;
                            const int i3 = map[-39997];
                            if (i3 > 0)
                                boneKeys[i3].allocated = 0;
                            const int b4 = map[40004];
                            if (b4 > 0)
                                displayKeys[b4].allocated = 0;
                            const int m4 = map[4];
                            if (m4 > 0)
                                morphKeys[m4].allocated = 0;
                            const int i4 = map[-39996];
                            if (i4 > 0)
                                boneKeys[i4].allocated = 0;
                        }
                    }
                }
                // hovered-row highlight over the morph map grid
                // (0x446815..0x446A0F)
                int colStartB = (yTop - kColY0Bone) / kColPitch;
                if (yTop - kColPitch * colStartB - kColY0Bone <= 7)
                    colStartB -= 1;
                if (colStartB < 0)
                    colStartB = 0;
                int colEndB = (yBottom - kColY0Bone) / kColPitch;
                if (yBottom - kColPitch * colEndB - kColY0Bone > 7)
                    colEndB += 1;
                if (rowStart < rowEnd) {
                    int* map = reinterpret_cast<int*>(app->at(kMapMorph)) +
                               (colStartB + 200 * rowStart);
                    int rows = rowEnd - rowStart;
                    do {
                        if (colStartB < colEndB) {
                            int cols = colEndB - colStartB;
                            do {
                                const int bv = map[40000];  // bone map
                                if (bv <= 0) {
                                    if (bv == -10)
                                        displayKeys[0].allocated = flag;
                                } else {
                                    displayKeys[bv].allocated = flag;
                                }
                                const int mv = map[0];      // morph map
                                if (mv <= 0) {
                                    if (mv == -10)
                                        morphKeys[0].allocated = flag;
                                } else {
                                    morphKeys[mv].allocated = flag;
                                }
                                const int iv = map[-40000]; // IK map
                                if (iv <= 0) {
                                    if (iv == -10)
                                        boneKeys[0].allocated = flag;
                                } else {
                                    boneKeys[iv].allocated = flag;
                                }
                                ++map;
                            } while (--cols);
                        }
                        map += 200;
                    } while (--rows);
                }
                // hover rect (0x446A13..0x446A25)
                app->TimelineRangeFirstOffset() = rowStart;
                app->TimelineRangeLastOffset() = rowEnd;
                app->TimelineRangeFirstBase() = colStartB;
                app->TimelineRangeLastBase() = colEndB;
            }
            PanelPaint(app);  // 0x446A2D
            // rect reset (0x446A32..0x446A46)
            app->TimelineRangeFirstOffset() = 0;
            app->TimelineRangeLastOffset() = 0;
            app->TimelineRangeFirstBase() = 0;
            app->TimelineRangeLastBase() = 0;
        }
    }

    // --- 6. physics-enabled hover helper (0x446A4C) -------------------------
    if (app->PendingTimelineSelectionRow() != TimelineSelectionRow::None)
        Sub416280(app);
}

}  // namespace mikudancestudio
