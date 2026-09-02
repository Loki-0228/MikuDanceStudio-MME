// ===========================================================================
// VA 0x00414610 - PanelPaint  (original: sub_414610, ~0x880 bytes)
// ===========================================================================
// Right-panel repaint (called from the scroll handlers and the editor click
// path).  Everything draws onto the offscreen panel DC (this+0x2D4) which
// WM_PAINT (0x47C0A0) later blits to the window at (6,145).
//
// Draw flow (original instruction ranges in parentheses):
//   1. 0x414638  compat DC over the panel bitmap (this+0x2DC); SRCCOPY from
//      the bitmap at (91,0) size (sidebar, listH) erases stale rows of the
//      list column, then DeleteDC.
//   2. 0x414696 / 0x414757  two band-background passes: PS_SOLID 1px pen +
//      solid brush of colour this+0xA063C resp. this+0xA0640; 100 bands,
//      28 px apart, starting y = 30 resp. y = 16 while y < 2816; a band is
//      painted only when its visibility byte (this+0xA04F8, one byte per
//      band, odd bands at +1) is set; Rectangle(91, y, listW, y+13) with
//      listW = (sub1d574)+0x1D4E4.  Pens/brushes restored via the handles
//      saved at entry (oldPen/oldBrush) and deleted in the original order.
//   3. 0x414812  vertical grid lines: rows = (sidebar-84)/13; each row
//      (x = 100+13i) gets a 1 px line from y=16 to listH ((sub1d574)+
//      0x1D4E8) and, every 5th row, a "%-4d" label (DrawGlyph 0x40E290,
//      size 12, at (x-3, 3)).  The selected row (scroll+idx == this+0x980)
//      uses colour this+0xA0650, multiples of 5 this+0xA0654, others
//      this+0xA0658 (unsigned % 5, as the original div).
//   4. 0x414986  zeroes the three 200-int hit maps (this+0x984 / 0x27A84 /
//      0x4EB84) and fills the four fixed-band maps (this+0x75C84..0x765E4)
//      with -1 plus 200 accessory rows at this+0x76904 with 0xFF.
//   5. 0x414A1F  icon DC over this+0x2F0 (11x11 icon sheet, two rows);
//      band rows are SRCAND mask blits from source (44, row) plus SRCPAINT
//      from (22/0, row); negative links use source column 33.
//   6. 0x414A29  display mode (optflag0 @0x2F8 != 0): four fixed band lists
//      (this+0x374/0x378/0x37C/0x380, record strides 84/40/24/36, band
//      y = 17/31/45/59, flag byte +72/+36/+20/+33) plus the accessory bands
//      (this+0x9D9D0 index array -> pointers at this+0x384, stride 60,
//      y = 73+14k while y < 2873); else bone-edit mode: bone list
//      (model+0x26E8, stride 28, flag +20, link = model+0x2DBC int), morph
//      list (model+0x26E4, stride 20, flag +16, links model+0x2DB8[head],
//      count model+0x2D80) and IK list (model+0x26E0, stride 60, flags +56/
//      +57, links model+0x2DB4[head], count model+0x2D84).  The mask byte
//      used for the IK icon rows is *(*(model+0x26BC) + sizeof(mikudancestudio::mdl::BoneRecord) *head + 0x1EC).
//      The per-record "visibility" byte (+16/+56) is re-derived from the
//      drag-rect (this+0xA05C0..0xA05CC, byte this+0xA05D0) and mode
//      words this+0x24 / this+0xC0 whenever the shared mask
//      (byte this+0xA0189 && dword this+0xA03EB == 0) and a negative link
//      apply.  Row maps: bone this+0x4EB84, morph this+0x27A84, IK
//      this+0x984; sentinels 0 (cleared), -1, -2, -10 (index 0).
//   7. 0x415C6B  SCROLLINFO at this+0x960 for the list scrollbar (control
//      428, SB_CTL): cbSize 28 / fMask SIF_ALL / nMin 0; nMax =
//      max(pos+rows, <listlen>+20, selected+rows/2) - 1, where <listlen>
//      is this+0x9E16C in display mode and model+0x31B0 (+ the extra
//      this+0x9E16C clamp) in bone-edit mode; nPage = rows; nPos = pos.
//   8. 0x415D0E  when dword this+0xA03EB == 0 && byte this+0xA0189 != 0:
//      0x969696 selection box from (this+0xA018C, this+0xA0190) to the
//      clamped corner ((this+4)-6, (this+8)-145); then DeleteDC(icons),
//      PostViewRefresh (0x40D130), and InvalidateRect{left 6, top 145,
//      right sidebar-19, bottom bottom-248} on the main window.
//
// GDI object lifetimes, loop bounds and coordinate constants mirror the
// original instruction stream one-to-one; the repeated per-record drawing
// of the four fixed bands is factored into DrawBandList() (behaviour
// identical, address range annotated).
//
// Reference: ../translated/MikuMikuDance/fcn_00414610.cpp
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdio>
#include <cstdint>
#include <cstring>

#include "mikudancestudio/accessory_layout.hpp"
#include "mikudancestudio/global_key_layout.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/model.hpp"

namespace mikudancestudio {
namespace {

// --- layout / draw constants from the original instruction stream --------
constexpr int kListLeft = 91;     // 0x5B  list column x
constexpr int kIconLeft = 95;     // 0x5F  marker x; centred on grid x=100
constexpr int kRowH = 13;         // 0x0D  row pitch (grid lines, bands)
constexpr int kBandH = 14;        // 0x0E  band pitch (icon rows)
constexpr int kBandLastY = 2816;  // 0xB00 band loops run while y < 2816
constexpr int kGridX0 = 100;      // 0x64  first grid-line x
constexpr int kGridTopY = 16;     // 0x10  grid lines start at y = 16
constexpr int kIcon = 11;         // 0x0B  icon square
constexpr int kAccY0 = 73;        // 0x49  first accessory band y
constexpr int kAccLastY = 2873;   //      accessory bands run while y < 2873
constexpr int kSheetMaskX = 44;   // 0x2C  SRCAND source column
constexpr int kSheetOnX = 22;     // 0x16  SRCPAINT "enabled" column
constexpr int kSheetNegX = 33;    // 0x21  SRCPAINT "reverse link" column
constexpr int kSheetAltX = 11;    // 0x0B  SRCPAINT alternate column
constexpr int kSheetOffX = 0;     //      SRCPAINT "disabled" column

// --- row-hit map bases (200 ints per band/row) ---------------------------
constexpr std::size_t kMapIk = 0x984;       // IK map (bone-edit mode)
constexpr std::size_t kMapMorph = 0x27A84;  // morph map (bone-edit mode)
constexpr std::size_t kMapBone = 0x4EB84;   // bone map (bone-edit mode)
constexpr std::size_t kMapBand0 = 0x75C84;  // 84-byte band map (y=17)
constexpr std::size_t kMapBand1 = 0x75FA4;  // 40-byte band map (y=31)
constexpr std::size_t kMapBand2 = 0x762C4;  // 24-byte band map (y=45)
constexpr std::size_t kMapBand3 = 0x765E4;  // 36-byte band map (y=59)
constexpr std::size_t kMapAcc = 0x76904;    // accessory band maps (y=73+14k)

// --- model object offsets (bone-edit mode) --------------------------------
constexpr std::size_t kModelMorphCnt = 0x2D80;  // morph list count
constexpr std::size_t kModelIkCnt = 0x2D84;     // IK list count
constexpr std::size_t kModelIkLink = 0x2DB4;    // IK link table (int[count])
constexpr std::size_t kModelMorphLink = 0x2DB8; // morph link table
constexpr std::size_t kModelBoneLink = 0x2DBC;  // bone link value (int)
constexpr std::size_t kModelListLen = 0x31B0;   // list length (scroll nMax)
constexpr std::size_t kModelZeroIk = 0x3900;    // IK index allowed a zero link

// --- app offsets not yet named in offsets.hpp -----------------------------
constexpr std::size_t kHbitmapPanel = 0x2DC;    // panel back-store bitmap
constexpr std::size_t kHbitmapSheet = 0x2F0;    // 11x11 icon sheet bitmap
constexpr std::size_t kPtrBand0List = 0x374;    // 84-byte band list ptr
constexpr std::size_t kPtrBand1List = 0x378;    // 40-byte band list ptr
constexpr std::size_t kPtrBand2List = 0x37C;    // 24-byte band list ptr
constexpr std::size_t kPtrBand3List = 0x380;    // 36-byte band list ptr
constexpr std::size_t kScrollInfo2 = 0x960;     // SCROLLINFO for control 428
constexpr std::size_t kIdxAccBands = 0x9DA50;   // accessory band index array
// (0x414DB1 `lea eax,[esi+9DA50h]`: the SAME 200-int array PostLanguageSweep
// memsets to -1 before the joint walk - dual use as the accessory band
// index source.  An earlier revision read 0x9D9D0, which stays zero, so the
// idx<0 break never fired and the band loop dereferenced a null accessory
// slot at startup.)
// (The 0x1D574 render wrapper's "list width/height" reads below are its
// screenWidth/screenHeight fields, sub+0x1D4E4/0x1D4E8.)

// Current model slot for the slot-order index (this+0x910) - 0x414F6F.
unsigned char* CurrentModel(MMDApp* app) {
    return app->SelectedModel();
}

// One fixed global timeline list.  Per-record body of
// 0x414A51 / 0x414B30 / 0x414C10 / 0x414CF0; the accessory band loop
// (0x414E00) keeps its own body because its map is laid out per row.
template <typename Key>
void DrawBandList(MMDApp* app, HDC panel, HDC icons, Key* keys,
                  int bandY, std::size_t mapBase, int rowCount) {
    // TEMP(build fix, physics session): the original never sees these
    // four lists null because 0x466D20 (reached via the window proc on
    // WM_CREATE) allocates them before LocalizeUI; that path is not
    // ported yet, so guard until it lands.
    if (keys == nullptr)
        return;
    const std::int32_t scroll = app->state.timelineStartFrame;
    const std::int32_t limit = scroll + rowCount;
    if (static_cast<std::int32_t>(keys[0].frame) >= limit)
        return;
    int record = 0;
    for (;;) {
        const Key& key = keys[record];
        const int row = static_cast<std::int32_t>(key.frame) - scroll;
        if (row >= 0) {
            const int x = kRowH * row + kIconLeft;
            BitBlt(panel, x, bandY, kIcon, kIcon, icons, kSheetMaskX, 0, SRCAND);
            BitBlt(panel, x, bandY, kIcon, kIcon, icons,
                   key.selected ? kSheetOnX : kSheetOffX, 0, SRCPAINT);
            app->raw<std::int32_t>(mapBase + 4 * static_cast<unsigned>(row)) = record;
        }
        const int next = static_cast<int>(key.next);
        record = next;
        if (next == 0 ||
            static_cast<std::int32_t>(keys[next].frame) >= limit)
            return;
    }
}

}  // namespace

void PanelPaint(MMDApp* app) {
    const std::int32_t sidebar = app->SidebarWidth();
    D3DRenderer* sub = app->Renderer();
    const std::int32_t listW = sub->screenWidth;   // sub+0x1D4E4 "list width"
    const std::int32_t listH = sub->screenHeight;  // sub+0x1D4E8 "list height"
    HDC panel = app->PanelDC();
    const std::int32_t scroll = app->state.timelineStartFrame;

    // --- 1. restore list region from the panel bitmap (0x414638) ----------
    HDC dc = CreateCompatibleDC(nullptr);
    SelectObject(dc, app->raw<HGDIOBJ>(kHbitmapPanel));
    BitBlt(panel, kListLeft, 0, sidebar, listH, dc, kListLeft, 0, SRCCOPY);
    DeleteDC(dc);

    // --- 2. band backgrounds (0x414696 / 0x414757) ------------------------
    HPEN pen = CreatePen(
        PS_SOLID, 1,
        app->ThemeColor(UiThemeColor::PanelHeader));
    HBRUSH brush = CreateSolidBrush(
        app->ThemeColor(UiThemeColor::PanelHeader));
    HGDIOBJ oldPen = SelectObject(panel, pen);
    HGDIOBJ oldBrush = SelectObject(panel, brush);
    unsigned char* flags = app->state.buf656632 + 1;
    for (int y = 30; y < kBandLastY; y += kBandH * 2, flags += 2) {
        if (*flags)
            Rectangle(panel, kListLeft, y, listW, y + kRowH);
    }
    SelectObject(panel, oldPen);
    DeleteObject(pen);
    SelectObject(panel, oldBrush);
    DeleteObject(brush);

    HPEN pen2 = CreatePen(
        PS_SOLID, 1,
        app->ThemeColor(UiThemeColor::PanelBody));
    HBRUSH brush2 = CreateSolidBrush(
        app->ThemeColor(UiThemeColor::PanelBody));
    SelectObject(panel, pen2);
    SelectObject(panel, brush2);
    flags = app->state.buf656632;
    for (int y = 16; y < kBandLastY; y += kBandH * 2, flags += 2) {
        if (*flags)
            Rectangle(panel, kListLeft, y, listW, y + kRowH);
    }
    SelectObject(panel, oldPen);
    DeleteObject(pen2);
    SelectObject(panel, oldBrush);
    DeleteObject(brush2);

    // --- 3. row labels + grid lines (0x414812) -----------------------------
    const int rows = (sidebar - 84) / kRowH;      // signed, as the original
    if (rows < 0)
        return;
    if (rows > 0) {
        char label[0x100];
        char sel[0x100];
        int x = kGridX0;
        for (int idx = 0; idx < rows; ++idx) {
            const std::int32_t n = scroll + idx;
            COLORREF color;
            if (n == app->state.currentFrame) {
                sprintf_s(sel, sizeof(sel), "%-4d", n);
                DrawGlyph(app, sel, panel, 12, x - 3, 3,
                          app->state.themeColors[30],
                          app->raw<std::uint8_t>(offsets::kDwordCol656976 + 1),
                          app->raw<std::uint8_t>(offsets::kDwordCol656976 + 2), 1);
                color = static_cast<COLORREF>(
                    app->state.themeColors[30]);
            } else if (static_cast<std::uint32_t>(n) % 5 != 0) {
                color = static_cast<COLORREF>(
                    app->state.themeColors[32]);
            } else {
                sprintf_s(label, sizeof(label), "%-4d", n);
                DrawGlyph(app, label, panel, 12, x - 3, 3,
                          app->state.themeColors[33],
                          reinterpret_cast<const unsigned char*>(&app->state.themeColors[33])[1],
                          reinterpret_cast<const unsigned char*>(&app->state.themeColors[33])[2], 1);
                color = static_cast<COLORREF>(
                    app->state.themeColors[31]);
            }
            HPEN line = CreatePen(PS_SOLID, 1, color);
            SelectObject(panel, line);
            MoveToEx(panel, x, kGridTopY, nullptr);
            LineTo(panel, x, listH);
            SelectObject(panel, oldPen);
            DeleteObject(line);
            x += kRowH;
        }
    }

    // --- 4. clear the hit maps (0x414986 / 0x4149CB) -----------------------
    // Each hit map has exactly 200 entries.  The x86 routine clears the
    // corresponding element in the three independent arrays; deriving the
    // other arrays through a raw +/- 40000-int displacement works only for
    // the old 32-bit state blob.  In the x64 layout that walk crosses
    // unrelated state (including the accessory-track pointer table).
    if (rows > 0) {
        std::int32_t* const ikMap =
            reinterpret_cast<std::int32_t*>(app->at(kMapIk));
        std::int32_t* const morphMap =
            reinterpret_cast<std::int32_t*>(app->at(kMapMorph));
        std::int32_t* const boneMap =
            reinterpret_cast<std::int32_t*>(app->at(kMapBone));
        for (int index = 0; index < 200; ++index) {
            ikMap[index] = 0;
            morphMap[index] = 0;
            boneMap[index] = 0;
        }
    }
    {
        std::int32_t* const band0Map =
            reinterpret_cast<std::int32_t*>(app->at(kMapBand0));
        std::int32_t* const band1Map =
            reinterpret_cast<std::int32_t*>(app->at(kMapBand1));
        std::int32_t* const band2Map =
            reinterpret_cast<std::int32_t*>(app->at(kMapBand2));
        std::int32_t* const band3Map =
            reinterpret_cast<std::int32_t*>(app->at(kMapBand3));
        unsigned char* acc = app->at(kMapAcc);
        for (int row = 0; row < 200; ++row) {
            band0Map[row] = -1;
            band1Map[row] = -1;
            band2Map[row] = -1;
            band3Map[row] = -1;
            memset(acc, 0xFF, 0x320);
            acc += 800;
        }
    }

    // --- 5. icon DC over the icon sheet (0x414A1F) -------------------------
    HDC icons = CreateCompatibleDC(nullptr);
    SelectObject(icons, app->raw<HGDIOBJ>(kHbitmapSheet));

    // Shared scrollbar + selection-box + refresh tail (0x415C6B..0x415E64).
    // a/b are the two "list length + 20" nMax candidates (second one is the
    // this+0x9E16C clamp applied only in bone-edit mode; both branches pass
    // it, which is a no-op difference for display mode since a == b there).
    auto FinishPanel = [&](std::uint32_t a, std::uint32_t b) {
        const std::uint32_t pos = static_cast<std::uint32_t>(scroll);
        // The reference keeps this transient SCROLLINFO in an x86 scratch
        // region at app+0x960.  That byte address is not a state field in
        // the x64 layout (it overlaps accessory-track pointers), so make
        // the temporary object explicit instead of preserving the alias.
        SCROLLINFO scrollInfo{};
        scrollInfo.cbSize = sizeof(scrollInfo);
        scrollInfo.fMask = SIF_ALL;
        scrollInfo.nMin = 0;
        std::uint32_t nMax = pos + static_cast<std::uint32_t>(rows);
        if (nMax < a) nMax = a;
        if (nMax < b) nMax = b;
        const std::uint32_t mid =
            static_cast<std::uint32_t>(app->state.currentFrame) +
            static_cast<std::uint32_t>(rows / 2);
        if (nMax < mid) nMax = mid;
        scrollInfo.nMax = static_cast<int>(nMax) - 1;
        scrollInfo.nPage = static_cast<DWORD>(rows);
        scrollInfo.nPos = static_cast<int>(pos);
        SetScrollInfo(GetDlgItem(static_cast<HWND>(app->Hwnd()), 428), SB_CTL,
                      &scrollInfo,
                      TRUE);

        RECT rc;
        GetClientRect(static_cast<HWND>(app->Hwnd()), &rc);
        if (app->SelectionBoxDragging() != 0 &&
            app->TimelineSelectionChanged() == 0) {
            HPEN box = CreatePen(PS_SOLID, 1, 0x969696);
            SelectObject(panel, box);
            int x2 = app->MouseX() - 6;
            int y2 = app->MouseY() - 145;
            if (x2 < kListLeft) x2 = kListLeft;
            if (y2 < 17) y2 = 17;
            if (x2 > sidebar - 28) x2 = sidebar - 28;
            if (y2 > rc.bottom - 396) y2 = rc.bottom - 396;
            const int ax = app->SelectionBoxAnchorX();
            const int ay = app->SelectionBoxAnchorY();
            MoveToEx(panel, ax, ay, nullptr);
            LineTo(panel, ax, y2);
            LineTo(panel, x2, y2);
            LineTo(panel, x2, ay);
            LineTo(panel, ax, ay);
            SelectObject(panel, oldPen);
            DeleteObject(box);
        }
        DeleteDC(icons);
        PostViewRefresh(app);
        rc.bottom -= 248;
        rc.left = 6;
        rc.top = 145;
        rc.right = sidebar - 19;
        InvalidateRect(static_cast<HWND>(app->Hwnd()), &rc, FALSE);
    };

    if (app->state.optflag0 != 0) {
        // --- 6a. display mode: four fixed bands + accessory bands ----------
        const std::int32_t limit = scroll + rows;
        DrawBandList(app, panel, icons,
                     app->raw<mdl::CameraKey*>(kPtrBand0List),
                     17, kMapBand0, rows);
        DrawBandList(app, panel, icons,
                     app->raw<mdl::LightKey*>(kPtrBand1List),
                     31, kMapBand1, rows);
        DrawBandList(app, panel, icons,
                     app->raw<mdl::SelfShadowKey*>(kPtrBand2List),
                     45, kMapBand2, rows);
        DrawBandList(app, panel, icons,
                     app->raw<mdl::GravityKey*>(kPtrBand3List),
                     59, kMapBand3, rows);
        // accessory bands (0x414DC7..0x414F17)
        const std::int32_t* idxArr =
            &app->raw<std::int32_t>(kIdxAccBands);
        int band = 0;
        int bandY = kAccY0;
        while (bandY < kAccLastY) {
            const int idx = idxArr[band];
            if (idx < 0)
                break;
            mdl::AccessoryKey* list = app->AccessoryKeys(idx);
            if (list == nullptr)  // TEMP(build fix): see DrawBandList note
                break;
            int record = 0;
            if (static_cast<std::int32_t>(list[0].frame) < limit) {
                for (;;) {
                    const mdl::AccessoryKey& key = list[record];
                    const int row = static_cast<std::int32_t>(key.frame) - scroll;
                    if (row >= 0) {
                        const int x = kRowH * row + kIconLeft;
                        BitBlt(panel, x, bandY, kIcon, kIcon, icons, kSheetMaskX, 0,
                               SRCAND);
                        BitBlt(panel, x, bandY, kIcon, kIcon, icons,
                               key.selected ? kSheetOnX : kSheetOffX,
                               0, SRCPAINT);
                        app->raw<std::int32_t>(
                            kMapAcc + 4 * static_cast<unsigned>(band + 200 * row)) =
                            record;
                    }
                    const int next = static_cast<int>(key.next);
                    record = next;
                    if (next == 0 ||
                        static_cast<std::int32_t>(list[next].frame) >= limit)
                        break;
                }
            }
            ++band;
            bandY += kBandH;
        }
        const std::uint32_t len =
            static_cast<std::uint32_t>(app->LastRegisteredFrame()) + 20;
        FinishPanel(len, len);
    } else {
        // --- 6b. bone-edit mode: bone / morph / IK lists -------------------
        unsigned char* model = CurrentModel(app);
        const std::int32_t limit = scroll + rows;

        // model display/IK list (model+0x26E8) - 0x414F6F..0x415122
        mdl::DisplayKey* displayKeys = mdl::DisplayKeys(model);
        if (static_cast<std::int32_t>(displayKeys[0].frame) < limit) {
            int record = 0;
            for (;;) {
                const mdl::DisplayKey& key = displayKeys[record];
                const int row =
                    static_cast<std::int32_t>(key.frame) - scroll;
                if (row >= 0) {
                    const int link = *reinterpret_cast<std::int32_t*>(model + kModelBoneLink);
                    if (link != 0) {
                        const int x = kRowH * row + kIconLeft;
                        const int y = kBandH * link + 17;
                        BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX, 0, SRCAND);
                        BitBlt(panel, x, y, kIcon, kIcon, icons,
                               key.allocated ? kSheetOnX : kSheetOffX,
                               0, SRCPAINT);
                        reinterpret_cast<std::int32_t*>(app->at(kMapBone))
                            [link + 200 * row] = record != 0 ? record : -10;
                    }
                }
                const int next = static_cast<int>(key.next);
                record = next;
                if (next == 0 ||
                    static_cast<std::int32_t>(displayKeys[next].frame) >= limit)
                    break;
            }
        }

        // morph list (model+0x26E4, 20-byte records) - 0x415129..0x41557A
        if (*reinterpret_cast<std::int32_t*>(model + kModelMorphCnt) > 0) {
            mdl::MorphKey* morphList = mdl::MorphKeys(model);
            std::int32_t* morphLinks =
                *reinterpret_cast<std::int32_t**>(model + kModelMorphLink);
            std::int32_t* mapMorph =
                reinterpret_cast<std::int32_t*>(app->at(kMapMorph));
            const std::int32_t morphCnt =
                *reinterpret_cast<std::int32_t*>(model + kModelMorphCnt);
            for (int head = 0; head < morphCnt; ++head) {
                if (static_cast<std::int32_t>(morphList[head].frame) >= limit)
                    continue;
                int rec = head;
                for (;;) {
                    mdl::MorphKey& key = morphList[rec];
                    const int row =
                        static_cast<std::int32_t>(key.frame) - scroll;
                    if (row >= 0) {
                        const int link = morphLinks[head];
                        if (link != 0) {
                            // visibility flag (+16) maintenance (0x4151B7)
                            if (app->SelectionBoxDragging() != 0 &&
                                app->TimelineSelectionChanged() == 0 &&
                                link < 0) {
                                const int neg = -link;
                                if (app->TimelineRangeFirstOffset() <= row &&
                                    row < app->TimelineRangeLastOffset() &&
                                    neg < app->TimelineRangeLastBase() &&
                                    app->TimelineRangeFirstBase() <= neg) {
                                    key.allocated = static_cast<std::uint8_t>(
                                        !app->CtrlModifierActive());
                                } else if (!app->ShiftModifierActive()) {
                                    key.allocated = 0;
                                }
                            }
                            // row drawing (0x415231..0x4154B1)
                            const int x = kRowH * row + kIconLeft;
                            if (key.allocated != 0) {
                                if (link >= 0) {
                                    const int y = kBandH * link + 17;
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX, 0,
                                           SRCAND);
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetOnX, 0,
                                           SRCPAINT);
                                } else {
                                    const int y = 17 - kBandH * link;
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX, 0,
                                           SRCAND);
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetNegX, 0,
                                           SRCPAINT);
                                    mapMorph[200 * row - link] = -1;
                                }
                            } else if (link >= 0) {
                                if (mapMorph[200 * row + link] >= 0) {
                                    const int y = kBandH * link + 17;
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX, 0,
                                           SRCAND);
                                    BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetOffX, 0,
                                           SRCPAINT);
                                }
                            } else if (mapMorph[200 * row - link] >= 0) {
                                const int y = 17 - kBandH * link;
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX, 0,
                                       SRCAND);
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetAltX, 0,
                                       SRCPAINT);
                                mapMorph[200 * row - link] = -2;
                            }
                            // final map write (0x4154CB)
                            if (link >= 0)
                                mapMorph[200 * row + link] = rec != 0 ? rec : -10;
                        }
                    }
                    const int next = static_cast<int>(key.next);
                    rec = next;
                    if (next == 0 ||
                        static_cast<std::int32_t>(morphList[next].frame) >= limit)
                        break;
                }
            }
        }

        // IK list (model+0x26E0, 60-byte records) - 0x41557A..0x415C6B
        const std::int32_t ikCnt = *reinterpret_cast<std::int32_t*>(model + kModelIkCnt);
        if (ikCnt > 0) {
            mdl::BoneKey* ikList = mdl::BoneKeys(model);
            mdl::BoneRecord* bones = mdl::Bones(model);
            std::int32_t* ikLinks =
                *reinterpret_cast<std::int32_t**>(model + kModelIkLink);
            std::int32_t* mapIk =
                reinterpret_cast<std::int32_t*>(app->at(kMapIk));
            int head = 0;       // chain-head index (x)
            int rec = 0;
            for (;;) {
                if (static_cast<std::int32_t>(ikList[head].frame) < limit) {
                    rec = head;
                    break;
                }
            advanceOuter:
                if (++head >= ikCnt)
                    goto ikDone;
            }
            for (;;) {
                mdl::BoneKey& key = ikList[rec];
                const int row = static_cast<std::int32_t>(key.frame) - scroll;
                if (row >= 0) {
                    const int link = ikLinks[head];
                    if (link != 0 || head == *reinterpret_cast<std::int32_t*>(model + kModelZeroIk)) {
                        // visibility flag (+56) maintenance (0x415631..0x4156CB)
                        if (app->SelectionBoxDragging() != 0 &&
                            app->TimelineSelectionChanged() == 0 &&
                            link < 0) {
                            const int neg = -link;
                            if (neg < app->TimelineRangeLastBase() &&
                                row < app->TimelineRangeLastOffset() &&
                                app->TimelineRangeFirstBase() <= neg &&
                                app->TimelineRangeFirstOffset() <= row) {
                                key.allocated =
                                    app->TimelineRangeApplyEnabled() != 0
                                        ? 0
                                        : static_cast<std::uint8_t>(
                                              !app->CtrlModifierActive());
                            } else if (!app->ShiftModifierActive()) {
                                key.allocated = 0;
                            }
                        }
                        // row drawing (0x4156D1..0x415B98)
                        const bool mask =
                            key.physicsDisabled == 0 && bones[head].f492 != 0;
                        const int x = kRowH * row + kIconLeft;
                        if (key.allocated != 0) {
                            if (link >= 0) {
                                const int y = kBandH * link + 17;
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX,
                                       mask ? kIcon : 0, SRCAND);
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetOnX,
                                       mask ? kIcon : 0, SRCPAINT);
                            } else {
                                const int y = 17 - kBandH * link;
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX,
                                       mask ? kIcon : 0, SRCAND);
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetNegX, 0,
                                       SRCPAINT);
                                mapIk[200 * row - link] = -1;
                            }
                        } else if (link >= 0) {
                            if (mapIk[200 * row + link] >= 0) {
                                const int y = kBandH * link + 17;
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX,
                                       mask ? kIcon : 0, SRCAND);
                                BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetOffX,
                                       mask ? kIcon : 0, SRCPAINT);
                            }
                        } else if (mapIk[200 * row - link] == -1) {
                            const int y = 17 - kBandH * link;
                            BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX,
                                   mask ? kIcon : 0, SRCAND);
                            BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetNegX,
                                   mask ? kIcon : 0, SRCPAINT);
                        } else {
                            const int y = 17 - kBandH * link;
                            BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetMaskX,
                                   mask ? kIcon : 0, SRCAND);
                            // 0x415A35 and 0x415A5C feed the common BitBlt call
                            // with source (11,11) and (11,0), respectively.
                            BitBlt(panel, x, y, kIcon, kIcon, icons, kSheetAltX,
                                   mask ? kIcon : 0, SRCPAINT);
                            mapIk[200 * row - link] = -2;
                        }
                        // final map write (LABEL_156, 0x415B98)
                        if (link >= 0)
                            mapIk[200 * row + link] = rec != 0 ? rec : -10;
                    }
                }
                const int next = static_cast<int>(key.next);
                rec = next;
                if (next == 0 ||
                    static_cast<std::int32_t>(ikList[next].frame) >= limit)
                    goto advanceOuter;
            }
        }
    ikDone:
        const std::uint32_t a =
            static_cast<std::uint32_t>(*reinterpret_cast<std::int32_t*>(model + kModelListLen)) + 20;
        const std::uint32_t b =
            static_cast<std::uint32_t>(app->LastRegisteredFrame()) + 20;
        FinishPanel(a, b);
    }
}

}  // namespace mikudancestudio
