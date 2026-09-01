// ===========================================================================
// VA 0x00407A70 - ConvertAnsiToWide  (original: sub_407A70)
// ===========================================================================
// Robust ANSI -> UTF-16 conversion used for command lines and file names:
//   1. wcscpy_s(dest, count, L"")          (global Source @ 0x529688 head)
//   2. if input empty -> done
//   3. MultiByteToWideChar(CP_ACP, 0, in, -1, null, 0) for the size, then
//      convert into a heap buffer; success -> wcsncpy_s into dest.
//   4. failure -> three/four _mbstowcs_s_l fallbacks using locale handles
//      read from the caller-provided locale table (`this[30001..30004]`,
//      i.e. table + 0x1D4B4 .. + 0x1D4C0 as 4-byte slots).
//
// Deviation note (phase 1): in the original, `this` is loaded from
// [Block+0xA06C4] which is 0 at WinMain time; the fallback locale loads
// would fault there too, so a null table falls back to C-locale mbstowcs_s
// instead of crashing during development.
// ===========================================================================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdlib>
#include <cstring>

#include "mikudancestudio/globals.hpp"

namespace mikudancestudio {

void ConvertAnsiToWide(void* localeTableBase, const char* multiByteStr,
                       wchar_t* destination, int destinationWords) {
    wcscpy_s(destination, static_cast<rsize_t>(destinationWords), L"");

    if (*multiByteStr == '\0')
        return;

    int wideLen = MultiByteToWideChar(0, 0, multiByteStr, -1, nullptr, 0);
    wchar_t* wideBuf = static_cast<wchar_t*>(
        operator new(sizeof(wchar_t) * wideLen));

    if (MultiByteToWideChar(0, 0, multiByteStr,
                            static_cast<int>(strlen(multiByteStr)) + 1,
                            wideBuf, wideLen)) {
        wcsncpy_s(destination, static_cast<rsize_t>(destinationWords),
                  wideBuf, static_cast<rsize_t>(-1));
    } else {
        // Locale table slots 30001..30004 (pointer arithmetic as in the
        // decompilation: 4-byte stride on a byte base).
        void* locales[4] = {nullptr, nullptr, nullptr, nullptr};
        if (localeTableBase != nullptr) {
            auto* base = static_cast<unsigned char*>(localeTableBase);
            for (int i = 0; i < 4; ++i)
                std::memcpy(&locales[i], base + 4 * (30001 + i), sizeof(void*));
        }

        bool converted = false;
        if (locales[0] == nullptr) {
            // documented deviation: original dereferences table+30001..4
            converted = mbstowcs_s(nullptr, destination,
                                   static_cast<size_t>(destinationWords),
                                   multiByteStr, static_cast<size_t>(-1)) == 0;
        } else {
            for (int i = 0; i < 3 && !converted; ++i) {
                converted = _mbstowcs_s_l(nullptr, destination,
                                          static_cast<size_t>(destinationWords),
                                          multiByteStr, static_cast<size_t>(-1),
                                          reinterpret_cast<_locale_t>(locales[i])) == 0;
            }
            if (!converted) {
                _mbstowcs_s_l(nullptr, destination,
                              static_cast<size_t>(destinationWords),
                              multiByteStr, static_cast<size_t>(-1),
                              reinterpret_cast<_locale_t>(locales[3]));
            }
        }
        free(wideBuf);
        return;
    }
    free(wideBuf);
}

}  // namespace mikudancestudio
