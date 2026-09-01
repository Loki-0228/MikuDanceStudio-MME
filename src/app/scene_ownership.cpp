// ===========================================================================
// MikuDanceStudio - scene-resident allocation teardown
// ===========================================================================
#include <cstdio>
#include <cstdlib>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/scene_ownership.hpp"

namespace mikudancestudio {

namespace {

void TraceAccessoryRelease(int slot, const char* phase, const void* track) {
    const char* directory = std::getenv("MIKUDANCESTUDIO_PMM_TRACE_DIR");
    if (directory == nullptr || directory[0] == '\0')
        return;
    char path[MAX_PATH];
    sprintf_s(path, "%s\\pmm_model_load.log", directory);
    FILE* stream = nullptr;
    if (fopen_s(&stream, path, "ab") != 0 || stream == nullptr)
        return;
    fprintf(stream, "stage=accessory-track-%s slot=%d track=%p\r\n", phase,
            slot, track);
    fclose(stream);
}

}  // namespace

void ReleaseSceneModels(MMDApp& app) {
    for (int slot = 0; slot < 100; ++slot) {
        unsigned char*& model = app.ModelSlot(slot);
        if (model == nullptr)
            continue;
        ModelDispose(model);
        std::free(model);
        model = nullptr;
    }
}

void ReleaseGlobalTimelineTracks(MMDApp& app) {
    if (app.CameraKeys() != nullptr) {
        std::free(app.CameraKeys());
        app.CameraKeys() = nullptr;
    }
    if (app.LightKeys() != nullptr) {
        std::free(app.LightKeys());
        app.LightKeys() = nullptr;
    }
    if (app.ShadowKeys() != nullptr) {
        std::free(app.ShadowKeys());
        app.ShadowKeys() = nullptr;
    }
    if (app.GravityKeys() != nullptr) {
        std::free(app.GravityKeys());
        app.GravityKeys() = nullptr;
    }
}

void ReleaseAccessoriesAndTracks(MMDApp& app) {
    for (int slot = 0; slot < 255; ++slot) {
        mdl::AccessoryRecord*& accessory = app.AccessorySlot(slot);
        if (accessory != nullptr) {
            DisposeAccessory(accessory);
            std::free(accessory);
            accessory = nullptr;
        }
        mdl::AccessoryKey*& track = app.AccessoryKeys(slot);
        if (track != nullptr) {
            TraceAccessoryRelease(slot, "before-free", track);
            std::free(track);
            TraceAccessoryRelease(slot, "after-free", nullptr);
            track = nullptr;
        }
    }
}

}  // namespace mikudancestudio
