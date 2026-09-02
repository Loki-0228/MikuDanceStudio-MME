// ===========================================================================
// VA 0x004C4460 - WinMain  (original: _WinMain@16)
// ===========================================================================
// Full 1:1 port.  Flow (verified against decompilation + disassembly):
//   1. operator new(0xA4530) -> ctor (0x42AE60) -> Block (0x54593C)
//      -> memset 0 -> InitDefaults (0x40A730)
//   2. command line -> ConvertAnsiToWide (0x407A70) into wchar_t[256]
//      @ this+0xA0900;  empty command line -> swprintf_s(buf, 0x100, L"%s%s")
//      with ZERO varargs (faithful to the original call site).
//   3. InitMainWindowAndD3D (0x47A5B0);  failure -> return 0.
//   4. FPS-capped PeekMessage loop; idle branch computes the frame delta
//      from timeGetTime() and calls the frame driver (0x46B090).
//   5. WM_QUIT -> ShutdownCleanup (0x462C40) -> free(Block), Block = 0.
// ===========================================================================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <timeapi.h>   // timeGetTime (excluded by WIN32_LEAN_AND_MEAN)
#include <new>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cwchar>
#include <cstring>

#include "mikudancestudio/globals.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/ported_funcs.hpp"

namespace {

LONG CALLBACK DiagnosticExceptionHandler(EXCEPTION_POINTERS* info) {
    if (info == nullptr || info->ExceptionRecord == nullptr ||
        info->ContextRecord == nullptr)
        return EXCEPTION_CONTINUE_SEARCH;
    const DWORD code = info->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION &&
        code != EXCEPTION_ILLEGAL_INSTRUCTION &&
        code != EXCEPTION_ARRAY_BOUNDS_EXCEEDED &&
        code != EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_CONTINUE_SEARCH;
    char directory[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA(
        "MIKUDANCESTUDIO_STATE_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return EXCEPTION_CONTINUE_SEARCH;
    char path[MAX_PATH]{};
    std::snprintf(path, sizeof(path), "%s\\first_chance_exception.txt",
                  directory);
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES)
        return EXCEPTION_CONTINUE_SEARCH;
    FILE* stream = nullptr;
    if (fopen_s(&stream, path, "wb") != 0 || stream == nullptr)
        return EXCEPTION_CONTINUE_SEARCH;
    CONTEXT* context = info->ContextRecord;
    MEMORY_BASIC_INFORMATION memory{};
    VirtualQuery(info->ExceptionRecord->ExceptionAddress, &memory,
                 sizeof(memory));
    char modulePath[MAX_PATH]{};
    GetModuleFileNameA(static_cast<HMODULE>(memory.AllocationBase), modulePath,
                       MAX_PATH);
#if defined(_M_IX86)
    std::fprintf(stream,
        "code=0x%08X address=%p image_base=%p module_base=%p "
        "module=%s eip=%08X esp=%08X ebp=%08X "
        "eax=%08X ebx=%08X ecx=%08X edx=%08X esi=%08X edi=%08X\r\n",
        static_cast<unsigned>(code), info->ExceptionRecord->ExceptionAddress,
        GetModuleHandleA(nullptr), memory.AllocationBase, modulePath,
        context->Eip, context->Esp, context->Ebp, context->Eax,
        context->Ebx, context->Ecx, context->Edx, context->Esi,
        context->Edi);
#else
    std::fprintf(stream,
        "code=0x%08X address=%p image_base=%p module_base=%p "
        "module=%s rip=%016llX rsp=%016llX rbp=%016llX "
        "rax=%016llX rbx=%016llX rcx=%016llX rdx=%016llX "
        "rsi=%016llX rdi=%016llX\r\n",
        static_cast<unsigned>(code), info->ExceptionRecord->ExceptionAddress,
        GetModuleHandleA(nullptr), memory.AllocationBase, modulePath,
        static_cast<unsigned long long>(context->Rip),
        static_cast<unsigned long long>(context->Rsp),
        static_cast<unsigned long long>(context->Rbp),
        static_cast<unsigned long long>(context->Rax),
        static_cast<unsigned long long>(context->Rbx),
        static_cast<unsigned long long>(context->Rcx),
        static_cast<unsigned long long>(context->Rdx),
        static_cast<unsigned long long>(context->Rsi),
        static_cast<unsigned long long>(context->Rdi));
#endif
#if defined(_M_IX86)
    const std::uintptr_t* stack =
        reinterpret_cast<const std::uintptr_t*>(context->Esp);
#else
    const std::uintptr_t* stack =
        reinterpret_cast<const std::uintptr_t*>(context->Rsp);
#endif
    __try {
        for (int i = 0; i < 32; ++i)
            std::fprintf(stream, "stack[%02d]=%p\r\n", i,
                         reinterpret_cast<const void*>(stack[i]));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    // Module-range scan of the faulting stack: every dword that lands in
    // the main image is a candidate return address (the raw stack[0..31)
    // window starts at Esp and misses deeper frames when the fault is a
    // jump through a bad pointer).
    const std::uintptr_t imageBase = reinterpret_cast<std::uintptr_t>(
        GetModuleHandleA(nullptr));
    __try {
        for (int i = 0; i < 4096; ++i) {
            const std::uintptr_t address = stack[i];
            if (address >= imageBase && address < imageBase + 0x140000)
                std::fprintf(stream,
                             "ret_candidate[sp+%04X]=%p (rva %06X)\r\n",
                             static_cast<unsigned>(i * sizeof(*stack)),
                             reinterpret_cast<const void*>(address),
                             static_cast<unsigned>(address - imageBase));
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    std::fclose(stream);
    return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd) {
    using namespace mikudancestudio;

    (void)hPrevInstance;

    if (GetEnvironmentVariableA("MIKUDANCESTUDIO_STATE_DUMP_DIR", nullptr, 0) != 0)
        AddVectoredExceptionHandler(1, DiagnosticExceptionHandler);

    // operator new(0xA4530) with the original's null-check (VC9 new
    // semantics), then ctor 0x42AE60 and the global Block assignment.
    MMDApp* app = new (std::nothrow) MMDApp();
    g_Block = app;
    if (app == nullptr)
        return 0;  // original would proceed on null only if new failed;
                   // ctor/defaults below are guarded by early exit.
    app->state = MMDAppState{};  // original: memset(p, 0, 0xA4530)
    app->InitDefaults();                                          // 0x40A730

    if (*lpCmdLine != '\0') {
        wchar_t converted[256];  // `Source` local in the original (ebp-0x204)
        ConvertAnsiToWide(app->LocaleTablePtr(),                  // [Block+0xA06C4]
                          lpCmdLine, converted, 0x100);           // 0x407A70
        wcscpy_s(app->EnvFileName(), 0x100, converted);
    } else {
        // Original pushes only Format/BufferCount/Buffer - no varargs
        // (verified in disassembly at 0x4C44CA..0x4C44DA).  VC9 read two
        // garbage stack "pointers" and produced a junk path that the load
        // step then failed to open.  Deterministic equivalent: two empty
        // strings, so the buffer ends up empty and the load fails the same
        // way without relying on undefined behaviour of the modern CRT.
        swprintf_s(app->EnvFileName(), 0x100, g_SourceFormat, L"", L"");
    }

    if (!InitMainWindowAndD3D(g_Block, hInstance, nShowCmd))       // 0x47A5B0
        return 0;

    MSG msg;
    std::uint32_t timeHigh = 0;         // v7/ebp - high 32 bits of last tick
    DWORD timeLow = timeGetTime();      // Time/ebx
    app->MilliToSec() = 0.001f;         // [Block+0xA0B70] = flt_5318D0

    PeekMessageA(&msg, nullptr, 0, 0, 0);  // PM_NOREMOVE prime, original arg set
    while (msg.message != WM_QUIT) {       // 18
        if (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        } else {
            std::uint32_t nowLow = timeGetTime();           // v9
            std::uint32_t nowHigh = 0;                       // v10
            // v13 = (now - last) * 0.001  (64-bit delta, double mul)
            float delta = static_cast<float>(
                static_cast<double>(nowLow - timeLow) * 0.001000000047497451);
            app->DeltaTime() = delta;                        // [Block+0xA077C]

            // v14 = 1/fpsLimit - delta;  Sleep when cap enabled and positive.
            float sleepSec = 1.0f / app->FpsLimit() - delta;
            if (app->RecordingWindow() == nullptr && sleepSec > 0.0f) {
                Sleep(static_cast<DWORD>(sleepSec * 1000.0f));
                std::uint64_t addMs =
                    static_cast<std::uint64_t>(sleepSec / 0.001000000047497451);
                nowHigh = static_cast<std::uint32_t>(addMs >> 32);
                nowLow += static_cast<std::uint32_t>(addMs);
                app->DeltaTime() = sleepSec + app->DeltaTime();
            }
            app->TimeNowLow() = nowLow;                      // [Block+0xA0B68]
            app->TimeNowHigh() = nowHigh;                    // [Block+0xA0B6C]
            timeLow = nowLow;
            timeHigh = nowHigh;

            FrameDriver(g_Block);                            // 0x46B090
        }
    }

    if (g_Block != nullptr) {
        MMDApp* victim = g_Block;
        ShutdownCleanup(g_Block);                            // 0x462C40
        delete victim;
        g_Block = nullptr;
    }
    return static_cast<int>(msg.wParam);
}
