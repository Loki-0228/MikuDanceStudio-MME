// ===========================================================================
// Frame-driver interaction modes (0x0046B090 region @ 0x470Cxx..0x4712xx)
// ===========================================================================
// Verified raw offsets (Ghidra ground truth):
//   this+4/+8       previous mouse X/Y      this+0xC/+0x10 current mouse X/Y
//   this+0x24 (36)  precision selector A    this+0xC0 (192) selector B
//   this+0x88 (136) transform selector A    this+0x8C (140) selector B
//   this+0x2F8 (760) model-mode flag        this+0x330 (816) anim gate
//   this+0x308/30C/310 camera translate X/Y/Z
//   this+0x314/318  camera rotation angles
//   this+0x9EDD1 (650705) drag-active flag
//   this+0x9ED98 (650648) select-state gate
//   this+0x910 (2320) frame index  this+0xA0430 (656432) frame cursor
//   this+0xA05D1 (656849) view-dirty  this+0xA442C (672812) view-lock
//
// Camera-translate block ported verbatim from the raw listing at 4638..4686:
//   gate: dragActive & (this+0x8C == 3) & !modelMode
//   scale: selector A(this+0x24==3) -> 0.01, selector B(this+0xC0==3) ->
//          PI/180, else g_MouseScaleC
//   camY += (prevY - curY) * scale;  camX -= (prevX - curX) * scale
//   then 0x40D070/0x40D130 view refresh under the frame-cursor gate.
//
// Runtime float globals (file image differs -> initialized at runtime in the
// original; initializer not yet located - values from the translated
// annotations, deviation recorded in ARCHITECTURE.md):
//   0x52B8F0 = 0.01f   0x52E9C0 = PI/180   0x52E8C0 = PI/360
//   0x52D738 = best-evidence PI/360 (unknown, TODO(port))
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "mikudancestudio/d3dx_dyn.hpp"
#include "mikudancestudio/globals.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "frame_state_dump.hpp"

namespace mikudancestudio {

// Exact doubles from the original image.  These stay double until the final
// store, matching the x87 instruction stream at 0x470E4B..0x470EDF.
double g_MouseScaleA = 0.5;                       // VA 0x0052B8F0
double g_MouseScaleB = 0.004999999888241291;      // VA 0x0052E9C0
double g_MouseScaleC = 0.05000000074505806;       // VA 0x0052D738
float g_MouseScaleD = 0.008726646f;       // VA 0x0052E8C0 (PI/360)

constexpr double kRotateScale = 0.009999999776482582;  // VA 0x52E9C8
constexpr double kRightModifiedScale = 0.10000000149011612; // 0x52BEA8

static void ViewRefreshGate(MMDApp* s) {
    // Original dword gates (Ghidra: this+0x9ED98 select-state gate,
    // this+0xA0430 frame cursor).  The old literals 650136/657456 were
    // decimal slips - 650136 landed inside aviOutputPath and 657456 inside
    // exeDir.  0x9ED98 is a dword over v9ed98 and the two shadow bytes;
    // 0x2F8 is a dword over the first four radio flags.
    const bool frameGate =
        (s->state.v9ed98 | s->state.playbackStartsAtCurrentFrame |
         s->state.projectedShadowBlendEnabled) != 0 &&
        s->state.slotIdx == s->state.cameraParentModel &&
        (s->state.optflag[0] | s->state.optflag[1] |
         s->state.optflag[2] | s->state.optflag[3]) == 0 &&
        s->state.cameraParentModel >= 0 &&
        s->state.playbackActive == 0;
    if (frameGate) {
        // 0xA05D1 = b6568481 view-dirty byte (the old literal 658257 was a
        // decimal slip that landed on the frame-range dialog HWND's byte 1)
        s->state.b6568481 = 1;                                   // 0xA05D1
        s->state.windowLayoutReady = 0;
        PostLanguageSweep2(s);                                    // 0x40D070
        s->state.windowLayoutReady = 1;
    }
    PostViewRefresh(s);                                           // 0x40D130
}

namespace {

void PollKey(MMDApp* app, std::int32_t& keyState, int virtualKey) {
    const std::int32_t oldState = keyState;
    const bool down = (GetKeyState(virtualKey) & 0x8000) != 0;
    std::int32_t state = 0;
    if (down)
        state = oldState == 0 ? 1 : 3;
    else if (oldState == 1 || oldState == 3)
        state = 2;
    keyState = state;
    if (state == 1 || state == 2)
        app->state.messageSeen = 1;
}

float DragScale(MMDApp* app) {
    if (app->ShiftModifierActive())
        return 0.5f;                            // flt_52960C / dbl_52B8F0
    if (app->CtrlModifierActive())
        return 0.005f;                          // flt_529604 / dbl_52E9C0
    return 0.05f;                               // flt_52E9A4 / dbl_52D738
}

void PanCameraPosition(MMDApp* app, int dx, int dy) {
    const float scale = DragScale(app);
    auto& api = d3dx::Get();
    api.Load();
    if (api.rotX != nullptr && api.rotY != nullptr &&
        api.rotZ != nullptr && api.multiply != nullptr) {
        d3dx::D3DXMATRIXF mx{}, my{}, mz{}, matrix{};
        api.rotY(&my, app->CameraYaw());
        api.rotX(&mx, app->CameraPitch());
        api.multiply(&matrix, &mx, &my);
        api.rotZ(&mz, -app->CameraRoll());
        api.multiply(&matrix, &mz, &matrix);

        const float sx = static_cast<float>(-dx) * scale;
        const float sy = static_cast<float>(dy) * scale;
        app->CameraPositionX() += sx * matrix.m[0][0] +
                                  sy * matrix.m[1][0] + matrix.m[3][0];
        app->CameraPositionY() += sx * matrix.m[0][1] +
                                  sy * matrix.m[1][1] + matrix.m[3][1];
        app->CameraPositionZ() -= sx * matrix.m[0][2] +
                                  sy * matrix.m[1][2] + matrix.m[3][2];
        return;
    }

    // The original cannot start without d3dx9_32.dll.  Keep the restored
    // build operable in that documented fallback environment.
    app->CameraPositionX() -= static_cast<float>(dx) * scale;
    app->CameraPositionY() += static_cast<float>(dy) * scale;
}

void PanSelectedCameraKey(MMDApp* app, int dx, int dy) {
    double pixelScale = 0.0000199999994947575;
    if (app->ShiftModifierActive())
        pixelScale = 0.000199999994947575;
    else if (app->CtrlModifierActive())
        pixelScale = 0.00000199999999495049;

    const float depth = std::fabs(app->CameraPositionZ());
    const float depthFactor = depth <= 0.3f
        ? 10.0f
        : std::max(depth, 1.0f);
    const double scale = pixelScale * static_cast<double>(depthFactor) *
        static_cast<double>(app->CameraFov());
    app->CameraPositionX() = static_cast<float>(
        app->CameraPositionX() - static_cast<double>(dx) * scale);
    app->CameraPositionY() = static_cast<float>(
        app->CameraPositionY() + static_cast<double>(dy) * scale);
}

int ViewportToolAtPoint(MMDApp* app) {
    app->ViewportToolHovered() = 0;
    app->ViewportToolOperation() = ViewportToolAction::None;
    if (app->ViewportInputActive() == 0)
        return 0;

    const int x = app->MouseX();
    const int y = app->MouseY();
    const RECT view = app->ViewportRect();
    HWND window = app->FloatingWindow();
    if (window == nullptr)
        window = app->state.hwnd;
    RECT client{};
    GetClientRect(window, &client);

    int operation = 0;
    if (y > view.top - 25 && y < view.top) {
        if (x > client.right - 60 && x < client.right - 36)
            operation = 1;
        else if (x > client.right - 30 && x < client.right - 6)
            operation = 2;
    }

    float scale = 1.0f;
    if (D3DRenderer* r = app->Renderer())
        scale = r->viewScale;  // wrapper+120048
    if (!(scale > 0.0f))
        scale = 1.0f;

    if (operation == 0 && app->PlaybackActive() == 0) {
        const float fx = static_cast<float>(x);
        const float fy = static_cast<float>(y);
        const float x0 = view.right - 127.0f * scale;
        const float x1 = view.right - 92.0f * scale;
        const float x2 = view.right - 86.0f * scale;
        const float x3 = view.right - 51.0f * scale;
        const float x4 = view.right - 48.0f * scale;
        const float x5 = view.right - 13.0f * scale;
        const float upperTop = view.bottom - 80.0f * scale;
        const float upperBottom = view.bottom - 50.0f * scale;
        const float lowerTop = view.bottom - 40.0f * scale;
        const float lowerBottom = view.bottom - 10.0f * scale;
        int column = -1;
        if (fx > x0 && fx < x1)
            column = 0;
        else if (fx > x2 && fx < x3)
            column = 1;
        else if (fx > x4 && fx < x5)
            column = 2;
        if (column >= 0) {
            const bool modelPanel =
                app->state.optflag[0] != 0;
            const std::uint32_t axisMode =
                app->state.v32c;
            if (fy > upperTop && fy < upperBottom) {
                if (modelPanel)
                    operation = 18 + column;
                else if (axisMode == 0 || axisMode == 1)
                    operation = 9 + column;
            } else if (fy > lowerTop && fy < lowerBottom) {
                if (modelPanel)
                    operation = 15 + column;
                else if (axisMode == 0 || axisMode == 2)
                    operation = 12 + column;
            }
        }

        if (operation == 0 && app->EditMode() == ViewportEditMode::Camera) {
            const int cx = app->ViewportToolCenterX();
            const int cy = app->ViewportToolCenterY();
            const int px = x - cx;
            const int py = y - cy;
            const bool horizontal =
                ((px > 7 && px < 35) || (px > -35 && px < -7)) &&
                py > -7 && py < 7;
            const bool vertical =
                ((py > 7 && py < 35) || (py > -35 && py < -7)) &&
                px > -7 && px < 7;
            if (horizontal)
                operation = 3;
            else if (vertical)
                operation = 4;
            else {
                const int radius2 = px * px + py * py;
                if (radius2 > 1225 && radius2 < 2025)
                    operation = 5;
            }
        } else if (operation == 0 &&
                   app->EditMode() == ViewportEditMode::Light) {
            const int px = x - app->ViewportToolCenterX();
            const int py = y - app->ViewportToolCenterY();
            if (px > 41 && px < 61 && py > -8 && py < 8)
                operation = 6;
            else if (px > -13 && px < 8 && py > -61 && py < -42)
                operation = 7;
            else if (px > -13 && px < 11 && py > -16 && py < 8)
                operation = 8;
        }

        if (operation == 0 &&
            app->InteractionDragMode() == ViewportDragMode::None) {
            const float coordLeft = view.right - 120.0f * scale;
            const float coordTop = view.bottom - 105.0f * scale;
            if (fx > coordLeft && fx < coordLeft + 25.0f * scale &&
                fy > coordTop && fy < coordTop + 25.0f * scale)
                operation = 21;
        }
    }

    if (operation != 0) {
        app->ViewportToolHovered() = 1;
        app->ViewportToolOperation() =
            static_cast<ViewportToolAction>(operation);
    }
    return operation;
}

HWND ViewportWindow(MMDApp* app) {
    HWND window = app->FloatingWindow();
    if (window == nullptr)
        window = app->state.hwnd;
    return window;
}

float ViewportScale(MMDApp* app) {
    D3DRenderer* r = app->Renderer();
    const float scale = r != nullptr ? r->viewScale : 1.0f;  // +120048
    return scale > 0.0f ? scale : 1.0f;
}

void WarpCursor(MMDApp* app, POINT point) {
    HWND window = ViewportWindow(app);
    if (window == nullptr)
        return;
    ClientToScreen(window, &point);
    SetCursorPos(point.x, point.y);
    app->state.v9f12c = 1;
}

void BeginCenteredDrag(MMDApp* app, int operation) {
    const RECT view = app->ViewportRect();
    POINT point{(view.left + view.right) / 2,
                (view.top + view.bottom) / 2};
    app->MouseX() = point.x;
    app->MouseY() = point.y;
    app->PreviousMouseX() = point.x;
    app->PreviousMouseY() = point.y;
    app->ViewToolDragOperation() =
        static_cast<ViewportToolAction>(operation);
    app->state.b6568483 = 0;
    WarpCursor(app, point);
    while (ShowCursor(FALSE) >= 0) {}
}

void RestoreViewportToolCursor(MMDApp* app, int operation) {
    const RECT view = app->ViewportRect();
    const float scale = ViewportScale(app);
    POINT point{};
    if (operation == 1 || operation == 2) {
        HWND mainWindow = app->state.hwnd;
        RECT client{};
        GetClientRect(mainWindow, &client);
        point.x = client.right - (operation == 1 ? 48 : 18);
        point.y = view.top - 12;
    } else {
        const int column = operation == 9 || operation == 12 ||
                           operation == 15 || operation == 18 ? 0
                         : operation == 10 || operation == 13 ||
                           operation == 16 || operation == 19 ? 1 : 2;
        const bool upper = operation == 9 || operation == 10 ||
                           operation == 11 || operation == 18 ||
                           operation == 19 || operation == 20;
        static constexpr float kCenterX[3] = {110.0f, 70.0f, 30.0f};
        point.x = view.right - static_cast<int>(kCenterX[column] * scale);
        point.y = view.bottom - static_cast<int>((upper ? 65.0f : 25.0f) * scale);
    }
    WarpCursor(app, point);
    while (ShowCursor(TRUE) < 0) {}
}

unsigned char* ActiveBoneModel(MMDApp* app) {
    return app->ModelSlot(app->state.slotIdx);
}

bool BoneCanBePicked(MMDApp* app, const unsigned char* bone) {
    if ((mdl::At<std::uint32_t>(const_cast<unsigned char*>(bone), 500) & 8u) == 0)
        return false;
    const std::uint8_t type = bone[484];
    const int physicsMode = app->PlaybackPhysicsMode();
    if (physicsMode == 2 && type != 0 && bone[493] == 0)
        return false;
    if (physicsMode == 1 && type != 0)
        return false;
    return true;
}

void PickBoneAtCursor(MMDApp* app) {
    unsigned char* model = ActiveBoneModel(app);
    if (model == nullptr)
        return;
    auto* bones = mikudancestudio::mdl::Bones(model);
    auto* selected = mikudancestudio::mdl::Mdl(model)->boneSelection;
    const int count = mikudancestudio::mdl::Mdl(model)->boneCount;
    if (bones == nullptr || selected == nullptr || count <= 0)
        return;

    const int x = app->MouseX();
    const int y = app->MouseY();
    const bool additive = app->ShiftModifierActive();
    int candidate = -1;
    bool changed = false;
    for (int i = 0; i < count; ++i) {
        unsigned char* bone = mikudancestudio::mdl::BoneBytes(bones, i);
        if (!BoneCanBePicked(app, bone))
            continue;
        const int dx = mdl::At<std::int32_t>(bone, 452) - x;
        const int dy = mdl::At<std::int32_t>(bone, 456) - y;
        if (dx * dx + dy * dy >= 64)
            continue;
        if (additive) {
            selected[i] = selected[i] == 0 ? 1 : 0;
            if (selected[i] != 0)
                mikudancestudio::mdl::Mdl(model)->selectedBone = i;
            else if (mikudancestudio::mdl::Mdl(model)->selectedBone == i)
                mikudancestudio::mdl::Mdl(model)->selectedBone = -1;
            changed = true;
        } else {
            candidate = std::max(candidate, i);
        }
    }
    if (!additive && candidate >= 0) {
        std::memset(selected, 0, static_cast<std::size_t>(count));
        selected[candidate] = 1;
        mikudancestudio::mdl::Mdl(model)->selectedBone = candidate;
        changed = true;
    }
    if (changed) {
        PostLanguageSweep(app);
        PostLanguageSweep2(app);
    }
}

void UpdateBoneBoxSelection(MMDApp* app) {
    if (app->BoneBoxSelectionActive() == 0)
        return;
    unsigned char* model = ActiveBoneModel(app);
    if (model == nullptr)
        return;
    auto* bones = mikudancestudio::mdl::Bones(model);
    auto* selected = mikudancestudio::mdl::Mdl(model)->boneSelection;
    const int count = mikudancestudio::mdl::Mdl(model)->boneCount;
    if (bones == nullptr || selected == nullptr || count <= 0)
        return;

    const int left = std::min(app->BoneBoxStartX(),
                              app->MouseX());
    const int right = std::max(app->BoneBoxStartX(),
                               app->MouseX());
    const int top = std::min(app->BoneBoxStartY(),
                             app->MouseY());
    const int bottom = std::max(app->BoneBoxStartY(),
                                app->MouseY());
    const bool additive = app->ShiftModifierActive();
    bool changed = false;
    for (int i = 0; i < count; ++i) {
        unsigned char* bone = mikudancestudio::mdl::BoneBytes(bones, i);
        const std::uint8_t type = bone[484];
        const bool eligible = type <= 6 || type == 8;
        const int x = mdl::At<std::int32_t>(bone, 452);
        const int y = mdl::At<std::int32_t>(bone, 456);
        const bool inside = eligible && x > left && x < right &&
                            y > top && y < bottom;
        const std::uint8_t next = inside ? 1 : (additive ? selected[i] : 0);
        changed |= selected[i] != next;
        selected[i] = next;
    }
    if (changed)
        PostLanguageSweep(app);
}

void BeginOrEndViewportToolDrag(MMDApp* app, int operation) {
    const int leftState = app->LeftMouseButtonState();
    if (leftState == 1) {
        if (operation == 1 || operation == 2) {
            BeginCenteredDrag(app, operation);
            return;
        }
        static constexpr std::uint8_t kDragMode[21] = {
            0, 0, 0, 1, 2, 3, 7, 8, 9, 4, 5, 6,
            10, 11, 12, 13, 14, 15, 16, 17, 18};
        if (operation >= 3 && operation <= 20) {
            const int mode = kDragMode[operation];
            if (operation >= 9)
                BeginCenteredDrag(app, operation);
            app->InteractionDragMode() = static_cast<ViewportDragMode>(mode);
            if (operation == 5) {
                app->ViewportToolDragOriginX() = app->ViewportToolCenterX();
                app->ViewportToolDragOriginY() = app->ViewportToolCenterY();
            }
            if (operation <= 8) {
                app->EditMode() = ViewportEditMode::ToolDrag;
                Sub42D6E0(app);
            } else if (operation <= 14) {
                Sub42D6E0(app);
                app->state.a06B5 = 1;
            } else {
                const int target = app->state.v9ed9c;
                RefreshRequest(target == 2
                    ? app->state.selLightAccSlotOrUint32 : -1);
                app->state.a06B5 = 1;
            }
        } else if (operation == 21) {
            int& target = app->state.v9ed9c;
            const int limit = app->state.optflag[0] != 0 ? 3 : 2;
            if (++target >= limit)
                target = 0;
            PostLanguageSweep(app);
        } else if (operation == 0 &&
                   app->PlaybackActive() == 0) {
            const RECT view = app->ViewportRect();
            const int x = app->MouseX();
            const int y = app->MouseY();
            if (x > view.left && x < view.right &&
                y > view.top && y < view.bottom) {
                const ViewportEditMode panelMode = app->EditMode();
                if (panelMode == ViewportEditMode::Bone)
                    PickBoneAtCursor(app);
                else if (panelMode == ViewportEditMode::BoneBox) {
                    app->BoneBoxStartX() = x;
                    app->BoneBoxStartY() = y;
                    app->BoneBoxSelectionActive() = 1;
                }
            }
        }
    } else if (leftState == 2) {
        app->PendingTimelineSelectionRow() = TimelineSelectionRow::None;;
        const int viewOperation =
            static_cast<int>(app->ViewToolDragOperation());
        const int mode = static_cast<int>(app->InteractionDragMode());
        if (viewOperation == 1 || viewOperation == 2) {
            RestoreViewportToolCursor(app, viewOperation);
            app->ViewToolDragOperation() = ViewportToolAction::None;
        }
        if (mode != 0) {
            if (mode >= 1 && mode <= 3)
                app->EditMode() = ViewportEditMode::Camera;
            else if (mode >= 7 && mode <= 9)
                app->EditMode() = ViewportEditMode::Light;
            if (viewOperation >= 9 && viewOperation <= 20) {
                RestoreViewportToolCursor(app, viewOperation);
                app->ViewToolDragOperation() = ViewportToolAction::None;
            }
            app->InteractionDragMode() = ViewportDragMode::None;
            app->state.a06B5 = 0;
        }
        app->BoneBoxSelectionActive() = 0;
    }
}

}  // namespace

void MouseInteractionBegin(MMDApp* app) {
    // 0x46FF02 -> 0x42D3A0 -> 0x40E3D0.  State values are exactly
    // 0 idle, 1 pressed, 2 released, 3 held.
    struct KeySlot { int key; std::int32_t MMDAppState::* state; };
    static constexpr KeySlot kKeySlots[] = {
        {VK_UP, &MMDAppState::upKeyState},
        {VK_DOWN, &MMDAppState::downKeyState},
        {VK_LEFT, &MMDAppState::leftKeyState},
        {VK_RIGHT, &MMDAppState::rightKeyState},
        {VK_SPACE, &MMDAppState::spaceKeyState},
        {VK_DELETE, &MMDAppState::deleteKeyState},
        {VK_ESCAPE, &MMDAppState::escKeyState},
        {VK_TAB, &MMDAppState::tabKeyState},
        {VK_RETURN, &MMDAppState::bC},
        {VK_MENU, &MMDAppState::menuKeyState},
        {221, &MMDAppState::keyState221},
        {226, &MMDAppState::keyState226},
    };
    for (const KeySlot& slot : kKeySlots)
        PollKey(app, app->state.*slot.state, slot.key);

    // Letter hotkeys share the dialog re-entry guard ints (see
    // MMDAppState::dialogFlags): lowercase and uppercase VK pairs fold
    // onto the same slot.
    static constexpr struct { int key; int flagIndex; } kLetterKeys[] = {
        {'x', 0}, {'X', 0}, {'z', 1}, {'Z', 1},
        {'c', 2}, {'C', 2}, {'v', 3}, {'V', 3},
        {'d', 4}, {'D', 4}, {'a', 5}, {'A', 5},
        {'b', 6}, {'B', 6}, {'s', 7}, {'S', 7},
        {'g', 8}, {'G', 8}, {'i', 9}, {'I', 9},
        {'h', 10}, {'H', 10}, {'k', 11}, {'K', 11},
        {'p', 12}, {'P', 12}, {'u', 13}, {'U', 13},
        {'j', 14}, {'J', 14}, {'f', 15}, {'F', 15},
        {'r', 16}, {'R', 16}, {'l', 17}, {'L', 17},
    };
    for (const auto& slot : kLetterKeys)
        PollKey(app, app->state.dialogFlags[slot.flagIndex], slot.key);

    for (int i = 0; i < 10; ++i)
        PollKey(app, app->state.numpadKeyState[i], VK_NUMPAD0 + i);

    PollKey(app, app->ShiftModifierState(), VK_SHIFT);
    PollKey(app, app->CtrlModifierState(), VK_CONTROL);
    PollKey(app, app->LeftMouseButtonState(), VK_LBUTTON);
    PollKey(app, app->RightMouseButtonState(), VK_RBUTTON);
    PollKey(app, app->MiddleMouseButtonState(), VK_MBUTTON);

    // A/B automation cannot safely hold global Ctrl/Shift while IDA and
    // PowerShell share the desktop.  Mirror the original internal key state
    // only during an active test drag; normal runs never set this variable.
    char forcedModifier[8]{};
    if (app->LeftMouseButtonState() != 0 &&
        GetEnvironmentVariableA("MIKUDANCESTUDIO_AB_MODIFIER", forcedModifier,
                                sizeof(forcedModifier)) != 0) {
        if (_stricmp(forcedModifier, "Shift") == 0)
            app->ShiftModifierState() = 3;
        else if (_stricmp(forcedModifier, "Ctrl") == 0)
            app->CtrlModifierState() = 3;
    }

    // 0x46FF07..0x46FF32 immediately follows the 0x42D3A0 call: input is
    // active when the foreground HWND is A0D38, or the main HWND when A0D38
    // is null.
    HWND active = app->FloatingWindow();
    if (active == nullptr)
        active = app->state.hwnd;
    char forceInput[2]{};
    const bool forced = GetEnvironmentVariableA(
        "MIKUDANCESTUDIO_AB_FORCE_INPUT", forceInput, sizeof(forceInput)) != 0;
    app->ViewportInputActive() = static_cast<std::uint8_t>(
        forced || GetForegroundWindow() == active);
    const int operation = ViewportToolAtPoint(app);
    BeginOrEndViewportToolDrag(app, operation);
    if (app->ViewportInputActive() == 0)
        return;

    if (app->LeftMouseButtonHeld())
        UpdateBoneBoxSelection(app);

    const int dx = app->MouseX() - app->PreviousMouseX();
    const int dy = app->MouseY() - app->PreviousMouseY();

    if (app->LeftMouseButtonHeld()) {
        const ViewportToolAction viewMode = app->ViewToolDragOperation();
        if (viewMode == ViewportToolAction::CameraOrbit) {
            app->CameraDistance() -= static_cast<float>(dy) * 0.1f;
            ViewRefreshGate(app);
        } else if (viewMode == ViewportToolAction::CameraPan) {
            if (app->state.optflag[0] != 0)
                PanCameraPosition(app, dx, dy);
            else {
                app->ViewOffsetX() -= static_cast<float>(dx) * DragScale(app);
                app->ViewOffsetY() += static_cast<float>(dy) * DragScale(app);
            }
            ViewRefreshGate(app);
        }
    }

    if (app->RightMouseButtonHeld()) {
        if (app->ShiftModifierActive()) {
            app->ViewOffsetY() = static_cast<float>(
                app->ViewOffsetY() + dy * kRightModifiedScale);
            app->ViewOffsetX() = static_cast<float>(
                app->ViewOffsetX() - dx * kRightModifiedScale);
        } else if (app->CtrlModifierActive()) {
            app->CameraDistance() = static_cast<float>(
                app->CameraDistance() - dy * kRightModifiedScale);
        } else {
            app->CameraYaw() = static_cast<float>(
                app->CameraYaw() - dx * kRotateScale);
            app->CameraPitch() = static_cast<float>(
                app->CameraPitch() - dy * kRotateScale);
        }
        ViewRefreshGate(app);
    }

    if (app->MiddleMouseButtonHeld()) {
        bool selectedCameraKey = false;
        if (app->state.optflag[0] == 0) {
            const double scale = app->ShiftModifierActive()
                ? g_MouseScaleA
                : (app->CtrlModifierActive()
                    ? g_MouseScaleB : g_MouseScaleC);
            app->ViewOffsetY() = static_cast<float>(
                app->ViewOffsetY() + dy * scale);
            app->ViewOffsetX() = static_cast<float>(
                app->ViewOffsetX() - dx * scale);
        } else if (app->CameraParentModel() < 0) {
            PanCameraPosition(app, dx, dy);
        } else {
            PanSelectedCameraKey(app, dx, dy);
            selectedCameraKey = true;
        }
        if (selectedCameraKey)
            PostViewRefresh(app);  // 0x471128
        else
            ViewRefreshGate(app);
    }
}

void MouseInteractionEnd(MMDApp* app) {
    app->PreviousMouseX() = app->MouseX();
    app->PreviousMouseY() = app->MouseY();
}

// runtime-initialized in the original (.rdata image differs); the
// registry-echo conversion pair 0x52B760/0x52B768 turns the stored radian
// value into the "%3.4f" display figure (best evidence: PI/180 divisor)
double g_Scale52E9F0 = 0.20000000298023224; // VA 0x0052E9F0
double g_Scale52E8C8 = 0.019999999552965164;// VA 0x0052E8C8
double g_ConvA52B760 = 180.0;               // VA 0x0052B760
double g_ConvB52B768 = 3.141592025756836;   // VA 0x0052B768

// ---- light/camera/registry chains (raw listing 10776..11260) --------------
// Common shape per axis mode (selector A=this+0x24==3, B=this+0xC0==3):
//   direct  (this+0x9ED9C == 1): app[axis] += dy * scale
//   registry(this+0x9ED9C == 2 && slot): slot[axis'] -=/+ dy * scale,
//     echo "%3.4f" into the axis edit box via SetWindowTextA.
// dy = (this+8) - (this+0x10);  slots: this+0x9DD70[this+0x9E170].
namespace {

int DyOf(MMDApp* s) {
    return s->MouseY() - s->PreviousMouseY();
}

bool SelA3(MMDApp* s) { return s->ShiftModifierActive(); }
bool SelB3(MMDApp* s) { return s->CtrlModifierActive(); }

unsigned char* RegSlotOf(MMDApp* s) {
    const unsigned idx = s->state.selLightAccSlotOrUint32;
    return reinterpret_cast<unsigned char*>(s->AccessorySlot(idx));
}

void EchoEdit(MMDApp* s, int dlgItem, const char* text) {
    HWND window = static_cast<HWND>(s->Hwnd());
    if (window == nullptr)
        window = static_cast<HWND>(s->Hwnd());
    SetWindowTextA(GetDlgItem(window, dlgItem), text);
}

}  // namespace

void ModeCameraAdjust(MMDApp* app, int axis) {
    // 0x4786FB..0x4790DF, modes 13..15.  The same operation widgets edit
    // either the camera target or the selected accessory position.
    static const std::size_t kSlotOff[3] = {0x214, 0x218, 0x21C};
    static const int kEdit[3] = {0x1DE, 0x1DF, 0x1E0};
    if (axis < 0 || axis > 2) { ViewRefreshGate(app); return; }
    const int target = app->state.v9ed9c;
    const int dy = DyOf(app);
    double scale;
    if (SelA3(app)) scale = g_MouseScaleA;            // 0x52B8F0
    else if (SelB3(app)) scale = g_MouseScaleB;       // 0x52E9C0
    else scale = g_MouseScaleC;                       // 0x52D738
    if (target == 2 &&
        app->state.optflag[0] != 0) {
        unsigned char* slot = RegSlotOf(app);
        if (slot != nullptr) {
            float& value = *reinterpret_cast<float*>(slot + kSlotOff[axis]);
            value = static_cast<float>(static_cast<double>(value) -
                                       static_cast<double>(dy) * scale);
            char buf[0x100];
            sprintf_s(buf, 0x100, "%3.4f",
                      *reinterpret_cast<float*>(slot + kSlotOff[axis]));
            EchoEdit(app, kEdit[axis], buf);
        }
    } else if (app->state.cameraParentModel >= 0 || target == 1) {
        app->CameraPosition()[axis] = static_cast<float>(
            static_cast<double>(app->CameraPosition()[axis]) +
            static_cast<double>(dy) * scale);
    } else {
        auto& api = d3dx::Get();
        api.Load();
        if (api.rotX && api.rotY && api.rotZ && api.multiply &&
            api.vec3Transform) {
            d3dx::D3DXMATRIXF rx{}, ry{}, rz{}, rotation{};
            api.rotZ(&rz, -app->CameraRoll());
            api.rotX(&rx, -app->CameraPitch());
            api.multiply(&rotation, &rz, &rx);
            api.rotY(&ry, -app->CameraYaw());
            api.multiply(&rotation, &rotation, &ry);
            float local[3]{};
            local[axis] = static_cast<float>(static_cast<double>(dy) * scale);
            float world[4]{};
            api.vec3Transform(world, local, &rotation);
            app->CameraPositionX() += world[0];
            app->CameraPositionY() += world[1];
            app->CameraPositionZ() += world[2];
        } else {
            app->CameraPosition()[axis] = static_cast<float>(
                static_cast<double>(app->CameraPosition()[axis]) +
                static_cast<double>(dy) * scale);
        }
    }
    ViewRefreshGate(app);
}

void ModeAngleAdjust(MMDApp* app, int axis) {
    // 0x4790E0..0x4798xx, modes 16..18.  Stored angles are radians;
    // accessory edit controls display degrees.
    static const std::size_t kSlotOff[3] = {0x220, 0x224, 0x228};
    static const int kEdit[3] = {0x1E1, 0x1E2, 0x1E3};
    if (axis < 0 || axis > 2) { ViewRefreshGate(app); return; }
    const int target = app->state.v9ed9c;
    const int dy = DyOf(app);
    double scale;
    if (SelA3(app)) scale = g_Scale52E9F0;            // 0.2
    else if (SelB3(app)) scale = 0.0020000000949949026;
    else scale = g_Scale52E8C8;                       // 0x52E8C8
    if (target == 2 &&
        app->state.optflag[0] != 0) {
        unsigned char* slot = RegSlotOf(app);
        if (slot != nullptr) {
            float& value = *reinterpret_cast<float*>(slot + kSlotOff[axis]);
            value = static_cast<float>(static_cast<double>(value) +
                                       static_cast<double>(dy) * scale);
            char buf[0x100];
            sprintf_s(buf, 0x100, "%3.4f",
                      *reinterpret_cast<float*>(slot + kSlotOff[axis]) /
                          g_ConvB52B768 * g_ConvA52B760);
            EchoEdit(app, kEdit[axis], buf);
        }
    } else {
        app->CameraRotation()[axis] = static_cast<float>(
            static_cast<double>(app->CameraRotation()[axis]) +
            static_cast<double>(dy) * scale);
    }
    ViewRefreshGate(app);
}

void ModeRotate(MMDApp* app, int /*axis*/) {
    // app+0x34C modes 1..3: view-axis bone rotation.
    BoneEditModes(app);
    ViewRefreshGate(app);
}

void ModeTranslate(MMDApp* app, int /*axis*/) {
    // app+0x34C modes 4..6: selected-bone local-axis rotation.
    BoneEditModes(app);
    ViewRefreshGate(app);
}

void ModeScale(MMDApp* app) {
    // app+0x34C mode 7 is horizontal selected-root translation, not scaling.
    BoneEditModes(app);
    ViewRefreshGate(app);
}

void ModeBoneRotate(MMDApp* app, int) {
    // app+0x34C modes 8/9 add vertical or combined screen-plane movement.
    BoneEditModes(app);
    ViewRefreshGate(app);
}

void ModePhysicsBody(MMDApp* app, int) {
    // app+0x34C modes 10..12: local-axis translation or active record edits.
    BoneEditModes(app);
    ViewRefreshGate(app);
}

// ---------------------------------------------------------------------------
// VA 0x0041ACD0 - Sub41ACD0(app, oldMode): physics-mode switch apply
// (command dispatch 0x47FA60/0x47FA88, control 0x213 family).  When the mode
// byte at +0x340 (kByte340) changes, the accessory ground position stored at
// app+0x308/+0x30C/+0xA08DC is re-anchored: both the OLD mode (the `mode`
// argument) and the NEW mode (current byte +0x340) resolve to a bone-record
// origin of the selected model, each origin matrix is rotated through the
// accessory rotation R = RotY(+0x314) * RotX(+0x310) * RotZ(+0x318) (and the
// parent transform at +0xA0438 when the parent index +0xA0430 >= 0), and the
// difference of the rotated translations is subtracted from the position.
//
// Bone-record resolution (selected model = slots[+0x910], records at
// *(model+0x26BC), 0x25C stride - the same array as physics_create.cpp):
//   mode 1: record index = *(model+0x4CCEC); origin = -(rec+100/104/108)
//   mode 2: record index = max(0, *(model+0x2D90)); origin = -(M * p) with
//           M the 4x4 matrix at rec+0x34 (floats 13..28) and p at rec+0x134
//           (floats 77..79); rows summed left-associative, single rounding
//           at the float store (x87 extended accumulation - commutative, so
//           the second block's commuted first add in the listing is
//           value-identical)
//   else:   origin = (0, 0, 0)
// ---------------------------------------------------------------------------
void Sub41ACD0(MMDApp* app, int mode) {
    unsigned char* st = app->storage();
    auto& api = d3dx::Get();
    if (!(api.Load() && api.rotX && api.rotY && api.rotZ && api.multiply &&
          api.translation))
        return;  // original hard-links d3dx9_32.dll; unreachable there
    // R = RotY * RotX * RotZ  (0x41ACEB..0x41AD5D)
    d3dx::D3DXMATRIXF ry{}, rot{}, rz{};
    api.rotY(&ry, app->CameraYaw());
    api.rotX(&rot, app->CameraPitch());
    api.multiply(&rot, &ry, &rot);
    api.rotZ(&rz, app->CameraRoll());
    api.multiply(&rot, &rot, &rz);
    if (app->CameraParentModel() >= 0)
        api.multiply(&rot,
                     reinterpret_cast<d3dx::D3DXMATRIXF*>(
                         &app->CameraAttachmentBasis()),
                     &rot);

    unsigned char* model = app->SelectedModel();
    auto* bones = mikudancestudio::mdl::Bones(model);

    auto modeOrigin = [&](int modeValue, d3dx::D3DXMATRIXF* out) {
        if (modeValue == 1) {                                  // 0x41AD79
            unsigned char* rec = mikudancestudio::mdl::BoneBytes(
                bones, *reinterpret_cast<std::int32_t*>(model + 0x4CCEC));
            const float tx = -*reinterpret_cast<float*>(rec + 100);
            const float ty = -*reinterpret_cast<float*>(rec + 104);
            const float tz = -*reinterpret_cast<float*>(rec + 108);
            api.translation(out, tx, ty, tz);
        } else if (modeValue == 2) {                           // 0x41ADC6
            std::int32_t idx = mikudancestudio::mdl::Mdl(model)->selectedBone;
            if (idx < 0)
                idx = 0;
            const float* f = reinterpret_cast<const float*>(
                mikudancestudio::mdl::BoneBytes(bones, idx));
            const float z = f[15] * f[77] + f[19] * f[78] +
                             f[23] * f[79] + f[27];
            const float y = f[14] * f[77] + f[18] * f[78] +
                             f[22] * f[79] + f[26];
            const float x = f[13] * f[77] + f[17] * f[78] +
                             f[21] * f[79] + f[25];
            api.translation(out, -x, -y, -z);
        } else {
            api.translation(out, 0.0f, 0.0f, 0.0f);
        }
    };

    d3dx::D3DXMATRIXF tOld{}, tNew{};
    modeOrigin(mode, &tOld);                          // a2 (old mode)
    modeOrigin(static_cast<int>(app->CameraReferenceMode()), &tNew);
    api.multiply(&tOld, &tOld, &rot);                 // 0x41B005
    api.multiply(&tNew, &tNew, &rot);                 // 0x41B01A
    app->ViewOffsetX() -= tOld.m[3][0] - tNew.m[3][0];
    app->ViewOffsetY() -= tOld.m[3][1] - tNew.m[3][1];
    app->CameraDistance() -= tOld.m[3][2] - tNew.m[3][2];
}

}  // namespace mikudancestudio
