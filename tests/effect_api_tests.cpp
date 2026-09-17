#include "mikudancestudio/effect_api.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/scene_ownership.hpp"
#include "btBulletDynamicsCommon.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

extern "C" {
int ExpGetPmdOrder(int);
int ExpGetAcsOrder(int);
int ExpGetCurrentObject();
std::uint32_t ExpGetPmdID(int);
std::uint32_t ExpGetAcsID(int);
char* ExpGetPmdFilename(int);
char* ExpGetAcsFilename(int);
}

namespace {
bool effectEnabled = true;
bool englishMenu = true;

HMENU MakeEffectPopup(bool english) {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 40005, L"Effect Mapping");
    AppendMenuW(menu, MF_STRING | MF_CHECKED, english ? 40020 : 40006, L"Enable Effect");
    AppendMenuW(menu, MF_STRING | MF_CHECKED, 40001, L"Auto Reload");
    return menu;
}

LRESULT CALLBACK FakeEffectWindow(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    if (message == WM_COMMAND) {
        HMENU root = GetMenu(hwnd);
        HMENU popup = GetSubMenu(root, 0);
        if (LOWORD(wp) == 40006) {
            effectEnabled = (GetMenuState(popup, 40006, MF_BYCOMMAND) & MF_CHECKED) == 0;
            CheckMenuItem(popup, 40006, effectEnabled ? MF_CHECKED : MF_UNCHECKED);
            return 0;
        }
        if (LOWORD(wp) == 260) {
            RemoveMenu(root, 0, MF_BYPOSITION);
            DestroyMenu(popup);
            englishMenu = !englishMenu;
            AppendMenuW(root, MF_POPUP, reinterpret_cast<UINT_PTR>(MakeEffectPopup(englishMenu)), L"MMEffect");
            return 0;
        }
    }
    return DefWindowProcW(hwnd, message, wp, lp);
}

void Check(bool condition, const char* description) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", description);
        std::exit(1);
    }
}
}

int main() {
    using namespace mikudancestudio;
    auto app = std::make_unique<MMDApp>();
    std::memset(&app->state, 0, sizeof(app->state));
    g_Block = app.get();
    app->WindowLayoutReady() = 1;
    app->state.fpsOverlayFrameCount = 17;
    {
        ScopedSceneMutation load(*app);
        Check(app->SceneMutationInProgress(), "Loading suspends scene access");
        app->WindowLayoutReady() = 1; // A nested WM_SIZE must not reopen rendering.
        FrameDriver(app.get()); // Renderer/scene are deliberately uninitialized.
        {
            ScopedSceneMutation reset(*app);
            Check(app->SceneMutationInProgress(), "Nested abort/reset remains guarded");
        }
        FrameDriver(app.get());
        Check(app->SceneMutationInProgress() && app->state.fpsOverlayFrameCount == 17,
              "Partial scenes are not rendered after nested reset or resize");
    }
    Check(!app->SceneMutationInProgress() && app->WindowLayoutReady() == 1,
          "Scene access and previous layout gate restored on scope exit");
    // Exercise native command routing and popup recreation without loading a
    // third-party DLL: the fake follows MME's resource IDs and switch handler.
    WNDCLASSW wc{};
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"EffectMenuRegression";
    wc.lpfnWndProc = FakeEffectWindow;
    Check(RegisterClassW(&wc) != 0, "Register menu test window");
    HWND window = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
        0, 0, 400, 300, nullptr, nullptr, wc.hInstance, nullptr);
    Check(window != nullptr, "Create menu test window");
    HMENU menu = CreateMenu();
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(MakeEffectPopup(true)), L"MMEffect");
    SetMenu(window, menu);
    app->state.hwnd = window;
    InstallEffectMenuCompatibility(app.get());
    InstallEffectMenuCompatibility(app.get()); // Installing twice must not chain twice.
    Check(GetMenuState(menu, 40020, MF_BYCOMMAND) == UINT(-1) &&
          (GetMenuState(menu, 40006, MF_BYCOMMAND) & MF_CHECKED) != 0,
          "English enable item uses the native switch command");
    SendMessageW(window, WM_COMMAND, 40006, 0);
    Check(!effectEnabled, "Menu command disables the engine");
    for (int i = 0; i < 3; ++i) {
        SendMessageW(window, WM_COMMAND, 260, 0);
        Check(GetMenuState(menu, 40006, MF_BYCOMMAND) == MF_UNCHECKED && !effectEnabled,
              "Language recreation preserves disabled state and command ID");
    }
    SendMessageW(window, WM_COMMAND, 40006, 0);
    Check(effectEnabled, "Menu command re-enables after language changes");
    SendMessageW(window, WM_COMMAND, 260, 0);
    Check((GetMenuState(menu, 40006, MF_BYCOMMAND) & MF_CHECKED) != 0 && effectEnabled,
          "Language recreation preserves enabled state");
    auto model = std::make_unique<mdl::ModelRecord>();
    auto replacement = std::make_unique<mdl::ModelRecord>();
    auto accessory = std::make_unique<mdl::AccessoryRecord>();
    auto renderer = std::make_unique<D3DRenderer>();
    std::memset(renderer.get(), 0, sizeof(*renderer));
    app->Renderer() = renderer.get();
    // Explicit Chinese fallback, independent of the machine's active locale.
    renderer->localeTable[0] = _create_locale(LC_ALL, ".936");
    Check(renderer->localeTable[0] != nullptr, "Chinese locale available");
    app->ModelSlot(3) = reinterpret_cast<unsigned char*>(model.get());
    app->AccessorySlot(7) = accessory.get();
    model->comboSelIndex = 1;
    app->state.accessoryRenderSplitOrder = 1;
    app->state.activeRenderObject = model.get();

    BeginEffectObjectRegistration();
    Check(ExpGetCurrentObject() == 0, "Unregistered models must not reach MME draw hook");
    Check(ExpGetPmdOrder(0) == 2 && ExpGetCurrentObject() == 2,
          "Registered model uses combined order");
    Check(ExpGetPmdID(0) == static_cast<std::uint32_t>(
              reinterpret_cast<std::uintptr_t>(model.get())), "Model ID ABI");
    Check(ExpGetAcsID(0) == static_cast<std::uint32_t>(
              reinterpret_cast<std::uintptr_t>(accessory.get())), "Accessory ID ABI");
    Check(ExpGetPmdID(1) == 0 && ExpGetAcsID(1) == 0, "Missing IDs are zero");

    app->state.activeRenderObject = accessory.get();
    Check(ExpGetCurrentObject() == 0, "Model registration must not register accessory");
    Check(ExpGetAcsOrder(0) == -1 && ExpGetCurrentObject() == -1,
          "Pre-model accessory order");
    app->state.accessoryRenderSplitOrder = 0;
    Check(ExpGetAcsOrder(0) == 2 && ExpGetCurrentObject() == 2,
          "Post-model accessory order");

    // Simulate deletion followed by reuse of the same occupied ordinal.
    app->ModelSlot(3) = reinterpret_cast<unsigned char*>(replacement.get());
    replacement->comboSelIndex = 1;
    BeginEffectObjectRegistration();
    Check(ExpGetCurrentObject() == 0, "Accessory registration expires each frame");
    app->state.activeRenderObject = replacement.get();
    Check(ExpGetCurrentObject() == 0, "Replacement must not inherit old registration");
    Check(ExpGetPmdOrder(0) == 1 && ExpGetCurrentObject() == 1,
          "Replacement registers normally");

    wcscpy_s(replacement->path, L"C:\\models\\星穹铁道—遐蝶.pmx");
    wchar_t decoded[256]{};
    Check(MultiByteToWideChar(936, 0, ExpGetPmdFilename(0), -1, decoded, 256) > 0 &&
              wcscmp(decoded, replacement->path) == 0,
          "Chinese model filename survives locale fallback");
    wcscpy_s(accessory->sourcePath, L"C:\\effects\\镰刀.x");
    Check(MultiByteToWideChar(936, 0, ExpGetAcsFilename(0), -1, decoded, 256) > 0 &&
              wcscmp(decoded, accessory->sourcePath) == 0,
          "Chinese accessory filename survives locale fallback");
    wcscpy_s(replacement->path, L"C:\\models\\ミク.pmx");
    Check(MultiByteToWideChar(932, 0, ExpGetPmdFilename(0), -1, decoded, 256) > 0 &&
              wcscmp(decoded, replacement->path) == 0, "CP932 path remains compatible");
    Check(ExpGetPmdFilename(1) == nullptr && ExpGetAcsFilename(1) == nullptr,
          "Missing filenames remain null");

    // Language changes must touch the named UI flag, not x86 offset 12740
    // (which is inside boneListRowType on x64), and use ABI-correct names.
    app->AccessorySlot(7) = nullptr;
    app->state.optflag[0] = 1;
    HWND combo = CreateWindowA("COMBOBOX", "", WS_CHILD | CBS_DROPDOWNLIST,
        0, 0, 200, 100, window, reinterpret_cast<HMENU>(436), wc.hInstance, nullptr);
    Check(combo != nullptr, "Create model selector");
    strcpy_s(replacement->name, "JP model");
    strcpy_s(replacement->nameEn, "EN model");
    std::memset(replacement->boneListRowType, 0x5A, sizeof(replacement->boneListRowType));
    for (int language : {1, 2, 0}) {
        app->state.englishUI = static_cast<unsigned char>(language);
        LocalizeUI(app.get());
        Check(replacement->physicsFlags == (language == 1), "Model UI language flag");
        for (auto type : replacement->boneListRowType)
            Check(type == 0x5A, "Language change preserves model row metadata");
        char name[64]{};
        SendMessageA(combo, CB_GETLBTEXT, 1, reinterpret_cast<LPARAM>(name));
        Check(strcmp(name, language == 1 ? "EN model" : "JP model") == 0,
              "Model selector reads the correct name on both ABIs");
    }
    DestroyWindow(window);
    app->state.hwnd = nullptr;
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    _free_locale(renderer->localeTable[0]);
    app->ModelSlot(3) = nullptr;
    app->AccessorySlot(7) = nullptr;
    app->Renderer() = nullptr;

    // A ground body remains after scene models have been disposed. The old
    // x86 collision-array offsets crashed here in a normal x64 WM_CLOSE.
    auto* physics = static_cast<PhysicsScene*>(std::calloc(1, sizeof(PhysicsScene)));
    app->Physics() = physics;
    physics->collisionConfig = new btDefaultCollisionConfiguration();
    physics->dispatcher = new btCollisionDispatcher(physics->collisionConfig);
    physics->broadphase = new bt32BitAxisSweep3(btVector3(-100, -100, -100),
                                             btVector3(100, 100, 100));
    physics->solver = new btSequentialImpulseConstraintSolver();
    physics->world = new btDiscreteDynamicsWorld(physics->dispatcher,
        physics->broadphase, physics->solver, physics->collisionConfig);
    auto* shape = new btBoxShape(btVector3(1, 1, 1));
    auto* motion = new btDefaultMotionState(btTransform::getIdentity());
    physics->groundBody = new btRigidBody(0, motion, shape);
    physics->world->addRigidBody(physics->groundBody);
    physics->world->addConstraint(new btPoint2PointConstraint(
        *physics->groundBody, btVector3(0, 0, 0)));
    ShutdownCleanup(app.get());
    Check(app->Physics() == nullptr, "Physics world teardown completes");
    g_Block = nullptr;
    std::puts("PASS effect menu, language, scene mutation, IDs, order lifecycle, filenames, and teardown");
}
