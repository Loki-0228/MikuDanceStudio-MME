// ===========================================================================
// The one rule that decides which of a model's name fields reaches the UI.
// ===========================================================================
// MMDApp::EnglishUI() (x86 0xA0B4C / x64 0xA1B90, state field 658252) is
// *two*-valued in the original image: 0 = Japanese UI, 1 = English UI, and
// every original call site tests it as a boolean (`test al, al`), so a
// non-zero byte always meant "the English UI".  The port added a third value,
// 2 = Simplified Chinese (menu command 260 cycles 0 -> 1 -> 2), which is *not*
// English: the Chinese UI shows the names the model itself carries, exactly
// like the Japanese UI, and only translates the application's fixed vocabulary
// (ui_translation.cpp's dictionary + the zh column of LocalizeUI's table).
//
// `!= 0` is therefore the wrong test for every site that picks a *name*: under
// the Chinese UI it sent the bone lists to the English name fields while the
// morph lists (already using `== 1`) stayed on the model's own names - the
// reported "bone list English, facial list Japanese" split.  Name selection
// must go through UiUsesEnglishNames() so all lists agree.
#pragma once

namespace mikudancestudio {

constexpr unsigned char kUiJapanese = 0;  // 0xA0B4C == 0
constexpr unsigned char kUiEnglish = 1;   // 0xA0B4C == 1
constexpr unsigned char kUiChinese = 2;   // port-only third state (menu 260)

// True only for the English UI.  Bone, morph, display-frame, accessory and
// model names then come from the model's English fields (nameEn); every other
// language (Japanese and Chinese) shows the model's own names (name).
constexpr bool UiUsesEnglishNames(unsigned char englishUI) {
    return englishUI == kUiEnglish;
}

}  // namespace mikudancestudio
