// ===========================================================================
// VA 0x00463640 - EditCommit  (original: sub_463640, 0xB2A bytes)
// VA 0x0044BEF0 - EditCommitTail  (original: sub_44BEF0)
// VA 0x00464170 - EditSubclassProc  (original: sub_464170)
// VA 0x0040EC90 - TrackbarSubclassProc  (original: sub_40EC90)
// ===========================================================================
// sub_464170 is the wndproc installed on the 29 numeric EDIT controls of
// sub_466D20 (0x467BE4..0x46A8E7): on VK_RETURN it commits the edit
// (sub_463640) and returns focus to the main window; on WM_KILLFOCUS it
// commits with the newly focused window as the argument.  Everything else
// forwards to the captured EDIT class proc at app+0xA08D0 (stored once, at
// the first install site 0x467BD7 - all 29 controls share the class proc).
//
// sub_40EC90 is the wndproc installed on the 13 msctls_trackbar32 controls:
// WM_LBUTTONUP merely refocuses the main window (keyboard shortcuts keep
// working after a slider drag), everything forwards to the trackbar class
// proc captured once at 0x4686E2 into app+0xA0CDC.
//
// sub_463640 dispatches per control id (argument = the edit's HWND):
//   417  frame number  - atol -> app+0x980; >0x80000000 resets to 0/"0";
//        Sub432FA0 + PostViewRefresh tail
//   461-466  center/rotation mirrors - parsed value scaled into the work
//        record (647588..96 / 647540..48) and echoed into the paired
//        trackbar (455..460) via TBM_SETPOS; RefreshRequest(-2) tail
//   448  fov degrees -> app+647656, slider 447 echo, projection rebuild
//        (PerspectiveFovLH fov*0.01745329238474369, aspect from the
//        wrapper at +0x1D4EC, zn 1.0, zf 100000.0) + SetTransform slot 3;
//        RefreshRequest(-1) tail
//   478-485  light-accessory fields (gated on accessory != null; index
//        byte app+647536 into the 0x9DD70 array): rgb 532/536/540 direct,
//        direction 544/548/552 as deg*PI/180 (PI = 3.141592025756836 -
//        the float-truncated literal), 556 direct, 485 clamps to [0,1]
//        into +1184 and rewrites the edit text "%3.2f"; tail
//        RefreshRequest(acc index)
//   561  -> app+658732 = (10000-v)/100000, slider 560 echo, -3 tail
//   506/511/516/521  morph values into the 136-stride morph records of the
//        active model (slot app+0x910 into app+0x780; index at model+
//        11676/80/84/88, records at model+9924, value at +48), sliders
//        505/510/515/520 echo x100
// Every path ends in sub_44BEF0, which additionally owns the camera/bone
// position-rotation edits 544..550 (bone mode writes the 604-stride bone
// record +320..328/+332 via the Z*X*Y euler composition and the dirty mark
// at model+11672; camera mode writes app+820..828/784..792/657628) and
// finishes with RefreshRequest(-1)+PostViewRefresh when it handled one.
//
// Reference: ../translated/MikuMikuDance/fcn_00463640.cpp (rough), live
// IDA decompilation + disassembly of 0x463640/0x44BEF0/0x464170/0x40EC90
// (the asm is authoritative for the subclass-proc slots: Hex-Rays prints
// decoy indices for both Block+... reads).
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/accessory_layout.hpp"
#include "mikudancestudio/d3dx_dyn.hpp"

namespace mikudancestudio {

// Dependencies still stubbed (stubs.cpp): Sub432FA0 frame-apply refresh
// chain, Sub42D6E0 bone-edit keyframe register.
void Sub432FA0(MMDApp* app);                       // VA 0x00432FA0
void Sub42D6E0(MMDApp* app);                       // VA 0x0042D6E0
void RefreshRequest(int area);                     // VA 0x00440AC0 (ui_refresh)

// Deg->rad literal of 0x463640/0x44BEF0: dbl_52BB20 bits 0x3F91DF46A0000000
// (also used by the projection rebuild at 0x4639D5).
static constexpr double kDegToRad = 0.01745329238474369;
// PI literal of the euler conversions (float-truncated 24-bit mantissa).
static constexpr double kPiLit = 3.141592025756836;

namespace {

// Active model = slot array app+0x780 indexed by byte app+0x910.
unsigned char* SlotModel(MMDApp* app) {
    return app->SelectedModel();
}

// Light accessory = array app+0x9DD70 indexed by byte app+0x9E170.
mdl::AccessoryRecord* LightAccessory(MMDApp* app) {
    return app->AccessorySlot(app->SelectedAccessorySlot());
}

// Z*X*Y euler composition of the 547/548/549 bone-rotation commits:
// D3DXMatrixRotationZ x rotX x rotY, result quaternion into the bone record
// (+332 inside the 604-stride bone array).
void ComposeEulerToBone(MMDApp* app, unsigned char* model, int sel) {
    d3dx::D3DXMATRIXF rot{};
    d3dx::D3DXMATRIXF tmp{};
    auto* d3dx = &d3dx::Get();
    if (!d3dx->Load())
        return;
    d3dx->rotZ(&rot, app->state.eulerZ);
    d3dx->rotX(&tmp, app->state.eulerX);
    d3dx->multiply(&rot, &rot, &tmp);
    d3dx->rotY(&tmp, app->state.eulerY);
    d3dx->multiply(&rot, &rot, &tmp);
    mikudancestudio::mdl::BoneRecord* bones = mikudancestudio::mdl::Bones(model);
    d3dx->quatFromMatrix(reinterpret_cast<float*>(&bones[sel].rotQuat[0]),
                         &rot);
    mikudancestudio::mdl::Mdl(model)->bonePhysicsState[sel] = 1;
}

}  // namespace

// ---------------------------------------------------------------------------
// VA 0x0044BEF0 - per-edit tail; owns ids 544..550 (see file header).
// ---------------------------------------------------------------------------
void Sub44BEF0(MMDApp* app, HWND edit) {
    HWND base = app->FloatingWindow();
    if (base == nullptr)
        base = app->MainWindow();
    const bool cameraMode = app->CameraMode() != 0;

    if (edit == GetDlgItem(base, 544)) {           // 0x44BF31
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        if (!cameraMode) {
            unsigned char* model = SlotModel(app);
            const int sel = mikudancestudio::mdl::Mdl(model)->selectedBone;
            if (sel >= 0) {
                Sub42D6E0(app);
                mikudancestudio::mdl::Bones(model)[sel].trans[0] = static_cast<float>(v);
                mikudancestudio::mdl::Mdl(model)->bonePhysicsState[sel] = 1;
            }
            PostViewRefresh(app);
            return;
        }
        app->CameraPositionX() = static_cast<float>(v);
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, 545)) {           // 0x44BFE5
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        if (!cameraMode) {
            unsigned char* model = SlotModel(app);
            const int sel = mikudancestudio::mdl::Mdl(model)->selectedBone;
            if (sel >= 0) {
                Sub42D6E0(app);
                mikudancestudio::mdl::Bones(model)[sel].trans[1] = static_cast<float>(v);
                mikudancestudio::mdl::Mdl(model)->bonePhysicsState[sel] = 1;
            }
            PostViewRefresh(app);
            return;
        }
        app->CameraPositionY() = static_cast<float>(v);
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, 546)) {           // 0x44C099
        char text[256];
        GetWindowTextA(edit, text, 100);
        double v = atof(text);
        if (!cameraMode) {
            unsigned char* model = SlotModel(app);
            const int sel = mikudancestudio::mdl::Mdl(model)->selectedBone;
            if (sel >= 0) {
                Sub42D6E0(app);
                mikudancestudio::mdl::Bones(model)[sel].trans[2] = static_cast<float>(v);
                mikudancestudio::mdl::Mdl(model)->bonePhysicsState[sel] = 1;
            }
            PostViewRefresh(app);
            return;
        }
        if (app->CameraParentModel() >= 0)         // attached-camera mirror
            v = -v;
        app->CameraPositionZ() = static_cast<float>(v);
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, 547)) {           // 0x44C158
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        if (!cameraMode) {
            unsigned char* model = SlotModel(app);
            const int sel = mikudancestudio::mdl::Mdl(model)->selectedBone;
            if (sel >= 0) {
                Sub42D6E0(app);
                app->state.eulerX =
                    static_cast<float>(v * kPiLit / 180.0);
                app->state.eulerY =
                    static_cast<float>(-static_cast<double>(
                                           app->state.eulerY) *
                                       kPiLit / 180.0);
                app->state.eulerZ =
                    static_cast<float>(kPiLit *
                                       -static_cast<double>(
                                           app->state.eulerZ) /
                                       180.0);
                ComposeEulerToBone(app, model, sel);
            }
            PostViewRefresh(app);
            return;
        }
        app->CameraPitch() = static_cast<float>(-v * kPiLit / 180.0);
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, 548)) {           // 0x44C2C7
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        if (cameraMode) {
            app->CameraYaw() = static_cast<float>(v * kPiLit / 180.0);
            RefreshRequest(-1);
            PostViewRefresh(app);
            return;
        }
        unsigned char* model = SlotModel(app);
        const int sel = mikudancestudio::mdl::Mdl(model)->selectedBone;
        if (sel >= 0) {
            Sub42D6E0(app);
            app->state.eulerX =      // re-convert stored
                static_cast<float>(                        // degrees
                    static_cast<double>(
                        app->state.eulerX) *
                    kPiLit / 180.0);
            app->state.eulerY =
                static_cast<float>(-v * kPiLit / 180.0);
            app->state.eulerZ =
                static_cast<float>(kPiLit *
                                   -static_cast<double>(
                                       app->state.eulerZ) /
                                   180.0);
            ComposeEulerToBone(app, model, sel);
        }
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, 549)) {           // 0x44C434
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        if (cameraMode) {
            app->CameraRoll() = static_cast<float>(v * kPiLit / 180.0);
            RefreshRequest(-1);
            PostViewRefresh(app);
            return;
        }
        unsigned char* model = SlotModel(app);
        const int sel = mikudancestudio::mdl::Mdl(model)->selectedBone;
        if (sel >= 0) {
            Sub42D6E0(app);
            app->state.eulerX =
                static_cast<float>(
                    static_cast<double>(
                        app->state.eulerX) *
                    kPiLit / 180.0);
            app->state.eulerY =
                static_cast<float>(-static_cast<double>(
                                       app->state.eulerY) *
                                   kPiLit / 180.0);
            app->state.eulerZ =
                static_cast<float>(kPiLit * -v / 180.0);
            ComposeEulerToBone(app, model, sel);
        }
        PostViewRefresh(app);
        return;
    }
    if (edit == GetDlgItem(base, 550) && cameraMode) {  // 0x44C561
        char text[256];
        GetWindowTextA(edit, text, 100);
        const double v = atof(text);
        app->CameraDistance() = static_cast<float>(-v);
        RefreshRequest(-1);
        PostViewRefresh(app);
        return;
    }
    PostViewRefresh(app);                           // 0x44C5B1 fallthrough
}

// ---------------------------------------------------------------------------
// VA 0x00463640 - edit commit (see file header for the per-id behaviour).
// ---------------------------------------------------------------------------
void Sub463640(MMDApp* app, HWND edit) {
    auto& s = *app;
    const HWND main = s.MainWindow();
    s.state.bC = 1;    // 0x463672 dword store

    if (edit == GetDlgItem(main, 417)) {            // frame number
        char text[256];
        GetWindowTextA(edit, text, 8);
        const long v = atol(text);
        s.CurrentFrame() = static_cast<std::int32_t>(v);
        if (static_cast<unsigned long>(v) > 0x80000000UL) {
            s.CurrentFrame() = 0;
            SetWindowTextA(edit, "0");
        }
        Sub432FA0(app);
        PostViewRefresh(app);
        Sub44BEF0(app, edit);
        return;
    }
    if (edit == GetDlgItem(main, 461) ||            // center x
        edit == GetDlgItem(main, 462) || edit == GetDlgItem(main, 463)) {
        char text[256];
        GetWindowTextA(edit, text, 8);
        const double v = static_cast<double>(atol(text)) * 0.00390625;
        const int id = edit == GetDlgItem(main, 461) ? 461
                     : edit == GetDlgItem(main, 462) ? 462 : 463;
        s.LightColor()[id - 461] = static_cast<float>(v);
        SendMessageA(GetDlgItem(main, 455 + (id - 461)), 0x405, 1,
                     static_cast<LPARAM>(static_cast<int>(v * 256.0)));
        RefreshRequest(-2);
        Sub44BEF0(app, edit);
        return;
    }
    if (edit == GetDlgItem(main, 464) ||            // rot x/y/z mirror
        edit == GetDlgItem(main, 465) || edit == GetDlgItem(main, 466)) {
        char text[256];
        GetWindowTextA(edit, text, 8);
        const double v = atof(text);
        const int id = edit == GetDlgItem(main, 464) ? 464
                     : edit == GetDlgItem(main, 465) ? 465 : 466;
        s.LightDirection()[id - 464] = static_cast<float>(v);
        SendMessageA(GetDlgItem(main, 458 + (id - 464)), 0x405, 1,
                     static_cast<LPARAM>(static_cast<int>(v * 100.0)));
        RefreshRequest(-2);
        Sub44BEF0(app, edit);
        return;
    }
    if (edit == GetDlgItem(main, 448)) {            // fov + projection
        char text[256];
        GetWindowTextA(edit, text, 8);
        const float v = static_cast<float>(atol(text));
        s.CameraFov() = v;
        SendMessageA(GetDlgItem(main, 447), 0x405, 1,
                     static_cast<LPARAM>(static_cast<int>(v)));
        D3DRenderer* r = s.Renderer();
        const float fovRad = static_cast<float>(
            static_cast<double>(s.CameraFov()) * kDegToRad);
        d3dx::D3DXMATRIXF mat{};
        auto* d3dx = &d3dx::Get();
        if (d3dx->Load())
            d3dx->perspectiveFovLH(&mat, fovRad, r->aspectRatio, 1.0f,
                                   100000.0f);
        IDirect3DDevice9* dev = r->device;
        dev->SetTransform(D3DTS_PROJECTION,
                          reinterpret_cast<const D3DMATRIX*>(&mat));
        RefreshRequest(-1);
        Sub44BEF0(app, edit);
        return;
    }
    // 478..485: light-accessory fields (id = 0x1DE..0x1E5).
    {
        mdl::AccessoryRecord* acc = LightAccessory(app);
        int lightId = 0;
        if (edit == GetDlgItem(main, 478)) lightId = 478;
        else if (edit == GetDlgItem(main, 479)) lightId = 479;
        else if (edit == GetDlgItem(main, 480)) lightId = 480;
        else if (edit == GetDlgItem(main, 481)) lightId = 481;
        else if (edit == GetDlgItem(main, 482)) lightId = 482;
        else if (edit == GetDlgItem(main, 483)) lightId = 483;
        else if (edit == GetDlgItem(main, 484)) lightId = 484;
        else if (edit == GetDlgItem(main, 485)) lightId = 485;
        if (lightId != 0 && acc != nullptr) {
            char text[256];
            GetWindowTextA(edit, text, 8);
            const double v = atof(text);
            if (lightId <= 480) {
                acc->position[lightId - 478] = static_cast<float>(v);
            } else if (lightId <= 483) {
                acc->rotation[lightId - 481] =
                    static_cast<float>(v / 180.0 * kPiLit);
            } else if (lightId == 484) {
                acc->scale = static_cast<float>(v);
            } else {                                 // 485: clamp + rewrite
                // v7 = 0; if (v < 0 || (v7 = 1, v > 1)) v = v7
                double out = v;
                if (v < 0.0)
                    out = 0.0;
                else if (v > 1.0)
                    out = 1.0;
                acc->opacity = static_cast<float>(out);
                const HWND same = GetDlgItem(main, 485);
                const LPARAM len = GetWindowTextLengthA(same);
                SendMessageA(same, 0xB1 /*EM_SETSEL*/, 0, len);
                char fmt[256];
                sprintf_s(fmt, 0x100, "%3.2f", out);
                SendMessageA(same, 0xC2 /*WM_SETTEXT*/, 0,
                             reinterpret_cast<LPARAM>(fmt));
            }
            RefreshRequest(s.SelectedAccessorySlot());
            Sub44BEF0(app, edit);
            return;
        }
        if (lightId != 0) {                          // acc == null: tail only
            RefreshRequest(s.SelectedAccessorySlot());
            Sub44BEF0(app, edit);
            return;
        }
    }
    if (edit == GetDlgItem(main, 561)) {            // 0x231 shadow range
        char text[256];
        GetWindowTextA(edit, text, 8);
        const double v = atof(text);
        s.state.physicsInterval =   // 0xA0D2C
            static_cast<float>((10000.0 - v) / 100000.0);
        SendMessageA(GetDlgItem(main, 560), 0x405, 1,
                     static_cast<LPARAM>(static_cast<int>(v)));
        RefreshRequest(-3);
        Sub44BEF0(app, edit);
        return;
    }
    // 506/511/516/521: morph values of the active model.  The four controls
    // address the four selector slots in order; their control IDs are spaced
    // by five, but their model fields are consecutive.
    {
        int morphEdit = 0;
        if (edit == GetDlgItem(main, 506)) morphEdit = 506;
        else if (edit == GetDlgItem(main, 511)) morphEdit = 511;
        else if (edit == GetDlgItem(main, 516)) morphEdit = 516;
        else if (edit == GetDlgItem(main, 521)) morphEdit = 521;
        if (morphEdit != 0) {
            char text[256];
            GetWindowTextA(edit, text, 8);
            const double v = atof(text);
            unsigned char* model = SlotModel(app);
            const int selector =
                mikudancestudio::mdl::Mdl(model)->selectedMorphs[
                    (morphEdit - 506) / 5];
            if (mikudancestudio::mdl::Morphs(model) != nullptr && selector >= 0)
                mikudancestudio::mdl::Morphs(model)[selector].value =
                    static_cast<float>(v);
            SendMessageA(GetDlgItem(main, morphEdit - 1), 0x405, 1,
                         static_cast<LPARAM>(static_cast<int>(v * 100.0)));
            Sub44BEF0(app, edit);
            return;
        }
    }
    Sub44BEF0(app, edit);                           // default tail
}

// ---------------------------------------------------------------------------
// VA 0x00464170 - EDIT subclass proc.
// ---------------------------------------------------------------------------
LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT Msg, WPARAM wParam,
                                  LPARAM lParam) {
    MMDApp* app = g_Block;
    if (Msg == WM_KEYDOWN) {
        if (wParam == 13) {                          // VK_RETURN
            Sub463640(app, hWnd);
            SetFocus(app->MainWindow());
            return 0;
        }
    } else if (Msg == WM_KILLFOCUS) {
        Sub463640(app, reinterpret_cast<HWND>(wParam));
    }
    return CallWindowProcA(
        app->OriginalEditProc(),
        hWnd, Msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// VA 0x0040EC90 - trackbar subclass proc.
// ---------------------------------------------------------------------------
LRESULT CALLBACK TrackbarSubclassProc(HWND hWnd, UINT Msg, WPARAM wParam,
                                      LPARAM lParam) {
    MMDApp* app = g_Block;
    if (Msg == WM_LBUTTONUP)
        SetFocus(app->MainWindow());
    return CallWindowProcA(
        app->OriginalTrackbarProc(),
        hWnd, Msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// The 42 SetWindowLongPtrA(GWLP_WNDPROC) install sites of sub_466D20
// (0x467BE4..0x46A8E7), in original per-site order.  The first EDIT (417)
// additionally captures the EDIT class proc into app+0xA08D0 (0x467BD7) and
// the first trackbar (447) captures the trackbar class proc into
// app+0xA0CDC (0x4686E2); every later site only installs.
// ---------------------------------------------------------------------------
void InstallControlSubclasses(MMDApp* app, HWND hwnd) {
    static const int kEditIds[] = {
        417, 448, 461, 462, 463, 464, 465, 466,
        478, 479, 480, 481, 482, 483, 484, 485,
        506, 511, 516, 521, 544, 545, 546, 547, 548, 549, 550, 554, 561,
    };
    static const int kTrackIds[] = {
        447, 455, 456, 457, 458, 459, 460,
        505, 510, 515, 520, 534, 560,
    };

    HWND firstEdit = GetDlgItem(hwnd, 417);
    app->OriginalEditProc() = reinterpret_cast<WNDPROC>(
        GetWindowLongPtrA(firstEdit, GWLP_WNDPROC));     // 0x467BD7
    SetWindowLongPtrA(firstEdit, GWLP_WNDPROC,
                   reinterpret_cast<LONG_PTR>(EditSubclassProc));

    HWND firstTrack = GetDlgItem(hwnd, 447);
    app->OriginalTrackbarProc() = reinterpret_cast<WNDPROC>(
        GetWindowLongPtrA(firstTrack, GWLP_WNDPROC));    // 0x4686E2
    SetWindowLongPtrA(firstTrack, GWLP_WNDPROC,
                   reinterpret_cast<LONG_PTR>(TrackbarSubclassProc));

    for (int id : kEditIds) {
        if (id == 417)
            continue;
        SetWindowLongPtrA(GetDlgItem(hwnd, id), GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(EditSubclassProc));
    }
    for (int id : kTrackIds) {
        if (id == 447)
            continue;
        SetWindowLongPtrA(GetDlgItem(hwnd, id), GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(TrackbarSubclassProc));
    }
}

void InstallControlSubclass(MMDApp* app, HWND control, int id) {
    static const int kEditIds[] = {
        417, 448, 461, 462, 463, 464, 465, 466,
        478, 479, 480, 481, 482, 483, 484, 485,
        506, 511, 516, 521, 544, 545, 546, 547, 548, 549, 550, 554, 561,
    };
    static const int kTrackIds[] = {
        447, 455, 456, 457, 458, 459, 460,
        505, 510, 515, 520, 534, 560,
    };
    if (control == nullptr)
        return;
    for (int editId : kEditIds) {
        if (id != editId)
            continue;
        if (id == 417) {
            app->OriginalEditProc() = reinterpret_cast<WNDPROC>(
                GetWindowLongPtrA(control, GWLP_WNDPROC));
        }
        SetWindowLongPtrA(control, GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(EditSubclassProc));
        return;
    }
    for (int trackId : kTrackIds) {
        if (id != trackId)
            continue;
        if (id == 447) {
            app->OriginalTrackbarProc() = reinterpret_cast<WNDPROC>(
                GetWindowLongPtrA(control, GWLP_WNDPROC));
        }
        SetWindowLongPtrA(control, GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(TrackbarSubclassProc));
        return;
    }
}

}  // namespace mikudancestudio
