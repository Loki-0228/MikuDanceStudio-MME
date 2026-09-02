// ===========================================================================
// VA 0x0040A730 - MMDApp::InitDefaults  (original: sub_40A730)
// ===========================================================================
// Field-by-field port of the post-construction default initializer called
// from WinMain right after the object is zeroed.  Write order follows the
// decompilation exactly; fields whose semantics are still unrecovered keep
// their layout placeholder names (vNNN / aNNNN).
// ===========================================================================
#include <cstdio>
#include <cstring>

#include "mikudancestudio/globals.hpp"
#include "mikudancestudio/mmd_app.hpp"

namespace mikudancestudio {

void MMDApp::InitDefaults() {
    auto& s = *this;

    s.ViewOffsetX() = 0.0f;
    s.CameraDistance() = -45.0f;
    s.ViewOffsetY() = 0.0f;
    state.sidebarResizeDragging = 0;
    s.CameraRotation()[0] = 0.0f;
    s.WaveEnabled() = 0;
    s.CameraRotation()[1] = 0.0f;
    s.DirectSoundAvailable() = 1;
    s.CameraRotation()[2] = 0.0f;
    s.UiOptionFlag(0) = 1;
    s.CameraPosition()[0] = 0.0f;
    s.UiOptionFlag(1) = 1;
    s.UiOptionFlag(2) = 1;
    s.CameraPosition()[1] = 10.0f;
    s.UiOptionFlag(3) = 1;
    s.UiOptionFlag(4) = 1;
    s.CameraPosition()[2] = 0.0f;
    s.UiOptionFlag(5) = 1;
    s.UiOptionFlag(6) = 1;
    s.CameraPerspective() = 0;
    s.GroundGridEnabled() = 1;
    s.ViewportInputActive() = 1;
    s.FpsOverlayEnabled() = 0;
    state.v32c = 0;
    s.PlaybackActive() = 0;
    s.CameraReferenceMode() = CameraAttachmentReference::None;
    s.PlaybackLoopEnabled() = 0;
    state.v342 = 0;
    state.viewportToolHovered = 0;
    state.viewToolDragOperation = 0;
    state.interactionDragMode = 0;
    s.ClearModelSlots();
    s.EditMode() = ViewportEditMode::None;
    state.groundShadowEnabled = 1;
    state.aviBackgroundEnabled = 0;
    s.CaptureMode() = ScreenCaptureMode::Disabled;
    s.ViewportToolOperation() = ViewportToolAction::None;
    s.BoneBoxSelectionActive() = 0;
    s.TimelineStartFrame() = 0;
    s.CurrentFrame() = 0;
    s.PendingTimelineSelectionRow() = TimelineSelectionRow::None;

    // 6-iteration light-default loop
    for (int i = 0; i < 6; ++i) {
        state.lightA[i] = static_cast<unsigned char>(-64);
        state.lightB[i] = 0;
        state.lightC[i] = 64;
        state.lightD[i] = 127;
    }

    state.v350 = 0;
    for (std::int32_t& v : state.v9da24)
        v = 0;
    s.CameraKeys() = nullptr;
    s.LightKeys() = nullptr;
    s.ShadowKeys() = nullptr;
    s.GravityKeys() = nullptr;
    for (int i = 0; i < 255; ++i) {
        s.AccessoryKeys(i) = nullptr;
        s.AccessorySlot(i) = nullptr;
    }
    state.v9e650 = 0.0f;
    s.FpsLimit() = 60.0f;
    state.modelOffsetX = 0.0f;
    s.LastRegisteredFrame() = 0;
    state.modelOffsetY = 0.0f;
    s.AviBackgroundTexture() = nullptr;
    state.modelOffsetZ = 0.0f;
    s.AviBackgroundSurface() = nullptr;
    s.PictureBackgroundTexture() = nullptr;
    s.AviOverlayVertices() = nullptr;
    s.AviFile() = nullptr;
    s.AviStream() = nullptr;
    s.AviFrameReader() = nullptr;
    s.AviUsesThirtyFpsTiming() = 0;
    s.PictureBackgroundEnabled() = 0;
    state.v9eb7e = 0;
    state.blinkPhase = 0;
    s.DisplayObjectListMatchCount() = 0;
    s.DisplayObjectListScrollPosition() = 0;
    s.CaptureTexture() = nullptr;
    state.groundGridVertices = nullptr;
    s.CaptureRenderTarget() = nullptr;
    s.FrameStepPlayback() = 0;
    s.SceneModified() = 0;
    state.v9ed98 = 0;
    s.PlaybackStartsAtCurrentFrame() = 0;
    state.a0B10 = 0;
    s.GroundShadowColorDialog() = nullptr;
    state.modelInfoDialog = nullptr;
    state.projectedShadowBlendEnabled = 1;
    state.v9ed9c = 0;
    state.englishUI = 1;
    FrameRangeDialog() = nullptr;
    s.EnhancedModelDirty() = 0;
    state.frameCopyDialog = nullptr;
    state.cameraRecordArray = nullptr;
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
    state.a0CC8OrUint32 = 0;
    s.PhysicsResetPending() = 0;
    s.PlaybackFrameChanged() = 0;
    state.a0CD4 = 0;
    state.gravityNoise = 10;
    s.AccessoryRenderSplitOrder() = 1;
    s.GravityMagnitude() = 9.8000002f;
    state.v9edcc = 0.0f;
    s.GravityX() = 0.0f;
    s.GravityY() = -1.0f;
    s.GravityZ() = 0.0f;
    state.v9edd0 = 0;
    s.RecordingCompletionFlag() = nullptr;
    state.v9edd8 = 0;
    state.groundGridIndices = nullptr;
    state.spriteOverlayPrimitiveCount = 0;
    // fcn_0040a730 lines 161-182: six dwords zeroed in two blocks of three
    // around the RecentFile sprintf calls - 0x9EE20/0x9EE1C/0x9EE18 first,
    // then 0x9EE14/0x9EE10/0x9EE0C.
    state.sceneFontTexture = nullptr;      // 0x9EE20
    state.textOverlayPrimitiveCount = 0;   // 0x9EE1C
    state.overlayVertices = nullptr;       // 0x9EE18
    sprintf_s(RecentFile(0), 0x100, "%s", g_Locale);
    sprintf_s(RecentFile(1), 0x100, "%s", g_Locale);
    sprintf_s(RecentFile(2), 0x100, "%s", g_Locale);
    state.overlayTexture = nullptr;        // 0x9EE14
    state.spriteOverlayPrimitiveCount = 0; // 0x9EE10
    state.v9ee0cOrUint32 = nullptr;        // 0x9EE0C
    state.lineOverlayPrimitiveCount = 0;
    state.groundPlaneVertices = nullptr;
    s.AviBackgroundTexture() = nullptr;                    // original writes twice
    s.PictureOverlayVertices() = nullptr;
    state.v9f12c = 0;
    s.RecordingWindow() = nullptr;
    s.CaptureReadbackPixels() = nullptr;
    state.projectedShadowRestoreTexture = nullptr;
    s.LeftViewportVertices() = nullptr;
    s.RightViewportVertices() = nullptr;
    state.selfShadowCompositionEnabled = 0;
    s.PhysicsInterval() = 0.01125f;
    s.FloatingWindow() = nullptr;
    state.selectionBoxDragging = 0;
    state.a0194 = 0;
    state.modelOutlineRenderingSuppressed = 0;
    state.a0196 = 0;
    state.a0197 = 1;
    state.modelOutlineColorRed = 0;
    state.modelOutlineColorGreen = 0;
    state.modelOutlineColorBlue = 0;
    std::memset(state.buf655780, 0, 0x40);
    state.a01E4 = 0;
    state.activeRenderObject = nullptr;
    state.activeRenderPass = 0;
    state.a0270 = 0;
    s.FullscreenMode() = 0;
    state.a027COrBuf_bytes = 44;
    state.a02A8 = 0;
    state.a02B5 = 0;
    state.a02B4 = 0;
    s.AudioSeekReady() = 0;
    s.AviStereoOutput() = 0;
    state.a03B7 = 0;
    s.AviStereoWidthMultiplier() = 2;
    state.depthDeviceEnabled = 0;
    state.a03BC = nullptr;
    state.a03C0 = nullptr;
    state.a03C4 = nullptr;
    state.a03C8 = 0;
    state.depthTextureCallback = nullptr;
    state.a03D0 = 0;
    state.a03D4 = 0;
    state.a03DC = 1;
    state.a03DD = 0;
    state.depthTextureCompositionEnabled = 1;
    state.a03DF = 0;
    s.SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
    state.a03E5 = 0;
    state.a03E6 = 0;
    state.a03E7 = 0;
    state.a03E8 = 0;
    state.automaticFrameAdvanceEnabled = 0;
    state.messageSeen = 1;
    s.TimelineSelectionChanged() = 0;
    s.ViewModeComboSelection() = 0;
    s.CameraParentModel() = -1;
    s.CameraParentBone() = 0;
    state.displayClipboard = nullptr;
    s.FrameVolumeControlEnabled() = 0;
    s.FrameNormalization() = 100;
    state.sub04b0OrUint32 = nullptr;

    // 16-float colour cluster: zero all, then identity diagonals at
    // rows 0/2/4/6 (original writes 1.0 at +0/+20/+40/+60).
    for (float& v : state.cameraAttachmentBasis)
        v = 0.0f;
    state.cameraAttachmentBasis[15] = 1.0f;
    state.cameraAttachmentBasis[10] = 1.0f;
    state.cameraAttachmentBasis[5] = 1.0f;
    state.cameraAttachmentBasis[0] = 1.0f;

    s.CameraAttachmentTransformSuppressed() = 0;
    state.a04B8 = 0;
    s.WindowLayoutReady() = 1;
    std::memset(state.buf656632, 0, 0xC8);
    state.b6568480 = 0;
    state.b6568481 = 0;
    state.b6568482 = 0;
    state.b6568483 = 0;
    state.a0668OrUint32 = 0;
    state.a0665 = 0;
    state.a066C = 0;
    state.a066D = 0;
    state.a06B4 = 0;
    state.a06B5 = 0;
    state.a06B6 = 0;
}

}  // namespace mikudancestudio
