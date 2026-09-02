// ===========================================================================
// Physics-model editor (menu 262, dialog 0x2AC, sub_465020 family)
// ===========================================================================
// Split out of src/window/command_view_menu.cpp (the menu-251..302 command
// family) so the dialog's helper bodies can be ported independently.
// Every function keeps its original x86 VA; behaviour notes live in the
// per-function comments.  Still TODO(port) unless noted otherwise.
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#include "mikudancestudio/d3dx_dyn.hpp"
#include "mikudancestudio/global_key_layout.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/model.hpp"

namespace mikudancestudio {

// ---- JP strings, byte-exact Shift-JIS as in the binary ------------------
// (twins of the statics in command_view_menu.cpp - file-local by idiom)
// 0x52E724: "編集結果を'強化モデル保存'で保存してモデルとして保存して下さい"
static const char kMsgEnhanceModelJp[] =
    "ÒWÊÍ'g£"
    "fÛ¶'ÅV"
    "µ¢fÆµ"
    "ÄÛ¶µÄº³¢";
// 0x52E764: "モデル強化"
static const char kCaptionEnhanceModelJp[] =
    "fg£";
void Sub45F480(HWND hDlg) {  // VA 0x0045F480 frame-edit camera combo refresh
    (void)hDlg; /* TODO(port) */
}

LRESULT CALLBACK Sub41EC50(HWND, UINT, WPARAM, LPARAM) {  // VA 0x0041EC50
    return 0; /* TODO(port) frame-edit edit-box subclass wndproc */
}

void Sub45F670(HWND hDlg) {  // VA 0x0045F670 frame-edit dialog init
    (void)hDlg; /* TODO(port) */
}

void Sub41FF30(MMDApp* app) {  // VA 0x0041FF30 camera-side frame collect
    (void)app; /* TODO(port) */
}

void Sub4204F0(MMDApp* app) {  // VA 0x004204F0 bone-side frame collect
    (void)app; /* TODO(port) */
}

void Sub43CA50(HWND hDlg) {  // VA 0x0043CA50 bone combo refresh
    (void)hDlg; /* TODO(port) */
}

void Sub43CCD0(HWND hDlg, int idx) {  // VA 0x0043CCD0 camera combo refresh
    (void)hDlg; (void)idx; /* TODO(port) */
}

void Sub4214A0(HWND hDlg, int idx) {  // VA 0x004214A0 bone combo refresh
    (void)hDlg; (void)idx; /* TODO(port) */
}

void Sub421C20(HWND hDlg, int flag) {  // VA 0x00421C20 frame-mode change
    (void)hDlg; (void)flag; /* TODO(port) */
}

void Sub421CE0(HWND hDlg, int mode, int idx) {  // VA 0x00421CE0
    (void)hDlg; (void)mode; (void)idx; /* TODO(port) */
}

void Sub4220F0(HWND hDlg) {  // VA 0x004220F0 frame-edit dialog finalize
    (void)hDlg; /* TODO(port) */
}

void Sub420DC0(HWND hDlg) {  // VA 0x00420DC0
    (void)hDlg; /* TODO(port) */
}

// ===========================================================================
// 0x465020 - case 262 frame-edit dialog (0x2AC EN / 0x2AB JP), modeless at
// app+0xA0B74.  WM_INITDIALOG: subclass edit 705 + edits 709..723 +
// 744..767 + 740 with sub_41EC50 (old proc of 705 saved at app+0xA0B78),
// sub_45F670 init.
// WM_COMMAND (app+0xA0B6C = 1 first):
//   0x2AD/0x2DF   camera/bone combo "select" buttons (collect + refresh)
//   0x2C2/0x2E1   delete the selected camera/bone frame: CB_DELETESTRING and
//                 renumber every frame reference in the 172-byte camera
//                 records (fields +84/+256/+428/+600/+772 per 860-byte
//                 group) resp. 140-byte bone records (+132/+272/+412/+552/
//                 +692 per 700-byte group); -1 marks deleted slots
//   0x2D4/0x2D5   camera frame copy to / restore from scratch app+0xA0B80
//   0x2E2/0x2E3   bone frame copy to / restore from scratch app+0xA0C34
//   0x2D6..0x2D8  bone frame +36 mode 0/1/2
//   0x2D9..0x2DB  checkbox 731: record +80/+81 flags
//   0x320/0x321   camera/bone list mode flip
//   id 1/2        close: free the frame buffers (app+0xA0B7C/0xA0C30),
//                 DestroyWindow, app+0xA0B74 = 0, re-enable main-window
//                 combos 0x1B4/0x198; id 1 also shows the preserve box and
//                 sets app+0xA0B64 = 1
// CBN_SELCHANGE of 704/736/741/742/707/708/743: link combo selection to the
// frame arrays (helpers sub_43CCD0/sub_4214A0/sub_420DC0).
// Reference: MikuMikuDance.exe sub_465020.
// ===========================================================================
INT_PTR CALLBACK Sub465020(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    MMDApp* app = g_Block;
    const HWND hCtrl = reinterpret_cast<HWND>(lParam);
    if (msg == WM_INITDIALOG) {
        if (app->state.floatingWindow != 0) {
            SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
        }
        app->FrameCopyEditProc() =
            reinterpret_cast<WNDPROC>(
                GetWindowLongPtrA(GetDlgItem(hDlg, 705), GWLP_WNDPROC));
        SetWindowLongPtrA(GetDlgItem(hDlg, 705), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)Sub41EC50);
        for (int i = 709; i <= 723; ++i) {
            SetWindowLongPtrA(GetDlgItem(hDlg, i), GWLP_WNDPROC,
                           (LONG)(LONG_PTR)Sub41EC50);
        }
        for (int j = 744; j <= 767; ++j) {
            SetWindowLongPtrA(GetDlgItem(hDlg, j), GWLP_WNDPROC,
                           (LONG)(LONG_PTR)Sub41EC50);
        }
        SetWindowLongPtrA(GetDlgItem(hDlg, 740), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)Sub41EC50);
        Sub45F670(hDlg);  // 0x45F670
        return 0;
    }
    if (msg != WM_COMMAND) {
        return 0;
    }
    // dirty write: the frame-edit dialog deliberately invalidates the
    // cached time (original blob reuse of app+0xA0B6C == timeNowHigh)
    reinterpret_cast<std::int32_t&>(app->state.timeNowHigh) = 1;
    switch (LOWORD(wParam)) {
    case 0x2AD:  // camera combo "select"
        Sub41FF30(app);   // 0x41FF30
        Sub45F480(hDlg);  // 0x45F480
        return 0;
    case 0x2DF:  // bone combo "select"
        Sub4204F0(app);   // 0x4204F0
        Sub43CA50(hDlg);  // 0x43CA50
        return 0;
    case 0x2C2: {  // delete camera frame (0x4650C6)
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 704), 0x147 /*CB_GETCURSEL*/, 0, 0));
        SendMessageA(GetDlgItem(hDlg, 704), 0x144 /*CB_DELETESTRING*/, sel, 0);
        SendMessageA(GetDlgItem(hDlg, 741), 0x144 /*CB_DELETESTRING*/, sel, 0);
        SendMessageA(GetDlgItem(hDlg, 742), 0x144 /*CB_DELETESTRING*/, sel, 0);
        unsigned char* cam =
            static_cast<unsigned char*>(app->state.cameraRecordArray);
        const std::int32_t curCam = app->state.selAcc;
        for (std::int32_t k = 0; k < 1400000; k += 140) {
            if (*reinterpret_cast<std::int32_t*>(cam + k + 132) >= 0) {
                if (*reinterpret_cast<std::int32_t*>(cam + k + 28) == curCam ||
                    *reinterpret_cast<std::int32_t*>(cam + k + 32) == curCam) {
                    SendMessageA(GetDlgItem(hDlg, 736), 0x144 /*CB_DELETESTRING*/,
                                 *reinterpret_cast<std::int32_t*>(cam + k + 132),
                                 0);
                    for (std::int32_t m = 0; m < 1400000; m += 700) {
                        std::int32_t* f;
                        f = reinterpret_cast<std::int32_t*>(cam + m + 132);
                        if (*f > *reinterpret_cast<std::int32_t*>(cam + k + 132))
                            --*f;
                        f = reinterpret_cast<std::int32_t*>(cam + m + 272);
                        if (*f > *reinterpret_cast<std::int32_t*>(cam + k + 132))
                            --*f;
                        f = reinterpret_cast<std::int32_t*>(cam + m + 412);
                        if (*f > *reinterpret_cast<std::int32_t*>(cam + k + 132))
                            --*f;
                        f = reinterpret_cast<std::int32_t*>(cam + m + 552);
                        if (*f > *reinterpret_cast<std::int32_t*>(cam + k + 132))
                            --*f;
                        f = reinterpret_cast<std::int32_t*>(cam + m + 692);
                        if (*f > *reinterpret_cast<std::int32_t*>(cam + k + 132))
                            --*f;
                    }
                    *reinterpret_cast<std::int32_t*>(cam + k + 132) = -1;
                }
            }
        }
        *reinterpret_cast<std::int32_t*>(cam + 172 * curCam + 84) = -1;
        for (std::int32_t n = 0; n < 1720000; n += 860) {
            std::int32_t* f;
            f = reinterpret_cast<std::int32_t*>(cam + n + 84);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(cam + n + 256);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(cam + n + 428);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(cam + n + 600);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(cam + n + 772);
            if (*f > sel) --*f;
        }
        if (SendMessageA(GetDlgItem(hDlg, 704), 0x146 /*CB_GETCOUNT*/, 0, 0) !=
            0) {
            if (*reinterpret_cast<std::int32_t*>(cam + 84) == 0) {
                app->state.selAcc = 0;
            }
        } else {
            app->state.selAcc = -1;
        }
        SendMessageA(GetDlgItem(hDlg, 704), 0x14E /*CB_SETCURSEL*/, 0, 0);
        Sub43CCD0(hDlg, app->state.selAcc);  // 0x43CCD0
        mikudancestudio::mdl::BoneRecord* bone =
            reinterpret_cast<mikudancestudio::mdl::BoneRecord*>(
                app->state.boneRecordArray);
        if (SendMessageA(GetDlgItem(hDlg, 736), 0x146 /*CB_GETCOUNT*/, 0, 0) !=
            0) {
            if (*reinterpret_cast<std::int32_t*>(bone + 132) == 0) {
                app->state.sel8c = 0;
            }
        } else {
            app->state.sel8c = -1;
        }
        // LABEL_46 (0x465407)
        SendMessageA(GetDlgItem(hDlg, 736), 0x14E /*CB_SETCURSEL*/, 0, 0);
        Sub4214A0(hDlg, app->state.sel8c);  // 0x4214A0
        return 0;
    }
    case 0x2E1: {  // delete bone frame (0x46544D)
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 736), 0x147 /*CB_GETCURSEL*/, 0, 0));
        SendMessageA(GetDlgItem(hDlg, 736), 0x144 /*CB_DELETESTRING*/, sel, 0);
        mikudancestudio::mdl::BoneRecord* bone =
            reinterpret_cast<mikudancestudio::mdl::BoneRecord*>(
                app->state.boneRecordArray);
        *reinterpret_cast<std::int32_t*>(
            bone + 140 * app->state.sel8c + 132) = -1;
        for (std::int32_t ii = 0; ii < 1400000; ii += 700) {
            std::int32_t* f;
            f = reinterpret_cast<std::int32_t*>(bone + ii + 132);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(bone + ii + 272);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(bone + ii + 412);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(bone + ii + 552);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(bone + ii + 692);
            if (*f > sel) --*f;
        }
        if (SendMessageA(GetDlgItem(hDlg, 736), 0x146 /*CB_GETCOUNT*/, 0, 0) !=
            0) {
            if (*reinterpret_cast<std::int32_t*>(bone + 132) == 0) {
                app->state.sel8c = 0;
            }
        } else {
            app->state.sel8c = -1;
        }
        // LABEL_46 (0x465407)
        SendMessageA(GetDlgItem(hDlg, 736), 0x14E /*CB_SETCURSEL*/, 0, 0);
        Sub4214A0(hDlg, app->state.sel8c);  // 0x4214A0
        return 0;
    }
    case 0x320:  // bone list mode on
        Sub4204F0(app);                 // 0x4204F0
        Sub421C20(hDlg, 1);             // 0x421C20
        return 0;
    case 0x321:  // bone list mode off
        Sub41FF30(app);                 // 0x41FF30
        Sub421C20(hDlg, 0);             // 0x421C20
        return 0;
    case 0x2D4: {  // copy camera frame to scratch (0x46564E)
        Sub41FF30(app);  // 0x41FF30
        std::memcpy(app->CameraFrameScratch(),
                    static_cast<unsigned char*>(app->state.cameraRecordArray) +
                        172 * app->state.selAcc,
                    0xAC);
        return 0;
    }
    case 0x2D5: {  // restore camera frame from scratch (0x465687)
        unsigned char* scratch = app->CameraFrameScratch();
        unsigned char* rec =
            static_cast<unsigned char*>(app->state.cameraRecordArray) +
            172 * app->state.selAcc;
        *reinterpret_cast<std::int32_t*>(rec + 28) =
            *reinterpret_cast<std::int32_t*>(scratch + 28);
        rec[80] = scratch[80];
        rec[81] = scratch[81];
        *reinterpret_cast<float*>(rec + 76) =
            *reinterpret_cast<float*>(scratch + 76);
        *reinterpret_cast<float*>(rec + 88) =
            *reinterpret_cast<float*>(scratch + 88);
        std::memcpy(rec + 52, scratch + 52, 12);
        *reinterpret_cast<float*>(rec + 92) =
            *reinterpret_cast<float*>(scratch + 92);
        std::memcpy(rec + 64, scratch + 64, 12);
        rec[36] = scratch[36];
        std::memcpy(rec + 40, scratch + 40, 12);
        Sub43CCD0(hDlg, app->state.selAcc);  // 0x43CCD0
        return 0;
    }
    case 0x2E2: {  // copy bone frame to scratch (0x465636)
        Sub4204F0(app);  // 0x4204F0
        std::memcpy(app->BoneFrameScratch(),
                    static_cast<unsigned char*>(app->state.boneRecordArray) +
                        140 * app->state.sel8c,
                    0x8C);
        return 0;
    }
    case 0x2E3: {  // restore bone frame from scratch (0x465863)
        unsigned char* scratch = app->BoneFrameScratch();
        unsigned char* rec =
            static_cast<unsigned char*>(app->state.boneRecordArray) +
            140 * app->state.sel8c;
        *reinterpret_cast<std::int32_t*>(rec + 28) =
            *reinterpret_cast<std::int32_t*>(scratch + 28);
        *reinterpret_cast<std::int32_t*>(rec + 32) =
            *reinterpret_cast<std::int32_t*>(scratch + 32);
        std::memcpy(rec + 72, scratch + 72, 12);
        std::memcpy(rec + 36, scratch + 36, 12);
        std::memcpy(rec + 108, scratch + 108, 12);
        std::memcpy(rec + 60, scratch + 60, 12);
        std::memcpy(rec + 96, scratch + 96, 12);
        std::memcpy(rec + 48, scratch + 48, 12);
        std::memcpy(rec + 120, scratch + 120, 12);
        std::memcpy(rec + 84, scratch + 84, 12);
        Sub4214A0(hDlg, app->state.sel8c);  // 0x4214A0
        return 0;
    }
    case 0x2D6:  // bone frame mode 0
        static_cast<unsigned char*>(app->state.cameraRecordArray)
            [172 * app->state.selAcc + 36] = 0;
        Sub421CE0(hDlg, 0, app->state.selAcc);  // 0x421CE0
        return 0;
    case 0x2D7:  // bone frame mode 1
        static_cast<unsigned char*>(app->state.cameraRecordArray)
            [172 * app->state.selAcc + 36] = 1;
        Sub421CE0(hDlg, 1, app->state.selAcc);  // 0x421CE0
        return 0;
    case 0x2D8:  // bone frame mode 2
        static_cast<unsigned char*>(app->state.cameraRecordArray)
            [172 * app->state.selAcc + 36] = 2;
        Sub421CE0(hDlg, 2, app->state.selAcc);  // 0x421CE0
        return 0;
    case 0x2D9: {  // checkbox 731 checked (0x465B3C)
        EnableWindow(GetDlgItem(hDlg, 731), TRUE);
        unsigned char* rec =
            static_cast<unsigned char*>(app->state.cameraRecordArray) +
            172 * app->state.selAcc;
        if (IsDlgButtonChecked(hDlg, 731) == 1) {
            rec[80] = 2;
            rec[81] = 1;  // LABEL_95
        } else {
            rec[80] = 1;
            rec[81] = 0;  // LABEL_87
        }
        return 0;
    }
    case 0x2DA: {  // checkbox 731 unchecked (0x465BB6)
        EnableWindow(GetDlgItem(hDlg, 731), FALSE);
        unsigned char* rec =
            static_cast<unsigned char*>(app->state.cameraRecordArray) +
            172 * app->state.selAcc;
        rec[80] = 0;
        rec[81] = 0;
        return 0;
    }
    case 0x2DB: {  // checkbox 731 tri-state click (0x465C14)
        unsigned char* rec =
            static_cast<unsigned char*>(app->state.cameraRecordArray) +
            172 * app->state.selAcc;
        if (rec[80] == 2) {
            rec[80] = 1;
            rec[81] = 0;  // LABEL_87
        } else if (rec[80] == 1) {
            rec[80] = 2;
            rec[81] = 1;  // LABEL_95
        }
        return 0;
    }
    default:
        break;
    }
    if (HIWORD(wParam) != 1 /*CBN_SELCHANGE*/) {
        if (LOWORD(wParam) == 1) {
            // OK: free buffers, close, re-enable, preserve box (0x4660C1)
            Sub4220C0(app);     // 0x4220C0 (stubs.cpp)
            Sub41FF30(app);     // 0x41FF30
            Sub4204F0(app);     // 0x4204F0
            Sub4220F0(hDlg);    // 0x4220F0
            if (app->state.cameraRecordArray != nullptr) {
                std::free(app->state.cameraRecordArray);
                app->state.cameraRecordArray = nullptr;
            }
            if (app->state.boneRecordArray != nullptr) {
                std::free(app->state.boneRecordArray);
                app->state.boneRecordArray = nullptr;
            }
            DestroyWindow(hDlg);
            app->state.frameCopyDialog = nullptr;
            EnableWindow(GetDlgItem(app->state.hwnd, 0x1B4), TRUE);
            EnableWindow(GetDlgItem(app->state.hwnd, 0x198), TRUE);
            if (app->state.englishUI != 0) {
                MessageBoxA(app->state.hwnd,
                            "Please preserve the edit result as a new model by "
                            "'save enhanced model'.",
                            "enhance model", 0);
            } else {
                MessageBoxA(app->state.hwnd, kMsgEnhanceModelJp,
                            kCaptionEnhanceModelJp, 0);
            }
            app->EnhancedModelDirty() = 1;
            return 1;
        }
        if (LOWORD(wParam) == 2) {
            // Cancel (0x4661E9)
            Sub4220C0(app);  // 0x4220C0 (stubs.cpp)
            if (app->state.cameraRecordArray != nullptr) {
                std::free(app->state.cameraRecordArray);
                app->state.cameraRecordArray = nullptr;
            }
            if (app->state.boneRecordArray != nullptr) {
                std::free(app->state.boneRecordArray);
                app->state.boneRecordArray = nullptr;
            }
            DestroyWindow(hDlg);
            app->state.frameCopyDialog = nullptr;
            EnableWindow(GetDlgItem(app->state.hwnd, 0x1B4), TRUE);
            EnableWindow(GetDlgItem(app->state.hwnd, 0x198), TRUE);
            return 0;
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 704)) {
        // camera combo selection (0x465C8A)
        Sub41FF30(app);  // 0x41FF30
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 704), 0x147 /*CB_GETCURSEL*/, 0, 0));
        unsigned char* cam =
            static_cast<unsigned char*>(app->state.cameraRecordArray);
        for (int idx = 0; idx * 172 < 1720000; ++idx) {
            if (*reinterpret_cast<std::int32_t*>(cam + 172 * idx + 84) == sel) {
                Sub43CCD0(hDlg, idx);  // 0x43CCD0
                app->state.selAcc = idx;
            }
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 736)) {
        // bone combo selection (0x465D0E)
        Sub4204F0(app);  // 0x4204F0
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 736), 0x147 /*CB_GETCURSEL*/, 0, 0));
        mikudancestudio::mdl::BoneRecord* bone =
            reinterpret_cast<mikudancestudio::mdl::BoneRecord*>(
                app->state.boneRecordArray);
        for (int idx = 0; idx * 140 < 1400000; ++idx) {
            if (*reinterpret_cast<std::int32_t*>(bone + 140 * idx + 132) == sel) {
                Sub4214A0(hDlg, idx);  // 0x4214A0
                app->state.sel8c = idx;
            }
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 741)) {
        // camera "from" combo (0x465D91): map sel -> record frame field +28
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 741), 0x147 /*CB_GETCURSEL*/, 0, 0));
        unsigned char* cam =
            static_cast<unsigned char*>(app->state.cameraRecordArray);
        mikudancestudio::mdl::BoneRecord* bone =
            reinterpret_cast<mikudancestudio::mdl::BoneRecord*>(
                app->state.boneRecordArray);
        unsigned char* target =
            reinterpret_cast<unsigned char*>(
                bone + app->state.sel8c) + 28;
        for (int grp = 0, v = 2; v - 2 < 10000; ++grp, v += 5) {
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 84) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v - 2;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 256) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v - 1;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 428) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 600) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v + 1;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 772) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v + 2;
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 742)) {
        // camera "to" combo (0x465EC5): same mapping into record field +32
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 742), 0x147 /*CB_GETCURSEL*/, 0, 0));
        unsigned char* cam =
            static_cast<unsigned char*>(app->state.cameraRecordArray);
        mikudancestudio::mdl::BoneRecord* bone =
            reinterpret_cast<mikudancestudio::mdl::BoneRecord*>(
                app->state.boneRecordArray);
        unsigned char* target =
            reinterpret_cast<unsigned char*>(
                bone + app->state.sel8c) + 32;
        for (int grp = 0, v = 2; v - 2 < 10000; ++grp, v += 5) {
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 84) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v - 2;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 256) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v - 1;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 428) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 600) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v + 1;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 772) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v + 2;
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 707)) {
        // camera start-frame combo (0x46600B)
        unsigned char* cam =
            static_cast<unsigned char*>(app->state.cameraRecordArray);
        *reinterpret_cast<std::int32_t*>(
            cam + 172 * app->state.selAcc + 28) =
            static_cast<std::uint16_t>(
                SendMessageA(GetDlgItem(hDlg, 707), 0x147 /*CB_GETCURSEL*/, 0, 0)) -
            1;
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 708)) {
        // camera end-frame combo (0x466059)
        unsigned char* cam =
            static_cast<unsigned char*>(app->state.cameraRecordArray);
        cam[172 * app->state.selAcc + 32] =
            static_cast<std::uint8_t>(
                SendMessageA(GetDlgItem(hDlg, 708), 0x147 /*CB_GETCURSEL*/, 0, 0));
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 743)) {
        Sub420DC0(hDlg);  // 0x420DC0
        return 0;
    }
    return 0;
}

}  // namespace mikudancestudio
