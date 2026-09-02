// ===========================================================================
// Model-edge / English-name dialog (menu 259, sub_464BD0 family)
// ===========================================================================
// Split out of src/window/command_view_menu.cpp (the menu-251..302 command
// family) so the dialog's helper bodies can be ported independently.
// Every function keeps its original x86 VA; behaviour notes live in the
// per-function comments.  Still TODO(port) unless noted otherwise.
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#include "mikudancestudio/d3dx_dyn.hpp"
#include "mikudancestudio/global_key_layout.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/model.hpp"

namespace mikudancestudio {
void Sub43C430(HWND hDlg) {  // VA 0x0043C430 model-edge dialog init
    (void)hDlg; /* TODO(port) */
}

void Sub45F240(HWND hDlg) {  // VA 0x0045F240 model-edge dialog apply
    (void)hDlg; /* TODO(port) */
}

void Sub45F050(HWND hDlg) {  // VA 0x0045F050 model-edge combo 669 refresh
    (void)hDlg; /* TODO(port) */
}

void Sub45EF10(HWND hDlg) {  // VA 0x0045EF10 model-edge combo 673 refresh
    (void)hDlg; /* TODO(port) */
}

void Sub45EDC0(HWND hDlg) {  // VA 0x0045EDC0 model-edge combo 677 refresh
    (void)hDlg; /* TODO(port) */
}

}  // namespace mikudancestudio
