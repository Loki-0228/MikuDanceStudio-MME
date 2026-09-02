// ===========================================================================
// VA 0x00433A40 - playback-start state initialization
// VA 0x004341E0 - playback-stop UI/state restoration
// ===========================================================================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mikudancestudio/accessory_layout.hpp"
#include "mikudancestudio/d3dx_dyn.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/model.hpp"

namespace mikudancestudio {

void Sub4A02C0(unsigned char* model);  // VA 0x004A02C0
void Sub413120(MMDApp* app, int index);  // VA 0x00413120
void Sub412330(MMDApp* app);  // VA 0x00412330

namespace {

constexpr double kFrameRate = 30.0;
constexpr double kPiOver180 = 0.01745329238474369;

float ReadF32(const unsigned char* p) {
    float value;
    std::memcpy(&value, p, sizeof value);
    return value;
}

void EnableRange(HWND parent, int first, int last, BOOL enabled) {
    for (int id = first; id <= last; ++id)
        EnableWindow(GetDlgItem(parent, id), enabled);
}

void SetMenuRange(HMENU menu, int first, int last, UINT state) {
    for (int id = first; id <= last; ++id)
        EnableMenuItem(menu, id, state);
}

void SetPhysicsMenuState(HWND hwnd, UINT state) {
    HMENU submenu = GetSubMenu(GetMenu(hwnd), 7);
    MENUITEMINFOA item{};
    item.cbSize = sizeof item;
    item.fMask = MIIM_STATE;
    item.fState = state;
    SetMenuItemInfoA(submenu, 2, TRUE, &item);
}

}  // namespace

// Exported at mikudancestudio scope for the fullscreen record restore path
// (0x4629D0 in src/app/avi_record_start.cpp); the bodies were
// originally file-local here.
// VA 0x00429790.  UINT 1 is MF_GRAYED, matching the immediate operands in
// the original rather than treating it as a Boolean.
void DisablePlaybackMenus(HWND hwnd) {
    HMENU menu = GetMenu(hwnd);
    SetMenuRange(menu, 202, 210, MF_GRAYED);
    SetMenuRange(menu, 212, 213, MF_GRAYED);
    SetMenuRange(menu, 217, 220, MF_GRAYED);
    SetMenuRange(menu, 222, 232, MF_GRAYED);
    SetMenuRange(menu, 237, 242, MF_GRAYED);
    EnableMenuItem(menu, 251, MF_GRAYED);
    EnableMenuItem(menu, 252, MF_GRAYED);
    EnableMenuItem(menu, 276, MF_GRAYED);
    SetPhysicsMenuState(hwnd, MFS_DISABLED | MFS_GRAYED);
    DrawMenuBar(hwnd);
}

// VA 0x004298E0.  This restores the broad menu set and then reapplies the
// camera/model-mode restrictions that exist outside playback.
void RestorePlaybackMenus(MMDApp* app) {
    HWND hwnd = static_cast<HWND>(app->Hwnd());
    HMENU menu = GetMenu(hwnd);
    SetMenuRange(menu, 202, 210, MF_ENABLED);
    SetMenuRange(menu, 212, 213, MF_ENABLED);
    SetMenuRange(menu, 217, 220, MF_ENABLED);
    SetMenuRange(menu, 222, 232, MF_ENABLED);
    SetMenuRange(menu, 237, 242, MF_ENABLED);
    EnableMenuItem(menu, 276, MF_ENABLED);

    if (app->state.optflag0 != 0) {
        const int disabled[] = {217, 220, 218, 202, 203, 219, 222, 251, 252};
        for (int id : disabled)
            EnableMenuItem(menu, id, MF_GRAYED);
        SetMenuRange(menu, 224, 231, MF_GRAYED);
        SetMenuRange(menu, 273, 275, MF_GRAYED);
        EnableWindow(GetDlgItem(hwnd, 424), FALSE);
        EnableRange(hwnd, 437, 445, FALSE);
    } else {
        SetMenuRange(menu, 237, 242, MF_GRAYED);
        const int enabled[] = {202, 203, 219, 222, 251, 252};
        for (int id : enabled)
            EnableMenuItem(menu, id, MF_ENABLED);
        SetMenuRange(menu, 224, 231, MF_ENABLED);
        SetMenuRange(menu, 273, 275, MF_ENABLED);
        EnableWindow(GetDlgItem(hwnd, 424), TRUE);

        unsigned char* model = app->SelectedModel();
        const UINT state = model != nullptr && mikudancestudio::mdl::Mdl(model)->physicsMode == 2
                               ? MFS_DISABLED | MFS_GRAYED
                               : MFS_ENABLED;
        SetPhysicsMenuState(hwnd, state);
    }
    DrawMenuBar(hwnd);
}

namespace {

void ApplyProjection(MMDApp* app) {
    D3DRenderer* wrapper = app->Renderer();
    auto& api = d3dx::Get();
    d3dx::D3DXMATRIXF matrix{};
    const float fov = static_cast<float>(
        static_cast<double>(app->CameraFov()) * kPiOver180);
    if (api.Load())
        api.perspectiveFovLH(&matrix, fov, wrapper->aspectRatio,
                             1.0f, 100000.0f);
    IDirect3DDevice9* device = wrapper->device;  // +120032
    device->SetTransform(D3DTS_PROJECTION,
                         reinterpret_cast<const D3DMATRIX*>(&matrix));
}

void CopyCameraTerminal(MMDApp* app, const mdl::CameraKey& key) {
    std::memcpy(app->CameraPosition(), key.eye, sizeof(key.eye));
    std::memcpy(app->CameraRotation(), key.target, sizeof(key.target));
    app->CameraDistance() = key.distance;
    app->CameraPerspective() = key.perspective;
    app->CameraFov() = static_cast<float>(key.fov);
    app->CameraParentModel() = key.parentModel;
    app->CameraParentBone() = key.parentBone;
    ApplyProjection(app);
}

void CopyLightTerminal(MMDApp* app, const mdl::LightKey& key) {
    std::memcpy(app->LightDirection(), key.direction, sizeof(key.direction));
    std::memcpy(app->LightColor(), key.color, sizeof(key.color));
    app->ApplyTimelineLightState();
    D3DRenderer* wrapper = app->Renderer();
    IDirect3DDevice9* device = wrapper->device;  // +120032
    device->SetLight(0, &app->SceneLight());
}

template <typename Key>
static const Key* SeekTypedTrackEnd(Key* keys, std::uint32_t& cursor,
                                    float frame,
                                    bool& ended) {
    cursor = 0;
    ended = false;
    if (static_cast<double>(keys[0].frame) >= static_cast<double>(frame)) {
        return &keys[0];
    }
    for (;;) {
        const Key& current = keys[cursor];
        if (current.next == 0) {
            ended = true;
            return &current;
        }
        cursor = current.next;
        if (static_cast<double>(keys[cursor].frame) >=
            static_cast<double>(frame)) {
            return &keys[cursor];
        }
    }
}

void InitGlobalTracks(MMDApp* app, float frame) {
    app->raw<float>(776) = 0.0f;
    app->raw<float>(780) = 0.0f;

    bool ended = false;
    app->CameraTrackActive() = 1;
    const mdl::CameraKey* cameraKey = SeekTypedTrackEnd(
        app->CameraKeys(), app->CameraTrackCursor(), frame, ended);
    if (ended) {
        app->CameraTrackActive() = 0;
        CopyCameraTerminal(app, *cameraKey);
    }

    app->LightTrackActive() = 1;
    const mdl::LightKey* lightKey = SeekTypedTrackEnd(
        app->LightKeys(), app->LightTrackCursor(), frame, ended);
    if (ended) {
        app->LightTrackActive() = 0;
        CopyLightTerminal(app, *lightKey);
    }

    app->ShadowTrackActive() = 1;
    const mdl::SelfShadowKey* shadowKey = SeekTypedTrackEnd(
        app->ShadowKeys(), app->ShadowTrackCursor(), frame, ended);
    if (ended) {
        app->ShadowTrackActive() = 0;
        app->ShadowMode() =
            static_cast<std::int8_t>(shadowKey->mode);
        app->ShadowDistance() = shadowKey->distance;
    }

    app->GravityTrackActive() = 1;
    const mdl::GravityKey* gravityKey = SeekTypedTrackEnd(
        app->GravityKeys(), app->GravityTrackCursor(), frame, ended);
    if (ended) {
        app->GravityTrackActive() = 0;
        app->GravityNoiseEnabled() = gravityKey->noiseEnabled;
        app->GravityNoise() = gravityKey->noise;
        app->GravityMagnitude() = gravityKey->acceleration;
        std::memcpy(app->GravityDirection(), gravityKey->direction,
                    sizeof(gravityKey->direction));
    }
}

void InitAccessoryTracks(MMDApp* app, float frame) {
    for (int i = 0; i < 255; ++i) {
        app->AccessoryTrackActive(i) = 0;
        mdl::AccessoryRecord* accessory = app->AccessorySlot(i);
        if (accessory == nullptr)
            continue;

        app->AccessoryTrackActive(i) = 1;
        mdl::AccessoryKey* keys = app->AccessoryKeys(i);
        auto& cursor = app->AccessoryTrackCursor(i);
        cursor = 0;
        if (static_cast<double>(keys[0].frame) >= static_cast<double>(frame))
            continue;

        for (;;) {
            const int cur = static_cast<int>(cursor);
            const mdl::AccessoryKey& rec = keys[cur];
            const int next = static_cast<int>(rec.next);
            if (next != 0) {
                cursor = static_cast<std::uint32_t>(next);
                if (static_cast<double>(keys[next].frame) >=
                    static_cast<double>(frame))
                    break;
                continue;
            }

            app->AccessoryTrackActive(i) = 0;
            accessory->visible = rec.visible;
            accessory->shadowEnabled = rec.shadowEnabled;
            accessory->parentModel = rec.parentModel;
            accessory->parentBone = rec.parentBone;
            std::memcpy(accessory->rotation, rec.rotation,
                        sizeof accessory->rotation);
            std::memcpy(accessory->position, rec.position,
                        sizeof accessory->position);
            accessory->scale = rec.scale;
            accessory->opacity = rec.opacity;
            break;
        }
    }
}

void SavePlaybackUndoSnapshot(MMDApp* app, unsigned char* model) {
    mdl::ModelRecord& state = *mdl::Mdl(model);
    const int boneCount = static_cast<int>(state.boneCount);
    unsigned char* selected = state.bonePhysicsState;
    bool anySelected = false;
    for (int i = 0; i < boneCount; ++i)
        anySelected = anySelected || selected[i] != 0;
    if (!anySelected)
        return;

    HWND hwnd = static_cast<HWND>(app->Hwnd());
    EnableWindow(GetDlgItem(hwnd, 400), TRUE);
    EnableWindow(GetDlgItem(hwnd, 401), FALSE);
    state.undoDirty = 1;
    state.redoDirty = 0;
    std::uint32_t& undoIndex = state.undoState[0];
    undoIndex = (undoIndex + 1) % 30;
    state.undoState[1] = undoIndex;
    auto& undo = state.undoRings[0].slots[undoIndex];
    undo.operation = 3;
    undo.dirty = boneCount;
    undo.frame = app->state.currentFrame;
    auto*& buffer = undo.bonePose;
    std::free(buffer);
    buffer = static_cast<mikudancestudio::mdl::BonePoseSnapshot*>(
        std::malloc(sizeof(*buffer) * boneCount));
    std::memset(buffer, 0, static_cast<std::size_t>(36) * boneCount);
    auto* records = buffer;
    mikudancestudio::mdl::BoneRecord* bones = mikudancestudio::mdl::Bones(model);
    for (int i = 0; i < boneCount; ++i) {
        auto& dst = records[i];
        mikudancestudio::mdl::BoneRecord* bone = &bones[i];
        dst.boneIndex = i;
        std::memcpy(dst.position, bone->trans, sizeof(dst.position));
        std::memcpy(dst.rotation, bone->rotQuat, sizeof(dst.rotation));
        dst.physicsDisabled = selected[i];
        selected[i] = 0;
    }
}

}  // namespace

void UpdateBoneFrames(MMDApp* app) {
    const float cursor = app->PlaybackCursorSeconds();
    app->SavedPlaybackPhysicsMode() = app->PlaybackPhysicsMode();
    if (app->raw<std::uint8_t>(offsets::kByteA066D) != 0)
        app->PlaybackPhysicsMode() = 2;

    for (int i = 0; i < 100; ++i) {
        unsigned char* model = app->ModelSlot(i);
        if (model != nullptr)
            Sub4A2CD0(model, cursor,
                      app->PlaybackPhysicsMode());
    }

    if (app->PlaybackFrameChanged() != 0)
        app->PlaybackFrameChanged() = 0;
    else
        app->PhysicsResetPending() = 1;

    const float frame = static_cast<float>(
        static_cast<double>(cursor) * kFrameRate);
    if (app->state.optflag0 != 0 ||
        app->state.v9ed98 != 0)
        InitGlobalTracks(app, frame);
    InitAccessoryTracks(app, frame);

    HWND hwnd = static_cast<HWND>(app->Hwnd());
    EnableRange(hwnd, 400, 401, FALSE);
    EnableRange(hwnd, 409, 410, FALSE);
    EnableRange(hwnd, 415, 468, FALSE);
    EnableRange(hwnd, 471, 487, FALSE);
    EnableRange(hwnd, 490, 501, FALSE);
    EnableRange(hwnd, 504, 527, FALSE);
    HWND overlay = app->FloatingWindow();
    EnableRange(overlay != nullptr ? overlay : hwnd, 536, 550, FALSE);
    DisablePlaybackMenus(hwnd);
    PostLanguageSweep2(app);
}

void Sub4341E0(MMDApp* app) {
    if (getenv("MIKUDANCESTUDIO_TRACE_REC")) {
        FILE* tf = fopen(getenv("MIKUDANCESTUDIO_TRACE_REC"), "a");
        if (tf) { fputs("Sub4341E0 stop-playback enter\n", tf); fclose(tf); }
    }
    HWND hwnd = static_cast<HWND>(app->Hwnd());
    EnableRange(hwnd, 400, 401, TRUE);
    EnableRange(hwnd, 409, 410, TRUE);
    EnableRange(hwnd, 415, 468, TRUE);
    EnableRange(hwnd, 471, 487, TRUE);
    EnableRange(hwnd, 490, 501, TRUE);
    EnableRange(hwnd, 504, 527, TRUE);
    HWND overlay = app->FloatingWindow();
    EnableRange(overlay != nullptr ? overlay : hwnd, 536, 550, TRUE);

    const std::size_t saved = 650103;
    EnableWindow(GetDlgItem(hwnd, 497), app->raw<std::uint8_t>(saved + 0));
    EnableWindow(GetDlgItem(hwnd, 498), app->raw<std::uint8_t>(saved + 1));
    EnableWindow(GetDlgItem(hwnd, 431), app->raw<std::uint8_t>(saved + 4));
    EnableWindow(GetDlgItem(hwnd, 421), app->raw<std::uint8_t>(saved + 2));
    EnableWindow(GetDlgItem(hwnd, 422), app->raw<std::uint8_t>(saved + 3));
    EnableWindow(GetDlgItem(hwnd, 400), app->raw<std::uint8_t>(saved + 5));
    EnableWindow(GetDlgItem(hwnd, 401), app->raw<std::uint8_t>(saved + 6));

    const bool enable250 =
        (app->raw<std::uint32_t>(645672) == 0 ||
         app->state.optflag0 != 0) &&
        !(app->raw<std::uint32_t>(645700) != 0 &&
          app->state.optflag0 != 0);
    EnableMenuItem(GetMenu(hwnd), 250,
                   enable250 ? MF_ENABLED : MF_GRAYED);
    RestorePlaybackMenus(app);

    app->PlaybackPhysicsMode() = app->SavedPlaybackPhysicsMode();

    unsigned char* activeModel = app->SelectedModel();
    if (app->raw<std::uint8_t>(offsets::kByte342) != 0) {
        if (activeModel != nullptr)
            SavePlaybackUndoSnapshot(app, activeModel);
        const int frame = static_cast<int>(
            static_cast<double>(app->PlaybackCursorSeconds()) *
            kFrameRate);
        app->state.currentFrame = frame;
        app->state.timelineStartFrame =
            frame > 6 ? frame - 6 : 0;
        char text[50];
        sprintf_s(text, "%d", frame);
        HWND edit = GetDlgItem(hwnd, 417);
        SendMessageA(edit, EM_SETSEL, 0, GetWindowTextLengthA(edit));
        SendMessageA(edit, EM_REPLACESEL, FALSE,
                     reinterpret_cast<LPARAM>(text));
        PanelPaint(app);
        if (app->state.waveEnabled != 0) {
            TimelineDrawTicks(app->state.timelineStartFrame,
                              app->SidebarWidth());
            RECT rect{6, 95,
                      app->SidebarWidth() - 3, 146};
            InvalidateRect(hwnd, &rect, FALSE);
        }
    }

    const int frame = app->state.currentFrame;
    for (int i = 0; i < 100; ++i) {
        unsigned char* model = app->ModelSlot(i);
        if (model != nullptr)
            Sub4B4260(model, frame, app->PlaybackPhysicsMode());
    }
    if (app->state.optflag0 == 0 &&
        activeModel != nullptr)
        Sub4A02C0(activeModel);

    app->CameraParentModel() = -1;
    SendMessageA(GetDlgItem(hwnd, 450), CB_SETCURSEL, 0, 0);

    if (app->state.optflag0 != 0) {
        app->ViewOffsetX() = 0.0f;
        app->ViewOffsetY() = 0.0f;
        ReloadModels(app);
        Sub411070(app);
        Sub411B90(app);
        Sub412330(app);
    } else if (app->state.v9ed98 != 0) {
        if (app->CameraParentModel() >= 0) {
            ModelApplyMorphs(activeModel);
            SetPhysicsMode(activeModel, 0, app->ModelSlots(),
                           app->PlaybackPhysicsMode());
        }
        app->ViewOffsetX() = 0.0f;
        app->ViewOffsetY() = 0.0f;
        ReloadModels(app);
        Sub411070(app);
        Sub411B90(app);
        Sub412330(app);
        app->CameraAttachmentTransformSuppressed() = 0;
        PostModelReload(app);
        app->raw<std::uint8_t>(offsets::kByteA04B8) = 1;
    }

    for (int i = 0; i < 255; ++i)
        if (app->AccessorySlot(i) != nullptr)
            Sub413120(app, i);
    Sub4134E0(app);
    if (app->EditMode() == ViewportEditMode::BoneBox)
        Sub4168D0(app);
    app->PhysicsResetPending() = 1;
    PostViewRefresh(app);
    PostLanguageSweep2(app);
}

}  // namespace mikudancestudio
