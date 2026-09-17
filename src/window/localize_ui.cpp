// ===========================================================================
// VA 0x00441AD0 - LocalizeUI  (original: sub_441AD0, 0x13D3 bytes)
// ===========================================================================
// UI language pass + selector population, called from 0x0047A5B0 right
// after CreateWindowExA (controls were created by 0x00466D20 during
// WM_CREATE).  NOT the D3D init - reclassified from the earlier stub name.
//
//   1. combobox 433 (bone-mode): reset + add entries
//      "x/y/z axis move", "rotation" (+ "distance","view angle" in model
//      mode, this+760) + "all"; select index 3.
//   2. per-control texts for ids 400..567 (English literals below; the JP
//      branch re-sets the same strings the controls were created with -
//      restored from ui_controls.inc by id, single source of truth).
//   3. model-slot language flag sweep: for each of the 100 slots at
//      this+1920, *(slot+12740) = english; subobject(this+204)+596 too.
//   4. InvalidateRect + sub_42F1E0 + sub_40D070 [stubs]
//   5. camera-mode branch (this+760): combobox 434 entries
//      gravity/s shadow/light/camera (EN) else sub_49C850(slot).
//   6. selector comboboxes 436/474/449: save cur selection, reset, add
//      "camera/light/accessory" / "ground" / JP/EN third label, add every
//      loaded model's name (slot+8826 EN / slot+8776 JP), restore selection.
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "mikudancestudio/text_encoding.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "ui_controls.inc"

namespace mikudancestudio {
namespace {

struct TextEntry { int id; const char* en; const wchar_t* zh; };

// English texts in original call order (0x441AD0 EN branch).  The zh column
// is the port's Simplified-Chinese translation, cross-checked against both
// the EN literal and the JP creation string in ui_controls.inc.
constexpr TextEntry kEnTexts[] = {
    {400, "undo", L"\u64a4\u6d88"}, {401, "redo", L"\u91cd\u505a"},
    {418, "<", L"<"}, {419, ">", L">"}, {429, "current", L"\u5f53\u524d"},
    {416, "v-sel", L"\u9876\u70b9\u9009\u62e9"}, {423, "delete", L"\u5220\u9664"},
    {420, "copy", L"\u590d\u5236"}, {421, "paste", L"\u7c98\u8d34"},
    {422, "revers", L"\u53cd\u8f6c"}, {415, "range-sel", L"\u8303\u56f4\u9009\u62e9"},
    {424, "expand", L"\u5c55\u5f00"}, {432, "liner", L"\u7ebf\u6027"},
    {431, "paste", L"\u7c98\u8d34"}, {430, "copy", L"\u590d\u5236"},
    {435, "load", L"\u8bfb\u53d6"}, {437, "delete", L"\u5220\u9664"},
    {440, "shadow", L"\u9634\u5f71"}, {441, "Add-syn", L"\u6dfb\u52a0\u540c\u6b65"},
    {442, "OP", L"OP"}, {438, "register", L"\u767b\u8bb0"},
    {451, "reset", L"\u91cd\u7f6e"}, {446, "perspect", L"\u900f\u89c6"},
    {452, "register", L"\u767b\u8bb0"}, {454, "+", L"+"},
    {453, "-", L"-"}, {467, "reset", L"\u91cd\u7f6e"}, {468, "register", L"\u767b\u8bb0"},
    {562, "off", L"\u5173\u95ed"}, {563, "mode1", L"\u6a21\u5f0f1"},
    {564, "mode2", L"\u6a21\u5f0f2"}, {565, "register", L"\u767b\u8bb0"},
    {470, "+", L"+"}, {469, "-", L"-"}, {489, "+", L"+"}, {488, "-", L"-"},
    {472, "load", L"\u8bfb\u53d6"}, {473, "delete", L"\u5220\u9664"},
    {477, "Add-syn", L"\u6dfb\u52a0\u540c\u6b65"}, {487, "register", L"\u767b\u8bb0"},
    {402, "front", L"\u6b63\u9762"}, {403, "back", L"\u80cc\u9762"},
    {404, "top", L"\u4e0a\u9762"}, {405, "left", L"\u5de6\u9762"},
    {406, "right", L"\u53f3\u9762"}, {407, "btm", L"\u4e0b\u9762"},
    {408, "play", L"\u64ad\u653e"}, {535, "track", L"\u8f68\u8ff9"},
    {490, "select", L"\u9009\u62e9"}, {493, "rotate", L"\u65cb\u8f6c"},
    {492, "move", L"\u79fb\u52a8"}, {491, "BOX-sel", L"\u6846\u9009"},
    {494, "select all", L"\u5168\u9009"}, {501, "unregisted", L"\u672a\u767b\u8bb0"},
    {496, "copy", L"\u590d\u5236"}, {497, "paste", L"\u7c98\u8d34"},
    {498, "revers", L"\u53cd\u8f6c"}, {500, "register", L"\u767b\u8bb0"},
    {499, "physics", L"\u7269\u7406"}, {495, "reset", L"\u91cd\u7f6e"},
    {503, "+", L"+"}, {502, "-", L"-"}, {529, "+", L"+"}, {528, "-", L"-"},
    {567, "+", L"+"}, {566, "-", L"-"},
    {524, "register", L"\u767b\u8bb0"}, {525, "register", L"\u767b\u8bb0"},
    {527, "register", L"\u767b\u8bb0"}, {526, "register", L"\u767b\u8bb0"},
    {543, "dist", L"\u8ddd\u79bb"}, {551, "info", L"\u4fe1\u606f"},
    {552, "low pow", L"\u7701\u7535"}, {557, "axis", L"\u5750\u6807\u8f74"},
    {555, "set", L"\u8bbe\u7f6e"}, {556, "Fshadow", L"\u81ea\u9634\u5f71"},
};

HWND Dlg(MMDApp* app, int id) {
    return GetDlgItem(static_cast<HWND>(app->Hwnd()), id);
}

const wchar_t* JpControlText(int id) {
    // JP branch re-sets the creation strings; single source = ui_controls.inc
    // (regenerated dual-encoding table: wide-variant captions come back
    // verbatim, ANSI-variant captions are ASCII and widened in place, which
    // matches the original's SetWindowTextW re-set of the same strings).
    static wchar_t buf[64];
    for (const ui::ControlSpec& c : ui::kControls) {
        if (c.id != id)
            continue;
        if (c.wtext != nullptr)
            return c.wtext;
        int i = 0;
        for (; i < 63 && c.atext != nullptr && c.atext[i] != '\0'; ++i)
            buf[i] = static_cast<unsigned char>(c.atext[i]);
        buf[i] = L'\0';
        return buf;
    }
    return L"";
}

}  // namespace

void LocalizeUI(MMDApp* app) {
    RefreshMenuLanguage(app);
    auto& s = *app;
    const bool english = s.EnglishUI() == 1;                       // 658252
    const bool chinese = s.EnglishUI() == 2;
    const bool modelMode = s.state.optflag[0] != 0;  // 760

    HWND combo433 = Dlg(app, 433);
    SendMessageA(combo433, CB_RESETCONTENT, 0, 0);

    if (english) {
        for (const TextEntry& t : kEnTexts)
            SetWindowTextA(Dlg(app, t.id), t.en);
        SetWindowTextA(Dlg(app, 536), modelMode ? "To model" : "camera");
        SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"x axis move");
        SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"y axis move");
        SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"z axis move");
        SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"rotation");
        if (modelMode) {
            SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"distance");
            SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"view angle");
        }
        SendMessageA(combo433, CB_ADDSTRING, 0, (LPARAM)"all");
    } else if (chinese) {
        for (const TextEntry& t : kEnTexts)
            SetWindowTextW(Dlg(app, t.id), t.zh);
        SetWindowTextW(Dlg(app, 536), modelMode ? L"\u5230\u6a21\u578b" : L"\u76f8\u673a");
        SendMessageW(combo433, CB_ADDSTRING, 0, (LPARAM)L"X\u8f74\u79fb\u52a8");
        SendMessageW(combo433, CB_ADDSTRING, 0, (LPARAM)L"Y\u8f74\u79fb\u52a8");
        SendMessageW(combo433, CB_ADDSTRING, 0, (LPARAM)L"Z\u8f74\u79fb\u52a8");
        SendMessageW(combo433, CB_ADDSTRING, 0, (LPARAM)L"\u65cb\u8f6c");
        if (modelMode) {
            SendMessageW(combo433, CB_ADDSTRING, 0, (LPARAM)L"\u8ddd\u79bb");
            SendMessageW(combo433, CB_ADDSTRING, 0, (LPARAM)L"\u89c6\u91ce\u89d2");
        }
        SendMessageW(combo433, CB_ADDSTRING, 0, (LPARAM)L"\u5168\u90e8");
    } else {
        for (const TextEntry& t : kEnTexts)
            SetWindowTextW(Dlg(app, t.id), JpControlText(t.id));
        SetWindowTextW(Dlg(app, 536), JpControlText(536));
        // JP combo strings verified byte-exact against the push sequence
        // 0x442909..0x442991 of the original: Ｘ移動(0x52D3D4) Ｙ移動(0x52D3CC)
        // Ｚ移動(0x52D3C4) 回　転(0x52D470, full-width space) [距　離(0x52D3BC)
        // 視野角(0x52D3B4) in model mode] すべて(0x52D3AC).
        SendMessageW(combo433, CB_ADDSTRING, 0,
                     (LPARAM)L"\xff38\x79fb\x52d5");               // 0x52D3D4
        SendMessageW(combo433, CB_ADDSTRING, 0,
                     (LPARAM)L"\xff39\x79fb\x52d5");               // 0x52D3CC
        SendMessageW(combo433, CB_ADDSTRING, 0,
                     (LPARAM)L"\xff3a\x79fb\x52d5");               // 0x52D3C4
        SendMessageW(combo433, CB_ADDSTRING, 0,
                     (LPARAM)L"\x56de\x3000\x8ee2");               // 0x52D470
        if (modelMode) {
            SendMessageW(combo433, CB_ADDSTRING, 0,
                         (LPARAM)L"\x8ddd\x3000\x96e2");           // 0x52D3BC
            SendMessageW(combo433, CB_ADDSTRING, 0,
                         (LPARAM)L"\x8996\x91ce\x89d2");           // 0x52D3B4
        }
        SendMessageW(combo433, CB_ADDSTRING, 0,
                     (LPARAM)L"\x3059\x3079\x3066");               // 0x52D3AC
    }
    SendMessageA(combo433, CB_SETCURSEL, 3, 0);

    // model-slot language flag sweep (20 x 5 slots at this+1920); the x64
    // twin sub_7FF7CB438E0 sweeps all 255 slots (mov r8d, 0FFh ... dec r8
    // at 0x7FF7CB43A4E0, writing model+0x3560).
    for (int i = 0; i < kModelSlotCount; ++i) {
        unsigned char* slot = s.ModelSlot(i);
        if (slot != nullptr)
            mdl::Mdl(slot)->physicsFlags = static_cast<unsigned char>(english);
    }
    if (s.Audio() != nullptr)
        s.Audio()->englishUI = static_cast<unsigned char>(english);

    InvalidateRect(static_cast<HWND>(s.Hwnd()), nullptr, FALSE);
    PostLanguageSweep(app);                                        // 0x42F1E0
    PostLanguageSweep2(app);                                       // 0x40D070

    if (modelMode) {
        HWND combo434 = Dlg(app, 434);
        SendMessageA(combo434, CB_DELETESTRING, 0, 0);
        SendMessageA(combo434, CB_DELETESTRING, 0, 0);
        SendMessageA(combo434, CB_DELETESTRING, 0, 0);
        if (english) {
            SendMessageA(combo434, CB_INSERTSTRING, 0, (LPARAM)"gravity");
            SendMessageA(combo434, CB_INSERTSTRING, 0, (LPARAM)"s shadow");
            SendMessageA(combo434, CB_INSERTSTRING, 0, (LPARAM)"light");
            SendMessageA(combo434, CB_INSERTSTRING, 0, (LPARAM)"camera");
        } else if (chinese) {
            SendMessageW(combo434, CB_INSERTSTRING, 0, (LPARAM)L"\u91cd\u529b");
            SendMessageW(combo434, CB_INSERTSTRING, 0, (LPARAM)L"\u81ea\u9634\u5f71");
            SendMessageW(combo434, CB_INSERTSTRING, 0, (LPARAM)L"\u7167\u660e");
            SendMessageW(combo434, CB_INSERTSTRING, 0, (LPARAM)L"\u76f8\u673a");
        } else {
            // JP strings verified byte-exact: 重力(0x52D3A4) セルフ影(0x52D398)
            // 照明(0x52D390) カメラ(0x52B784).
            SendMessageW(combo434, CB_INSERTSTRING, 0,
                         (LPARAM)L"\x91cd\x529b");                 // 0x52D3A4
            SendMessageW(combo434, CB_INSERTSTRING, 0,
                         (LPARAM)L"\x30bb\x30eb\x30d5\x5f71");     // 0x52D398
            SendMessageW(combo434, CB_INSERTSTRING, 0,
                         (LPARAM)L"\x7167\x660e");                 // 0x52D390
            SendMessageW(combo434, CB_INSERTSTRING, 0,
                         (LPARAM)L"\x30ab\x30e1\x30e9");           // 0x52B784
        }
        SendMessageA(combo434, CB_SETCURSEL, 0, 0);
    } else {
        PostLoadInit(s.SelectedModel());                           // 0x49C850
    }

    // selector comboboxes 436/474/449: preserve selection across refill
    HWND combo436 = Dlg(app, 436), combo474 = Dlg(app, 474), combo449 = Dlg(app, 449);
    LRESULT sel436 = SendMessageA(combo436, CB_GETCURSEL, 0, 0);
    LRESULT sel474 = SendMessageA(combo474, CB_GETCURSEL, 0, 0);
    LRESULT sel449 = SendMessageA(combo449, CB_GETCURSEL, 0, 0);
    SendMessageA(combo436, CB_RESETCONTENT, 0, 0);
    SendMessageA(combo474, CB_RESETCONTENT, 0, 0);
    SendMessageA(combo449, CB_RESETCONTENT, 0, 0);
    if (english) {
        SendMessageA(combo436, CB_ADDSTRING, 0, (LPARAM)"camera/light/accessory");  // 0x52D2AC
        SendMessageA(combo474, CB_ADDSTRING, 0, (LPARAM)"ground");                 // 0x52D2A4
        SendMessageA(combo449, CB_ADDSTRING, 0, (LPARAM)"non");                    // 0x52D38C
    } else if (chinese) {
        SendMessageW(combo436, CB_ADDSTRING, 0, (LPARAM)L"\u76f8\u673a/\u7167\u660e/\u914d\u4ef6");
        SendMessageW(combo474, CB_ADDSTRING, 0, (LPARAM)L"\u5730\u9762");
        SendMessageW(combo449, CB_ADDSTRING, 0, (LPARAM)L"\u65e0");
    } else {
        // Verified byte-exact: 0x52D370 ｶﾒﾗ･照明･ｱｸｾｻﾘ / 0x52D368 地面 /
        // 0x52D360 なし (push sequence 0x442CE0..0x442D31).
        SendMessageW(combo436, CB_ADDSTRING, 0,
                     (LPARAM)L"\xff76\xff92\xff97\xff65\x7167\x660e"
                              L"\xff65\xff71\xff78\xff7e\xff7b\xff98");  // 0x52D370
        SendMessageW(combo474, CB_ADDSTRING, 0,
                     (LPARAM)L"\x5730\x9762");                           // 0x52D368
        SendMessageW(combo449, CB_ADDSTRING, 0,
                     (LPARAM)L"\x306a\x3057");                           // 0x52D360
    }

    // add every loaded model's name to all three selectors; x64 twin
    // sub_7FF7CB438E70 runs the comboSelIndex (model+0x3108) slot scan and
    // the order walk to 255 (cmp edx, 0FFh at 0x7FF7CB43A96C / cmp ebp,
    // 0FFh at 0x7FF7CB43AA8D) - same family as model_edge_dialog.cpp.
    for (int order = 1; order < kModelSlotCount; ++order) {
        int slotIdx = -1;
        for (int i = 0; i < kModelSlotCount; ++i) {
            unsigned char* slot = s.ModelSlot(i);
            if (slot != nullptr &&
                mikudancestudio::mdl::Mdl(slot)->comboSelIndex == order) {
                slotIdx = i;
                break;
            }
        }
        if (slotIdx < 0)
            continue;
        unsigned char* slot = s.ModelSlot(slotIdx);
        const auto label = text_encoding::ModelName(*mdl::Mdl(slot), english);
        const LPARAM name = reinterpret_cast<LPARAM>(label.c_str());
        SendMessageW(combo436, CB_ADDSTRING, 0, name);
        SendMessageW(combo474, CB_ADDSTRING, 0, name);
        SendMessageW(combo449, CB_ADDSTRING, 0, name);
    }

    SendMessageA(combo436, CB_SETCURSEL, sel436, 0);
    SendMessageA(combo474, CB_SETCURSEL, sel474, 0);
    SendMessageA(combo449, CB_SETCURSEL, sel449, 0);
}

}  // namespace mikudancestudio
