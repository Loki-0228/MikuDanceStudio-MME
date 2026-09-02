// ===========================================================================
// Enhance-model IO: toon collect (0x41EA20), save model (0x41EC10), model colour (0x4A4850)
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
int Sub41EA20(HWND hDlg) {  // VA 0x0041EA20 enhance-model dialog collect
    (void)hDlg; return 0; /* TODO(port) */
}

// VA 0x0041EC10 - save enhanced model file (thiscall(app, wide path)).
void Sub41EC10(MMDApp* app, const wchar_t* path) {
    (void)app; (void)path; /* TODO(port) */
}

// VA 0x004A4850 - set model color (thiscall(model, r, g, b)).
void Sub4A4850(MMDApp* model, int r, int g, int b) {
    (void)model; (void)r; (void)g; (void)b; /* TODO(port) */
}

// ---- not-yet-ported original call targets with NO stub elsewhere -------
// (Sub439E40/43A650/43B720/43BB30 - the four frame-line edit commands -
//  moved to src/window/frame_line_edit.cpp; declared in ported_funcs.hpp.)
// VA 0x0040B5A0 - UI language refresh (thiscall, this = app).
void Sub40B5A0(MMDApp* app) { (void)app; /* TODO(port) */ }
// VA 0x0042AE20 - path copy: real port moved to src/media/media_load.cpp.

// ---- helpers ported in other translation units (declared here with their
//      original VAs; not yet registered in ported_funcs.hpp) --------------

// VA 0x004629D0 - fullscreen enter/restore window manager; the real body
// now lives in src/app/avi_record_start.cpp (declared in ported_funcs.hpp).
// VA 0x004076E0 - locale refresh after toon reload (thiscall, this = locale).
void Sub4076E0(void* locale) { (void)locale; /* TODO(port) */ }

}  // namespace mikudancestudio
