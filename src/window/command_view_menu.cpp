// ===========================================================================
// CommandDispatch family: View/Edit option-menu cases 251..302 (0x47E8A0)
// ===========================================================================
// Cases 251..302 (0xFB..0x12E) of the 368-case switch in CommandDispatch
// (0x47E8A0): display/physics option toggles (byte-flag flips), menu check
// sync (CheckMenuItem) and enable state (EnableMenuItem), refresh tails
// (0x42C810 / 0x40CAC0 / 0x414610 PanelPaint), modal editor dialogs
// (DialogBoxParamA / CreateDialogParamA), the bone/camera rotation dialogs
// (case 300) and the render-to-picture / save-enhanced-model / open-oni
// file dialogs (cases 276/263/296).  Case 278 reloads the toon texture set
// (0x424DC0, InitToonTextures).
//
// Calling convention of the original (sub_47E8A0): thiscall, this = app
// (ebp = ecx at 0x47E8F4); arg_0 = sending control HWND (only used by the
// default handler's notify==1 compare chain, which can never match a menu
// id); arg_4 = wParam (id = LOWORD).  All `hwnd` uses in this family read
// app+0xA06B8 (the main window), i.e. the `hwnd` parameter of the port.
//
// Case map (id -> behaviour -> original VA).  Index byte_48F650[id-0xC8]
// selects the jump-table entry jpt_47E903 (0x48F30C); targets below are the
// `jumptable 0047E903 case N` labels confirmed by IDA:
//
//   id  hex  VA            behaviour
//   --- ---  ------------  ---------------------------------------------------
//   251 0FB  0x0048A1CC    gate app+0x2F8: return if set; 0xBC=1; modal
//                          "frame control" dialog (0x294 EN / 0x27C JP,
//                          sub_44D3F0); return when it closes with 2,
//                          else default (no-op)
//   252 0FC  0x0048A219    same gate/flag; dialog 0x295 EN / 0x283 JP
//                          (sub_44D510); same close handling
//   253 0FD  0x00489FC8    if app+0xA0B44 (dialog) or app+0x2F8 set:
//                          default (no-op); else CreateDialogParamA
//                          (0x296 EN / 0x285 JP, sub_44C7F0), store
//                          app+0xA0B44, ShowWindow(5) + UpdateWindow
//   254 0FE  0x0048A26A    toggle physics-display byte app+0x9ED9A with
//                          CheckMenuItem(GetMenu(hwnd), 0xFE, 0/8) -
//                          same flag as menu 0xD8
//   255 0FF  0x0048A883    0x54=1, sub_439E40(app), dirty 0xA0B0D=1
//   256 100  0x0048A89B    0x5C=1, sub_43A650(app), dirty 0xA0B0D=1
//   257 101  0x0048A8B3    0x64=1, sub_43B720(app), dirty 0xA0B0D=1
//   258 102  0x0048A8CB    0x68=1, sub_43BB30(app), dirty 0xA0B0D=1
//   259 103  0x0048A8E3    if app+0xA0B50 or app+0x2F8 set: default no-op;
//                          else CreateDialogParamA (0x29A EN / 0x299 JP,
//                          sub_464BD0) stored at app+0xA0B50, show+update
//   260 104  0x0048A862    toggle English UI byte 0xA0B4C (setz), then
//                          LocalizeUI (0x441AD0) + sub_40B5A0 (0x40B5A0)
//   261 105  0x0048A958    gate app+0x2F8; 0xBC=1; modal "enhance model"
//                          dialog (0x2AA EN / 0x2A9 JP, sub_43C9A0);
//                          if closed with 2 return; MessageBoxA
//                          "Please preserve the edit result as a new model
//                          by 'save enhanced model'." (JP 0x52E724), caption
//                          "enhance model" (JP 0x52E764), MB_TOPMOST;
//                          0xA0B64=1
//   262 106  0x0048A9EC    if app+0xA0B74 or app+0x2F8 set: default no-op /
//                          return; 0x48=1, 0xBC=1; CreateDialogParamA
//                          (0x2AC EN / 0x2AB JP, sub_465020) at app+0xA0B74
//   263 107  0x0048AA6F    "save enhanced model": SetCurrentDirectoryW(exe),
//                          swprintf_s(kFmt529688), OPENFILENAMEW (lStruct
//                          0x4C, filter "Polygon Model files(*.pmd)" /
//                          "*.pmd" / "All Files(*.*)", nFilterIndex 1,
//                          Flags 6, defext "pmd", title "save enhanced
//                          model" / JP 0x52F650, initial dir = DirModel
//                          when menu 0x12D checked else "UserFile\Model");
//                          dir-copy gate like 0x12D -> ExtractDirFromPath
//                          (app+0x9F338) -> Sub42AE20(DirModel);
//                          sub_41EC10(app, path) (save model file)
//   264 108  0x0048ABB0    0x40=1; toggle byte app+0xA0CC8 with
//                          CheckMenuItem 0x108
//   265 109  0x0048ACA7    app+0xA0CC4 = 1; check 0x109, uncheck
//                          0x10D/0x10E/0x110; ModelKinematicSync(0x4B2210)
//                          over the 100 model slots; 0x9EDB5=1
//   266 10A  0x0048AEE1    if app+0xA0CCC set: return; 0x4C=1, 0xBC=1;
//                          CreateDialogParamA (0x323 EN / 0x322 JP,
//                          sub_479E90) at app+0xA0CCC
//   267 10B  0x0048AF57    0x44=1; MessageBoxA "Bullet Physics Library
//                          Ver.2.75\n\nhttp://www.bulletphysics.com/",
//                          caption "about physical engine" (JP 0x52F5E0),
//                          MB_TOPMOST
//   268 10C  0x0048AECE    0x54=1, 0x9EDB5=1
//   269 10D  0x0048AC0E    app+0xA0CC4 = 2; check 0x10D, uncheck
//                          0x109/0x10E/0x110; kinematic sync loop;
//                          0x9EDB5=1
//   270 10E  0x0048AD38    app+0xA0CC4 = 3; check 0x10E, uncheck
//                          0x10D/0x109/0x110; sync loop; 0x9EDB5=1
//   271 10F  0x0048AE60    0x60=1; menu 0x10F (IK display) toggled from its
//                          GetMenuState bit 8; byte app+0xA066D mirrors
//   272 110  0x0048ADCC    app+0xA0CC4 = 0; check 0x110, uncheck
//                          0x10D/0x109/0x10E; sync loop (no 0x9EDB5)
//   273 111  0x0048DBFF    gate app+0x2F8; 0x48=1; per-bone selection:
//                          sel[i] = (frames[i*0x25C+0x1EC] != 0),
//                          curBone = i when visible; PostLanguageSweep +
//                          PostLanguageSweep2
//   274 112  0x0048DC96    gate app+0x2F8; 0x30=1; clear frame-selection
//                          flags of morph (+0x10/0x14), other (+0x14/0x1C)
//                          and bone (+0x38/0x3C) tables, then mark the
//                          bone frames of every visible bone and its
//                          parent chain (+0x38 when +0x39==0); PanelPaint
//   275 113  0x0048DE30    0x38=1, 0xBC=1; modal dialog 0x32D EN / 0x32C JP
//                          (sub_461CE0) - no close handling (always return)
//   276 114  0x0048AFA4    "render to picture file": SetCurrentDirectoryW,
//                          swprintf_s(kFmt529688), OPENFILENAMEW ("All
//                          picture format" / JP 0x52F218, defext "bmp",
//                          title "render to picture file" / JP 0x52F1D0,
//                          initial dir DirUser / "UserFile" on menu 0x12D);
//                          dir-copy gate, wcscpy_s(app+0x9F134, path);
//                          grow locale render size (0x1D4E4/0x1D4E8 ->
//                          0x1D4FC/0x1D500) + PostDeviceReset when render
//                          W/H grew; CreateWindowExA "RecWindow" class
//                          (title 0x52F1C0 "描画中(0 frame)", style
//                          0x80C80000, X = sidebar+0x32, Y = 0x64,
//                          AdjustWindowRect 0x80C00000) stored at
//                          app+0xA0D24; CreateWindow-failed MessageBox
//                          (EN/JP 0x52DFF8); scene vtable+0x94 call when
//                          locale+0x1D4F8 == 0; Sub4168D0 when (0x9EB84==3
//                          || 0x91C==1) && 0x9E400!=0; Sub42C810;
//                          InvalidateRect(&app+0xA0D40); 0xA03B7=1
//   277 115  0x0048B2F7    0x58=1; menu 0x115 (shadow) toggled from
//                          GetMenuState bit 8; scene vtable+0xE4(scene,
//                          0xA1, on/off)
//   278 116  0x0048B401    InitToonTextures (0x424DC0), sub_4076E0(locale
//                          0xA06C4) - toon texture reload
//   279 117  0x0048B393    0x60=1; menu 0x117 (self shadow) toggle;
//                          byte app+0xA0188 mirrors
//   280 118  0x0048B416    app+0xA0D38 ? SaveFlagSubsystem (0x461FA0)
//                          : InitFlagSubsystem (0x461E00)
//   281 119  0x0048B433    0x6C=1; menu 0x119 (transparent window) toggle;
//                          SetWindowPos(app+0xA0D38, HWND_TOPMOST /
//                          HWND_NOTOPMOST, 0,0,0,0, 0x43)
//   282 11A  0x0048B624    0x40=1; toggle byte app+0xA0194 with
//                          CheckMenuItem 0x11A
//   283 11B  0x0048B682    0x44=1; toggle byte app+0xA0195 with
//                          CheckMenuItem 0x11B
//   284 11C  0x0048B6E0    0x6C=1; toggle byte app+0xA0196 with
//                          CheckMenuItem 0x11C
//   285 11D  0x0048B73E    0x6C=1; toggle byte app+0xA0197 with
//                          CheckMenuItem 0x11D; sub048+0x44 -> ptr,
//                          ptr+0xD4 = 1 / -1
//   286 11E  0x0048B7B8    0x48=1, 0xBC=1; ChooseColorA (lStruct 0x24,
//                          rgbResult from app+0xA0198/9C/A0, Flags 3,
//                          cust colors app+0xA01A4); write R/G/B back,
//                          Sub4A4850(model, r, g, b) over the 100 slots
//   287 11F  0x0048B8CF    toggle byte app+0xA01E4 with CheckMenuItem 0x11F
//   288 120  0x0048A08F    0xBC=1; modal dialog 0x325 EN / 0x324 JP
//                          (sub_4641F0); return when closed
//   289 121  0x0048A0DF    0x38=1, 0xBC=1; modal dialog 0x32B EN / 0x32A JP
//                          (sub_42E370); return when closed
//   290 122  0x0048B926    app+0xA0274=1; sub_4629D0 (0x4629D0),
//                          PostDeviceReset (0x440DB0)
//   291 123  0x0048B93E    0x5C=1; app+0xA03B8 ? DisableKinect (0x42A020)
//                          : OpenNiInit (0x429CB0) with 0
//   292 124  0x0048B964    0x38=1; auto-record toggle (byte app+0xA0D68):
//                          off->on: CheckMenuItem 0x124, SetTimer(hwnd,
//                          0x65, 0x5DC), 0x9EDB5=1, model+0x38FD =
//                          app+0xA03EA per slot; on->off: KillTimer(0x65),
//                          uncheck, 0xA0B0D=1, 0x9EDB5=1
//   293 125  0x0048BA1E    toggle byte app+0xA03DC with CheckMenuItem 0x125
//   294 126  0x0048BA75    0x74=1; toggle byte app+0xA03DD with
//                          CheckMenuItem 0x126
//   295 127  0x0048BAD3    0x40=1; toggle byte app+0xA03DE with
//                          CheckMenuItem 0x127
//   296 128  0x0048BB31    "open oni data": app+0x2F8 set -> MessageBox
//                          "Please select model!" (JP 0x52E2F4), caption
//                          "open oni data" / "oni"; else GetOpenFileNameW
//                          ("oni files(*.oni)", defext "oni", Flags
//                          0x1000, initial dir "UserFile", owner
//                          app+0xA0D38 ?: hwnd); DisableKinect when
//                          0xA03B8; WideToSjisPath (0x407910) then
//                          OpenNiInit (0x429CB0)
//   297 129  0x0048BC9B    0x40=1; toggle byte app+0xA03E9 with
//                          CheckMenuItem 0x129
//   298 12A  0x0048BD18    menu 0x12A (2D/3D render) toggle; locale
//                          byte +0x1D570 = on/off; InitRenderStates
//                          (0x406E90) on the locale subobject
//   299 12B  0x0048BDA1    menu 0x12B toggle; BM_SETCHECK(0xF1, on/off)
//                          into GetDlgItem(app+0xA0D38 ?: hwnd, 0x228);
//                          byte app+0xA4420 mirrors
//   300 12C  0x0048A2C1    "rotation" dialog: 0xBC=1.  app+0x2F8 set:
//                          camera path - posx/y/z + cam2/3/4 + camangle
//                          through degree/radian conversion (dbl_52B768
//                          pi, dbl_52B760 180.0, dbl_52E678 (float)pi),
//                          dialog 0x298 EN / 0x289 JP (sub_40FBC0),
//                          writeback, dirty, PostViewRefresh.  else bone
//                          path: cur bone frames pos/quat snapshot,
//                          D3DXMatrixRotationQuaternion + asin/atan2
//                          extraction (0x40A690/0x40A6B0) + quadrant
//                          fixes (+-pi on m01/m13 when fabs(cos(t3))
//                          < flt_52B740), clamp, dialog 0x297 EN /
//                          0x288 JP (sub_40F860), Sub42D6E0, pos/angle
//                          writeback, RotationZ*X*Y matrix, quaternion
//                          writeback, frameFlag, dirty, PostViewRefresh
//   301 12D  0x0048D700    menu 0x12D (enhanced mode / accessory column)
//                          toggle only
//   302 12E  0x0048E0A6    "reset rotation": 0x34=1.  app+0x2F8 set:
//                          cam2/3/4 = 0, dirty, PostViewRefresh.  else
//                          cur bone (2D90 != -1): Sub42D6E0, identity
//                          matrix, D3DXQuaternionRotationMatrix into the
//                          bone quat, frameFlag[cur]=1, dirty,
//                          PostViewRefresh (tail shared with case 300)
//
// Fidelity notes:
//   * All flags/menu calls follow the original byte-for-byte (CheckMenuItem
//     flags 0/8, SetWindowPos flags 0x43, BM_SETCHECK 0/1, MB_TOPMOST
//     0x40000).  The `def_47E903` default handler only acts when
//     HIWORD(wParam) == BN_CLICKED and the control equals
//     GetDlgItem(hwnd, 0x1B4) - a no-op for every menu id in this family,
//     so `break` == the original's `jmp def_47E903`.
//   * `jmp loc_48F2E0` (function epilogue) is `return`.
//   * Float math follows the original x87 shape (double intermediates,
//     float stores).  Constants are bit-exact: dbl_52B768 = 0x400921FB
//     00000000 (pi truncated to 0x921FB mantissa), dbl_52B760 = 180.0,
//     dbl_52E678 = 0x400921FC80000000 (= (double)(float)pi),
//     flt_52B740 = 0x358637BD, flt_52B73C = 0xC0490FD8 (-pi float),
//     flt_52B738 = 0x40490FD8 (+pi float).
//   * Dialog procedures of this family are ported below (Sub44D3F0 .. 
//     Sub4641F0 with their original VAs); helpers they call that have no
//     port anywhere yet remain as external-linkage stubs in this TU with the
//     original VAs (call sites preserved, TODO(port); stubs.cpp is
//     off-limits).
//
// Reference: ../translated/MikuMikuDance/fcn_0047e8a0.cpp
//   (the translated file covers only cases 200..0xDC; this port follows the
//   IDA decompilation of MikuMikuDance.exe 0x47E8A0 directly)
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
#include "mikudancestudio/accessory_layout.hpp"
#include "mikudancestudio/global_key_layout.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/model.hpp"

namespace mikudancestudio {

// Defined in stubs.cpp - declared here (before the anonymous namespace) so
// the dialog procs inside it can call it.
void Sub4220C0(MMDApp* app);  // VA 0x004220C0 (stubs.cpp)

namespace {

// ---------------------------------------------------------------------------
// App-state offsets used by this family but not yet registered in
// offsets.hpp (kept file-local until gen_offsets.py catches up).
// ---------------------------------------------------------------------------
constexpr std::size_t kOff30 = 0x30;    // dialog-open flag 0x30 (case 274)
constexpr std::size_t kOff34 = 0x34;    // dialog-open flag 0x34 (case 302)
constexpr std::size_t kOff38 = 0x38;    // dialog-open flag 0x38 (289/292/275)
constexpr std::size_t kOff40 = 0x40;    // dialog-open flag 0x40 (264/282/295/297)
constexpr std::size_t kOff44 = 0x44;    // dialog-open flag 0x44 (267/283)
constexpr std::size_t kOff48 = 0x48;    // dialog-open flag 0x48 (262/273/276/286)
constexpr std::size_t kOff4C = 0x4C;    // dialog-open flag 0x4C (266)
constexpr std::size_t kOff54 = 0x54;    // dialog-open flag 0x54 (255/268)
constexpr std::size_t kOff58 = 0x58;    // dialog-open flag 0x58 (277)
constexpr std::size_t kOff5C = 0x5C;    // dialog-open flag 0x5C (256/291)
constexpr std::size_t kOff60 = 0x60;    // dialog-open flag 0x60 (263/271/279)
constexpr std::size_t kOff64 = 0x64;    // dialog-open flag 0x64 (257)
constexpr std::size_t kOff68 = 0x68;    // dialog-open flag 0x68 (258)
constexpr std::size_t kOff6C = 0x6C;    // dialog-open flag 0x6C (281/284/285)
constexpr std::size_t kOff74 = 0x74;    // dialog-open flag 0x74 (294)

// Rotation-dialog scratch floats (case 300/302): shared with the dialog
// procs sub_40FBC0 / sub_40F860 in the original, hence app fields.
constexpr std::size_t kFltA0B28 = 0xA0B28;  // temp 0 (pos x)
constexpr std::size_t kFltA0B2C = 0xA0B2C;  // temp 1 (pos y)
constexpr std::size_t kFltA0B30 = 0xA0B30;  // temp 2 (pos z)
constexpr std::size_t kFltA0B34 = 0xA0B34;  // temp 3 (rot x / cam2)
constexpr std::size_t kFltA0B38 = 0xA0B38;  // temp 4 (rot y / cam3)
constexpr std::size_t kFltA0B3C = 0xA0B3C;  // temp 5 (rot z / cam4)
constexpr std::size_t kFltA0B40 = 0xA0B40;  // temp 6 (-camangle, camera path)

constexpr std::size_t kDwordA0B44 = 0xA0B44;  // model-info dialog handle (253)
constexpr std::size_t kDwordA0B74 = 0xA0B74;  // frame-copy dialog handle (262)
constexpr std::size_t kDwordA0CCC = 0xA0CCC;  // accessory-frame dialog hndl (266)
constexpr std::size_t kByteA0188 = 0xA0188;   // self-shadow flag mirror (279)
constexpr std::size_t kByteA03EA = 0xA03EA;   // auto-record source byte (292)
constexpr std::size_t kByteA03E9 = 0xA03E9;   // auto-frame checkbox flag (297)
constexpr std::size_t kByteA066D = 0xA066D;   // IK-display flag mirror (271)
constexpr std::size_t kDwordA02AC = 0xA02AC;  // saved render width (276)
constexpr std::size_t kDwordA02B0 = 0xA02B0;  // saved render height (276)
constexpr std::size_t kWcsA9F134 = 0x9F134;   // current render path buffer (276)
constexpr std::size_t kByteA0B64 = 0xA0B64;   // enhance-model dialog flag (261)

// Dialog-proc state fields (offsets not yet registered in offsets.hpp).
constexpr std::size_t kDwordA06B8 = 0xA06B8;  // main window handle
constexpr std::size_t kDwordA0B1C = 0xA0B1C;  // frame-order int array (288/289)
constexpr std::size_t kDwordA0B48 = 0xA0B48;  // saved wndproc of edit 646 (253)
constexpr std::size_t kDwordA0B54 = 0xA0B54;  // saved wndproc of edit 667 (259)
constexpr std::size_t kDwordA0B58 = 0xA0B58;  // model-edge combo 669 cursor (259)
constexpr std::size_t kDwordA0B5C = 0xA0B5C;  // model-edge combo 673 cursor (259)
constexpr std::size_t kDwordA0B60 = 0xA0B60;  // model-edge combo 677 cursor (259)
constexpr std::size_t kDwordA0B6C = 0xA0B6C;  // frame-edit dialog dirty flag (262)
constexpr std::size_t kDwordA0B78 = 0xA0B78;  // saved wndproc of edit 705 (262)
constexpr std::size_t kDwordA0B7C = 0xA0B7C;  // camera-frame record array ptr
constexpr std::size_t kScratchCam = 0xA0B80;  // 172-byte camera-frame scratch (262)
constexpr std::size_t kDwordA0C2C = 0xA0C2C;  // current camera frame index (262)
constexpr std::size_t kDwordA0C30 = 0xA0C30;  // bone-frame record array ptr
constexpr std::size_t kScratchBone = 0xA0C34; // 140-byte bone-frame scratch (262)
constexpr std::size_t kDwordA0C50 = 0xA0C50;  // saved wndproc of edit 709 (266)
constexpr std::size_t kDwordA0CC0 = 0xA0CC0;  // current bone frame index (262)
constexpr std::size_t kByteA0CD4 = 0xA0CD4;   // accessory-frame checkbox 731 (266)

// Model-field offsets (model = slot array app+0x780 [byte app+0x910]).
                                                // stride: parent dword@0x30,
                                                // stored pos 0x140..0x148,
                                                // stored quat 0x14C..0x158,
                                                // visible byte +0x1EC
constexpr std::size_t kModelFrameFlag = 0x2D98; // frame-insert flag array
constexpr std::size_t kModelBoneFrames = 0x26E0;  // bone display frames, 0x3C
                                                  // stride: parent@+8, sel@+0x38
constexpr std::size_t kModelMorphFrames = 0x26E4; // morph frames, 0x14 stride
constexpr std::size_t kModelOtherFrames = 0x26E8; // other frames, 0x1C stride
constexpr std::size_t kModelOrder2D7C = 0x2D7C;   // combo order byte (+1 based)
constexpr std::size_t kModelOrder2D7D = 0x2D7D;   // applied order byte (288 OK)
constexpr std::size_t kModelFps31C0 = 0x31C0;     // current-FPS float (253)
constexpr std::size_t kModelNames33D8 = 0x33D8;   // enhance-model names, 10 x
                                                  // char[100] (261)
constexpr std::size_t kDwordA0CD0 = 0xA0CD0;      // saved wndproc of edit 709
                                                  // capture (266, 0x466370 tail)

// Global frame-table pointer slots (0x460080 OK sweep; each holds a pointer
// to a 10000-record table: camera 0x54-stride, light 0x28, self-shadow
// 0x18, gravity 0x24) and the 255 accessory-track pointer array (0x384).
constexpr std::size_t kOff374Tbl = 0x374;
constexpr std::size_t kOff378Tbl = 0x378;
constexpr std::size_t kOff37CTbl = 0x37C;
constexpr std::size_t kOff380Tbl = 0x380;

// Bit-exact original constants (see fidelity notes in the header).
inline double Bits64(std::uint64_t b) {
    double d;
    std::memcpy(&d, &b, sizeof d);
    return d;
}
inline float Bits32(std::uint32_t b) {
    float f;
    std::memcpy(&f, &b, sizeof f);
    return f;
}
constexpr std::uint64_t kDbl52B768bits = 0x400921FB00000000ull;  // pi (truncated)
constexpr std::uint64_t kDbl52B760bits = 0x4066800000000000ull;  // 180.0
constexpr std::uint64_t kDbl52E678bits = 0x400921FC80000000ull;  // (double)(float)pi
constexpr std::uint32_t kFlt52B740bits = 0x358637BDu;  // ~1e-6 threshold
constexpr std::uint32_t kFlt52B73Cbits = 0xC0490FD8u;  // -pi float
constexpr std::uint32_t kFlt52B738bits = 0x40490FD8u;  // +pi float

// 0x529688: swprintf_s format L"\0\0%s%s" - the leading NULs make the
// original call (which passes NO varargs) write an empty string; the %s
// slots are never consumed.  Kept byte-identical (same idiom as
// command_control_450.cpp case 472).

// Active model = slot array at app+0x780 indexed by byte app+0x910.
static unsigned char* ActiveModel(MMDApp* app) {
    return app->SelectedModel();
}

// 0x40A680 / 0x40A690 / 0x40A6B0 / 0x40A6D0 - the D3DX angle helpers of the
// bone-rotation dialog (case 300).  sub_40A6B0 dispatches to the CRT
// atan2 (cintrin table entry "atan2" at 0x544C40): arg0 = x, arg1 = y;
// every result is rounded to float before it is stored.
double AngleFabs(float x) { return static_cast<double>(std::fabs(x)); }
double AngleAsin(float x) { return static_cast<double>(std::asin(x)); }
double AngleCos(float x)  { return static_cast<double>(std::cos(x)); }
double AngleAtan2(float y, float x) {
    return static_cast<double>(static_cast<float>(std::atan2(y, x)));
}

// Scene object = locale subsystem (app+0xA06C4) -> Renderer()->device
// (+0x1D4E0); vtable slot 0x94: __stdcall(obj, 0, locale->captureSurface)
// (0x48B28B, case 276; x86 passed the pointer slot's low dword - bit-exact
// pass-through of the pointer is the x64-correct equivalent).
void CallSceneVtable94(MMDApp* app) {
    D3DRenderer* locale = app->Renderer();
    void* scene = locale->device;  // 0x1D4E0
    using Fn = void(__stdcall*)(void*, int, void*);
    Fn fn = *reinterpret_cast<Fn*>(
        static_cast<unsigned char*>(*reinterpret_cast<void**>(scene)) + 0x94);
    fn(scene, 0, locale->captureSurface);  // 0x1D534
}

// Scene vtable slot 0xE4: __stdcall(obj, 0xA1, on)  (0x48B33C, case 277).
void CallSceneVtableE4(MMDApp* app, int on) {
    D3DRenderer* locale = app->Renderer();
    void* scene = locale->device;  // 0x1D4E0
    using Fn = void(__stdcall*)(void*, int, int);
    Fn fn = *reinterpret_cast<Fn*>(
        static_cast<unsigned char*>(*reinterpret_cast<void**>(scene)) + 0xE4);
    fn(scene, 0xA1, on);
}

// Shared tail of cases 300/302 (0x48A805): D3DXQuaternionRotationMatrix of
// the current bone's quaternion from `m`, frameFlag[curBone] = 1,
// dirty = 1, PostViewRefresh.
void BoneRotationWriteback(MMDApp* app, const d3dx::D3DXMATRIXF& m) {
    unsigned char* model = ActiveModel(app);
    const std::int32_t cur = mdl::Mdl(model)->selectedBone;
    mdl::BoneRecord* const bones = mdl::Bones(model);
    float quat[4];
    d3dx::Get().quatFromMatrix(quat, &m);
    std::memcpy(bones[cur].rotQuat, quat, sizeof(quat));
    unsigned char* frameFlag =
        *reinterpret_cast<unsigned char**>(model + kModelFrameFlag);
    frameFlag[cur] = 1;
    app->SceneModified() = 1;
    PostViewRefresh(app);
}

// ---------------------------------------------------------------------------
// Dialog procedures of this family, ported from the MikuMikuDance.exe
// decompilation (each carries its original VA).  They reach the app state via
// the global g_Block and share the rotation temp floats app+0xA0B28..0xA0B40.
// Helpers they call that have no port anywhere yet stay as external-linkage
// stubs in this TU (call sites preserved, TODO(port); stubs.cpp is off-limits).
// ---------------------------------------------------------------------------

// ---- JP strings, byte-exact Shift-JIS as in the binary ------------------
// 0x52E724: "編集結果を'強化モデル保存'で保存してモデルとして保存して下さい"
static const char kMsgEnhanceModelJp[] =
    "\x95\xD2\x8F\x57\x8C\x8B\x89\xCA\x82\xCD\x27\x8A\x67\x92\xA3"
    "\x83\x82\x83\x66\x83\x8B\x95\xDB\x91\xB6\x27\x82\xC5\x90\x56"
    "\x82\xB5\x82\xA2\x83\x82\x83\x66\x83\x8B\x82\xC6\x82\xB5\x82"
    "\xC4\x95\xDB\x91\xB6\x82\xB5\x82\xC4\x89\xBA\x82\xB3\x82\xA2";
// 0x52E764: "モデル強化"
static const char kCaptionEnhanceModelJp[] =
    "\x83\x82\x83\x66\x83\x8B\x8A\x67\x92\xA3";
// 0x52F5E0: "物理エンジンについて"
static const char kCaptionPhysEngineJp[] =
    "\x95\xA8\x97\x9D\x83\x47\x83\x93\x83\x57\x83\x93\x82\xC9\x82\xC2"
    "\x82\xA2\x82\xC4";
// 0x52DFF8: "描画ウィンドウ作成"
static const char kCaptionCreateWndJp[] =
    "\x98\x5E\x89\xE6\x83\x45\x83\x42\x83\x93\x83\x68\x83\x45\x8D\xEC"
    "\x90\xAC";
// 0x52F1C0: window title "描画中(0 frame)" (ANSI)
static const char kRecWndTitle[] =
    "\x98\x5E\x89\xE6\x92\x86(0 frame)";
// 0x52E2F4: "モデルを選択していないとoniデータは読めません"
static const char kMsgOpenOniJp[] =
    "\x83\x4A\x83\x81\x83\x89\x81\x45\x8F\xC6\x96\xBE\x83\x82\x81\x5B"
    "\x83\x68\x82\xC5\x82\xCD" "oni" "\x83\x66\x81\x5B\x83\x5E\x82\xCD"
    "\x93\xC7\x82\xDF\x82\xDC\x82\xB9\x82\xF1";

// 0x52F650: JP title "拡張モデル保存"
static const wchar_t kTitleSaveModelJp[] =
    L"\x62E1\x5F35\x30E2\x30C7\x30EB\x4FDD\x5B58";
// 0x52F218: JP picture filter label "書きの画像フォーマット" (label is
// followed by a double NUL in the binary - the pattern list is empty).
static const wchar_t kFilterPicJp[] =
    L"\x5168\x3066\x306E\x5BFE\x5FDC\x30D5\x30A9\x30FC\x30DE\x30C3\x30C8"
    L"\x00\x00"
    L"*.bmp;*.jpg;*.png;*.dds;*.dib;*.pfm;*.hdr\x00\x00"
    L"Bmp files(*.bmp)\x00\x00*.bmp\x00\x00";
// 0x52F1D0: JP title "画像ファイル出力"
static const wchar_t kTitleRenderJp[] =
    L"\x753B\x50CF\x30D5\x30A1\x30A4\x30EB\x51FA\x529B";
// 0x52DC84: JP title "ファイルを開く"
static const wchar_t kTitleOpenJp[] =
    L"\x30D5\x30A1\x30A4\x30EB\x3092\x958B\x304F";

// ---- dialog-helper call targets with no port anywhere yet (TODO(port) ----
// (Sub4220C0 is declared at mikudancestudio scope below - defined in stubs.cpp.)

// Forward declarations (bodies below / later in this TU).
void Sub41E980(MMDApp* app, float fps);                   // VA 0x0041E980
void Sub45FD80(MMDApp* app, int idx, float v);            // VA 0x0045FD80
void Sub43BED0(MMDApp* app, HWND hEdit, int idx);         // VA 0x0043BED0
void Sub412B20(MMDApp* app, std::int32_t frame);          // VA 0x00412B20
void Sub43E000(MMDApp* app, HWND hDlg) {  // VA 0x0043E000 frame-control apply
    (void)app; (void)hDlg; /* TODO(port) */
}
void Sub43E680(MMDApp* app, HWND hDlg) {  // VA 0x0043E680 frame-control apply
    (void)app; (void)hDlg; /* TODO(port) */
}
// VA 0x0042E270 - edit-646 (0x286) subclass of the model-info dialog (253):
// on WM_KEYDOWN+VK_RETURN it reads the edit text, applies it through the
// FPS setter (0x41E980) and parks the percent value on trackbar 647
// (TBM_SETPOS, dbl 0x52B8E0 = 100.0); anything else goes to the saved
// original procedure (app+0xA0B48, captured when case 253 subclasses).
LRESULT CALLBACK Sub42E270(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MMDApp* app = g_Block;
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {  // 0x100 / 0x0D
        const HWND dlg = app->raw<HWND>(kDwordA0B44);
        if (hwnd == GetDlgItem(dlg, 0x286 /*646*/)) {
            char buf[8];
            GetWindowTextA(hwnd, buf, 8);
            const float v = static_cast<float>(atof(buf));
            Sub41E980(app, v);                          // 0x41E980
            SendMessageA(GetDlgItem(dlg, 0x287 /*647*/),
                         0x405 /*TBM_SETPOS*/, 1,
                         static_cast<int>(v * 100.0));  // dbl 0x52B8E0
            return 0;
        }
    }
    return CallWindowProcA(
        reinterpret_cast<WNDPROC>(
            app->raw<std::intptr_t>(kDwordA0B48)),
        hwnd, msg, wParam, lParam);
}
// VA 0x0041E950 - current-FPS getter: camera mode (byte 0x2F8) reports a
// constant 1.0, model mode the active model's FPS float (+0x31C0).
float Sub41E950(MMDApp* app) {
    if (app->state.optflag0 != 0)
        return 1.0f;
    unsigned char* model = app->SelectedModel();
    return *reinterpret_cast<float*>(model + kModelFps31C0);
}
// VA 0x0041E980 - FPS setter (model mode only): marks the in-dialog and
// dirty flags, then stores into the active model's FPS float (+0x31C0).
void Sub41E980(MMDApp* app, float fps) {
    if (app->state.optflag0 != 0)
        return;
    app->state.messageSeen = 1;
    app->SceneModified() = 1;
    unsigned char* model = app->SelectedModel();
    *reinterpret_cast<float*>(model + kModelFps31C0) = fps;
}
// VA 0x0041E9C0 - enhance-model dialog init: mirrors the ten 100-byte SJIS
// name slots of the active model (+0x33D8) into edits 709..718.
void Sub41E9C0(MMDApp* app, HWND hDlg) {
    unsigned char* model = app->SelectedModel();
    for (int i = 0; i < 10; ++i)
        SendMessageA(GetDlgItem(hDlg, 0x2C5 + i), 0xC2 /*WM_SETTEXT*/, 0,
                     reinterpret_cast<LPARAM>(model + kModelNames33D8 +
                                              100 * i));
}
int Sub41EA20(HWND hDlg) {  // VA 0x0041EA20 enhance-model dialog collect
    (void)hDlg; return 0; /* TODO(port) */
}
// VA 0x0045ECC0 - model-edge dialog (259) edit subclass: WM_KEYDOWN +
// VK_RETURN sets the in-dialog flag (app+0xA0D6C) and dispatches to the
// edge collector (0x43BED0) with 0/1/2/3 for edits 0x29B/0x2A0/0x2A4/
// 0x2A8 (edit 0x2A0 additionally runs the language sweep 0x42F1E0);
// everything else goes to the saved original proc (app+0xA0B54).
LRESULT CALLBACK Sub45ECC0(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MMDApp* app = g_Block;
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        app->state.messageSeen = 1;
        const HWND dlg = app->FrameRangeDialog();
        if (hwnd == GetDlgItem(dlg, 0x29B)) {
            Sub43BED0(app, hwnd, 0);                        // 0x43BED0
            return 0;
        }
        if (hwnd == GetDlgItem(dlg, 0x2A0)) {
            Sub43BED0(app, hwnd, 1);
            PostLanguageSweep(app);                         // 0x42F1E0
            return 0;
        }
        if (hwnd == GetDlgItem(dlg, 0x2A4)) {
            Sub43BED0(app, hwnd, 2);
            return 0;
        }
        if (hwnd == GetDlgItem(dlg, 0x2A8)) {
            Sub43BED0(app, hwnd, 3);
            return 0;
        }
    }
    return CallWindowProcA(
        reinterpret_cast<WNDPROC>(
            app->raw<std::intptr_t>(kDwordA0B54)),
        hwnd, msg, wParam, lParam);
}
void Sub43C430(HWND hDlg) {  // VA 0x0043C430 model-edge dialog init
    (void)hDlg; /* TODO(port) */
}
void Sub45F240(HWND hDlg) {  // VA 0x0045F240 model-edge dialog apply
    (void)hDlg; /* TODO(port) */
}
void Sub45F050(HWND hDlg) {  // VA 0x0045F050 model-edge combo 669 refresh
    (void)hDlg; /* TODO(port) */
}
void Sub45F480(HWND hDlg) {  // VA 0x0045F480 frame-edit camera combo refresh
    (void)hDlg; /* TODO(port) */
}
void Sub45EF10(HWND hDlg) {  // VA 0x0045EF10 model-edge combo 673 refresh
    (void)hDlg; /* TODO(port) */
}
void Sub45EDC0(HWND hDlg) {  // VA 0x0045EDC0 model-edge combo 677 refresh
    (void)hDlg; /* TODO(port) */
}
LRESULT CALLBACK Sub41EC50(HWND, UINT, WPARAM, LPARAM) {  // VA 0x0041EC50
    return 0; /* TODO(port) frame-edit edit-box subclass wndproc */
}
void Sub45F670(HWND hDlg) {  // VA 0x0045F670 frame-edit dialog init
    (void)hDlg; /* TODO(port) */
}
void Sub41FF30(MMDApp* app) {  // VA 0x0041FF30 camera-side frame collect
    (void)app; /* TODO(port) */
}
void Sub4204F0(MMDApp* app) {  // VA 0x004204F0 bone-side frame collect
    (void)app; /* TODO(port) */
}
void Sub43CA50(HWND hDlg) {  // VA 0x0043CA50 bone combo refresh
    (void)hDlg; /* TODO(port) */
}
void Sub43CCD0(HWND hDlg, int idx) {  // VA 0x0043CCD0 camera combo refresh
    (void)hDlg; (void)idx; /* TODO(port) */
}
void Sub4214A0(HWND hDlg, int idx) {  // VA 0x004214A0 bone combo refresh
    (void)hDlg; (void)idx; /* TODO(port) */
}
void Sub421C20(HWND hDlg, int flag) {  // VA 0x00421C20 frame-mode change
    (void)hDlg; (void)flag; /* TODO(port) */
}
void Sub421CE0(HWND hDlg, int mode, int idx) {  // VA 0x00421CE0
    (void)hDlg; (void)mode; (void)idx; /* TODO(port) */
}
void Sub4220F0(HWND hDlg) {  // VA 0x004220F0 frame-edit dialog finalize
    (void)hDlg; /* TODO(port) */
}
void Sub420DC0(HWND hDlg) {  // VA 0x00420DC0
    (void)hDlg; /* TODO(port) */
}
void Sub41E810(int count, HWND hDlg) {  // VA 0x0041E810 reorder-dialog fill
    (void)count; (void)hDlg; /* TODO(port) */
}
// VA 0x0041E910 - reorder-dialog OK apply: walks the order array
// (app+0xA0B1C, filled by 0x41E7B0 with slotIndex+1 per order) and stores
// the applied order byte into model+0x2D7D.  The original indexes the
// 0x780 slot array with the stored slotIndex+1 value verbatim (no -1),
// i.e. it addresses slot+1 - a quirk of the original kept as-is.
void Sub41E910(MMDApp* app, int count, HWND hDlg) {
    (void)hDlg;  // the original's second pushed argument is never read
    std::int32_t* order = app->raw<std::int32_t*>(kDwordA0B1C);
    for (int i = 0; i < count; ++i) {
        unsigned char* model = app->ModelSlot(order[i]);
        model[kModelOrder2D7D] = static_cast<unsigned char>(i);
    }
}
void Sub423160(HWND hDlg) {  // VA 0x00423160 accessory-frame dialog init
    (void)hDlg; /* TODO(port) */
}
// VA 0x00466370 - accessory-frame dialog (266) edit subclass: WM_KEYDOWN +
// VK_RETURN reads the edit (0x2C5..0x2C9 = 709..713) as a float and applies
// it through 0x45FD80 with channel 0..4; edits 710..712 additionally park
// the percent value on their trackbars 637..639 (TBM_SETPOS, dbl 0x52B8E0
// = 100.0); everything else goes to the saved proc (app+0xA0CD0).
LRESULT CALLBACK Sub466370(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MMDApp* app = g_Block;
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        const HWND dlg = app->raw<HWND>(kDwordA0CCC);
        char buf[0x100];
        for (int ch = 0; ch < 5; ++ch) {
            if (hwnd != GetDlgItem(dlg, 0x2C5 + ch))
                continue;
            GetWindowTextA(hwnd, buf, 0x100);
            const float v = static_cast<float>(atof(buf));
            if (ch >= 1 && ch <= 3)
                SendMessageA(GetDlgItem(dlg, 0x27C + ch),
                             0x405 /*TBM_SETPOS*/, 1,
                             static_cast<int>(v * 100.0));  // dbl 0x52B8E0
            Sub45FD80(app, ch, v);                           // 0x45FD80
            return 0;
        }
    }
    return CallWindowProcA(
        reinterpret_cast<WNDPROC>(
            app->raw<std::intptr_t>(kDwordA0CD0)),
        hwnd, msg, wParam, lParam);
}
// VA 0x0045FD80 - accessory-frame dialog channel apply.  Channels 0..3
// write the gravity cluster (magnitude / direction X / Y / Z, app+0x9EDC4 /
// 0x9EDB8 / 0x9EDBC / 0x9EDC0), normalise the direction (D3DXVec3Normalize,
// in place), scale it by magnitude * dbl 0x52C170 (10.0) and push it into
// the physics world setGravity (world = *(app+0x9EDB0)->world, vtable
// slot 0x34/4 = 13).  An all-zero direction gets Y nudged to
// flt 0x529624 = 0.1f first.  Channel 4 stores (int)value into app+0x9EDC8.
// Ends with RefreshRequest(-4) on every path.
void Sub45FD80(MMDApp* app, int idx, float v) {
    switch (idx) {
    case 0:
        app->state.gravityMagnitude = v;
        if (app->state.gravityX == 0.0f &&
            app->state.gravityY == 0.0f &&
            app->state.gravityZ == 0.0f)
            app->state.gravityY = 0.1f;  // flt 0x529624
        break;
    case 1:
        app->state.gravityX = v;
        if (v == 0.0f && app->state.gravityY == 0.0f &&
            app->state.gravityZ == 0.0f)
            app->state.gravityY = 0.1f;
        break;
    case 2:
        app->state.gravityY = v;
        if (app->state.gravityX == 0.0f &&
            app->state.gravityZ == 0.0f && v == 0.0f)
            app->state.gravityY = 0.1f;
        break;
    case 3:
        app->state.gravityZ = v;
        if (app->state.gravityX == 0.0f &&
            app->state.gravityY == 0.0f && v == 0.0f)
            app->state.gravityY = 0.1f;
        break;
    default:
        if (idx == 4)
            app->state.gravityNoise =
                static_cast<std::int32_t>(v);
        RefreshRequest(-4);                                 // 0x440AC0
        return;
    }

    // common tail: normalise, scale by magnitude * 10, setGravity
    float dir[3] = {app->state.gravityX,
                    app->state.gravityY,
                    app->state.gravityZ};
    if (d3dx::Get().vec3Normalize != nullptr && d3dx::Get().Load())
        d3dx::Get().vec3Normalize(dir, dir);
    else {  // documented fallback (same formula; 1/sqrt)
        const float inv = 1.0f / std::sqrt(dir[0] * dir[0] +
                                           dir[1] * dir[1] +
                                           dir[2] * dir[2]);
        dir[0] *= inv;
        dir[1] *= inv;
        dir[2] *= inv;
    }
    const float mag = app->state.gravityMagnitude;
    float vec[4] = {static_cast<float>(dir[0] * mag * 10.0),
                    static_cast<float>(dir[1] * mag * 10.0),
                    static_cast<float>(dir[2] * mag * 10.0), 0.0f};
    PhysicsScene* physicsScene = app->Physics();
    if (physicsScene != nullptr) {
        void* world = physicsScene->world;   // scene slot 0x40
        if (world != nullptr) {
            using SetGravity = void(__thiscall*)(void*, const float*);
            auto* vt = *reinterpret_cast<void***>(world);
            reinterpret_cast<SetGravity>(vt[0x34 / 4])(world, vec);
        }
    }
    RefreshRequest(-4);                                     // 0x440AC0
}
// VA 0x00460080 - accessory-frame dialog OK: dirty flag, clear the
// selection marks of the four global frame tables (camera 0x374 +0x48,
// light 0x378 +0x24, self-shadow 0x37C +0x14, gravity 0x380 +0x21) and of
// all 255 accessory tracks (0x384 blobs, +0x18, 0x3C stride), rebuild via
// 0x412B20 at the current frame, then RefreshRequest(-4) + PanelPaint.
void Sub460080(MMDApp* app) {
    app->SceneModified() = 1;
    mdl::CameraKey* camera = *reinterpret_cast<mdl::CameraKey**>(
        app->at(kOff374Tbl));
    mdl::LightKey* light = *reinterpret_cast<mdl::LightKey**>(
        app->at(kOff378Tbl));
    mdl::SelfShadowKey* shadow =
        *reinterpret_cast<mdl::SelfShadowKey**>(
        app->at(kOff37CTbl));
    mdl::GravityKey* gravity = *reinterpret_cast<mdl::GravityKey**>(
        app->at(kOff380Tbl));
    for (std::size_t i = 0; i < 10000; ++i) {
        camera[i].selected = 0;
        light[i].selected = 0;
        shadow[i].selected = 0;
        gravity[i].selected = 0;
    }
    for (int slot = 0; slot < 255; ++slot) {
        mdl::AccessoryKey* acc = app->AccessoryKeys(slot);
        if (acc == nullptr)
            continue;
        for (std::size_t i = 0; i < 10000; ++i)
            acc[i].selected = 0;
    }
    Sub412B20(app, app->state.currentFrame);  // 0x412B20
    RefreshRequest(-4);                                       // 0x440AC0
    PanelPaint(app);                                          // 0x414610
}
// 0x00412330 Sub412330 (gravity-track apply + physics dialog refresh) is
// ported in src/model/track_apply.cpp.
void Sub4403C0(int on) {  // VA 0x004403C0 shadow/edge display toggle
    (void)on; /* TODO(port) */
}
// VA 0x0041E7B0 - order-array builder: for order = 1..count-1 scan the 100
// model slots for the model whose order byte (+0x2D7C) equals `order` and
// store slotIndex+1 into the array at app+0xA0B1C.
void Sub41E7B0(MMDApp* app, int count) {
    std::int32_t* order = app->raw<std::int32_t*>(kDwordA0B1C);
    for (int ord = 1; ord < count; ++ord) {
        for (int i = 0; i < 100; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr &&
                static_cast<unsigned char>(model[kModelOrder2D7C]) == ord) {
                order[ord] = i + 1;
                break;
            }
        }
    }
}
// VA 0x0045EC80 - reorder apply (inverse direction of 0x41E910): for i =
// 1..count-1 store i into model+0x2D7C.  The original addresses the slot
// via base 0x77C (= 0x780 - 4), cancelling the +1 stored by 0x41E7B0;
// the second argument (dialog) is never read.
void Sub45EC80(MMDApp* app, int count, HWND hDlg) {
    (void)hDlg;
    std::int32_t* order = app->raw<std::int32_t*>(kDwordA0B1C);
    for (int i = 1; i < count; ++i) {
        unsigned char* model = app->ModelSlot(order[i] - 1);
        model[kModelOrder2D7C] = static_cast<unsigned char>(i);
    }
    Sub44D940(app);                                       // 0x44D940
}

// Original data-segment globals dword_545930 / dword_545938 (not in Block).
int g_dword545930 = 0;  // reorder-dialog frame count (0x545930)
int g_dword545938 = 0;  // frame-copy dialog combo count (0x545938)

// Newly-required unported dependencies (0x45ECC0 / 0x460080 call sites) -
// file-local stub bodies so the call sites link (stubs.cpp must not be
// modified).  TODO(port): real bodies.
void Sub43BED0(MMDApp* app, HWND hEdit, int idx) {  // VA 0x0043BED0
    (void)app; (void)hEdit; (void)idx;              // model-edge collector
}
void Sub412B20(MMDApp* app, std::int32_t frame) {   // VA 0x00412B20
    (void)app; (void)frame;                         // frame-table rebuild
}

// ---- JP strings of the dialogs, byte-exact -------------------------------
// 0x52E628: "ON (X印)" (SJIS 0x88F3 = 印); 0x52E624: "OFF".
static const char kOnXMarkJp[] = "ON (X\x88\xF3)";
static const char kOff[] = "OFF";
// 0x52D370 / 0x52D368 / 0x52D360: JP wide labels "ｶﾒﾗ･照明･ｱｸｾｻﾘ" / "画面" /
// "なし" (half-width katakana mixed with kanji; sent via SendMessageW).
static const wchar_t kJpCamLightAcc[] = {
    0xFF76, 0xFF92, 0xFF97, 0xFF65, 0x7167, 0x660E, 0xFF65, 0xFF71, 0xFF78,
    0xFF7E, 0xFF7B, 0xFF98, 0x0000};
static const wchar_t kJpScreen[] = {0x5730, 0x9762, 0x0000};
static const wchar_t kJpNone[] = {0x306A, 0x3057, 0x0000};

// ===========================================================================
// 0x44D3F0 - case 251 "frame control" dialog (0x294 EN / 0x27C JP).
// WM_INITDIALOG: top-most the dialog when the accessory column exists
// (app+0xA0D38), prefill the 12 edit boxes 686..697 (even columns "1.0",
// odd columns "0.0" via EM_REPLACESEL), focus 686 and select all.
// WM_COMMAND: id 1 -> sub_43E000 apply + EndDialog(1); id 2 -> EndDialog(2).
// Reference: MikuMikuDance.exe sub_44D3F0.
// ===========================================================================
INT_PTR CALLBACK Sub44D3F0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        MMDApp* app = g_Block;
        if (app->state.floatingWindow != 0) {
            // original: SetWindowPos(hDlg, HWND_MESSAGE|2, ...) == HWND_TOP
            SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
        }
        for (int i = 0; i < 12; i += 2) {
            SendMessageA(GetDlgItem(hDlg, i + 686), 0xC2 /*EM_REPLACESEL*/, 0,
                         (LPARAM)"1.0");
        }
        for (int j = 0; j < 12; j += 2) {
            SendMessageA(GetDlgItem(hDlg, j + 687), 0xC2 /*EM_REPLACESEL*/, 0,
                         (LPARAM)"0.0");
        }
        SetFocus(GetDlgItem(hDlg, 686));
        SendMessageA(GetDlgItem(hDlg, 686), 0xB1 /*EM_SETSEL*/, 0,
                     GetWindowTextLengthA(GetDlgItem(hDlg, 686)));
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 1) {
            Sub43E000(g_Block, hDlg);  // 0x43E000
            EndDialog(hDlg, 1);
            return 0;
        }
        if (id == 2) {
            EndDialog(hDlg, 2);
            return 0;
        }
    }
    return 0;
}

// ===========================================================================
// 0x44D510 - case 252 "frame control" dialog (0x295 EN / 0x283 JP).
// WM_INITDIALOG: same top-most gate, but only edits 686 ("1.0") and 687
// ("0.0") are prefilled; focus 686 + select all.
// WM_COMMAND: id 1 -> sub_43E680 apply + EndDialog(1); id 2 -> EndDialog(2).
// Reference: MikuMikuDance.exe sub_44D510.
// ===========================================================================
INT_PTR CALLBACK Sub44D510(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        MMDApp* app = g_Block;
        if (app->state.floatingWindow != 0) {
            SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
        }
        SendMessageA(GetDlgItem(hDlg, 686), 0xC2 /*EM_REPLACESEL*/, 0,
                     (LPARAM)"1.0");
        SendMessageA(GetDlgItem(hDlg, 687), 0xC2 /*EM_REPLACESEL*/, 0,
                     (LPARAM)"0.0");
        SetFocus(GetDlgItem(hDlg, 686));
        SendMessageA(GetDlgItem(hDlg, 686), 0xB1 /*EM_SETSEL*/, 0,
                     GetWindowTextLengthA(GetDlgItem(hDlg, 686)));
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 1) {
            Sub43E680(g_Block, hDlg);  // 0x43E680
            EndDialog(hDlg, 1);
            return 0;
        }
        if (id == 2) {
            EndDialog(hDlg, 2);
            return 0;
        }
    }
    return 0;
}

// ===========================================================================
// 0x44C7F0 - case 253 model-info dialog (0x296 EN / 0x285 JP), modeless.
// WM_INITDIALOG: top-most gate; subclass the FPS edit 646 with sub_42E270
// (old proc saved at app+0xA0B48); fill 646 with "%3.2f" of sub_41E950
// (current FPS) and set trackbar 647 (range 0..200, tick 0x64, pos fps*100).
// WM_COMMAND id 2: DestroyWindow + clear app+0xA0B44, return 1.
// WM_HSCROLL: TBM_GETPOS/100 -> sub_41E980, refresh 646, return 1.
// Reference: MikuMikuDance.exe sub_44C7F0.
// ===========================================================================
INT_PTR CALLBACK Sub44C7F0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    MMDApp* app = g_Block;
    switch (msg) {
    case WM_INITDIALOG: {
        if (app->state.floatingWindow != 0) {
            SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
        }
        app->raw<std::int32_t>(kDwordA0B48) =
            GetWindowLongPtrA(GetDlgItem(hDlg, 646), GWLP_WNDPROC);
        SetWindowLongPtrA(GetDlgItem(hDlg, 646), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)Sub42E270);
        const float fps = Sub41E950(app);  // 0x41E950
        char buf[0x100];
        sprintf_s(buf, 0x100, "%3.2f", static_cast<double>(fps));
        SendMessageA(GetDlgItem(hDlg, 646), 0xC2 /*EM_REPLACESEL*/, 0,
                     (LPARAM)buf);
        SendMessageA(GetDlgItem(hDlg, 647), 0x407 /*TBM_SETRANGEMIN*/, 0, 0);
        SendMessageA(GetDlgItem(hDlg, 647), 0x408 /*TBM_SETRANGEMAX*/, 0, 200);
        SendMessageA(GetDlgItem(hDlg, 647), 0x414 /*TBM_SETTICFREQ*/, 0x64, 0);
        SendMessageA(GetDlgItem(hDlg, 647), 0x405 /*TBM_SETPOS*/, 1,
                     (int)(fps * 100.0));
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 2) {
            DestroyWindow(hDlg);
            app->raw<std::int32_t>(kDwordA0B44) = 0;
            return 1;
        }
        break;
    case WM_HSCROLL: {
        const double v =
            static_cast<double>(
                SendMessageA(GetDlgItem(hDlg, 647), 0x400 /*TBM_GETPOS*/, 0, 0)) /
            100.0;
        Sub41E980(app, static_cast<float>(v));  // 0x41E980
        char buf[0x100];
        sprintf_s(buf, 0x100, "%3.2f", v);
        SendMessageA(GetDlgItem(hDlg, 646), 0xB1 /*EM_SETSEL*/, 0,
                     GetWindowTextLengthA(GetDlgItem(hDlg, 646)));
        SendMessageA(GetDlgItem(hDlg, 646), 0xC2 /*EM_REPLACESEL*/, 0,
                     (LPARAM)buf);
        return 1;
    }
    default:
        break;
    }
    return 0;
}

// ===========================================================================
// 0x43C9A0 - case 261 "enhance model" dialog (0x2AA EN / 0x2A9 JP).
// WM_INITDIALOG: top-most gate + sub_41E9C0 init.
// WM_COMMAND: id 1 -> EndDialog(1) when sub_41EA20 succeeds else EndDialog(2);
// id 2 -> EndDialog(2).
// Reference: MikuMikuDance.exe sub_43C9A0.
// ===========================================================================
INT_PTR CALLBACK Sub43C9A0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        MMDApp* app = g_Block;
        if (app->state.floatingWindow != 0) {
            SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
        }
        Sub41E9C0(app, hDlg);  // 0x41E9C0
        return 0;
    }
    if (msg != WM_COMMAND) {
        return 0;
    }
    const WORD id = LOWORD(wParam);
    if (id == 1) {
        if (Sub41EA20(hDlg) != 0) {  // 0x41EA20
            EndDialog(hDlg, 1);
        } else {
            EndDialog(hDlg, 2);
        }
        return 0;
    }
    if (id == 2) {
        EndDialog(hDlg, 2);
        return 0;
    }
    return 0;
}

// ===========================================================================
// 0x464BD0 - case 259 model-edge dialog (0x29A EN / 0x299 JP), modeless at
// app+0xA0B50.  WM_INITDIALOG: subclass edits 667/672/676/680 with
// sub_45ECC0 (old proc of 667 saved at app+0xA0B54), sub_43C430 init.
// WM_COMMAND: sets app+0xA0B6C = 1.  id 1/2 close (sub_45F240 apply,
// DestroyWindow, app+0xA0B50 = 0, re-enable main-window combo 0x1B4; id 1
// additionally shows the "preserve edit result" box and sets app+0xA0B64).
// ids 0x29E/0x29F/0x2A2/0x2A3/0x2A6/679 are the prev/next buttons of the
// three combos 669/673/677 (CB_SETCURSEL wrap-around + refresh helper).
// CBN_SELCHANGE of 669/673/677 refreshes the matching combo helper.
// Reference: MikuMikuDance.exe sub_464BD0.
// ===========================================================================
INT_PTR CALLBACK Sub464BD0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    MMDApp* app = g_Block;
    const HWND hCtrl = reinterpret_cast<HWND>(lParam);
    if (msg == WM_INITDIALOG) {
        app->raw<std::int32_t>(kDwordA0B54) =
            GetWindowLongPtrA(GetDlgItem(hDlg, 667), GWLP_WNDPROC);
        SetWindowLongPtrA(GetDlgItem(hDlg, 667), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)Sub45ECC0);
        SetWindowLongPtrA(GetDlgItem(hDlg, 672), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)Sub45ECC0);
        SetWindowLongPtrA(GetDlgItem(hDlg, 676), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)Sub45ECC0);
        SetWindowLongPtrA(GetDlgItem(hDlg, 680), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)Sub45ECC0);
        Sub43C430(hDlg);  // 0x43C430
        return 0;
    }
    if (msg != WM_COMMAND) {
        return 0;
    }
    app->raw<std::int32_t>(kDwordA0B6C) = 1;
    switch (LOWORD(wParam)) {
    case 1:
    case 2:
        Sub45F240(hDlg);  // 0x45F240
        DestroyWindow(hDlg);
        app->state.a0B50OrPtr = 0;
        EnableWindow(GetDlgItem(app->raw<HWND>(kDwordA06B8), 0x1B4), TRUE);
        if (LOWORD(wParam) == 1) {
            if (app->state.englishUI != 0) {
                MessageBoxA(app->raw<HWND>(kDwordA06B8),
                            "Please preserve the edit result as a new model by "
                            "'save enhanced model'.",
                            "enhance model", 0);
            } else {
                MessageBoxA(app->raw<HWND>(kDwordA06B8), kMsgEnhanceModelJp,
                            kCaptionEnhanceModelJp, 0);
            }
            app->EnhancedModelDirty() = 1;
        }
        return 1;
    case 0x29E: {  // combo 669 next button
        const int cur = app->raw<std::int32_t>(kDwordA0B58);
        const int count = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 669), 0x146 /*CB_GETCOUNT*/, 0, 0));
        SendMessageA(GetDlgItem(hDlg, 669), 0x14E /*CB_SETCURSEL*/,
                     cur + 1 >= count ? 0 : cur + 1, 0);
        Sub45F050(hDlg);  // 0x45F050
        return 0;
    }
    case 0x29F: {  // combo 669 prev button
        int sel = app->raw<std::int32_t>(kDwordA0B58) - 1;
        if (sel < 0) {
            sel = static_cast<int>(
                      SendMessageA(GetDlgItem(hDlg, 669), 0x146 /*CB_GETCOUNT*/,
                                   0, 0)) -
                  1;
        }
        SendMessageA(GetDlgItem(hDlg, 669), 0x14E /*CB_SETCURSEL*/, sel, 0);
        Sub45F050(hDlg);  // 0x45F050 (LABEL_16)
        return 0;
    }
    case 0x2A2: {  // combo 673 next button
        const int cur = app->raw<std::int32_t>(kDwordA0B5C);
        const int count = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 673), 0x146 /*CB_GETCOUNT*/, 0, 0));
        SendMessageA(GetDlgItem(hDlg, 673), 0x14E /*CB_SETCURSEL*/,
                     cur + 1 >= count ? 0 : cur + 1, 0);
        Sub45EF10(hDlg);  // 0x45EF10
        return 0;
    }
    case 0x2A3: {  // combo 673 prev button
        int sel = app->raw<std::int32_t>(kDwordA0B5C) - 1;
        if (sel < 0) {
            sel = static_cast<int>(
                      SendMessageA(GetDlgItem(hDlg, 673), 0x146 /*CB_GETCOUNT*/,
                                   0, 0)) -
                  1;
        }
        SendMessageA(GetDlgItem(hDlg, 673), 0x14E /*CB_SETCURSEL*/, sel, 0);
        Sub45EF10(hDlg);  // 0x45EF10 (LABEL_23)
        return 0;
    }
    case 0x2A6: {  // combo 677 next button
        const int cur = app->raw<std::int32_t>(kDwordA0B60);
        const int count = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 677), 0x146 /*CB_GETCOUNT*/, 0, 0));
        SendMessageA(GetDlgItem(hDlg, 677), 0x14E /*CB_SETCURSEL*/,
                     cur + 1 >= count ? 0 : cur + 1, 0);
        Sub45EDC0(hDlg);  // 0x45EDC0
        return 0;
    }
    case 679: {  // combo 677 prev button
        int sel = app->raw<std::int32_t>(kDwordA0B60) - 1;
        if (sel < 0) {
            sel = static_cast<int>(
                      SendMessageA(GetDlgItem(hDlg, 677), 0x146 /*CB_GETCOUNT*/,
                                   0, 0)) -
                  1;
        }
        SendMessageA(GetDlgItem(hDlg, 677), 0x14E /*CB_SETCURSEL*/, sel, 0);
        Sub45EDC0(hDlg);  // 0x45EDC0
        return 0;
    }
    default:
        break;
    }
    if (HIWORD(wParam) == 1 /*CBN_SELCHANGE*/) {
        if (hCtrl == GetDlgItem(hDlg, 669)) {
            Sub45F050(hDlg);  // 0x45F050 (LABEL_16)
            return 0;
        }
        if (hCtrl == GetDlgItem(hDlg, 673)) {
            Sub45EF10(hDlg);  // 0x45EF10 (LABEL_23)
            return 0;
        }
        if (hCtrl == GetDlgItem(hDlg, 677)) {
            Sub45EDC0(hDlg);  // 0x45EDC0
        }
    }
    return 0;
}

// ===========================================================================
// 0x465020 - case 262 frame-edit dialog (0x2AC EN / 0x2AB JP), modeless at
// app+0xA0B74.  WM_INITDIALOG: subclass edit 705 + edits 709..723 +
// 744..767 + 740 with sub_41EC50 (old proc of 705 saved at app+0xA0B78),
// sub_45F670 init.
// WM_COMMAND (app+0xA0B6C = 1 first):
//   0x2AD/0x2DF   camera/bone combo "select" buttons (collect + refresh)
//   0x2C2/0x2E1   delete the selected camera/bone frame: CB_DELETESTRING and
//                 renumber every frame reference in the 172-byte camera
//                 records (fields +84/+256/+428/+600/+772 per 860-byte
//                 group) resp. 140-byte bone records (+132/+272/+412/+552/
//                 +692 per 700-byte group); -1 marks deleted slots
//   0x2D4/0x2D5   camera frame copy to / restore from scratch app+0xA0B80
//   0x2E2/0x2E3   bone frame copy to / restore from scratch app+0xA0C34
//   0x2D6..0x2D8  bone frame +36 mode 0/1/2
//   0x2D9..0x2DB  checkbox 731: record +80/+81 flags
//   0x320/0x321   camera/bone list mode flip
//   id 1/2        close: free the frame buffers (app+0xA0B7C/0xA0C30),
//                 DestroyWindow, app+0xA0B74 = 0, re-enable main-window
//                 combos 0x1B4/0x198; id 1 also shows the preserve box and
//                 sets app+0xA0B64 = 1
// CBN_SELCHANGE of 704/736/741/742/707/708/743: link combo selection to the
// frame arrays (helpers sub_43CCD0/sub_4214A0/sub_420DC0).
// Reference: MikuMikuDance.exe sub_465020.
// ===========================================================================
INT_PTR CALLBACK Sub465020(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    MMDApp* app = g_Block;
    const HWND hCtrl = reinterpret_cast<HWND>(lParam);
    if (msg == WM_INITDIALOG) {
        if (app->state.floatingWindow != 0) {
            SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
        }
        app->raw<std::int32_t>(kDwordA0B78) =
            GetWindowLongPtrA(GetDlgItem(hDlg, 705), GWLP_WNDPROC);
        SetWindowLongPtrA(GetDlgItem(hDlg, 705), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)Sub41EC50);
        for (int i = 709; i <= 723; ++i) {
            SetWindowLongPtrA(GetDlgItem(hDlg, i), GWLP_WNDPROC,
                           (LONG)(LONG_PTR)Sub41EC50);
        }
        for (int j = 744; j <= 767; ++j) {
            SetWindowLongPtrA(GetDlgItem(hDlg, j), GWLP_WNDPROC,
                           (LONG)(LONG_PTR)Sub41EC50);
        }
        SetWindowLongPtrA(GetDlgItem(hDlg, 740), GWLP_WNDPROC,
                       (LONG)(LONG_PTR)Sub41EC50);
        Sub45F670(hDlg);  // 0x45F670
        return 0;
    }
    if (msg != WM_COMMAND) {
        return 0;
    }
    app->raw<std::int32_t>(kDwordA0B6C) = 1;
    switch (LOWORD(wParam)) {
    case 0x2AD:  // camera combo "select"
        Sub41FF30(app);   // 0x41FF30
        Sub45F480(hDlg);  // 0x45F480
        return 0;
    case 0x2DF:  // bone combo "select"
        Sub4204F0(app);   // 0x4204F0
        Sub43CA50(hDlg);  // 0x43CA50
        return 0;
    case 0x2C2: {  // delete camera frame (0x4650C6)
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 704), 0x147 /*CB_GETCURSEL*/, 0, 0));
        SendMessageA(GetDlgItem(hDlg, 704), 0x144 /*CB_DELETESTRING*/, sel, 0);
        SendMessageA(GetDlgItem(hDlg, 741), 0x144 /*CB_DELETESTRING*/, sel, 0);
        SendMessageA(GetDlgItem(hDlg, 742), 0x144 /*CB_DELETESTRING*/, sel, 0);
        unsigned char* cam = app->raw<unsigned char*>(kDwordA0B7C);
        const std::int32_t curCam = app->raw<std::int32_t>(kDwordA0C2C);
        for (std::int32_t k = 0; k < 1400000; k += 140) {
            if (*reinterpret_cast<std::int32_t*>(cam + k + 132) >= 0) {
                if (*reinterpret_cast<std::int32_t*>(cam + k + 28) == curCam ||
                    *reinterpret_cast<std::int32_t*>(cam + k + 32) == curCam) {
                    SendMessageA(GetDlgItem(hDlg, 736), 0x144 /*CB_DELETESTRING*/,
                                 *reinterpret_cast<std::int32_t*>(cam + k + 132),
                                 0);
                    for (std::int32_t m = 0; m < 1400000; m += 700) {
                        std::int32_t* f;
                        f = reinterpret_cast<std::int32_t*>(cam + m + 132);
                        if (*f > *reinterpret_cast<std::int32_t*>(cam + k + 132))
                            --*f;
                        f = reinterpret_cast<std::int32_t*>(cam + m + 272);
                        if (*f > *reinterpret_cast<std::int32_t*>(cam + k + 132))
                            --*f;
                        f = reinterpret_cast<std::int32_t*>(cam + m + 412);
                        if (*f > *reinterpret_cast<std::int32_t*>(cam + k + 132))
                            --*f;
                        f = reinterpret_cast<std::int32_t*>(cam + m + 552);
                        if (*f > *reinterpret_cast<std::int32_t*>(cam + k + 132))
                            --*f;
                        f = reinterpret_cast<std::int32_t*>(cam + m + 692);
                        if (*f > *reinterpret_cast<std::int32_t*>(cam + k + 132))
                            --*f;
                    }
                    *reinterpret_cast<std::int32_t*>(cam + k + 132) = -1;
                }
            }
        }
        *reinterpret_cast<std::int32_t*>(cam + 172 * curCam + 84) = -1;
        for (std::int32_t n = 0; n < 1720000; n += 860) {
            std::int32_t* f;
            f = reinterpret_cast<std::int32_t*>(cam + n + 84);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(cam + n + 256);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(cam + n + 428);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(cam + n + 600);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(cam + n + 772);
            if (*f > sel) --*f;
        }
        if (SendMessageA(GetDlgItem(hDlg, 704), 0x146 /*CB_GETCOUNT*/, 0, 0) !=
            0) {
            if (*reinterpret_cast<std::int32_t*>(cam + 84) == 0) {
                app->raw<std::int32_t>(kDwordA0C2C) = 0;
            }
        } else {
            app->raw<std::int32_t>(kDwordA0C2C) = -1;
        }
        SendMessageA(GetDlgItem(hDlg, 704), 0x14E /*CB_SETCURSEL*/, 0, 0);
        Sub43CCD0(hDlg, app->raw<std::int32_t>(kDwordA0C2C));  // 0x43CCD0
        mikudancestudio::mdl::BoneRecord* bone =
            reinterpret_cast<mikudancestudio::mdl::BoneRecord*>(
                app->raw<unsigned char*>(kDwordA0C30));
        if (SendMessageA(GetDlgItem(hDlg, 736), 0x146 /*CB_GETCOUNT*/, 0, 0) !=
            0) {
            if (*reinterpret_cast<std::int32_t*>(bone + 132) == 0) {
                app->raw<std::int32_t>(kDwordA0CC0) = 0;
            }
        } else {
            app->raw<std::int32_t>(kDwordA0CC0) = -1;
        }
        // LABEL_46 (0x465407)
        SendMessageA(GetDlgItem(hDlg, 736), 0x14E /*CB_SETCURSEL*/, 0, 0);
        Sub4214A0(hDlg, app->raw<std::int32_t>(kDwordA0CC0));  // 0x4214A0
        return 0;
    }
    case 0x2E1: {  // delete bone frame (0x46544D)
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 736), 0x147 /*CB_GETCURSEL*/, 0, 0));
        SendMessageA(GetDlgItem(hDlg, 736), 0x144 /*CB_DELETESTRING*/, sel, 0);
        mikudancestudio::mdl::BoneRecord* bone =
            reinterpret_cast<mikudancestudio::mdl::BoneRecord*>(
                app->raw<unsigned char*>(kDwordA0C30));
        *reinterpret_cast<std::int32_t*>(
            bone + 140 * app->raw<std::int32_t>(kDwordA0CC0) + 132) = -1;
        for (std::int32_t ii = 0; ii < 1400000; ii += 700) {
            std::int32_t* f;
            f = reinterpret_cast<std::int32_t*>(bone + ii + 132);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(bone + ii + 272);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(bone + ii + 412);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(bone + ii + 552);
            if (*f > sel) --*f;
            f = reinterpret_cast<std::int32_t*>(bone + ii + 692);
            if (*f > sel) --*f;
        }
        if (SendMessageA(GetDlgItem(hDlg, 736), 0x146 /*CB_GETCOUNT*/, 0, 0) !=
            0) {
            if (*reinterpret_cast<std::int32_t*>(bone + 132) == 0) {
                app->raw<std::int32_t>(kDwordA0CC0) = 0;
            }
        } else {
            app->raw<std::int32_t>(kDwordA0CC0) = -1;
        }
        // LABEL_46 (0x465407)
        SendMessageA(GetDlgItem(hDlg, 736), 0x14E /*CB_SETCURSEL*/, 0, 0);
        Sub4214A0(hDlg, app->raw<std::int32_t>(kDwordA0CC0));  // 0x4214A0
        return 0;
    }
    case 0x320:  // bone list mode on
        Sub4204F0(app);                 // 0x4204F0
        Sub421C20(hDlg, 1);             // 0x421C20
        return 0;
    case 0x321:  // bone list mode off
        Sub41FF30(app);                 // 0x41FF30
        Sub421C20(hDlg, 0);             // 0x421C20
        return 0;
    case 0x2D4: {  // copy camera frame to scratch (0x46564E)
        Sub41FF30(app);  // 0x41FF30
        std::memcpy(app->at(kScratchCam),
                    app->raw<unsigned char*>(kDwordA0B7C) +
                        172 * app->raw<std::int32_t>(kDwordA0C2C),
                    0xAC);
        return 0;
    }
    case 0x2D5: {  // restore camera frame from scratch (0x465687)
        unsigned char* rec = app->raw<unsigned char*>(kDwordA0B7C) +
                             172 * app->raw<std::int32_t>(kDwordA0C2C);
        *reinterpret_cast<std::int32_t*>(rec + 28) =
            app->raw<std::int32_t>(kScratchCam + 28);
        rec[80] = app->raw<std::uint8_t>(kScratchCam + 80);
        rec[81] = app->raw<std::uint8_t>(kScratchCam + 81);
        *reinterpret_cast<float*>(rec + 76) =
            app->raw<float>(kScratchCam + 76);
        *reinterpret_cast<float*>(rec + 88) =
            app->raw<float>(kScratchCam + 88);
        std::memcpy(rec + 52, app->at(kScratchCam) + 52, 12);
        *reinterpret_cast<float*>(rec + 92) =
            app->raw<float>(kScratchCam + 92);
        std::memcpy(rec + 64, app->at(kScratchCam) + 64, 12);
        rec[36] = app->raw<std::uint8_t>(kScratchCam + 36);
        std::memcpy(rec + 40, app->at(kScratchCam) + 40, 12);
        Sub43CCD0(hDlg, app->raw<std::int32_t>(kDwordA0C2C));  // 0x43CCD0
        return 0;
    }
    case 0x2E2: {  // copy bone frame to scratch (0x465636)
        Sub4204F0(app);  // 0x4204F0
        std::memcpy(app->at(kScratchBone),
                    app->raw<unsigned char*>(kDwordA0C30) +
                        140 * app->raw<std::int32_t>(kDwordA0CC0),
                    0x8C);
        return 0;
    }
    case 0x2E3: {  // restore bone frame from scratch (0x465863)
        unsigned char* rec = app->raw<unsigned char*>(kDwordA0C30) +
                             140 * app->raw<std::int32_t>(kDwordA0CC0);
        *reinterpret_cast<std::int32_t*>(rec + 28) =
            app->raw<std::int32_t>(kScratchBone + 28);
        *reinterpret_cast<std::int32_t*>(rec + 32) =
            app->raw<std::int32_t>(kScratchBone + 32);
        std::memcpy(rec + 72, app->at(kScratchBone) + 72, 12);
        std::memcpy(rec + 36, app->at(kScratchBone) + 36, 12);
        std::memcpy(rec + 108, app->at(kScratchBone) + 108, 12);
        std::memcpy(rec + 60, app->at(kScratchBone) + 60, 12);
        std::memcpy(rec + 96, app->at(kScratchBone) + 96, 12);
        std::memcpy(rec + 48, app->at(kScratchBone) + 48, 12);
        std::memcpy(rec + 120, app->at(kScratchBone) + 120, 12);
        std::memcpy(rec + 84, app->at(kScratchBone) + 84, 12);
        Sub4214A0(hDlg, app->raw<std::int32_t>(kDwordA0CC0));  // 0x4214A0
        return 0;
    }
    case 0x2D6:  // bone frame mode 0
        app->raw<unsigned char*>(kDwordA0B7C)
            [172 * app->raw<std::int32_t>(kDwordA0C2C) + 36] = 0;
        Sub421CE0(hDlg, 0, app->raw<std::int32_t>(kDwordA0C2C));  // 0x421CE0
        return 0;
    case 0x2D7:  // bone frame mode 1
        app->raw<unsigned char*>(kDwordA0B7C)
            [172 * app->raw<std::int32_t>(kDwordA0C2C) + 36] = 1;
        Sub421CE0(hDlg, 1, app->raw<std::int32_t>(kDwordA0C2C));  // 0x421CE0
        return 0;
    case 0x2D8:  // bone frame mode 2
        app->raw<unsigned char*>(kDwordA0B7C)
            [172 * app->raw<std::int32_t>(kDwordA0C2C) + 36] = 2;
        Sub421CE0(hDlg, 2, app->raw<std::int32_t>(kDwordA0C2C));  // 0x421CE0
        return 0;
    case 0x2D9: {  // checkbox 731 checked (0x465B3C)
        EnableWindow(GetDlgItem(hDlg, 731), TRUE);
        unsigned char* rec = app->raw<unsigned char*>(kDwordA0B7C) +
                             172 * app->raw<std::int32_t>(kDwordA0C2C);
        if (IsDlgButtonChecked(hDlg, 731) == 1) {
            rec[80] = 2;
            rec[81] = 1;  // LABEL_95
        } else {
            rec[80] = 1;
            rec[81] = 0;  // LABEL_87
        }
        return 0;
    }
    case 0x2DA: {  // checkbox 731 unchecked (0x465BB6)
        EnableWindow(GetDlgItem(hDlg, 731), FALSE);
        unsigned char* rec = app->raw<unsigned char*>(kDwordA0B7C) +
                             172 * app->raw<std::int32_t>(kDwordA0C2C);
        rec[80] = 0;
        rec[81] = 0;
        return 0;
    }
    case 0x2DB: {  // checkbox 731 tri-state click (0x465C14)
        unsigned char* rec = app->raw<unsigned char*>(kDwordA0B7C) +
                             172 * app->raw<std::int32_t>(kDwordA0C2C);
        if (rec[80] == 2) {
            rec[80] = 1;
            rec[81] = 0;  // LABEL_87
        } else if (rec[80] == 1) {
            rec[80] = 2;
            rec[81] = 1;  // LABEL_95
        }
        return 0;
    }
    default:
        break;
    }
    if (HIWORD(wParam) != 1 /*CBN_SELCHANGE*/) {
        if (LOWORD(wParam) == 1) {
            // OK: free buffers, close, re-enable, preserve box (0x4660C1)
            Sub4220C0(app);     // 0x4220C0 (stubs.cpp)
            Sub41FF30(app);     // 0x41FF30
            Sub4204F0(app);     // 0x4204F0
            Sub4220F0(hDlg);    // 0x4220F0
            if (app->raw<std::int32_t>(kDwordA0B7C) != 0) {
                std::free(app->raw<void*>(kDwordA0B7C));
                app->raw<std::int32_t>(kDwordA0B7C) = 0;
            }
            if (app->raw<std::int32_t>(kDwordA0C30) != 0) {
                std::free(app->raw<void*>(kDwordA0C30));
                app->raw<std::int32_t>(kDwordA0C30) = 0;
            }
            DestroyWindow(hDlg);
            app->raw<std::int32_t>(kDwordA0B74) = 0;
            EnableWindow(GetDlgItem(app->raw<HWND>(kDwordA06B8), 0x1B4), TRUE);
            EnableWindow(GetDlgItem(app->raw<HWND>(kDwordA06B8), 0x198), TRUE);
            if (app->state.englishUI != 0) {
                MessageBoxA(app->raw<HWND>(kDwordA06B8),
                            "Please preserve the edit result as a new model by "
                            "'save enhanced model'.",
                            "enhance model", 0);
            } else {
                MessageBoxA(app->raw<HWND>(kDwordA06B8), kMsgEnhanceModelJp,
                            kCaptionEnhanceModelJp, 0);
            }
            app->EnhancedModelDirty() = 1;
            return 1;
        }
        if (LOWORD(wParam) == 2) {
            // Cancel (0x4661E9)
            Sub4220C0(app);  // 0x4220C0 (stubs.cpp)
            if (app->raw<std::int32_t>(kDwordA0B7C) != 0) {
                std::free(app->raw<void*>(kDwordA0B7C));
                app->raw<std::int32_t>(kDwordA0B7C) = 0;
            }
            if (app->raw<std::int32_t>(kDwordA0C30) != 0) {
                std::free(app->raw<void*>(kDwordA0C30));
                app->raw<std::int32_t>(kDwordA0C30) = 0;
            }
            DestroyWindow(hDlg);
            app->raw<std::int32_t>(kDwordA0B74) = 0;
            EnableWindow(GetDlgItem(app->raw<HWND>(kDwordA06B8), 0x1B4), TRUE);
            EnableWindow(GetDlgItem(app->raw<HWND>(kDwordA06B8), 0x198), TRUE);
            return 0;
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 704)) {
        // camera combo selection (0x465C8A)
        Sub41FF30(app);  // 0x41FF30
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 704), 0x147 /*CB_GETCURSEL*/, 0, 0));
        unsigned char* cam = app->raw<unsigned char*>(kDwordA0B7C);
        for (int idx = 0; idx * 172 < 1720000; ++idx) {
            if (*reinterpret_cast<std::int32_t*>(cam + 172 * idx + 84) == sel) {
                Sub43CCD0(hDlg, idx);  // 0x43CCD0
                app->raw<std::int32_t>(kDwordA0C2C) = idx;
            }
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 736)) {
        // bone combo selection (0x465D0E)
        Sub4204F0(app);  // 0x4204F0
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 736), 0x147 /*CB_GETCURSEL*/, 0, 0));
        mikudancestudio::mdl::BoneRecord* bone =
            reinterpret_cast<mikudancestudio::mdl::BoneRecord*>(
                app->raw<unsigned char*>(kDwordA0C30));
        for (int idx = 0; idx * 140 < 1400000; ++idx) {
            if (*reinterpret_cast<std::int32_t*>(bone + 140 * idx + 132) == sel) {
                Sub4214A0(hDlg, idx);  // 0x4214A0
                app->raw<std::int32_t>(kDwordA0CC0) = idx;
            }
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 741)) {
        // camera "from" combo (0x465D91): map sel -> record frame field +28
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 741), 0x147 /*CB_GETCURSEL*/, 0, 0));
        unsigned char* cam = app->raw<unsigned char*>(kDwordA0B7C);
        mikudancestudio::mdl::BoneRecord* bone =
            reinterpret_cast<mikudancestudio::mdl::BoneRecord*>(
                app->raw<unsigned char*>(kDwordA0C30));
        unsigned char* target =
            reinterpret_cast<unsigned char*>(
                bone + app->raw<std::int32_t>(kDwordA0CC0)) + 28;
        for (int grp = 0, v = 2; v - 2 < 10000; ++grp, v += 5) {
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 84) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v - 2;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 256) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v - 1;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 428) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 600) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v + 1;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 772) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v + 2;
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 742)) {
        // camera "to" combo (0x465EC5): same mapping into record field +32
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 742), 0x147 /*CB_GETCURSEL*/, 0, 0));
        unsigned char* cam = app->raw<unsigned char*>(kDwordA0B7C);
        mikudancestudio::mdl::BoneRecord* bone =
            reinterpret_cast<mikudancestudio::mdl::BoneRecord*>(
                app->raw<unsigned char*>(kDwordA0C30));
        unsigned char* target =
            reinterpret_cast<unsigned char*>(
                bone + app->raw<std::int32_t>(kDwordA0CC0)) + 32;
        for (int grp = 0, v = 2; v - 2 < 10000; ++grp, v += 5) {
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 84) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v - 2;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 256) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v - 1;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 428) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 600) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v + 1;
            if (*reinterpret_cast<std::int32_t*>(cam + 860 * grp + 772) == sel)
                *reinterpret_cast<std::int32_t*>(target) = v + 2;
        }
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 707)) {
        // camera start-frame combo (0x46600B)
        unsigned char* cam = app->raw<unsigned char*>(kDwordA0B7C);
        *reinterpret_cast<std::int32_t*>(
            cam + 172 * app->raw<std::int32_t>(kDwordA0C2C) + 28) =
            static_cast<std::uint16_t>(
                SendMessageA(GetDlgItem(hDlg, 707), 0x147 /*CB_GETCURSEL*/, 0, 0)) -
            1;
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 708)) {
        // camera end-frame combo (0x466059)
        unsigned char* cam = app->raw<unsigned char*>(kDwordA0B7C);
        cam[172 * app->raw<std::int32_t>(kDwordA0C2C) + 32] =
            static_cast<std::uint8_t>(
                SendMessageA(GetDlgItem(hDlg, 708), 0x147 /*CB_GETCURSEL*/, 0, 0));
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 743)) {
        Sub420DC0(hDlg);  // 0x420DC0
        return 0;
    }
    return 0;
}

// ===========================================================================
// 0x42E370 - case 289 frame-reorder dialog (0x32B EN / 0x32A JP), modal.
// WM_INITDIALOG: top-most gate; g_dword545930 = CB_GETCOUNT(main combo
// 0x1B4) - 1; sub_41E810 fills listbox 628 from the int array at
// app+0xA0B1C.  WM_COMMAND ids 630/631 move the selected listbox entry up/
// down (LB_GETTEXT/DELETESTRING/INSERTSTRING/SETCURSEL) and swap the two
// adjacent dwords of the app+0xA0B1C array; id 632 (OK) applies via
// sub_41E910 + EndDialog(1), id 2 -> EndDialog(2); both free the array.
// Reference: MikuMikuDance.exe sub_42E370.
// ===========================================================================
INT_PTR CALLBACK Sub42E370(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    MMDApp* app = g_Block;
    if (msg == WM_INITDIALOG) {
        if (app->state.floatingWindow != 0) {
            SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
        }
        g_dword545930 = static_cast<int>(
            SendMessageA(GetDlgItem(app->raw<HWND>(kDwordA06B8), 0x1B4),
                         0x146 /*CB_GETCOUNT*/, 0, 0)) -
            1;
        Sub41E810(g_dword545930, hDlg);  // 0x41E810
        return 0;
    }
    if (msg != WM_COMMAND) {
        return 0;
    }
    const WORD id = LOWORD(wParam);
    if (id == 630) {  // move up (0x42E3A6)
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 628), 0x188 /*LB_GETCURSEL*/, 0, 0));
        if (sel >= 1) {
            char buf[100];
            SendMessageA(GetDlgItem(hDlg, 628), 0x189 /*LB_GETTEXT*/, sel,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, 628), 0x182 /*LB_DELETESTRING*/, sel,
                         0);
            SendMessageA(GetDlgItem(hDlg, 628), 0x181 /*LB_INSERTSTRING*/,
                         sel - 1, (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, 628), 0x186 /*LB_SETCURSEL*/, sel - 1,
                         0);
            std::int32_t* arr = app->raw<std::int32_t*>(kDwordA0B1C);
            const std::int32_t tmp = arr[sel - 1];
            arr[sel - 1] = arr[sel];
            arr[sel] = tmp;
        }
        return 0;
    }
    if (id == 631) {  // move down (0x42E451)
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 628), 0x188 /*LB_GETCURSEL*/, 0, 0));
        if (sel != -1 && sel < g_dword545930 - 1) {
            char buf[100];
            SendMessageA(GetDlgItem(hDlg, 628), 0x189 /*LB_GETTEXT*/, sel,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, 628), 0x182 /*LB_DELETESTRING*/, sel,
                         0);
            SendMessageA(GetDlgItem(hDlg, 628), 0x181 /*LB_INSERTSTRING*/,
                         sel + 1, (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, 628), 0x186 /*LB_SETCURSEL*/, sel + 1,
                         0);
            std::int32_t* arr = app->raw<std::int32_t*>(kDwordA0B1C);
            const std::int32_t tmp = arr[sel + 1];
            arr[sel + 1] = arr[sel];
            arr[sel] = tmp;
        }
        return 0;
    }
    if (id == 632) {  // OK (0x42E52E)
        Sub41E910(app, g_dword545930, hDlg);  // 0x41E910
        EndDialog(hDlg, 1);
        if (app->raw<std::int32_t>(kDwordA0B1C) == 0) {
            return 0;
        }
        std::free(app->raw<void*>(kDwordA0B1C));
        app->raw<std::int32_t>(kDwordA0B1C) = 0;
        return 0;
    }
    if (id == 2) {  // Cancel (0x42E57E)
        EndDialog(hDlg, 2);
        if (app->raw<std::int32_t>(kDwordA0B1C) == 0) {
            return 0;
        }
        std::free(app->raw<void*>(kDwordA0B1C));
        app->raw<std::int32_t>(kDwordA0B1C) = 0;
        return 0;
    }
    return 0;
}

// ===========================================================================
// 0x479E90 - case 266 accessory-frame dialog (0x323 EN / 0x322 JP), modeless
// at app+0xA0CCC.  WM_INITDIALOG: top-most gate; subclass edits 709..713
// with sub_466370 (old proc of 709 saved at app+0xA0C50); sub_423160 init.
// WM_COMMAND: checkbox 731 mirrors app+0xA0CD4 and enables edit 713;
// EN_UPDATE on 709..713 -> atof + sub_45FD80(idx, f) (710..712 also sync
// their trackbars 637..639); id 1 -> sub_460080; id 2 -> DestroyWindow,
// app+0xA0CCC = 0, sub_412330, return 1.  WM_HSCROLL on trackbars
// 637/638/639 -> TBM_GETPOS/100 -> "%3.2f" into 710/711/712 + sub_45FD80.
// Reference: MikuMikuDance.exe sub_479E90.
// ===========================================================================
INT_PTR CALLBACK Sub479E90(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    MMDApp* app = g_Block;
    const HWND hCtrl = reinterpret_cast<HWND>(lParam);
    if (msg == WM_INITDIALOG) {
        if (app->state.floatingWindow != 0) {
            SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
        }
        app->raw<std::int32_t>(kDwordA0C50) =
            GetWindowLongPtrA(GetDlgItem(hDlg, 709), GWLP_WNDPROC);
        for (int i = 709; i <= 713; ++i) {
            SetWindowLongPtrA(GetDlgItem(hDlg, i), GWLP_WNDPROC,
                           (LONG)(LONG_PTR)Sub466370);
        }
        Sub423160(hDlg);  // 0x423160
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 731) {
            if (IsDlgButtonChecked(hDlg, 731) == 1) {
                app->raw<std::uint8_t>(kByteA0CD4) = 1;
                EnableWindow(GetDlgItem(hDlg, 713), TRUE);
            } else {
                app->raw<std::uint8_t>(kByteA0CD4) = 0;
                EnableWindow(GetDlgItem(hDlg, 713), FALSE);
            }
        } else if (HIWORD(wParam) == 768 /*EN_UPDATE*/) {
            char buf[0x100];
            if (hCtrl == GetDlgItem(app->raw<HWND>(kDwordA0CCC), 709)) {
                GetWindowTextA(hCtrl, buf, 256);
                Sub45FD80(app, 0, static_cast<float>(atof(buf)));  // 0x45FD80
            } else if (hCtrl == GetDlgItem(app->raw<HWND>(kDwordA0CCC), 710)) {
                GetWindowTextA(hCtrl, buf, 256);
                const float f = static_cast<float>(atof(buf));
                SendMessageA(GetDlgItem(app->raw<HWND>(kDwordA0CCC), 637),
                             0x405 /*TBM_SETPOS*/, 1, (int)(f * 100.0));
                Sub45FD80(app, 1, f);  // 0x45FD80
            } else if (hCtrl == GetDlgItem(app->raw<HWND>(kDwordA0CCC), 711)) {
                GetWindowTextA(hCtrl, buf, 256);
                const float f = static_cast<float>(atof(buf));
                SendMessageA(GetDlgItem(app->raw<HWND>(kDwordA0CCC), 638),
                             0x405 /*TBM_SETPOS*/, 1, (int)(f * 100.0));
                Sub45FD80(app, 2, f);  // 0x45FD80
            } else if (hCtrl == GetDlgItem(app->raw<HWND>(kDwordA0CCC), 712)) {
                GetWindowTextA(hCtrl, buf, 256);
                const float f = static_cast<float>(atof(buf));
                SendMessageA(GetDlgItem(app->raw<HWND>(kDwordA0CCC), 639),
                             0x405 /*TBM_SETPOS*/, 1, (int)(f * 100.0));
                Sub45FD80(app, 3, f);  // 0x45FD80
            } else if (hCtrl == GetDlgItem(app->raw<HWND>(kDwordA0CCC), 713)) {
                GetWindowTextA(hCtrl, buf, 256);
                Sub45FD80(app, 4, static_cast<float>(atof(buf)));  // 0x45FD80
            }
        } else if (id == 1) {
            Sub460080(app);  // 0x460080
        } else if (id == 2) {
            DestroyWindow(hDlg);
            app->raw<std::int32_t>(kDwordA0CCC) = 0;
            Sub412330(app);  // 0x412330
            return 1;
        }
        return 0;
    }
    if (msg != WM_HSCROLL) {
        return 0;
    }
    if (hCtrl == GetDlgItem(hDlg, 637)) {
        const double v =
            static_cast<double>(
                SendMessageA(hCtrl, 0x400 /*TBM_GETPOS*/, 0, 0)) /
            100.0;
        char buf[0x100];
        sprintf_s(buf, 0x100, "%3.2f", v);
        SetWindowTextA(GetDlgItem(hDlg, 710), buf);
        Sub45FD80(app, 1, static_cast<float>(v));  // 0x45FD80
    } else if (hCtrl == GetDlgItem(hDlg, 638)) {
        const double v =
            static_cast<double>(
                SendMessageA(hCtrl, 0x400 /*TBM_GETPOS*/, 0, 0)) /
            100.0;
        char buf[0x100];
        sprintf_s(buf, 0x100, "%3.2f", v);
        SetWindowTextA(GetDlgItem(hDlg, 711), buf);
        Sub45FD80(app, 2, static_cast<float>(v));  // 0x45FD80
    } else if (hCtrl == GetDlgItem(hDlg, 639)) {
        const double v =
            static_cast<double>(
                SendMessageA(hCtrl, 0x400 /*TBM_GETPOS*/, 0, 0)) /
            100.0;
        char buf[0x100];
        sprintf_s(buf, 0x100, "%3.2f", v);
        SetWindowTextA(GetDlgItem(hDlg, 712), buf);
        Sub45FD80(app, 3, static_cast<float>(v));  // 0x45FD80
    }
    return 1;
}

// ===========================================================================
// 0x461CE0 - case 275 shadow/edge display dialog (0x32D EN / 0x32C JP).
// WM_INITDIALOG: CB_RESETCONTENT(669), CB_ADDSTRING "ON (X mark)" / JP
// "ON (X印)" then "OFF", CB_SETCURSEL(0).  WM_COMMAND: id 1 -> sub_4403C0
// (CB_GETCURSEL != 0) then EndDialog(1); id 2 -> EndDialog(1) as well (the
// caller of case 275 never inspects the result).
// Reference: MikuMikuDance.exe sub_461CE0.
// ===========================================================================
INT_PTR CALLBACK Sub461CE0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    if (msg == WM_INITDIALOG) {
        SendMessageA(GetDlgItem(hDlg, 669), 0x14B /*CB_RESETCONTENT*/, 0, 0);
        const char* first =
            g_Block->state.englishUI != 0
                ? "ON (X mark)"
                : kOnXMarkJp;
        SendMessageA(GetDlgItem(hDlg, 669), 0x143 /*CB_ADDSTRING*/, 0,
                     (LPARAM)first);
        SendMessageA(GetDlgItem(hDlg, 669), 0x143 /*CB_ADDSTRING*/, 0,
                     (LPARAM)kOff);
        SendMessageA(GetDlgItem(hDlg, 669), 0x14E /*CB_SETCURSEL*/, 0, 0);
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 1) {
            if (SendMessageA(GetDlgItem(hDlg, 669), 0x147 /*CB_GETCURSEL*/, 0,
                             0) != 0) {
                Sub4403C0(1);  // 0x4403C0
            } else {
                Sub4403C0(0);  // 0x4403C0
            }
            EndDialog(hDlg, 1);
            return 0;
        }
        if (id == 2) {
            EndDialog(hDlg, 1);
            return 0;
        }
    }
    return 0;
}

// ===========================================================================
// 0x40FBC0 - case 300 camera-rotation dialog (0x298 EN / 0x289 JP).
// WM_INITDIALOG: top-most gate; the seven temp floats app+0xA0B28..0xA0B40
// (set by the case-300 camera path: posx/y/z, -cam2.., cam3.., cam4..,
// -camangle) are written as "%7.3f" into edits 637..642 + 644 via
// EM_REPLACESEL; focus 637 + select all.
// WM_COMMAND: id 1 -> read all seven edits back into the temps (atof),
// EndDialog(1); id 2 -> EndDialog(2).  The caller converts back to radians.
// Reference: MikuMikuDance.exe sub_40FBC0.
// ===========================================================================
INT_PTR CALLBACK Sub40FBC0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    MMDApp* app = g_Block;
    if (msg == WM_INITDIALOG) {
        if (app->state.floatingWindow != 0) {
            SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
        }
        static const std::size_t kEditIds[7] = {637, 638, 639, 640, 641, 642, 644};
        static const std::size_t kFltOffs[7] = {kFltA0B28, kFltA0B2C, kFltA0B30,
                                                kFltA0B34, kFltA0B38, kFltA0B3C,
                                                kFltA0B40};
        char buf[20];
        for (int i = 0; i < 7; ++i) {
            sprintf_s(buf, 0x14, "%7.3f",
                      static_cast<double>(app->raw<float>(kFltOffs[i])));
            SendMessageA(GetDlgItem(hDlg, kEditIds[i]), 0xC2 /*EM_REPLACESEL*/,
                         0, (LPARAM)buf);
        }
        SetFocus(GetDlgItem(hDlg, 637));
        SendMessageA(GetDlgItem(hDlg, 637), 0xB1 /*EM_SETSEL*/, 0,
                     GetWindowTextLengthA(GetDlgItem(hDlg, 637)));
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 1) {
            static const std::size_t kEditIds[7] = {637, 638, 639, 640, 641, 642, 644};
            static const std::size_t kFltOffs[7] = {kFltA0B28, kFltA0B2C, kFltA0B30,
                                                    kFltA0B34, kFltA0B38, kFltA0B3C,
                                                    kFltA0B40};
            char buf[20];
            for (int i = 0; i < 7; ++i) {
                GetWindowTextA(GetDlgItem(hDlg, kEditIds[i]), buf, 20);
                app->raw<float>(kFltOffs[i]) =
                    static_cast<float>(atof(buf));
            }
            EndDialog(hDlg, 1);
        } else if (id == 2) {
            EndDialog(hDlg, 2);
        }
    }
    return 0;
}

// ===========================================================================
// 0x40F860 - case 300 bone-rotation dialog (0x297 EN / 0x288 JP).
// WM_INITDIALOG: top-most gate; the six temp floats app+0xA0B28..0xA0B3C
// (pos x/y/z + the three euler angles in degrees, set by the case-300 bone
// path) are written as "%7.3f" into edits 637..642; focus 637 + select all.
// WM_COMMAND: id 1 -> read the six edits back (atof), EndDialog(1);
// id 2 -> EndDialog(2).  The caller converts back to radians.
// Reference: MikuMikuDance.exe sub_40F860.
// ===========================================================================
INT_PTR CALLBACK Sub40F860(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    MMDApp* app = g_Block;
    if (msg == WM_INITDIALOG) {
        if (app->state.floatingWindow != 0) {
            SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
        }
        static const std::size_t kEditIds[6] = {637, 638, 639, 640, 641, 642};
        static const std::size_t kFltOffs[6] = {kFltA0B28, kFltA0B2C, kFltA0B30,
                                                kFltA0B34, kFltA0B38, kFltA0B3C};
        char buf[20];
        for (int i = 0; i < 6; ++i) {
            sprintf_s(buf, 0x14, "%7.3f",
                      static_cast<double>(app->raw<float>(kFltOffs[i])));
            SendMessageA(GetDlgItem(hDlg, kEditIds[i]), 0xC2 /*EM_REPLACESEL*/,
                         0, (LPARAM)buf);
        }
        SetFocus(GetDlgItem(hDlg, 637));
        SendMessageA(GetDlgItem(hDlg, 637), 0xB1 /*EM_SETSEL*/, 0,
                     GetWindowTextLengthA(GetDlgItem(hDlg, 637)));
        return 0;
    }
    if (msg == WM_COMMAND) {
        const WORD id = LOWORD(wParam);
        if (id == 1) {
            static const std::size_t kEditIds[6] = {637, 638, 639, 640, 641, 642};
            static const std::size_t kFltOffs[6] = {kFltA0B28, kFltA0B2C, kFltA0B30,
                                                    kFltA0B34, kFltA0B38, kFltA0B3C};
            char buf[20];
            for (int i = 0; i < 6; ++i) {
                GetWindowTextA(GetDlgItem(hDlg, kEditIds[i]), buf, 20);
                app->raw<float>(kFltOffs[i]) =
                    static_cast<float>(atof(buf));
            }
            EndDialog(hDlg, 1);
        } else if (id == 2) {
            EndDialog(hDlg, 2);
        }
    }
    return 0;
}

// ===========================================================================
// 0x4641F0 - case 288 frame-copy dialog (0x325 EN / 0x324 JP), modal.
// WM_INITDIALOG: top-most gate; g_dword545938 = CB_GETCOUNT(main combo
// 0x1B4); allocate the int array app+0xA0B1C; CB_GETLBTEXT(main combo 0x1B4,
// i) 1..count-1 into listbox 0x274; sub_41E7B0 init.
// WM_COMMAND ids 630/631: move the listbox 0x274 entry up/down and swap the
// adjacent dwords of the app+0xA0B1C array.  id 632 (OK): rebuild the main
// combos 0x1B4/0x1DA/0x1C1 (EN: "camera/light/accessory" / "ground" / "non",
// JP wide: "ｶﾒﾗ･照明･ｱｸｾｻﾘ" / "画面" / "なし") from the listbox contents
// (count-1 entries), CB_SETCURSEL(0x1B4, 0), sub_45EC80 apply,
// EndDialog(1).  id 2: EndDialog(2).  Both free app+0xA0B1C.
// Reference: MikuMikuDance.exe sub_4641F0 (decompiled by hand from the
// disassembly; Hex-Rays fails on this routine).
// ===========================================================================
INT_PTR CALLBACK Sub4641F0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    MMDApp* app = g_Block;
    if (msg == WM_INITDIALOG) {
        if (app->state.floatingWindow != 0) {
            SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, 3);
        }
        const int count = static_cast<int>(
            SendMessageA(GetDlgItem(app->raw<HWND>(kDwordA06B8), 0x1B4),
                         0x146 /*CB_GETCOUNT*/, 0, 0));
        g_dword545938 = count;
        app->raw<std::int32_t*>(kDwordA0B1C) = new std::int32_t[count];
        char buf[0x100];
        for (int i = 1; i < count; ++i) {
            SendMessageA(GetDlgItem(app->raw<HWND>(kDwordA06B8), 0x1B4),
                         0x148 /*CB_GETLBTEXT*/, i, (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x180 /*LB_ADDSTRING*/, 0,
                         (LPARAM)buf);
        }
        Sub41E7B0(app, count);  // 0x41E7B0
        return 0;
    }
    if (msg != WM_COMMAND) {
        return 0;
    }
    const WORD id = LOWORD(wParam);
    if (id == 630) {  // move up (0x464225)
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x188 /*LB_GETCURSEL*/, 0, 0));
        if (sel >= 1) {
            char buf[0x100];
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x189 /*LB_GETTEXT*/, sel,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x182 /*LB_DELETESTRING*/,
                         sel, 0);
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x181 /*LB_INSERTSTRING*/,
                         sel - 1, (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x186 /*LB_SETCURSEL*/,
                         sel - 1, 0);
            std::int32_t* arr = app->raw<std::int32_t*>(kDwordA0B1C);
            const std::int32_t tmp = arr[sel + 1];
            arr[sel + 1] = arr[sel];
            arr[sel] = tmp;
        }
        return 0;
    }
    if (id == 631) {  // move down (0x4642DE)
        const int sel = static_cast<int>(
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x188 /*LB_GETCURSEL*/, 0, 0));
        if (sel != -1 && sel < g_dword545938 - 1) {
            char buf[0x100];
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x189 /*LB_GETTEXT*/, sel,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x182 /*LB_DELETESTRING*/,
                         sel, 0);
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x181 /*LB_INSERTSTRING*/,
                         sel + 1, (LPARAM)buf);
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x186 /*LB_SETCURSEL*/,
                         sel + 1, 0);
            std::int32_t* arr = app->raw<std::int32_t*>(kDwordA0B1C);
            const std::int32_t tmp = arr[sel + 2];
            arr[sel + 2] = arr[sel + 1];
            arr[sel + 1] = tmp;
        }
        return 0;
    }
    if (id == 632) {  // OK (0x4643AD)
        HWND mainWnd = app->raw<HWND>(kDwordA06B8);
        SendMessageA(GetDlgItem(mainWnd, 0x1B4), 0x14B /*CB_RESETCONTENT*/, 0,
                     0);
        SendMessageA(GetDlgItem(mainWnd, 0x1DA), 0x14B /*CB_RESETCONTENT*/, 0,
                     0);
        SendMessageA(GetDlgItem(mainWnd, 0x1C1), 0x14B /*CB_RESETCONTENT*/, 0,
                     0);
        if (app->state.englishUI != 0) {
            SendMessageA(GetDlgItem(mainWnd, 0x1B4), 0x143 /*CB_ADDSTRING*/, 0,
                         (LPARAM)"camera/light/accessory");
            SendMessageA(GetDlgItem(mainWnd, 0x1DA), 0x143 /*CB_ADDSTRING*/, 0,
                         (LPARAM)"ground");
            SendMessageA(GetDlgItem(mainWnd, 0x1C1), 0x143 /*CB_ADDSTRING*/, 0,
                         (LPARAM)"non");
        } else {
            SendMessageW(GetDlgItem(mainWnd, 0x1B4), 0x143 /*CB_ADDSTRING*/, 0,
                         (LPARAM)kJpCamLightAcc);
            SendMessageW(GetDlgItem(mainWnd, 0x1DA), 0x143 /*CB_ADDSTRING*/, 0,
                         (LPARAM)kJpScreen);
            SendMessageW(GetDlgItem(mainWnd, 0x1C1), 0x143 /*CB_ADDSTRING*/, 0,
                         (LPARAM)kJpNone);
        }
        char buf[0x100];
        for (int i = 0; i < g_dword545938 - 1; ++i) {
            SendMessageA(GetDlgItem(hDlg, 0x274), 0x189 /*LB_GETTEXT*/, i,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(mainWnd, 0x1B4), 0x143 /*CB_ADDSTRING*/, 0,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(mainWnd, 0x1DA), 0x143 /*CB_ADDSTRING*/, 0,
                         (LPARAM)buf);
            SendMessageA(GetDlgItem(mainWnd, 0x1C1), 0x143 /*CB_ADDSTRING*/, 0,
                         (LPARAM)buf);
        }
        SendMessageA(GetDlgItem(mainWnd, 0x1B4), 0x14E /*CB_SETCURSEL*/, 0, 0);
        Sub45EC80(app, g_dword545938, hDlg);  // 0x45EC80
        EndDialog(hDlg, 1);
        if (app->raw<std::int32_t>(kDwordA0B1C) == 0) {
            return 0;
        }
        std::free(app->raw<void*>(kDwordA0B1C));
        app->raw<std::int32_t>(kDwordA0B1C) = 0;
        return 0;
    }
    if (id == 2) {  // Cancel (0x464613)
        EndDialog(hDlg, 2);
        if (app->raw<std::int32_t>(kDwordA0B1C) == 0) {
            return 0;
        }
        std::free(app->raw<void*>(kDwordA0B1C));
        app->raw<std::int32_t>(kDwordA0B1C) = 0;
        return 0;
    }
    return 0;
}

// ---- not-yet-ported original call targets with NO stub elsewhere -------
// (Sub439E40/43A650/43B720/43BB30 - the four frame-line edit commands -
//  moved to src/window/frame_line_edit.cpp; declared in ported_funcs.hpp.)
// VA 0x0040B5A0 - UI language refresh (thiscall, this = app).
void Sub40B5A0(MMDApp* app) { (void)app; /* TODO(port) */ }
// VA 0x0041EC10 - save enhanced model file (thiscall(app, wide path)).
void Sub41EC10(MMDApp* app, const wchar_t* path) {
    (void)app; (void)path; /* TODO(port) */
}
// VA 0x004629D0 - fullscreen enter/restore window manager; the real body
// now lives in src/app/avi_record_start.cpp (declared in ported_funcs.hpp).
// VA 0x004076E0 - locale refresh after toon reload (thiscall, this = locale).
void Sub4076E0(void* locale) { (void)locale; /* TODO(port) */ }
}  // namespace

// VA 0x004A4850 - set model color (thiscall(model, r, g, b)).
void Sub4A4850(MMDApp* model, int r, int g, int b) {
    (void)model; (void)r; (void)g; (void)b; /* TODO(port) */
}

namespace {

}  // namespace

// VA 0x0042AE20 - path copy: real port moved to src/media/media_load.cpp.

// ---- helpers ported in other translation units (declared here with their
//      original VAs; not yet registered in ported_funcs.hpp) --------------
void PanelPaint(MMDApp* app);                      // VA 0x00414610
void Sub42C810(MMDApp* app);                       // VA 0x0042C810 (stubs.cpp)
void AviBgOverlayRefresh(MMDApp* app);             // VA 0x004168D0 (bg_overlay.cpp)
void PicBgOverlayRefresh(MMDApp* app);             // VA 0x00417130 (bg_overlay.cpp)
void PostDeviceReset(MMDApp* app);             // VA 0x00440DB0 (device_reset.cpp)
void Sub4220C0(MMDApp* app);                       // VA 0x004220C0 (stubs.cpp)
void Sub42D6E0(MMDApp* app);                       // VA 0x0042D6E0 (500.cpp)
void WideToSjisPath(char* dst, const wchar_t* src, rsize_t size);  // 0x407910
void DisableKinect(MMDApp* app);                   // VA 0x0042A020 (oni_kinect.cpp)
void OpenNiInit(MMDApp* app, const char* sjisPath);// VA 0x00429CB0 (oni_kinect.cpp)

void CmdViewMenu(MMDApp* app, HWND hwnd, std::uint16_t id,
                 std::uint16_t notify) {
    (void)notify;  // see header: the default handler's notify gate can never
                   // match any id of this family
    HINSTANCE hInst = app->raw<HINSTANCE>(0);
    const bool english = app->state.englishUI != 0;

    switch (id) {
    // ------------------------------------------------------------------
    // 251 (0x0048A1CC): frame control dialog (0x294 EN / 0x27C JP).
    // Gate: app+0x2F8 must be clear; closes with 2 -> return, else the
    // default handler (no-op for menu ids).
    // ------------------------------------------------------------------
    case 251: {
        if (app->state.optflag0 != 0) {
            return;
        }
        app->state.bC = 1;
        if (DialogBoxParamA(hInst,
                            MAKEINTRESOURCEA(english ? 0x294 : 0x27C),
                            hwnd, Sub44D3F0, 0) == 2) {
            return;
        }
        break;
    }

    // ------------------------------------------------------------------
    // 252 (0x0048A219): dialog 0x295 EN / 0x283 JP (sub_44D510).
    // ------------------------------------------------------------------
    case 252: {
        if (app->state.optflag0 != 0) {
            return;
        }
        app->state.bC = 1;
        if (DialogBoxParamA(hInst,
                            MAKEINTRESOURCEA(english ? 0x295 : 0x283),
                            hwnd, Sub44D510, 0) == 2) {
            return;
        }
        break;
    }

    // ------------------------------------------------------------------
    // 253 (0x00489FC8): model-info dialog (0x296 EN / 0x285 JP,
    // sub_44C7F0) as a modeless window stored at app+0xA0B44.
    // ------------------------------------------------------------------
    case 253: {
        if (app->raw<std::int32_t>(kDwordA0B44) != 0) {
            break;  // jnz def_47E903 (no-op)
        }
        if (app->state.optflag0 != 0) {
            break;
        }
        HWND dlg = CreateDialogParamA(
            hInst, MAKEINTRESOURCEA(english ? 0x296 : 0x285),
            hwnd, Sub44C7F0, 0);
        app->raw<HWND>(kDwordA0B44) = dlg;
        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
        break;
    }

    // ------------------------------------------------------------------
    // 254 (0x0048A26A): toggle physics-display flag app+0x9ED9A and menu
    // item 0xFE (same flag as menu 0xD8 / CmdTogglePhysicsDisplay).
    // ------------------------------------------------------------------
    case 254: {
        if (app->state.projectedShadowBlendEnabled != 0) {
            app->state.projectedShadowBlendEnabled = 0;
            CheckMenuItem(GetMenu(hwnd), 0xFE, MF_UNCHECKED);
        } else {
            app->state.projectedShadowBlendEnabled = 1;
            CheckMenuItem(GetMenu(hwnd), 0xFE, MF_CHECKED);
        }
        break;
    }

    // ------------------------------------------------------------------
    // 255..258 (0x0048A883/0x48A89B/0x48A8B3/0x48A8CB): open the four
    // frame-edit dialogs: flag 0x54/0x5C/0x64/0x68 = 1, helper call,
    // dirty 0xA0B0D = 1.
    // ------------------------------------------------------------------
    case 255:
        app->raw<std::int32_t>(kOff54) = 1;
        Sub439E40(app);                              // 0x439E40
        app->SceneModified() = 1;
        return;

    case 256:
        app->raw<std::int32_t>(kOff5C) = 1;
        Sub43A650(app);                              // 0x43A650
        app->SceneModified() = 1;
        return;

    case 257:
        app->raw<std::int32_t>(kOff64) = 1;
        Sub43B720(app);                              // 0x43B720
        app->SceneModified() = 1;
        return;

    case 258:
        app->raw<std::int32_t>(kOff68) = 1;
        Sub43BB30(app);                              // 0x43BB30
        app->SceneModified() = 1;
        return;

    // ------------------------------------------------------------------
    // 259 (0x0048A8E3): model-edge dialog (0x29A EN / 0x299 JP,
    // sub_464BD0) at app+0xA0B50.
    // ------------------------------------------------------------------
    case 259: {
        if (app->state.a0B50OrPtr != 0) {
            break;
        }
        if (app->state.optflag0 != 0) {
            break;
        }
        HWND dlg = CreateDialogParamA(
            hInst, MAKEINTRESOURCEA(english ? 0x29A : 0x299),
            hwnd, Sub464BD0, 0);
        app->FrameRangeDialog() = dlg;
        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
        return;
    }

    // ------------------------------------------------------------------
    // 260 (0x0048A862): toggle English UI (byte 0xA0B4C), then the
    // language sweep (0x441AD0 LocalizeUI) and UI refresh (0x40B5A0).
    // ------------------------------------------------------------------
    case 260: {
        const std::uint8_t cur =
            app->state.englishUI;
        app->state.englishUI = (cur == 0) ? 1 : 0;
        LocalizeUI(app);                             // 0x441AD0
        Sub40B5A0(app);                              // 0x40B5A0
        return;
    }

    // ------------------------------------------------------------------
    // 261 (0x0048A958): enhance-model dialog (0x2AA EN / 0x2A9 JP,
    // sub_43C9A0); on close (result != 2) the save reminder box,
    // then 0xA0B64 = 1.
    // ------------------------------------------------------------------
    case 261: {
        if (app->state.optflag0 != 0) {
            return;
        }
        app->state.bC = 1;
        const INT_PTR r = DialogBoxParamA(
            hInst, MAKEINTRESOURCEA(english ? 0x2AA : 0x2A9),
            hwnd, Sub43C9A0, 0);
        if (r == 2) {
            return;
        }
        if (english) {
            MessageBoxA(hwnd,
                        "Please preserve the edit result as a new model by "
                        "'save enhanced model'.",
                        "enhance model", 0x40000 /*MB_TOPMOST*/);
        } else {
            MessageBoxA(hwnd, kMsgEnhanceModelJp, kCaptionEnhanceModelJp,
                        0x40000 /*MB_TOPMOST*/);
        }
        app->EnhancedModelDirty() = 1;
        return;
    }

    // ------------------------------------------------------------------
    // 262 (0x0048A9EC): frame-edit dialog (0x2AC EN / 0x2AB JP,
    // sub_465020) at app+0xA0B74.
    // ------------------------------------------------------------------
    case 262: {
        if (app->raw<std::int32_t>(kDwordA0B74) != 0) {
            break;
        }
        if (app->state.optflag0 != 0) {
            return;
        }
        app->raw<std::int32_t>(kOff48) = 1;
        app->state.bC = 1;
        HWND dlg = CreateDialogParamA(
            hInst, MAKEINTRESOURCEA(english ? 0x2AC : 0x2AB),
            hwnd, Sub465020, 0);
        app->raw<HWND>(kDwordA0B74) = dlg;
        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
        return;
    }

    // ------------------------------------------------------------------
    // 263 (0x0048AA6F): "save enhanced model" file dialog.  Filter
    // "Polygon Model files(*.pmd)\0*.pmd\0All Files(*.*)\0\0" (0x52F688),
    // title EN "save enhanced model" / JP 0x52F650, defext "pmd",
    // Flags 6, nFilterIndex 1; initial dir = DirModel when menu 0x12D is
    // checked else "UserFile\Model" (0x52DCBC); on success the menu gate
    // copies the directory into DirModel, then sub_41EC10 saves the file.
    // ------------------------------------------------------------------
    case 263: {
        app->raw<std::int32_t>(kOff60) = 1;
        app->state.bC = 1;
        SetCurrentDirectoryW(app->ExeDir());
        wchar_t fileBuf[0x100];
        fileBuf[0] = L'\0';
        OPENFILENAMEW ofn;
        std::memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = 0x4C;
        ofn.hwndOwner = hwnd;
        ofn.nFilterIndex = 1;
        ofn.lpstrFilter =
            L"Polygon Model files(*.pmd)\x00\x00*.pmd\x00\x00"
            L"All Files(*.*)\x00\x00";
        ofn.lpstrFile = fileBuf;
        ofn.nMaxFile = 0x100;
        ofn.Flags = 6;  // OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0) {
            ofn.lpstrInitialDir = app->DirModel();
        } else {
            ofn.lpstrInitialDir = L"UserFile\\Model";
        }
        ofn.lpstrDefExt = L"pmd";
        wchar_t fileTitle[0x100];
        ofn.nMaxFileTitle = 0x100;
        ofn.lpstrFileTitle = fileTitle;
        ofn.lpstrTitle = english ? L"save enhanced model" : kTitleSaveModelJp;
        if (!GetSaveFileNameW(&ofn)) {
            return;
        }
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0) {
            wchar_t* dir = ExtractDirFromPath(
                app->PathWorkspace().projectDirectory,
                fileBuf);
            Sub42AE20(app->DirModel(), dir);
        }
        Sub41EC10(app, fileBuf);                     // 0x41EC10
        return;
    }

    // ------------------------------------------------------------------
    // 264 (0x0048ABB0): toggle byte app+0xA0CC8 with CheckMenuItem 0x108.
    // ------------------------------------------------------------------
    case 264: {
        app->raw<std::int32_t>(kOff40) = 1;
        if (app->raw<std::uint8_t>(offsets::kByteA0CC8) != 0) {
            app->raw<std::uint8_t>(offsets::kByteA0CC8) = 0;
            CheckMenuItem(GetMenu(hwnd), 0x108, MF_UNCHECKED);
        } else {
            app->raw<std::uint8_t>(offsets::kByteA0CC8) = 1;
            CheckMenuItem(GetMenu(hwnd), 0x108, MF_CHECKED);
        }
        return;
    }

    // ------------------------------------------------------------------
    // 265/269/270/272 (0x0048ACA7/0x48AC0E/0x48AD38/0x48ADCC): view-mode
    // radios - app+0xA0CC4 = 1/2/3/0, the matching menu item
    // (0x109/0x10D/0x10E/0x110) is checked, the other three unchecked,
    // then ModelKinematicSync (0x4B2210, thiscall(model, app+0xA0CC4))
    // over the 100 model slots.  Cases 265/269/270 set 0x9EDB5 = 1.
    // ------------------------------------------------------------------
    case 265: {
        app->PlaybackPhysicsMode() = 1;
        CheckMenuItem(GetMenu(hwnd), 0x10D, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x109, MF_CHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x10E, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x110, MF_UNCHECKED);
        for (int i = 0; i < 100; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr) {
                ModelKinematicSync(model);  // orig: thiscall(model, 0xA0CC4)
            }
        }
        app->PhysicsResetPending() = 1;
        return;
    }

    case 269: {
        app->PlaybackPhysicsMode() = 2;
        CheckMenuItem(GetMenu(hwnd), 0x10D, MF_CHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x109, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x10E, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x110, MF_UNCHECKED);
        for (int i = 0; i < 100; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr) {
                ModelKinematicSync(model);
            }
        }
        app->PhysicsResetPending() = 1;
        return;
    }

    case 270: {
        app->PlaybackPhysicsMode() = 3;
        CheckMenuItem(GetMenu(hwnd), 0x10D, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x109, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x10E, MF_CHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x110, MF_UNCHECKED);
        for (int i = 0; i < 100; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr) {
                ModelKinematicSync(model);
            }
        }
        app->PhysicsResetPending() = 1;
        return;
    }

    case 272: {
        app->PlaybackPhysicsMode() = 0;
        CheckMenuItem(GetMenu(hwnd), 0x10D, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x109, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x10E, MF_UNCHECKED);
        CheckMenuItem(GetMenu(hwnd), 0x110, MF_CHECKED);
        for (int i = 0; i < 100; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr) {
                ModelKinematicSync(model);
            }
        }
        return;
    }

    // ------------------------------------------------------------------
    // 266 (0x0048AEE1): accessory-frame dialog (0x323 EN / 0x322 JP,
    // sub_479E90) at app+0xA0CCC.
    // ------------------------------------------------------------------
    case 266: {
        if (app->raw<std::int32_t>(kDwordA0CCC) != 0) {
            return;
        }
        app->raw<std::int32_t>(kOff4C) = 1;
        app->state.bC = 1;
        HWND dlg = CreateDialogParamA(
            hInst, MAKEINTRESOURCEA(english ? 0x323 : 0x322),
            hwnd, Sub479E90, 0);
        app->raw<HWND>(kDwordA0CCC) = dlg;
        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
        return;
    }

    // ------------------------------------------------------------------
    // 267 (0x0048AF57): physical-engine about box.
    // ------------------------------------------------------------------
    case 267: {
        app->raw<std::int32_t>(kOff44) = 1;
        if (english) {
            MessageBoxA(hwnd,
                        "Bullet Physics Library Ver.2.75\n\n"
                        "http://www.bulletphysics.com/",
                        "about physical engine", 0x40000 /*MB_TOPMOST*/);
        } else {
            MessageBoxA(hwnd,
                        "Bullet Physics Library Ver.2.75\n\n"
                        "http://www.bulletphysics.com/",
                        kCaptionPhysEngineJp, 0x40000 /*MB_TOPMOST*/);
        }
        return;
    }

    // ------------------------------------------------------------------
    // 268 (0x0048AECE): 0x54 = 1, undo-available byte 0x9EDB5 = 1.
    // ------------------------------------------------------------------
    case 268:
        app->raw<std::int32_t>(kOff54) = 1;
        app->PhysicsResetPending() = 1;
        return;

    // ------------------------------------------------------------------
    // 271 (0x0048AE60): toggle menu 0x10F (IK display) from its own
    // GetMenuState bit 8; byte app+0xA066D mirrors the checked state.
    // ------------------------------------------------------------------
    case 271: {
        app->raw<std::int32_t>(kOff60) = 1;
        if ((GetMenuState(GetMenu(hwnd), 0x10F, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x10F, MF_CHECKED);
            app->raw<std::uint8_t>(kByteA066D) = 1;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x10F, MF_UNCHECKED);
            app->raw<std::uint8_t>(kByteA066D) = 0;
        }
        return;
    }

    // ------------------------------------------------------------------
    // 273 (0x0048DBFF): bone visibility -> selection: sel[i] = 1 and
    // curBone = i for every bone whose frame visibility byte +0x1EC is
    // set, then the language sweeps (0x42F1E0 / 0x40D070).
    // ------------------------------------------------------------------
    case 273: {
        if (app->state.optflag0 != 0) {
            return;
        }
        app->raw<std::int32_t>(kOff48) = 1;
        unsigned char* model = ActiveModel(app);
        const std::int32_t boneCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->boneCount);
        if (boneCount > 0) {
            mdl::BoneRecord* const bones = mdl::Bones(model);
            unsigned char* sel = mdl::Mdl(model)->boneSelection;
            for (std::int32_t i = 0; i < boneCount; ++i) {
                if (bones[i].f492 != 0) {
                    sel[i] = 1;
                    mdl::Mdl(model)->selectedBone = i;
                } else {
                    sel[i] = 0;
                }
            }
        }
        PostLanguageSweep(app);   // 0x42F1E0
        PostLanguageSweep2(app);  // 0x40D070
        return;
    }

    // ------------------------------------------------------------------
    // 274 (0x0048DC96): clear the frame-selection flags (morph +0x10 /
    // 0x14-step, other +0x14 / 0x1C-step, bone +0x38 / 0x3C-step), then
    // mark the display frames of every visible bone (visibility byte
    // +0x1EC) and its parent chain: frame +0x38 = 1 when +0x39 == 0.
    // PanelPaint tail.
    // ------------------------------------------------------------------
    case 274: {
        if (app->state.optflag0 != 0) {
            return;
        }
        app->raw<std::int32_t>(kOff30) = 1;
        unsigned char* model = ActiveModel(app);
        // morph frames
        unsigned char* mf =
            *reinterpret_cast<unsigned char**>(model + kModelMorphFrames);
        for (std::size_t o = 0; o < 0x61A80u; o += 0x14) {
            mf[o + 0x10] = 0;
        }
        // other (camera/light/Ik) frames
        unsigned char* of =
            *reinterpret_cast<unsigned char**>(model + kModelOtherFrames);
        for (std::size_t o = 0; o < 0x6D60u; o += 0x1C) {
            of[o + 0x14] = 0;
        }
        // bone display frames
        unsigned char* bf =
            *reinterpret_cast<unsigned char**>(model + kModelBoneFrames);
        for (std::size_t o = 0; o < 0x112A880u; o += 0x3C) {
            bf[o + 0x38] = 0;
        }
        // mark visible bones + parent chains
        const std::int32_t boneCount =
            static_cast<std::int32_t>(mdl::Mdl(model)->boneCount);
        mdl::BoneRecord* const bones = mdl::Bones(model);
        if (boneCount > 0) {
            for (std::int32_t i = 0; i < boneCount; ++i) {
                if (bones[i].f492 == 0) {
                    continue;
                }
                const std::size_t stride = 0x3C * static_cast<std::size_t>(i);
                if (bf[stride + 0x39] == 0) {
                    bf[stride + 0x38] = 1;
                }
                const std::int32_t parent =
                    *reinterpret_cast<std::int32_t*>(bf + stride + 8);
                if (parent > 0) {
                    std::size_t off = stride;
                    for (;;) {
                        if (bf[off + 0x39] == 0) {
                            bf[off + 0x38] = 1;
                        }
                        const std::int32_t p =
                            *reinterpret_cast<std::int32_t*>(bf + off + 8);
                        off = 0x3C * static_cast<std::size_t>(p);
                        if (*reinterpret_cast<std::int32_t*>(bf + off + 8) <=
                            0) {
                            if (bf[off + 0x39] == 0) {
                                bf[off + 0x38] = 1;
                            }
                            break;
                        }
                    }
                } else {
                    if (bf[stride + 0x39] == 0) {
                        bf[stride + 0x38] = 1;
                    }
                }
            }
        }
        PanelPaint(app);  // 0x414610
        return;
    }

    // ------------------------------------------------------------------
    // 275 (0x0048DE30): modal dialog 0x32D EN / 0x32C JP (sub_461CE0);
    // no close handling in the original (always returns).
    // ------------------------------------------------------------------
    case 275:
        app->raw<std::int32_t>(kOff38) = 1;
        app->state.bC = 1;
        DialogBoxParamA(hInst,
                        MAKEINTRESOURCEA(english ? 0x32D : 0x32C),
                        hwnd, Sub461CE0, 0);
        return;

    // ------------------------------------------------------------------
    // 276 (0x0048AFA4): "render to picture file".  Save dialog with the
    // picture filter (EN 0x52F3F8 / JP 0x52F218, label followed by an
    // empty pattern list as in the binary), defext "bmp", title "render
    // to picture file" / JP 0x52F1D0, initial dir DirUser / "UserFile";
    // dir gate 0x12D like 263; the chosen path goes to app+0x9F134 and
    // the render target grows (locale screenWidth/screenHeight, 0x1D4E4/
    // 0x1D4E8 -> presentParameters.BackBufferWidth/Height, 0x1D4FC/
    // 0x1D500 + PostDeviceReset) when wider/taller; then the recording
    // window ("RecWindow" class, title 0x52F1C0) is created at
    // app+0xA0D24 and the refresh tail runs.
    // ------------------------------------------------------------------
    case 276: {
        app->raw<std::int32_t>(kOff48) = 1;
        app->state.bC = 1;
        SetCurrentDirectoryW(app->ExeDir());
        wchar_t fileBuf[0x100];
        fileBuf[0] = L'\0';
        OPENFILENAMEW ofn;
        std::memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = 0x4C;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = english
            ? L"All picture format\x00\x00"
              L"*.bmp;*.jpg;*.png;*.dds;*.dib;*.pfm;*.hdr\x00\x00"
              L"Bmp files(*.bmp)\x00\x00*.bmp\x00\x00"
            : kFilterPicJp;
        ofn.lpstrFile = fileBuf;
        ofn.nFilterIndex = 1;
        ofn.nMaxFile = 0x100;
        ofn.Flags = 6;  // OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0) {
            ofn.lpstrInitialDir = app->DirUser();
        } else {
            ofn.lpstrInitialDir = L"UserFile";
        }
        ofn.lpstrDefExt = L"bmp";
        wchar_t fileTitle[0x100];
        ofn.nMaxFileTitle = 0x100;
        ofn.lpstrFileTitle = fileTitle;
        ofn.lpstrTitle = english ? L"render to picture file" : kTitleRenderJp;
        if (!GetSaveFileNameW(&ofn)) {
            return;
        }
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) != 0) {
            wchar_t* dir = ExtractDirFromPath(
                app->PathWorkspace().projectDirectory,
                fileBuf);
            Sub42AE20(app->DirUser(), dir);
        }
        wcscpy_s(reinterpret_cast<wchar_t*>(app->at(kWcsA9F134)),
                 0x100, fileBuf);
        D3DRenderer* locale = app->Renderer();
        const std::int32_t oldW = locale->screenWidth;   // 0x1D4E4
        const std::int32_t oldH = locale->screenHeight;  // 0x1D4E8
        const std::int32_t renderW =
            app->RenderWidth();
        const std::int32_t renderH =
            app->RenderHeight();
        if (renderW > oldW || renderH > oldH) {
            locale->presentParameters.BackBufferWidth = renderW;   // 0x1D4FC
            locale->presentParameters.BackBufferHeight = renderH;  // 0x1D500
            locale->screenWidth = renderW;                         // 0x1D4E4
            locale->screenHeight = renderH;                        // 0x1D4E8
            PostDeviceReset(app);  // 0x440DB0
        }
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.right = renderW;
        rc.bottom = renderH;
        AdjustWindowRect(&rc, 0x80C00000, FALSE);
        HWND recWnd = CreateWindowExA(
            0, "RecWindow", kRecWndTitle, 0x80C80000,
            app->SidebarWidth() + 0x32, 0x64,
            rc.right - rc.left, rc.bottom - rc.top, hwnd, nullptr, hInst,
            nullptr);
        app->RecordingWindow() = recWnd;                    // 0xA0D24
        if (recWnd == nullptr) {
            MessageBoxA(hwnd, "CreateWindow failed",
                        english ? "create main window" : kCaptionCreateWndJp,
                        0x40000 /*MB_TOPMOST*/);
            return;
        }
        ShowWindow(recWnd, SW_SHOW);
        UpdateWindow(recWnd);
        if (locale->multisampleAvailable == 0) {  // 0x1D4F8
            CallSceneVtable94(app);
        }
        const bool needRefresh =
            app->CaptureMode() == ScreenCaptureMode::BackgroundRefresh ||
            app->state.aviBackgroundEnabled == 1;
        if (needRefresh && app->raw<std::int32_t>(offsets::kDword9E400) != 0) {
            AviBgOverlayRefresh(app);  // 0x4168D0
        }
        Sub42C810(app);  // 0x42C810
        InvalidateRect(hwnd, &app->ViewportRect(), FALSE);
        app->raw<std::uint8_t>(offsets::kByteA03B7) = 1;
        return;
    }

    // ------------------------------------------------------------------
    // 277 (0x0048B2F7): shadow toggle - menu 0x115 from GetMenuState bit
    // 8, scene vtable+0xE4(scene, 0xA1, on/off).
    // ------------------------------------------------------------------
    case 277: {
        app->raw<std::int32_t>(kOff58) = 1;
        if ((GetMenuState(GetMenu(hwnd), 0x115, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x115, MF_CHECKED);
            CallSceneVtableE4(app, 1);
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x115, MF_UNCHECKED);
            CallSceneVtableE4(app, 0);
        }
        return;
    }

    // ------------------------------------------------------------------
    // 278 (0x0048B401): reload the toon texture set (0x424DC0
    // InitToonTextures), then the locale refresh sub_4076E0.
    // ------------------------------------------------------------------
    case 278:
        InitToonTextures(app);  // 0x424DC0
        Sub4076E0(app->Renderer());  // 0x4076E0
        return;

    // ------------------------------------------------------------------
    // 279 (0x0048B393): self-shadow toggle - menu 0x117, byte
    // app+0xA0188 mirrors.
    // ------------------------------------------------------------------
    case 279: {
        app->raw<std::int32_t>(kOff60) = 1;
        if ((GetMenuState(GetMenu(hwnd), 0x117, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x117, MF_CHECKED);
            app->raw<std::uint8_t>(kByteA0188) = 1;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x117, MF_UNCHECKED);
            app->raw<std::uint8_t>(kByteA0188) = 0;
        }
        return;
    }

    // ------------------------------------------------------------------
    // 280 (0x0048B416): flag-subsystem save/init keyed on app+0xA0D38.
    // ------------------------------------------------------------------
    case 280:
        if (app->state.floatingWindow != 0) {
            SaveFlagSubsystem(app);  // 0x461FA0
        } else {
            InitFlagSubsystem(app);  // 0x461E00
        }
        return;

    // ------------------------------------------------------------------
    // 281 (0x0048B433): transparent-window toggle - menu 0x119 plus
    // SetWindowPos of the accessory column (app+0xA0D38) to
    // HWND_TOPMOST / HWND_NOTOPMOST with flags 0x43.
    // ------------------------------------------------------------------
    case 281: {
        app->raw<std::int32_t>(kOff6C) = 1;
        if ((GetMenuState(GetMenu(hwnd), 0x119, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x119, MF_CHECKED);
            if (app->state.floatingWindow != 0) {
                SetWindowPos(app->state.floatingWindow,
                             HWND_TOPMOST, 0, 0, 0, 0, 0x43);
            }
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x119, MF_UNCHECKED);
            if (app->state.floatingWindow != 0) {
                SetWindowPos(app->state.floatingWindow,
                             HWND_NOTOPMOST, 0, 0, 0, 0, 0x43);
            }
        }
        return;
    }

    // ------------------------------------------------------------------
    // 282..285 (0x0048B624/0x48B682/0x48B6E0/0x48B73E): option toggles -
    // bytes app+0xA0194..0xA0197 mirrored by menu items 0x11A..0x11D.
    // Case 285 additionally stores 1 / -1 into scene->groundBody -> +0xD4.
    // ------------------------------------------------------------------
    case 282: {
        app->raw<std::int32_t>(kOff40) = 1;
        if (app->state.a0194 != 0) {
            app->state.a0194 = 0;
            CheckMenuItem(GetMenu(hwnd), 0x11A, MF_UNCHECKED);
        } else {
            app->state.a0194 = 1;
            CheckMenuItem(GetMenu(hwnd), 0x11A, MF_CHECKED);
        }
        return;
    }

    case 283: {
        app->raw<std::int32_t>(kOff44) = 1;
        if (app->state.modelOutlineRenderingSuppressed != 0) {
            app->state.modelOutlineRenderingSuppressed = 0;
            CheckMenuItem(GetMenu(hwnd), 0x11B, MF_UNCHECKED);
        } else {
            app->state.modelOutlineRenderingSuppressed = 1;
            CheckMenuItem(GetMenu(hwnd), 0x11B, MF_CHECKED);
        }
        return;
    }

    case 284: {
        app->raw<std::int32_t>(kOff6C) = 1;
        if (app->state.a0196 != 0) {
            app->state.a0196 = 0;
            CheckMenuItem(GetMenu(hwnd), 0x11C, MF_UNCHECKED);
        } else {
            app->state.a0196 = 1;
            CheckMenuItem(GetMenu(hwnd), 0x11C, MF_CHECKED);
        }
        return;
    }

    case 285: {
        app->raw<std::int32_t>(kOff6C) = 1;
        unsigned char* target = reinterpret_cast<unsigned char*>(
            app->Physics()->groundBody);   // scene slot 0x44
        if (app->state.a0197 != 0) {
            app->state.a0197 = 0;
            CheckMenuItem(GetMenu(hwnd), 0x11D, MF_UNCHECKED);
            *reinterpret_cast<std::int32_t*>(target + 0xD4) = -1;
        } else {
            app->state.a0197 = 1;
            CheckMenuItem(GetMenu(hwnd), 0x11D, MF_CHECKED);
            *reinterpret_cast<std::int32_t*>(target + 0xD4) = 1;
        }
        return;
    }

    // ------------------------------------------------------------------
    // 286 (0x0048B7B8): background-color picker - ChooseColorA with the
    // current R/G/B from app+0xA0198/0xA019C/0xA01A0, custom colors at
    // app+0xA01A4, flags 3; the new color is written back and applied
    // via Sub4A4850(model, r, g, b) over the 100 model slots.
    // ------------------------------------------------------------------
    case 286: {
        app->raw<std::int32_t>(kOff48) = 1;
        app->state.bC = 1;
        CHOOSECOLORA cc;
        std::memset(&cc, 0, sizeof(cc));
        cc.lStructSize = 0x24;
        cc.hwndOwner = hwnd;
        const std::uint8_t r =
            app->state.modelOutlineColorRed;
        const std::uint16_t g =
            app->state.modelOutlineColorGreen;
        const std::uint8_t b =
            app->state.modelOutlineColorBlue;
        cc.rgbResult = (static_cast<COLORREF>(b) << 16) |
                       (static_cast<COLORREF>(g) << 8) | r;
        cc.lpCustColors =
            reinterpret_cast<COLORREF*>(app->at(offsets::kBufBuf655780));
        cc.Flags = 3;  // CC_RGBINIT | CC_FULLOPEN
        if (!ChooseColorA(&cc)) {
            return;
        }
        const std::uint8_t nr = cc.rgbResult & 0xFF;
        const std::uint8_t ng = (cc.rgbResult >> 8) & 0xFF;
        const std::uint8_t nb = (cc.rgbResult >> 16) & 0xFF;
        app->state.modelOutlineColorRed = nr;
        app->state.modelOutlineColorGreen = ng;
        app->state.modelOutlineColorBlue = nb;
        for (int i = 0; i < 100; ++i) {
            unsigned char* model = app->ModelSlot(i);
            if (model != nullptr) {
                Sub4A4850(reinterpret_cast<MMDApp*>(model), nr, ng, nb);
            }
        }
        return;
    }

    // ------------------------------------------------------------------
    // 287 (0x0048B8CF): toggle byte app+0xA01E4 with CheckMenuItem 0x11F.
    // ------------------------------------------------------------------
    case 287: {
        if (app->raw<std::uint8_t>(offsets::kByteA01E4) != 0) {
            CheckMenuItem(GetMenu(hwnd), 0x11F, MF_UNCHECKED);
            app->raw<std::uint8_t>(offsets::kByteA01E4) = 0;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x11F, MF_CHECKED);
            app->raw<std::uint8_t>(offsets::kByteA01E4) = 1;
        }
        return;
    }

    // ------------------------------------------------------------------
    // 288 (0x0048A08F): modal dialog 0x325 EN / 0x324 JP (sub_4641F0);
    // return when closed.
    // ------------------------------------------------------------------
    case 288:
        app->state.bC = 1;
        DialogBoxParamA(hInst,
                        MAKEINTRESOURCEA(english ? 0x325 : 0x324),
                        hwnd, Sub4641F0, 0);
        return;

    // ------------------------------------------------------------------
    // 289 (0x0048A0DF): modal dialog 0x32B EN / 0x32A JP (sub_42E370);
    // return when closed.
    // ------------------------------------------------------------------
    case 289:
        app->raw<std::int32_t>(kOff38) = 1;
        app->state.bC = 1;
        DialogBoxParamA(hInst,
                        MAKEINTRESOURCEA(english ? 0x32B : 0x32A),
                        hwnd, Sub42E370, 0);
        return;

    // ------------------------------------------------------------------
    // 290 (0x0048B926): full scene reload - 0xA0274 = 1,
    // sub_4629D0 (0x4629D0), PostDeviceReset (0x440DB0).
    // ------------------------------------------------------------------
    case 290:
        app->FullscreenMode() = 1;
        Sub4629D0(app);  // 0x4629D0
        PostDeviceReset(app);  // 0x440DB0
        return;

    // ------------------------------------------------------------------
    // 291 (0x0048B93E): open-ONI gate - app+0xA03B8 selects between the
    // plugin loader (0x42A020) and the OpenNi init (0x429CB0, arg 0).
    // ------------------------------------------------------------------
    case 291:
        app->raw<std::int32_t>(kOff5C) = 1;
        if (app->state.depthDeviceEnabled != 0) {
            DisableKinect(app);  // 0x42A020
        } else {
            OpenNiInit(app, nullptr);  // 0x429CB0
        }
        return;

    // ------------------------------------------------------------------
    // 292 (0x0048B964): auto-frame-record toggle (byte app+0xA0D68):
    // on - CheckMenuItem 0x124, SetTimer(hwnd, 0x65, 0x5DC, 0), per-slot
    // model+0x38FD = app+0xA03EA, 0x9EDB5 = 1; off - KillTimer(0x65),
    // uncheck, dirty + 0x9EDB5 = 1.
    // ------------------------------------------------------------------
    case 292: {
        app->raw<std::int32_t>(kOff38) = 1;
        if (app->raw<std::uint8_t>(offsets::kByteAutorep) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x124, MF_CHECKED);
            app->raw<std::uint8_t>(offsets::kByteAutorep) = 1;
            SetTimer(hwnd, 0x65, 0x5DC, nullptr);
            app->PhysicsResetPending() = 1;
            for (int i = 0; i < 100; ++i) {
                unsigned char* model = app->ModelSlot(i);
                if (model != nullptr) {
                    model[0x38FD] =
                        app->raw<std::uint8_t>(kByteA03EA);
                }
            }
        } else {
            KillTimer(hwnd, 0x65);
            CheckMenuItem(GetMenu(hwnd), 0x124, MF_UNCHECKED);
            app->raw<std::uint8_t>(offsets::kByteAutorep) = 0;
            app->SceneModified() = 1;
            app->PhysicsResetPending() = 1;
        }
        return;
    }

    // ------------------------------------------------------------------
    // 293/294/295 (0x0048BA1E/0x48BA75/0x48BAD3): option toggles - bytes
    // app+0xA03DC/0xA03DD/0xA03DE mirrored by menu items 0x125..0x127.
    // ------------------------------------------------------------------
    case 293: {
        if (app->raw<std::uint8_t>(offsets::kByteA03DC) != 0) {
            CheckMenuItem(GetMenu(hwnd), 0x125, MF_UNCHECKED);
            app->raw<std::uint8_t>(offsets::kByteA03DC) = 0;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x125, MF_CHECKED);
            app->raw<std::uint8_t>(offsets::kByteA03DC) = 1;
        }
        return;
    }

    case 294: {
        app->raw<std::int32_t>(kOff74) = 1;
        if (app->raw<std::uint8_t>(offsets::kByteA03DD) != 0) {
            CheckMenuItem(GetMenu(hwnd), 0x126, MF_UNCHECKED);
            app->raw<std::uint8_t>(offsets::kByteA03DD) = 0;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x126, MF_CHECKED);
            app->raw<std::uint8_t>(offsets::kByteA03DD) = 1;
        }
        return;
    }

    case 295: {
        app->raw<std::int32_t>(kOff40) = 1;
        if (app->raw<std::uint8_t>(offsets::kByteA03DE) != 0) {
            CheckMenuItem(GetMenu(hwnd), 0x127, MF_UNCHECKED);
            app->raw<std::uint8_t>(offsets::kByteA03DE) = 0;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x127, MF_CHECKED);
            app->raw<std::uint8_t>(offsets::kByteA03DE) = 1;
        }
        return;
    }

    // ------------------------------------------------------------------
    // 296 (0x0048BB31): "open oni data" - model-edit gate
    // (app+0x2F8): message box "Please select model!" / JP 0x52E2F4,
    // caption "open oni data" / "oni"; else the oni file dialog
    // (filter 0x52F168, defext "oni", Flags 0x1000, initial dir
    // "UserFile", owner app+0xA0D38 ?: hwnd), DisableKinect when
    // app+0xA03B8, wide->SJIS conversion (0x407910) and OpenNiInit
    // (0x429CB0).
    // ------------------------------------------------------------------
    case 296: {
        if (app->state.optflag0 != 0) {
            if (english) {
                MessageBoxA(hwnd, "Please select model!", "open oni data",
                            0x40000 /*MB_TOPMOST*/);
            } else {
                MessageBoxA(hwnd, kMsgOpenOniJp, "oni",
                            0x40000 /*MB_TOPMOST*/);
            }
            return;
        }
        SetCurrentDirectoryW(app->ExeDir());
        wchar_t fileBuf[0x100];
        fileBuf[0] = L'\0';
        OPENFILENAMEW ofn;
        std::memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = 0x4C;
        ofn.hwndOwner = app->state.floatingWindow != 0
                            ? app->state.floatingWindow
                            : hwnd;
        ofn.lpstrFilter =
            L"oni files(*.oni)\x00\x00*.oni\x00\x00All Files(*.*)\x00\x00";
        ofn.lpstrFile = fileBuf;
        ofn.nMaxFile = 0x100;
        ofn.Flags = 0x1000;  // OFN_FILEMUSTEXIST
        ofn.lpstrInitialDir = L"UserFile";
        ofn.lpstrDefExt = L"oni";
        wchar_t fileTitle[0x100];
        ofn.nMaxFileTitle = 0x100;
        ofn.lpstrFileTitle = fileTitle;
        ofn.lpstrTitle = english ? L"open file data" : kTitleOpenJp;
        if (!GetOpenFileNameW(&ofn)) {
            return;
        }
        if (app->state.depthDeviceEnabled != 0) {
            DisableKinect(app);  // 0x42A020
        }
        char sjisPath[0x100];
        WideToSjisPath(sjisPath, fileBuf, 0x100);  // 0x407910
        OpenNiInit(app, sjisPath);  // 0x429CB0
        return;
    }

    // ------------------------------------------------------------------
    // 297 (0x0048BC9B): toggle byte app+0xA03E9 with CheckMenuItem 0x129.
    // ------------------------------------------------------------------
    case 297: {
        app->raw<std::int32_t>(kOff40) = 1;
        if (app->raw<std::uint8_t>(kByteA03E9) != 0) {
            app->raw<std::uint8_t>(kByteA03E9) = 0;
            CheckMenuItem(GetMenu(hwnd), 0x129, MF_UNCHECKED);
        } else {
            app->raw<std::uint8_t>(kByteA03E9) = 1;
            CheckMenuItem(GetMenu(hwnd), 0x129, MF_CHECKED);
        }
        return;
    }

    // ------------------------------------------------------------------
    // 298 (0x0048BD18): render-mode toggle - menu 0x12A, locale byte
    // runtimeToggle (+0x1D570) mirrors, InitRenderStates (0x406E90) on the
    // locale subobject (the ported helper takes the app).
    // ------------------------------------------------------------------
    case 298: {
        D3DRenderer* locale = app->Renderer();
        if ((GetMenuState(GetMenu(hwnd), 0x12A, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x12A, MF_CHECKED);
            locale->runtimeToggle = 1;  // 0x1D570
            InitRenderStates(app);  // 0x406E90 (orig: thiscall(locale))
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x12A, MF_UNCHECKED);
            locale->runtimeToggle = 0;  // 0x1D570
            InitRenderStates(app);
        }
        return;
    }

    // ------------------------------------------------------------------
    // 299 (0x0048BDA1): accessory-column toggle - menu 0x12B, checkbox
    // 0x228 of the accessory window (app+0xA0D38 ?: main) via
    // BM_SETCHECK, byte app+0xA4420 mirrors.
    // ------------------------------------------------------------------
    case 299: {
        HWND owner = app->state.floatingWindow != 0
                         ? app->state.floatingWindow
                         : hwnd;
        if ((GetMenuState(GetMenu(hwnd), 0x12B, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x12B, MF_CHECKED);
            SendMessageA(GetDlgItem(owner, 0x228), BM_SETCHECK, 1, 0);
            app->FrameVolumeControlEnabled() = 1;
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x12B, MF_UNCHECKED);
            SendMessageA(GetDlgItem(owner, 0x228), BM_SETCHECK, 0, 0);
            app->FrameVolumeControlEnabled() = 0;
        }
        return;
    }

    // ------------------------------------------------------------------
    // 300 (0x0048A2C1): rotation dialog.  Camera path when app+0x2F8 is
    // set (posx/y/z + cam2/3/4 + camangle through the pi/180 conversion,
    // dialog 0x298 EN / 0x289 JP, writeback, dirty, PostViewRefresh);
    // bone path otherwise (quaternion extraction via D3DXMatrixRotation-
    // Quaternion + asin/atan2 (0x40A690/0x40A6B0), quadrant fixes,
    // clamp, dialog 0x297 EN / 0x288 JP, Sub42D6E0, pos/angle writeback,
    // RotationZ*X*Y matrix, quaternion writeback, frameFlag, dirty,
    // PostViewRefresh).  The 16-float temp block app+0xA0B28..0xA0B40 is
    // shared with the dialog procs in the original.
    // ------------------------------------------------------------------
    case 300: {
        const double kD1 = Bits64(kDbl52B768bits);  // pi (truncated)
        const double kD2 = Bits64(kDbl52B760bits);  // 180.0
        const double kD3 = Bits64(kDbl52E678bits);  // (double)(float)pi
        const float kPi = Bits32(kFlt52B738bits);   // +pi float
        const float kNegPi = Bits32(kFlt52B73Cbits);
        const float kThr = Bits32(kFlt52B740bits);
        app->state.bC = 1;
        if (app->state.optflag0 != 0) {
            // ---- camera path (0x48A2D9..0x48A407) -----------------------
            const float t0 = app->CameraPositionX();
            const float t1 = app->CameraPositionY();
            const float t2 = app->CameraPositionZ();
            const float t3 = static_cast<float>(
                -static_cast<double>(app->CameraPitch()) *
                kD2 / kD1);
            const float t4 = static_cast<float>(
                static_cast<double>(app->CameraYaw()) *
                kD2 / kD1);
            const float t5 = static_cast<float>(
                static_cast<double>(app->CameraRoll()) *
                kD2 / kD1);
            const float t6 = static_cast<float>(
                -static_cast<double>(app->CameraDistance()));
            if (DialogBoxParamA(
                    hInst, MAKEINTRESOURCEA(english ? 0x298 : 0x289),
                    hwnd, Sub40FBC0, 0) == 2) {
                return;
            }
            app->CameraPositionX() = t0;
            app->CameraPositionY() = t1;
            app->CameraPositionZ() = t2;
            app->CameraPitch() = static_cast<float>(
                -static_cast<double>(t3) * kD3 / kD2);
            app->CameraYaw() = static_cast<float>(
                static_cast<double>(t4) * kD3 / kD2);
            app->CameraRoll() = static_cast<float>(
                kD2 / (kD3 * static_cast<double>(t5)));
            app->CameraDistance() =
                static_cast<float>(-static_cast<double>(t6));
            app->SceneModified() = 1;
            PostViewRefresh(app);  // 0x40D130
            return;
        }
        // ---- bone path (0x48A40C..0x48A85D) -----------------------------
        unsigned char* model = ActiveModel(app);
        const std::int32_t curBone = mdl::Mdl(model)->selectedBone;
        if (curBone == -1) {
            return;
        }
        mikudancestudio::mdl::BoneRecord* bone = mdl::Bones(model) + curBone;
        const float t0 = bone->trans[0];
        const float t1 = bone->trans[1];
        const float t2 = bone->trans[2];
        // rotation matrix from the bone's stored quaternion (0x14C)
        d3dx::D3DXMATRIXF m{};
        d3dx::Get().matrixRotationQuaternion(
            &m, reinterpret_cast<const float*>(bone->rotQuat));
        // euler extraction: t5 = atan2(m10, m01), t3 = asin(-m20),
        // t4 = atan2(m21, m13) - helper results are float-rounded
        float t5 = static_cast<float>(AngleAtan2(m.m[1][0], m.m[0][1]));
        float t3 = static_cast<float>(AngleAsin(-m.m[2][0]));
        float t4 = static_cast<float>(AngleAtan2(m.m[2][1], m.m[1][3]));
        // quadrant fixes (0x48A531..0x48A5BB): when
        // fabs(cos(t3)) < flt_52B740, add -pi/+pi to t5 and t4 depending
        // on the sign of m01 / m13 (m13 == 0 -> +pi, as in the original's
        // 0.0-vs-m13 compare)
        const double r1 = AngleFabs(static_cast<float>(AngleCos(t3)));
        if (r1 < static_cast<double>(kThr)) {
            t5 = static_cast<float>(static_cast<double>(t5) +
                                    (!(m.m[0][1] > 0.0f) ? kNegPi : kPi));
            t4 = static_cast<float>(static_cast<double>(t4) +
                                    (!(m.m[1][3] > 0.0f) ? kNegPi : kPi));
        }
        // clamp to zero (0x48A5BF..0x48A632)
        if (static_cast<float>(AngleFabs(t3)) < kThr) {
            t3 = 0.0f;
        }
        if (static_cast<float>(AngleFabs(t4)) < kThr) {
            t4 = 0.0f;
        }
        if (static_cast<float>(AngleFabs(t5)) < kThr) {
            t5 = 0.0f;
        }
        // degrees for the dialog (0x48A634..0x48A677)
        t3 = static_cast<float>(static_cast<double>(t3) * kD2 / kD1);
        t4 = static_cast<float>(-static_cast<double>(t4) * kD2 / kD1);
        t5 = static_cast<float>(-kD1 * kD2 / static_cast<double>(t5));
        if (DialogBoxParamA(
                hInst, MAKEINTRESOURCEA(english ? 0x297 : 0x288),
                hwnd, Sub40F860, 0) == 2) {
            return;
        }
        Sub42D6E0(app);  // 0x42D6E0 (bone-edit keyframe register)
        // write back position and angles
        model = ActiveModel(app);
        bone = mdl::Bones(model) + curBone;
        bone->trans[0] = t0;
        bone->trans[1] = t1;
        bone->trans[2] = t2;
        t3 = static_cast<float>(static_cast<double>(t3) * kD1 / kD2);
        t4 = static_cast<float>(-static_cast<double>(t4) * kD1 / kD2);
        t5 = static_cast<float>(-kD2 / (static_cast<double>(t5) * kD1));
        // combined rotation matrix: Rz(t5) * Rx(t3) * Ry(t4) (0x48A746..)
        d3dx::D3DXMATRIXF tmp{};
        d3dx::D3DXMATRIXF rot{};
        d3dx::Get().rotZ(&rot, t5);
        d3dx::Get().rotX(&tmp, t3);
        d3dx::Get().multiply(&rot, &rot, &tmp);
        d3dx::Get().rotY(&tmp, t4);
        d3dx::Get().multiply(&rot, &rot, &tmp);
        // shared tail (0x48A805): quaternion writeback + frameFlag +
        // dirty + PostViewRefresh
        BoneRotationWriteback(app, rot);
        return;
    }

    // ------------------------------------------------------------------
    // 301 (0x0048D700): menu 0x12D (enhanced-mode) toggle only.
    // ------------------------------------------------------------------
    case 301: {
        if ((GetMenuState(GetMenu(hwnd), 0x12D, 0) & 8) == 0) {
            CheckMenuItem(GetMenu(hwnd), 0x12D, MF_CHECKED);
        } else {
            CheckMenuItem(GetMenu(hwnd), 0x12D, MF_UNCHECKED);
        }
        return;
    }

    // ------------------------------------------------------------------
    // 302 (0x0048E0A6): reset rotation.  app+0x2F8 set: cam2/3/4 = 0,
    // dirty, PostViewRefresh.  else: current bone (2D90 != -1),
    // Sub42D6E0, identity matrix, quaternion writeback via the shared
    // tail (0x48A805), frameFlag[cur] = 1, dirty, PostViewRefresh.
    // ------------------------------------------------------------------
    case 302: {
        app->raw<std::int32_t>(kOff34) = 1;
        if (app->state.optflag0 != 0) {
            app->CameraPitch() = 0.0f;
            app->CameraYaw() = 0.0f;
            app->CameraRoll() = 0.0f;
            app->SceneModified() = 1;
            PostViewRefresh(app);
            return;
        }
        unsigned char* model = ActiveModel(app);
        if (mdl::Mdl(model)->selectedBone == -1) {
            return;
        }
        Sub42D6E0(app);  // 0x42D6E0
        d3dx::D3DXMATRIXF ident{};
        for (int i = 0; i < 4; ++i) {
            ident.m[i][i] = 1.0f;
        }
        BoneRotationWriteback(app, ident);
        return;
    }

    default:
        // def_47E903 (0x482897): only reachable actions are gated on
        // notify == BN_CLICKED and the sending control being
        // GetDlgItem(hwnd, 0x1B4) - a no-op for every menu id 251..302.
        break;
    }
}

}  // namespace mikudancestudio
