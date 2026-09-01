// Bone-edit undo snapshot (original VA 0x0042D6E0).
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdlib>
#include <cstring>
#include <new>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"

namespace mikudancestudio {

void Sub42D6E0(MMDApp* app) {
    if (app == nullptr)
        return;
    const unsigned modelSlot = app->raw<std::uint8_t>(2320);
    auto* model = app->raw<unsigned char*>(1920 + 4 * modelSlot);
    if (model == nullptr)
        return;

    const int boneCount = mikudancestudio::mdl::Mdl(model)->boneCount;
    auto* selected = mikudancestudio::mdl::Mdl(model)->boneSelection;
    auto* dirty = mikudancestudio::mdl::Mdl(model)->bonePhysicsState;
    auto* bones = mikudancestudio::mdl::Bones(model);
    if (boneCount <= 0 || selected == nullptr || bones == nullptr)
        return;

    int selectedCount = 0;
    for (int i = 0; i < boneCount; ++i)
        selectedCount += selected[i] != 0;
    if (selectedCount == 0)
        return;

    HWND window = app->raw<HWND>(657080);
    if (window == nullptr)
        window = static_cast<HWND>(app->Hwnd());
    EnableWindow(GetDlgItem(window, 400), TRUE);
    EnableWindow(GetDlgItem(window, 401), FALSE);

    model[12732] = 1;
    model[12733] = 0;
    auto& ringIndex = mikudancestudio::mdl::Mdl(model)->undoState[0];
    if (++ringIndex >= 30)
        ringIndex = 0;
    mikudancestudio::mdl::Mdl(model)->undoState[1] = ringIndex;

    auto& undo = mikudancestudio::mdl::Mdl(model)->undoRings[0].slots[ringIndex];
    undo.operation = 1;
    undo.dirty = selectedCount;
    auto*& oldSnapshot = undo.bonePose;
    if (oldSnapshot != nullptr) {
        std::free(oldSnapshot);
        oldSnapshot = nullptr;
    }

    auto* snapshot = static_cast<mikudancestudio::mdl::BonePoseSnapshot*>(
        ::operator new(sizeof(mikudancestudio::mdl::BonePoseSnapshot) * selectedCount));
    oldSnapshot = snapshot;
    std::memset(snapshot, 0, static_cast<std::size_t>(36) * selectedCount);

    int output = 0;
    for (int i = 0; i < boneCount; ++i) {
        if (selected[i] == 0)
            continue;
        auto& record = snapshot[output++];
        mikudancestudio::mdl::BoneRecord* bone = &bones[i];
        record.boneIndex = i;
        std::memcpy(record.position, bone->trans, sizeof(record.position));
        std::memcpy(record.rotation, bone->rotQuat, sizeof(record.rotation));
        record.physicsDisabled = dirty != nullptr ? dirty[i] : 0;
    }
}

}  // namespace mikudancestudio
