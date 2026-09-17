#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/d3dx_effect.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
}
int main() {
    using namespace mikudancestudio;
    HWND window = CreateWindowW(L"STATIC", L"Device reset regression", WS_POPUP,
        0, 0, 64, 64, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    Check(window != nullptr, "create hidden device window");
    auto* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    Check(d3d != nullptr, "create D3D9");
    auto app = std::make_unique<MMDApp>();
    std::memset(&app->state, 0, sizeof(app->state));
    auto renderer = std::make_unique<D3DRenderer>();
    std::memset(renderer.get(), 0, sizeof(*renderer));
    app->Renderer() = renderer.get();
    auto& pp = renderer->presentParameters;
    pp.Windowed = TRUE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferWidth = pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = window;
    Check(SUCCEEDED(d3d->CreateDevice(0, D3DDEVTYPE_HAL, window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &renderer->device)), "create device");
    auto* device = renderer->device;
    HMODULE dll = LoadLibraryW(L"d3dx9_43.dll");
    Check(dll != nullptr, "load real D3DX runtime");
    using CreateEffect = HRESULT(WINAPI*)(IDirect3DDevice9*, const void*, UINT,
        const void*, void*, DWORD, void*, void**, void**);
    auto create = reinterpret_cast<CreateEffect>(GetProcAddress(dll, "D3DXCreateEffect"));
    Check(create != nullptr, "resolve effect compiler");
    const char source[] = "technique T { pass P { ZEnable = false; } }";
    void* effect = nullptr;
    Check(SUCCEEDED(create(device, source, sizeof(source) - 1, nullptr, nullptr,
        0, nullptr, &effect, nullptr)), "create real effect with state blocks");
    renderer->effect = reinterpret_cast<ID3DXEffect*>(effect);
    Check(SUCCEEDED(d3dx::FxSetTechnique(effect, "T")), "select test technique");
    for (UINT size : {64u, 128u, 256u, 64u}) {
        // Exercise state blocks before loss, and fresh capture resources on
        // each cycle, as AVI export does when growing/restoring its target.
        Check(SUCCEEDED(device->BeginScene()), "begin effect before reset");
        UINT passes = 0;
        Check(SUCCEEDED(d3dx::FxBegin(effect, &passes)) && passes == 1, "begin effect");
        Check(SUCCEEDED(d3dx::FxBeginPass(effect, 0)), "begin pass");
        Check(SUCCEEDED(d3dx::FxEndPass(effect)), "end pass");
        Check(SUCCEEDED(d3dx::FxEnd(effect)), "end effect");
        Check(SUCCEEDED(device->EndScene()), "end scene");
        Check(SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
            &renderer->backbufferSurface)), "retain backbuffer as host does");
        Check(SUCCEEDED(device->CreateRenderTarget(32, 32, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &renderer->captureSurface, nullptr)), "capture target");
        pp.BackBufferWidth = pp.BackBufferHeight = size;
        PostDeviceReset(app.get());
        Check(renderer->backbufferSurface == nullptr && renderer->captureSurface == nullptr,
              "reset releases old default-pool surfaces");
        Check(SUCCEEDED(device->TestCooperativeLevel()), "device operational after reset");
        IDirect3DSurface9* back = nullptr;
        Check(SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)), "new backbuffer");
        D3DSURFACE_DESC desc{}; back->GetDesc(&desc); back->Release();
        Check(desc.Width == size && desc.Height == size, "export target resized successfully");
        Check(SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF336699, 1, 0)),
              "render after reset");
    }
    reinterpret_cast<IUnknown*>(effect)->Release(); renderer->effect = nullptr;
    PostDeviceReset(app.get()); // Fixed-function/no-effect path remains valid.
    Check(SUCCEEDED(device->TestCooperativeLevel()), "reset without effect");
    device->Release(); renderer->device = nullptr;
    d3d->Release(); FreeLibrary(dll); DestroyWindow(window);
    std::puts("Real D3DX effect and repeated export-size device resets passed.");
}
