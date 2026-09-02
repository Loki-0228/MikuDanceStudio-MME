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
constexpr std::size_t kByteOptflag0 = 760; // 0x2F8 // 1 UI option flag
constexpr std::size_t kPtrSub04b0 = 650656; // 0x9EDA0 // ptr(0x4B0 obj)@0x47A6E1 subsystem ptr
constexpr std::size_t kDword300 = 768; // 0x300 // 0
constexpr std::size_t kDword304 = 772; // 0x304 // 0
constexpr std::size_t kFloatPosx = 820; // 0x334 // 0.0 (0,10,0) candidate
constexpr std::size_t kByte340 = 832; // 0x340 // 0
constexpr std::size_t kDword350 = 848; // 0x350 // 0
constexpr std::size_t kDword97C = 2428; // 0x97C // 0
constexpr std::size_t kDword980 = 2432; // 0x980 // 0
constexpr std::size_t kBufBuf9ddx = 646512; // 0x9DD70 // zeroed 0x3FC
constexpr std::size_t kPtrSub048 = 650672; // 0x9EDB0 // ptr(0x48 obj)@0x47A727 subsystem ptr
constexpr std::size_t kDword9F128 = 651560; // 0x9F128 // 0
constexpr std::size_t kBufFontsub = 652088; // 0x9F338 // inline subobj (0x408E70) ctor 0x42AE60 target
constexpr std::size_t kByteA03E4 = 656356; // 0xA03E4 // 1
constexpr std::size_t kDwordA0430 = 656432; // 0xA0430 // -1
constexpr std::size_t kDwordCol656864 = 656864; // 0xA05E0 // 4605510 COLORREF (color.txt)
constexpr std::size_t kDwordCol656868 = 656868; // 0xA05E4 // 1315860 COLORREF (color.txt)
constexpr std::size_t kPtrHwnd = 657080; // 0xA06B8 // CreateWindowExA@0x47BD4B main HWND
constexpr std::size_t kPtrSub06c = 657088; // 0xA06C0 // ptr(0x6C obj)@0x47A698 subsystem ptr
constexpr std::size_t kPtrSub1d574 = 657092; // 0xA06C4 // ptr(0x1D574 obj)@0x47A5F2 locale/font subsystem (locale slots +120004..)
constexpr std::size_t kDwordSidebar = 657096; // 0xA06C8 // 250 min left panel width
constexpr std::size_t kFloatCamangle = 657628; // 0xA08DC // -45.0
constexpr std::size_t kFloatFpslimit = 657632; // 0xA08E0 // 60.0 FPS cap
constexpr std::size_t kWcsEnvfile = 657664; // 0xA0900 // cmdline/default wchar[256] startup scene
constexpr std::size_t kFloatPhysicsint = 658732; // 0xA0D2C // 0.01125 candidate physics interval
constexpr std::size_t kDwordA0D38 = 658744; // 0xA0D38 // 0
constexpr std::size_t kDwordA0D6C = 658796; // 0xA0D6C // 1
constexpr std::size_t kDwordVal672804 = 672804; // 0xA4424 // 100
constexpr std::size_t kByteOptflag1 = 761; // 0x2F9 // 1 UI option flag
constexpr std::size_t kDwordMsgseen = 658796; // 0xA0D6C // 1 on every message WndProc 0x4C3A10
constexpr std::size_t kDwordToolhover = 836; // 0x344 // 0/1 viewport tool hover 0x470243..0x470BE1
constexpr std::size_t kDwordViewdragmode = 840; // 0x348 // 0..2 view tool drag 0x473B1E
constexpr std::size_t kDwordInteractionmode = 844; // 0x34C // 0..0x12 operation drag 0x473B5C..0x47477D
constexpr std::size_t kPtrToontex = 650720; // 0x9EDE0 // 11 texture slots 0x424DC0
constexpr std::size_t kPtrFonttex = 650784; // 0x9EE20 // scene font texture 0x42AE80
constexpr std::size_t kByteF9edd0 = 650704; // 0x9EDD0 // timeline flag 0x46B090
constexpr std::size_t kByteFa03b7 = 656311; // 0xA03B7 // timeline flag 2 0x46B090
constexpr std::size_t kByteFa04b8 = 656568; // 0xA04B8 // reload flag 0x46B090
constexpr std::size_t kByteFa0478 = 656504; // 0xA0478 // reload aux 0x46B090
constexpr std::size_t kDwordFa0d6c = 658796; // 0xA0D6C // reset guard 0x46B090
constexpr std::size_t kByteBa0d61 = 658785; // 0xA0D61 // reset guard 2 0x46B090
constexpr std::size_t kDwordFa0274 = 655988; // 0xA0274 // windowed/device-lost pump 0x46B090
constexpr std::size_t kByteF9edd8 = 650712; // 0x9EDD8 // anim frame flag 0x46B090
constexpr std::size_t kByteB9edb6 = 650678; // 0x9EDB6 // frame equality flag 0x46B090
constexpr std::size_t kByteB9ed99 = 650649; // 0x9ED99 // display toggle 0x47E8A0 0xD7
constexpr std::size_t kByteB9ed9a = 650650; // 0x9ED9A // display toggle 0x47E8A0 0xD8
constexpr std::size_t kByteB9edd1 = 650705; // 0x9EDD1 // drag active 0x46B090
constexpr std::size_t kByteSlotidx = 2320; // 0x910 // model slot order index 0x466D20/0x441AD0
constexpr std::size_t kByte9e170 = 647536; // 0x9E170 // selected light/accessory slot 0x463640 light acc index; alias of 450-local kOff9E170
constexpr std::size_t kWcsWavpath = 208; // 0xD0 // 0 0x41DC85 wide path readback source
constexpr std::size_t kBufAcctrk = 900; // 0x384 // 0 0x384 accessory key-track ptr array (255), abuts 0x780 slots
constexpr std::size_t kDword9eda8 = 650664; // 0x9EDA8 // 0 frame-driver T0 (0x46F38E)
constexpr std::size_t kDword9edac = 650668; // 0x9EDAC // 0 frame-driver T1/QPC (0x46F388 region)
constexpr std::size_t kDwordA0d38 = 658744; // 0xA0D38 // accessory-column hide gate
constexpr std::size_t kDwordHideTop = 658756; // 0xA0D44 // hide rect top
constexpr std::size_t kDwordHideBottom = 658764; // 0xA0D4C // hide rect bottom
constexpr std::size_t kDwordBoneDragMode = 844; // 0x34C // bone-drag stage mode 1..6 0x475A6E
constexpr std::size_t kBytePhysicsMode = 14590; // 0x38FE // 2=PMX physics mode 0x4B77E0
constexpr std::size_t kFloatA0cf0 = 658672; // 0xA0CF0 // Ambient.r = 1.0

}  // namespace mikudancestudio::offsets
