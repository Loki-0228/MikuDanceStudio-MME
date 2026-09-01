// ===========================================================================
// VA 0x0040A730 - MMDApp::InitDefaults  (original: sub_40A730)
// ===========================================================================
// Field-by-field port of the post-construction default initializer called
// from WinMain right after the object is zeroed.  Write order follows the
// decompilation exactly (decimal offsets kept in comments for auditing;
// constants come from scripts/gen_offsets.py - machine-computed hex).
// ===========================================================================
#include <cstdio>
#include <cstring>

#include "mikudancestudio/globals.hpp"
#include "mikudancestudio/mmd_app.hpp"

namespace mikudancestudio {

void MMDApp::InitDefaults() {
    namespace off = offsets;
    auto& s = *this;

    s.ViewOffsetX() = 0.0f;
    s.CameraDistance() = -45.0f;
    s.ViewOffsetY() = 0.0f;
    s.raw<unsigned char>(off::kByteC8) = 0;               // 200
    s.CameraRotation()[0] = 0.0f;
    s.WaveEnabled() = 0;                                  // 657100
    s.CameraRotation()[1] = 0.0f;
    s.DirectSoundAvailable() = 1;                         // 720
    s.CameraRotation()[2] = 0.0f;
    s.UiOptionFlag(0) = 1;                                // 760
    s.CameraPosition()[0] = 0.0f;
    s.UiOptionFlag(1) = 1;                                // 761
    s.UiOptionFlag(2) = 1;                                // 762
    s.CameraPosition()[1] = 10.0f;
    s.UiOptionFlag(3) = 1;                                // 763
    s.UiOptionFlag(4) = 1;                                // 764
    s.CameraPosition()[2] = 0.0f;
    s.UiOptionFlag(5) = 1;                                // 765
    s.UiOptionFlag(6) = 1;                                // 766
    s.CameraPerspective() = 0;
    s.GroundGridEnabled() = 1;                             // 797
    s.ViewportInputActive() = 1;                           // 650705
    s.FpsOverlayEnabled() = 0;                             // 798
    s.raw<std::uint32_t>(off::kDword32C) = 0;             // 812
    s.PlaybackActive() = 0;                                // 816
    s.CameraReferenceMode() = CameraAttachmentReference::None;
    s.PlaybackLoopEnabled() = 0;                           // 833
    s.raw<unsigned char>(off::kByte342) = 0;              // 834
    s.raw<std::uint32_t>(off::kDword344) = 0;             // 836
    s.raw<std::uint32_t>(off::kDword348) = 0;             // 840
    s.raw<std::uint32_t>(off::kDword34C) = 0;             // 844
    s.ClearModelSlots();
    s.EditMode() = ViewportEditMode::None;                 // 2324
    s.raw<unsigned char>(off::kByte918) = 1;              // 2328
    s.raw<std::uint32_t>(off::kDword91C) = 0;             // 2332
    s.CaptureMode() = ScreenCaptureMode::Disabled;        // 650116
    s.ViewportToolOperation() = ViewportToolAction::None; // 2348
    s.BoneBoxSelectionActive() = 0;                       // 2368
    s.TimelineStartFrame() = 0;                           // 2428
    s.CurrentFrame() = 0;                                  // 2432
    s.PendingTimelineSelectionRow() = TimelineSelectionRow::None;

    // 6-iteration light-default loop:
    //   for i in [0,6): [645642+i]=0xC0 [645648+i]=0 [645654+i]=0x40 [645660+i]=0x7F
    for (int i = 0; i < 6; ++i) {
        s.raw<unsigned char>(off::kByteLightA + i) = static_cast<unsigned char>(-64);
        s.raw<unsigned char>(off::kByteLightB + i) = 0;
        s.raw<unsigned char>(off::kByteLightC + i) = 64;
        s.raw<unsigned char>(off::kByteLightD + i) = 127;
    }

    s.raw<std::uint32_t>(off::kDword350) = 0;             // 848
    for (std::size_t d = 645668; d <= 645700; d += 4)      // 645668..645700 (9)
        s.raw<std::uint32_t>(d) = 0;
    s.CameraKeys() = nullptr;
    s.LightKeys() = nullptr;
    s.ShadowKeys() = nullptr;
    s.GravityKeys() = nullptr;
    for (int i = 0; i < 255; ++i) {
        s.AccessoryKeys(i) = nullptr;
        s.AccessorySlot(i) = nullptr;
    }
    s.raw<float>(off::kFloat9E650) = 0.0f;                // 648784
    s.FpsLimit() = 60.0f;                                 // 657632
    s.raw<float>(off::kFloatFpsa) = 0.0f;                 // 657636
    s.LastRegisteredFrame() = 0;                           // 647532
    s.raw<float>(off::kFloatFpsb) = 0.0f;                 // 657640
    s.AviBackgroundTexture() = nullptr;
    s.raw<float>(off::kFloatFpsc) = 0.0f;                 // 657644
    s.AviBackgroundSurface() = nullptr;                   // 648180
    s.PictureBackgroundTexture() = nullptr;
    s.AviOverlayVertices() = nullptr;
    s.AviFile() = nullptr;
    s.AviStream() = nullptr;
    s.AviFrameReader() = nullptr;
    s.AviUsesThirtyFpsTiming() = 0;                       // 648208
    s.PictureBackgroundEnabled() = 0;                     // 648232
    s.raw<unsigned char>(off::kByte9EB7E) = 0;            // 650110
    s.raw<unsigned char>(off::kByte9EB7F) = 0;            // 650111
    s.DisplayObjectListMatchCount() = 0;
    s.DisplayObjectListScrollPosition() = 0;
    s.CaptureTexture() = nullptr;
    s.raw<std::uint32_t>(off::kDword300) = 0;             // 768
    s.CaptureRenderTarget() = nullptr;
    s.FrameStepPlayback() = 0;                             // 650640
    s.SceneModified() = 0;
    s.raw<unsigned char>(off::kByte9ED98) = 0;            // 650648
    s.PlaybackStartsAtCurrentFrame() = 0;
    s.raw<std::uint32_t>(off::kDwordA0B10) = 0;           // 658192
    s.raw<std::uint32_t>(off::kDwordA0B14) = 0;           // 658196
    s.raw<std::uint32_t>(off::kDwordA0B44) = 0;           // 658244
    s.raw<unsigned char>(off::kByte9ED9A) = 1;            // 650650
    s.raw<std::uint32_t>(off::kDword9ED9C) = 0;           // 650652
    s.raw<unsigned char>(off::kByteEnglish) = 1;          // 658252
    s.raw<std::uint32_t>(off::kDwordA0B50) = 0;           // 658256
    s.EnhancedModelDirty() = 0;                            // 658276
    s.raw<std::uint32_t>(off::kDwordA0B74) = 0;           // 658292
    s.raw<std::uint32_t>(off::kDwordA0B7C) = 0;           // 658300
    // Frame-config trio 0xA0B00/0xA0B04/0xA0B08 (start frame / end frame /
    // fps).  Original InitDefaults leaves these alone - the ONLY writer in
    // the whole binary is the frame-range dialog OK handler (0x40F3C4:
    // atol/atol/atof of edits 609/610/611, whose fps edit the dialog init
    // fills with "30").  A fresh app therefore runs with 0/0/0.0f: the
    // frame-step target formula is gated behind 0x9ED90, which only that
    // dialog arms, and the fps-caption display divides by zero exactly
    // like the original's __ftol2_sse (-> INT_MIN) into the still-null
    // HWND 0xA0D24 (a no-op SetWindowTextW).  The phase-15 30.0f seed
    // was reclaimed in phase 19.
    s.PlaybackPhysicsMode() = 2;
    s.raw<unsigned char>(off::kByteA0CC8) = 0;            // 658632
    s.PhysicsResetPending() = 0;
    s.PlaybackFrameChanged() = 0;
    s.raw<unsigned char>(off::kByteA0CD4) = 0;            // 658644
    s.raw<std::uint32_t>(off::kDword9EDC8) = 10;          // 650696
    s.raw<std::uint32_t>(off::kDwordA0B20) = 1;           // 658208
    s.GravityMagnitude() = 9.8000002f;                    // 650692
    s.raw<float>(off::kFloat9EDCC) = 0.0f;                // 650700
    s.GravityX() = 0.0f;                                  // 650680
    s.GravityY() = -1.0f;                                 // 650684
    s.GravityZ() = 0.0f;                                  // 650688
    s.raw<unsigned char>(off::kByte9EDD0) = 0;            // 650704
    s.RecordingCompletionFlag() = nullptr;                // 650708
    s.raw<unsigned char>(off::kByte9EDD8) = 0;            // 650712
    s.raw<std::uint32_t>(off::kDword304) = 0;             // 772
    s.raw<std::uint32_t>(0x9EE10) = 0;                    // 650784
    s.raw<std::uint32_t>(0x9EE0C) = 0;                    // 650780
    s.raw<std::uint32_t>(0x9EE08) = 0;                    // 650776
    sprintf_s(RecentFile(0), 0x100, "%s", g_Locale);      // 650788
    sprintf_s(RecentFile(1), 0x100, "%s", g_Locale);      // 651044
    sprintf_s(RecentFile(2), 0x100, "%s", g_Locale);      // 651300
    s.OverlayTexture() = nullptr;
    s.raw<std::uint32_t>(0x9EE00) = 0;                    // 650768
    s.raw<std::uint32_t>(0x9EDFC) = 0;                    // 650764
    s.raw<std::uint32_t>(off::kDword9F124) = 0;           // 651556
    s.raw<std::uint32_t>(off::kDword9F128) = 0;           // 651560
    s.AviBackgroundTexture() = nullptr;                    // original writes twice
    s.PictureOverlayVertices() = nullptr;
    s.raw<unsigned char>(off::kByte9F12C) = 0;            // 651564
    s.RecordingWindow() = nullptr;                        // 658724
    s.CaptureReadbackPixels() = nullptr;                  // 652084
    s.raw<std::uint32_t>(off::kDword9F130) = 0;           // 651568
    s.LeftViewportVertices() = nullptr;
    s.RightViewportVertices() = nullptr;
    s.raw<unsigned char>(off::kByteA0D28) = 0;            // 658728
    s.PhysicsInterval() = 0.01125f;                       // 658732
    s.FloatingWindow() = nullptr;
    s.raw<unsigned char>(off::kByteA0189) = 0;            // 655753
    s.raw<unsigned char>(655764) = 0;                     // 655764
    s.raw<unsigned char>(655765) = 0;                     // 655765
    s.raw<unsigned char>(655766) = 0;                     // 655766
    s.raw<unsigned char>(655767) = 1;                     // 655767
    s.raw<std::uint32_t>(off::kDwordA0198) = 0;           // 655768
    s.raw<std::uint32_t>(off::kDwordA019C) = 0;           // 655772
    s.raw<std::uint32_t>(off::kDwordA01A0) = 0;           // 655776
    std::memset(at(655780), 0, 0x40);             // 655780
    s.raw<unsigned char>(off::kByteA01E4) = 0;            // 655844
    s.raw<std::uint32_t>(off::kDwordA0268) = 0;           // 655976
    s.raw<std::uint32_t>(off::kDwordA026C) = 0;           // 655980
    s.raw<std::uint32_t>(off::kDwordA0270) = 0;           // 655984
    s.FullscreenMode() = 0;                               // 655988
    s.raw<std::uint32_t>(off::kDwordA027C) = 44;          // 655996
    s.raw<unsigned char>(off::kByteA02A8) = 0;            // 656040
    s.raw<unsigned char>(off::kByteA02B5) = 0;            // 656053
    s.raw<unsigned char>(off::kByteA02B4) = 0;            // 656052
    s.AudioSeekReady() = 0;
    s.AviStereoOutput() = 0;                               // 658785
    s.raw<unsigned char>(off::kByteA03B7) = 0;            // 656311
    s.AviStereoWidthMultiplier() = 2;                      // 658788
    s.raw<unsigned char>(off::kByteA03B8) = 0;            // 656312
    for (std::size_t d = 656316; d <= 656340; d += 4)      // 656316..340 (7)
        s.raw<std::uint32_t>(d) = 0;
    s.raw<unsigned char>(off::kByteA03DC) = 1;            // 656348
    s.raw<unsigned char>(off::kByteA03DD) = 0;            // 656349
    s.raw<unsigned char>(off::kByteA03DE) = 1;            // 656350
    s.raw<unsigned char>(off::kByteA03DF) = 0;            // 656351
    s.SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
    for (std::size_t d = 656357; d <= 656361; ++d)         // 656357..361 (5)
        s.raw<unsigned char>(d) = 0;
    s.raw<std::uint32_t>(off::kDwordA0D6C) = 1;           // 658796
    s.TimelineSelectionChanged() = 0;                      // 656363
    s.ViewModeComboSelection() = 0;                       // 656428
    s.CameraParentModel() = -1;
    s.CameraParentBone() = 0;
    s.raw<std::uint32_t>(off::kDword35C) = 0;             // 860
    s.FrameVolumeControlEnabled() = 0;                    // 672800
    s.FrameNormalization() = 100;                         // 672804
    s.raw<std::uint32_t>(off::kPtrSub04b0) = 0;           // 650656 (subsystem slot)

    // 16-float colour cluster (656440..656503): original zeroes the 12
    // non-one slots then writes 1.0 at 656440/656460/656480/656500.
    for (int f = 0; f < 16; ++f)
        s.raw<float>(off::kFloatColor16 + 4 * f) = 0.0f;
    s.raw<float>(off::kFloatColor16 + 4 * 15) = 1.0f;     // 656500
    s.raw<float>(off::kFloatColor16 + 4 * 10) = 1.0f;     // 656480
    s.raw<float>(off::kFloatColor16 + 4 * 5) = 1.0f;      // 656460
    s.raw<float>(off::kFloatColor16 + 4 * 0) = 1.0f;      // 656440

    s.CameraAttachmentTransformSuppressed() = 0;
    s.raw<unsigned char>(off::kByteA04B8) = 0;            // 656568
    s.WindowLayoutReady() = 1;                             // 672812
    std::memset(at(656632), 0, 0xC8);             // 656632
    s.raw<unsigned char>(off::kByteB6568480) = 0;            // 656848
    s.raw<unsigned char>(off::kByteB6568481) = 0;            // 656849
    s.raw<unsigned char>(off::kByteB6568482) = 0;            // 656850
    s.raw<unsigned char>(off::kByteB6568483) = 0;            // 656851
    s.raw<std::uint32_t>(off::kDwordA0668) = 0;           // 657000
    s.raw<unsigned char>(off::kByteA0665) = 0;            // 656997
    s.raw<unsigned char>(off::kByteA066C) = 0;            // 657004
    s.raw<unsigned char>(off::kByteA066D) = 0;            // 657005
    s.raw<unsigned char>(off::kByteA06B4) = 0;            // 657076
    s.raw<unsigned char>(off::kByteA06B5) = 0;            // 657077
    s.raw<unsigned char>(off::kByteA06B6) = 0;            // 657078
}

}  // namespace mikudancestudio
