// ===========================================================================
// Dialog-procedure scaffold helpers (command_file_menu.cpp /
// command_view_menu.cpp)
// ===========================================================================
// The modal/modeless dialog procedures of the two menu families repeat the
// same WM_INITDIALOG / WM_COMMAND scaffolding line for line: the
// "owned-mode top" SetWindowPos dance gated on the floating render window
// (app+0xA0D38), the EM_REPLACESEL prefill of the edit controls, the
// SetFocus + EM_SETSEL(0, len) select-all pair, and the GetWindowTextA +
// atol/atof read-back of the OK handler.  The helpers below capture exactly
// those sequences - message order and parameters preserved verbatim (each
// dialog's original-VA comment carries the provenance).
//
// Two "top" variants exist because the original uses two different
// SetWindowPos shapes: the view-menu family raises the dialog to HWND_TOP,
// the file-menu family to HWND_TOPMOST.  Both gates test the same
// floating-window handle; the insert-after constant differs, so they stay
// separate helpers (behaviour, not style).
// =========================================================================//
#ifndef MIKUDANCESTUDIO_WINDOW_DIALOG_SCAFFOLD_HPP
#define MIKUDANCESTUDIO_WINDOW_DIALOG_SCAFFOLD_HPP

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdlib>

#include "mikudancestudio/mmd_app.hpp"

namespace mikudancestudio {

// EM_REPLACESEL prefill of one edit control (the dialogs fill their empty
// edits by replacing the empty selection, not by SetWindowText).
inline void PrefillEdit(HWND hDlg, int controlId, const char* text) {
    SendMessageA(GetDlgItem(hDlg, controlId), EM_REPLACESEL, 0,
                 reinterpret_cast<LPARAM>(text));
}

// SetFocus on the edit, then EM_SETSEL 0..GetWindowTextLength - the
// focus-and-select-all pair every prefilling dialog ends WM_INITDIALOG
// with (message order preserved).
inline void SelectAllEdit(HWND hDlg, int controlId) {
    SetFocus(GetDlgItem(hDlg, controlId));
    SendMessageA(GetDlgItem(hDlg, controlId), EM_SETSEL, 0,
                 GetWindowTextLengthA(GetDlgItem(hDlg, controlId)));
}

// GetWindowTextA + atol / atof read-back of one edit control.  maxChars is
// the original's GetWindowTextA cap (20 at every read site except the
// accessory-order count edit, which reads 256).
inline int ReadIntFromEdit(HWND hDlg, int controlId, int maxChars = 20) {
    char text[0x100];
    GetWindowTextA(GetDlgItem(hDlg, controlId), text, maxChars);
    return static_cast<int>(atol(text));
}

inline float ReadFloatFromEdit(HWND hDlg, int controlId, int maxChars = 20) {
    char text[0x100];
    GetWindowTextA(GetDlgItem(hDlg, controlId), text, maxChars);
    return static_cast<float>(atof(text));
}

// View-menu family top dance: raise to HWND_TOP (flags 3) when the
// floating render window exists.
inline void MakeDialogTopIfRequested(MMDApp* app, HWND hDlg) {
    if (app->state.floatingWindow != 0) {
        // original: SetWindowPos(hDlg, HWND_MESSAGE|2, ...) == HWND_TOP
        SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
    }
}

// File-menu family top dance: HWND_TOPMOST (flags 3) on the same gate.
inline void MakeDialogTopmostIfRequested(MMDApp* app, HWND hDlg) {
    if (app->FloatingWindow() != nullptr) {
        SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, 3u);
    }
}

}  // namespace mikudancestudio

#endif  // MIKUDANCESTUDIO_WINDOW_DIALOG_SCAFFOLD_HPP
