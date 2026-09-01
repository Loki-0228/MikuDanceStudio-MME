// =============================================================================
// MMDxShow.dll — Phase B: baseclass layer (source_base.cpp)
// =============================================================================
// Faithful ports of the CUnknown / CBaseFilter / CSource / CAMThread /
// CBasePin / CBaseOutputPin / CSourceStream virtuals and their support
// functions, as declared by src/mmdxshow/mmdxshow.hpp.
//
// Ground truth: Hex-Rays decompilation of the original DLL (IDA session on
// MikuMikuDanceE_v932/Data/MMDxShow.dll, image base 0x10000000); the corpus
// under translated/MMDxShow/*.cpp was used as a cross-check only.
//
// KNOWN DEVIATIONS (all forced by the frozen header; none observable in this
// DLL's single-filter / single-pin reality — see the Phase-B report):
//   * Pin slots the header leaves unnamed are reached through byte offsets:
//       pin +0x14 m_pName(WCHAR*), +0x20 m_pLock, +0x24 byte flags,
//       +0x2C m_pNotify, +0x34 m_mt (0x48 bytes), +0x80 m_tStart,
//       +0x88 m_tStop, +0x90 m_dRate.
//     The header's pin member "m_pName" is actually m_dir (+0x1C,
//     PIN_DIRECTION) and "m_pQSinkOwner" is the owner CBaseFilter* (+0x28).
//   * Filter slots reached through byte offsets: +0x28 m_clsid (16 bytes),
//       +0x38 m_pLock (always &m_CritSec), +0x40 m_pGraph, +0x44 m_pEventSink
//       (IMediaEventSink*, QI'd in JoinFilterGraph).
//   * Pin_v24 (0x10002E90) and Pin_v28 (0x100030F0) take ONE argument in the
//     binary (the media type / the peer pin); the header declares them with
//     ZERO parameters.  Their bodies are inlined at the call sites
//     (AgreeMediaType / ReceiveConnection) and placeholder 0-arg definitions
//     are provided for vtable emission.  HEADER ARBITRATION: give them
//     (const AM_MEDIA_TYPE*) / (IPin*) parameters.
//   * The original CBasePin ctor receives (pUnkOuter, pFilter, pLock,
//     pName(LPCWSTR), dir); the header's is (const char*, CBaseFilter*,
//     HRESULT*).  m_dir is hard-coded to PINDIR_OUTPUT (the only pins in this
//     DLL are output pins; the original's CBaseOutputPin layer passed 1) and
//     the wide name (L"Out") is duplicated by the CSourceStream ctor.
//   * The original CBaseFilter ctor receives the filter CLSID by value; the
//     header's CSource ctor has no CLSID parameter, so the +0x28 slot is
//     filled with MMDXSHOW_CLSID_PushSourceDIBSq (the DLL's only filter).
//   * Run() forwards tStart to the pin's primary-vtable +0x1C entry, which the
//     header names CompleteConnect(IPin*, pmt) (its default body 0x10003040
//     ignores the args) — the two 32-bit halves are passed via pointer casts.
//   * IAMovieSetup::Register/Unregister call the IFilterMapper(v1) helper
//     0x10003400; that body lives as a static inside dll_main.cpp, so a
//     faithful copy is duplicated below.
// =============================================================================

#include "mmdxshow.hpp"
#include <new>
#include <stdlib.h>   // _wtoi

// =============================================================================
// Undeclared-slot access + extern GUID re-declarations
// =============================================================================

// GUIDs instantiated in dll_main.cpp (DEFINE_GUID = extern "C" linkage).
extern "C" {
extern const GUID MMDXSHOW_GUID_NULL;            // .rdata 0x100089EC
extern const GUID MMDXSHOW_CLSID_FilterMapper;   // .rdata 0x10008680
extern const GUID MMDXSHOW_IID_IFilterMapper;    // .rdata 0x100085C0
}

// .rdata 0x10008660 {1E651CC0-B199-11D0-8212-00C04FC32C45}: the DirectShow
// memory-allocator CLSID used by Pin_v48; and .rdata 0x100085F0 = IID_IMemAllocator.
static const GUID MMDXSHOW_CLSID_MemoryAllocator =
    {0x1e651cc0, 0xb199, 0x11d0, {0x82, 0x12, 0x0, 0xc0, 0x4f, 0xc3, 0x2c, 0x45}};
static const GUID MMDXSHOW_IID_IMemAllocator =
    {0x56a8689c, 0xad4, 0x11ce, {0xb0, 0x3a, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70}};
// .rdata 0x100085E0 = IID_IMemInputPin (peer QI in Pin_v28)
static const GUID MMDXSHOW_IID_IMemInputPin =
    {0x56a8689d, 0xad4, 0x11ce, {0xb0, 0x3a, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70}};
// .rdata 0x10008590 {56A868A2-0AD4-11CE-B03A-0020AF0BA770} (JoinFilterGraph QI)
static const GUID MMDXSHOW_IID_56A868A2 =
    {0x56a868a2, 0xad4, 0x11ce, {0xb0, 0x3a, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70}};

// Byte-offset slot access for the members the header leaves unnamed/misnamed
// (and for the protected cross-class reads the original performs directly).
static inline char* Slot(void* p, intptr_t off) { return (char*)p + off; }
static inline char* Slot(const void* p, intptr_t off) { return (char*)p + off; }

// --- CBaseFilter slots (relative to the primary subobject) ---
static inline CLSID&          F_clsid(CBaseFilter* f)  { return *(CLSID*)Slot(f, 0x28); }
static inline CRITICAL_SECTION*& F_pLock(CBaseFilter* f){ return *(CRITICAL_SECTION**)Slot(f, 0x38); }
static inline FILTER_STATE&   F_state(CBaseFilter* f)  { return *(FILTER_STATE*)Slot(f, 0x14); }
static inline IFilterGraph*&  F_pGraph(CBaseFilter* f) { return *(IFilterGraph**)Slot(f, 0x40); }
static inline IUnknown*&      F_pEventSink(CBaseFilter* f) { return *(IUnknown**)Slot(f, 0x44); }

// --- CBasePin slots (relative to the CBasePin primary subobject) ---
static inline WCHAR*&             P_pNameW(CBasePin* p)   { return *(WCHAR**)Slot(p, 0x14); }
static inline PIN_DIRECTION&      P_dir(CBasePin* p)      { return *(PIN_DIRECTION*)Slot(p, 0x1C); }
static inline CRITICAL_SECTION*&  P_pLock(CBasePin* p)    { return *(CRITICAL_SECTION**)Slot(p, 0x20); }
static inline unsigned char&      P_flag24(CBasePin* p)   { return *(unsigned char*)Slot(p, 0x24); }
static inline unsigned char&      P_flag25(CBasePin* p)   { return *(unsigned char*)Slot(p, 0x25); }
static inline unsigned char&      P_flag26(CBasePin* p)   { return *(unsigned char*)Slot(p, 0x26); }
static inline CBaseFilter*&       P_pOwner(CBasePin* p)   { return *(CBaseFilter**)Slot(p, 0x28); }
static inline IPin*&              P_connected(CBasePin* p){ return *(IPin**)Slot(p, 0x18); }
static inline IMemAllocator*&     P_alloc(CBasePin* p)    { return *(IMemAllocator**)Slot(p, 0x98); }
static inline IMemInputPin*&      P_input(CBasePin* p)    { return *(IMemInputPin**)Slot(p, 0x9C); }
static inline void*&              P_pNotify(CBasePin* p)  { return *(void**)Slot(p, 0x2C); }
static inline AM_MEDIA_TYPE&      P_mt(CBasePin* p)       { return *(AM_MEDIA_TYPE*)Slot(p, 0x34); }
static inline REFERENCE_TIME&     P_tStart(CBasePin* p)   { return *(REFERENCE_TIME*)Slot(p, 0x80); }
static inline REFERENCE_TIME&     P_tStop(CBasePin* p)    { return *(REFERENCE_TIME*)Slot(p, 0x88); }
static inline double&             P_dRate(CBasePin* p)    { return *(double*)Slot(p, 0x90); }

// The CAMThread root of a CSourceStream pin, reached the way the original
// does it (CBasePin subobject - 0x48 == object base == CAMThread subobject).
static inline CAMThread* PinToThread(CBasePin* p)
{
    return reinterpret_cast<CAMThread*>(Slot(p, -0x48));
}

// =============================================================================
// Small support helpers (original VAs in banners)
// =============================================================================

// VA 0x10005C90 - lstrlenW equivalent (byte-exact loop)
static int MMDxShow_wcslen(LPCWSTR p)
{
    int n = -1;
    do { ++n; } while (p[n]);
    return n;
}

// VA 0x10005C50 - bounded wide-string copy: copies at most a3-1 wchars and
// always NUL-terminates; stops at the source NUL.
static WCHAR* MMDxShow_wcsncpyClamp(WCHAR* dst, const WCHAR* src, int cchMax)
{
    if (cchMax == 0)
        return dst;
    for (int i = 1; i < cchMax; ++i) {
        WCHAR c = *src;
        *dst++ = *src++;
        if (c == 0)
            return dst;                     // early-out on embedded NUL
    }
    *dst = 0;                               // clamp termination
    return dst;
}

// VA 0x10005CB0 - "%d" -> wide (wsprintfA + MultiByteToWideChar, 32 wchars)
static int MMDxShow_IntToWide(int v, LPWSTR dst)
{
    CHAR buf[32];
    wsprintfA(buf, "%d", v);
    return MultiByteToWideChar(0, 0, buf, -1, dst, 32);
}

// VA 0x10002A70 - CBaseFilter::NotifyEvent: forward an event code through the
// graph's IMediaEventSink (slot +0x44).  EC-complete-style codes (==1) pass
// the filter's IBaseFilter subobject as p2.
static HRESULT MMDxShow_NotifyEvent(CBaseFilter* pFilter, LONG lCode, LONG_PTR lParam1, LONG_PTR lParam2)
{
    IUnknown* pSink = F_pEventSink(pFilter);
    if (pSink == NULL)
        return E_POINTER;
    LONG_PTR p2 = (lCode == 1)
        ? (LONG_PTR)static_cast<IBaseFilter*>(pFilter)
        : lParam2;
    // IMediaEventSink::Notify = vtable slot +0x0C
    return ((HRESULT (__stdcall *)(IUnknown*, LONG, LONG_PTR, LONG_PTR))
            (*(void***)pSink)[3])(pSink, lCode, lParam1, p2);
}

// =============================================================================
// CMediaType comparison helpers (C ABI, AM_MEDIA_TYPE*)
//   Used by CheckMediaType (0x100021E0 / 0x10001430) and the connection
//   machinery (0x10004080 -> 0x10003F90).  AM_MEDIA_TYPE offsets used below
//   (validated against the binary): +0x00 majortype, +0x10 subtype,
//   +0x2C formattype, +0x40 cbFormat, +0x44 pbFormat (GUID_NULL at 0x100089EC).
// =============================================================================

// VA 0x10004DD0 - partial equality: majortype, subtype, formattype, then
// cbFormat bytes of pbFormat.  Returns TRUE when all equal.
static BOOL MMDxShow_MediaTypePartialEqual(const AM_MEDIA_TYPE* pmt1,
                                           const AM_MEDIA_TYPE* pmt2)
{
    if (!MMDxShow_IsEqualGUID16(&pmt1->majortype, &pmt2->majortype))
        return FALSE;
    if (!MMDxShow_IsEqualGUID16(&pmt1->subtype, &pmt2->subtype))
        return FALSE;
    if (!MMDxShow_IsEqualGUID16(&pmt1->formattype, &pmt2->formattype))
        return FALSE;
    if (pmt1->cbFormat != pmt2->cbFormat)
        return FALSE;
    if (pmt1->cbFormat == 0)
        return TRUE;
    // the original is an unrolled memcmp over the format block
    return memcmp(pmt1->pbFormat, pmt2->pbFormat, pmt1->cbFormat) == 0;
}

// VA 0x10004BA0 - TRUE when the media type's majortype OR formattype is the
// GUID_NULL wildcard (0x100089EC) — i.e. the type cannot be matched exactly.
static BOOL MMDxShow_MediaTypeIsWildcard(const AM_MEDIA_TYPE* pmt)
{
    if (MMDxShow_IsEqualGUID16(&pmt->majortype, &MMDXSHOW_GUID_NULL))
        return TRUE;
    if (MMDxShow_IsEqualGUID16(&pmt->formattype, &MMDXSHOW_GUID_NULL))
        return TRUE;
    return FALSE;
}

// VA 0x10004BD0 - partial match with GUID_NULL wildcards: pmtMatch's
// majortype must be GUID_NULL or equal; its subtype likewise; a GUID_NULL
// formattype matches anything, otherwise formattype must be equal and the
// format blocks byte-identical.
static BOOL MMDxShow_MediaTypePartialMatch(const AM_MEDIA_TYPE* pmt,
                                           const AM_MEDIA_TYPE* pmtMatch)
{
    if (MMDxShow_IsEqualGUID16(&pmtMatch->majortype, &MMDXSHOW_GUID_NULL) ||
        MMDxShow_IsEqualGUID16(&pmt->majortype, &pmtMatch->majortype))
    {
        if (!MMDxShow_IsEqualGUID16(&pmtMatch->subtype, &MMDXSHOW_GUID_NULL) &&
            !MMDxShow_IsEqualGUID16(&pmt->subtype, &pmtMatch->subtype))
            return FALSE;
        if (MMDxShow_IsEqualGUID16(&pmtMatch->formattype, &MMDXSHOW_GUID_NULL))
            return TRUE;
        if (!MMDxShow_IsEqualGUID16(&pmt->formattype, &pmtMatch->formattype))
            return FALSE;
        if (pmt->cbFormat != pmtMatch->cbFormat)
            return FALSE;
        if (pmt->cbFormat == 0)
            return TRUE;
        return memcmp(pmt->pbFormat, pmtMatch->pbFormat, pmt->cbFormat) == 0;
    }
    return FALSE;              // majortype mismatch and not a wildcard
}

// VA 0x10004F50 - free a heap (CoTaskMemAlloc'd) AM_MEDIA_TYPE: free its
// innards first, then the struct itself.  NULL is a no-op.
static void MMDxShow_DeleteMediaType(AM_MEDIA_TYPE* pmt)
{
    if (pmt != NULL) {
        MMDxShow_FreeMediaType(pmt);      // 0x10004D70
        CoTaskMemFree(pmt);
    }
}

// =============================================================================
// CUnknown — delegating (outer) IUnknown for the interface branches
//   0x10001180 (QI), 0x10001710 (AddRef), 0x100011A0 (Release) are the
//   IBaseFilter/IPin sub-vtable entries; 0x10001900/0x10001920/0x10002740 are
//   the IAMovieSetup/IQualityControl (+0x10 subobject) wrappers of the same
//   thunks.  MSVC regenerates all of them from these definitions.
// =============================================================================

// 0x10001180 family
STDMETHODIMP CBaseFilter::QueryInterface(REFIID riid, void** ppv)
{
    return m_pOuterUnknown->QueryInterface(riid, ppv);
}

// 0x10001710 family
STDMETHODIMP_(ULONG) CBaseFilter::AddRef()
{
    return m_pOuterUnknown->AddRef();
}

// 0x100011A0 family
STDMETHODIMP_(ULONG) CBaseFilter::Release()
{
    return m_pOuterUnknown->Release();
}

STDMETHODIMP CBasePin::QueryInterface(REFIID riid, void** ppv)
{
    return m_pOuterUnknown->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CBasePin::AddRef()
{
    return m_pOuterUnknown->AddRef();
}

STDMETHODIMP_(ULONG) CBasePin::Release()
{
    return m_pOuterUnknown->Release();
}

// =============================================================================
// CUnknown — NonDelegating trio (shared interlocked stubs 0x10005080/0x100050A0)
// =============================================================================

// 0x10005080: InterlockedIncrement then the min-1 clamp.
STDMETHODIMP_(ULONG) CUnknown::NonDelegatingAddRef()
{
    InterlockedIncrement(&m_cRef);
    const ULONG c = static_cast<ULONG>(m_cRef);
    return c <= 1 ? 1 : c;                                  // 0x10005095
}

// 0x100050A0: on zero, re-increment to 1 (0x100050B4) and invoke the scalar
// deleting dtor (primary vtable +0x0C) with flag 1; else the min-1 clamp.
STDMETHODIMP_(ULONG) CUnknown::NonDelegatingRelease()
{
    if (InterlockedDecrement(&m_cRef) == 0) {
        ++m_cRef;                                           // 0x100050B4
        delete this;                                        // 0x100050C0
        return 0;
    }
    const ULONG c = static_cast<ULONG>(m_cRef);
    return c <= 1 ? 1 : c;
}

// Base-level QI default: every concrete class overrides this (CBaseFilter
// 0x10002790, CBasePin 0x10002DC0); the shared tail (0x10005030 shape) is
// IUnknown-or-E_NOINTERFACE.
STDMETHODIMP CUnknown::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    static const GUID kIID_IUnknown_ =
        {0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
    if (MMDxShow_IsEqualGUID16(&riid, &kIID_IUnknown_))
        return MMDxShow_ReturnSelf(static_cast<INonDelegatingUnknown*>(this), ppv);
    return E_NOINTERFACE;
}

// ~CUnknown — the base destructor is a no-op in the original (inlined into
// the derived destructor cores: 0x10001F20 -> 0x10003600 filter side,
// 0x10001DC0 -> 0x10002D50/0x10005D50 pin side, both of which are ported as
// ~CSource / ~CSourceStream below).
CUnknown::~CUnknown()
{
}

// =============================================================================
// CAMThread — construction / teardown / thread API
// =============================================================================

// VA 0x10005D10 - CAMThread ctor core: manual-reset send event at +0x04,
// auto-reset reply event at +0x08, two critical sections at +0x18/+0x30,
// no thread handle yet (m_uParam/m_uReply stay uninitialized, faithful).
CAMThread::CAMThread()
{
    m_hEventSend  = CreateEventA(NULL, TRUE,  FALSE, NULL);   // 0x10005BE0(1)
    m_hEventReply = CreateEventA(NULL, FALSE, FALSE, NULL);   // 0x10005BE0(0)
    InitializeCriticalSection(&m_CritSecSend);
    InitializeCriticalSection(&m_CritSecReply);
    m_hThread = NULL;                                         // 0x10005d3b
}

// VA 0x10005D50 - CAMThread dtor core (runs implicitly after ~CSourceStream):
// stop-and-close the worker thread, delete both critical sections and both
// events.
CAMThread::~CAMThread()
{
    HANDLE hThread = (HANDLE)InterlockedExchange((volatile LONG*)&m_hThread, 0);
    if (hThread) {
        WaitForSingleObject(hThread, 0xFFFFFFFF);
        CloseHandle(hThread);
    }
    DeleteCriticalSection(&m_CritSecReply);                   // 0x10005d80 (+0x30)
    DeleteCriticalSection(&m_CritSecSend);                    // 0x10005d86 (+0x18)
    CloseHandle(m_hEventReply);                               // CAMEvent dtor (+0x08)
    CloseHandle(m_hEventSend);                                // CAMEvent dtor (+0x04)
}

// VA 0x10005C10 - CoInitializeEx(NULL, APARTMENTTHREADED=0/COINIT=4) via
// GetModuleHandleA/GetProcAddress.  The original pushes the literal 4.
static HRESULT MMDxShow_ThreadCoInit(void)
{
    HMODULE h = GetModuleHandleA("ole32.dll");
    if (h != NULL) {
        typedef HRESULT (__stdcall *PFN_CoInitEx)(LPVOID, DWORD);
        PFN_CoInitEx pfn = (PFN_CoInitEx)GetProcAddress(h, "CoInitializeEx");
        if (pfn != NULL)
            return pfn(NULL, 4);
    }
    return E_FAIL;                                            // 0x80004005
}

// VA 0x10005DA0 - CreateThread start address: CoInitializeEx, run the root
// vtable +0x00 entry (ThreadProc), CoUninitialize on success.
static DWORD WINAPI MMDxShow_InitialThreadProc(LPVOID lpParameter)
{
    const HRESULT hrCo = MMDxShow_ThreadCoInit();             // 0x10005C10
    const DWORD dw = reinterpret_cast<CAMThread*>(lpParameter)->ThreadProc();
    if (hrCo >= 0)
        CoUninitialize();
    return dw;
}

// VA 0x10005DD0 - CAMThread::CreateThread (raw root pointer, exactly how
// CheckConnect reaches it: pin primary - 0x48).  Returns 1 when the thread
// was created, 0 when it already existed or creation failed.
static BOOL MMDxShow_ThreadCreate(CAMThread* pRoot)
{
    CRITICAL_SECTION* pcs = (CRITICAL_SECTION*)Slot(pRoot, 0x18);
    EnterCriticalSection(pcs);
    HANDLE* phThread = (HANDLE*)Slot(pRoot, 0x14);
    if (*phThread != NULL) {
        LeaveCriticalSection(pcs);
        return 0;                                             // already running
    }
    DWORD dwTid;
    HANDLE h = CreateThread(NULL, 0, MMDxShow_InitialThreadProc, pRoot, 0, &dwTid);
    *phThread = h;
    LeaveCriticalSection(pcs);
    return h != NULL ? 1 : 0;
}

// VA 0x10005E20 - CAMThread::CallWorker: post cmd, wait for the reply.
// E_FAIL when no thread is running (m_hThread == NULL).
static DWORD MMDxShow_ThreadCallWorker(CAMThread* pRoot, DWORD dwCmd)
{
    CRITICAL_SECTION* pcs = (CRITICAL_SECTION*)Slot(pRoot, 0x18);
    EnterCriticalSection(pcs);
    if (*(HANDLE*)Slot(pRoot, 0x14) == NULL) {
        LeaveCriticalSection(pcs);
        return (DWORD)E_FAIL;                                 // 0x80004005
    }
    *(DWORD*)Slot(pRoot, 0x0C) = dwCmd;                       // m_uParam
    SetEvent(*(HANDLE*)Slot(pRoot, 0x04));                    // m_hEventSend
    WaitForSingleObject(*(HANDLE*)Slot(pRoot, 0x08), 0xFFFFFFFF); // m_hEventReply
    const DWORD dwReply = *(DWORD*)Slot(pRoot, 0x10);         // m_uReply
    LeaveCriticalSection(pcs);
    return dwReply;
}

// VA 0x10005E80 - CAMThread::GetRequest (blocking): wait for the send event,
// return the parameter.  (Member form: used by ThreadProc below.)
static DWORD MMDxShow_ThreadGetRequest(CAMThread* pRoot)
{
    WaitForSingleObject(*(HANDLE*)Slot(pRoot, 0x04), 0xFFFFFFFF);
    return *(DWORD*)Slot(pRoot, 0x0C);
}

// VA 0x10005EA0 - CAMThread::CheckRequest: TRUE when a request is pending.
static BOOL MMDxShow_ThreadCheckRequest(CAMThread* pRoot, DWORD* pParam)
{
    if (WaitForSingleObject(*(HANDLE*)Slot(pRoot, 0x04), 0) != WAIT_OBJECT_0)
        return 0;
    if (pParam != NULL)
        *pParam = *(DWORD*)Slot(pRoot, 0x0C);
    return 1;
}

// VA 0x10005ED0 - CAMThread::Reply: store the reply, re-arm the send event,
// signal the reply event.
static BOOL MMDxShow_ThreadReply(CAMThread* pRoot, DWORD dw)
{
    const HANDLE hSend = *(HANDLE*)Slot(pRoot, 0x04);
    *(DWORD*)Slot(pRoot, 0x10) = dw;                          // m_uReply
    ResetEvent(hSend);
    return SetEvent(*(HANDLE*)Slot(pRoot, 0x08));             // m_hEventReply
}

// VA 0x10002570 - CAMThread::ThreadProc / CSourceStream::ThreadProc (the
// shared command pump in BOTH root vtables, +0x00): drain stale requests,
// then serve RUN(1)/PAUSE(2) via the +0x18 loop entry, STOP(3)/KILL(4), and
// anything else with E_NOINTERFACE, until KILL.
DWORD CAMThread::ThreadProc()
{
    // 0x10002576..0x10002586: consume pending non-zero params
    while (MMDxShow_ThreadGetRequest(this) != 0)
        MMDxShow_ThreadReply(this, (DWORD)E_NOTIMPL);         // 0x8000FFFF

    const int v2 = (int)CamThread_v0C();                      // slot +0x0C
    if (v2 >= 0) {
        MMDxShow_ThreadReply(this, 0);
        DWORD cmd;
        do {
            cmd = MMDxShow_ThreadGetRequest(this);            // 0x10005E80
            switch (cmd) {
            case 1:                                           // RUN
            case 2:                                           // PAUSE
                MMDxShow_ThreadReply(this, 0);
                CamThread_v18();                              // slot +0x18
                break;
            case 3:                                           // STOP
            case 4:                                           // KILL
                MMDxShow_ThreadReply(this, 0);
                break;
            default:
                MMDxShow_ThreadReply(this, (DWORD)E_NOINTERFACE); // 0x80004001
                break;
            }
        } while (cmd != 4);
        return (DWORD)(CamThread_v10() < 0);                  // slot +0x10
    }
    CamThread_v10();                                          // slot +0x10
    MMDxShow_ThreadReply(this, (DWORD)v2);
    return 1;
}

// CSourceStream re-exports the shared pump in its own root-vtable +0x00 slot;
// the original points both slots at the single 0x10002570 body.
DWORD CSourceStream::ThreadProc()
{
    return CAMThread::ThreadProc();                           // 0x10002570 (shared)
}

// VA 0x100011D0 - three-byte shared stub (`xor eax,eax; ret`) used for the
// CAMThread root-vtable +0x0C/+0x10/+0x14 entries of BOTH CSourceStream
// (0x100084A4) and CPushPinDIBSq (0x10008244), and for CBaseFilter's primary
// +0x20 entry (Filter_v20).  It returns plain 0 (S_OK) — NOT E_NOINTERFACE
// (crt_ledger.cpp's label for 0x100011D0 is wrong).

DWORD CAMThread::CamThread_v0C()   // root +0x0C
{
    return 0;
}

DWORD CAMThread::CamThread_v10()   // root +0x10
{
    return 0;
}

DWORD CAMThread::CamThread_v14()   // root +0x14
{
    return 0;
}

// VA 0x10002630 - CAMThread root +0x18: the buffer-processing loop
// (DoBufferProcessingLoop).  Calls the sibling pin (object+0x48) through its
// primary vtable +0x40 (GetDeliveryBuffer), +0x08 of this root (FillBuffer),
// +0x44 (Deliver), +0x4C (post-error stop) — the +0x14 slot entry runs once
// on entry.  On FillBuffer failure the filter is notified (0x10002A70, EC 3).
DWORD CAMThread::CamThread_v18()
{
    MMDXTrace("DoBufferProcessingLoop enter\n");
    CamThread_v14();                                          // slot +0x14 entry call
    CSourceStream* pStream = static_cast<CSourceStream*>(this);
    CBasePin& pin = *static_cast<CBaseOutputPin*>(pStream);
    DWORD cmd;
    for (;;) {
        while (MMDxShow_ThreadCheckRequest(this, &cmd)) {
            if (cmd == 2 || cmd == 1) {
                MMDxShow_ThreadReply(this, 0);
            } else {
                if (cmd == 3)
                    return 1;
                MMDxShow_ThreadReply(this, (DWORD)E_NOTIMPL); // 0x8000FFFF
            }
            if (cmd == 3)
                return 1;
        }
        IMediaSample* pSample;
        for (;;) {
            if (pin.Pin_v40(&pSample, NULL, NULL, 0) >= 0)     // slot +0x40
                break;
            MMDXTrace("GetDeliveryBuffer retry hr pending\n");
            Sleep(1);
            if (MMDxShow_ThreadCheckRequest(this, &cmd))
                goto pending;
        }
        {
            const HRESULT hr = FillBuffer(pSample);           // slot +0x08
            if (hr != 0) {
                pSample->Release();
                pin.Pin_v4C();                                // slot +0x4C
                if (hr == 1)
                    return 0;
                // 0x10002A70 via the filter back-pointer at pin+0xA0
                CBaseFilter* pFilter = *(CBaseFilter**)Slot(&pin, 0xA0);   // pin +0xA0 back-pointer
                MMDxShow_NotifyEvent(pFilter, 3, hr, 0);
                return (DWORD)hr;
            }
            const HRESULT hrDeliver = pin.Pin_v44(pSample);   // slot +0x44
            pSample->Release();
            if (hrDeliver != 0)
                return 0;                                     // 0x100026d8
        }
        continue;
pending:
        (void)0;
    }
}

// =============================================================================
// CBaseFilter — primary-vtable continued
// =============================================================================

// VA 0x100028B0 - StreamTime: current graph-clock time minus m_tStart.
// VFW_E_NO_CLOCK (0x80040213) when no clock is set.
HRESULT CBaseFilter::Filter_v10_StreamTime(void* pRefTime)
{
    if (m_pClock == NULL)
        return (HRESULT)0x80040213;                           // VFW_E_NO_CLOCK
    // IReferenceClock::GetTime = vtable slot +0x0C
    const HRESULT hr = m_pClock->GetTime((REFERENCE_TIME*)pRefTime);
    if (hr < 0)
        return hr;
    REFERENCE_TIME* prt = (REFERENCE_TIME*)pRefTime;
    // 0x100028d7..0x100628df: 32-bit lo/hi subtract with borrow == plain
    // 64-bit wrap-around subtraction
    *prt = (REFERENCE_TIME)((ULONGLONG)*prt - (ULONGLONG)m_tStart);
    return S_OK;
}

// VA 0x10002AB0 - GetPinVersion: plain member fetch.
LONG CBaseFilter::GetPinVersion()
{
    return m_PinVersion;                                      // +0x48
}

// VA 0x100011D0 - Filter_v20 (returns 0; Register/Unregister treat 0 as
// "no setup data" — see those bodies).
int CBaseFilter::Filter_v20()
{
    return 0;
}

// =============================================================================
// CBaseFilter — IBaseFilter / IPersist (sub-vtable at object +0x0C)
// =============================================================================

// VA 0x10002860 - IPersist::GetClassID: copy the 16-byte CLSID stored by the
// ctor at +0x28 (spans the header's m_pGraph + m_pad2C[0..1]).
STDMETHODIMP CBaseFilter::GetClassID(CLSID* pClassID)
{
    if (pClassID == NULL)
        return E_POINTER;
    memcpy(pClassID, Slot(this, 0x28), sizeof(CLSID));        // this[7..10]
    return S_OK;
}

// VA 0x10002890 - GetState: the original ignores dwMillis entirely.
STDMETHODIMP CBaseFilter::GetState(DWORD /*dwMillis*/, FILTER_STATE* pState)
{
    if (pState == NULL)
        return E_POINTER;
    *pState = m_State;                                        // +0x14
    return S_OK;
}

// VA 0x100037A0 - Stop: when running/paused, call each CONNECTED pin's
// primary-vtable +0x18 entry (BreakConnect — which parks the worker thread);
// keep only the first failure.  State -> State_Stopped.
STDMETHODIMP CBaseFilter::Stop()
{
    EnterCriticalSection(F_pLock(this));                      // *(this+0x38)
    HRESULT hr = S_OK;
    if (m_State != State_Stopped) {
        const int cPins = GetPinCount();                      // slot +0x18
        for (int i = 0; i < cPins; ++i) {
            CBasePin* pPin = GetPin(i);                       // slot +0x1C
            if (P_connected(pPin) != NULL) {
                const HRESULT hrPin = pPin->BreakConnect();   // slot +0x18
                if (hrPin < 0 && hr >= 0)
                    hr = hrPin;
            }
        }
    }
    m_State = State_Stopped;                                  // 0x1000382a
    LeaveCriticalSection(F_pLock(this));
    return hr;
}

// VA 0x10003850 - Pause: if already paused, or there are no pins, just set
// State_Paused.  Otherwise call each connected pin's primary-vtable +0x14
// entry (CheckConnect — thread start + allocator commit) and fail out on the
// first error.
STDMETHODIMP CBaseFilter::Pause()
{
    EnterCriticalSection(F_pLock(this));
    if (m_State != State_Stopped) {
        m_State = State_Paused;                               // 0x100038cf
        LeaveCriticalSection(F_pLock(this));
        return S_OK;
    }
    const int cPins = GetPinCount();
    if (cPins <= 0)
        goto set_paused;
    for (int i = 0; i < cPins; ++i) {
        CBasePin* pPin = GetPin(i);
        if (P_connected(pPin) != NULL) {
            const HRESULT hr = pPin->CheckConnect(NULL);      // slot +0x14
            if (hr < 0) {
                LeaveCriticalSection(F_pLock(this));
                return hr;
            }
        }
    }
set_paused:
    m_State = State_Paused;
    LeaveCriticalSection(F_pLock(this));
    return S_OK;
}

// VA 0x10003910 - Run(tStart): store m_tStart; when stopped, Pause() first;
// unless already running, forward tStart to every connected pin's
// primary-vtable +0x1C entry (the 0x10003040 stub ignores it); State ->
// State_Running.
STDMETHODIMP CBaseFilter::Run(REFERENCE_TIME tStart)
{
    EnterCriticalSection(F_pLock(this));
    m_tStart = tStart;                                        // +0x20/+0x24
    if (m_State == State_Stopped) {
        const HRESULT hr = Pause();                           // sub-vtable +0x14
        if (hr < 0) {
            LeaveCriticalSection(F_pLock(this));
            return hr;
        }
    }
    if (m_State != State_Running) {
        const int cPins = GetPinCount();
        for (int i = 0; i < cPins; ++i) {
            CBasePin* pPin = GetPin(i);
            if (P_connected(pPin) != NULL) {
                // slot +0x1C (CompleteConnect in the header) receives the
                // start time as its two pointer-sized arguments
                const HRESULT hr = pPin->CompleteConnect(
                    reinterpret_cast<IPin*>(static_cast<ULONG_PTR>((ULONG)((ULONGLONG)tStart & 0xFFFFFFFFu))),
                    reinterpret_cast<const AM_MEDIA_TYPE*>(static_cast<ULONG_PTR>((ULONG)((ULONGLONG)tStart >> 32))));
                if (hr < 0) {
                    LeaveCriticalSection(F_pLock(this));
                    return hr;
                }
            }
        }
    }
    m_State = State_Running;                                  // 0x100039cf
    LeaveCriticalSection(F_pLock(this));
    return S_OK;
}

// VA 0x10003680 - SetSyncSource: AddRef the new clock, Release the old.
STDMETHODIMP CBaseFilter::SetSyncSource(IReferenceClock* pClock)
{
    EnterCriticalSection(F_pLock(this));
    if (pClock != NULL)
        pClock->AddRef();
    if (m_pClock != NULL)
        m_pClock->Release();
    m_pClock = pClock;                                        // +0x18
    LeaveCriticalSection(F_pLock(this));
    return S_OK;
}

// VA 0x10003700 - GetSyncSource: AddRef'd transfer.
STDMETHODIMP CBaseFilter::GetSyncSource(IReferenceClock** ppClock)
{
    if (ppClock == NULL)
        return E_POINTER;
    EnterCriticalSection(F_pLock(this));
    if (m_pClock != NULL)
        m_pClock->AddRef();
    *ppClock = m_pClock;
    LeaveCriticalSection(F_pLock(this));
    return S_OK;
}

// VA 0x10004A10 - EnumPins: news a 0x30-byte CEnumPins on the primary
// subobject.
STDMETHODIMP CBaseFilter::EnumPins(IEnumPins** ppEnum)
{
    if (ppEnum == NULL)
        return E_POINTER;
    CEnumPins* pEnum = new CEnumPins(this, NULL);             // operator new(0x30)
    *ppEnum = pEnum;
    return pEnum != NULL ? S_OK : E_OUTOFMEMORY;
}

// VA 0x10001CA0 - FindPin: Id is the decimal pin index (1-based); returns the
// +0x0C IPin subobject AddRef'd.  VFW_E_NOT_FOUND (0x80040216) otherwise.
STDMETHODIMP CBaseFilter::FindPin(LPCWSTR Id, IPin** ppPin)
{
    if (ppPin == NULL)
        return E_POINTER;
    CBasePin* pPin = GetPin(_wtoi(Id) - 1);                   // slot +0x1C
    *ppPin = pPin ? static_cast<IPin*>(pPin) : NULL;
    if (pPin == NULL)
        return (HRESULT)0x80040216;                           // VFW_E_NOT_FOUND
    static_cast<IPin*>(pPin)->AddRef();
    return S_OK;
}

// VA 0x100028F0 - QueryFilterInfo: instance name from +0x3C, graph from
// +0x40 (AddRef'd).
STDMETHODIMP CBaseFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
    if (pInfo == NULL)
        return E_POINTER;
    if (m_pName != NULL)
        MMDxShow_wcsncpyClamp(pInfo->achName, m_pName, 128);  // 0x10005C50
    else
        pInfo->achName[0] = 0;
    pInfo->pGraph = F_pGraph(this);                           // +0x40
    if (pInfo->pGraph != NULL)
        pInfo->pGraph->AddRef();
    return S_OK;
}

// VA 0x10002950 - JoinFilterGraph: store the graph (QI it for the
// 0x56A868A2 interface into the +0x44 sink slot and Release immediately —
// a faithful no-op probe), replace the instance name.
STDMETHODIMP CBaseFilter::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
    EnterCriticalSection(F_pLock(this));
    F_pGraph(this) = pGraph;                                  // +0x40
    if (pGraph != NULL) {
        IUnknown* pUnk = NULL;
        if (pGraph->QueryInterface(MMDXSHOW_IID_56A868A2, (void**)&pUnk) >= 0)
            pUnk->Release();                                  // 0x100029b4
    } else {
        F_pEventSink(this) = NULL;                            // +0x44
    }
    if (m_pName != NULL) {                                    // +0x3C
        ::operator delete(m_pName);
        m_pName = NULL;
    }
    if (pName != NULL) {
        const int cch = MMDxShow_wcslen(pName) + 1;           // 0x10005C90
        m_pName = (WCHAR*)operator new(2 * cch);
        if (m_pName != NULL)
            memcpy(m_pName, pName, 2 * cch);
    }
    LeaveCriticalSection(F_pLock(this));
    return S_OK;
}

// VA 0x10002A60 - QueryVendorInfo: plain E_NOTIMPL.
STDMETHODIMP CBaseFilter::QueryVendorInfo(LPWSTR* /*pVendorInfo*/)
{
    return E_NOTIMPL;
}

// =============================================================================
// IAMovieSetup (sub-vtable at object +0x10) — Register/Unregister
//   Both fetch the setup blob through the primary +0x20 entry (Filter_v20,
//   the 0x100011D0 stub returning 0), so both take the early-out and return
//   S_FALSE before touching the mapper — kept bit-faithful, full body ported.
// =============================================================================

// Faithful copy of 0x10003400 (RegisterFilterMapper1) — the original body
// lives as a static inside dll_main.cpp and is not linkable from here.
static HRESULT MMDxShow_RegisterFilterMapper1(const MMDXSHOW_FILTER_SETUP* pSetup,
                                              IFilterMapper* pMapper, BOOL bRegister)
{
    if (pSetup == NULL)
        return S_FALSE;

    const CLSID clsid = *pSetup->clsID;
    HRESULT hr = pMapper->UnregisterFilter(clsid);                    // +0x1C
    if (bRegister) {
        hr = pMapper->RegisterFilter(clsid, pSetup->szName, pSetup->dwMerit); // +0x0C
        if (SUCCEEDED(hr)) {
            for (UINT p = 0; p < pSetup->nPins; ++p) {
                const MMDXSHOW_PIN_SETUP* pPin = &pSetup->lpPin[p];
                hr = pMapper->RegisterPin(clsid, pPin->strName,       // +0x14
                                          pPin->bRendered, pPin->bOutput,
                                          pPin->bZero, pPin->bMany,
                                          *pPin->clsConnectsToFilter,
                                          pPin->strConnectsToPin);
                if (FAILED(hr))
                    break;
                for (UINT t = 0; t < pPin->nMediaTypes; ++t) {
                    const MMDXSHOW_MEDIATYPE_SETUP* pMt = &pPin->lpMediaType[t];
                    hr = pMapper->RegisterPinType(clsid, pPin->strName, // +0x18
                                                  *pMt->clsMajorType,
                                                  *pMt->clsSubType);
                    if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                        return hr;
                }
            }
        }
    }
    return (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) ? S_OK : hr;
}

// VA 0x10003A10 - Register (v1 IFilterMapper path; hr of the mapper call is
// discarded, always returns S_OK when it gets that far).
STDMETHODIMP CBaseFilter::Register()
{
    const MMDXSHOW_FILTER_SETUP* pSetup =
        (const MMDXSHOW_FILTER_SETUP*)(INT_PTR)Filter_v20();  // primary +0x20
    if (pSetup == NULL)
        return 1;                                             // S_FALSE (always, here)
    CoInitialize(NULL);
    IFilterMapper* pMapper = NULL;
    if (CoCreateInstance(MMDXSHOW_CLSID_FilterMapper, NULL, CLSCTX_INPROC_SERVER,
                         MMDXSHOW_IID_IFilterMapper, (LPVOID*)&pMapper) >= 0) {
        MMDxShow_RegisterFilterMapper1(pSetup, pMapper, TRUE);        // 0x10003400
        pMapper->Release();
    }
    CoFreeUnusedLibraries();
    CoUninitialize();
    return S_OK;
}

// VA 0x10003A80 - Unregister (maps ERROR_FILE_NOT_FOUND to S_OK).
STDMETHODIMP CBaseFilter::Unregister()
{
    const MMDXSHOW_FILTER_SETUP* pSetup =
        (const MMDXSHOW_FILTER_SETUP*)(INT_PTR)Filter_v20();  // primary +0x20
    if (pSetup == NULL)
        return 1;                                             // S_FALSE (always, here)
    CoInitialize(NULL);
    IFilterMapper* pMapper = NULL;
    HRESULT hr;
    if ((hr = CoCreateInstance(MMDXSHOW_CLSID_FilterMapper, NULL, CLSCTX_INPROC_SERVER,
                               MMDXSHOW_IID_IFilterMapper, (LPVOID*)&pMapper)) >= 0) {
        hr = MMDxShow_RegisterFilterMapper1(pSetup, pMapper, FALSE);  // 0x10003400
        pMapper->Release();
    }
    CoFreeUnusedLibraries();
    CoUninitialize();
    return (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) ? S_OK : hr;
}

// =============================================================================
// CBaseFilter — NonDelegatingQueryInterface (primary +0x00) — VA 0x10002790
//   IBaseFilter (.rdata 0x10008600) / IMediaFilter (0x10008610) / IPersist
//   (0x10008A1C) -> the +0x0C subobject;  IAMovieSetup (0x100085D0) -> +0x10;
//   else the shared IUnknown tail.
// =============================================================================
STDMETHODIMP CBaseFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    static const GUID kIID_IAMovieSetup_ =
        {0xA3D8CEC0, 0x7E5A, 0x11CF, {0xBB, 0xC5, 0x00, 0x80, 0x5F, 0x6C, 0xEF, 0x20}};
    if (MMDxShow_IsEqualGUID16(&riid, &IID_IBaseFilter) ||
        MMDxShow_IsEqualGUID16(&riid, &IID_IMediaFilter) ||
        MMDxShow_IsEqualGUID16(&riid, &IID_IPersist))
        return MMDxShow_ReturnSelf(reinterpret_cast<char*>(this) + 0x0C, ppv);
    if (MMDxShow_IsEqualGUID16(&riid, &kIID_IAMovieSetup_))
        return MMDxShow_ReturnSelf(reinterpret_cast<char*>(this) + 0x10, ppv);
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

// =============================================================================
// CSource — pin array + construction / destruction
//   ctor chain 0x10001A70 -> 0x10001E90 (CSource core) -> 0x100049B0
//   (CBaseFilter core) -> 0x10005000 (CUnknown core: LockModule first).
// =============================================================================

// VA 0x10001E90 (+0x100049B0 +0x10005000) - CSource ctor.
CSource::CSource(const char* pName, LPUNKNOWN pUnkOuter, const CLSID* pClsID)
    : CBaseFilter(pName, pUnkOuter)
{
    MMDxShow_LockModule();                 // 0x10005000 / 0x10004F80 (runs first in the original)
    m_State      = State_Stopped;          // +0x14 (0x100049ca)
    m_pClock     = NULL;                   // +0x18
    m_tStart     = 0;                      // +0x20
    F_clsid(this) = *pClsID;                                  // +0x28 (by value in the original)
    F_pLock(this) = &m_CritSec;            // +0x38 (0x100049f1: a4 = &m_CritSec)
    m_pName      = NULL;                   // +0x3C
    F_pGraph(this) = NULL;                 // +0x40
    F_pEventSink(this) = NULL;             // +0x44
    m_PinVersion = 1;                      // +0x48 (0x100049fd)
    m_cPins      = 0;                      // +0x50 (0x10001ef2)
    m_ppPins     = NULL;                   // +0x54 (0x10001ef5)
    InitializeCriticalSection(&m_CritSec);// +0x58 (0x10001ef8)
}

// VA 0x10001F20 (+0x10003600) - ~CSource core: destroy every pin through the
// root-vtable +0x04 deleting dtor, delete the filter critsec, the instance
// name, release the clock, unlock the module.
CSource::~CSource()
{
    // vftable resets to CSource's are implicit
    while (m_cPins > 0) {                                     // 0x10001f5c
        CSourceStream* pPin = m_ppPins[m_cPins - 1];
        if (pPin != NULL)
            delete pPin;                                      // root vtable +0x04, flag 1
    }
    DeleteCriticalSection(&m_CritSec);                        // 0x10001f91
    // 0x10003600 tail:
    ::operator delete(m_pName);                                 // +0x3C (delete NULL is a no-op)
    if (m_pClock != NULL) {                                   // +0x18
        m_pClock->Release();
        m_pClock = NULL;
    }
    MMDxShow_UnlockModule();                                  // 0x10004FA0
}

// VA 0x10002090 - CSource::GetPinCount: critsec-guarded read of m_cPins.
int CSource::GetPinCount()
{
    EnterCriticalSection(&m_CritSec);                         // +0x58
    const int cPins = m_cPins;                                // +0x50
    LeaveCriticalSection(&m_CritSec);
    return cPins;
}

// VA 0x100020B0 - CSource::GetPin: critsec-guarded; returns the CBasePin
// subobject (m_ppPins[n] + 0x48) or NULL when out of range / empty slot.
CBasePin* CSource::GetPin(int n)
{
    EnterCriticalSection(&m_CritSec);
    if (n >= 0 && n < m_cPins && m_ppPins != NULL && m_ppPins[n] != NULL) {
        CBasePin* pPin = m_ppPins[n];                         // +0x48 adjust applied by C++
        LeaveCriticalSection(&m_CritSec);
        return pPin;
    }
    LeaveCriticalSection(&m_CritSec);
    return NULL;
}

// VA 0x10001FC0 - CSource::AddPin: grow the array by one under the critsec.
void CSource::AddPin(CSourceStream* pPin)
{
    EnterCriticalSection(&m_CritSec);
    CSourceStream** ppNew = static_cast<CSourceStream**>(
        operator new(sizeof(*ppNew) * (m_cPins + 1)));
    if (ppNew != NULL) {
        if (m_ppPins != NULL) {
            memcpy(ppNew, m_ppPins, sizeof(*ppNew) * m_cPins);
            ppNew[m_cPins] = pPin;                            // 0x1000204d
            ::operator delete(m_ppPins);
        }
        ppNew[m_cPins] = pPin;                                // 0x10002062 (stored twice, faithful)
        m_ppPins = ppNew;
        ++m_cPins;
        LeaveCriticalSection(&m_CritSec);
    } else {
        LeaveCriticalSection(&m_CritSec);
        // the original returns E_OUTOFMEMORY through the ctor's phr slot;
        // the header's AddPin returns void.
    }
}

// VA 0x10001C20 - CSource::RemovePin: drop one entry (shift the tail left);
// returns 1 when the pin was not found.
void CSource::RemovePin(CSourceStream* pPin)
{
    if (m_cPins <= 0)
        return;
    int i = 0;
    for (; i < m_cPins; ++i)
        if (m_ppPins[i] == pPin)
            break;
    if (i >= m_cPins)
        return;                                               // not found -> 1
    if (m_cPins == 1) {
        ::operator delete(m_ppPins);
        --m_cPins;
        m_ppPins = NULL;
        return;
    }
    for (int j = i + 1; j < m_cPins; ++j)
        m_ppPins[j - 1] = m_ppPins[j];
    --m_cPins;
}

// =============================================================================
// CBasePin — construction / NonDelegating trio / primary-vtable slots
// =============================================================================

// VA 0x100046B0 - CBasePin ctor core (original signature:
// (pUnkOuter, pFilter, pLock, pName(LPCWSTR), dir); the header's is
// (pName(char*), pFilter, phr) — see the deviations note up top).
CBasePin::CBasePin(const char* /*pName*/, CBaseFilter* pFilter, HRESULT* /*phr*/)
    : CUnknown("", NULL)
{
    MMDxShow_LockModule();                                    // 0x10005000/0x10004F80
    P_pNameW(this)   = NULL;                                  // +0x14 (dup'd by CSourceStream)
    m_Connected      = NULL;                                  // +0x18
    P_dir(this)      = PINDIR_OUTPUT;                         // +0x1C (a7 == 1 for output pins)
    P_pLock(this)    = &pFilter->m_CritSec;                   // +0x20 (a4 = parent+0x58)
    P_flag24(this)   = 0;                                     // +0x24
    P_flag25(this)   = 0;                                     // +0x25 (connect-while-active)
    P_flag26(this)   = 0;                                     // +0x26 (enum-first flag)
    P_pOwner(this)   = pFilter;                               // +0x28 (a3)
    P_pNotify(this)  = NULL;                                  // +0x2C
    m_TypeVersion    = 1;                                     // +0x30 (0x10004716)
    MMDxShow_InitMediaType(&P_mt(this));                      // +0x34 (0x10004DC0)
    P_tStart(this)   = 0;                                     // +0x80
    P_tStop(this)    = (REFERENCE_TIME)((ULONGLONG)0x7FFFFFFF << 32 | 0xFFFFFFFF); // +0x88 (lo=-1, hi=0x7FFFFFFF)
    P_dRate(this)    = 1.0;                                   // +0x90
    m_pAllocator     = NULL;                                  // +0x98 (CBaseOutputPin layer 0x10004997)
    m_pInputPin      = NULL;                                  // +0x9C (0x1000499d)
    // (+0xA0 m_pFilter is stored by the CSourceStream ctor, 0x1000218c)
}

// VA 0x10002DC0 - CBasePin::NonDelegatingQueryInterface:
//   IID_IPin (0x10008640) -> +0x0C subobject;  IID_IQualityControl
//   (0x100085A0) -> +0x10;  else the shared tail.
STDMETHODIMP CBasePin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    if (MMDxShow_IsEqualGUID16(&riid, &IID_IPin))
        return MMDxShow_ReturnSelf(reinterpret_cast<char*>(this) + 0x0C, ppv);
    if (MMDxShow_IsEqualGUID16(&riid, &IID_IQualityControl))
        return MMDxShow_ReturnSelf(reinterpret_cast<char*>(this) + 0x10, ppv);
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

// VA 0x10002E50 / 0x10002E70 - CBasePin NonDelegatingAddRef / Release
// delegate to the owner filter's IBaseFilter (+0x0C) AddRef/Release (which
// forward to the outer unknown).
STDMETHODIMP_(ULONG) CBasePin::NonDelegatingAddRef()
{
    return static_cast<IBaseFilter*>(P_pOwner(this))->AddRef();   // 0x10002E50
}

STDMETHODIMP_(ULONG) CBasePin::NonDelegatingRelease()
{
    return static_cast<IBaseFilter*>(P_pOwner(this))->Release();  // 0x10002E70
}

// VA 0x10003030 - GetMediaTypeVersion: plain member fetch (+0x30).
LONG CBasePin::GetMediaTypeVersion()
{
    return m_TypeVersion;
}

// VA 0x10003040 - CompleteConnect default: 5-byte stub (`xor eax,eax; ret 8`)
// succeeding unconditionally (its Run-time invocation via slot +0x1C relies
// on the arguments being ignored).
HRESULT CBasePin::CompleteConnect(IPin* /*pReceivePin*/, const AM_MEDIA_TYPE* /*pmt*/)
{
    return S_OK;
}

// VA 0x10003330 - CommitAllocator: slot +0x98 allocator -> IMemAllocator
// vtable +0x14 (Commit); VFW_E_NO_ALLOCATOR (0x8004020A) when absent.
static HRESULT MMDxPin_CommitAllocator(CBasePin* pPin)
{
    if (P_alloc(pPin) != NULL)
        return P_alloc(pPin)->Commit();
    return (HRESULT)0x8004020A;                               // VFW_E_NO_ALLOCATOR
}

// VA 0x10003350 - the deactivate half: clear the +0x24 run flag, Decommit the
// allocator (IMemAllocator vtable +0x18); VFW_E_NO_ALLOCATOR when absent.
static HRESULT MMDxPin_DecommitAllocator(CBasePin* pPin)
{
    P_flag24(pPin) = 0;                                       // 0x10003350
    if (P_alloc(pPin) != NULL)
        return P_alloc(pPin)->Decommit();
    return (HRESULT)0x8004020A;                               // VFW_E_NO_ALLOCATOR
}

// VA 0x10002370 - CBasePin::CheckConnect (primary +0x14): the pin ACTIVATION.
// Enters the filter critsec twice (m_pLock at +0x38 points at the same
// recursive CS), bails with S_FALSE when the filter is already
// paused/running, S_OK when not connected; otherwise commits the allocator,
// creates the worker thread (0x10005DD0 on pin-0x48) and pumps commands 0
// then 1 through it (0x10005E20).
HRESULT CBasePin::CheckConnect(IPin* /*pPin*/)
{
    CRITICAL_SECTION* pcsFilter = &m_pFilter->m_CritSec;      // this[40]+88
    EnterCriticalSection(pcsFilter);
    EnterCriticalSection(F_pLock(m_pFilter));                 // filter+0x38 (same CS, recursive)
    const FILTER_STATE state = F_state(m_pFilter);            // filter+0x14
    const BOOL bActive = (state == State_Paused || state == State_Running);
    LeaveCriticalSection(F_pLock(m_pFilter));
    if (bActive) {
        LeaveCriticalSection(pcsFilter);
        return 1;                                             // S_FALSE
    }
    if (m_Connected == NULL) {
        LeaveCriticalSection(pcsFilter);
        return 0;                                             // S_OK
    }
    HRESULT hr = MMDxPin_CommitAllocator(this);               // 0x10003330
    if (hr < 0) {
        LeaveCriticalSection(pcsFilter);
        return hr;
    }
    if (MMDxShow_ThreadCreate(PinToThread(this))) {           // 0x10005DD0(pin-0x48)
        hr = (HRESULT)MMDxShow_ThreadCallWorker(PinToThread(this), 0);
        if (hr < 0) {
            LeaveCriticalSection(pcsFilter);
            return hr;
        }
        const HRESULT hr2 = (HRESULT)MMDxShow_ThreadCallWorker(PinToThread(this), 1);
        LeaveCriticalSection(pcsFilter);
        return hr2;
    }
    LeaveCriticalSection(pcsFilter);
    return E_FAIL;                                            // 0x80004005 (0x10002458)
}

// VA 0x100024B0 - CBasePin::BreakConnect (primary +0x18): the pin
// DEACTIVATION.  Decommit the allocator, park the worker (commands 3 then 4)
// and close its handle (0x10001E60).  S_OK when not connected.
HRESULT CBasePin::BreakConnect()
{
    CRITICAL_SECTION* pcsFilter = &m_pFilter->m_CritSec;      // this[40]+88
    EnterCriticalSection(pcsFilter);
    if (m_Connected != NULL) {
        HRESULT hr = MMDxPin_DecommitAllocator(this);         // 0x10003350
        if (hr < 0) {
            LeaveCriticalSection(pcsFilter);
            return hr;
        }
        CAMThread* pThread = PinToThread(this);
        if (*(HANDLE*)Slot(pThread, 0x14) != NULL) {          // *(pin-0x34) == m_hThread
            hr = (HRESULT)MMDxShow_ThreadCallWorker(pThread, 3);
            if (hr < 0) {
                LeaveCriticalSection(pcsFilter);
                return hr;
            }
            hr = (HRESULT)MMDxShow_ThreadCallWorker(pThread, 4);
            if (hr < 0) {
                LeaveCriticalSection(pcsFilter);
                return hr;
            }
            // 0x10001E60: InterlockedExchange(&m_hThread, 0); wait; close
            HANDLE hThread = (HANDLE)InterlockedExchange(
                (volatile LONG*)Slot(pThread, 0x14), 0);
            if (hThread != NULL) {
                WaitForSingleObject(hThread, 0xFFFFFFFF);
                CloseHandle(hThread);
            }
        }
    }
    LeaveCriticalSection(pcsFilter);
    return 0;                                                 // S_OK
}

// VA 0x100022B0 - CBasePin::GetMediaType(int, pmt) (primary +0x34): locks the
// FILTER critsec; iPosition < 0 -> E_INVALIDARG, iPosition > 0 -> raw
// 0x40103 (VFW_S_NO_MORE_ITEMS), iPosition == 0 -> the single-arg GetMediaType
// through the CAMThread-root vtable +0x1C of (this - 0x48) — the cross-base
// dispatch, preserved exactly via the downcast + virtual call.
HRESULT CBasePin::GetMediaType(int iPosition, AM_MEDIA_TYPE* pmt)
{
    CRITICAL_SECTION* pcsFilter = &m_pFilter->m_CritSec;      // this[40]+88
    EnterCriticalSection(pcsFilter);
    if (iPosition < 0) {
        LeaveCriticalSection(pcsFilter);
        return E_INVALIDARG;                                  // 0x80070057
    }
    if (iPosition > 0) {
        LeaveCriticalSection(pcsFilter);
        return (HRESULT)0x40103;                              // VFW_S_NO_MORE_ITEMS
    }
    // iPosition == 0: (*(rootvtable + 0x1C))(pin - 0x48, pmt)
    CSourceStream* pStream = static_cast<CSourceStream*>(this);
    const HRESULT hr = pStream->GetMediaType1(pmt);           // root vtable +0x1C
    LeaveCriticalSection(pcsFilter);
    return hr;
}

// VA 0x10002E90 (+0x10004F70 thunk -> 0x10004EC0) - Pin_v24 = SetMediaType:
// no-op when pmt is &m_mt, otherwise replace m_mt with a deep copy.
HRESULT CBasePin::Pin_v24(const AM_MEDIA_TYPE* pmt)
{
    if (pmt == &P_mt(this))
        return S_OK;
    MMDxShow_FreeMediaType(&P_mt(this));                      // 0x10004D70
    if (MMDxShow_CopyMediaType(&P_mt(this), pmt) >= 0)        // 0x10004D00
        return S_OK;
    return E_OUTOFMEMORY;
}

// VA 0x100030F0 (+0x10002EB0) - Pin_v28 = initialize the connection with the
// peer: reject same-filter peers (the original compares PIN_INFO.pFilter
// against the +0x1C slot — the DIRECTION member, an apparent original bug
// kept bit-faithful), then QI the peer for IMemInputPin into +0x9C.
HRESULT CBasePin::Pin_v28(IPin* pPeer)
{
    // 0x10002EB0:
    PIN_INFO info;
    pPeer->QueryPinInfo(&info);                               // IPin vtable +0x24
    IBaseFilter* pPeerFilter = info.pFilter;
    // 0x10002eca: compare with the +0x1C slot (m_dir) — kept as-is
    if (reinterpret_cast<IBaseFilter*>((INT_PTR)P_dir(this)) == pPeerFilter)
        return (HRESULT)0x80040208;
    // 0x10003102:
    const HRESULT hr = pPeer->QueryInterface(MMDXSHOW_IID_IMemInputPin,
                                             (void**)&P_input(this));     // +0x9C
    return hr >= 0 ? S_OK : hr;                               // 0x10003117/0x1000311f
}

// VA 0x10003130 - Pin_v2C: connection teardown — Decommit+Release the
// allocator (+0x98), Release+NULL the IMemInputPin (+0x9C).
HRESULT CBasePin::Pin_v2C()
{
    if (m_pAllocator != NULL) {
        const HRESULT hr = m_pAllocator->Decommit();          // vtable +0x18
        if (hr < 0)
            return hr;
        m_pAllocator->Release();
        m_pAllocator = NULL;
    }
    if (m_pInputPin != NULL) {
        m_pInputPin->Release();
        m_pInputPin = NULL;
    }
    return S_OK;
}

// VA 0x100030D0 - Pin_v30 (one ignored argument in the binary, retn 4):
// tail-call Pin_v38(m_pInputPin(+0x9C), &m_pAllocator(+0x98)) — i.e.
// DecideAllocator on the remembered peer.
HRESULT CBasePin::Pin_v30(void* /*a1*/)
{
    return Pin_v38(m_pInputPin, &m_pAllocator);
}

// VA 0x100031A0 - Pin_v38 = DecideAllocator: read the peer's buffer
// requirements (IMemInputPin vtable +0x14, the old ActiveMovie
// GetBufferRequirements), take its allocator (vtable +0x0C), size it via the
// +0x3C slot and hand it back (NotifyAllocator, vtable +0x10); on any
// failure create a fresh allocator through the +0x48 slot
// (CLSID_MemoryAllocator / IID_IMemAllocator, 0x10002770) and retry.
HRESULT CBasePin::Pin_v38(IMemInputPin* pInput, IMemAllocator** ppAlloc)
{
    *ppAlloc = NULL;                                          // *a3 = 0
    ALLOCATOR_PROPERTIES props = { 0, 0, 0, 0 };              // v6/v7/v8 zeroed
    // old ActiveMovie IMemInputPin::GetBufferRequirements = vtable +0x14
    // (dropped from modern SDK headers; raw dispatch kept bit-faithful)
    ((void (__stdcall *)(IMemInputPin*, ALLOCATOR_PROPERTIES*))
        (*(void***)pInput)[5])(pInput, &props);
    // 0x100031dc: v7 (the ALLOCATOR_PROPERTIES dword at +8 == cbAlign)
    // defaults to 1 when the peer reports none.  A cbAlign of 0 handed to
    // the system CMemAllocator::SetProperties is rejected with
    // VFW_E_BADALIGN (0x8004020E) and the whole connection unwinds - the
    // original binaries rely on the 1 default.
    if (props.cbAlign == 0)
        props.cbAlign = 1;
    if (pInput->GetAllocator(ppAlloc) >= 0 &&                 // vtable +12 decimal (0x0C)
        Pin_v3C(*ppAlloc, &props) >= 0 &&                     // slot +0x3C
        pInput->NotifyAllocator(*ppAlloc, 0) >= 0)            // vtable +16 decimal (0x10)
        return S_OK;
    if (*ppAlloc != NULL) {
        (*ppAlloc)->Release();
        *ppAlloc = NULL;
    }
    HRESULT hr = Pin_v48((void**)ppAlloc);                    // slot +0x48 (fresh allocator)
    if (hr >= 0) {
        hr = Pin_v3C(*ppAlloc, &props);                       // slot +0x3C
        if (hr >= 0) {
            hr = pInput->NotifyAllocator(*ppAlloc, 0);        // vtable +0x10
            if (hr >= 0)
                return S_OK;
        }
    }
    if (*ppAlloc != NULL) {
        (*ppAlloc)->Release();
        *ppAlloc = NULL;
    }
    return hr;
}

// VA 0x100032A0 - Pin_v40 = GetDeliveryBuffer: forward to m_pAllocator
// (+0x98) GetBuffer (IMemAllocator vtable +0x1C); E_NOINTERFACE when absent.
HRESULT CBasePin::Pin_v40(IMediaSample** ppSample, REFERENCE_TIME* pStart,
                          REFERENCE_TIME* pEnd, DWORD dwFlags)
{
    if (m_pAllocator != NULL)
        return m_pAllocator->GetBuffer(ppSample, pStart, pEnd, dwFlags);
    return E_NOINTERFACE;                                     // 0x80004002
}

// VA 0x100032E0 - Pin_v44 = Deliver: forward the sample to m_pInputPin
// (+0x9C) Receive (IMemInputPin vtable +0x18); VFW_E_NOT_CONNECTED (0x80040209).
HRESULT CBasePin::Pin_v44(IMediaSample* pSample)
{
    if (m_pInputPin != NULL)
        return m_pInputPin->Receive(pSample);
    return (HRESULT)0x80040209;                               // VFW_E_NOT_CONNECTED
}

// VA 0x10003190 (+0x10002770) - Pin_v48: CoCreateInstance(
// CLSID_MemoryAllocator{1E651CC0-B199-11D0-8212-00C04FC32C45}, NULL,
// CLSCTX_INPROC_SERVER, IID_IMemAllocator, ppObj).
HRESULT CBasePin::Pin_v48(void** ppObj)
{
    return CoCreateInstance(MMDXSHOW_CLSID_MemoryAllocator, NULL,
                            CLSCTX_INPROC_SERVER, MMDXSHOW_IID_IMemAllocator, ppObj);
}

// VA 0x10003310 / 0x10003380 / 0x100033A0 - Pin_v4C / Pin_v50 / Pin_v54:
// forward to the CONNECTED pin's IPin vtable +0x38 / +0x3C / +0x40
// (EndOfStream / BeginFlush / EndFlush in the old ActiveMovie order) or
// fail with VFW_E_NOT_CONNECTED.
static HRESULT MMDxPin_ForwardToConnected(CBasePin* pPin, DWORD vtSlot)
{
    IPin* pConnected = P_connected(pPin);
    if (pConnected != NULL)
        return ((HRESULT (__stdcall *)(IPin*))(*(void***)pConnected)[vtSlot / 4])(
            pConnected);
    return (HRESULT)0x80040209;                               // VFW_E_NOT_CONNECTED
}

HRESULT CBasePin::Pin_v4C() { return MMDxPin_ForwardToConnected(this, 0x38); }
HRESULT CBasePin::Pin_v50() { return MMDxPin_ForwardToConnected(this, 0x3C); }
HRESULT CBasePin::Pin_v54() { return MMDxPin_ForwardToConnected(this, 0x40); }

// VA 0x100033C0 - Pin_v58: forward NewSegment(tStart, tStop, dRate) to the
// connected pin (IPin vtable +0x44).
HRESULT CBasePin::Pin_v58(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    if (m_Connected != NULL)
        return m_Connected->NewSegment(tStart, tStop, dRate);
    return (HRESULT)0x80040209;                               // VFW_E_NOT_CONNECTED
}

// VA 0x10001310 - Pin_v3C = DecideBufferSize (primary +0x3C; BOTH the
// CSourceStream (0x10008444) and CPushPinDIBSq (0x100081E4) vtables point
// here).  Locks the filter critsec via the +0xA0 back-pointer; E_POINTER on
// null args; E_FAIL when the pin object is not bitmap-ready (bytes
// object+0x5B3 / object+0x5B0 — CPushPinDIBSq's m_bStreamEnded /
// m_bBitmapSet); raises the wanted buffer size from the display-state
// object at pin-primary +0x78 (its +0x44 dword), SetProperties on the
// allocator (IMemAllocator vtable +0x0C) and E_FAIL when the granted
// per-buffer size falls short.
HRESULT CBasePin::Pin_v3C(void* a1, void* a2)
{
    IMemAllocator* pAlloc = (IMemAllocator*)a1;
    ALLOCATOR_PROPERTIES* pProps = (ALLOCATOR_PROPERTIES*)a2;
    CRITICAL_SECTION* pcsFilter = &m_pFilter->m_CritSec;      // *(this+0xA0)+0x58
    EnterCriticalSection(pcsFilter);
    if (a1 == NULL || a2 == NULL) {
        LeaveCriticalSection(pcsFilter);
        return E_POINTER;                                     // 0x80004003
    }
    char* pObjectBase = Slot(this, -0x48);            // pin object base
    // 0x100013af: m_bStreamEnded(object+0x5B3) || !m_bBitmapSet(object+0x5B0)
    if (*(unsigned char*)(pObjectBase + 0x5B3) != 0 ||
        *(unsigned char*)(pObjectBase + 0x5B0) == 0) {
        LeaveCriticalSection(pcsFilter);
        return E_FAIL;                                        // 0x80004005
    }
    void* pDisplayState = *(void**)Slot(this, 120);           // 0x100013b8 (pin+0x78)
    const DWORD cbWanted = *(DWORD*)Slot(pDisplayState, 68);  // +0x44
    pProps->cBuffers = 1;                                     // *a3 = 1
    if (cbWanted > (DWORD)pProps->cbBuffer)
        pProps->cbBuffer = (LONG)cbWanted;                    // 0x100013c7
    ALLOCATOR_PROPERTIES actual;                              // v10/v11 stack block
    const HRESULT hr = pAlloc->SetProperties(pProps, &actual);// vtable +0x0C
    if (hr < 0) {
        LeaveCriticalSection(pcsFilter);
        return hr;
    }
    if (actual.cbBuffer < pProps->cbBuffer) {
        LeaveCriticalSection(pcsFilter);
        return E_FAIL;                                        // 0x80004005 (LABEL_6)
    }
    LeaveCriticalSection(pcsFilter);
    return S_OK;
}

// =============================================================================
// CBasePin — IPin (sub-vtable at primary +0x0C)
// =============================================================================

// VA 0x10003ED0 - AgreeMediaType (helper of the connection machinery; the
// original reaches the +0x24/+0x28/+0x30/+0x2C slots virtually — those
// dispatches are kept, Pin_v24/Pin_v28 inlined per the header-arity note).
static HRESULT MMDxPin_AgreeMediaType(CBasePin* pPin, IPin* pReceivePin,
                                      const AM_MEDIA_TYPE* pmt)
{
    HRESULT hr = pPin->Pin_v28(pReceivePin);         // slot +0x28 (0x100030F0)
    if (hr < 0) {
        pPin->Pin_v2C();                                      // slot +0x2C (0x10003130)
        return hr;
    }
    hr = pPin->CheckMediaType(pmt);                           // slot +0x20
    if (hr != 0) {
        if (hr >= 0 || hr == E_FAIL || hr == E_INVALIDARG)    // 0x10003f53..63
            hr = (HRESULT)0x8004022A;                         // VFW_E_NO_ACCEPTABLE_TYPES
    } else {
        P_connected(pPin) = pReceivePin;                      // this[6]
        pReceivePin->AddRef();
        hr = pPin->Pin_v24(pmt);                 // slot +0x24 (0x10002E90)
        if (hr >= 0) {
            hr = pReceivePin->ReceiveConnection(
                    static_cast<IPin*>(pPin), pmt);           // IPin vtable +0x10
            if (hr >= 0) {
                hr = pPin->Pin_v30(pReceivePin);              // slot +0x30 (0x100030D0)
                if (hr >= 0)
                    return hr;
                pReceivePin->Disconnect();                    // IPin vtable +0x14
            }
        }
    }
    pPin->Pin_v2C();                                          // slot +0x2C (0x10003f6f)
    if (P_connected(pPin) != NULL) {
        P_connected(pPin)->Release();
        P_connected(pPin) = NULL;
    }
    return hr;
}

// VA 0x10003F90 - TryMediaTypes: walk pEnum, offering every type (optionally
// filtered by partial match against pmt) via AgreeMediaType.
static HRESULT MMDxPin_TryMediaTypes(CBasePin* pPin, IPin* pReceivePin,
                                     const AM_MEDIA_TYPE* pmt,
                                     IEnumMediaTypes* pEnum)
{
    const HRESULT hrReset = pEnum->Reset();                   // vtable +0x14
    if (hrReset < 0)
        return hrReset;
    HRESULT hrFail = S_OK;                                    // v8 = 0
    for (;;) {
        AM_MEDIA_TYPE* pmtCur = NULL;
        if (pEnum->Next(1, &pmtCur, NULL) != 0)               // vtable +0x0C
            break;                                            // enumeration exhausted
        HRESULT hrThis;
        if (pmt == NULL || MMDxShow_MediaTypePartialMatch(pmtCur, pmt)) {
            const HRESULT hrTry = MMDxPin_AgreeMediaType(pPin, pReceivePin, pmtCur);
            hrThis = hrTry;
            if (hrTry < 0 && hrFail >= 0 &&
                hrTry != E_FAIL && hrTry != E_INVALIDARG && hrTry != (HRESULT)0x8004022A)
                hrFail = hrTry;                               // first real failure sticks
        } else {
            hrThis = (HRESULT)0x80040207;                     // VFW_E_NO_ACCEPTABLE_TYPES
        }
        MMDxShow_DeleteMediaType(pmtCur);                     // 0x10004F50
        if (hrThis == 0)
            return S_OK;
    }
    return hrFail != 0 ? hrFail : (HRESULT)0x80040207;
}

// VA 0x10004080 - AgreeMediaTypeOrEnum: with a complete type, negotiate it
// directly; otherwise enumerate up to twice (the peer's enumerator first
// when the +0x26 flag is set, else ours first) and TryMediaTypes each.
static HRESULT MMDxPin_AgreeMediaTypeOrEnum(CBasePin* pPin, IPin* pReceivePin,
                                            const AM_MEDIA_TYPE* pmt)
{
    if (pmt != NULL && !MMDxShow_MediaTypeIsWildcard(pmt))    // 0x10004BA0
        return MMDxPin_AgreeMediaType(pPin, pReceivePin, pmt);
    HRESULT hrFail = (HRESULT)0x80040207;                     // v10
    for (int i = 0; i < 2; ++i) {
        IEnumMediaTypes* pEnum = NULL;
        IPin* pEnumSrc = ((DWORD)P_flag26(pPin) == (DWORD)i)  // v4 == *(BYTE*)(this+38)
            ? pReceivePin
            : static_cast<IPin*>(pPin);                       // this+0x0C subobject
        const HRESULT hrEnum = pEnumSrc->EnumMediaTypes(&pEnum); // IPin vtable +0x30
        if (hrEnum >= 0) {
            const HRESULT hr = MMDxPin_TryMediaTypes(pPin, pReceivePin, pmt, pEnum);
            pEnum->Release();
            if (hr >= 0)
                return S_OK;
            if (hr != E_FAIL && hr != E_INVALIDARG && hr != (HRESULT)0x8004022A)
                hrFail = hr;
        }
    }
    return hrFail;
}

// VA 0x100047B0 - IPin::Connect.
STDMETHODIMP CBasePin::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt)
{
    if (pReceivePin == NULL)
        return E_POINTER;
    EnterCriticalSection(P_pLock(this));                      // +0x20
    if (m_Connected != NULL) {
        LeaveCriticalSection(P_pLock(this));
        return (HRESULT)0x80040204;                           // VFW_E_ALREADY_CONNECTED
    }
    if (F_state(P_pOwner(this)) == State_Stopped ||           // *(*(this+0x1C)+0x14)... see note
        P_flag25(this) != 0) {
        const HRESULT hr = MMDxPin_AgreeMediaTypeOrEnum(this, pReceivePin, pmt); // 0x10004080
        if (hr >= 0) {
            LeaveCriticalSection(P_pLock(this));
            return S_OK;
        }
        Pin_v2C();                                            // slot +0x2C
        LeaveCriticalSection(P_pLock(this));
        return hr;
    }
    LeaveCriticalSection(P_pLock(this));
    return (HRESULT)0x80040224;                               // VFW_E_WRONG_STATE
}

// VA 0x10004160 - IPin::ReceiveConnection.
STDMETHODIMP CBasePin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
    if (pConnector == NULL || pmt == NULL)
        return E_POINTER;
    EnterCriticalSection(P_pLock(this));                      // +0x20
    if (m_Connected != NULL) {
        LeaveCriticalSection(P_pLock(this));
        return (HRESULT)0x80040204;                           // VFW_E_ALREADY_CONNECTED
    }
    if (F_state(P_pOwner(this)) != State_Stopped && P_flag25(this) == 0) {
        LeaveCriticalSection(P_pLock(this));
        return (HRESULT)0x80040224;                           // VFW_E_WRONG_STATE
    }
    HRESULT hr = Pin_v28(pConnector);          // slot +0x28 (0x100030F0)
    if (hr >= 0) {
        hr = CheckMediaType(pmt);                             // slot +0x20
        if (hr != 0) {
            Pin_v2C();                                        // slot +0x2C
            if (hr >= 0 || hr == E_FAIL || hr == E_INVALIDARG)
                hr = (HRESULT)0x8004022A;                     // VFW_E_NO_ACCEPTABLE_TYPES
        } else {
            m_Connected = pConnector;                         // +0x18
            pConnector->AddRef();
            hr = Pin_v24(pmt);             // slot +0x24 (0x10002E90)
            if (hr >= 0) {
                hr = Pin_v30(pConnector);                     // slot +0x30
                if (hr >= 0) {
                    LeaveCriticalSection(P_pLock(this));
                    return S_OK;
                }
            }
            m_Connected->Release();
            m_Connected = NULL;
            Pin_v2C();                                        // slot +0x2C
        }
    } else {
        Pin_v2C();                                            // slot +0x2C
    }
    LeaveCriticalSection(P_pLock(this));
    return hr;
}

// VA 0x10002EE0 - DisconnectPeer (helper): S_FALSE when already
// disconnected; otherwise teardown via the +0x2C slot and drop the
// connection reference.
static HRESULT MMDxPin_DisconnectPeer(CBasePin* pPin)
{
    if (P_connected(pPin) == NULL)
        return 1;                                             // S_FALSE
    const HRESULT hr = pPin->Pin_v2C();                       // slot +44 decimal (0x2C)
    if (hr >= 0) {
        P_connected(pPin)->Release();
        P_connected(pPin) = NULL;
        return S_OK;
    }
    return hr;
}

// VA 0x100042D0 - IPin::Disconnect: refuses while the owner filter is not
// stopped (unless the +0x25 flag allows it).
STDMETHODIMP CBasePin::Disconnect()
{
    EnterCriticalSection(P_pLock(this));                      // +0x20
    if (F_state(P_pOwner(this)) != State_Stopped &&           // *(*(this+0x1C...))+0x14
        P_flag25(this) == 0) {
        LeaveCriticalSection(P_pLock(this));
        return (HRESULT)0x80040224;                           // VFW_E_WRONG_STATE
    }
    const HRESULT hr = MMDxPin_DisconnectPeer(this);          // 0x10002EE0
    LeaveCriticalSection(P_pLock(this));
    return hr;
}

// VA 0x10002F20 - IPin::ConnectedTo.
STDMETHODIMP CBasePin::ConnectedTo(IPin** ppPin)
{
    if (ppPin == NULL)
        return E_POINTER;
    *ppPin = m_Connected;                                     // +0x18
    if (m_Connected == NULL)
        return (HRESULT)0x80040209;                           // VFW_E_NOT_CONNECTED
    m_Connected->AddRef();
    return S_OK;
}

// VA 0x10004360 - IPin::ConnectionMediaType: deep copy of m_mt (+0x34) under
// the pin lock; VFW_E_NOT_CONNECTED when disconnected.
STDMETHODIMP CBasePin::ConnectionMediaType(AM_MEDIA_TYPE* pmt)
{
    if (pmt == NULL)
        return E_POINTER;
    EnterCriticalSection(P_pLock(this));                      // +0x20
    if (m_Connected != NULL) {                                // +0x18
        MMDxShow_CopyMediaType(pmt, &P_mt(this));             // 0x10004D00
        LeaveCriticalSection(P_pLock(this));
        return S_OK;
    }
    MMDxShow_InitMediaType(pmt);                              // 0x10004B80
    LeaveCriticalSection(P_pLock(this));
    return (HRESULT)0x80040209;                               // VFW_E_NOT_CONNECTED
}

// VA 0x10002F60 - IPin::QueryPinInfo.
STDMETHODIMP CBasePin::QueryPinInfo(PIN_INFO* pInfo)
{
    if (pInfo == NULL)
        return E_POINTER;
    CBaseFilter* pOwner = P_pOwner(this);                     // +0x28
    pInfo->pFilter = pOwner ? static_cast<IBaseFilter*>(pOwner) : NULL;
    if (pOwner != NULL)
        pInfo->pFilter->AddRef();                             // vtable +0x04
    if (P_pNameW(this) != NULL)                               // +0x14
        MMDxShow_wcsncpyClamp(pInfo->achName, P_pNameW(this), 128);
    else
        pInfo->achName[0] = 0;
    pInfo->dir = P_dir(this);                                 // +0x1C
    return S_OK;
}

// VA 0x10002FE0 - IPin::QueryDirection.
STDMETHODIMP CBasePin::QueryDirection(PIN_DIRECTION* pPinDir)
{
    if (pPinDir == NULL)
        return E_POINTER;
    *pPinDir = P_dir(this);                                   // +0x1C
    return S_OK;
}

// VA 0x10001D00 - FindPinIndex (helper): index of the pin whose IPin
// subobject (m_ppPins[i] + 0x54) equals pIPin, else -1.
static int MMDxFilter_FindPinIndex(CBaseFilter* pFilter, IPin* pIPin)
{
    const int cPins = *(int*)Slot(pFilter, 0x50);             // m_cPins
    if (cPins <= 0)
        return -1;
    CSourceStream* const* ppPins = *(CSourceStream***)Slot(pFilter, 0x54);
    for (int i = 0; i < cPins; ++i)
        if (ppPins[i] != NULL && static_cast<IPin*>(ppPins[i]) == pIPin)
            return i;
    return -1;
}

// VA 0x10001D40 - IPin::QueryId: the decimal 1-based pin index as a string.
STDMETHODIMP CBasePin::QueryId(LPWSTR* Id)
{
    if (Id == NULL)
        return E_POINTER;
    const int idx = MMDxFilter_FindPinIndex(P_pOwner(this), static_cast<IPin*>(this));
    const int n = idx + 1;
    if (n < 1)
        return (HRESULT)0x80040216;                           // VFW_E_NOT_FOUND
    WCHAR* p = (WCHAR*)CoTaskMemAlloc(8);
    *Id = p;
    if (p == NULL)
        return E_OUTOFMEMORY;
    MMDxShow_IntToWide(n, p);                                 // 0x10005CB0
    return S_OK;
}

// VA 0x10003000 - IPin::QueryAccept: the +0x20 slot; S_FALSE on any failure
// HRESULT (0x10003024).
STDMETHODIMP CBasePin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
    if (pmt == NULL)
        return E_POINTER;
    const HRESULT hr = CheckMediaType(pmt);                   // slot +0x20
    if (hr < 0)
        return 1;                                             // S_FALSE
    return hr;
}

// VA 0x100048D0 - IPin::EnumMediaTypes: news a 0x14-byte CEnumMediaTypes on
// the primary subobject.
STDMETHODIMP CBasePin::EnumMediaTypes(IEnumMediaTypes** ppEnum)
{
    if (ppEnum == NULL)
        return E_POINTER;
    CEnumMediaTypes* pEnum = new CEnumMediaTypes(this, NULL); // operator new(0x14)
    *ppEnum = pEnum;
    return pEnum != NULL ? S_OK : E_OUTOFMEMORY;
}

// VA 0x100011C0 - IPin::QueryInternalConnections: E_NOTIMPL stub.
STDMETHODIMP CBasePin::QueryInternalConnections(IPin** /*apPin*/, ULONG* /*nPin*/)
{
    return E_NOTIMPL;                                         // 0x80004001
}

// VA 0x10003370 - IPin::EndOfStream / BeginFlush / EndFlush: the same
// eight-byte E_NOTIMPL stub shared with the default GetMediaType1.
STDMETHODIMP CBasePin::EndOfStream()  { return E_NOTIMPL; }   // 0x8000FFFF
STDMETHODIMP CBasePin::BeginFlush()   { return E_NOTIMPL; }
STDMETHODIMP CBasePin::EndFlush()     { return E_NOTIMPL; }

// VA 0x10003090 - IPin::NewSegment: cache the segment in the pin
// (+0x80/+0x88/+0x90) — nothing is forwarded here (Pin_v58 forwards).
STDMETHODIMP CBasePin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop,
                                  double dRate)
{
    P_tStart(this) = tStart;                                  // +0x80
    P_tStop(this)  = tStop;                                   // +0x88
    P_dRate(this)  = dRate;                                   // +0x90
    return S_OK;
}

// =============================================================================
// CBasePin — IQualityControl (sub-vtable at primary +0x10)
// =============================================================================

// VA 0x100011E0 - SetSink: E_FAIL stub (the sink slot is only written by
// Notify below).
STDMETHODIMP CBasePin::SetSink(IQualityControl* /*piqc*/)
{
    return E_FAIL;                                            // 0x80004005
}

// VA 0x10003050 - Notify: store pSelf into the +0x2C slot under the pin lock.
STDMETHODIMP CBasePin::Notify(IBaseFilter* pSelf, Quality /*q*/)
{
    EnterCriticalSection(P_pLock(this));                      // +0x20
    P_pNotify(this) = pSelf;                                  // +0x2C
    LeaveCriticalSection(P_pLock(this));
    return S_OK;
}

// =============================================================================
// CSourceStream
//   ctor chain 0x10001960 -> 0x10002110 (this core) -> 0x10005D10 (CAMThread)
//   -> 0x10004970 (CBaseOutputPin: CBasePin 0x100046B0 with dir=1 and the
//   parent's critsec as pLock) -> 0x10002197 AddPin (0x10001FC0).
// =============================================================================

CSourceStream::CSourceStream(const char* pName, HRESULT* phr,
                             CSource* pParent, LPCWSTR pName2)
    : CAMThread(), CBaseOutputPin(pName, pParent, phr)
{
    // CAMThread core 0x10005D10 runs first (base ctor), then the CBasePin
    // core 0x100046B0 (base ctor), then this body:

    // 0x10004755..0x1000478a: duplicate the wide pin name into +0x14
    if (pName2 != NULL) {
        const int cch = MMDxShow_wcslen(pName2) + 1;          // 0x10005C90
        WCHAR* p = (WCHAR*)operator new(2 * cch);
        P_pNameW(this) = p;                                   // +0x14
        if (p != NULL)
            memcpy(p, pName2, 2 * cch);
    }
    m_pFilter = pParent;                                      // this[58] -> +0xA0 (0x1000218c)
    *phr = S_OK;                                              // *a3 (AddPin's 0x10001FC0 result path)
    pParent->AddPin(this);                                    // 0x10001FC0
}

// VA 0x10001DC0 (+0x10002D50) - CSourceStream dtor core: remove ourselves
// from the parent's pin array, run the CBasePin teardown (delete the wide
// name, free m_mt, unlock the module).  Declared as ~CSourceStream in the
// header; the compiler chains it after the ~CPushPinDIBSq body exactly like
// the original's 0x10001A50 -> 0x10001DC0 order.
CSourceStream::~CSourceStream()
{
    CBasePin* pPin = static_cast<CBasePin*>(this);
    static_cast<CSource*>(*(CBaseFilter**)Slot(pPin, 0xA0))->RemovePin(this);   // 0x10001C20
    // 0x10002D50 (the original's CBasePin-layer dtor):
    ::operator delete(P_pNameW(pPin));                         // +0x14
    MMDxShow_FreeMediaType(&P_mt(pPin));                       // +0x34 (0x10004DB0)
    MMDxShow_UnlockModule();                                  // 0x10004FA0
    // ~CAMThread() (0x10005D50: thread close, critsecs, events) runs as the
    // implicit base destructor immediately after the dtor body in the
    // original ordering.
}

// VA 0x100021E0 - CSourceStream::CheckMediaType (primary +0x20): build our
// preferred type through the root-vtable +0x1C single-arg GetMediaType (the
// cross-base dispatch, this - 0x48) and compare it with 0x10004DD0.
HRESULT CSourceStream::CheckMediaType(const AM_MEDIA_TYPE* pmt)
{
    CRITICAL_SECTION* pcsFilter = &m_pFilter->m_CritSec;      // this[40]+88
    EnterCriticalSection(pcsFilter);
    AM_MEDIA_TYPE mt;
    MMDxShow_InitMediaType(&mt);                              // 0x10004DC0
    // (*(rootvtable + 0x1C))(pin - 0x48, &mt)
    this->GetMediaType1(&mt);
    const BOOL bEqual = MMDxShow_MediaTypePartialEqual(&mt, pmt);  // 0x10004DD0
    MMDxShow_FreeMediaType(&mt);                              // 0x10004DB0
    LeaveCriticalSection(pcsFilter);
    if (bEqual)
        return S_OK;                                          // v5 == 0 -> 0
    return E_FAIL;                                            // 0x80004005 (0x10002284)
}

// VA 0x10003370 - CSourceStream default GetMediaType1 (root vtable +0x1C):
// eight-byte E_NOTIMPL stub (0x8000FFFF); the binary shares this one body
// with IPin::EndOfStream/BeginFlush/EndFlush.  CPushPinDIBSq overrides it
// with 0x100011F0.
HRESULT CSourceStream::GetMediaType1(AM_MEDIA_TYPE* pmt)
{
    (void)pmt;                 // the original ignores the argument entirely
    return E_NOTIMPL;          // mov eax, 0x8000FFFF; ret 4
}
