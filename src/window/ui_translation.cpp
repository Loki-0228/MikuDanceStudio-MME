#include "mikudancestudio/ui_translation.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include <cstring>
#include <cwchar>

namespace mikudancestudio {
namespace {
struct Translation { const char* en; const wchar_t* zh; };
constexpr Translation kLabels[] = {
    {"bone/frame manipulation", L"骨骼 / 帧操作"},
    {"frame manipulation", L"帧操作"},
    {"Interpolation curve", L"插值曲线"}, {"auto", L"自动"},
    {"model manipulation", L"模型操作"}, {"disp", L"显示"},
    {"ON", L"开"}, {"OFF", L"关"},
    {"camera", L"相机"}, {"camer", L"相机"},
    {"view angle ", L"视角"}, {"view angle", L"视角"},
    {"follow bone", L"跟随骨骼"}, {"bone manipulation", L"骨骼操作"},
    {"light manipulation", L"照明操作"}, {"facial manipulation", L"表情操作"},
    {"eyes", L"眼睛"}, {"brow", L"眉毛"}, {"mouth", L"嘴部"}, {"other", L"其他"},
    {"self_shadow manipulation", L"自阴影操作"},
    {"shadow range", L"阴影范围"}, {"detail", L"精细"}, {"far", L"远处"},
    {"accessory manipulation", L"配件操作"}, {"display", L"显示"}, {"shadow", L"阴影"},
    {"view", L"视图"}, {"model", L"模型"}, {"bone", L"骨骼"},
    {"play", L"播放"}, {"stop", L"停止"}, {"repeat", L"循环"},
    {"from flame", L"从当前帧"}, {"stop flame", L"结束后返回"}, {"vol", L"音量"},
    {"camera light accessary", L"相机 / 照明 / 配件"},
    {"camera/light/accessory", L"相机/照明/配件"},
    {"Playing", L"播放中"}, {"Recording(Esc:Stop)", L"录制中(Esc:停止)"},
    {"angle", L"角度"}, {"bone place", L"骨骼位置"},
    {"camera bone trace mode", L"相机跟随骨骼"},
    {"camera bone trace mode (release trace button)", L"相机跟随骨骼（点击跟随按钮取消）"},
    {"camera model trace mode", L"相机跟随模型"},
    {"light", L"照明"}, {"s shadow", L"自阴影"}, {"gravity", L"重力"},
    {"To model", L"到模型"}, {"To camera", L"到相机"}, {"btm", L"下面"},
    {"distance", L"距离"}, {"all", L"全部"}, {"rotation", L"旋转"},
    {"x axis move", L"X轴移动"}, {"y axis move", L"Y轴移动"}, {"z axis move", L"Z轴移动"},
    {"ground", L"地面"}, {"non", L"无"}, {"Go", L"跳转"},
    {"global", L"全局"}, {"local", L"局部"},
    {"frame (", L"帧 ("}, {"1fps or less", L"低于1帧/秒"},
};
}

const wchar_t* ChineseUiText(const char* english) {
    if (english != nullptr)
        for (const auto& entry : kLabels)
            if (std::strcmp(english, entry.en) == 0) return entry.zh;
    return nullptr;
}

int DrawWideUiGlyph(const wchar_t* text, HDC hdc, int size, int x, int y,
                    unsigned char r, unsigned char g, unsigned char b, int bold) {
    LOGFONTW lf{};
    // Keep the same logical size as the English renderer.  Position tuning is
    // handled at the call boundary so Chinese glyphs do not become smaller.
    lf.lfHeight = -size;
    lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    wcscpy_s(lf.lfFaceName, L"Microsoft YaHei UI");
    HFONT font = CreateFontIndirectW(&lf);
    HGDIOBJ previous = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(r, g, b));
    const int length = static_cast<int>(std::wcslen(text));
    TextOutW(hdc, x, y, text, length);
    SIZE extent{};
    GetTextExtentPoint32W(hdc, text, length, &extent);
    SelectObject(hdc, previous);
    DeleteObject(font);
    return extent.cx;
}

int DrawUiGlyph(MMDApp* app, const char* text, HDC hdc, int size, int x, int y,
                unsigned char r, unsigned char g, unsigned char b, int bold) {
    if (app->EnglishUI() == 2)
        if (const auto* zh = ChineseUiText(text))
            // The translated glyphs sit too low in the original text rows.
            // Preserve their size and horizontal anchor; raise the drawing
            // origin by three logical pixels to clear the bottom separator.
            return DrawWideUiGlyph(zh, hdc, size, x, y - 3, r, g, b, bold);
    return DrawGlyph(app, text, hdc, size, x, y, r, g, b, bold);
}

void SetUiControlText(MMDApp* app, HWND control, const char* english) {
    if (app->EnglishUI() == 2)
        if (const auto* zh = ChineseUiText(english)) {
            SetWindowTextW(control, zh);
            return;
        }
    SetWindowTextA(control, english);
}

LRESULT AddUiComboText(MMDApp* app, HWND combo, const char* english) {
    if (app->EnglishUI() == 2)
        if (const auto* zh = ChineseUiText(english))
            return SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(zh));
    return SendMessageA(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(english));
}
}
