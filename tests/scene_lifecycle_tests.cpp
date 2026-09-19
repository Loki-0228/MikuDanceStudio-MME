#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/ui_language.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/panel_controls.hpp"
#include <commctrl.h>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cstring>

namespace mikudancestudio {
void LoadSceneFile();
void CmdControl450(MMDApp*, HWND, std::uint16_t, std::uint16_t);
void DispatchLetterHotkeys(MMDApp*, HWND);
void PumpDeleteRebuild(MMDApp*, HWND, bool);
}
using namespace mikudancestudio;
namespace {
void Check(bool ok, const char* what) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(7); }
}
int confirmedDialogs = 0;
LRESULT CALLBACK ConfirmTestDialog(int code, WPARAM wp, LPARAM lp) {
    if (code == HCBT_ACTIVATE) {
        HWND dialog = reinterpret_cast<HWND>(wp);
        wchar_t cls[32]{}; GetClassNameW(dialog, cls, 32);
        if (std::wcscmp(cls, L"#32770") == 0 && GetDlgItem(dialog, IDOK)) {
            ++confirmedDialogs;
            PostMessageW(dialog, WM_COMMAND, IDOK, 0);
        }
    }
    return CallNextHookEx(nullptr, code, wp, lp);
}
// Exercise production message/command paths against the loaded model. These
// edits exist only in this test process; the user's PMM is never saved.
void CheckEditorCommands(MMDApp* app) {
    const HWND window = app->MainWindow();
    auto pressLetter = [&](unsigned char key) {
        unsigned char keys[256]{};
        app->LetterHotkeys().Update(keys, true);
        keys[key] = 0x80;
        app->LetterHotkeys().Update(keys, true);
        DispatchLetterHotkeys(app, GetDlgItem(window, 494));
        keys[key] = 0;
        app->LetterHotkeys().Update(keys, true);
    };
    int slot = 0;
    while (slot < kModelSlotCount && (!app->ModelSlot(slot) ||
           mdl::Mdl(app->ModelSlot(slot))->boneCount < 2)) ++slot;
    Check(slot < kModelSlotCount, "scene has editable bones");
    app->SetSelectedModelSlot(slot); app->state.optflag[0] = 0;
    auto* raw = app->ModelSlot(slot); auto& model = *mdl::Mdl(raw);
    PostLoadInit(raw); PostLanguageSweep(app); PostLanguageSweep2(app);

    // A click without WM_MOUSEMOVE must use its own coordinates and select
    // the visible root row, even when the previously stored position is stale.
    app->MouseX() = 900; app->MouseY() = 900;
    SendMessageW(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(40, 167));
    SendMessageW(window, WM_LBUTTONUP, 0, MAKELPARAM(40, 167));
    Check(model.selectedBone == static_cast<int>(model.displayRootBone) &&
          model.boneSelection[model.displayRootBone], "name-column click selects the bone");

    CmdControl450(app, window, 494, 0);
    int target = -1;
    for (unsigned i = 1; i < model.boneCount; ++i) {
        if (!model.boneSelection[i] || model.boneTable[i].parent < 0) continue;
        target = static_cast<int>(i);
        model.boneTable[i].trans[1] = 5.f;
        model.boneTable[i].rotQuat[2] = .5f;
    }
    Check(target >= 0, "select-all includes parented bones");
    CmdControl450(app, window, 495, 0);
    for (unsigned i = 1; i < model.boneCount; ++i)
        if (model.boneSelection[i] && model.boneTable[i].parent >= 0)
            Check(model.boneTable[i].trans[1] == 0.f &&
                  model.boneTable[i].rotQuat[2] == 0.f && model.boneTable[i].rotQuat[3] == 1.f,
                  "initialize resets every selected parented bone");

    // Confirm the actual F-paste dialog and verify the target track, not just
    // its enable predicate. Use a name absent from the target model.
    model.selectedBone = target;
    operator delete(app->BoneClipboard());
    app->BoneClipboard() = static_cast<mdl::BoneClipboardRecord*>(operator new(sizeof(mdl::BoneClipboardRecord)));
    auto& copy = *app->BoneClipboard(); copy = {};
    std::strcpy(copy.name, "different source bone");
    copy.rotation[3] = 1.f; copy.rotation[2] = .25f;
    copy.frame = 0; app->ClipboardCounts().bones = 1;
    app->CurrentFrame() = 100;
    const int dialogsBefore = confirmedDialogs;
    HHOOK hook = SetWindowsHookExW(WH_CBT, ConfirmTestDialog, nullptr, GetCurrentThreadId());
    Check(hook != nullptr, "install test-owned dialog responder");
    app->state.ctrlModifierState = 3;
    pressLetter('F');
    app->state.ctrlModifierState = 0;
    UnhookWindowsHookEx(hook);
    Check(confirmedDialogs == dialogsBefore + 1, "one paste confirmation was handled");
    auto* keys = mdl::BoneKeys(raw);
    int key = target;
    while (keys[key].next && keys[key].frame < 100) key = static_cast<int>(keys[key].next);
    Check(keys[key].frame == 100 && keys[key].rotation[2] != 0.f, "OK pastes onto selected bone");
    pressLetter('I');
    Check(keys[key].frame == 101, "I shortcut inserts at a keyed frame from panel focus");
    pressLetter('K');
    Check(keys[key].frame == 100, "K shortcut removes the inserted empty frame from panel focus");
    std::memset(model.boneSelection, 0, model.boneCount); model.boneSelection[target] = 1;
    app->state.deleteKeyState = 1;
    PumpDeleteRebuild(app, GetDlgItem(window, 494), true);
    app->state.deleteKeyState = 0;
    Check(keys[key].frame == 0, "Del deletes the current selected key");

    app->state.optflag[0] = 1;
    app->state.cameraPerspective = 0;
    HWND slider = GetDlgItem(window, panel::kFovSlider);
    SendMessageW(slider, TBM_SETPOS, TRUE, 65);
    SendMessageW(window, WM_HSCROLL, TB_THUMBTRACK, reinterpret_cast<LPARAM>(slider));
    Check(app->CameraFov() == 65.f, "FOV slider message reaches camera state");
    ApplyFrameCameraProjection(app);
    D3DMATRIX projection{};
    Check(SUCCEEDED(app->Renderer()->device->GetTransform(D3DTS_PROJECTION, &projection)) &&
          projection._22 > 1.5f && projection._22 < 1.6f, "FOV reaches live D3D projection");
    std::puts("editor command integration passed");
}
}
int wmain(int argc, wchar_t** argv) {
    using namespace mikudancestudio;
    if (argc < 2) return 2;
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    auto* app = new MMDApp();
    g_Block = app;
    app->state = MMDAppState{};
    app->InitDefaults();
    if (!InitMainWindowAndD3D(app, GetModuleHandleW(nullptr), SW_HIDE)) return 3;
    ShowWindow(app->MainWindow(), SW_HIDE);
#ifdef REQUIRE_MME
    Check(GetModuleHandleW(L"MMEffect.dll") && GetModuleHandleW(L"MMHack.dll"),
          "MME test requires local MMEffect/MMHack and d3d9 proxy DLLs");
    std::puts("MME loaded");
#endif
    const int cycles = argc > 2 ? (std::max)(1, _wtoi(argv[2])) : 10;
    int expectedModels = -1, expectedAccessories = -1;
    for (int cycle = 0; cycle < cycles; ++cycle) {
        app->EnglishUI() = cycle % 3;
        LocalizeUI(app);
        std::printf("cycle %d language %d: reset\n", cycle, int(app->EnglishUI()));
        ResetAppState(app);
        wcscpy_s(app->EnvFileName(), 256, argv[1]);
        std::puts("load PMM");
        LoadSceneFile();
        int models = 0, accessories = 0;
        for (int slot = 0; slot < kModelSlotCount; ++slot)
            if (app->ModelSlot(slot)) ++models;
        for (int slot = 0; slot < 256; ++slot)
            if (app->AccessorySlot(slot)) ++accessories;
        std::printf("loaded %d models, %d accessories\n", models, accessories);
        if (models == 0 && accessories == 0) return 5;
        if (cycle == 0) { expectedModels = models; expectedAccessories = accessories; }
        if (models != expectedModels || accessories != expectedAccessories) return 6;
        std::puts("render 12 frames with physics");
        for (int frame = 0; frame < 12; ++frame) {
            app->DeltaTime() = 1.f / 30.f;
            app->state.messageSeen = 10;
            FrameDriver(app);
            if (!HeapValidate(GetProcessHeap(), 0, nullptr)) return 4;
        }
        if (argc > 3) CheckEditorCommands(app);
        std::puts("cycle passed");
    }
    ResetAppState(app);
    ShutdownCleanup(app); delete app; g_Block = nullptr;
    std::puts("PMM new/reopen and language-switch lifecycle passed");
}
