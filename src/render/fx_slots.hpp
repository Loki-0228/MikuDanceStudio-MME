// ===========================================================================
// ID3DXEffect vtable slot helpers (shared by model_renderers.cpp and
// accessory.cpp)
// ===========================================================================
// The original binaries drive the d3dx9 effect object exclusively through
// its vtable, and one method keeps the same SLOT NUMBER in the x86 build
// (d3dx9_32.dll, byte offset = slot*4) and the x64 rebuild (d3dx9_43.dll,
// byte offset = slot*8).  Naming a call by slot is therefore the only
// spelling that is correct on both architectures; the old byte-offset
// helpers divided an x86 offset by sizeof(void*) and halved every slot on
// x64, landing each call on an unrelated method.  Slots used here are
// pinned against the x64 original (base 0x7FF7CB420000) and agree with
// the d3dx9effect.h declaration order (IUnknown, ID3DXBaseEffect, then
// ID3DXEffect) and with device_reset.cpp's OnLostDevice slot 69:
//   slot 22 SetBool       x64 byte 176: 0x7FF7CB4C162F ("parthf"),
//                         0x7FF7CB4C3500 ("transp"), 0x7FF7CB4FE831 ("spadd")
//   slot 32 SetFloatArray x64 byte 256: 0x7FF7CB4C21E2 ("LightDir"),
//                         0x7FF7CB4FE3EF ("EgColor")
//   slot 38 SetMatrix     x64 byte 304: 0x7FF7CB4C22B3
//                         ("matWorldViewProj"), 0x7FF7CB4FE2D0 ("matWRotate")
//   slot 39 GetMatrix     x64 byte 312: 0x7FF7CB4FE29F ("matRotate") - the
//                         original reads the parameter back here between
//                         two matrix multiplies; SetMatrixTranspose is the
//                         later slot 44 and is never called
//   slot 58 SetTechnique  x64 byte 464: 0x7FF7CB4C1201 ("ZValuePlotTec"),
//                         0x7FF7CB4C374F ("ColorRenderTec"),
//                         0x7FF7CB4D87C0+ ("BShadow*/BufferShadow*")
//   slot 63 Begin         x64 byte 504: 0x7FF7CB4C1974, 0x7FF7CB4FE848
//   slot 64 BeginPass     x64 byte 512: 0x7FF7CB4FE86C
//   slot 65 CommitChanges x64 byte 520: no original call site
//   slot 66 EndPass       x64 byte 528: 0x7FF7CB4FE888
//   slot 67 End           x64 byte 536: 0x7FF7CB4C1D07, 0x7FF7CB4FE898
// =========================================================================//
#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cstddef>

#include "mikudancestudio/d3dx_dyn.hpp"

namespace mikudancestudio::fx {

// ID3DXEffect vtable slots in d3dx9effect.h declaration order; the
// numbering is identical for the x86 and x64 d3dx9 runtimes.
enum Slot : std::size_t {
    kSetBool = 22,
    kSetFloatArray = 32,
    kSetMatrix = 38,
    kGetMatrix = 39,
    kSetTechnique = 58,
    kBegin = 63,
    kBeginPass = 64,
    kCommitChanges = 65,
    kEndPass = 66,
    kEnd = 67,
};

template <typename Fn>
Fn Method(void* effect, Slot slot) {
    return reinterpret_cast<Fn>(
        (*reinterpret_cast<void***>(effect))[static_cast<std::size_t>(slot)]);
}

inline HRESULT SetTechnique(void* effect, const char* technique) {
    using Fn = HRESULT(__stdcall*)(void*, const char*);
    return Method<Fn>(effect, kSetTechnique)(effect, technique);
}

// The original writes these through slot 22, SetBool (BOOL shares the
// 4-byte integer ABI); the call sites pass flag-style ints such as
// "spadd"/"parthf"/"transp".
inline HRESULT SetBool(void* effect, const char* parameter, int value) {
    using Fn = HRESULT(__stdcall*)(void*, const char*, int);
    return Method<Fn>(effect, kSetBool)(effect, parameter, value);
}

inline HRESULT SetFloatArray(void* effect, const char* parameter,
                             const float* values, UINT count) {
    using Fn = HRESULT(__stdcall*)(void*, const char*, const float*, UINT);
    return Method<Fn>(effect, kSetFloatArray)(effect, parameter, values,
                                              count);
}

inline HRESULT SetMatrix(void* effect, const char* parameter,
                         const d3dx::D3DXMATRIXF* matrix) {
    using Fn = HRESULT(__stdcall*)(void*, const char*,
                                   const d3dx::D3DXMATRIXF*);
    return Method<Fn>(effect, kSetMatrix)(effect, parameter, matrix);
}

inline HRESULT GetMatrix(void* effect, const char* parameter,
                         d3dx::D3DXMATRIXF* matrix) {
    using Fn = HRESULT(__stdcall*)(void*, const char*, d3dx::D3DXMATRIXF*);
    return Method<Fn>(effect, kGetMatrix)(effect, parameter, matrix);
}

inline HRESULT Begin(void* effect, UINT* passes) {
    using Fn = HRESULT(__stdcall*)(void*, UINT*, DWORD);
    return Method<Fn>(effect, kBegin)(effect, passes, 0);
}

// The original's discarded-pass-count form (a stack UINT the caller never
// reads), used where only the side effect of entering the technique
// matters.
inline HRESULT Begin(void* effect) {
    UINT passes = 0;
    return Begin(effect, &passes);
}

inline HRESULT BeginPass(void* effect, UINT pass = 0) {
    using Fn = HRESULT(__stdcall*)(void*, UINT);
    return Method<Fn>(effect, kBeginPass)(effect, pass);
}

inline HRESULT EndPass(void* effect) {
    using Fn = HRESULT(__stdcall*)(void*);
    return Method<Fn>(effect, kEndPass)(effect);
}

inline HRESULT End(void* effect) {
    using Fn = HRESULT(__stdcall*)(void*);
    return Method<Fn>(effect, kEnd)(effect);
}

}  // namespace mikudancestudio::fx
