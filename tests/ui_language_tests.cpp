// ===========================================================================
// User-acceptance regressions for the UI language of model names (defect 3)
// and for the Japanese captions of the bone-operation panel (defect 4).
// ===========================================================================
// Field report (defect 3): with the Chinese UI the bone list showed English
// bone names while the facial (morph) list showed the model's own Japanese
// names; the mix drifted with the order in which models and languages were
// loaded.
// Root cause: name selection tested the same UI-language byte (0 = Japanese,
// 1 = English, 2 = Chinese) two different ways - `EnglishUI() == 1` in the
// morph combos, the panel tree and the palette, but `EnglishUI() != 0` in the
// bone-combo paths (RefillBoneRegisterCombo, RepopulateBoneCombo, the select
// dialog, the physics dialog).  The original's byte is two-valued, so `!= 0`
// meant "English UI"; the port's third value 2 = Chinese is *not* English, so
// every `!= 0` bone list flipped to the English name fields under the Chinese
// UI while the morph lists stayed on the model's own names.
//
// Field report (defect 4): bone-operation panel buttons had no (or English)
// captions under the Japanese UI.  Controls 494 ("select all") and 501
// ("select unregistered") have an empty Japanese caption in the generated
// creation table (src/window/ui_controls.inc) and previously fell back to the
// English labels, and 562 ("nothing"/off) carried the byte-shifted mis-read
// "q_j0W0" of its UTF-16 caption.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/panel_controls.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/ui_language.hpp"
#include "../src/window/ui_controls.inc"  // generated creation table (caption sweep)

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

namespace mikudancestudio {
// LocalizeUI's caption lookup and localized-id list (src/window/localize_ui.cpp);
// not exported through a header because only the UI pass and these tests use
// them.
std::wstring UiControlCaption(int id, unsigned char language);
int UiLocalizedControlCount();
int UiLocalizedControlId(int index);
}  // namespace mikudancestudio

using namespace mikudancestudio;

static void Check(bool condition, const char* description) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", description);
        std::exit(1);
    }
}

namespace {

const char* LanguageName(unsigned char language) {
    return language == kUiJapanese ? "Japanese"
         : language == kUiEnglish  ? "English"
                                   : "Chinese";
}

// Combo item text.  Combo 443/450 are ANSI windows (CreateWindowExA in the
// creation table), the four morph combos are Unicode windows; each side is read
// with the message that matches the window, exactly like the production code.
std::string ItemTextA(HWND combo, int index) {
    char buffer[256]{};
    SendMessageA(combo, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(buffer));
    return buffer;
}

std::wstring ItemTextW(HWND combo, int index) {
    wchar_t buffer[256]{};
    SendMessageW(combo, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(buffer));
    return buffer;
}

HWND MakeCombo(HWND parent, int id, bool wide) {
    return wide ? CreateWindowExW(0, L"COMBOBOX", L"",
                                  WS_CHILD | CBS_DROPDOWNLIST, 0, 0, 200, 200,
                                  parent,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                  GetModuleHandleW(nullptr), nullptr)
                : CreateWindowExA(0, "COMBOBOX", "",
                                  WS_CHILD | CBS_DROPDOWNLIST, 0, 0, 200, 200,
                                  parent,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                  GetModuleHandleW(nullptr), nullptr);
}

// ---------------------------------------------------------------------------
// Defect 3: one language for every list.
//
// The bone names are deliberately ASCII so the assertion is independent of the
// host ANSI code page (the ANSI combos translate SJIS names through CP_ACP);
// the morph names are wide, which is how PMX supplies them.
// ---------------------------------------------------------------------------
void NameLanguageRegression() {
    // The rule itself: Chinese is not English.
    Check(!UiUsesEnglishNames(kUiJapanese), "Japanese UI uses the model's own names");
    Check(UiUsesEnglishNames(kUiEnglish), "English UI uses the English names");
    Check(!UiUsesEnglishNames(kUiChinese),
          "Chinese UI is not English (regression: `!= 0` sent bone lists to nameEn)");

    HWND parent = CreateWindowExW(0, L"STATIC", L"ui language test", WS_OVERLAPPED,
                                  0, 0, 320, 240, nullptr, nullptr,
                                  GetModuleHandleW(nullptr), nullptr);
    Check(parent != nullptr, "create hidden control owner");

    HWND ikCombo = MakeCombo(parent, panel::kIkChainCombo, false);              // 443
    HWND boneCombo = MakeCombo(parent, panel::kBoneRegisterCombo, false);       // 450
    HWND morphCombos[4] = {
        MakeCombo(parent, panel::kMorphCombo0, true),                           // 504
        MakeCombo(parent, panel::kMorphCombo1, true),                           // 509
        MakeCombo(parent, panel::kMorphCombo2, true),                           // 514
        MakeCombo(parent, panel::kMorphCombo3, true),                           // 519
    };
    Check(ikCombo && boneCombo && morphCombos[0] && morphCombos[1] &&
              morphCombos[2] && morphCombos[3],
          "create the language test combos");

    std::array<mdl::BoneRecord, 2> bones{};
    std::strcpy(bones[0].name, "bone-jp");
    std::strcpy(bones[0].nameEn, "bone-en");
    std::strcpy(bones[1].name, "ik-jp");
    std::strcpy(bones[1].nameEn, "ik-en");
    bones[1].type = mdl::BoneType::Ik;

    std::array<mdl::MorphRecord, 4> morphs{};
    const wchar_t* jpMorph[4] = {L"morph-jp0", L"morph-jp1", L"morph-jp2", L"morph-jp3"};
    const wchar_t* enMorph[4] = {L"morph-en0", L"morph-en1", L"morph-en2", L"morph-en3"};
    const mdl::MorphPanel panelOf[4] = {
        mdl::MorphPanel::eyebrow, mdl::MorphPanel::eye,
        mdl::MorphPanel::mouth, mdl::MorphPanel::other};
    for (int lane = 0; lane < 4; ++lane) {
        morphs[lane].jpText = const_cast<wchar_t*>(jpMorph[lane]);
        morphs[lane].enText = const_cast<wchar_t*>(enMorph[lane]);
        morphs[lane].panel = panelOf[lane];
    }

    // The IK chain names its own bone (combo 443) - bone 0 - while the bone
    // register combo (450) lists every selectable bone, so both lists start
    // with the same name and can be compared directly.
    mdl::IkChain chain{};
    chain.boneIndex = 0;

    auto model = std::make_unique<mdl::ModelRecord>();
    auto app = std::make_unique<MMDApp>();
    MMDApp* previousBlock = g_Block;
    g_Block = app.get();

    app->Hwnd() = parent;
    app->ModelSlot(0) = reinterpret_cast<unsigned char*>(model.get());
    app->SetSelectedModelSlot(0);
    model->hwnd = parent;
    model->boneTable = bones.data();
    model->boneCount = static_cast<std::uint32_t>(bones.size());
    model->morphs = morphs.data();
    model->morphCount = static_cast<std::uint32_t>(morphs.size());
    model->ikChains = &chain;
    model->ikChainCount = 1;
    model->displayRootBone = 0;
    for (int lane = 0; lane < 4; ++lane)
        model->selectedMorphs[lane] = lane;

    const unsigned char languages[] = {kUiJapanese, kUiEnglish, kUiChinese};
    for (unsigned char language : languages) {
        app->EnglishUI() = language;
        const bool englishNames = UiUsesEnglishNames(language);
        const std::string expectedBone = englishNames ? "bone-en" : "bone-jp";
        const std::wstring expectedMorph = englishNames ? L"morph-en1" : L"morph-jp1";

        PostLoadInit(reinterpret_cast<unsigned char*>(model.get()));
        RefillBoneRegisterCombo(app.get(), 0);

        // IK chain combo 443 and the model/camera follow-bone list (450).
        Check(ItemTextA(ikCombo, 0) == expectedBone,
              "IK bone combo follows the UI language");
        Check(ItemTextA(boneCombo, 0) == expectedBone,
              "follow-bone list follows the UI language (RefillBoneRegisterCombo)");
        Check(ItemTextA(boneCombo, 0) == ItemTextA(ikCombo, 0),
              "the two bone lists must agree");

        // The facial (morph) list is the reference: same language or not.
        const std::wstring morphName = ItemTextW(morphCombos[1], 0);
        Check(morphName == expectedMorph, "facial list follows the UI language");
        const bool bonesEnglish = ItemTextA(boneCombo, 0) == "bone-en";
        const bool morphsEnglish = morphName == L"morph-en1";
        Check(bonesEnglish == morphsEnglish,
              "bone list and facial list must use one language");
        std::printf("PASS %s UI: bone list \"%s\", facial list \"%ls\"\n",
                    LanguageName(language), ItemTextA(boneCombo, 0).c_str(),
                    morphName.c_str());
    }

    // The reported case, from a language switch instead of a fresh load: a
    // stale per-model language byte must not move the bone list either.
    app->EnglishUI() = kUiEnglish;
    PostLoadInit(reinterpret_cast<unsigned char*>(model.get()));
    RefillBoneRegisterCombo(app.get(), 0);
    app->EnglishUI() = kUiChinese;
    PostLoadInit(reinterpret_cast<unsigned char*>(model.get()));
    RefillBoneRegisterCombo(app.get(), 0);
    Check(ItemTextA(boneCombo, 0) == "bone-jp" &&
              ItemTextW(morphCombos[1], 0) == L"morph-jp1",
          "EN -> CHS switch keeps bone and facial lists in one language");
    std::puts("PASS English -> Chinese switch keeps one name language");

    g_Block = previousBlock;
    DestroyWindow(parent);
}

// ---------------------------------------------------------------------------
// Defect 4: no bone-operation panel caption may be empty in any language, and
// the three captions the generated table lost keep their originals.
//
// The windowed equivalent (creating all 168 controls and calling LocalizeUI)
// cannot run headless: LocalizeUI ends in PostLanguageSweep, which walks the
// loaded model's display groups and the panel back-store, neither of which
// exists without a device and an open model.  UiControlCaption() is that pass's
// caption lookup factored out, so asserting on it pins the same strings.
// ---------------------------------------------------------------------------
void CaptionRegression() {
    // 494 全て選択 / 501 未登録選 (bone-operation panel) and 562 なし
    // (self-shadow mode "off") - recovered originals, see localize_ui.cpp.
    Check(UiControlCaption(494, kUiJapanese) == std::wstring(L"\u5168\u3066\u9078\u629e"),
          "494 Japanese caption is 全て選択 (not the English fallback)");
    Check(UiControlCaption(501, kUiJapanese) == std::wstring(L"\u672a\u767b\u9332\u9078"),
          "501 Japanese caption is 未登録選 (not the English fallback)");
    Check(UiControlCaption(562, kUiJapanese) == std::wstring(L"\u306a\u3057"),
          "562 Japanese caption is なし (not the byte-shifted q_j0W0)");
    Check(UiControlCaption(494, kUiEnglish) == std::wstring(L"select all") &&
              UiControlCaption(501, kUiEnglish) == std::wstring(L"unregisted") &&
              UiControlCaption(562, kUiEnglish) == std::wstring(L"off"),
          "the English captions are unchanged");
    Check(UiControlCaption(494, kUiChinese) == std::wstring(L"\u5168\u9009") &&
              UiControlCaption(501, kUiChinese) == std::wstring(L"\u672a\u767b\u8bb0"),
          "the Chinese captions are unchanged");
    std::puts("PASS bone-operation panel captions 494/501 and shadow toggle 562");

    // Every control of the bone-operation panel (option band 4 of
    // HandleWindowSize: the select/rotate/move group, the keyframe buttons and
    // the two column toggles) carries a caption in all three languages.
    const int bonePanel[] = {
        panel::kBoneSelectRadio, 491, panel::kBoneMoveRadio, panel::kBoneRotateRadio,
        494, 495, 496, panel::kBonePasteButton, panel::kBoneReversePasteButton,
        panel::kPhysicsCheckbox, panel::kRegisterFrameButton, 501, 502, 503};
    for (int id : bonePanel) {
        for (unsigned char language : {kUiJapanese, kUiEnglish, kUiChinese}) {
            if (UiControlCaption(id, language).empty()) {
                std::fprintf(stderr, "FAIL: bone panel control %d has no %s caption\n",
                             id, LanguageName(language));
                std::exit(1);
            }
        }
    }
    std::puts("PASS every bone-operation panel control has a caption in JP/EN/CHS");

    // No control LocalizeUI rewrites may end up blank in any language: the JP
    // pass used to write L"" over a control whose caption was missing from both
    // the creation table and the English table.
    const int localized = UiLocalizedControlCount();
    Check(localized > 0, "the localized control table is populated");
    for (int index = 0; index < localized; ++index) {
        const int id = UiLocalizedControlId(index);
        Check(id > 0, "localized control id");
        for (unsigned char language : {kUiJapanese, kUiEnglish, kUiChinese}) {
            if (UiControlCaption(id, language).empty()) {
                std::fprintf(stderr, "FAIL: localized control %d has no %s caption\n",
                             id, LanguageName(language));
                std::exit(1);
            }
        }
    }
    std::puts("PASS no localized control loses its caption");

    // Sweep the generated creation table: any BUTTON that carries an English
    // caption must carry a Japanese one too (icon-only toggles carry neither).
    int captionedButtons = 0;
    for (const ui::ControlSpec& control : ui::kControls) {
        if (control.wcls == nullptr || std::wcscmp(control.wcls, L"BUTTON") != 0)
            continue;
        if (UiControlCaption(control.id, kUiEnglish).empty())
            continue;
        ++captionedButtons;
        if (UiControlCaption(control.id, kUiJapanese).empty()) {
            std::fprintf(stderr, "FAIL: button %d has an English caption but no Japanese one\n",
                         control.id);
            std::exit(1);
        }
    }
    Check(captionedButtons > 0, "the creation table advertises captioned buttons");
    std::printf("PASS %d captioned buttons all keep a Japanese caption\n",
                captionedButtons);
}

}  // namespace

int main() {
    NameLanguageRegression();
    CaptionRegression();
    std::puts("UI language regressions passed");
    return 0;
}
