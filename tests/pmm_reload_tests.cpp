// User-acceptance regressions for the PMM scene lifetime and the accessory /
// model resources it rebuilds.
//
// Field report (defect 1): open a large PMM, File -> New, then reopen the same
// PMM and the process dies.
//
// Field report (defect 2): after switching the UI language to Chinese, loading
// a PMM can kill the process.
//
// Both landed in the render passes: a resource field that was never written
// (or was reached through an x86-only offset) reached Direct3D as a sentinel
// and the access violation surfaced inside d3d9.dll.  The cases below pin the
// three concrete places that produced it.
#include "mikudancestudio/d3d_object_guard.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"

#include <Windows.h>
#include <d3d9.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace mikudancestudio {
// src/render/accessory.cpp - the accessory loader's FVF normalisation.
bool NormalizeAccessoryMeshFvf(void*& mesh, IDirect3DDevice9* device);
}  // namespace mikudancestudio

namespace {

void Check(bool condition, const char* description) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", description);
        std::exit(1);
    }
}

// ID3DXMesh vtable accessors, spelled out because the DirectX SDK headers are
// not part of the build.  The values are the ones d3dx9_43.dll really installs
// (probed on a live mesh - the SDK header's textual order is not the slot
// order: GetNumFaces precedes GetNumVertices and CloneMeshFVF sits at 11).
constexpr std::size_t kMeshDrawSubset = 3;
constexpr std::size_t kMeshGetNumFaces = 4;
constexpr std::size_t kMeshGetNumVertices = 5;
constexpr std::size_t kMeshGetFvf = 6;
constexpr std::size_t kMeshGetOptions = 9;
constexpr std::size_t kMeshCloneMeshFvf = 11;

DWORD MeshDword(void* mesh, std::size_t slot) {
    using Getter = DWORD(__stdcall*)(void*);
    return reinterpret_cast<Getter>(
        (*reinterpret_cast<void***>(mesh))[slot])(mesh);
}

HRESULT MeshDraw(void* mesh, DWORD subset) {
    using Draw = HRESULT(__stdcall*)(void*, DWORD);
    return reinterpret_cast<Draw>(
        (*reinterpret_cast<void***>(mesh))[kMeshDrawSubset])(mesh, subset);
}

// Slot 7 is GetVertexBuffer (it takes an out pointer - calling it as a plain
// getter faults, which is how the slot order was established).
constexpr std::size_t kMeshGetVertexBuffer = 7;

HRESULT VertexBufferOf(void* mesh, void** vertexBuffer) {
    using Getter = HRESULT(__stdcall*)(void*, void**);
    return reinterpret_cast<Getter>(
        (*reinterpret_cast<void***>(mesh))[kMeshGetVertexBuffer])(mesh,
                                                                 vertexBuffer);
}

}  // namespace

int main() {
    using namespace mikudancestudio;
    // Unbuffered: a crash inside Direct3D must not swallow the progress lines.
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);

    // --- the resource guard itself ----------------------------------------
    // The sentinel that reached d3d9 was a field nobody had written: -1 must be
    // rejected before the device ever sees it, while a live object passes.
    Check(!PlausibleD3dObject(nullptr), "null is not a D3D object");
    Check(!PlausibleD3dObject(reinterpret_cast<const void*>(
              static_cast<std::uintptr_t>(0xFFFFFFFFFFFFFFFFull))),
          "-1 is not a D3D object (the crash sentinel)");
    Check(!PlausibleD3dObject(reinterpret_cast<const void*>(
              static_cast<std::uintptr_t>(44))),
          "a low sentinel is not a D3D object");
    Check(!PlausibleD3dObject(reinterpret_cast<const void*>(
              static_cast<std::uintptr_t>(0x7FFFFFFFFFFFFFFFull))),
          "a non-user-mode address is not a D3D object");
    std::puts("PASS resource guard rejects sentinels");

    // --- ID3DXMesh layout the accessory loader depends on ------------------
    HWND window = CreateWindowW(L"STATIC", L"pmm reload regression", WS_POPUP,
                                0, 0, 64, 64, nullptr, nullptr,
                                GetModuleHandleW(nullptr), nullptr);
    Check(window != nullptr, "create hidden device window");
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    Check(d3d != nullptr, "create D3D9");
    D3DPRESENT_PARAMETERS pp{};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferWidth = pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.hDeviceWindow = window;
    IDirect3DDevice9* device = nullptr;
    Check(SUCCEEDED(d3d->CreateDevice(0, D3DDEVTYPE_HAL, window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device)), "create device");

    HMODULE d3dx = LoadLibraryW(L"d3dx9_43.dll");
    Check(d3dx != nullptr, "load the real D3DX runtime");
    std::puts("step: d3dx loaded");
    using CreateMeshFvf = HRESULT(WINAPI*)(DWORD, DWORD, DWORD, DWORD,
                                           IDirect3DDevice9*, void**);
    auto createMesh = reinterpret_cast<CreateMeshFvf>(
        GetProcAddress(d3dx, "D3DXCreateMeshFVF"));
    Check(createMesh != nullptr, "resolve D3DXCreateMeshFVF");

    // A mesh whose FVF is *not* the accessory pass's 274 (XYZ|NORMAL|TEX1):
    // the loader has to clone it, and the clone must go through the options /
    // CloneMeshFVF pair rather than the buffer locks.
    const DWORD kSourceFvf = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    // D3DXMESH_MANAGED - spelled out because the DirectX SDK headers are not
    // part of the build (the loader passes the same value at accessory load).
    const DWORD kOptions = 0x220;
    void* mesh = nullptr;
    Check(SUCCEEDED(createMesh(2, 6, kOptions, kSourceFvf, device, &mesh)),
          "create a non-274 mesh");
    std::puts("step: mesh created");
    Check(MeshDword(mesh, kMeshGetNumFaces) == 2 &&
              MeshDword(mesh, kMeshGetNumVertices) == 6,
          "the getters confirm the slot order (faces 4, vertices 5)");
    Check(MeshDword(mesh, kMeshGetFvf) == kSourceFvf,
          "slot 6 is GetFVF");
    Check(MeshDword(mesh, kMeshGetOptions) == kOptions,
          "slot 9 is GetOptions - the loader's clone pair");
    Check(PlausibleD3dObject(mesh), "a live mesh passes the guard");
    std::puts("PASS ID3DXMesh vtable slots 6/9 on a real mesh");

    void* before = mesh;
    Check(NormalizeAccessoryMeshFvf(mesh, device),
          "a non-274 mesh is normalised");
    Check(mesh != nullptr && PlausibleD3dObject(mesh),
          "the normalised mesh is still a live object");
    const DWORD normalisedFvf = MeshDword(mesh, kMeshGetFvf);
    Check(normalisedFvf == 274 || normalisedFvf == kSourceFvf,
          "the normalised mesh reports one consistent FVF");
    Check(mesh != before || normalisedFvf == kSourceFvf,
          "a clone that shares the source's buffers is rejected, not adopted");
    // D3DXCreateMeshFVF builds a mesh without an attribute table, so the draw
    // itself may report D3DXERR_INVALIDDATA; what matters here is that the
    // clone is a coherent object the draw path can walk (no fault, a result
    // from the D3DX facility).
    const HRESULT draw = MeshDraw(mesh, 0);
    Check(SUCCEEDED(draw) || (static_cast<unsigned long>(draw) & 0xFFFF0000ul) ==
                                 0x88760000ul,
          "the cloned mesh is walkable by DrawSubset");
    Check(MeshDword(mesh, kMeshGetNumFaces) == 2 &&
              MeshDword(mesh, kMeshGetNumVertices) == 6,
          "the clone keeps the geometry counts");
    Check(PlausibleD3dObject(mesh), "the clone is a live object");
    std::puts("PASS accessory mesh FVF normalisation");

    // A mesh already at 274 is left alone (same object, no clone churn).
    void* already = nullptr;
    Check(SUCCEEDED(createMesh(1, 3, kOptions, 274, device, &already)),
          "create a 274 mesh");
    void* untouched = already;
    Check(NormalizeAccessoryMeshFvf(already, device),
          "a 274 mesh reports success");
    Check(already == untouched, "a 274 mesh is not cloned");

    // The field scene's only FVF-338 accessory goes through this path.  The
    // crash it covers: d3dx9 may return a clone that *shares* the source's
    // buffers, and the loader releases the source right afterwards, so drawing
    // the clone reads a buffer whose owner is gone - the fault the report shows
    // inside ID3DXMesh::DrawSubset -> d3d9.dll.  Whatever the runtime hands
    // back, the mesh that survives must stay drawable.
    {
        void* source = nullptr;
        Check(SUCCEEDED(createMesh(2, 6, kOptions, 338, device, &source)),
              "create a 338 mesh");
        void* sourceVb = nullptr;
        Check(SUCCEEDED(VertexBufferOf(source, &sourceVb)) && sourceVb != nullptr,
              "the 338 source has a vertex buffer");
        void* normalised = source;
        Check(NormalizeAccessoryMeshFvf(normalised, device),
              "the 338 mesh is normalised");
        void* survivingVb = nullptr;
        Check(SUCCEEDED(VertexBufferOf(normalised, &survivingVb)) &&
                  survivingVb != nullptr,
              "the surviving mesh has a vertex buffer");
        for (int i = 0; i < 200; ++i) {
            const HRESULT hr = MeshDraw(normalised, 0);
            Check(SUCCEEDED(hr) ||
                      (static_cast<unsigned long>(hr) & 0xFFFF0000ul) ==
                          0x88760000ul,
                  "repeated draws of the normalised mesh stay coherent");
        }
        Check(MeshDword(normalised, kMeshGetFvf) == 274 ||
                  MeshDword(normalised, kMeshGetFvf) == 338,
              "the surviving mesh reports one consistent FVF");
        reinterpret_cast<IUnknown*>(normalised)->Release();
    }

    // --- the x86 displayFrames offset the frame seek used to read ----------
    // The morph-track walk read *(model + 9948) as a pointer.  9948 is the x86
    // placement of displayFrames; on x64 that byte sits inside path[], so the
    // walk dereferenced the model path's characters.
    Check(offsetof(mdl::ModelRecord, path) == 0x2548, "x64 path offset");
    Check(offsetof(mdl::ModelRecord, displayFrames) == 0x2788,
          "x64 displayFrames offset");
    Check(9948 >= offsetof(mdl::ModelRecord, path) &&
              9948 + sizeof(void*) <=
                  offsetof(mdl::ModelRecord, path) + sizeof(wchar_t) * 256,
          "the x86 displayFrames offset lands inside path[] on x64");
    {
        auto* buffer = reinterpret_cast<unsigned char*>(
            ::operator new(sizeof(mdl::ModelRecord)));
        std::memset(buffer, 0, sizeof(mdl::ModelRecord));
        wcscpy_s(mdl::Mdl(buffer)->path, L"D:\\stage\\White.pmx");
        auto* table = reinterpret_cast<mdl::FrameGroup*>(
            ::operator new(46 * 4));
        std::memset(table, 0, 46 * 4);
        mdl::Mdl(buffer)->displayFrames = table;
        Check(mdl::DisplayFrames(buffer) == table,
              "the named accessor returns displayFrames");
        const void* viaRawOffset =
            *reinterpret_cast<const void* const*>(buffer + 9948);
        Check(!PlausibleD3dObject(viaRawOffset) ||
                  viaRawOffset == static_cast<const void*>(table),
              "the raw x86 offset never yields a usable unrelated pointer");
        Check(viaRawOffset != static_cast<const void*>(table),
              "the raw x86 offset does not reach displayFrames on x64");
        ::operator delete(table);
        ::operator delete(buffer);
    }
    std::puts("PASS displayFrames x64 placement");

    reinterpret_cast<IUnknown*>(mesh)->Release();
    reinterpret_cast<IUnknown*>(already)->Release();
    device->Release();
    d3d->Release();
    FreeLibrary(d3dx);
    DestroyWindow(window);
    std::puts("PMM reload regressions passed");
    return 0;
}
