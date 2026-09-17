#pragma once

#include <Windows.h>

namespace mikudancestudio {
class MMDApp;

// Only application-owned labels go through this dictionary. Model, bone,
// morph and accessory names keep their original text.
const wchar_t* ChineseUiText(const char* english);
int DrawUiGlyph(MMDApp* app, const char* text, HDC hdc, int size, int x, int y,
                unsigned char r, unsigned char g, unsigned char b, int bold);
int DrawWideUiGlyph(const wchar_t* text, HDC hdc, int size, int x, int y,
                    unsigned char r, unsigned char g, unsigned char b, int bold);
void SetUiControlText(MMDApp* app, HWND control, const char* english);
LRESULT AddUiComboText(MMDApp* app, HWND combo, const char* english);
}
