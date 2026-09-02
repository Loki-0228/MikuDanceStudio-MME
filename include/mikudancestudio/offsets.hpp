// ===========================================================================
// MikuDanceStudio - original binary state-field offsets (MikuMikuDance v932, x86)
// ===========================================================================
// Transient compatibility layer for the raw<T>(offset) access style.
// Each constant here is a promotion candidate: as its field gains a named
// MMDAppState member (app_layout.hpp), the constant and its uses are
// deleted.  This header disappears when the last raw<T>(offset) call
// does.  Worklist: python scripts/promote.py ledger
// ===========================================================================
#pragma once

#include <cstddef>

namespace mikudancestudio::offsets {

constexpr std::size_t kObjectSize = 0xA4530;  // new(0xA4530) in WinMain

constexpr std::size_t kPtrSub025c = 204; // 0xCC // ptr(0x25C obj)@0x47A648 subsystem ptr (0x25C)
constexpr std::size_t kDword300 = 768; // 0x300 // 0
constexpr std::size_t kDword304 = 772; // 0x304 // 0
constexpr std::size_t kFloatCam2 = 784; // 0x310 // 0.0 camera cluster
constexpr std::size_t kFloatCam3 = 788; // 0x314 // 0.0 camera cluster
constexpr std::size_t kFloatCam4 = 792; // 0x318 // 0.0 camera cluster
constexpr std::size_t kDword32C = 812; // 0x32C // 0
constexpr std::size_t kFloatPosx = 820; // 0x334 // 0.0 (0,10,0) candidate
constexpr std::size_t kFloatPosy = 824; // 0x338 // 10.0
constexpr std::size_t kFloatPosz = 828; // 0x33C // 0.0
constexpr std::size_t kByte340 = 832; // 0x340 // 0
constexpr std::size_t kDword350 = 848; // 0x350 // 0
constexpr std::size_t kDword97C = 2428; // 0x97C // 0
constexpr std::size_t kDword980 = 2432; // 0x980 // 0
constexpr std::size_t kByteLightA = 645642; // 0x9DA0A // 0xC0 x6 loop A
constexpr std::size_t kByteLightB = 645648; // 0x9DA10 // 0x00 x6 loop B
constexpr std::size_t kByteLightC = 645654; // 0x9DA16 // 0x40 x6 loop C
constexpr std::size_t kByteLightD = 645660; // 0x9DA1C // 0x7F x6 loop D
constexpr std::size_t kDword9DA28 = 645672; // 0x9DA28 // 0
constexpr std::size_t kDword9DA2C = 645676; // 0x9DA2C // 0
constexpr std::size_t kDword9DA30 = 645680; // 0x9DA30 // 0
constexpr std::size_t kDword9DA34 = 645684; // 0x9DA34 // 0
constexpr std::size_t kDword9DA38 = 645688; // 0x9DA38 // 0
constexpr std::size_t kDword9DA3C = 645692; // 0x9DA3C // 0
constexpr std::size_t kDword9DA40 = 645696; // 0x9DA40 // 0
constexpr std::size_t kDword9DA44 = 645700; // 0x9DA44 // 0
constexpr std::size_t kBufBuf9ddx = 646512; // 0x9DD70 // zeroed 0x3FC
constexpr std::size_t kByte9EB7E = 650110; // 0x9EB7E // 0
constexpr std::size_t kByte9ED90 = 650640; // 0x9ED90 // 0
constexpr std::size_t kPtrSub04b0 = 650656; // 0x9EDA0 // ptr(0x4B0 obj)@0x47A6E1 subsystem ptr
constexpr std::size_t kPtrSub048 = 650672; // 0x9EDB0 // ptr(0x48 obj)@0x47A727 subsystem ptr
constexpr std::size_t kDword9ED9C = 650652; // 0x9ED9C // 0
constexpr std::size_t kByte9EDD8 = 650712; // 0x9EDD8 // 0
constexpr std::size_t kDword9EE0C = 650764; // 0x9EE0C // 0
constexpr std::size_t kDword9F128 = 651560; // 0x9F128 // 0
constexpr std::size_t kByte9F12C = 651564; // 0x9F12C // 0
constexpr std::size_t kDword9F130 = 651568; // 0x9F130 // 0
constexpr std::size_t kBufFontsub = 652088; // 0x9F338 // inline subobj (0x408E70) ctor 0x42AE60 target
constexpr std::size_t kDwordA0268 = 655976; // 0xA0268 // 0
constexpr std::size_t kDwordA026C = 655980; // 0xA026C // 0
constexpr std::size_t kDwordA0270 = 655984; // 0xA0270 // 0
constexpr std::size_t kDwordA027C = 655996; // 0xA027C // 44
constexpr std::size_t kByteA02A8 = 656040; // 0xA02A8 // 0
constexpr std::size_t kByteA02B4 = 656052; // 0xA02B4 // 0
constexpr std::size_t kByteA02B5 = 656053; // 0xA02B5 // 0
constexpr std::size_t kByteA02B6 = 656054; // 0xA02B6 // 0
constexpr std::size_t kDwordA03BC = 656316; // 0xA03BC // 0
constexpr std::size_t kDwordA03C0 = 656320; // 0xA03C0 // 0
constexpr std::size_t kDwordA03C4 = 656324; // 0xA03C4 // 0
constexpr std::size_t kDwordA03D8 = 656344; // 0xA03D8 // 0 OpenNIGetVersion slot (0x429CB0)
constexpr std::size_t kByteA03DF = 656351; // 0xA03DF // 0
constexpr std::size_t kFloatA03e0 = 656352; // 0xA03E0 // 60.0 fps limit saved across Kinect capture (0x429CB0/0x42A020)
constexpr std::size_t kByteA03ea = 656362; // 0xA03EA // 0 DxOpenNI version code 0x0D=1.30/0x0E=1.40/0x0F=1.50 (0x429CB0, menu 292)
constexpr std::size_t kByteA03E4 = 656356; // 0xA03E4 // 1
constexpr std::size_t kByteA03E8 = 656360; // 0xA03E8 // 0
constexpr std::size_t kByteA03E9 = 656361; // 0xA03E9 // 0
constexpr std::size_t kDwordA0430 = 656432; // 0xA0430 // -1
constexpr std::size_t kByteA04B8 = 656568; // 0xA04B8 // 0
constexpr std::size_t kBufBuf656632 = 656632; // 0xA04F8 // zeroed 0xC8
constexpr std::size_t kDwordCol656864 = 656864; // 0xA05E0 // 4605510 COLORREF (color.txt)
constexpr std::size_t kDwordCol656868 = 656868; // 0xA05E4 // 1315860 COLORREF (color.txt)
constexpr std::size_t kDwordCol656976 = 656976; // 0xA0650 // 9895830 COLORREF (color.txt)
constexpr std::size_t kDwordCol656988 = 656988; // 0xA065C // 0 COLORREF (color.txt)
constexpr std::size_t kDwordCol656992 = 656992; // 0xA0660 // 6579455 COLORREF (color.txt)
constexpr std::size_t kByteA06B4 = 657076; // 0xA06B4 // 0
constexpr std::size_t kByteA06B5 = 657077; // 0xA06B5 // 0
constexpr std::size_t kByteA06B6 = 657078; // 0xA06B6 // 0
constexpr std::size_t kPtrHwnd = 657080; // 0xA06B8 // CreateWindowExA@0x47BD4B main HWND
constexpr std::size_t kPtrSub06c = 657088; // 0xA06C0 // ptr(0x6C obj)@0x47A698 subsystem ptr
constexpr std::size_t kPtrSub1d574 = 657092; // 0xA06C4 // ptr(0x1D574 obj)@0x47A5F2 locale/font subsystem (locale slots +120004..)
constexpr std::size_t kDwordSidebar = 657096; // 0xA06C8 // 250 min left panel width
constexpr std::size_t kWcsExedir = 657102; // 0xA06CE // Drive+Dir wchar[256] exe dir
constexpr std::size_t kFloatDeltatime = 657084; // 0xA06BC // per frame frame delta seconds (WinMain)
constexpr std::size_t kFloatCamangle = 657628; // 0xA08DC // -45.0
constexpr std::size_t kFloatFpslimit = 657632; // 0xA08E0 // 60.0 FPS cap
constexpr std::size_t kWcsEnvfile = 657664; // 0xA0900 // cmdline/default wchar[256] startup scene
constexpr std::size_t kDwordA0B10 = 658192; // 0xA0B10 // 0
constexpr std::size_t kDwordA0B44 = 658244; // 0xA0B44 // 0
constexpr std::size_t kByteA0B64 = 658276; // 0xA0B64 // 0
constexpr std::size_t kDwordTimenowlo = 658280; // 0xA0B68 // timeGetTime
constexpr std::size_t kDwordTimenowhi = 658284; // 0xA0B6C // timeGetTime
constexpr std::size_t kDwordA0B74 = 658292; // 0xA0B74 // 0
constexpr std::size_t kDwordA0B7C = 658300; // 0xA0B7C // 0
constexpr std::size_t kFloatPhysicsint = 658732; // 0xA0D2C // 0.01125 candidate physics interval
constexpr std::size_t kDwordA0D38 = 658744; // 0xA0D38 // 0
constexpr std::size_t kDwordA0D6C = 658796; // 0xA0D6C // 1
constexpr std::size_t kByteFlag672800 = 672800; // 0xA4420 // mmconfig.ini
constexpr std::size_t kDwordVal672804 = 672804; // 0xA4424 // 100
constexpr std::size_t kByteOptflag0 = 760; // 0x2F8 // 1 UI option flag
constexpr std::size_t kByteOptflag1 = 761; // 0x2F9 // 1 UI option flag
constexpr std::size_t kByteOptflag2 = 762; // 0x2FA // 1 UI option flag
constexpr std::size_t kByteOptflag3 = 763; // 0x2FB // 1 UI option flag
constexpr std::size_t kByteOptflag4 = 764; // 0x2FC // 1 UI option flag
constexpr std::size_t kByteOptflag5 = 765; // 0x2FD // 1 UI option flag
constexpr std::size_t kByteOptflag6 = 766; // 0x2FE // 1 UI option flag
constexpr std::size_t kByteA0CD4 = 658644; // 0xA0CD4 // 0
constexpr std::size_t kByteB6568481 = 656849; // 0xA05D1 // 0
constexpr std::size_t kByteB6568482 = 656850; // 0xA05D2 // 0
constexpr std::size_t kByteB6568483 = 656851; // 0xA05D3 // 0
constexpr std::size_t kByteA0665 = 656997; // 0xA0665 // 0
constexpr std::size_t kByteA066C = 657004; // 0xA066C // 0
constexpr std::size_t kByteA066D = 657005; // 0xA066D // 0
constexpr std::size_t kDwordMsgseen = 658796; // 0xA0D6C // 1 on every message WndProc 0x4C3A10
constexpr std::size_t kPtrHfontui = 656572; // 0xA04BC // Tahoma13/MS_PGothic12 0x466D20 font phase
constexpr std::size_t kBufLogfont = 656508; // 0xA047C // lfMessageFont copy 0x466D20 font phase
constexpr std::size_t kDwordToolhover = 836; // 0x344 // 0/1 viewport tool hover 0x470243..0x470BE1
constexpr std::size_t kDwordViewdragmode = 840; // 0x348 // 0..2 view tool drag 0x473B1E
constexpr std::size_t kDwordInteractionmode = 844; // 0x34C // 0..0x12 operation drag 0x473B5C..0x47477D
constexpr std::size_t kPtrToontex = 650720; // 0x9EDE0 // 11 texture slots 0x424DC0
constexpr std::size_t kFloatToonedge = 655632; // 0xA0110 // 37-float edge table 0x424DC0
constexpr std::size_t kPtrFonttex = 650784; // 0x9EE20 // scene font texture 0x42AE80
constexpr std::size_t kByteF9edd0 = 650704; // 0x9EDD0 // timeline flag 0x46B090
constexpr std::size_t kByteFa03b7 = 656311; // 0xA03B7 // timeline flag 2 0x46B090
constexpr std::size_t kByteFa04b8 = 656568; // 0xA04B8 // reload flag 0x46B090
constexpr std::size_t kByteFa0478 = 656504; // 0xA0478 // reload aux 0x46B090
constexpr std::size_t kDwordFa0d6c = 658796; // 0xA0D6C // reset guard 0x46B090
constexpr std::size_t kByteBa0d61 = 658785; // 0xA0D61 // reset guard 2 0x46B090
constexpr std::size_t kDwordFa0274 = 655988; // 0xA0274 // windowed/device-lost pump 0x46B090
constexpr std::size_t kByteF9edd8 = 650712; // 0x9EDD8 // anim frame flag 0x46B090
constexpr std::size_t kDwordF9eddc = 650716; // 0x9EDDC // anim frame source 0x46B090
constexpr std::size_t kDwordFa0b00D = 658432; // 0xA0C00 // same slot dword view frame_driver.cpp raw<u32>
constexpr std::size_t kDwordFa0b04D = 658436; // 0xA0C04 // same slot dword view frame_driver.cpp raw<u32>
constexpr std::size_t kFloatF9e654 = 648788; // 0x9E654 // frameA/30 0x46B090
constexpr std::size_t kFloatF9e658 = 648792; // 0x9E658 // frameB/30 0x46B090
constexpr std::size_t kDwordF9e648 = 648776; // 0x9E648 // frameA copy 0x46B090
constexpr std::size_t kDwordF9e64c = 648780; // 0x9E64C // frameB copy 0x46B090
constexpr std::size_t kByteB9edb6 = 650678; // 0x9EDB6 // frame equality flag 0x46B090
constexpr std::size_t kDwordF9ed94 = 650644; // 0x9ED94 // anim aux 0x46B090
constexpr std::size_t kByteB9ed99 = 650649; // 0x9ED99 // display toggle 0x47E8A0 0xD7
constexpr std::size_t kByteB9ed9a = 650650; // 0x9ED9A // display toggle 0x47E8A0 0xD8
constexpr std::size_t kByteB9edd1 = 650705; // 0x9EDD1 // drag active 0x46B090
constexpr std::size_t kByteSlotidx = 2320; // 0x910 // model slot order index 0x466D20/0x441AD0
constexpr std::size_t kByte9e170 = 647536; // 0x9E170 // selected light/accessory slot 0x463640 light acc index; alias of 450-local kOff9E170
constexpr std::size_t kFloatEulery = 656580; // 0xA04C4 // 0.0 edit euler work Y 0x44BEF0
constexpr std::size_t kFloatEulerz = 656584; // 0xA04C8 // 0.0 edit euler work Z 0x44BEF0
constexpr std::size_t kWcsWavpath = 208; // 0xD0 // 0 0x41DC85 wide path readback source
constexpr std::size_t kBufAcctrk = 900; // 0x384 // 0 0x384 accessory key-track ptr array (255), abuts 0x780 slots
constexpr std::size_t kFloat9e1e8 = 647656; // 0x9E1E8 // 30.0 0x41B22E fov W4
constexpr std::size_t kWcs9e1ec = 647660; // 0x9E1EC // 0 0x41DCC6 wide buf, %s-quirk swprintf_s target
constexpr std::size_t kByteA0188 = 655752; // 0xA0188 // 0 0x41E25E selfshadow config bool
constexpr std::size_t kPtrBmp752 = 752; // 0x2F0 // bitmap 0x65 (101) LoadBitmapA 0x467303
constexpr std::size_t kPtrBmp756 = 756; // 0x2F4 // bitmap 0x77 (119) LoadBitmapA 0x467311
constexpr std::size_t kDword9eda8 = 650664; // 0x9EDA8 // 0 frame-driver T0 (0x46F38E)
constexpr std::size_t kDword9edac = 650668; // 0x9EDAC // 0 frame-driver T1/QPC (0x46F388 region)
constexpr std::size_t kDwordScrollCb = 2372; // 0x944 // 28 (cbSize) SCROLLINFO@2372 (0x47C0A0)
constexpr std::size_t kDwordScrollFmask = 2376; // 0x948 // 7 SIF_ALL
constexpr std::size_t kDwordScrollNmin = 2380; // 0x94C // 0
constexpr std::size_t kDwordScrollNmax = 2384; // 0x950 // bone-list range
constexpr std::size_t kDwordScrollNpage = 2388; // 0x954 // (bottom-398)/14-1
constexpr std::size_t kDwordScrollNpos = 2392; // 0x958 // bone-list pos
constexpr std::size_t kDwordA0d38 = 658744; // 0xA0D38 // accessory-column hide gate
constexpr std::size_t kDwordHideTop = 658756; // 0xA0D44 // hide rect top
constexpr std::size_t kDwordHideBottom = 658764; // 0xA0D4C // hide rect bottom
constexpr std::size_t kDwordBoneDragMode = 844; // 0x34C // bone-drag stage mode 1..6 0x475A6E
constexpr std::size_t kDwordA0b74 = 658292; // 0xA0B74 // 0 = bone edit (vs accessory/light)
constexpr std::size_t kPtrA0b7c = 658300; // 0xA0B7C // accessory records base (stride 0xAC)
constexpr std::size_t kPtrA0c30 = 658480; // 0xA0C30 // 0x8C-stride records base
constexpr std::size_t kBytePhysicsMode = 14590; // 0x38FE // 2=PMX physics mode 0x4B77E0
constexpr std::size_t kBufBufa02b7 = 656055; // 0xA02B7 // SJIS filename out (0x100) 0x42A160/0x42A650
constexpr std::size_t kDword9e3ec = 648172; // 0x9E3EC // 0 DrawDib handle (AVI bg playback)
constexpr std::size_t kDwordA02ac = 656044; // 0xA02AC // 0 record RT width snapshot (0x45E820)
constexpr std::size_t kDwordA02b0 = 656048; // 0xA02B0 // 0 record RT height snapshot (0x45E820)
constexpr std::size_t kDwordA0278 = 655992; // 0xA0278 // 0 saved HMENU across fullscreen record (0x4629D0)
constexpr std::size_t kStructA0ce0 = 658656; // 0xA0CE0 // D3DMATERIAL9, accessory projected-ground shadow
constexpr std::size_t kFloatA0cf0 = 658672; // 0xA0CF0 // Ambient.r = 1.0

}  // namespace mikudancestudio::offsets
