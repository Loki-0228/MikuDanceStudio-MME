// ===========================================================================
// VA 0x0046FF35..0x004739E2 - ConsumeLetterHotkeys  (main-pump letter ladder)
// ===========================================================================
// The main pump (sub_46B090, x64 twin sub_7FF7CB4474F0) polls the keyboard
// at 0x46FF02 (frame_modes.cpp MouseInteractionBegin / model_query_gaps.cpp
// Sub42D3A0) and then walks a consumption ladder that checks each letter's
// pressed-this-frame cell in MMDAppState::dialogFlags (values: 0 idle,
// 1 pressed, 2 released, 3 held; only ==1 is consumed here) and dispatches
// the equivalent of a menu command.  This port runs the ladder from
// FrameDriver right after MouseInteractionBegin.
//
// Common gate, recomputed per block in the original (x86 registers ebx=app,
// var_14C4=GetFocus(), var_14D0="focus not in a panel edit"):
//   focusOK       = GetFocus() == main window || ViewportInputActive()
//                   (byte 0x9EDD1, set at 0x46FF29/0x46FF32 from the
//                   foreground window - already maintained by
//                   MouseInteractionBegin)
//   focusInEdit   = focus is one of the panel edits 0x1C1 / 0x1C2 / 0x1DA /
//                   0x1DB / 0x220..0x226 / 0x22A on (floating ? floating :
//                   main) - the controls whose TAB/caret sections clear the
//                   original's flag (x64 0x44E766..0x44ED12)
//   shift / ctrl  = shiftModifierState / ctrlModifierState == 3 (held)
// Block-specific extra gates are noted inline (playbackActive == 0 is the
// "frame advanced" gate app+0x330; FrameStepPlayback() is byte 0x9ED90;
// optflag[0] == 0 selects model/bone mode).
//
// SLOT MAPPING (verified against both binaries): the x86 poll fills
// cell +0x4C (slot 7) with 'g' and +0x50 (slot 8) with 's' (0x42D4CE /
// 0x42D4E5), and the ladder's three-way "seek frame / fine shadow" block
// is gated on slot 7 - i.e. on the G key, exactly as MMD 9.32 documents
// (G: go to frame, Shift+G: self-shadow map, Ctrl+G: the "fine shadow
// mode" notice; S: select unregistered bones, Ctrl+S: save + beep).
// The poll tables (frame_modes.cpp / model_query_gaps.cpp kLetterKeys)
// carry the same binary pairing after the 2026-09 s/g transpose fix.
//
// Not ported here (not letters): the ']' (VK 221) interpolation-toggle
// block 0x4721C9, the VK 221 edit-focus block 0x472246, the arrow-key
// camera/light repeat section 0x47283F..0x472A49, the Enter register-frame
// blocks and the ESC handling that surround the ladder in the pump.
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/ported_funcs.hpp"

namespace mikudancestudio {

// command_view_menu.cpp - the Ctrl+G branch body (x86 0x4726EE, x64
// 0x7FF7CB44F625).  Defined at namespace scope there; declared here to
// avoid touching ported_funcs.hpp.
void FineShadowModeNotice(MMDApp* app);

// ui_frame_step.cpp - frame-apply chain; declared locally like
// command_control_500.cpp does (not in ported_funcs.hpp).
void Sub432FA0(MMDApp* app);                              // VA 0x432FA0

namespace {

// Byte 0xA0D28 twin (x64 0xA1DC8): Shift+G self-shadow map display toggle.
// Consumed by the pump's shadow-map render pass (x86 0x46DE61) and the
// init path (sub_40A730); no named MMDAppState field exists yet, so the
// toggle lives here until app_layout.hpp gains one.
std::uint8_t g_selfShadowMapDisplay = 0;

// Port dialogFlags slots as written by frame_modes.cpp kLetterKeys.
enum LetterSlot {
    kSlotX = 0, kSlotZ = 1, kSlotC = 2, kSlotV = 3,
    kSlotD = 4,  kSlotA = 5,  kSlotB = 6,
    kSlotG = 7,  // receives 'g'/'G' (binary pairing, see header note)
    kSlotS = 8,  // receives 's'/'S'
    kSlotI = 9,  kSlotH = 10, kSlotK = 11, kSlotP = 12, kSlotU = 13,
    kSlotJ = 14, kSlotF = 15, kSlotR = 16, kSlotL = 17,
};

// The 0x22A read and the edit-focus probe run on the floating viewport
// window when one exists (x86 0x472731 / 0x471E67).
HWND LadderOwner(MMDApp* app) {
    HWND owner = app->FloatingWindow();
    return owner != nullptr ? owner : static_cast<HWND>(app->Hwnd());
}

// The panel edits whose focus suppresses the ladder (see header).
bool FocusInPanelEdit(MMDApp* app) {
    const HWND owner = LadderOwner(app);
    const HWND focus = GetFocus();
    static const int kEdits[] = {
        0x1C1, 0x1C2, 0x1DA, 0x1DB,
        0x220, 0x221, 0x222, 0x223, 0x224, 0x225, 0x226, 0x22A,
    };
    for (int id : kEdits) {
        if (GetDlgItem(owner, id) == focus)
            return true;
    }
    return false;
}

// The ladder re-dispatches menu command ids through the main window
// (SendMessageA(main, WM_COMMAND, id, 0) at e.g. 0x471F84).
void SendMenuCommand(MMDApp* app, int id) {
    SendMessageA(static_cast<HWND>(app->Hwnd()), WM_COMMAND, id, 0);
}

}  // namespace

void ConsumeLetterHotkeys(MMDApp* app) {
    auto& state = app->state;
    const HWND main = static_cast<HWND>(app->Hwnd());
    const HWND focus = GetFocus();                        // var_14C4
    const bool focusOK =
        focus == main || app->ViewportInputActive() != 0;  // 0x9EDD1
    const bool focusInEdit = FocusInPanelEdit(app);        // !var_14D0
    const bool shift = state.shiftModifierState == 3;      // +0x24
    const bool ctrl = state.ctrlModifierState == 3;        // +0xC0
    const bool playing = app->PlaybackActive() != 0;       // +0x330
    const bool modelMode = state.optflag[0] == 0;          // +0x2F8
    const auto pressed = [&state](int slot) {
        return state.dialogFlags[slot] == 1;
    };

    // ---- 'B' (0x471EB5): toggle background color black/white ------------
    // Body = command case 282 minus its dialogFlags[4] write: flip
    // app+0xA0194 and mirror menu item 0x11A.
    if (focusOK && !focusInEdit && pressed(kSlotB)) {
        HMENU menu = GetMenu(main);
        if (state.a0194 != 0) {
            state.a0194 = 0;
            CheckMenuItem(menu, 0x11A, MF_UNCHECKED);
        } else {
            state.a0194 = 1;
            CheckMenuItem(menu, 0x11A, MF_CHECKED);
        }
    }

    // ---- 'V' (0x471F37): transparent models / outline / paste ------------
    if (focusOK && !focusInEdit && pressed(kSlotV)) {
        if (ctrl && !playing) {
            SendMenuCommand(app, 0x1A5);                   // case 421 paste
        } else if (shift) {
            // Body = command case 283 minus dialogFlags[5]: flip the
            // outline-suppression byte and menu 0x11B.
            HMENU menu = GetMenu(main);
            if (state.modelOutlineRenderingSuppressed != 0) {
                state.modelOutlineRenderingSuppressed = 0;
                CheckMenuItem(menu, 0x11B, MF_UNCHECKED);
            } else {
                state.modelOutlineRenderingSuppressed = 1;
                CheckMenuItem(menu, 0x11B, MF_CHECKED);
            }
        } else {
            // Body = command case 0xD6 minus the command bookkeeping: flip
            // app+0x9EB7E, mirror menu 0xD6 and push the flag into every
            // loaded model's displayState (the pump walks the whole slot
            // array; the port's slot array is 100 entries).
            HMENU menu = GetMenu(main);
            state.v9eb7e = state.v9eb7e != 0 ? 0 : 1;
            CheckMenuItem(menu, 0xD6,
                          state.v9eb7e != 0 ? MF_CHECKED : MF_UNCHECKED);
            for (int slot = 0; slot < 100; ++slot) {
                unsigned char* model = app->ModelSlot(slot);
                if (model != nullptr)
                    mdl::Mdl(model)->displayState = state.v9eb7e;
            }
        }
    }

    // ---- 'C' (0x4720B7): copy frames / bone-select toggle ----------------
    if (focusOK && !playing && !focusInEdit && pressed(kSlotC)) {
        if (ctrl) {
            SendMenuCommand(app, 0x1A4);                   // case 420 copy
        } else if (modelMode) {
            SendMessageA(GetDlgItem(main, 0x1EB), BM_SETCHECK, 0, 0);
            SendMessageA(GetDlgItem(main, 0x1EC), BM_SETCHECK, 0, 0);
            SendMessageA(GetDlgItem(main, 0x1ED), BM_SETCHECK, 0, 0);
            if (IsDlgButtonChecked(main, 0x1EA) != 1) {
                app->EditMode() = ViewportEditMode::Bone;
                SendMessageA(GetDlgItem(main, 0x1EA), BM_SETCHECK, 1, 0);
            } else {
                app->EditMode() = ViewportEditMode::None;
                SendMessageA(GetDlgItem(main, 0x1EA), BM_SETCHECK, 0, 0);
            }
        }
    }

    // ---- 'H' (0x472324): register all facials (clear all frames, 0xE5) ---
    if (focusOK && modelMode && !playing && !focusInEdit &&
        pressed(kSlotH)) {
        SendMenuCommand(app, 0xE5);                        // case 229
    }

    // ---- 'I' (0x472375): insert bone/camera frame line -------------------
    if (focusOK && !playing && !focusInEdit && pressed(kSlotI))
        Sub439E40(app);                                    // 0x439E40

    // ---- 'K' (0x4723AB): delete bone/camera frame line -------------------
    if (focusOK && !playing && !focusInEdit && pressed(kSlotK))
        Sub43A650(app);                                    // 0x43A650

    // ---- 'U' (0x4723E1): insert facial/light frame line ------------------
    if (focusOK && !playing && !focusInEdit && pressed(kSlotU))
        Sub43B720(app);                                    // 0x43B720

    // ---- 'J' (0x472417): delete facial/light frame line ------------------
    if (focusOK && !playing && !focusInEdit && pressed(kSlotJ))
        Sub43BB30(app);                                    // 0x43BB30

    // ---- 'G' (0x4726B2): seek frame / shadow map / fine shadow -----------
    if (focusOK && !focusInEdit && pressed(kSlotG)) {
        if (shift) {
            g_selfShadowMapDisplay = g_selfShadowMapDisplay != 0 ? 0 : 1;
        } else if (ctrl) {
            FineShadowModeNotice(app);                     // 0x4726EE
        } else {
            // Same body as command case 553: read the frame edit 0x22A on
            // the floating window when present, atol + clamp >= 0, apply
            // as the current frame (0x980), run the frame-apply chain and
            // echo "%d" into 0x1A1 on the main window.
            char buf[0x100];
            GetWindowTextA(GetDlgItem(LadderOwner(app), 0x22A), buf, 0xA);
            long value = std::atol(buf);
            if (value < 0)
                value = 0;
            app->CurrentFrame() = static_cast<std::int32_t>(value);
            Sub432FA0(app);                                // 0x432FA0
            PostViewRefresh(app);                          // 0x40D130
            sprintf_s(buf, 0x100u, "%d", app->CurrentFrame());
            SetWindowTextA(GetDlgItem(main, 0x1A1), buf);
        }
    }

    // ---- 'L' (0x472804): cycle the transform-channel selector ------------
    // ++app+0x9ED9C, wrapping mod 3 in camera/accessory mode, mod 2 in
    // model mode (the consumer is the mouse-wheel channel chain,
    // ModeCameraAdjust / frame_modes.cpp v9ed9c).
    if (focusOK && !focusInEdit && pressed(kSlotL)) {
        ++state.v9ed9c;
        const int limit = modelMode ? 2 : 3;
        if (state.v9ed9c >= limit)
            state.v9ed9c = 0;
    }

    // ---- Ctrl+'S' (0x472A65): save + bell (no edit-focus gate) -----------
    if (focusOK && pressed(kSlotS) && ctrl) {
        SendMenuCommand(app, 0xCF);                        // case 0xCF save
        MessageBeep(0x40);
    }

    // ---- 'P' (0x472A95): play/stop via the play button 0x198 -------------
    if (focusOK && app->FrameStepPlayback() == 0 && !playing &&
        !focusInEdit && pressed(kSlotP)) {
        // The BM_SETCHECK wParam keys on app+0x330, which the gate already
        // forced to 0 - always 1 here.
        SendMessageA(GetDlgItem(main, 0x198), BM_SETCHECK, 1, 0);
        SendMenuCommand(app, 0x198);                       // case 408 play
        SetFocus(main);
    }

    // ---- 'F' (0x472B2A): paste keyframes to another bone (0xFA) ----------
    if (focusOK && app->FrameStepPlayback() == 0 && !focusInEdit &&
        pressed(kSlotF)) {
        SendMenuCommand(app, 0xFA);                        // case 250
    }

    // ---- 'X' (0x472D5C): redo / camera mode ------------------------------
    if (focusOK && modelMode && !playing && !focusInEdit && pressed(kSlotX)) {
        unsigned char* model = app->SelectedModel();
        if (ctrl && model != nullptr &&
            mdl::Mdl(model)->redoDirty != 0) {
            // Redo one ring entry (0x4A2490), then enable the undo button
            // / disable redo exactly like the pump tail, and re-seek the
            // frame when the new ring head is not a pose-only record.
            // The original passes &app+0x980 so the redo can move the
            // current frame itself.
            Sub4A2490(model, &app->CurrentFrame());
            auto* record = mdl::Mdl(model);
            if (record->undoState[0] == record->undoState[1]) {
                EnableWindow(GetDlgItem(main, 0x191), FALSE);
                record->redoDirty = 0;
            }
            EnableWindow(GetDlgItem(main, 0x190), TRUE);
            record->undoDirty = 1;
            PanelPaint(app);                               // 0x414610
            SelectionReeval(app);                          // 0x430510
            if (record->undoRings[0].slots[record->undoState[0]].operation
                    != 1) {
                Sub4B4260(model, static_cast<int>(app->CurrentFrame()),
                          app->PlaybackPhysicsMode());
            }
        } else if (!ctrl) {
            SendMessageA(GetDlgItem(main, 0x1EA), BM_SETCHECK, 0, 0);
            SendMessageA(GetDlgItem(main, 0x1EB), BM_SETCHECK, 0, 0);
            SendMessageA(GetDlgItem(main, 0x1EC), BM_SETCHECK, 0, 0);
            SendMessageA(GetDlgItem(main, 0x1ED), BM_SETCHECK, 1, 0);
            app->EditMode() = ViewportEditMode::Camera;    // 0x914 = 3
        }
    }

    // ---- 'Z' (0x472ED7): undo / light mode -------------------------------
    if (focusOK && modelMode && !playing && !focusInEdit && pressed(kSlotZ)) {
        unsigned char* model = app->SelectedModel();
        if (ctrl && model != nullptr &&
            mdl::Mdl(model)->undoDirty != 0) {
            // Undo one ring entry (0x4A1870), then the mirror-image button
            // tail of the X branch.  Like the redo, the frame pointer aims
            // straight at app+0x980.
            Sub4A1870(model, &app->CurrentFrame());
            auto* record = mdl::Mdl(model);
            if (record->undoRings[0].slots[record->undoState[0]].operation
                    == 0) {
                EnableWindow(GetDlgItem(main, 0x190), FALSE);
                record->undoDirty = 0;
            }
            if (record->undoState[0] == record->undoState[1]) {
                EnableWindow(GetDlgItem(main, 0x190), FALSE);
                record->undoDirty = 0;
            }
            EnableWindow(GetDlgItem(main, 0x191), TRUE);
            record->redoDirty = 1;
            PanelPaint(app);                               // 0x414610
            SelectionReeval(app);                          // 0x430510
        } else if (!ctrl) {
            SendMessageA(GetDlgItem(main, 0x1EA), BM_SETCHECK, 0, 0);
            SendMessageA(GetDlgItem(main, 0x1EB), BM_SETCHECK, 0, 0);
            SendMessageA(GetDlgItem(main, 0x1ED), BM_SETCHECK, 0, 0);
            SendMessageA(GetDlgItem(main, 0x1EC), BM_SETCHECK, 1, 0);
            app->EditMode() = ViewportEditMode::Light;     // 0x914 = 4
        }
    }

    // ---- 'A' (0x473092): select all bones (0x1EE) ------------------------
    // No focus gates at all in the original - fires even while an edit or
    // another window has focus.
    if (pressed(kSlotA))
        SendMenuCommand(app, 0x1EE);                       // case 494

    // ---- plain 'S' (0x4730B1): select unregistered bones (0x1F5) ---------
    if (pressed(kSlotS) && !ctrl)
        SendMenuCommand(app, 0x1F5);                       // case 501

    // ---- 'D' (0x4730D9): model-offset dialog (0xDB) ----------------------
    if (pressed(kSlotD))
        SendMenuCommand(app, 0xDB);                        // case 219

    // ---- 'R' (0x473989): frame control dialog (0xFB) ---------------------
    if (focusOK && modelMode && app->FrameStepPlayback() == 0 &&
        !focusInEdit && pressed(kSlotR)) {
        SendMenuCommand(app, 0xFB);                        // case 251
    }
}

}  // namespace mikudancestudio
