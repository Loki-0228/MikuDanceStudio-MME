#include "mikudancestudio/effect_api.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/ported_funcs.hpp"
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
    std::puts("PASS effect IDs, order lifecycle, slot reuse, filenames, and physics teardown");
}
