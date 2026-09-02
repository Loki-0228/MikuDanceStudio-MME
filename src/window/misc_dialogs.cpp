// ===========================================================================
// Misc dialog bodies: frame-control apply, reorder fill, accessory-edit init, physics ON/OFF apply
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
void Sub43E000(MMDApp* app, HWND hDlg) {  // VA 0x0043E000 frame-control apply
    (void)app; (void)hDlg; /* TODO(port) */
}

void Sub43E680(MMDApp* app, HWND hDlg) {  // VA 0x0043E680 frame-control apply
    (void)app; (void)hDlg; /* TODO(port) */
}

void Sub41E810(int count, HWND hDlg) {  // VA 0x0041E810 reorder-dialog fill
    (void)count; (void)hDlg; /* TODO(port) */
}

void Sub423160(HWND hDlg) {  // VA 0x00423160 accessory-frame dialog init
    (void)hDlg; /* TODO(port) */
}

// 0x00412330 Sub412330 (gravity-track apply + physics dialog refresh) is
// ported in src/model/track_apply.cpp.
void Sub4403C0(int on) {  // VA 0x004403C0 shadow/edge display toggle
    (void)on; /* TODO(port) */
}

}  // namespace mikudancestudio
