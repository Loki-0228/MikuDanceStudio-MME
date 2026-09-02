// ===========================================================================
// MikuDanceStudio - the application state object
// ===========================================================================
// Hand-maintained.  Placeholders (v<off>/f<off>/pad*) are promoted to real
// names as field semantics are recovered; keep each member's size/arch
// split intact while doing so - the offset pins below are the safety net:
//
//   * x86: every member's offset is pinned by static_assert against the
//     original binary layout (the ground truth this port mirrors).
//   * x64: anchor members are pinned; the rest keep their spacing relative
//     to the last anchor, and promoted members additionally carry an
//     xlate-sync assert so raw<T>(offset) callers keep landing correctly.
//
// Buffer widths are arch-invariant.  Unknown regions are byte arrays -
// never guessed.  Promotion worklist: python scripts/promote.py ledger
// ===========================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "mikudancestudio/offsets_xlate.hpp"

namespace mikudancestudio {

constexpr std::size_t kAppObjectSize =
    sizeof(void*) == 8 ? 0xA55E0 : 0xA4530;

template <std::size_t N>
struct RawPad { unsigned char b[N]; };
struct EmptyPad {};

struct MMDAppState {
    RawPad<48> pad0;
    // dialog-open flags (0x30..0x74): nonzero while the matching dialog
    // is up; the menu command switch reads them to reject re-entry
    std::int32_t dialogFlags[17];
    RawPad<72> pad0b;
    std::uint32_t bC;
    RawPad<8> pad1;
    unsigned char sidebarResizeDragging;
#if defined(_M_X64)
    RawPad<7> pad2;
#else
    RawPad<3> pad2;
#endif
    void* sub025c;  // x64 pin 208
    wchar_t wavPath[256];
#if defined(_M_X64)
    RawPad<3> pad4;
#endif
    unsigned char directSoundAvailable;
#if defined(_M_X64)
    RawPad<4> pad5;
#else
    RawPad<3> pad5;
#endif
    HDC hdcMainPanel;  // x64 pin 736
    void* bmpPanel;
    void* bmpPanelSpare;
    HDC hdcTimeline;
    void* bmpTimelineStrip;
    HDC hdcInterpCurve;
    void* bmpInterpCurve;
    void* bmpRes101;  // x64 pin 792
    void* bmpRes119;
    unsigned char optflag[7];  // camera/edit UI radio flags (0x2F8..0x2FE)
    RawPad<1> pad21;
    void* groundGridVertices;
    void* groundGridIndices;
    float viewOffsetX;
    float viewOffsetY;
    float cameraPitch;
    float cameraYaw;
    float cameraRoll;
    unsigned char cameraPerspective;
    unsigned char groundGridEnabled;
    unsigned char fpsOverlayEnabled;
    RawPad<1> pad31;
    float fpsOverlayElapsedSeconds;
#if defined(_M_X64)
    RawPad<4> pad32;
#endif
    std::uint32_t fpsOverlayFrameCount;
    std::uint32_t framesPerSecond;
    std::uint32_t v32c;
#if defined(_M_X64)
    RawPad<1> pad35;
#endif
    unsigned char playbackActive;
#if defined(_M_X64)
    RawPad<6> pad36;
#else
    RawPad<3> pad36;
#endif
    float cameraPosX;
    float cameraPosY;
#if defined(_M_X64)
    RawPad<4> pad38;
#endif
    float cameraPosZ;
    unsigned char cameraReferenceMode;
    unsigned char playbackLoopEnabled;
    unsigned char v342;
    RawPad<1> pad42;
    std::uint32_t viewportToolHovered;  // aka kDwordToolhover
#if defined(_M_X64)
    RawPad<4> pad43;
#endif
    std::uint32_t viewToolDragOperation;  // aka kDwordViewdragmode
    std::uint32_t interactionDragMode;  // aka kDwordBoneDragMode, kDwordInteractionmode
    std::uint32_t v350;
#if defined(_M_X64)
    RawPad<12> pad46;
#else
    RawPad<8> pad46;
#endif
    void* displayClipboard;  // x64 pin 936
#if defined(_M_X64)
    RawPad<32> pad47;
#else
    RawPad<20> pad47;
#endif
    void* cameraKeyTrack;  // x64 pin 976
    void* lightKeyTrack;  // x64 pin 984
    void* selfShadowKeyTrack;  // x64 pin 992
    void* gravityKeyTrack;  // x64 pin 1000
    void* accKeyTracks[255];  // x64 pin 1008
#if defined(_M_X64)
    void* modelSlots[255];  // x64 pin 3048
#else
    void* modelSlots[100];  // x64 pin 3048
#endif
    unsigned char slotIdx;  // x64 pin 5088
    RawPad<3> pad54;
    std::int32_t editMode;
    unsigned char groundShadowEnabled;
    RawPad<3> pad56;
    std::uint32_t aviBackgroundEnabled;
    RawPad<12> pad57;
    std::uint32_t viewportToolOperation;
    std::int32_t dragOriginX;
    std::int32_t dragOriginY;
    RawPad<8> pad60;
    std::int32_t boneBoxSelectionActive;
    std::int32_t scrollCbSize;
    std::int32_t scrollFMask;
    std::int32_t scrollNMin;
    std::int32_t scrollNMax;
    std::int32_t scrollNPage;
    std::int32_t scrollNPos;
    RawPad<32> pad67;
    std::int32_t timelineStartFrame;
    std::int32_t currentFrame;
    std::int32_t rowHitBone[200];  // x64 pin 5204
#if defined(_M_X64)
    RawPad<2823> pad70;
#else
    RawPad<5388> pad70;
#endif
    unsigned char pmxEncoding;
#ifndef _M_X64
    RawPad<1> pad71;
#endif
    unsigned char pmxIdxVert;
    RawPad<1> pad72;
    unsigned char pmxIdxBone;
#ifndef _M_X64
    RawPad<2> pad73;
#endif
    unsigned char pmxIdxRigid;
#if defined(_M_X64)
    RawPad<32> pad74;
#else
    RawPad<52> pad74;
#endif
    std::uint32_t morph0Count;
#if defined(_M_X64)
    RawPad<8> pad75;
#else
    RawPad<20> pad75;
#endif
    std::uint32_t physOffsetCount;
#if defined(_M_X64)
    RawPad<8> pad76;
#else
    RawPad<12> pad76;
#endif
    void* morph0Table;
    void* physOffsetRecords;
#if defined(_M_X64)
    RawPad<12> pad78;
#else
    RawPad<40> pad78;
#endif
    std::uint32_t physLastFrame;
#if defined(_M_X64)
    RawPad<3628> pad79;
#else
    RawPad<2872> pad79;
#endif
    short morphCount;
#if defined(_M_X64)
    RawPad<4> pad80;
#else
    RawPad<2> pad80;
#endif
    short boneCount;
#if defined(_M_X64)
    RawPad<4> pad81;
#else
    RawPad<2> pad81;
#endif
    short ikCount;
#if defined(_M_X64)
    RawPad<38> pad82;
#else
    RawPad<34> pad82;
#endif
    unsigned char facialFrameCount;
    RawPad<3> pad83;
    std::uint32_t rbGroupCount;
#if defined(_M_X64)
    RawPad<1068> pad84;
#else
    RawPad<1052> pad84;
#endif
    std::uint32_t rigidCount;
    std::uint32_t jointCount;
#if defined(_M_X64)
    RawPad<1850> pad86;
#else
    RawPad<1830> pad86;
#endif
    unsigned char physicsMode;
#if defined(_M_X64)
    RawPad<149669> pad87;
#else
    RawPad<147845> pad87;
#endif
    std::int32_t rowHitMorph[200];  // x64 pin 165204
    RawPad<151368> pad88;
    float centerBoneIndex;
    RawPad<7828> pad89;
    std::int32_t rowHitIk[200];  // x64 pin 325204
    RawPad<159200> pad90;
    std::int32_t rowHitBand0[200];  // x64 pin 485204
    std::int32_t rowHitBand1[200];  // x64 pin 486004
    std::int32_t rowHitBand2[200];  // x64 pin 486804
    std::int32_t rowHitBand3[200];  // x64 pin 487604
    std::int32_t rowHitAcc[800];  // x64 pin 488404
    RawPad<156805> pad95;
    unsigned char pendingTimelineSelectionRow;
    unsigned char lightA[6];
    unsigned char lightB[6];
    unsigned char lightC[6];
    unsigned char lightD[6];
    RawPad<2> pad100;
    std::int32_t v9da24[9];
    std::int32_t displayObjectListScrollPosition;
    std::int32_t displayObjectListMatchCount;
    std::int32_t jointLineMap[200];  // x64 pin 648480
    void* buf9ddx[255];  // x64 pin 649280
    std::uint32_t lastRegisteredFrame;  // x64 pin 651320
    unsigned char selLightAccSlotOrUint32;
    RawPad<3> pad115;
    float lightDirection;
    float v9e178;
    float v9e17c;
#if defined(_M_X64)
    RawPad<32> pad118;
#else
    RawPad<36> pad118;
#endif
    float lightColor;
    float v9e1a8;
    float v9e1ac;
#if defined(_M_X64)
    RawPad<24> pad121;
#else
    RawPad<28> pad121;
#endif
    std::uint32_t v9e1cc;
    RawPad<24> pad122;
    float cameraFov;
    wchar_t wcs9e1ec[256];
    void* drawDib;
    void* aviBackgroundTexture;
    void* aviBackgroundSurface;
    std::uint32_t v9e3f8OrPtr;
#if defined(_M_X64)
    RawPad<4> pad128;
#endif
    void* aviFile;
    void* v9e400OrUint32;
    void* aviFrameReader;
    std::int32_t aviStreamStart;
    std::int32_t aviStreamEnd;
    unsigned char aviUsesThirtyFpsTiming;
    RawPad<3> pad134;
    std::int32_t aviOffsetX;
    std::int32_t aviOffsetY;
    float aviScale;
    std::int32_t aviFrameWidth;
    std::int32_t aviFrameHeight;
    unsigned char v9e428;
#if defined(_M_X64)
    RawPad<7> pad140;
#else
    RawPad<3> pad140;
#endif
    void* v9e42cOrUint32;
    std::uint32_t v9e430OrPtr;
    std::int32_t pictureOffsetX;
    std::int32_t pictureOffsetY;
    float pictureScale;
    std::int32_t pictureWidth;
    std::int32_t pictureHeight;
    wchar_t pictureBackgroundPath[256];
    std::int32_t f9e648;
    float f9e64c;
    float v9e650;
    float f9e654;
    float f9e658;
#if defined(_M_X64)
    RawPad<1072> pad153;
#else
    RawPad<1314> pad153;
#endif
    unsigned char v9eb7e;
    unsigned char v9eb7f;
#if defined(_M_X64)
    RawPad<2> pad155;
#endif
    void* captureTexture;
    std::uint32_t captureMode;
#if defined(_M_X64)
    RawPad<4> pad157;
#endif
    void* captureRenderTarget;
    void* v9eb8c;
#if defined(_M_X64)
    // x64 blob keeps this region opaque; the path buffer itself lives
    // outside the compat blob (see MMDApp::m_aviOutputPath)
    RawPad<450> pad159;
#else
    wchar_t aviOutputPath[256];
#endif
    unsigned char v9ed90;
#if defined(_M_X64)
    RawPad<5> pad160;
#else
    RawPad<3> pad160;
#endif
    std::int32_t f9ed94;
    unsigned char v9ed98;  // aka kDwordF9ed98
    unsigned char playbackStartsAtCurrentFrame;  // aka kByteB9ed99
    unsigned char projectedShadowBlendEnabled;  // aka kByteB9ed9a
    RawPad<1> pad164;
    std::int32_t v9ed9c;
#if defined(_M_X64)
    RawPad<4> pad165;
#endif
    void* sub04b0OrUint32;
#ifndef _M_X64
    RawPad<4> pad166;
#endif
    std::uint32_t playbackClockAnchorLow;  // aka kDword9eda8
    std::uint32_t playbackClockAnchorHigh;  // aka kDword9edac
    void* physicsScene;
    unsigned char a9edb4;
    unsigned char physicsResetPending;
    unsigned char playbackFrameChanged;  // aka kByteB9edb6
    RawPad<1> pad172;
    float gravityX;
    float gravityY;
    float gravityZ;
    float gravityMagnitude;
    std::uint32_t gravityNoise;
    float v9edcc;
    unsigned char v9edd0;  // aka kByteF9edd0
    unsigned char viewportInputActive;  // aka kByteB9edd1
    RawPad<2> pad180;
    void* recordingCompletionFlag;
    unsigned char v9edd8;  // aka kByteF9edd8
    RawPad<3> pad182;
    std::int32_t f9eddc;
    void* toonTextures[11];
    void* v9ee0cOrUint32;
    std::uint32_t spriteOverlayPrimitiveCount;
#if defined(_M_X64)
    RawPad<4> pad186;
#endif
    void* overlayTexture;
    void* overlayVertices;
    std::uint32_t textOverlayPrimitiveCount;
#if defined(_M_X64)
    RawPad<4> pad189;
#endif
    void* sceneFontTexture;  // aka kPtrFonttex
    unsigned char recentFile0[256];
    unsigned char recentFile1[256];
    unsigned char recentFile2[256];
    std::uint32_t lineOverlayPrimitiveCount;
#if defined(_M_X64)
    RawPad<4> pad194;
#endif
    void* groundPlaneVertices;
    unsigned char v9f12c;
#if defined(_M_X64)
    RawPad<7> pad196;
#else
    RawPad<3> pad196;
#endif
    void* projectedShadowRestoreTexture;
#if defined(_M_X64)
    RawPad<296> pad197;
#else
    RawPad<512> pad197;
#endif
    void* captureReadbackPixels;
    unsigned char fontSubOrPtr;
#if defined(_M_X64)
    RawPad<3239> pad199;
#else
    RawPad<3535> pad199;
#endif
    void* leftViewportVertices;
    void* rightViewportVertices;
    float toonEdgeTable[30];
    unsigned char selfShadowCfgOrUint32;
    unsigned char selectionBoxDragging;
#ifndef _M_X64
    RawPad<10> pad204;
#endif
    unsigned char a0194;
    unsigned char modelOutlineRenderingSuppressed;
    unsigned char a0196;
    unsigned char a0197;
#if defined(_M_X64)
    RawPad<2> pad208;
#endif
    std::int32_t modelOutlineColorRed;
    std::int32_t modelOutlineColorGreen;
    std::int32_t modelOutlineColorBlue;
    unsigned char buf655780[64];
    unsigned char a01E4;
#if defined(_M_X64)
    RawPad<99> pad213;
#else
    RawPad<131> pad213;
#endif
    void* activeRenderObject;
    std::int32_t activeRenderPass;
    std::int32_t a0270;
    unsigned char fullscreenMode;  // aka kDwordFa0274
#if defined(_M_X64)
    RawPad<7> pad217;
#else
    RawPad<3> pad217;
#endif
    HMENU savedMenu;
    std::uint32_t a027COrBuf_bytes;
#if defined(_M_X64)
    RawPad<22> pad219;
#else
    RawPad<40> pad219;
#endif
    unsigned char a02A8;
#if defined(_M_X64)
    RawPad<5> pad220;
#else
    RawPad<3> pad220;
#endif
    std::int32_t recRTW;
    std::int32_t recRTH;
    unsigned char a02B4;
    unsigned char a02B5;
    unsigned char a02B6;
    unsigned char sjisOut[256];
    unsigned char a03B7;  // aka kByteFa03b7
    unsigned char depthDeviceEnabled;
    RawPad<3> pad228;
    HMODULE a03BC;
    void* a03C0;
    void* a03C4;
    std::uint32_t a03C8;
#if defined(_M_X64)
    RawPad<4> pad232;
#endif
    void* depthTextureCallback;
    std::uint32_t a03D0;
    std::uint32_t a03D4;
    void* a03D8;
    unsigned char a03DC;
    unsigned char a03DD;
    unsigned char depthTextureCompositionEnabled;
    unsigned char a03DF;
    float fpsLimitSaved;
    unsigned char a03E4;
    unsigned char a03E5;
    unsigned char a03E6;
    unsigned char a03E7;
    unsigned char a03E8;
    unsigned char automaticFrameAdvanceEnabled;
    unsigned char openniVersion;
    unsigned char timelineSelectionChanged;
#if defined(_M_X64)
    RawPad<8> pad249;
#else
    RawPad<64> pad249;
#endif
    std::uint32_t a042C;
    std::int32_t cameraParentModel;
    std::int32_t cameraParentBone;
    // 16-float basis/colour matrix (0xA0438..0xA0478); rows 0/2/4/6 of the
    // original camera-attachment basis default to identity diag 1.0
    float cameraAttachmentBasis[16];
    unsigned char cameraAttachmentTransformSuppressed;  // aka kByteFa0478
#ifndef _M_X64
    RawPad<3> pad269;
#endif
    unsigned char logFont[60];
    unsigned char a04B8;  // aka kByteFa04b8
#if defined(_M_X64)
    RawPad<6> pad271;
#else
    RawPad<3> pad271;
#endif
    HFONT hFontUI;
    float eulerX;
    float eulerY;
    float eulerZ;
    std::uint32_t brushes[11];
    unsigned char buf656632[64];
#if defined(_M_X64)
    RawPad<111> pad277;
#else
    RawPad<152> pad277;
#endif
    unsigned char b6568480;
    unsigned char b6568481;
    unsigned char b6568482;
    unsigned char b6568483;
    unsigned char uiTextRed;
    unsigned char uiTextGreen;
    unsigned char uiTextBlue;
#if defined(_M_X64)
    RawPad<2> pad284;
#else
    RawPad<1> pad284;
#endif
    // 35-entry UI theme colour table (0xA0C98..0xA0CDC)
    std::uint32_t themeColors[35];
#ifndef _M_X64
    RawPad<1> pad319;
#endif
    unsigned char a0665;
#if defined(_M_X64)
    RawPad<3> pad320;
#else
    RawPad<2> pad320;
#endif
    void* a0668OrUint32;
    unsigned char a066C;
    unsigned char a066D;
    RawPad<2> pad323;
    std::int32_t savedPlaybackPhysicsMode;
#if defined(_M_X64)
    RawPad<40> pad324;
#else
    RawPad<64> pad324;
#endif
    unsigned char a06B4;
    unsigned char a06B5;
    unsigned char a06B6;
#if defined(_M_X64)
    RawPad<5> pad327;
#else
    RawPad<1> pad327;
#endif
    HWND hwnd;
    float deltaTime;
#if defined(_M_X64)
    RawPad<4> pad329;
#endif
    void* sub06c;
    void* rendererOrLocaleTable;
    std::int32_t sidebarWidth;
    unsigned char waveEnabled;
    RawPad<1> pad333;
    wchar_t exeDir[256];
    RawPad<2> pad334;
    void* origEditProc;
    std::int32_t renderW;
    std::int32_t renderH;
    float cameraDistance;
    float fpsLimit;
    float fpsA;
    float fpsB;
    float fpsC;
#ifndef _M_X64
    RawPad<16> pad342;
#endif
    wchar_t envFileName[256];
    std::int32_t aviRecordStartFrame;
    std::int32_t aviRecordEndFrame;
    float aviRecordFps;  // aka kFloatA0b08
    unsigned char aviIncludeWave;
    unsigned char sceneModified;
    RawPad<2> pad348;
    std::uint32_t a0B10;
    std::int32_t a0B14OrPtr;
#ifndef _M_X64
    RawPad<8> pad350;
#endif
    std::int32_t accessoryRenderSplitOrder;
#ifndef _M_X64
    RawPad<32> pad351;
#endif
    HWND a0B44OrInt32;
#ifndef _M_X64
    RawPad<4> pad352;
#endif
    unsigned char englishUI;
    RawPad<3> pad353;
#if defined(_M_X64)
    // x64: the frame-range dialog HWND lives outside the blob (MMDApp)
    std::int32_t a0B50OrPtr;
#else
    HWND frameRangeDialog;
#endif
#ifndef _M_X64
    RawPad<16> pad354;
#endif
    unsigned char a0B64;
    RawPad<3> pad355;
    std::uint32_t timeNowLow;
    std::uint32_t timeNowHigh;
    float milliToSec;
    HWND a0b74OrInt32;  // aka kDwordA0b74
#ifndef _M_X64
    RawPad<4> pad359;
#endif
    void* a0B7COrInt32;  // aka kPtrA0b7c
#if defined(_M_X64)
    RawPad<60> pad360;
#else
    RawPad<128> pad360;
#endif
    std::int32_t fa0b00D;
    std::int32_t fa0b04D;
#if defined(_M_X64)
    RawPad<32> pad362;
#else
    RawPad<36> pad362;
#endif
    std::int32_t selAcc;
    unsigned char records8c[4];
#if defined(_M_X64)
    RawPad<128> pad364;
#else
    RawPad<140> pad364;
#endif
    std::int32_t sel8c;
    std::int32_t playbackPhysicsMode;
    unsigned char a0CC8OrUint32;
#if defined(_M_X64)
    // x64: HWND slot kept outside the compat blob (MMDApp::m_accessoryFrameDialog)
    RawPad<9> pad367;
#else
    RawPad<3> pad367;
    HWND accessoryFrameDialog;  // 0xA0CCC
    RawPad<4> pad367b;
#endif
    unsigned char a0CD4;
#if defined(_M_X64)
    // x64: option kept outside the compat blob (MMDApp::m_aviCodecSelection)
    RawPad<9> pad368;
#else
    RawPad<3> pad368;
    std::int32_t aviCodecSelection;
#endif
    void* origTrackProc;
    unsigned char aviSettings;
#if defined(_M_X64)
    RawPad<3> pad370;
#else
    RawPad<11> pad370;
#endif
    float projectedShadowDiffuseAlpha;
    float projectedShadowAmbientIntensity;  // aka kFloatA0cf0
    float projectedShadowAmbientG;
    float projectedShadowAmbientB;
    float projectedShadowAmbientA;
    RawPad<12> pad375;
    float projectedShadowSpecularAlpha;
#if defined(_M_X64)
    RawPad<16> pad376;
#else
    RawPad<20> pad376;
#endif
    HWND recordingWindow;
    unsigned char selfShadowCompositionEnabled;
    RawPad<3> pad378;
    float physicsInterval;  // aka kFloatPhysicsint
    std::int32_t selfShadowMode;
    RawPad<4> pad380;
    HWND floatingWindow;  // aka kDwordA0d38
    std::int32_t separateWindowSidebarWidth;
    std::int32_t hideRight;
    std::int32_t hideTop;
    std::int32_t hideLeft;
    std::int32_t hideBottom;
    std::int32_t separateWindowX;
    std::int32_t separateWindowY;
    std::int32_t separateWindowWidth;
    std::int32_t separateWindowHeight;
    unsigned char separateWindowMaximized;
    unsigned char aviStereoOutput;  // aka kByteBa0d61
    RawPad<2> pad392;
    std::int32_t aviStereoWidthMultiplier;
    unsigned char autoRepeat;
    RawPad<3> pad394;
    std::uint32_t messageSeen;  // aka kDwordFa0d6c, kDwordMsgseen
    wchar_t dirModel[1000];
    wchar_t dirUser[1000];
    wchar_t dirAccs[1000];
    wchar_t dirMotion[1000];
    wchar_t dirPose[1000];
    wchar_t dirWave[1000];
    wchar_t dirBg[1000];
    unsigned char flag672800;
    RawPad<3> pad403;
    std::int32_t val672804;
    float sidebarRatio;
    unsigned char windowLayoutReady;
    // 0x100-byte load-status text buffer (sprintf_s target of the PMM
    // load-failure prompts), then the tail pad
    char statusText[256];
#if defined(_M_X64)
    RawPad<1407> padTail;
#else
    RawPad<3> padTail;
#endif
};

#ifndef _M_X64
static_assert(sizeof(MMDAppState) == kAppObjectSize,
              "x86 app state size must match the original");
#else
// x64 interior layout is provisional until the contested anchors
// return; bound the size to the neighbourhood of the 0xA55E0 truth
static_assert(sizeof(MMDAppState) <= 0xA55E0 + 2048 &&
              sizeof(MMDAppState) >= 0xA55E0 - 2048,
              "x64 app state size must be near the original");
#endif

#ifndef _M_X64
// every field pinned to its x86 offsets.hpp position
static_assert(offsetof(MMDAppState, dialogFlags) == 48,
              "dialogFlags x86");
static_assert(offsetof(MMDAppState, dialogFlags[16]) == 112,
              "dialogFlags[16] x86");
static_assert(offsetof(MMDAppState, bC) == 188,
              "bC x86");
static_assert(offsetof(MMDAppState, optflag) == 760,
              "optflag x86");
static_assert(offsetof(MMDAppState, optflag[6]) == 766,
              "optflag[6] x86");
static_assert(offsetof(MMDAppState, sidebarResizeDragging) == 200,
              "sidebarResizeDragging x86");
static_assert(offsetof(MMDAppState, sub025c) == 204,
              "sub025c x86");
static_assert(offsetof(MMDAppState, wavPath) == 208,
              "wavPath x86");
static_assert(offsetof(MMDAppState, directSoundAvailable) == 720,
              "directSoundAvailable x86");
static_assert(offsetof(MMDAppState, hdcMainPanel) == 724,
              "hdcMainPanel x86");
static_assert(offsetof(MMDAppState, bmpPanel) == 728,
              "bmpPanel x86");
static_assert(offsetof(MMDAppState, bmpPanelSpare) == 732,
              "bmpPanelSpare x86");
static_assert(offsetof(MMDAppState, hdcTimeline) == 736,
              "hdcTimeline x86");
static_assert(offsetof(MMDAppState, bmpTimelineStrip) == 740,
              "bmpTimelineStrip x86");
static_assert(offsetof(MMDAppState, hdcInterpCurve) == 744,
              "hdcInterpCurve x86");
static_assert(offsetof(MMDAppState, bmpInterpCurve) == 748,
              "bmpInterpCurve x86");
static_assert(offsetof(MMDAppState, bmpRes101) == 752,
              "bmpRes101 x86");
static_assert(offsetof(MMDAppState, bmpRes119) == 756,
              "bmpRes119 x86");
static_assert(offsetof(MMDAppState, groundGridVertices) == 768,
              "groundGridVertices x86");
static_assert(offsetof(MMDAppState, groundGridIndices) == 772,
              "groundGridIndices x86");
static_assert(offsetof(MMDAppState, viewOffsetX) == 776,
              "viewOffsetX x86");
static_assert(offsetof(MMDAppState, viewOffsetY) == 780,
              "viewOffsetY x86");
static_assert(offsetof(MMDAppState, cameraPitch) == 784,
              "cameraPitch x86");
static_assert(offsetof(MMDAppState, cameraYaw) == 788,
              "cameraYaw x86");
static_assert(offsetof(MMDAppState, cameraRoll) == 792,
              "cameraRoll x86");
static_assert(offsetof(MMDAppState, cameraPerspective) == 796,
              "cameraPerspective x86");
static_assert(offsetof(MMDAppState, groundGridEnabled) == 797,
              "groundGridEnabled x86");
static_assert(offsetof(MMDAppState, fpsOverlayEnabled) == 798,
              "fpsOverlayEnabled x86");
static_assert(offsetof(MMDAppState, fpsOverlayElapsedSeconds) == 800,
              "fpsOverlayElapsedSeconds x86");
static_assert(offsetof(MMDAppState, fpsOverlayFrameCount) == 804,
              "fpsOverlayFrameCount x86");
static_assert(offsetof(MMDAppState, framesPerSecond) == 808,
              "framesPerSecond x86");
static_assert(offsetof(MMDAppState, v32c) == 812,
              "v32c x86");
static_assert(offsetof(MMDAppState, playbackActive) == 816,
              "playbackActive x86");
static_assert(offsetof(MMDAppState, cameraPosX) == 820,
              "cameraPosX x86");
static_assert(offsetof(MMDAppState, cameraPosY) == 824,
              "cameraPosY x86");
static_assert(offsetof(MMDAppState, cameraPosZ) == 828,
              "cameraPosZ x86");
static_assert(offsetof(MMDAppState, cameraReferenceMode) == 832,
              "cameraReferenceMode x86");
static_assert(offsetof(MMDAppState, playbackLoopEnabled) == 833,
              "playbackLoopEnabled x86");
static_assert(offsetof(MMDAppState, v342) == 834,
              "v342 x86");
static_assert(offsetof(MMDAppState, viewportToolHovered) == 836,
              "viewportToolHovered x86");
static_assert(offsetof(MMDAppState, viewToolDragOperation) == 840,
              "viewToolDragOperation x86");
static_assert(offsetof(MMDAppState, interactionDragMode) == 844,
              "interactionDragMode x86");
static_assert(offsetof(MMDAppState, v350) == 848,
              "v350 x86");
static_assert(offsetof(MMDAppState, displayClipboard) == 860,
              "displayClipboard x86");
static_assert(offsetof(MMDAppState, cameraKeyTrack) == 884,
              "cameraKeyTrack x86");
static_assert(offsetof(MMDAppState, lightKeyTrack) == 888,
              "lightKeyTrack x86");
static_assert(offsetof(MMDAppState, selfShadowKeyTrack) == 892,
              "selfShadowKeyTrack x86");
static_assert(offsetof(MMDAppState, gravityKeyTrack) == 896,
              "gravityKeyTrack x86");
static_assert(offsetof(MMDAppState, accKeyTracks) == 900,
              "accKeyTracks x86");
static_assert(offsetof(MMDAppState, modelSlots) == 1920,
              "modelSlots x86");
static_assert(offsetof(MMDAppState, slotIdx) == 2320,
              "slotIdx x86");
static_assert(offsetof(MMDAppState, editMode) == 2324,
              "editMode x86");
static_assert(offsetof(MMDAppState, groundShadowEnabled) == 2328,
              "groundShadowEnabled x86");
static_assert(offsetof(MMDAppState, aviBackgroundEnabled) == 2332,
              "aviBackgroundEnabled x86");
static_assert(offsetof(MMDAppState, viewportToolOperation) == 2348,
              "viewportToolOperation x86");
static_assert(offsetof(MMDAppState, dragOriginX) == 2352,
              "dragOriginX x86");
static_assert(offsetof(MMDAppState, dragOriginY) == 2356,
              "dragOriginY x86");
static_assert(offsetof(MMDAppState, boneBoxSelectionActive) == 2368,
              "boneBoxSelectionActive x86");
static_assert(offsetof(MMDAppState, scrollCbSize) == 2372,
              "scrollCbSize x86");
static_assert(offsetof(MMDAppState, scrollFMask) == 2376,
              "scrollFMask x86");
static_assert(offsetof(MMDAppState, scrollNMin) == 2380,
              "scrollNMin x86");
static_assert(offsetof(MMDAppState, scrollNMax) == 2384,
              "scrollNMax x86");
static_assert(offsetof(MMDAppState, scrollNPage) == 2388,
              "scrollNPage x86");
static_assert(offsetof(MMDAppState, scrollNPos) == 2392,
              "scrollNPos x86");
static_assert(offsetof(MMDAppState, timelineStartFrame) == 2428,
              "timelineStartFrame x86");
static_assert(offsetof(MMDAppState, currentFrame) == 2432,
              "currentFrame x86");
static_assert(offsetof(MMDAppState, rowHitBone) == 2436,
              "rowHitBone x86");
static_assert(offsetof(MMDAppState, pmxEncoding) == 8624,
              "pmxEncoding x86");
static_assert(offsetof(MMDAppState, pmxIdxVert) == 8626,
              "pmxIdxVert x86");
static_assert(offsetof(MMDAppState, pmxIdxBone) == 8628,
              "pmxIdxBone x86");
static_assert(offsetof(MMDAppState, pmxIdxRigid) == 8631,
              "pmxIdxRigid x86");
static_assert(offsetof(MMDAppState, morph0Count) == 8684,
              "morph0Count x86");
static_assert(offsetof(MMDAppState, physOffsetCount) == 8708,
              "physOffsetCount x86");
static_assert(offsetof(MMDAppState, morph0Table) == 8724,
              "morph0Table x86");
static_assert(offsetof(MMDAppState, physOffsetRecords) == 8728,
              "physOffsetRecords x86");
static_assert(offsetof(MMDAppState, physLastFrame) == 8772,
              "physLastFrame x86");
static_assert(offsetof(MMDAppState, morphCount) == 11648,
              "morphCount x86");
static_assert(offsetof(MMDAppState, boneCount) == 11652,
              "boneCount x86");
static_assert(offsetof(MMDAppState, ikCount) == 11656,
              "ikCount x86");
static_assert(offsetof(MMDAppState, facialFrameCount) == 11692,
              "facialFrameCount x86");
static_assert(offsetof(MMDAppState, rbGroupCount) == 11696,
              "rbGroupCount x86");
static_assert(offsetof(MMDAppState, rigidCount) == 12752,
              "rigidCount x86");
static_assert(offsetof(MMDAppState, jointCount) == 12756,
              "jointCount x86");
static_assert(offsetof(MMDAppState, physicsMode) == 14590,
              "physicsMode x86");
static_assert(offsetof(MMDAppState, rowHitMorph) == 162436,
              "rowHitMorph x86");
static_assert(offsetof(MMDAppState, centerBoneIndex) == 314604,
              "centerBoneIndex x86");
static_assert(offsetof(MMDAppState, rowHitIk) == 322436,
              "rowHitIk x86");
static_assert(offsetof(MMDAppState, rowHitBand0) == 482436,
              "rowHitBand0 x86");
static_assert(offsetof(MMDAppState, rowHitBand1) == 483236,
              "rowHitBand1 x86");
static_assert(offsetof(MMDAppState, rowHitBand2) == 484036,
              "rowHitBand2 x86");
static_assert(offsetof(MMDAppState, rowHitBand3) == 484836,
              "rowHitBand3 x86");
static_assert(offsetof(MMDAppState, rowHitAcc) == 485636,
              "rowHitAcc x86");
static_assert(offsetof(MMDAppState, pendingTimelineSelectionRow) == 645641,
              "pendingTimelineSelectionRow x86");
static_assert(offsetof(MMDAppState, lightA) == 645642,
              "lightA x86");
static_assert(offsetof(MMDAppState, lightB) == 645648,
              "lightB x86");
static_assert(offsetof(MMDAppState, lightC) == 645654,
              "lightC x86");
static_assert(offsetof(MMDAppState, lightD) == 645660,
              "lightD x86");
static_assert(offsetof(MMDAppState, v9da24) == 645668,
              "v9da24 x86");
static_assert(offsetof(MMDAppState, v9da24[8]) == 645700,
              "v9da24[8] x86");
static_assert(offsetof(MMDAppState, displayObjectListScrollPosition) == 645704,
              "displayObjectListScrollPosition x86");
static_assert(offsetof(MMDAppState, displayObjectListMatchCount) == 645708,
              "displayObjectListMatchCount x86");
static_assert(offsetof(MMDAppState, jointLineMap) == 645712,
              "jointLineMap x86");
static_assert(offsetof(MMDAppState, buf9ddx) == 646512,
              "buf9ddx x86");
static_assert(offsetof(MMDAppState, lastRegisteredFrame) == 647532,
              "lastRegisteredFrame x86");
static_assert(offsetof(MMDAppState, selLightAccSlotOrUint32) == 647536,
              "selLightAccSlotOrUint32 x86");
static_assert(offsetof(MMDAppState, lightDirection) == 647540,
              "lightDirection x86");
static_assert(offsetof(MMDAppState, v9e178) == 647544,
              "v9e178 x86");
static_assert(offsetof(MMDAppState, v9e17c) == 647548,
              "v9e17c x86");
static_assert(offsetof(MMDAppState, lightColor) == 647588,
              "lightColor x86");
static_assert(offsetof(MMDAppState, v9e1a8) == 647592,
              "v9e1a8 x86");
static_assert(offsetof(MMDAppState, v9e1ac) == 647596,
              "v9e1ac x86");
static_assert(offsetof(MMDAppState, v9e1cc) == 647628,
              "v9e1cc x86");
static_assert(offsetof(MMDAppState, cameraFov) == 647656,
              "cameraFov x86");
static_assert(offsetof(MMDAppState, wcs9e1ec) == 647660,
              "wcs9e1ec x86");
static_assert(offsetof(MMDAppState, drawDib) == 648172,
              "drawDib x86");
static_assert(offsetof(MMDAppState, aviBackgroundTexture) == 648176,
              "aviBackgroundTexture x86");
static_assert(offsetof(MMDAppState, aviBackgroundSurface) == 648180,
              "aviBackgroundSurface x86");
static_assert(offsetof(MMDAppState, v9e3f8OrPtr) == 648184,
              "v9e3f8OrPtr x86");
static_assert(offsetof(MMDAppState, aviFile) == 648188,
              "aviFile x86");
static_assert(offsetof(MMDAppState, v9e400OrUint32) == 648192,
              "v9e400OrUint32 x86");
static_assert(offsetof(MMDAppState, aviFrameReader) == 648196,
              "aviFrameReader x86");
static_assert(offsetof(MMDAppState, aviStreamStart) == 648200,
              "aviStreamStart x86");
static_assert(offsetof(MMDAppState, aviStreamEnd) == 648204,
              "aviStreamEnd x86");
static_assert(offsetof(MMDAppState, aviUsesThirtyFpsTiming) == 648208,
              "aviUsesThirtyFpsTiming x86");
static_assert(offsetof(MMDAppState, aviOffsetX) == 648212,
              "aviOffsetX x86");
static_assert(offsetof(MMDAppState, aviOffsetY) == 648216,
              "aviOffsetY x86");
static_assert(offsetof(MMDAppState, aviScale) == 648220,
              "aviScale x86");
static_assert(offsetof(MMDAppState, aviFrameWidth) == 648224,
              "aviFrameWidth x86");
static_assert(offsetof(MMDAppState, aviFrameHeight) == 648228,
              "aviFrameHeight x86");
static_assert(offsetof(MMDAppState, v9e428) == 648232,
              "v9e428 x86");
static_assert(offsetof(MMDAppState, v9e42cOrUint32) == 648236,
              "v9e42cOrUint32 x86");
static_assert(offsetof(MMDAppState, v9e430OrPtr) == 648240,
              "v9e430OrPtr x86");
static_assert(offsetof(MMDAppState, pictureOffsetX) == 648244,
              "pictureOffsetX x86");
static_assert(offsetof(MMDAppState, pictureOffsetY) == 648248,
              "pictureOffsetY x86");
static_assert(offsetof(MMDAppState, pictureScale) == 648252,
              "pictureScale x86");
static_assert(offsetof(MMDAppState, pictureWidth) == 648256,
              "pictureWidth x86");
static_assert(offsetof(MMDAppState, pictureHeight) == 648260,
              "pictureHeight x86");
static_assert(offsetof(MMDAppState, pictureBackgroundPath) == 648264,
              "pictureBackgroundPath x86");
static_assert(offsetof(MMDAppState, f9e648) == 648776,
              "f9e648 x86");
static_assert(offsetof(MMDAppState, f9e64c) == 648780,
              "f9e64c x86");
static_assert(offsetof(MMDAppState, v9e650) == 648784,
              "v9e650 x86");
static_assert(offsetof(MMDAppState, f9e654) == 648788,
              "f9e654 x86");
static_assert(offsetof(MMDAppState, f9e658) == 648792,
              "f9e658 x86");
static_assert(offsetof(MMDAppState, v9eb7e) == 650110,
              "v9eb7e x86");
static_assert(offsetof(MMDAppState, v9eb7f) == 650111,
              "v9eb7f x86");
static_assert(offsetof(MMDAppState, captureTexture) == 650112,
              "captureTexture x86");
static_assert(offsetof(MMDAppState, captureMode) == 650116,
              "captureMode x86");
static_assert(offsetof(MMDAppState, captureRenderTarget) == 650120,
              "captureRenderTarget x86");
static_assert(offsetof(MMDAppState, v9eb8c) == 650124,
              "v9eb8c x86");
#ifndef _M_X64
static_assert(offsetof(MMDAppState, aviOutputPath) == 650128,
              "aviOutputPath x86");
#endif
static_assert(offsetof(MMDAppState, v9ed90) == 650640,
              "v9ed90 x86");
static_assert(offsetof(MMDAppState, f9ed94) == 650644,
              "f9ed94 x86");
static_assert(offsetof(MMDAppState, v9ed98) == 650648,
              "v9ed98 x86");
static_assert(offsetof(MMDAppState, playbackStartsAtCurrentFrame) == 650649,
              "playbackStartsAtCurrentFrame x86");
static_assert(offsetof(MMDAppState, projectedShadowBlendEnabled) == 650650,
              "projectedShadowBlendEnabled x86");
static_assert(offsetof(MMDAppState, v9ed9c) == 650652,
              "v9ed9c x86");
static_assert(offsetof(MMDAppState, sub04b0OrUint32) == 650656,
              "sub04b0OrUint32 x86");
static_assert(offsetof(MMDAppState, playbackClockAnchorLow) == 650664,
              "playbackClockAnchorLow x86");
static_assert(offsetof(MMDAppState, playbackClockAnchorHigh) == 650668,
              "playbackClockAnchorHigh x86");
static_assert(offsetof(MMDAppState, physicsScene) == 650672,
              "physicsScene x86");
static_assert(offsetof(MMDAppState, a9edb4) == 650676,
              "a9edb4 x86");
static_assert(offsetof(MMDAppState, physicsResetPending) == 650677,
              "physicsResetPending x86");
static_assert(offsetof(MMDAppState, playbackFrameChanged) == 650678,
              "playbackFrameChanged x86");
static_assert(offsetof(MMDAppState, gravityX) == 650680,
              "gravityX x86");
static_assert(offsetof(MMDAppState, gravityY) == 650684,
              "gravityY x86");
static_assert(offsetof(MMDAppState, gravityZ) == 650688,
              "gravityZ x86");
static_assert(offsetof(MMDAppState, gravityMagnitude) == 650692,
              "gravityMagnitude x86");
static_assert(offsetof(MMDAppState, gravityNoise) == 650696,
              "gravityNoise x86");
static_assert(offsetof(MMDAppState, v9edcc) == 650700,
              "v9edcc x86");
static_assert(offsetof(MMDAppState, v9edd0) == 650704,
              "v9edd0 x86");
static_assert(offsetof(MMDAppState, viewportInputActive) == 650705,
              "viewportInputActive x86");
static_assert(offsetof(MMDAppState, recordingCompletionFlag) == 650708,
              "recordingCompletionFlag x86");
static_assert(offsetof(MMDAppState, v9edd8) == 650712,
              "v9edd8 x86");
static_assert(offsetof(MMDAppState, f9eddc) == 650716,
              "f9eddc x86");
static_assert(offsetof(MMDAppState, toonTextures) == 650720,
              "toonTextures x86");
static_assert(offsetof(MMDAppState, v9ee0cOrUint32) == 650764,
              "v9ee0cOrUint32 x86");
static_assert(offsetof(MMDAppState, spriteOverlayPrimitiveCount) == 650768,
              "spriteOverlayPrimitiveCount x86");
static_assert(offsetof(MMDAppState, overlayTexture) == 650772,
              "overlayTexture x86");
static_assert(offsetof(MMDAppState, overlayVertices) == 650776,
              "overlayVertices x86");
static_assert(offsetof(MMDAppState, textOverlayPrimitiveCount) == 650780,
              "textOverlayPrimitiveCount x86");
static_assert(offsetof(MMDAppState, sceneFontTexture) == 650784,
              "sceneFontTexture x86");
static_assert(offsetof(MMDAppState, recentFile0) == 650788,
              "recentFile0 x86");
static_assert(offsetof(MMDAppState, recentFile1) == 651044,
              "recentFile1 x86");
static_assert(offsetof(MMDAppState, recentFile2) == 651300,
              "recentFile2 x86");
static_assert(offsetof(MMDAppState, lineOverlayPrimitiveCount) == 651556,
              "lineOverlayPrimitiveCount x86");
static_assert(offsetof(MMDAppState, groundPlaneVertices) == 651560,
              "groundPlaneVertices x86");
static_assert(offsetof(MMDAppState, v9f12c) == 651564,
              "v9f12c x86");
static_assert(offsetof(MMDAppState, projectedShadowRestoreTexture) == 651568,
              "projectedShadowRestoreTexture x86");
static_assert(offsetof(MMDAppState, captureReadbackPixels) == 652084,
              "captureReadbackPixels x86");
static_assert(offsetof(MMDAppState, fontSubOrPtr) == 652088,
              "fontSubOrPtr x86");
static_assert(offsetof(MMDAppState, leftViewportVertices) -
                  offsetof(MMDAppState, fontSubOrPtr) == 3536,
              "path-resolution workspace must end at the next app field");
static_assert(offsetof(MMDAppState, leftViewportVertices) == 655624,
              "leftViewportVertices x86");
static_assert(offsetof(MMDAppState, rightViewportVertices) == 655628,
              "rightViewportVertices x86");
static_assert(offsetof(MMDAppState, toonEdgeTable) == 655632,
              "toonEdgeTable x86");
static_assert(offsetof(MMDAppState, selfShadowCfgOrUint32) == 655752,
              "selfShadowCfgOrUint32 x86");
static_assert(offsetof(MMDAppState, selectionBoxDragging) == 655753,
              "selectionBoxDragging x86");
static_assert(offsetof(MMDAppState, a0194) == 655764,
              "a0194 x86");
static_assert(offsetof(MMDAppState, modelOutlineRenderingSuppressed) == 655765,
              "modelOutlineRenderingSuppressed x86");
static_assert(offsetof(MMDAppState, a0196) == 655766,
              "a0196 x86");
static_assert(offsetof(MMDAppState, a0197) == 655767,
              "a0197 x86");
static_assert(offsetof(MMDAppState, modelOutlineColorRed) == 655768,
              "modelOutlineColorRed x86");
static_assert(offsetof(MMDAppState, modelOutlineColorGreen) == 655772,
              "modelOutlineColorGreen x86");
static_assert(offsetof(MMDAppState, modelOutlineColorBlue) == 655776,
              "modelOutlineColorBlue x86");
static_assert(offsetof(MMDAppState, buf655780) == 655780,
              "buf655780 x86");
static_assert(offsetof(MMDAppState, a01E4) == 655844,
              "a01E4 x86");
static_assert(offsetof(MMDAppState, activeRenderObject) == 655976,
              "activeRenderObject x86");
static_assert(offsetof(MMDAppState, activeRenderPass) == 655980,
              "activeRenderPass x86");
static_assert(offsetof(MMDAppState, a0270) == 655984,
              "a0270 x86");
static_assert(offsetof(MMDAppState, fullscreenMode) == 655988,
              "fullscreenMode x86");
static_assert(offsetof(MMDAppState, savedMenu) == 655992,
              "savedMenu x86");
static_assert(offsetof(MMDAppState, a027COrBuf_bytes) == 655996,
              "a027COrBuf_bytes x86");
static_assert(offsetof(MMDAppState, a02A8) == 656040,
              "a02A8 x86");
static_assert(offsetof(MMDAppState, recRTW) == 656044,
              "recRTW x86");
static_assert(offsetof(MMDAppState, recRTH) == 656048,
              "recRTH x86");
static_assert(offsetof(MMDAppState, a02B4) == 656052,
              "a02B4 x86");
static_assert(offsetof(MMDAppState, a02B5) == 656053,
              "a02B5 x86");
static_assert(offsetof(MMDAppState, a02B6) == 656054,
              "a02B6 x86");
static_assert(offsetof(MMDAppState, sjisOut) == 656055,
              "sjisOut x86");
static_assert(offsetof(MMDAppState, a03B7) == 656311,
              "a03B7 x86");
static_assert(offsetof(MMDAppState, depthDeviceEnabled) == 656312,
              "depthDeviceEnabled x86");
static_assert(offsetof(MMDAppState, a03BC) == 656316,
              "a03BC x86");
static_assert(offsetof(MMDAppState, a03C0) == 656320,
              "a03C0 x86");
static_assert(offsetof(MMDAppState, a03C4) == 656324,
              "a03C4 x86");
static_assert(offsetof(MMDAppState, a03C8) == 656328,
              "a03C8 x86");
static_assert(offsetof(MMDAppState, depthTextureCallback) == 656332,
              "depthTextureCallback x86");
static_assert(offsetof(MMDAppState, a03D0) == 656336,
              "a03D0 x86");
static_assert(offsetof(MMDAppState, a03D4) == 656340,
              "a03D4 x86");
static_assert(offsetof(MMDAppState, a03D8) == 656344,
              "a03D8 x86");
static_assert(offsetof(MMDAppState, a03DC) == 656348,
              "a03DC x86");
static_assert(offsetof(MMDAppState, a03DD) == 656349,
              "a03DD x86");
static_assert(offsetof(MMDAppState, depthTextureCompositionEnabled) == 656350,
              "depthTextureCompositionEnabled x86");
static_assert(offsetof(MMDAppState, a03DF) == 656351,
              "a03DF x86");
static_assert(offsetof(MMDAppState, fpsLimitSaved) == 656352,
              "fpsLimitSaved x86");
static_assert(offsetof(MMDAppState, a03E4) == 656356,
              "a03E4 x86");
static_assert(offsetof(MMDAppState, a03E5) == 656357,
              "a03E5 x86");
static_assert(offsetof(MMDAppState, a03E6) == 656358,
              "a03E6 x86");
static_assert(offsetof(MMDAppState, a03E7) == 656359,
              "a03E7 x86");
static_assert(offsetof(MMDAppState, a03E8) == 656360,
              "a03E8 x86");
static_assert(offsetof(MMDAppState, automaticFrameAdvanceEnabled) == 656361,
              "automaticFrameAdvanceEnabled x86");
static_assert(offsetof(MMDAppState, openniVersion) == 656362,
              "openniVersion x86");
static_assert(offsetof(MMDAppState, timelineSelectionChanged) == 656363,
              "timelineSelectionChanged x86");
static_assert(offsetof(MMDAppState, a042C) == 656428,
              "a042C x86");
static_assert(offsetof(MMDAppState, cameraParentModel) == 656432,
              "cameraParentModel x86");
static_assert(offsetof(MMDAppState, cameraParentBone) == 656436,
              "cameraParentBone x86");
static_assert(offsetof(MMDAppState, cameraAttachmentBasis) == 656440,
              "cameraAttachmentBasis x86");
static_assert(offsetof(MMDAppState, cameraAttachmentBasis[15]) == 656500,
              "cameraAttachmentBasis[15] x86");
static_assert(offsetof(MMDAppState, cameraAttachmentTransformSuppressed) == 656504,
              "cameraAttachmentTransformSuppressed x86");
static_assert(offsetof(MMDAppState, logFont) == 656508,
              "logFont x86");
static_assert(offsetof(MMDAppState, a04B8) == 656568,
              "a04B8 x86");
static_assert(offsetof(MMDAppState, hFontUI) == 656572,
              "hFontUI x86");
static_assert(offsetof(MMDAppState, eulerX) == 656576,
              "eulerX x86");
static_assert(offsetof(MMDAppState, eulerY) == 656580,
              "eulerY x86");
static_assert(offsetof(MMDAppState, eulerZ) == 656584,
              "eulerZ x86");
static_assert(offsetof(MMDAppState, brushes) == 656588,
              "brushes x86");
static_assert(offsetof(MMDAppState, buf656632) == 656632,
              "buf656632 x86");
static_assert(offsetof(MMDAppState, b6568480) == 656848,
              "b6568480 x86");
static_assert(offsetof(MMDAppState, b6568481) == 656849,
              "b6568481 x86");
static_assert(offsetof(MMDAppState, b6568482) == 656850,
              "b6568482 x86");
static_assert(offsetof(MMDAppState, b6568483) == 656851,
              "b6568483 x86");
static_assert(offsetof(MMDAppState, uiTextRed) == 656852,
              "uiTextRed x86");
static_assert(offsetof(MMDAppState, uiTextGreen) == 656853,
              "uiTextGreen x86");
static_assert(offsetof(MMDAppState, uiTextBlue) == 656854,
              "uiTextBlue x86");
static_assert(offsetof(MMDAppState, themeColors) == 656856,
              "themeColors x86");
static_assert(offsetof(MMDAppState, themeColors[34]) == 656992,
              "themeColors[34] x86");
static_assert(offsetof(MMDAppState, a0665) == 656997,
              "a0665 x86");
static_assert(offsetof(MMDAppState, a0668OrUint32) == 657000,
              "a0668OrUint32 x86");
static_assert(offsetof(MMDAppState, a066C) == 657004,
              "a066C x86");
static_assert(offsetof(MMDAppState, a066D) == 657005,
              "a066D x86");
static_assert(offsetof(MMDAppState, savedPlaybackPhysicsMode) == 657008,
              "savedPlaybackPhysicsMode x86");
static_assert(offsetof(MMDAppState, a06B4) == 657076,
              "a06B4 x86");
static_assert(offsetof(MMDAppState, a06B5) == 657077,
              "a06B5 x86");
static_assert(offsetof(MMDAppState, a06B6) == 657078,
              "a06B6 x86");
static_assert(offsetof(MMDAppState, hwnd) == 657080,
              "hwnd x86");
static_assert(offsetof(MMDAppState, deltaTime) == 657084,
              "deltaTime x86");
static_assert(offsetof(MMDAppState, sub06c) == 657088,
              "sub06c x86");
static_assert(offsetof(MMDAppState, rendererOrLocaleTable) == 657092,
              "rendererOrLocaleTable x86");
static_assert(offsetof(MMDAppState, sidebarWidth) == 657096,
              "sidebarWidth x86");
static_assert(offsetof(MMDAppState, waveEnabled) == 657100,
              "waveEnabled x86");
static_assert(offsetof(MMDAppState, exeDir) == 657102,
              "exeDir x86");
static_assert(offsetof(MMDAppState, origEditProc) == 657616,
              "origEditProc x86");
static_assert(offsetof(MMDAppState, renderW) == 657620,
              "renderW x86");
static_assert(offsetof(MMDAppState, renderH) == 657624,
              "renderH x86");
static_assert(offsetof(MMDAppState, cameraDistance) == 657628,
              "cameraDistance x86");
static_assert(offsetof(MMDAppState, fpsLimit) == 657632,
              "fpsLimit x86");
static_assert(offsetof(MMDAppState, fpsA) == 657636,
              "fpsA x86");
static_assert(offsetof(MMDAppState, fpsB) == 657640,
              "fpsB x86");
static_assert(offsetof(MMDAppState, fpsC) == 657644,
              "fpsC x86");
static_assert(offsetof(MMDAppState, envFileName) == 657664,
              "envFileName x86");
static_assert(offsetof(MMDAppState, aviRecordStartFrame) == 658176,
              "aviRecordStartFrame x86");
static_assert(offsetof(MMDAppState, aviRecordEndFrame) == 658180,
              "aviRecordEndFrame x86");
static_assert(offsetof(MMDAppState, aviRecordFps) == 658184,
              "aviRecordFps x86");
static_assert(offsetof(MMDAppState, aviIncludeWave) == 658188,
              "aviIncludeWave x86");
static_assert(offsetof(MMDAppState, sceneModified) == 658189,
              "sceneModified x86");
static_assert(offsetof(MMDAppState, a0B10) == 658192,
              "a0B10 x86");
static_assert(offsetof(MMDAppState, a0B14OrPtr) == 658196,
              "a0B14OrPtr x86");
static_assert(offsetof(MMDAppState, accessoryRenderSplitOrder) == 658208,
              "accessoryRenderSplitOrder x86");
static_assert(offsetof(MMDAppState, a0B44OrInt32) == 658244,
              "a0B44OrInt32 x86");
#ifndef _M_X64
static_assert(offsetof(MMDAppState, frameRangeDialog) == 658256,
              "frameRangeDialog x86");
#endif
static_assert(offsetof(MMDAppState, englishUI) == 658252,
              "englishUI x86");
static_assert(offsetof(MMDAppState, frameRangeDialog) == 658256,
              "frameRangeDialog x86");
static_assert(offsetof(MMDAppState, a0B64) == 658276,
              "a0B64 x86");
static_assert(offsetof(MMDAppState, timeNowLow) == 658280,
              "timeNowLow x86");
static_assert(offsetof(MMDAppState, timeNowHigh) == 658284,
              "timeNowHigh x86");
static_assert(offsetof(MMDAppState, milliToSec) == 658288,
              "milliToSec x86");
static_assert(offsetof(MMDAppState, a0b74OrInt32) == 658292,
              "a0b74OrInt32 x86");
static_assert(offsetof(MMDAppState, a0B7COrInt32) == 658300,
              "a0B7COrInt32 x86");
static_assert(offsetof(MMDAppState, fa0b00D) == 658432,
              "fa0b00D x86");
static_assert(offsetof(MMDAppState, fa0b04D) == 658436,
              "fa0b04D x86");
static_assert(offsetof(MMDAppState, selAcc) == 658476,
              "selAcc x86");
static_assert(offsetof(MMDAppState, records8c) == 658480,
              "records8c x86");
static_assert(offsetof(MMDAppState, sel8c) == 658624,
              "sel8c x86");
static_assert(offsetof(MMDAppState, playbackPhysicsMode) == 658628,
              "playbackPhysicsMode x86");
static_assert(offsetof(MMDAppState, a0CC8OrUint32) == 658632,
              "a0CC8OrUint32 x86");
static_assert(offsetof(MMDAppState, a0CD4) == 658644,
              "a0CD4 x86");
#ifndef _M_X64
static_assert(offsetof(MMDAppState, accessoryFrameDialog) == 658636,
              "accessoryFrameDialog x86");
#endif
#ifndef _M_X64
static_assert(offsetof(MMDAppState, aviCodecSelection) == 658648,
              "aviCodecSelection x86");
#endif
static_assert(offsetof(MMDAppState, origTrackProc) == 658652,
              "origTrackProc x86");
static_assert(offsetof(MMDAppState, aviSettings) == 658656,
              "aviSettings x86");
static_assert(offsetof(MMDAppState, projectedShadowDiffuseAlpha) == 658668,
              "projectedShadowDiffuseAlpha x86");
static_assert(offsetof(MMDAppState, projectedShadowAmbientIntensity) == 658672,
              "projectedShadowAmbientIntensity x86");
static_assert(offsetof(MMDAppState, projectedShadowAmbientG) == 658676,
              "projectedShadowAmbientG x86");
static_assert(offsetof(MMDAppState, projectedShadowAmbientB) == 658680,
              "projectedShadowAmbientB x86");
static_assert(offsetof(MMDAppState, projectedShadowAmbientA) == 658684,
              "projectedShadowAmbientA x86");
static_assert(offsetof(MMDAppState, projectedShadowSpecularAlpha) == 658700,
              "projectedShadowSpecularAlpha x86");
static_assert(offsetof(MMDAppState, recordingWindow) == 658724,
              "recordingWindow x86");
static_assert(offsetof(MMDAppState, selfShadowCompositionEnabled) == 658728,
              "selfShadowCompositionEnabled x86");
static_assert(offsetof(MMDAppState, physicsInterval) == 658732,
              "physicsInterval x86");
static_assert(offsetof(MMDAppState, selfShadowMode) == 658736,
              "selfShadowMode x86");
static_assert(offsetof(MMDAppState, floatingWindow) == 658744,
              "floatingWindow x86");
static_assert(offsetof(MMDAppState, separateWindowSidebarWidth) == 658748,
              "separateWindowSidebarWidth x86");
static_assert(offsetof(MMDAppState, hideRight) == 658752,
              "hideRight x86");
static_assert(offsetof(MMDAppState, hideTop) == 658756,
              "hideTop x86");
static_assert(offsetof(MMDAppState, hideLeft) == 658760,
              "hideLeft x86");
static_assert(offsetof(MMDAppState, hideBottom) == 658764,
              "hideBottom x86");
static_assert(offsetof(MMDAppState, separateWindowX) == 658768,
              "separateWindowX x86");
static_assert(offsetof(MMDAppState, separateWindowY) == 658772,
              "separateWindowY x86");
static_assert(offsetof(MMDAppState, separateWindowWidth) == 658776,
              "separateWindowWidth x86");
static_assert(offsetof(MMDAppState, separateWindowHeight) == 658780,
              "separateWindowHeight x86");
static_assert(offsetof(MMDAppState, separateWindowMaximized) == 658784,
              "separateWindowMaximized x86");
static_assert(offsetof(MMDAppState, aviStereoOutput) == 658785,
              "aviStereoOutput x86");
static_assert(offsetof(MMDAppState, aviStereoWidthMultiplier) == 658788,
              "aviStereoWidthMultiplier x86");
static_assert(offsetof(MMDAppState, autoRepeat) == 658792,
              "autoRepeat x86");
static_assert(offsetof(MMDAppState, messageSeen) == 658796,
              "messageSeen x86");
static_assert(offsetof(MMDAppState, dirModel) == 658800,
              "dirModel x86");
static_assert(offsetof(MMDAppState, dirUser) == 660800,
              "dirUser x86");
static_assert(offsetof(MMDAppState, dirAccs) == 662800,
              "dirAccs x86");
static_assert(offsetof(MMDAppState, dirMotion) == 664800,
              "dirMotion x86");
static_assert(offsetof(MMDAppState, dirPose) == 666800,
              "dirPose x86");
static_assert(offsetof(MMDAppState, dirWave) == 668800,
              "dirWave x86");
static_assert(offsetof(MMDAppState, dirBg) == 670800,
              "dirBg x86");
static_assert(offsetof(MMDAppState, flag672800) == 672800,
              "flag672800 x86");
static_assert(offsetof(MMDAppState, val672804) == 672804,
              "val672804 x86");
static_assert(offsetof(MMDAppState, sidebarRatio) == 672808,
              "sidebarRatio x86");
static_assert(offsetof(MMDAppState, windowLayoutReady) == 672812,
              "windowLayoutReady x86");
static_assert(offsetof(MMDAppState, statusText) == 672813,
              "statusText x86");
#else
// anchors only - instruction-level x64 ground truth (vote-grade
static_assert(offsetof(MMDAppState, sub025c) == 208,
              "sub025c x64");
static_assert(offsetof(MMDAppState, hdcMainPanel) == 736,
              "hdcMainPanel x64");
static_assert(offsetof(MMDAppState, bmpRes101) == 792,
              "bmpRes101 x64");
static_assert(offsetof(MMDAppState, displayClipboard) == 936,
              "displayClipboard x64");
static_assert(offsetof(MMDAppState, cameraKeyTrack) == 976,
              "cameraKeyTrack x64");
static_assert(offsetof(MMDAppState, lightKeyTrack) == 984,
              "lightKeyTrack x64");
static_assert(offsetof(MMDAppState, selfShadowKeyTrack) == 992,
              "selfShadowKeyTrack x64");
static_assert(offsetof(MMDAppState, gravityKeyTrack) == 1000,
              "gravityKeyTrack x64");
static_assert(offsetof(MMDAppState, accKeyTracks) == 1008,
              "accKeyTracks x64");
static_assert(offsetof(MMDAppState, modelSlots) == 3048,
              "modelSlots x64");
static_assert(offsetof(MMDAppState, slotIdx) == 5088,
              "slotIdx x64");
static_assert(offsetof(MMDAppState, rowHitBone) == 5204,
              "rowHitBone x64");
static_assert(offsetof(MMDAppState, rowHitMorph) == 165204,
              "rowHitMorph x64");
static_assert(offsetof(MMDAppState, rowHitIk) == 325204,
              "rowHitIk x64");
static_assert(offsetof(MMDAppState, rowHitBand0) == 485204,
              "rowHitBand0 x64");
static_assert(offsetof(MMDAppState, rowHitBand1) == 486004,
              "rowHitBand1 x64");
static_assert(offsetof(MMDAppState, rowHitBand2) == 486804,
              "rowHitBand2 x64");
static_assert(offsetof(MMDAppState, rowHitBand3) == 487604,
              "rowHitBand3 x64");
static_assert(offsetof(MMDAppState, rowHitAcc) == 488404,
              "rowHitAcc x64");
static_assert(offsetof(MMDAppState, jointLineMap) == 648480,
              "jointLineMap x64");
static_assert(offsetof(MMDAppState, buf9ddx) == 649280,
              "buf9ddx x64");
static_assert(offsetof(MMDAppState, lastRegisteredFrame) == 651320,
              "lastRegisteredFrame x64");
#endif

#ifdef _M_X64
// ---------------------------------------------------------------------------
// xlate-sync pins: while raw<T>(x86_offset) callers remain, each promoted
// member must sit exactly where the offsets_xlate.hpp map sends its offset.
// A failure here means the struct layout drifted from the map - fix the
// surrounding RawPad sizes, never the semantics.
// ---------------------------------------------------------------------------
static_assert(xlate::Offset(200) == offsetof(MMDAppState, sidebarResizeDragging), "xlate sync 200");
static_assert(xlate::Offset(836) == offsetof(MMDAppState, viewportToolHovered), "xlate sync 836");
static_assert(xlate::Offset(840) == offsetof(MMDAppState, viewToolDragOperation), "xlate sync 840");
static_assert(xlate::Offset(844) == offsetof(MMDAppState, interactionDragMode), "xlate sync 844");
static_assert(xlate::Offset(860) == offsetof(MMDAppState, displayClipboard), "xlate sync 860");
static_assert(xlate::Offset(768) == offsetof(MMDAppState, groundGridVertices), "xlate sync 768");
static_assert(xlate::Offset(772) == offsetof(MMDAppState, groundGridIndices), "xlate sync 772");
static_assert(xlate::Offset(2328) == offsetof(MMDAppState, groundShadowEnabled), "xlate sync 2328");
static_assert(xlate::Offset(2332) == offsetof(MMDAppState, aviBackgroundEnabled), "xlate sync 2332");
static_assert(xlate::Offset(645642) == offsetof(MMDAppState, lightA), "xlate sync 645642");
static_assert(xlate::Offset(645648) == offsetof(MMDAppState, lightB), "xlate sync 645648");
static_assert(xlate::Offset(645654) == offsetof(MMDAppState, lightC), "xlate sync 645654");
static_assert(xlate::Offset(645660) == offsetof(MMDAppState, lightD), "xlate sync 645660");
static_assert(xlate::Offset(645668) == offsetof(MMDAppState, v9da24), "xlate sync 645668");
static_assert(xlate::Offset(650650) == offsetof(MMDAppState, projectedShadowBlendEnabled), "xlate sync 650650");
static_assert(xlate::Offset(650696) == offsetof(MMDAppState, gravityNoise), "xlate sync 650696");
static_assert(xlate::Offset(650776) == offsetof(MMDAppState, overlayVertices), "xlate sync 650776");
static_assert(xlate::Offset(650780) == offsetof(MMDAppState, textOverlayPrimitiveCount), "xlate sync 650780");
static_assert(xlate::Offset(650784) == offsetof(MMDAppState, sceneFontTexture), "xlate sync 650784");
static_assert(xlate::Offset(651556) == offsetof(MMDAppState, lineOverlayPrimitiveCount), "xlate sync 651556");
static_assert(xlate::Offset(651560) == offsetof(MMDAppState, groundPlaneVertices), "xlate sync 651560");
static_assert(xlate::Offset(651568) == offsetof(MMDAppState, projectedShadowRestoreTexture), "xlate sync 651568");
static_assert(xlate::Offset(655753) == offsetof(MMDAppState, selectionBoxDragging), "xlate sync 655753");
static_assert(xlate::Offset(655765) == offsetof(MMDAppState, modelOutlineRenderingSuppressed), "xlate sync 655765");
static_assert(xlate::Offset(655768) == offsetof(MMDAppState, modelOutlineColorRed), "xlate sync 655768");
static_assert(xlate::Offset(655772) == offsetof(MMDAppState, modelOutlineColorGreen), "xlate sync 655772");
static_assert(xlate::Offset(655776) == offsetof(MMDAppState, modelOutlineColorBlue), "xlate sync 655776");
static_assert(xlate::Offset(655976) == offsetof(MMDAppState, activeRenderObject), "xlate sync 655976");
static_assert(xlate::Offset(655980) == offsetof(MMDAppState, activeRenderPass), "xlate sync 655980");
static_assert(xlate::Offset(656312) == offsetof(MMDAppState, depthDeviceEnabled), "xlate sync 656312");
static_assert(xlate::Offset(656350) == offsetof(MMDAppState, depthTextureCompositionEnabled), "xlate sync 656350");
static_assert(xlate::Offset(656440) == offsetof(MMDAppState, cameraAttachmentBasis), "xlate sync 656440");
static_assert(xlate::Offset(658728) == offsetof(MMDAppState, selfShadowCompositionEnabled), "xlate sync 658728");
static_assert(xlate::Offset(658796) == offsetof(MMDAppState, messageSeen), "xlate sync 658796");
#endif

}  // namespace mikudancestudio
