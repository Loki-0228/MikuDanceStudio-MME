// ===========================================================================
// VA 0x00460430 - LoadModelFile / AddModel  (original: sub_460430)
// ===========================================================================
// Model-slot orchestrator: finds a free slot among the 100 at this+1920,
// allocates the 0x4CCF4 block (operator new + memset + default init
// 0x4A8DC0), runs the loader 0x4BF3E0, and on success registers the model
// name in the three comboboxes (436/474/449 via CB_ADDSTRING), selects it
// (CB_SETCURSEL), refreshes the bone/morph selectors (0x42F1E0 chain),
// resets the manipulation radios (BM_SETCHECK), enables the model-dependent
// menu items, and toggles the physics menu-item state (SetMenuItemInfoA).
// On failure the model is disposed (0x48F830) and the slot cleared.
// When all 100 slots are taken the original shows
// "you cannot add models over %d!" (JP: 0x52E198/0x52E18C).
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/ported_funcs.hpp"

namespace mikudancestudio {
namespace {

#include "model_labels.inc"  // GENERATED (kMsgModelLimitJp / kTitleAddModelJp)


}  // namespace

void LoadModelFile(MMDApp* app, const wchar_t* path) {      // 0x460430
    HWND hwnd = static_cast<HWND>(app->Hwnd());
    int slot = 0;
    while (app->ModelSlot(slot) != nullptr) {
        ++slot;
        if (slot >= 100) {
            app->raw<std::uint32_t>(188) = 1;
            char text[256];
            if (app->EnglishUI() == 0) {
                sprintf_s(text, 0x100, kMsgModelLimitJp, 100);
                MessageBoxA(hwnd, text, kTitleAddModelJp, 0);
            } else {
                sprintf_s(text, 0x100, "you cannot add models over %d!", 100);
                MessageBoxA(hwnd, text, "add model", 0);
            }
            return;
        }
    }

    void* block = operator new(mdl::kSize);   // architecture-correct model ABI
    if (block == nullptr)                     //  thunk 0x4C46F0 in original)
        return;
    unsigned char* model = static_cast<unsigned char*>(block);
    app->ModelSlot(slot) = model;
    std::memset(model, 0, mdl::kSize);
    ModelInitDefaults(model);                                 // 0x4A8DC0

    if (ModelLoadPMD(model, hwnd, path,
                     app->Renderer(), 1,
                     1,                                    // a6: box gate
                     app->EnglishUI(),                     // a7: -> m+12740
                     app->Physics(),                      // a8: -> m+60 scene
                     app->PathWorkspace())) {       // 0x4BF3E0
        app->SceneModified() = 1;
        const char* name =
            app->EnglishUI() == 0 ? mdl::Mdl(model)->name
                                  : mdl::Mdl(model)->nameEn;
        mdl::Mdl(model)->comboSelIndex = static_cast<std::uint8_t>(SendMessageA(
            GetDlgItem(hwnd, 436), CB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(name)));
        SendMessageA(GetDlgItem(hwnd, 474), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(name));
        SendMessageA(GetDlgItem(hwnd, 449), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(name));
        mdl::Mdl(model)->comboSelIndex2 = mdl::Mdl(model)->comboSelIndex;
        SendMessageA(GetDlgItem(hwnd, 436), CB_SETCURSEL,
                     mdl::Mdl(model)->comboSelIndex, 0);
        app->raw<std::uint32_t>(offsets::kDwordA042C) =
            mdl::Mdl(model)->comboSelIndex;
        app->SetSelectedModelSlot(static_cast<std::uint8_t>(slot));
        mikudancestudio::mdl::Mdl(model)->displayState = app->raw<std::uint8_t>(offsets::kByte9EB7E);

        if (app->state.optflag0 != 0) {
            Sub44D610(app);                                   // 0x44D610
            app->state.optflag0 = 0;
            PostLanguageSweep(app);                           // 0x42F1E0
            PostModelReload2(app);                            // 0x40D940
            HandleWindowSize(app);                            // 0x443300
            InvalidateRect(hwnd, nullptr, FALSE);
        } else {
            PostLanguageSweep(app);                           // 0x42F1E0
        }

        SendMessageA(GetDlgItem(hwnd, 491), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, 492), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, 493), BM_SETCHECK, 0, 0);
        SendMessageA(GetDlgItem(hwnd, 490), BM_SETCHECK, 1, 0);
        SendMessageA(GetDlgItem(hwnd, 440), BM_SETCHECK, 1, 0);
        app->EditMode() = ViewportEditMode::Bone;

        HMENU menu = GetMenu(hwnd);
        static const UINT kEnable[] = {0xFD, 0xD9, 0xDC, 0xDA, 0xCA, 0xCB,
                                       0xDB, 0xDE, 0xFB, 0xFC, 0x120, 0x121};
        for (UINT item : kEnable)
            EnableMenuItem(menu, item, 0);
        for (UINT j = 224; j <= 231; ++j)
            EnableMenuItem(GetMenu(hwnd), j, 0);
        for (UINT k = 273; k <= 275; ++k)
            EnableMenuItem(GetMenu(hwnd), k, 0);
        EnableMenuItem(GetMenu(hwnd), 1, 0);
        static const UINT kDisable[] = {0xEE, 0xEF, 0xF0, 0xF1, 0xF2};
        for (UINT item : kDisable)
            EnableMenuItem(GetMenu(hwnd), item, 1);
        EnableWindow(GetDlgItem(hwnd, 424), 1);

        MENUITEMINFOA mii;
        std::memset(&mii, 0, sizeof(mii));
        mii.cbSize = 48;
        mii.fMask = 1;  // MIIM_TYPE (original literal)
        unsigned char* curModel = app->SelectedModel();
        mii.fType = mdl::Mdl(curModel)->physicsMode != 2 ? 0 : 3;
        SetMenuItemInfoA(GetSubMenu(GetMenu(hwnd), 7), 2, FALSE, &mii);
        DrawMenuBar(hwnd);

        EnableWindow(GetDlgItem(hwnd, 421), 0);
        if (app->raw<std::int32_t>(offsets::kDword9DA2C) != 0 ||
            app->raw<std::int32_t>(offsets::kDword9DA30) != 0) {
            EnableWindow(GetDlgItem(hwnd, 421), 1);
            if (app->raw<std::int32_t>(offsets::kDword9DA28) != 0) {
                EnableWindow(GetDlgItem(hwnd, 422), 1);
                PostLanguageSweep2(app);                      // 0x40D070
                return;
            }
        } else if (app->raw<std::int32_t>(offsets::kDword9DA28) != 0) {
            EnableWindow(GetDlgItem(hwnd, 421), 1);
            EnableWindow(GetDlgItem(hwnd, 422), 1);
            PostLanguageSweep2(app);                          // 0x40D070
            return;
        }
        EnableWindow(GetDlgItem(hwnd, 422), 0);
        PostLanguageSweep2(app);                          // 0x40D070
    } else {
        void* v8 = app->ModelSlot(slot);
        if (v8 != nullptr) {
            ModelDispose(static_cast<unsigned char*>(v8));    // 0x48F830
            free(v8);
            app->ModelSlot(slot) = nullptr;
        }
    }
}

}  // namespace mikudancestudio
