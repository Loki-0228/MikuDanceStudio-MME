#pragma once

// ===========================================================================
// Direct3D object screening for the render passes
// ===========================================================================
// The port mirrors record layouts that carry raw D3D resource pointers.  When
// such a field is never written - or is reached through an offset that only
// holds it on x86 - the renderer hands Direct3D a sentinel (typically -1) and
// the access violation surfaces deep inside d3d9.dll, far from the port call
// that passed it (the crash report then names d3d9.dll and nothing else).
//
// Every such pointer is therefore screened before it is bound: a D3D object
// lives in committed, readable user memory and starts with a vtable pointer
// that lands in a mapped image.  The vtable itself sits in the image's
// read-only data (d3dx9_43.dll's mesh vtables are a plain .rdata structure),
// so only readability - not executability - is required of it.
//
// Header-only and dependency-light: the check is two VirtualQuery calls, so it
// is cheap enough for the per-draw path and usable from any translation unit.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace mikudancestudio {

inline bool PlausibleD3dObject(const void* object) noexcept {
    const ULONG_PTR value = reinterpret_cast<ULONG_PTR>(object);
    if (value < 0x10000 || value > 0x00007FFFFFFFFFFFULL)
        return false;
    MEMORY_BASIC_INFORMATION region{};
    if (VirtualQuery(object, &region, sizeof(region)) == 0 ||
        region.State != MEM_COMMIT ||
        (region.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
        return false;
    const void* vtable = *reinterpret_cast<const void* const*>(object);
    if (vtable == nullptr)
        return false;
    MEMORY_BASIC_INFORMATION code{};
    if (VirtualQuery(vtable, &code, sizeof(code)) == 0 ||
        code.State != MEM_COMMIT ||
        code.Type != MEM_IMAGE ||
        (code.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
        return false;
    return true;
}

}  // namespace mikudancestudio
