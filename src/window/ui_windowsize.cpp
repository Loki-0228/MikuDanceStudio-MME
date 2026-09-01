// ===========================================================================
// VA 0x00443300 - HandleWindowSize  (original: sub_443300, ~0x19B1 bytes)
// ===========================================================================
// WM_SIZE layout engine.  Called from MainWndProc (0x4C3A10 WM_SIZE), the
// command dispatch (0x47E8A0) and the scene loaders (0x458F80 / 0x460430 /
// 0x460B30).  After a resize the 168 child controls of the main window are
// fully repositioned:
//
//   1. 0x44331B  clamp: GetWindowRect; if width  > 2560 ->
//      SetWindowPos(cx=2560, SWP_NOZORDER); if height > 1920 ->
//      SetWindowPos(cy=1920, SWP_NOZORDER).  Position is preserved.
//   2. 0x4433A4  sidebar width (this+0xA06C8): GetClientRect; when the
//      accessory-column gate (this+0xA0D38) is set the sidebar is client
//      width - 3 and the label sweep PostLanguageSweep (0x42F1E0) runs;
//      otherwise sidebar = max(250, (int)(ratio @0xA4428 * client width))
//      and PanelPaint (0x414610) runs when the sidebar grew.
//   3. 0x4433F5  control layout, three coordinate families:
//      - left tool row (ids 400..559): X = sidebar/2 + k (or sidebar - k,
//        fixed 15 for 558), constant Y/W/H.
//      - main grid (ids 427..453): Y = clientBottom - k (stretched to the
//        window bottom), X = sidebar-derived or fixed; several controls get
//        W/H = clientBottom - k (e.g. 427 H, 428 W, 434/436/443 big panels).
//      - option bands (ids 455..535): X = const - v52 / const - v110 where
//        the two bases accumulate conditional deltas from the UI option
//        flag bytes this+0x2F8..0x2FE (kByteOptflag0..6), see the body.
//   4. 0x444B3B  visibility re-toggle: five id ranges (455..469, 471..488,
//      504..528, 402..414, 531..535) get ShowWindow(SW_HIDE) followed by
//      ShowWindow(SW_SHOW) when IsWindowVisible.
//   5. 0x444C9D  refresh tail: Sub42C810 (0x42C810) then Sub40CAC0
//      (0x40CAC0).
//
// All MoveWindow calls pass bRepaint = TRUE; every GetDlgItem handle is
// consumed by the immediately following MoveWindow, so the per-control
// sequence is rendered through one file-static helper (control order and
// parameters are unchanged).
//
// Reference: ../translated/MikuMikuDance/fcn_00443300.cpp is a partial
// auto-generated dump (coordinates/IDs mislabelled past 0x44365E); this
// port follows the live IDA disassembly/pseudocode of VA 0x00443300.
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/ported_funcs.hpp"

namespace mikudancestudio {

namespace {

// GetDlgItem + MoveWindow pair as in the original (0x44341E..0x443427 etc.).
void MoveCtrl(HWND parent, int id, int x, int y, int w, int h) {
    MoveWindow(GetDlgItem(parent, id), x, y, w, h, TRUE);
}

// Visibility re-toggle loop body (0x444B4A..0x444B6F and the four siblings).
void ToggleVisible(HWND parent, int firstId, int lastId) {
    for (int id = firstId; id <= lastId; ++id) {
        HWND ctrl = GetDlgItem(parent, id);
        if (IsWindowVisible(ctrl)) {
            ShowWindow(ctrl, SW_HIDE);
            ShowWindow(ctrl, SW_SHOW);
        }
    }
}

}  // namespace

void PanelPaint(MMDApp* app);  // VA 0x00414610 (defined in ui_panel_paint.cpp)
void Sub40CAC0(MMDApp* app);   // VA 0x0040CAC0 (stub in unported/stubs.cpp)

void HandleWindowSize(MMDApp* app) {
    auto& s = *app;
    const HWND hwnd = static_cast<HWND>(app->Hwnd());  // 657080 (0xA06B8)

    // --- clamp the window to 2560 x 1920 (0x44331B..0x443391) --------------
    RECT wr;
    GetWindowRect(hwnd, &wr);
    if (wr.right - wr.left > 2560)
        SetWindowPos(hwnd, nullptr, wr.left, wr.top, 2560,
                     wr.bottom - wr.top, SWP_NOZORDER);
    GetWindowRect(hwnd, &wr);
    if (wr.bottom - wr.top > 1920)
        SetWindowPos(hwnd, nullptr, wr.left, wr.top, wr.right - wr.left,
                     1920, SWP_NOZORDER);

    // --- sidebar width (0x4433A4..0x4433F2) --------------------------------
    RECT cr1;
    GetClientRect(hwnd, &cr1);  // var_10 - only .right feeds the sidebar
    std::int32_t& sidebar = s.SidebarWidth();
    if (s.FloatingWindow() != nullptr) {
        sidebar = cr1.right - 3;                               // 0x4433B8
        PostLanguageSweep(app);                                // 0x4433BE
    } else {
        std::int32_t v2 = static_cast<std::int32_t>(
            static_cast<double>(s.SidebarRatio()) *
            cr1.right);                                        // 0x4433CF
        if (v2 < 250) v2 = 250;                                // 0x4433D9
        const bool grew = sidebar < v2;                        // 0x4433E0
        sidebar = v2;                                          // 0x4433E6
        if (grew) PanelPaint(app);                             // 0x4433EC
    }
    const std::int32_t side = sidebar;
    RECT cb;                                                   // var_30
    GetClientRect(hwnd, &cb);                                  // 0x443563
    const std::int32_t bottom = cb.bottom;

    // --- left tool row (0x4433F5..0x443555) --------------------------------
    MoveCtrl(hwnd, 400, side / 2 - 80, 21, 70, 24);
    MoveCtrl(hwnd, 401, side / 2 + 10, 21, 70, 24);
    MoveCtrl(hwnd, 418, side / 2 - 49, 67, 20, 24);
    MoveCtrl(hwnd, 419, side / 2 + 29, 67, 20, 24);
    MoveCtrl(hwnd, 533, side / 2 - 79, 69, 20, 22);
    MoveCtrl(hwnd, 532, side / 2 + 59, 69, 20, 22);
    MoveCtrl(hwnd, 417, side / 2 - 27, 66, 54, 24);
    MoveCtrl(hwnd, 559, side - 30, 69, 20, 22);
    MoveCtrl(hwnd, 558, 15, 69, 20, 22);

    // --- main grid, anchored to the client bottom (0x443563..0x443B00) -----
    MoveCtrl(hwnd, 427, side - 19, 162, 16, bottom - 410);
    MoveCtrl(hwnd, 428, 97, bottom - 247, side - 116, 16);
    MoveCtrl(hwnd, 429, 8, bottom - 247, 80, 18);
    MoveCtrl(hwnd, 416, side / 2 + 15, bottom - 224, 50, 18);
    MoveCtrl(hwnd, 423, side / 2 + 70, bottom - 224, 50, 18);
    MoveCtrl(hwnd, 420, side / 2 - 119, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 421, side / 2 - 76, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 422, side / 2 - 33, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 425, side / 2 - 3, bottom - 201, 50, 14);
    MoveCtrl(hwnd, 426, side / 2 + 69, bottom - 201, 50, 14);
    MoveCtrl(hwnd, 434, side / 2 - 118, bottom - 193, 100, 400);
    MoveCtrl(hwnd, 415, side / 2 - 4, bottom - 181, 60, 18);
    MoveCtrl(hwnd, 424, side / 2 + 60, bottom - 181, 60, 18);
    MoveCtrl(hwnd, 432, 144, bottom - 26, 63, 18);
    MoveCtrl(hwnd, 431, 144, bottom - 52, 63, 18);
    MoveCtrl(hwnd, 430, 144, bottom - 78, 63, 18);
    MoveCtrl(hwnd, 433, 144, bottom - 135, 63, 120);
    MoveCtrl(hwnd, 530, 144, bottom - 105, 13, 13);
    MoveCtrl(hwnd, 436, 225, bottom - 138, 125, 200);
    MoveCtrl(hwnd, 435, 224, bottom - 112, 60, 21);
    MoveCtrl(hwnd, 437, 291, bottom - 112, 60, 21);
    MoveCtrl(hwnd, 438, 291, bottom - 35, 60, 28);
    MoveCtrl(hwnd, 439, 225, bottom - 84, 13, 13);
    MoveCtrl(hwnd, 440, 267, bottom - 86, 40, 18);
    MoveCtrl(hwnd, 441, 310, bottom - 86, 41, 18);
    MoveCtrl(hwnd, 442, 328, bottom - 62, 22, 22);
    MoveCtrl(hwnd, 443, 225, bottom - 61, 100, 100);
    MoveCtrl(hwnd, 444, 227, bottom - 36, 13, 13);
    MoveCtrl(hwnd, 445, 227, bottom - 19, 13, 13);
    MoveCtrl(hwnd, 451, 368, bottom - 139, 70, 23);
    MoveCtrl(hwnd, 446, 443, bottom - 139, 65, 23);
    MoveCtrl(hwnd, 448, 477, bottom - 111, 30, 14);
    MoveCtrl(hwnd, 447, 363, bottom - 95, 151, 20);
    MoveCtrl(hwnd, 452, 463, bottom - 40, 45, 33);
    MoveCtrl(hwnd, 449, 368, bottom - 53, 90, 100);
    MoveCtrl(hwnd, 450, 368, bottom - 28, 90, 300);
    MoveCtrl(hwnd, 454, 363, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 453, 363, bottom - 155, 14, 14);

    // --- option band 1: base v52 (0x443B02..0x443D9E) -----------------------
    // v52 = -18, or 117 while bone mode is off (optflag0 set) and the
    // expand toggle optflag1 is clear.
    const std::uint8_t opt0 = s.CameraMode();     // 760
    const std::uint8_t opt1 = s.UiOptionFlag(1);  // 761
    const std::uint8_t opt2 = s.UiOptionFlag(2);  // 762
    const std::uint8_t opt3 = s.UiOptionFlag(3);  // 763
    const std::uint8_t opt4 = s.UiOptionFlag(4);  // 764
    const std::uint8_t opt5 = s.UiOptionFlag(5);  // 765
    const std::uint8_t opt6 = s.UiOptionFlag(6);  // 766

    std::int32_t v52 = -18;
    if (opt0 != 0 && opt1 == 0) v52 = 117;                   // 0x443B1A
    MoveCtrl(hwnd, 455, 514 - v52, bottom - 140, 136, 18);
    MoveCtrl(hwnd, 461, 650 - v52, bottom - 140, 23, 14);
    MoveCtrl(hwnd, 456, 514 - v52, bottom - 122, 136, 18);
    MoveCtrl(hwnd, 462, 650 - v52, bottom - 122, 23, 14);
    MoveCtrl(hwnd, 457, 514 - v52, bottom - 104, 136, 18);
    MoveCtrl(hwnd, 463, 650 - v52, bottom - 104, 23, 14);
    MoveCtrl(hwnd, 458, 514 - v52, bottom - 84, 132, 20);
    MoveCtrl(hwnd, 464, 646 - v52, bottom - 84, 27, 14);
    MoveCtrl(hwnd, 459, 514 - v52, bottom - 66, 132, 20);
    MoveCtrl(hwnd, 465, 646 - v52, bottom - 66, 27, 14);
    MoveCtrl(hwnd, 460, 514 - v52, bottom - 48, 132, 20);
    MoveCtrl(hwnd, 466, 646 - v52, bottom - 48, 27, 14);
    MoveCtrl(hwnd, 468, 590 - v52, bottom - 26, 75, 20);
    MoveCtrl(hwnd, 467, 524 - v52, bottom - 26, 50, 20);
    MoveCtrl(hwnd, 470, 501 - v52, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 469, 501 - v52, bottom - 155, 14, 14);

    // --- option band 2: v52 += 162 with expand toggle optflag2 (0x443DB3) --
    if (opt0 != 0 && opt2 == 0) v52 += 162;
    MoveCtrl(hwnd, 567, 684 - v52, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 566, 684 - v52, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 562, 688 - v52, bottom - 135, 54, 22);
    MoveCtrl(hwnd, 563, 745 - v52, bottom - 135, 54, 22);
    MoveCtrl(hwnd, 564, 802 - v52, bottom - 135, 54, 22);
    MoveCtrl(hwnd, 560, 683 - v52, bottom - 80, 176, 25);
    MoveCtrl(hwnd, 561, 820 - v52, bottom - 100, 30, 14);
    MoveCtrl(hwnd, 565, 745 - v52, bottom - 36, 54, 30);

    // --- option band 3: v52 += 160 with expand toggle optflag6 (0x443F19) --
    if (opt0 != 0 && opt6 == 0) v52 += 160;
    MoveCtrl(hwnd, 489, 864 - v52, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 488, 864 - v52, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 471, 868 - v52, bottom - 140, 125, 150);
    MoveCtrl(hwnd, 472, 867 - v52, bottom - 116, 60, 18);
    MoveCtrl(hwnd, 473, 934 - v52, bottom - 116, 60, 18);
    MoveCtrl(hwnd, 476, 1000 - v52, bottom - 142, 13, 13);
    MoveCtrl(hwnd, 486, 1000 - v52, bottom - 127, 13, 13);
    MoveCtrl(hwnd, 477, 999 - v52, bottom - 113, 53, 17);
    MoveCtrl(hwnd, 474, 868 - v52, bottom - 95, 90, 100);
    MoveCtrl(hwnd, 475, 962 - v52, bottom - 95, 90, 300);
    MoveCtrl(hwnd, 478, 877 - v52, bottom - 70, 48, 14);
    MoveCtrl(hwnd, 479, 940 - v52, bottom - 70, 48, 14);
    MoveCtrl(hwnd, 480, 1003 - v52, bottom - 70, 48, 14);
    MoveCtrl(hwnd, 481, 877 - v52, bottom - 50, 48, 14);
    MoveCtrl(hwnd, 482, 940 - v52, bottom - 50, 48, 14);
    MoveCtrl(hwnd, 483, 1003 - v52, bottom - 50, 48, 14);
    MoveCtrl(hwnd, 484, 877 - v52, bottom - 30, 48, 14);
    MoveCtrl(hwnd, 485, 940 - v52, bottom - 30, 48, 14);
    MoveCtrl(hwnd, 487, 995 - v52, bottom - 32, 58, 26);

    // --- option band 4: v52 += 178 with expand toggle optflag3 (0x44422B);
    // band 4 controls use fixed X -------------------------------------------
    if (opt0 != 0 && opt3 == 0) v52 += 178;
    MoveCtrl(hwnd, 503, 363, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 502, 363, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 490, 366, bottom - 140, 60, 28);
    MoveCtrl(hwnd, 493, 430, bottom - 140, 60, 28);
    MoveCtrl(hwnd, 492, 494, bottom - 140, 60, 28);
    MoveCtrl(hwnd, 491, 366, bottom - 105, 60, 24);
    MoveCtrl(hwnd, 494, 430, bottom - 105, 60, 24);
    MoveCtrl(hwnd, 501, 494, bottom - 105, 60, 24);
    MoveCtrl(hwnd, 496, 366, bottom - 70, 60, 24);
    MoveCtrl(hwnd, 497, 430, bottom - 70, 60, 24);
    MoveCtrl(hwnd, 498, 494, bottom - 70, 60, 24);
    MoveCtrl(hwnd, 499, 500, bottom - 36, 48, 28);
    MoveCtrl(hwnd, 495, 430, bottom - 32, 60, 24);
    MoveCtrl(hwnd, 500, 366, bottom - 36, 60, 28);

    // --- option band 5: second base v110 (0x44444C..0x4448BE) --------------
    // optflag4/optflag0 both clear -> v52 += 179; then v110 = v52 - 180
    // (bone mode on) or v52 + 53; optflag5/optflag0 both clear -> v110 +=
    // 251; optflag0 clear -> v110 += 13.
    if (opt4 == 0 && opt0 == 0) v52 += 179;                   // 0x444453
    std::int32_t v110 = opt0 != 0 ? v52 - 180 : v52 + 53;     // 0x44445D
    MoveCtrl(hwnd, 529, 598 - v110, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 528, 598 - v110, bottom - 155, 14, 14);
    MoveCtrl(hwnd, 509, 618 - v110, bottom - 120, 92, 500);
    MoveCtrl(hwnd, 512, 602 - v110, bottom - 121, 16, 22);
    MoveCtrl(hwnd, 513, 710 - v110, bottom - 121, 16, 22);
    MoveCtrl(hwnd, 510, 597 - v110, bottom - 100, 135, 25);
    MoveCtrl(hwnd, 511, 642 - v110, bottom - 138, 28, 14);
    MoveCtrl(hwnd, 504, 618 - v110, bottom - 50, 92, 500);
    MoveCtrl(hwnd, 507, 602 - v110, bottom - 51, 16, 22);
    MoveCtrl(hwnd, 508, 710 - v110, bottom - 51, 16, 22);
    MoveCtrl(hwnd, 505, 597 - v110, bottom - 30, 135, 25);
    MoveCtrl(hwnd, 506, 642 - v110, bottom - 68, 28, 14);
    MoveCtrl(hwnd, 514, 749 - v110, bottom - 120, 92, 500);
    MoveCtrl(hwnd, 517, 733 - v110, bottom - 121, 16, 22);
    MoveCtrl(hwnd, 518, 841 - v110, bottom - 121, 16, 22);
    MoveCtrl(hwnd, 515, 729 - v110, bottom - 100, 135, 25);
    MoveCtrl(hwnd, 516, 773 - v110, bottom - 138, 28, 14);
    MoveCtrl(hwnd, 519, 749 - v110, bottom - 50, 92, 500);
    MoveCtrl(hwnd, 522, 733 - v110, bottom - 51, 16, 22);
    MoveCtrl(hwnd, 523, 841 - v110, bottom - 51, 16, 22);
    MoveCtrl(hwnd, 520, 729 - v110, bottom - 30, 135, 25);
    MoveCtrl(hwnd, 521, 773 - v110, bottom - 68, 28, 14);
    MoveCtrl(hwnd, 524, 675 - v110, bottom - 141, 50, 19);
    MoveCtrl(hwnd, 525, 675 - v110, bottom - 71, 50, 19);
    MoveCtrl(hwnd, 526, 807 - v110, bottom - 71, 50, 19);
    MoveCtrl(hwnd, 527, 807 - v110, bottom - 141, 50, 19);

    // The original updates v110 only after the complete facial group above;
    // the adjusted base belongs exclusively to the following view/play band.
    if (opt5 == 0 && opt0 == 0) v110 += 251;                  // 0x4448A8
    if (opt0 == 0) v110 += 13;                                // 0x444895
    MoveCtrl(hwnd, 402, 886 - v110, bottom - 141, 38, 17);
    MoveCtrl(hwnd, 403, 925 - v110, bottom - 141, 37, 17);
    MoveCtrl(hwnd, 404, 963 - v110, bottom - 141, 37, 17);
    MoveCtrl(hwnd, 405, 886 - v110, bottom - 121, 38, 17);
    MoveCtrl(hwnd, 406, 925 - v110, bottom - 121, 37, 17);
    MoveCtrl(hwnd, 407, 963 - v110, bottom - 121, 37, 17);
    MoveCtrl(hwnd, 408, 887 - v110, bottom - 63, 50, 24);
    MoveCtrl(hwnd, 409, 888 - v110, bottom - 36, 50, 14);
    MoveCtrl(hwnd, 410, 952 - v110, bottom - 36, 50, 14);
    MoveCtrl(hwnd, 411, 952 - v110, bottom - 58, 13, 13);
    MoveCtrl(hwnd, 412, 902 - v110, bottom - 99, 13, 13);
    MoveCtrl(hwnd, 531, 971 - v110, bottom - 99, 13, 13);
    MoveCtrl(hwnd, 413, 952 - v110, bottom - 19, 13, 13);
    MoveCtrl(hwnd, 414, 888 - v110, bottom - 19, 13, 13);
    MoveCtrl(hwnd, 534, 1017 - v110, bottom - 63, 22, 57);
    MoveCtrl(hwnd, 535, 1002 - v110, bottom - 141, 35, 37);

    // --- visibility re-toggle, five id ranges (0x444B3B..0x444C99) ---------
    ToggleVisible(hwnd, 455, 469);
    ToggleVisible(hwnd, 471, 488);
    ToggleVisible(hwnd, 504, 528);
    ToggleVisible(hwnd, 402, 414);
    ToggleVisible(hwnd, 531, 535);

    // --- refresh tail (0x444C9D..0x444CB0) ---------------------------------
    Sub42C810(app);
    Sub40CAC0(app);  // original tail call: `return sub_40CAC0(this);`
}

void Sub442EB0(MMDApp* app) {
    const HWND hwnd = static_cast<HWND>(app->Hwnd());
    const int side = app->SidebarWidth();
    RECT client{};
    GetClientRect(hwnd, &client);

    MoveCtrl(hwnd, 400, side / 2 - 80, 21, 70, 24);
    MoveCtrl(hwnd, 401, side / 2 + 10, 21, 70, 24);
    MoveCtrl(hwnd, 418, side / 2 - 49, 67, 20, 24);
    MoveCtrl(hwnd, 419, side / 2 + 29, 67, 20, 24);
    MoveCtrl(hwnd, 533, side / 2 - 79, 69, 20, 22);
    MoveCtrl(hwnd, 532, side / 2 + 59, 69, 20, 22);
    MoveCtrl(hwnd, 417, side / 2 - 27, 66, 54, 24);
    MoveCtrl(hwnd, 559, side - 30, 69, 20, 22);
    MoveCtrl(hwnd, 558, 15, 69, 20, 22);

    const int bottom = client.bottom;
    MoveCtrl(hwnd, 427, side - 19, 162, 16, bottom - 410);
    MoveCtrl(hwnd, 428, 97, bottom - 247, side - 116, 16);
    MoveCtrl(hwnd, 429, 8, bottom - 247, 80, 18);
    MoveCtrl(hwnd, 416, side / 2 + 15, bottom - 224, 50, 18);
    MoveCtrl(hwnd, 423, side / 2 + 70, bottom - 224, 50, 18);
    MoveCtrl(hwnd, 420, side / 2 - 119, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 421, side / 2 - 76, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 422, side / 2 - 33, bottom - 224, 40, 18);
    MoveCtrl(hwnd, 425, side / 2 - 3, bottom - 201, 50, 14);
    MoveCtrl(hwnd, 426, side / 2 + 69, bottom - 201, 50, 14);
    MoveCtrl(hwnd, 434, side / 2 - 118, bottom - 193, 100, 400);
    MoveCtrl(hwnd, 415, side / 2 - 4, bottom - 181, 60, 18);
    MoveCtrl(hwnd, 424, side / 2 + 60, bottom - 181, 60, 18);

    ToggleVisible(hwnd, 400, 401);
    ToggleVisible(hwnd, 415, 426);
    ToggleVisible(hwnd, 558, 559);
    ToggleVisible(hwnd, 434, 434);
    Sub42C810(app);
    Sub40CAC0(app);
}

}  // namespace mikudancestudio
