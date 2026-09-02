// ===========================================================================
// Left frame-editor refresh and single-frame stepping helpers.
//   0x0040D070  invalidate the two model-row status rectangles
//   0x00430F20  advance one frame
//   0x004312E0  move back one frame
// ===========================================================================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/ported_funcs.hpp"

namespace mikudancestudio {

void Sub4A0080(unsigned char* model, int frame);
void Sub4A02C0(unsigned char* model);
void Sub411B90(MMDApp* app);
void Sub412330(MMDApp* app);
void Sub413120(MMDApp* app, int index);
void Sub4134E0(MMDApp* app);
void Sub4168D0(MMDApp* app);
void Sub4C3530(void* subsystem, double time);

namespace {

void SnapshotAndClearBoneSelection(MMDApp* app) {
    for (int slot = 0; slot < 100; ++slot) {
        unsigned char* model = app->ModelSlot(slot);
        if (model == nullptr)
            continue;
        const mdl::ModelRecord& record = *mdl::Mdl(model);
        const std::int32_t count = record.boneCount;
        unsigned char* selected = record.boneSelection;
        int index = 0;
        while (index < count && selected[index] == 0)
            ++index;
        if (index >= count)
            continue;
        Sub4A0080(model, app->state.currentFrame);
        for (index = 0; index < count; ++index)
            selected[index] = 0;
    }
}

void SetFrameEditText(MMDApp* app) {
    const HWND hwnd = static_cast<HWND>(app->Hwnd());
    const HWND edit = GetDlgItem(hwnd, 0x1A1);
    const LRESULT length = GetWindowTextLengthA(edit);
    SendMessageA(edit, EM_SETSEL, 0, length);
    char text[0x100];
    sprintf_s(text, sizeof(text), "%d",
              app->state.currentFrame);
    SendMessageA(edit, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(text));
}

void ApplyFrameToModels(MMDApp* app) {
    for (int slot = 0; slot < 100; ++slot) {
        unsigned char* model = app->ModelSlot(slot);
        if (model == nullptr)
            continue;
        Sub4B4260(model, app->state.currentFrame,
                  app->PlaybackPhysicsMode());
        if (slot == app->SelectedModelSlot())
            Sub4A02C0(model);
    }
}

void RefreshFrameContext(MMDApp* app) {
    const HWND hwnd = static_cast<HWND>(app->Hwnd());
    if (app->state.optflag0 != 0) {
        ReloadModels(app);
        Sub411070(app);
        Sub411B90(app);
        Sub412330(app);
        for (int slot = 0; slot < 0xFF; ++slot) {
            if (app->AccessorySlot(slot) != nullptr)
                Sub413120(app, slot);
        }
        Sub4134E0(app);
        return;
    }

    if (app->state.v9ed98 != 0) {
        app->ViewOffsetX() = 0.0f;
        app->ViewOffsetY() = 0.0f;
        ReloadModels(app);
        Sub411070(app);
        Sub411B90(app);
        Sub412330(app);
        for (int slot = 0; slot < 0xFF; ++slot) {
            if (app->AccessorySlot(slot) != nullptr)
                Sub413120(app, slot);
        }
        Sub4134E0(app);
        const std::int32_t selected = app->CameraParentModel();
        if (selected >= 0) {
            unsigned char* model = app->ModelSlot(selected);
            if (model != nullptr) {
                ModelApplyMorphs(model);
                SetPhysicsMode(model, 0, app->ModelSlots(),
                    app->PlaybackPhysicsMode());
            }
        }
        PostModelReload(app);
        return;
    }

    EnableWindow(GetDlgItem(hwnd, 0x190), TRUE);
    EnableWindow(GetDlgItem(hwnd, 0x191), FALSE);
}

void RefreshTimeline(MMDApp* app, bool forward) {
    const std::uint32_t frame = static_cast<std::uint32_t>(
        app->state.currentFrame);
    const std::uint32_t visible = static_cast<std::uint32_t>(
        (app->SidebarWidth() - 0x54) / 0x1A);
    app->state.timelineStartFrame =
        frame <= visible ? 0 : static_cast<std::int32_t>(frame - visible);
    PanelPaint(app);

    if (app->WaveEnabled() != 0) {
        TimelineDrawTicks(app->state.timelineStartFrame,
                          app->SidebarWidth());
        RECT rect{6, 95,
                  app->SidebarWidth() - 3, 146};
        InvalidateRect(static_cast<HWND>(app->Hwnd()), &rect, FALSE);
        if (app->raw<std::uint8_t>(offsets::kByteA0196) != 0) {
            const bool seek =
                app->AutomaticFrameAdvanceEnabled() == 0;
            app->AudioSeekReady() = 1;
            if (seek) {
                SetFrameNormalized(
                    app->FrameNormalization());
                const std::int32_t previous =
                    app->state.currentFrame - 1;
                double time = static_cast<double>(
                    static_cast<std::uint32_t>(previous)) / 30.0;
                if (time < 0.0)
                    time = 0.0;
                Sub4C3530(app->Audio(), time);
            }
        }
    }

    if (forward) {
        app->PhysicsResetPending() =
            app->PlaybackPhysicsMode() == 3 ? 1 : 0;
        if (app->raw<std::int32_t>(offsets::kDword91C) == 1)
            Sub4168D0(app);
    } else {
        if (app->raw<std::int32_t>(offsets::kDword91C) == 1)
            Sub4168D0(app);
        if (app->PlaybackPhysicsMode() == 3)
            app->PhysicsResetPending() = 1;
    }
}

void StepFrame(MMDApp* app, bool forward) {
    app->CameraAttachmentTransformSuppressed() = 0;
    SnapshotAndClearBoneSelection(app);
    std::int32_t& frame = app->state.currentFrame;
    if (forward)
        ++frame;
    else if (frame != 0)
        --frame;
    SetFrameEditText(app);
    ApplyFrameToModels(app);
    RefreshFrameContext(app);
    RefreshTimeline(app, forward);
}

}  // namespace

void Sub40D070(MMDApp* app) {
    HWND target = app->raw<HWND>(164686 * sizeof(std::uint32_t));
    int leftBase = 0;
    RECT client{};
    if (target != nullptr) {
        GetClientRect(target, &client);
    } else {
        target = app->raw<HWND>(164270 * sizeof(std::uint32_t));
        GetClientRect(target, &client);
        leftBase = app->raw<std::int32_t>(164274 * sizeof(std::uint32_t)) + 9;
    }
    const int bottom = app->raw<std::int32_t>(164689 * sizeof(std::uint32_t));
    RECT rect{leftBase + 10, bottom - 22, client.right - 450, bottom - 1};
    InvalidateRect(target, &rect, FALSE);
    rect.left = leftBase + 68;
    rect.top = app->raw<std::int32_t>(164691 * sizeof(std::uint32_t));
    rect.right = leftBase + 139;
    rect.bottom = rect.top + 30;
    InvalidateRect(target, &rect, FALSE);
}

void Sub430F20(MMDApp* app) { StepFrame(app, true); }
void Sub4312E0(MMDApp* app) { StepFrame(app, false); }

void Sub432FA0(MMDApp* app) {
    if (app == nullptr)
        return;
    app->CameraAttachmentTransformSuppressed() = 0;
    SnapshotAndClearBoneSelection(app);

    const std::uint32_t frame =
        app->state.currentFrame;
    const std::uint32_t visible = static_cast<std::uint32_t>(
        (app->SidebarWidth() - 84) / 26);
    app->state.timelineStartFrame =
        frame <= visible ? 0 : static_cast<std::int32_t>(frame - visible);
    PanelPaint(app);
    if (app->WaveEnabled() != 0) {
        TimelineDrawTicks(app->state.timelineStartFrame,
                          app->SidebarWidth());
        RECT rect{6, 95,
                  app->SidebarWidth() - 3, 146};
        InvalidateRect(static_cast<HWND>(app->Hwnd()), &rect, FALSE);
    }

    ApplyFrameToModels(app);
    RefreshFrameContext(app);
    if (app->raw<std::int32_t>(offsets::kDword91C) == 1)
        Sub4168D0(app);
    app->PhysicsResetPending() = 1;
}

}  // namespace mikudancestudio
