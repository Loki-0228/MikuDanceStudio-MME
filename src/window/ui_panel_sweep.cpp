// ===========================================================================
// VA 0x0042F1E0 - PostLanguageSweep  (original: sub_42F1E0, ~0x1320 bytes)
// ===========================================================================
// Right-panel label-column sweep: shared tail of LocalizeUI (0x441AD0), the
// list-scroll handlers (0x440AC0 / 0x44BB30 / 0x44AEE0), HandleWindowSize
// (0x443300), HandleLButtonDown (0x446A70) and the model-load / command
// paths (0x460430, 0x458F80, 0x47E8A0, 0x46B090).  Everything draws into
// the offscreen panel DC (this+0x2D4) label column x 0..91, which WM_PAINT
// (0x47C0A0) later blits to the window at (6,145).
//
//   1. 0x42F1F2  compat DC over the panel back-store bitmap (this+0x2DC);
//      SRCCOPY of (0,0) size (91, listH @ sub+0x1D4E8) erases stale label
//      text, then DeleteDC; the 200 line-highlight flags (this+0xA04F8)
//      are zeroed.
//   2. 0x42F24C  bone-edit mode (optflag0 @0x2F8 == 0), current model slot
//      (this+0x910 -> slots @0x780): root line = 604-byte-stride name at
//      model+0x26BC indexed by model+0x3900 (+20 for EN), highlighted when
//      the flag byte model+0x2D94[rootIdx] is set.  Link tables
//      model+0x2DB4 / 0x2DB8 are zeroed, the per-line type bytes
//      (model+0x2DC0) cleared and the 20-byte display records at
//      model+0x2E88 reset to -999; record 0 = model+0x3900, iteration
//      counter model+0x31A8 = 1.
//   3. 0x42FA13  display-frame iteration (count @0x26D4, byte): per frame
//      the 101-byte record (JP name @+0, EN name @+50, flag @+100) at
//      model+0x26D0; flag==0 frames list their bones ("+"/"＋" 0x52C95C
//      glyph for frame > 1), flag!=0 frames go to the face (frame 2) /
//      rigid-group sections ("-"/"－" 0x52C958).  Rows draw in the theme
//      normal colour (0xA065C) or highlight (0xA0660) with the flag byte
//      this+0xA04F8[line] set for highlighted rows.  Face records
//      (model+0x26DC, 46B: bone@+40, morph@+42 u16, flag@+44) and rigid
//      groups (model+0x26D8, 46B: bone@+40, rigid@+42 u16) back-link their
//      morph/rigid index to the line number via model+0x2DB8 / 0x2DB4
//      (-line); each drawn line's record value at model+0x2E88+4*line is
//      the rigid index or -1-morph.  Frame 1's head line is stored at
//      model+0x2DBC.
//   4. 0x42F252  display mode (optflag0 != 0): fixed headers camera / light
//      / "s shadow" / gravity at y = 17/31/45/59 (EN literals, JP
//      Shift-JIS カメラ/照 明/セルフ影/重 力 @0x52C980..0x52C964),
//      highlighted per the toggle bytes this+0xA03E4..0xA03E7.
//   5. 0x42F5F6  joint list: the 255 joint pointers (this+0x9DD70) are
//      walked per type byte (record+0x49D) 0..254; rows below the joint
//      scroll gate (this+0x9DA48) draw the name (record+0x238) at
//      y = 73+14k, highlight per record+0x4AC, filling this+0x9DA50[line]
//      = joint index; this+0x9DA4C counts matches.
//   6. 0x42F730  tail call: PanelPaint (0x414610) repaints the icon/band
//      column, list scrollbar and selection box.
//
// Reference: ../translated/MikuMikuDance/fcn_0042f1e0.cpp is a mismatched
// decompiler dump (physics-debug renderer, not this function); the port
// follows the live IDA pseudocode/disassembly of VA 0x0042F1E0.
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstring>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/model.hpp"

namespace mikudancestudio {
namespace {

// --- app offsets not yet named in offsets.hpp -------------------------------
constexpr std::size_t kHbitmapPanel = 0x2DC;  // panel back-store bitmap
constexpr std::size_t kListLeft = 91;         // 0x5B  label column width
constexpr std::size_t kJointMap = 0x9DA50;    // joint index -> line map (200 ints)
constexpr std::size_t kJointFlagBase = 0xA04FC;  // joint-row highlight flags (this+656636)

// --- model display-tree offsets (model object, PMD/PMX chain) ---------------
constexpr std::size_t kModelSelLine = 0x2DBC;     // head line of frame 1 (bone list)
constexpr std::size_t kModelLineType = 0x2DC0;    // byte[200] per-line frame type
constexpr std::size_t kModelRecs = 0x2E88;        // int[200] per-line records (rigid idx / -1-morph)
constexpr std::size_t kModelSelFlag = 0x38FC;     // frame-1 selection flag (byte)

// --- physics joint record offsets (124-byte PMD records) --------------------
constexpr std::size_t kJointName = 0x238;  // name
constexpr std::size_t kJointType = 0x49D;  // type byte
constexpr std::size_t kJointFlag = 0x4AC;  // highlight flag byte

// JP panel texts (Shift-JIS; original byte_52C95C/52C958/52C980/52C978/
// 52C96C/52C964).  EN texts are inline literals (asc_52C960/52C6D0,
// aCamera/aLight/aSShadow/aGravity).
constexpr char kJpPlus[] = "\x81\x7b";                     // ＋ (0x52C95C)
constexpr char kJpMinus[] = "\x81\x7c";                    // － (0x52C958)
constexpr char kJpCamera[] = "\x83\x4a\x83\x81\x83\x89";   // カメラ (0x52C980)
constexpr char kJpLight[] = "\x8f\xc6\x81\x40\x96\xbe";    // 照 明 (0x52C978)
constexpr char kJpShadow[] = "\x83\x5a\x83\x8b\x83\x74\x89\x65";  // セルフ影 (0x52C96C)
constexpr char kJpGravity[] = "\x8f\x64\x81\x40\x97\xcd";  // 重 力 (0x52C964)

// Current model slot for the slot-order index (this+0x910) - 0x42F74B.
unsigned char* CurrentModel(MMDApp* app, std::size_t slotIdx) {
    return app->ModelSlot(static_cast<int>(slotIdx));
}

// Colour from the color.txt COLORREF at `colorOff` (0xA065C normal /
// 0xA0660 highlight), split into the three bytes the original pushes.
void DrawPanelText(MMDApp* app, const char* text, HDC hdc, int size, int x,
                   int y, std::size_t colorOff) {
    DrawGlyph(app, text, hdc, size, x, y,
              app->raw<std::uint8_t>(colorOff),
              app->raw<std::uint8_t>(colorOff + 1),
              app->raw<std::uint8_t>(colorOff + 2), 1);
}

// Display-frame name row (0x42FB18/0x42FB98 and siblings).
void DrawFrameName(MMDApp* app, const mdl::DisplayGroup& frame,
                   HDC hdc, int y, std::size_t colorOff) {
    const bool english = app->EnglishUI() != 0;
    DrawPanelText(app,
                  english ? frame.nameEn : frame.name,
                  hdc, 12, 12, y, colorOff);
}

// Rigid-group / face record name row (0x430401/0x430443, 0x43020C/0x430247).
void DrawRecordName(MMDApp* app, const mdl::FrameGroup& record, HDC hdc, int y,
                    std::size_t colorOff) {
    const bool english = app->EnglishUI() != 0;
    DrawPanelText(app,
                  english ? record.nameEn : record.name,
                  hdc, 12, 15, y, colorOff);
}

// Fixed header label of the display-mode band column (0x42F292 etc.).
void DrawHeader(MMDApp* app, HDC hdc, const char* text, int y,
                std::size_t colorOff) {
    DrawPanelText(app, text, hdc, 12, 12, y, colorOff);
}

}  // namespace

void PanelPaint(MMDApp* app);  // VA 0x00414610 (defined in ui_panel_paint.cpp)

void PostLanguageSweep(MMDApp* app) {
    auto& s = *app;
    const bool english = s.EnglishUI() != 0;                        // 658252
    const std::size_t slotIdx = s.SelectedModelSlot();  // 2320
    HDC panel = s.PanelDC();                                       // 724
    // listH = the 0x1D574 render wrapper's "list height" field
    // (screenHeight, sub+0x1D4E8).
    const std::int32_t listH = s.Renderer()->screenHeight;

    // --- 1. erase the label column from the panel bitmap (0x42F1F2) --------
    HDC dc = CreateCompatibleDC(nullptr);
    SelectObject(dc, s.raw<HGDIOBJ>(kHbitmapPanel));                // 732
    BitBlt(panel, 0, 0, kListLeft, listH, dc, 0, 0, SRCCOPY);
    DeleteDC(dc);

    std::memset(s.at(offsets::kBufBuf656632), 0, 0xC8);     // 200 flags

    if (s.raw<std::uint8_t>(offsets::kByteOptflag0) == 0) {
        // === bone-edit mode: current model display tree (0x42F744) ========
        unsigned char* model = CurrentModel(app, slotIdx);
        mdl::ModelRecord* const record = mdl::Mdl(model);
        auto* const boneLineByIndex =
            reinterpret_cast<std::int32_t*>(record->boneKeyIndices);
        auto* const morphLineByIndex =
            reinterpret_cast<std::int32_t*>(record->morphKeyIndices);
        const std::int32_t rootIdx =
            static_cast<std::int32_t>(record->displayRootBone);
        unsigned char* const flagTab = record->boneSelection;
        const mdl::BoneRecord& rootBone = mdl::Bones(model)[rootIdx];
        const char* rootName = english ? rootBone.nameEn : rootBone.name;
        if (flagTab[rootIdx] != 0) {
            DrawPanelText(app, rootName, panel, 12, 12, 17,
                          offsets::kDwordCol656992);
            s.raw<std::uint8_t>(offsets::kBufBuf656632) = 1;
        } else {
            DrawPanelText(app, rootName, panel, 12, 12, 17,
                          offsets::kDwordCol656988);
        }

        // --- zero link tables, clear line records (0x42F88C..0x42F9FB) -----
        std::memset(boneLineByIndex, 0,
                    sizeof(std::uint32_t) * record->boneCount);
        std::memset(morphLineByIndex, 0,
                    sizeof(std::uint32_t) * record->morphCount);
        *reinterpret_cast<std::int32_t*>(model + kModelSelLine) = 0;
        {
            int v17 = 0;
            for (int i = 11916; i < 12716; i += 20) {
                model[v17 + kModelLineType] = 0;
                *reinterpret_cast<std::int32_t*>(model + i - 4) = -999;
                model[v17 + kModelLineType + 1] = 0;
                *reinterpret_cast<std::int32_t*>(model + i) = -999;
                model[v17 + kModelLineType + 2] = 0;
                *reinterpret_cast<std::int32_t*>(model + i + 4) = -999;
                model[v17 + kModelLineType + 3] = 0;
                *reinterpret_cast<std::int32_t*>(model + i + 8) = -999;
                model[v17 + kModelLineType + 4] = 0;
                *reinterpret_cast<std::int32_t*>(model + i + 12) = -999;
                v17 += 5;
            }
        }
        *reinterpret_cast<std::int32_t*>(model + kModelRecs) = rootIdx;
        record->boneListRows = 1;

        // --- display-frame iteration (0x42FA13..0x4304F7) -------------------
        const mdl::DisplayGroup* const frames = mdl::DisplayGroups(model);
        const mdl::FrameGroup* const faces = mdl::DisplayFrames(model);
        const mdl::FrameGroup* const groups = mdl::RigidGroups(model);
        const std::int32_t groupCnt =
            static_cast<std::int32_t>(record->rigidBodyCount);
        std::uint8_t v20 = 1;  // display-frame index
        std::uint8_t v72 = 1;  // current frame type (= v20)
        int v21 = 1;           // tree line counter
        std::uint8_t v74 = 0;  // face index (0x430138)
        if (mdl::Mdl(model)->groupCount <= 1) {
            PanelPaint(app);
            return;
        }
        for (;;) {
            {
                const int v80 = v20;
                const bool visible = frames[v20].flags == 0;
                const std::int32_t iter = record->boneListRows;
                const std::int32_t iterMax = record->boneListPos;

                if (visible) {
                    // frame lists bones (0x42FA7E)
                    if (iter > iterMax && v21 < 200) {
                        if (english) {
                            if (v20 > 1)
                                DrawGlyph(app, "+", panel, 12, 1, 14 * v21 + 17, 0, 0, 0, 1);
                            DrawFrameName(app, frames[v20], panel, 14 * v21 + 17,
                                          offsets::kDwordCol656988);
                        } else {
                            if (v20 > 1)
                                DrawGlyph(app, kJpPlus, panel, 12, 1, 14 * v21 + 17, 0, 0, 0, 1);
                            DrawFrameName(app, frames[v20], panel, 14 * v21 + 17,
                                          offsets::kDwordCol656988);
                        }
                        if (((v72 == 1) & *reinterpret_cast<std::uint8_t*>(model + kModelSelFlag)) != 0) {
                            DrawFrameName(app, frames[v20], panel, 14 * v21 + 17,
                                          offsets::kDwordCol656992);
                            s.raw<std::uint8_t>(offsets::kBufBuf656632 + v21) = 1;
                        }
                        if (record->facialFrameCount != 0 && v72 == 2) {
                            // frame 2: highlight when any face flag (+44) is
                            // set - first face with a live flag (0x42FC57).
                            unsigned int v31 = 0;
                            while (faces[v31].selected == 0) {
                                ++v31;
                                if (v31 >= mdl::Mdl(model)->facialFrameCount)
                                    goto lab76;
                            }
                            DrawFrameName(app, frames[v20], panel, 14 * v21 + 17,
                                          offsets::kDwordCol656992);
                            s.raw<std::uint8_t>(offsets::kBufBuf656632 + v21) = 1;
                        }
                    lab76:
                        // highlight when a rigid group of this frame is visible (0x42FD26)
                        {
                            int v84 = 0;
                            if (groupCnt) {
                                while (groups[v84].groupIndex != v80 ||
                                       flagTab[groups[v84].targetIndex] == 0) {
                                    if (++v84 >= groupCnt)
                                        goto lab86;
                                }
                                DrawFrameName(app, frames[v20], panel, 14 * v21 + 17,
                                              offsets::kDwordCol656992);
                                s.raw<std::uint8_t>(offsets::kBufBuf656632 + v21) = 1;
                            }
                        }
                    lab86:
                        // line type byte + morph/rigid back-links (0x42FE37)
                        model[v21 + kModelLineType] = v72;
                        if (v72 == 1)
                            *reinterpret_cast<std::int32_t*>(model + kModelSelLine) = v21;
                        for (std::uint8_t j = 0;
                             j < mdl::Mdl(model)->facialFrameCount; ++j) {
                            const mdl::FrameGroup& face = faces[j];
                            if (face.groupIndex + 1 == v80)
                                morphLineByIndex[face.targetIndex] = -v21;
                        }
                        if (groupCnt) {
                            unsigned int v40 = 0;
                            do {
                                const mdl::FrameGroup& rigid = groups[v40];
                                if (rigid.groupIndex == v80)
                                    boneLineByIndex[rigid.targetIndex] = -v21;
                                ++v40;
                            } while (v40 < static_cast<unsigned int>(groupCnt));
                        }
                        v20 = v72;
                        ++v21;
                    }
                    ++record->boneListRows;
                    goto lab140;
                }

                // frame lists faces / rigid groups (0x42FF94)
                if (iter > iterMax && v21 < 200) {
                    if (english) {
                        if (v20 > 1)
                            DrawGlyph(app, "-", panel, 12, 1, 14 * v21 + 17, 0, 0, 0, 1);
                        DrawFrameName(app, frames[v20], panel, 14 * v21 + 17,
                                      offsets::kDwordCol656988);
                    } else {
                        if (v20 > 1)
                            DrawGlyph(app, kJpMinus, panel, 12, 1, 14 * v21 + 17, 0, 0, 0, 1);
                        DrawFrameName(app, frames[v20], panel, 14 * v21 + 17,
                                      offsets::kDwordCol656988);
                    }
                    model[v21++ + kModelLineType] = v72;
                    v20 = v72;
                }
                ++record->boneListRows;
                if (v20 == 1) {
                    *reinterpret_cast<std::int32_t*>(model + kModelSelLine) = v21 - 1;
                    goto lab140;
                }
                if (record->facialFrameCount == 0 || v20 != 2) {
                    // rigid-group section (0x43031D..0x4304D8)
                    if (groupCnt) {
                        int v87 = 4 * v21 + kModelRecs;
                        int v58 = 14 * v21 + 17;
                        unsigned int v85 = 0;
                        do {
                            const mdl::FrameGroup& rigid = groups[v85];
                            if (rigid.groupIndex == v80) {
                                if (record->boneListRows > record->boneListPos &&
                                    v21 < 200) {
                                    if (flagTab[rigid.targetIndex] != 0) {
                                        DrawRecordName(app, rigid, panel, v58,
                                                       offsets::kDwordCol656992);
                                        s.raw<std::uint8_t>(offsets::kBufBuf656632 + v21) = 1;
                                    } else {
                                        DrawRecordName(app, rigid, panel, v58,
                                                       offsets::kDwordCol656988);
                                    }
                                    *reinterpret_cast<std::int32_t*>(model + v87) =
                                        rigid.targetIndex;
                                    boneLineByIndex[rigid.targetIndex] = v21++;
                                    v87 += 4;
                                    v58 += 14;
                                }
                                ++record->boneListRows;
                            }
                            ++v85;
                        } while (v85 < static_cast<unsigned int>(groupCnt));
                        v20 = v72;  // LABEL_139
                    }
                    goto lab140;
                }
                // face section (frame 2) - break out to the face listing
                v74 = 0;  // 0x430138
                break;
            }
        lab140:
            v72 = ++v20;
            if (v20 >= mdl::Mdl(model)->groupCount) {
                PanelPaint(app);
                return;
            }
        }

        // --- face listing (0x430153..0x43030B) ------------------------------
        {
            int v86 = 4 * v21 + kModelRecs;
            int v47 = 14 * v21 + 17;
            for (;;) {
                if (record->boneListRows > record->boneListPos &&
                    v21 < 200)
                    break;
            lab123:
                ++record->boneListRows;
                if (++v74 >= record->facialFrameCount)
                    goto lab139;
            }
            const mdl::FrameGroup& face = faces[v74];
            if (english) {
                if (face.selected == 0) {
                    DrawPanelText(app, face.nameEn, panel,
                                  12, 15, v47, offsets::kDwordCol656988);
                    goto lab122;
                }
                DrawPanelText(app, face.nameEn, panel,
                              12, 15, v47, offsets::kDwordCol656992);
            } else {
                if (face.selected == 0) {
                    DrawPanelText(app, face.name, panel,
                                  12, 15, v47, offsets::kDwordCol656988);
                    goto lab122;
                }
                DrawPanelText(app, face.name, panel,
                              12, 15, v47, offsets::kDwordCol656992);
            }
            s.raw<std::uint8_t>(offsets::kBufBuf656632 + v21) = 1;
        lab122:
            *reinterpret_cast<std::int32_t*>(model + v86) =
                -1 - face.targetIndex;
            morphLineByIndex[face.targetIndex] = v21++;
            v86 += 4;
            v47 += 14;
            goto lab123;
        }
    lab139:
        v20 = v72;
        goto lab140;
    } else {
        // === display mode: fixed headers camera/light/shadow/gravity =======
        // (0x42F252..0x42F5F0)
        if (english) {
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Camera)) {
                DrawHeader(app, panel, "camera", 17, offsets::kDwordCol656992);
                s.raw<std::uint8_t>(offsets::kBufBuf656632) = 1;
            } else {
                DrawHeader(app, panel, "camera", 17, offsets::kDwordCol656988);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Light)) {
                DrawHeader(app, panel, "light", 31, offsets::kDwordCol656992);
                s.raw<std::uint8_t>(offsets::kBufBuf656632 + 1) = 1;
            } else {
                DrawHeader(app, panel, "light", 31, offsets::kDwordCol656988);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::SelfShadow)) {
                s.raw<std::uint8_t>(offsets::kBufBuf656632 + 2) = 1;
                DrawHeader(app, panel, "s shadow", 45, offsets::kDwordCol656992);
            } else {
                DrawHeader(app, panel, "s shadow", 45, offsets::kDwordCol656988);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Gravity)) {
                s.raw<std::uint8_t>(offsets::kBufBuf656632 + 3) = 1;
                DrawHeader(app, panel, "gravity", 59, offsets::kDwordCol656992);
            } else {
                DrawHeader(app, panel, "gravity", 59, offsets::kDwordCol656988);
            }
        } else {
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Camera)) {
                DrawHeader(app, panel, kJpCamera, 17, offsets::kDwordCol656992);
                s.raw<std::uint8_t>(offsets::kBufBuf656632) = 1;
            } else {
                DrawHeader(app, panel, kJpCamera, 17, offsets::kDwordCol656988);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Light)) {
                DrawHeader(app, panel, kJpLight, 31, offsets::kDwordCol656992);
                s.raw<std::uint8_t>(offsets::kBufBuf656632 + 1) = 1;
            } else {
                DrawHeader(app, panel, kJpLight, 31, offsets::kDwordCol656988);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::SelfShadow)) {
                s.raw<std::uint8_t>(offsets::kBufBuf656632 + 2) = 1;
                DrawHeader(app, panel, kJpShadow, 45, offsets::kDwordCol656992);
            } else {
                DrawHeader(app, panel, kJpShadow, 45, offsets::kDwordCol656988);
            }
            if (s.GlobalTrackSelected(GlobalTimelineTrack::Gravity)) {
                s.raw<std::uint8_t>(offsets::kBufBuf656632 + 3) = 1;
                DrawHeader(app, panel, kJpGravity, 59, offsets::kDwordCol656992);
            } else {
                DrawHeader(app, panel, kJpGravity, 59, offsets::kDwordCol656988);
            }
        }
    }

    // === joint list below the tree (0x42F5F6..0x42F72A) =====================
    std::memset(s.at(kJointMap), 0xFF, 0x320);       // 200 ints
    s.DisplayObjectListMatchCount() = 1;
    std::uint8_t v73 = 0;                                    // joint type 0..254
    int v79 = 0;                                             // type counter (scroll gate)
    int v5 = 0;                                              // joint line counter
    do {
        int v6 = 0;                                          // joint record index
        int v7 = 14 * v5 + 73;
        std::int32_t* map =
            reinterpret_cast<std::int32_t*>(s.at(kJointMap)) + v5;
        do {
            unsigned char* joint = static_cast<unsigned char*>(s.ObjectSlot(v6));
            if (joint != nullptr && joint[kJointType] == v73) {
                if (v79 >= s.DisplayObjectListScrollPosition() && v5 < 200) {
                    if (joint[kJointFlag] != 0) {
                        DrawPanelText(app,
                                      reinterpret_cast<const char*>(joint + kJointName),
                                      panel, 12, 12, v7, offsets::kDwordCol656992);
                        s.raw<std::uint8_t>(kJointFlagBase + v5) = 1;
                    } else {
                        DrawPanelText(app,
                                      reinterpret_cast<const char*>(joint + kJointName),
                                      panel, 12, 12, v7, offsets::kDwordCol656988);
                    }
                    *map = v6;
                    ++v5;
                    ++map;
                    v7 += 14;
                }
                ++s.DisplayObjectListMatchCount();
            }
            ++v6;
        } while (v6 < 255);
        ++v79;
        ++v73;
    } while (v73 != 0xFF);

    PanelPaint(app);  // 0x42F730 tail call (0x414610)
}

}  // namespace mikudancestudio
