// ===========================================================================
// VA 0x0047E8A0 - CommandDispatch  (original: sub_47E8A0, 0x10A6A = 68KB)
// ===========================================================================
// WM_COMMAND target from MainWndProc (0x004C3A10): a ~368-case switch on
// LOWORD(wParam).  The switch body is split by id family into the files
// below (each keeps its own case -> VA mapping in the header comment):
//   0x000..0x0FF  File menu          200..250  command_file_menu.cpp
//   0x0FB..0x12E  View/option menu   251..302  command_view_menu.cpp
//   0x190..0x1C1  control notif.     400..449  command_control_400.cpp
//   0x1C2..0x1F3  control notif.     450..499  command_control_450.cpp
//   0x1F4..0x237  control notif.     500..567  command_control_500.cpp
// The 19 cases implemented directly below (200 / 0xC9..0xDC) predate the
// family split and stay here; everything else funnels to the families.
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>

#include <cstdio>
#include <cstdint>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/ported_funcs.hpp"

namespace mikudancestudio {

// Family entry points (defined in the per-family TUs listed above).
void CmdFileMenu(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify);
void CmdViewMenu(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify);
void CmdControl400(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify);
void CmdControl450(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify);
void CmdControl500(MMDApp* app, HWND hwnd, std::uint16_t id, std::uint16_t notify);

void CommandDispatch(HWND ctrl, WPARAM wParam) {
    MMDApp* app = g_Block;
    auto& s = *app;
    HWND hwnd = static_cast<HWND>(s.Hwnd());
    const std::uint16_t id = LOWORD(wParam);
    const std::uint16_t notification = HIWORD(wParam);

    switch (id) {
    case 200:        // File: Exit (0x48BCF9) - flag + WM_CLOSE
        s.raw<std::uint32_t>(0x30) = 1;
        SendMessageA(hwnd, WM_CLOSE, 0, 0);
        break;

    case 0xC9: {     // Help: About - version banner (EN text verified)
        wchar_t text[256];
        swprintf_s(text, 256,
                   L"MikuDanceStudio Ver.%4.2f\n  (DirectX9 Version)\n\n"
                   L"programmed by Yu Higuchi", 9.32f);
        MessageBoxW(hwnd, text, L"About MikuDanceStudio", MB_OK);
        break;
    }

    case 0xCA:      // File: load VPD pose
        CmdLoadPose(app);
        break;
    case 0xCB:      // File: save VPD pose (selected-bones check inside)
        CmdSavePose(app);
        break;
    case 0xCC:      // File: New - reset scene state
        CmdResetState(app);
        break;
    case 0xCD:      // File: open PMM (dirty confirm + dialog)
        CmdOpenScene(app);
        break;
    case 0xCE:      // File: open WAV
        CmdOpenWave(app);
        break;
    case 0xCF:      // File: save PMM (0x489AF7) - overwrite via stored path,
        //          falling into the save-as flow (case 0xD0) when unnamed
        if (app->EnvFileName()[0] != L'\0') {
            SaveSceneFile(app);
        } else {
            CmdSaveScene(app);
        }
        break;
    case 0xD0:      // File: save PMM as (0x489B12 GetSaveFileNameW flow)
        CmdSaveScene(app);
        break;
    case 0xD1:      // File: load VMD motion
        CmdLoadMotion(app);
        break;
    case 0xD2:      // File: save VMD motion
        CmdSaveMotion(app);
        break;
    case 0xD3: {    // View: information display toggle (0x47EAC7/0x47EB00)
        auto& flag = s.raw<unsigned char>(offsets::kByte31E);
        if (flag != 0) {
            flag = 0;
            CheckMenuItem(GetMenu(hwnd), 0xD3, MF_UNCHECKED);
        } else {
            flag = 1;
            s.raw<float>(offsets::kFloat320) = 0.0f;   // timer cluster reset
            s.raw<std::uint32_t>(offsets::kDword324) = 0;
            CheckMenuItem(GetMenu(hwnd), 0xD3, MF_CHECKED);
        }
        break;
    }
    case 0xD5:      // Background: load AVI file
        CmdLoadAvi(app);
        break;
    case 0xD6: {    // View: character transparent mode (0x487FA0)
        auto& flag = s.raw<unsigned char>(offsets::kByte9EB7E);
        flag = flag ? 0 : 1;
        CheckMenuItem(GetMenu(hwnd), 0xD6, flag ? MF_CHECKED : MF_UNCHECKED);
        break;
    }
    case 0xD7: {    // View: coordinate axis display (0x47FBF5)
        auto& flag = s.raw<unsigned char>(offsets::kByte31D);
        flag = flag ? 0 : 1;
        CheckMenuItem(GetMenu(hwnd), 0xD7, flag ? MF_CHECKED : MF_UNCHECKED);
        break;
    }
    case 0xD8: {    // Background: AVI display (0x4871F9) - dword toggle
        auto& flag = s.raw<std::uint32_t>(offsets::kDword91C);
        flag = flag ? 0 : 1;
        CheckMenuItem(GetMenu(hwnd), 0xD8, flag ? MF_CHECKED : MF_UNCHECKED);
        break;
    }
    case 0xD9:      // Edit: select all bone frames (0x4831EA)
        SelectFrameGroup(app, 0);
        break;
    case 0xDA:      // Edit: select all disp/IK/OP frames (0x4832C8)
        SelectFrameGroup(app, 1);
        break;
    case 0xDC:      // Edit: select all facial frames (0x483258)
        SelectFrameGroup(app, 2);
        break;

    default:
        // Everything not handled above funnels into the per-family TUs by id
        // range (each family switches on its own cases; unlisted ids and the
        // original default region 303..399 are no-ops, matching def_47E903).
        if (id >= 200 && id <= 250) {
            CmdFileMenu(app, hwnd, id, notification);
        } else if (id >= 251 && id <= 302) {
            CmdViewMenu(app, hwnd, id, notification);
        } else if (id >= 400 && id <= 449) {
            CmdControl400(app, hwnd, id, notification);
        } else if (id >= 450 && id <= 499) {
            CmdControl450(app, hwnd, id, notification);
        } else if (id >= 500 && id <= 567) {
            CmdControl500(app, hwnd, id, notification);
        }
        break;
    }
}

}  // namespace mikudancestudio
