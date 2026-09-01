// =============================================================================
// MMDxShow.dll — Phase B: CPushSourceDIBSq  (push_source.cpp)
// =============================================================================
// Faithful port of the DIB-sequence push-source filter class from the original
// MikuMikuDanceE_v932/Data/MMDxShow.dll (image base 0x10000000):
//
//   CPushSourceDIBSq constructor                    0x10001A70
//   CPushSourceDIBSq scalar deleting destructor     0x10001B50
//        (destructor body proper)                   0x10001670
//   lpfnNew factory (g_Templates[0].m_lpfnNew)      0x10001B70
//   NonDelegatingQueryInterface (primary +0x00)     0x100018B0
//   IPushSource::QueryInterface        (+0x70 vtbl) 0x10001930
//   IPushSource::AddRef                             0x10001910
//   IPushSource::Release                            0x10001940
//   IPushSource slot +0x0C (SetBitmapInfo)          0x10001730
//   IPushSource slot +0x10 (GetStreamingState)      0x100017D0
//   IPushSource slot +0x14 (StartStreaming)         0x10001800
//   IPushSource slot +0x18 (BeginStreaming)         0x10001840
//   IPushSource slot +0x1C (GetRate)                0x10001880
//   IAMovieSetup::Register / Unregister             0x10003A10 / 0x10003A80
//     (shared CBaseFilter-level bodies — the CSource vtable @0x100084C8 and
//      the CPushSourceDIBSq vtable @0x10008290 point at the same functions)
//
// VTABLE ADDRESSES (CPushSourceDIBSq, verified byte-by-byte from .rdata):
//   primary (INonDelegatingUnknown root)  COL 0x10008F2C @ vtbl 0x100082E4
//   IBaseFilter sub-object (+0x0C)        COL 0x10009228 @ vtbl 0x100082A8
//   IAMovieSetup sub-object (+0x10)       COL 0x1000923C @ vtbl 0x10008290
//   IPushSource sub-object (+0x70)        COL 0x10009250 @ vtbl 0x1000826C
//   (each vtable start is preceded by its RTTI COL pointer at -4)
//
// OBJECT LAYOUT (0x78 bytes):
//   +0x00 .. +0x6F  CSource (m_cPins +0x50, m_ppPins +0x54, m_CritSec +0x58)
//   +0x70           IPushSource vptr
//   +0x74           CPushPinDIBSq* m_pPin        (set by the constructor)
//   => sizeof == 0x78 (operator new(0x78) at 0x10001B70)
//
// =============================================================================
// IPushSource SLOT MAP (vtable 0x1000826C; EXE-side consumer contract lives in
// src/app/dshow_record_graph.cpp — IID ECFAB031-72BA-4120-B9F7-8A3D5FD38DEC)
// =============================================================================
//  slot  VA         behavior
//   +0x00 0x10001930 QueryInterface — forwards to the outer unknown kept at
//                    object+0x04 (two-hop chain 0x10001930 -> 0x10001180).
//   +0x04 0x10001910 AddRef          — same forwarding, outer slot +0x04.
//   +0x08 0x10001940 Release         — same forwarding, outer slot +0x08.
//   +0x0C 0x10001730 SetBitmapInfo(const BITMAPINFOHEADER* pBIH /*40 bytes
//                    copied*/, int n, float fps): E_FAIL (0x80004005) once
//                    streaming has begun (pin+0x5B3); else news 0x2C bytes,
//                    copies 0x28 bytes to pin+0x5A0, stores n at pin+0x5A4,
//                    AvgTimePerFrame = 10000000 / (int64)(uint64)fps at
//                    pin+0x5A8, sets pin+0x5B0 = 1 and pin+0x5B1 = 1; S_OK.
//                    fps is a 32-bit stack float (retn 0x10).
//   +0x10 0x100017D0 GetStreamingState(BYTE* pState): E_FAIL if streaming;
//                    else *pState = pin+0x5B1 (one byte, unchecked pointer).
//   +0x14 0x10001800 StartStreaming(DWORD dwBits): E_FAIL if streaming; else
//                    pin+0x5B1 = 0, pin+0x5B4 = dwBits (stored verbatim),
//                    pin+0x5B2 = 1.
//   +0x18 0x10001840 BeginStreaming(): E_FAIL if streaming; else pin+0x5B3=1,
//                    pin+0x5B1=0, pin+0x5B2=0.  This latches the "streaming
//                    begun" flag that makes every other IPushSource method
//                    (and SetBitmapInfo) fail with E_FAIL from then on.
//   +0x1C 0x10001880 GetRate(float* pRate): E_FAIL if streaming; else writes
//                    the 4-byte constant 1.02f (.rdata 0x10008308).  This is
//                    the EXE "RecConfig/version" handshake: the EXE requires
//                    >= 1.01f or it reports "MMDxShow->dll is too old".
//
//  EXE contract cross-check (dshow_record_graph.cpp header comment): the EXE
//  speaks of "vtable slot 7" (RecConfig handshake filling a version float) and
//  "vtable slot 4" (per-frame capture entry).  0-based slot 7 == +0x1C ==
//  GetRate (1.02f >= 1.01f — matches).  0-based slot 4 == +0x10 ==
//  GetStreamingState (per-frame 1-byte state poll of pin+0x5B1, driven by the
//  frame loop 0x46F070); if the EXE comment counted 1-based, slot 4 ==
//  SetBitmapInfo, i.e. the 40-byte DIB-header push.  Verify against the EXE
//  port of 0x46F070 when it lands.
//
// =============================================================================
// HEADER NOTE — all arbitration items against mmdxshow.hpp are RESOLVED as of
// this version (NonDelegatingQueryInterface override declared; m_pPin member
// at +0x74 with sizeof(CPushSourceDIBSq)==0x78 asserted; IPushSource float/
// DWORD signatures; CSource ctor taking the CLSID; CPushPinDIBSq ctor in the
// binary's (phr, pParent) order, definition in push_pin.cpp;
// RegisterFilterMapper1 with external linkage in dll_main.cpp).  Everything
// below is attached as real member definitions; no file-static stand-ins
// remain.
// =============================================================================

#include <new>
#include <string.h>

#include "mmdxshow.hpp"

// --- GUID references (instantiated in dll_main.cpp under <initguid.h>) ------
// CLSID_FilterMapper   .rdata 0x10008680   (values pinned in dll_main.cpp)
// IID_IFilterMapper    .rdata 0x100085C0
DEFINE_GUID(MMDXSHOW_CLSID_FilterMapper,
    0xe436ebb2, 0x524f, 0x11ce, 0x9f, 0x53, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70);
DEFINE_GUID(MMDXSHOW_IID_IFilterMapper,
    0x56a868a3, 0xad4, 0x11ce, 0xb0, 0x3a, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70);

// 0x10003400 — defined with external linkage in dll_main.cpp.
HRESULT RegisterFilterMapper1(const MMDXSHOW_FILTER_SETUP* pSetup,
                              IFilterMapper* pMapper, BOOL bRegister);

// CPushPinDIBSq object offsets touched by this filter (0x5B8-byte object;
// reached through the m_pPin member at filter +0x74):
//   +0x5A0 (1440) BITMAPINFOHEADER* (0x2C bytes allocated, 0x28 copied)
//   +0x5A4 (1444) int n
//   +0x5A8 (1448) REFERENCE_TIME AvgTimePerFrame (64-bit)
//   +0x5B0 (1456) flag byte, set by SetBitmapInfo
//   +0x5B1 (1457) streaming-state byte (Get/Set by IPushSource)
//   +0x5B2 (1458) flag byte, set by StartStreaming and by the dtor
//   +0x5B3 (1459) "streaming begun" latch, set by BeginStreaming and the dtor;
//                 guards EVERY IPushSource method with E_FAIL
//   +0x5B4 (1460) DWORD stored by StartStreaming's second parameter

// VA 0x100018B0 - CPushSourceDIBSq::NonDelegatingQueryInterface (primary
// vtable slot +0x00; the only primary-slot override besides the dtor).
HRESULT STDMETHODCALLTYPE CPushSourceDIBSq::NonDelegatingQueryInterface(
    REFIID riid, void** ppv)
{
    if (!MMDxShow_IsEqualGUID16(&riid, &MMDXSHOW_IID_IPushSource))   // 0x10001000 / .rdata 0x1000830C
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);  // 0x10002790, non-virtual
    return MMDxShow_ReturnSelf(this ? reinterpret_cast<BYTE*>(this) + 0x70
                                    : NULL,    // 0x10004FD0 with this+0x70
                             ppv);
}

// VA 0x10001930 - CPushSourceDIBSq::QueryInterface (IPushSource sub-vtable
// +0x00).  Binary chain: 0x10001930 calls 0x10001180 with (this - 0x64) ==
// the IBaseFilter sub-object (obj+0x0C); 0x10001180 reads the outer unknown
// at obj+0x04 and forwards QueryInterface/AddRef/Release to it.  Expressed
// here directly on m_pOuterUnknown (+0x04).
HRESULT STDMETHODCALLTYPE CPushSourceDIBSq::QueryInterface(REFIID riid, void** ppv)
{
    return m_pOuterUnknown->QueryInterface(riid, ppv);
}

// VA 0x10001910 - CPushSourceDIBSq::AddRef (IPushSource sub-vtable +0x04).
ULONG STDMETHODCALLTYPE CPushSourceDIBSq::AddRef()
{
    return m_pOuterUnknown->AddRef();
}

// VA 0x10001940 - CPushSourceDIBSq::Release (IPushSource sub-vtable +0x08).
ULONG STDMETHODCALLTYPE CPushSourceDIBSq::Release()
{
    return m_pOuterUnknown->Release();
}

// =============================================================================
// IPushSource methods — declared in mmdxshow.hpp, defined here.
// `this` is the full CPushSourceDIBSq object (the compiler's adjustor thunk
// converts the obj+0x70 sub-object pointer the binary code sees).
// =============================================================================

// VA 0x10001730 - IPushSource slot +0x0C - SetBitmapInfo.
// Binary ABI: __stdcall(this, const void* pBIH, int n, float fps), retn 0x10.
HRESULT CPushSourceDIBSq::PushSource_v0C_SetBitmapInfo(const void* pBitmapInfo,
                                                       int n, float fps)
{
    BYTE* pb = reinterpret_cast<BYTE*>(m_pPin);

    if (pb[1459] != 0)                     // pin+0x5B3: streaming begun
        return E_FAIL;                     // 0x80004005

    // operator new(0x2C) — the VC8 CRT new in this DLL returns NULL on
    // exhaustion and the original then memcpys into NULL; keep the unchecked
    // shape (faithful).
    void* p = operator new(0x2C, std::nothrow);
    *reinterpret_cast<void**>(pb + 1440) = p;
    memcpy(p, pBitmapInfo, 0x28);          // 40 bytes = BITMAPINFOHEADER
    // (0x2C allocated, 0x28 copied — the 4 trailing bytes are never written;
    //  a pre-existing pin+0x5A0 allocation is overwritten WITHOUT delete.)
    *reinterpret_cast<int*>(pb + 1444) = n;

    *reinterpret_cast<LONGLONG*>(pb + 1448) =
        10000000 / (LONGLONG)(unsigned __int64)fps;      // AvgTimePerFrame

    pb[1456] = 1;                          // pin+0x5B0
    pb[1457] = 1;                          // pin+0x5B1 (state byte -> 1)
    return S_OK;                           // 0
}

// VA 0x100017D0 - IPushSource slot +0x10 - GetStreamingState.
// Binary ABI: __stdcall(this, BYTE* pState), retn 8; pState is NOT checked.
HRESULT CPushSourceDIBSq::PushSource_v10_GetStreamingState(void* pState)
{
    BYTE* pb = reinterpret_cast<BYTE*>(m_pPin);

    if (pb[1459] != 0)                     // pin+0x5B3: streaming begun
        return E_FAIL;
    *reinterpret_cast<BYTE*>(pState) = pb[1457];   // one byte, pin+0x5B1
    return S_OK;
}

// VA 0x10001800 - IPushSource slot +0x14 - StartStreaming.
// Binary ABI: __stdcall(this, DWORD dwBits), retn 8; dwBits is stored verbatim
// at pin+0x5B4 (0x10001828: mov [ecx+5B4h], edx).
HRESULT CPushSourceDIBSq::PushSource_v14_StartStreaming(DWORD dwBits)
{
    BYTE* pb = reinterpret_cast<BYTE*>(m_pPin);

    if (pb[1459] != 0)                     // pin+0x5B3: streaming begun
        return E_FAIL;
    pb[1457] = 0;                          // pin+0x5B1 = 0
    *reinterpret_cast<DWORD*>(pb + 1460) = dwBits;   // pin+0x5B4
    pb[1458] = 1;                          // pin+0x5B2 = 1
    MMDXTrace("StartStreaming bits=%x ready->1\n", (unsigned)dwBits);
    return S_OK;
}

// VA 0x10001840 - IPushSource slot +0x18 - BeginStreaming.
// Latches pin+0x5B3 — from here on every IPushSource method returns E_FAIL
// until the object is destroyed.
HRESULT CPushSourceDIBSq::PushSource_v18_BeginStreaming(void)
{
    BYTE* pb = reinterpret_cast<BYTE*>(m_pPin);

    if (pb[1459] != 0)                     // pin+0x5B3: streaming begun
        return E_FAIL;
    pb[1459] = 1;                          // pin+0x5B3 = 1
    pb[1457] = 0;                          // pin+0x5B1 = 0
    pb[1458] = 0;                          // pin+0x5B2 = 0
    return S_OK;
}

// VA 0x10001880 - IPushSource slot +0x1C - GetRate.
// Binary ABI: __stdcall(this, float* pRate), retn 8; writes exactly 4 bytes
// (fstp dword) from .rdata 0x10008308 = 1.02f — the EXE version handshake
// (requires >= 1.01f).
HRESULT CPushSourceDIBSq::PushSource_v1C_GetRate(float* pRate)
{
    BYTE* pb = reinterpret_cast<BYTE*>(m_pPin);

    if (pb[1459] != 0)                     // pin+0x5B3: streaming begun
        return E_FAIL;
    *pRate = 1.02f;
    return S_OK;
}

// =============================================================================
// IAMovieSetup::Register / Unregister (0x10003A10 / 0x10003A80).
// Declared at CBaseFilter level (shared by the CSource @0x100084C8 and
// CPushSourceDIBSq @0x10008290 vtables).  In the binary `this` arrives as the
// IAMovieSetup sub-object (obj+0x10) and is rebased with -0x10 before the
// primary-vtable dispatch; the MSVC adjustor thunk does that rebase for us.
// =============================================================================

// Primary-vtable slot +0x20 dispatch (binary 0x10003a14: mov eax,[ecx-10h];
// mov edx,[eax+20h]) — the "GetSetupData" slot.  In this DLL every class
// resolves it to 0x100011D0, which just returns 0 (NULL), so BOTH methods
// below bail out with S_FALSE before touching any registry; real registration
// runs exclusively through DllRegisterServer -> AMovieDllRegisterServer2
// (IFilterMapper2 / IFilterMapper dual path, dll_main.cpp).
static const MMDXSHOW_FILTER_SETUP* CallGetSetupData(CBaseFilter* pFilter)
{
    void** vtbl = *reinterpret_cast<void***>(pFilter);      // primary vtable
    typedef void* (__thiscall *Fn_t)(void*);
    return (const MMDXSHOW_FILTER_SETUP*)(((Fn_t)vtbl[8])(pFilter));
}

// VA 0x10003A10 / 0x10003A80 - IAMovieSetup::Register / Unregister are
// defined in source_base.cpp (CBaseFilter owns the slots per the header);
// both early-out with S_FALSE here because the GetSetupData slot (primary
// +0x20 -> 0x100011D0) always returns NULL in this DLL.

// =============================================================================
// Construction / destruction
// =============================================================================

// VA 0x10001A70 - CPushSourceDIBSq constructor.
// Binary body: CSource base ctor 0x10001E90 with pName = NULL and the filter
// CLSID (passed by value there; the header takes it by pointer), four vtable
// stores (implicit in C++), then operator new(0x5B8) + CPushPinDIBSq ctor
// 0x10001960(pin, phr, this), the pin pointer parked at +0x74, and
// *phr = pin ? S_OK : E_OUTOFMEMORY.
CPushSourceDIBSq::CPushSourceDIBSq(LPUNKNOWN pUnkOuter, HRESULT* phr)
    : CSource(NULL, pUnkOuter, &MMDXSHOW_CLSID_PushSourceDIBSq)
{
    // CPushPinDIBSq's ctor is defined by push_pin.cpp against the same
    // header declaration (HRESULT* phr, CSource* pParent) — binary order.
    void* pMem = operator new(0x5B8, std::nothrow);   // VC8 new: NULL on OOM
    CPushPinDIBSq* pPin = (CPushPinDIBSq*)pMem;
    if (pPin != NULL)
        pPin = new (pPin) CPushPinDIBSq(phr, this);
    m_pPin = pPin;
    if (phr != NULL)
        *phr = (pPin != NULL) ? S_OK : E_OUTOFMEMORY;   // 0x8007000E
}

// VA 0x10001B50 - CPushSourceDIBSq scalar deleting destructor (primary vtable
// slot +0x0C).  Body 0x10001670: re-set the four vtables (implicit here),
// force the pin's stop flags, then destroy the pin through its CAMThread-root
// vtable slot +0x04 invoked as (pin, 1) — the pin's per-class scalar deleting
// dtor 0x10001A50 (dtor core 0x100010D0).  The CSource/CBaseFilter dtor
// chain (0x10001F20) runs implicitly after this body.
CPushSourceDIBSq::~CPushSourceDIBSq()
{
    CPushPinDIBSq* pPin = m_pPin;
    BYTE* pb = reinterpret_cast<BYTE*>(pPin);

    pb[1459] = 1;                    // pin+0x5B3 — UNCHECKED in the binary:
    pb[1458] = 1;                    // pin+0x5B2   a failed pin allocation in
                                     // the ctor makes the dtor fault right here.
    if (pPin != NULL) {
        void** vtbl = *reinterpret_cast<void***>(pPin);   // CAMThread root vtable
        ((void (__thiscall *)(void*, unsigned))(vtbl[1]))(pPin, 1);
    }
}

// VA 0x10001B70 - MMDxShow_NewPushSourceDIBSq (g_Templates[0].m_lpfnNew).
// Binary: operator new(0x78) — the VC8 CRT new in this DLL returns NULL on
// exhaustion instead of throwing — then the ctor, then
// *phr = result ? S_OK : E_OUTOFMEMORY.
CUnknown* MMDxShow_NewPushSourceDIBSq(LPUNKNOWN pUnkOuter, HRESULT* phr)
{
    CPushSourceDIBSq* pObject =
        (CPushSourceDIBSq*)operator new(0x78, std::nothrow);
    if (pObject != NULL)
        pObject = new (pObject) CPushSourceDIBSq(pUnkOuter, phr);
    if (phr != NULL)
        *phr = (pObject != NULL) ? S_OK : E_OUTOFMEMORY;
    return pObject;
}
