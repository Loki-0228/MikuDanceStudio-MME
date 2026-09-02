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
//     to the last anchor.
//
// The historical offset-keyed access layer and its xlate table are
// gone; fields that the x64 blob could not host live in MMDApp mirrors
// with arch-split accessors.  Buffer widths are arch-invariant.  Unknown
// regions are byte arrays - never guessed.
// ===========================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>


namespace mikudancestudio {

constexpr std::size_t kAppObjectSize =
    sizeof(void*) == 8 ? 0xA55E0 : 0xA4530;

template <std::size_t N>
struct RawPad { unsigned char b[N]; };
struct EmptyPad {};

struct MMDAppState {
#if defined(_M_X64)
    RawPad<4> pad0inst;  // x86 keeps the instance handle at +0; on x64 it
                         // lives in the MMDApp mirror m_hInstance instead
#else
    void* hInstance;
#endif
    std::int32_t mouseX;               // +0x04
    std::int32_t mouseY;               // +0x08
    std::int32_t previousMouseX;       // +0x0C
    std::int32_t previousMouseY;       // +0x10
    std::int32_t upKeyState;           // +0x14 (VK_UP)
    std::int32_t downKeyState;         // +0x18 (VK_DOWN)
    std::int32_t leftKeyState;         // +0x1C (VK_LEFT)
    std::int32_t rightKeyState;        // +0x20 (VK_RIGHT)
    std::int32_t shiftModifierState;   // +0x24 (VK_SHIFT)
    std::int32_t spaceKeyState;        // +0x28 (VK_SPACE)
    std::int32_t escKeyState;          // +0x2C (VK_ESCAPE)
    // 0x30..0x78: dialog re-entry guards - nonzero while the matching
    // dialog is up; the menu command switch reads them to reject
    // re-entry.  The same ints double as letter key-down counters for
    // PollKey ('z x c v a b s g h i k' fill 0x30..0x60, 'p u j f r l'
    // fill 0x60..0x78); modal dialog pumps never run PollKey, so the
    // two roles never collide.
    std::int32_t dialogFlags[18];
    std::int32_t keyState221;               // +0x78 (GetKeyState(221))
    std::int32_t keyState226;               // +0x7C (GetKeyState(226))
    std::int32_t tabKeyState;               // +0x80 (VK_TAB)
    std::int32_t leftMouseButtonState;      // +0x84
    std::int32_t rightMouseButtonState;     // +0x88
    std::int32_t middleMouseButtonState;    // +0x8C
    std::int32_t numpadKeyState[10];        // +0x90..0xB8 (VK_NUMPAD0..9)
    std::int32_t deleteKeyState;            // +0xB8 (VK_DELETE)
    std::int32_t bC;                        // +0xBC (VK_RETURN state)
    std::int32_t ctrlModifierState;         // +0xC0 (VK_CONTROL)
    std::int32_t menuKeyState;              // +0xC4 (VK_MENU)
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
    // Clipboard pointer family.  x86 carries all eight slots in the blob
    // (4-byte pointers); the x64 blob only reserved displayClipboard, so
    // x64 keeps the seven siblings in MMDApp mirrors (raw accessors used
    // to hit unmapped identity offsets there - x64 latent corruption).
    void* v350Clipboard;    // +0x350 bone-copy records (aka v350)
#if defined(_M_X64)
    RawPad<8> pad46;
#else
    void* boneClipboard;    // +0x354
    void* morphClipboard;   // +0x358
#endif
    void* displayClipboard;  // x64 pin 936
#if defined(_M_X64)
    RawPad<32> pad47;
#else
    void* cameraClipboard;      // +0x360
    void* lightClipboard;       // +0x364
    void* shadowClipboard;      // +0x368
    void* gravityClipboard;     // +0x36C
    void* accessoryClipboard;   // +0x370
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
    std::int32_t viewportToolCenterX;   // +0x920
    std::int32_t viewportToolCenterY;   // +0x924
    float selectedClipW;                // +0x928 (selected bone's clip-space
                                        //  depth; scales mouse drags)
    std::uint32_t viewportToolOperation;
    std::int32_t dragOriginX;
    std::int32_t dragOriginY;
    std::int32_t boneBoxStartX;         // +0x938
    std::int32_t boneBoxStartY;         // +0x93C
    std::int32_t boneBoxSelectionActive;
    std::int32_t scrollCbSize;
    std::int32_t scrollFMask;
    std::int32_t scrollNMin;
    std::int32_t scrollNMax;
    std::int32_t scrollNPage;
    std::int32_t scrollNPos;
    RawPad<20> pad67;
    std::int32_t timelineScrollNPage;  // +0x970 timeline scrollbar cached nPage
    std::int32_t timelineScrollNMin;   // +0x974 timeline scrollbar cached nMin
    RawPad<4> pad67b;
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
    // Full 200x200 accessory hit grid (0x76904).  Promoted from [800] +
    // RawPad<156800>: byte-identical on both architectures (800*4 + 156800
    // == 40000*4), and member indexing lands elements >= 800 in the
    // reserved tail on x64 too, where the old offset path lost the xlate.
    std::int32_t rowHitAcc[40000];  // x64 pin 488404
    // 0x9DA04..0x9DA0A: interp-curve editor panel state (uniform-curve
    // found flag + the cached control-point pair, y as 127-complement)
    unsigned char interpCurveUniformFound;     // +0x9DA04
    unsigned char interpCurveControlCache[4];  // +0x9DA05..08
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
    unsigned char pictureBackgroundEnabled;
#if defined(_M_X64)
    RawPad<7> pad140;
#else
    RawPad<3> pad140;
#endif
    void* pictureBackgroundTexture;
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
    // Track-key cursors (0x9E65C..): the camera/light/self-shadow/gravity
    // play-cursor dwords with their active bytes, then the 55 accessory
    // track cursors.  In-blob on both arches; only the accessory active
    // byte array (0x9EA78) needs an x64 MMDApp mirror - the x64 region
    // here ends 42 bytes short of it.
    std::uint32_t cameraTrackCursor;      // +0x9E65C
    std::uint8_t cameraTrackActive;       // +0x9E660
    RawPad<3> pad153a;
    std::uint32_t lightTrackCursor;       // +0x9E664
    std::uint8_t lightTrackActive;        // +0x9E668
    RawPad<3> pad153b;
    std::uint32_t shadowTrackCursor;      // +0x9E66C
    std::uint8_t shadowTrackActive;       // +0x9E670
    RawPad<3> pad153c;
    std::uint32_t gravityTrackCursor;     // +0x9E674
    std::uint8_t gravityTrackActive;      // +0x9E678
    RawPad<3> pad153d;
    std::uint32_t accessoryTrackCursor[55];  // +0x9E67C..0x9E938
#if defined(_M_X64)
    RawPad<813> pad153;
#else
    RawPad<800> pad153e;                     // 0x9E938..0x9EA78
    std::uint8_t accessoryTrackActive[55];   // +0x9EA78
    RawPad<200> pad153;                      // ..0x9ED97
#endif
    // IsWindowEnabled snapshots of the 7 main-window playback controls
    // (0x1F1/0x1F2/0x1AF/0x1A5/0x1A6/0x190/0x191), restored after playback
    unsigned char playbackEnabledSnapshot[7];
    unsigned char v9eb7e;
    unsigned char blinkPhase;
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
    unsigned char v9ed98;
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
    // x64: the render-save path buffer lives outside the blob
    // (MMDApp::m_captureSavePath)
    RawPad<296> pad197;
#else
    // render/screenshot save path chosen by the render dialog (menu 276)
    // and consumed by the capture save ladder
    wchar_t captureSavePath[256];
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
    RawPad<2> pad204;
    std::int32_t selectionBoxAnchorX;   // +0xA018C
    std::int32_t selectionBoxAnchorY;   // +0xA0190
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
    unsigned char buf655780[48];
    unsigned char wireframeRenderingEnabled;  // +0xA01D4 wireframe fill mode
    RawPad<15> pad209;
    unsigned char a01E4;
#if defined(_M_X64)
    RawPad<99> pad213;  // the two projection matrices live in MMDApp
                        // mirrors here - the x64 blob region ends 32
                        // bytes short of the second matrix
#else
    RawPad<3> pad213a;
    // 0xA01E8/0xA0228: the fixed-function light and world view-projection
    // matrices (column-major, 16 floats each).  Kept as float[16] so this
    // header stays free of d3d9 includes; MMDApp reinterprets to D3DMATRIX.
    float lightViewProjectionMatrix[16];   // +0xA01E8
    float worldViewProjectionMatrix[16];   // +0xA0228
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
#if defined(_M_X64)
    std::uint32_t a027COrBuf_bytes;  // blob slot unused; the placement lives
                                     // in the MMDApp mirror m_savedPlacement
    RawPad<22> pad219;
#else
    // 0xA027C: the window placement saved across fullscreen recording
    // (GetWindowPlacement/SetWindowPlacement; .length is init 44)
    WINDOWPLACEMENT savedPlacement;
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
#if defined(_M_X64)
    std::uint32_t a03D4;  // blob slot unused; x64 uses the MMDApp mirror
                          // m_openniTrackingCallback (needs 8 bytes)
#else
    void* openniTrackingCallback;  // ?OpenNIIsTracking@@YGXPA_N@Z (registered
                                   // by the OpenNI plugin; read by the frame
                                   // driver's selection-callback branch)
#endif
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
    RawPad<8> pad249;  // the 64-byte selection block lives in a mirror
#else
    // 0xA03EC..0xA042B: eight stride-8 slots as int32[16] (no alignment
    // padding); the count sits at element 2*band, the +4 halves held the
    // record pointers and are mirrored by g_timelineSelectionRecords.
    // The canary watches all 64 bytes.
    std::int32_t timelineSelectionSlots[16];
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
    // Panel-row highlight flags (0xA04F8): camera/light/shadow/gravity
    // header flags at [0..3], tree-row flags over [0..199], joint-row
    // flags at [4..203] (the last four overlap the timeline-range anchors
    // below, as in the original).  x64 reserves only 175 bytes here before
    // b6568480, so x64 keeps 64 + pad and routes PanelRowFlags() to a
    // mirror; x86 carries the full region as one array.
#if defined(_M_X64)
    unsigned char buf656632[64];
    RawPad<111> pad277;  // timeline range lives in MMDApp mirrors here
#else
    unsigned char buf656632[200];
    // 0xA05C0..0xA05D0: the frame-range selection anchors (menu 262's
    // first/last offset+base pairs)
    std::int32_t timelineRangeFirstOffset;   // +0xA05C0
    std::int32_t timelineRangeFirstBase;     // +0xA05C4
    std::int32_t timelineRangeLastOffset;    // +0xA05C8
    std::int32_t timelineRangeLastBase;      // +0xA05CC
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
    // accessory-edit dialog close gate: the post-close refresh runs
    // only while nonzero (x64: mirror member MMDApp::m_accessoryApplyGate)
    unsigned char accessoryApplyGate;
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
    // +0xA0674 view-rotation matrix (D3DMATRIX).  The x64 blob reserves
    // only 40 bytes here, so x64 keeps an MMDApp mirror instead.
    float viewRotationTransform[16];
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
    // model-center offset (menu 219, "model-offset" dialog): applied to
    // every root-bone keyframe position on OK
    float modelOffsetX;
    float modelOffsetY;
    float modelOffsetZ;
#ifndef _M_X64
    std::int32_t frameRangeStartFrame;  // +0xA08F0 frame-range dialog start
    std::int32_t morphFrameShift;  // menu 225 morph-frame cleanup shift
    std::int32_t blinkStartFrame;  // menu 227 blink register range start
    std::int32_t blinkEndFrame;    // menu 227 blink register range end
#endif
    wchar_t envFileName[256];
    std::int32_t aviRecordStartFrame;
    std::int32_t aviRecordEndFrame;
    float aviRecordFps;
    unsigned char aviIncludeWave;
    unsigned char sceneModified;
    RawPad<2> pad348;
    std::uint32_t a0B10;
#if defined(_M_X64)
    // x64: the ground-shadow-color dialog handle lives outside the blob
    // (MMDApp::m_groundShadowColorDialog)
    std::int32_t a0B14OrInt32;
#else
    HWND groundShadowColorDialog;  // menu 248, modeless
#endif
#ifndef _M_X64
    WNDPROC groundShadowColorEditProc;  // saved wndproc of its value edit
    void* accessoryOrderArray;          // menu 249 reorder scratch array
#endif
    std::int32_t accessoryRenderSplitOrder;
#ifndef _M_X64
    // accessory-edit dialog scratch buffer 2 (menu 442; freed on close;
    // x64: mirror member MMDApp::m_accessoryEditArray)
    void* accessoryEditArray;
    // rotation-dialog working values, shared by the dialog procs
    // sub_40FBC0 / sub_40F860 (cases 300/302)
    float rotationDialogTemp[7];
#endif
    HWND modelInfoDialog;  // menu 253, modal
#ifndef _M_X64
    WNDPROC modelInfoEditProc;  // saved wndproc of its edit 646
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
    WNDPROC modelEdgeEditProc;      // saved wndproc of edit 667
    std::int32_t modelEdgeComboCursor[3];  // combos 669/673/677 edit pos
#endif
    unsigned char a0B64;
    RawPad<3> pad355;
    std::uint32_t timeNowLow;
    std::uint32_t timeNowHigh;
    float milliToSec;
    HWND frameCopyDialog;  // menu 262, modal
#ifndef _M_X64
    WNDPROC frameCopyEditProc;  // saved wndproc of edit 705
#endif
    void* cameraRecordArray;  // 172-byte records, index selAcc
#if defined(_M_X64)
    // x64: the 172-byte camera-frame staging buffer lives outside the
    // blob (MMDApp::m_cameraFrameScratch)
    RawPad<100> pad360;
#else
    // dialog 262 stages a 172-byte camera record here for editing; the
    // ground-shadow-color RGBA overlays bytes +112..+127 (menus 248/262
    // are never open together, mirroring the original blob reuse)
    unsigned char cameraFrameScratch[172];
#endif
    std::int32_t selAcc;
    void* boneRecordArray;  // 140-byte records, index sel8c
#if defined(_M_X64)
    // x64: the 140-byte bone-frame staging buffer lives outside the
    // blob (MMDApp::m_boneFrameScratch)
    RawPad<124> pad364;
#else
    // dialog 262 stages a 140-byte bone record here for editing; the
    // accessory-frame dialog reuses byte +28 as its saved wndproc
    unsigned char boneFrameScratch[140];
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
    WNDPROC accessoryFrameEditProc;  // saved wndproc of edit 709
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


// The offset pin wall lives in layout_pins.hpp (layout regression guard,
// decoupled from the struct definition).  Still inside this namespace.
#include "mikudancestudio/layout_pins.hpp"

}  // namespace mikudancestudio
