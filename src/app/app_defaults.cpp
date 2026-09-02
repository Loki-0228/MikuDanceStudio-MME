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
    m_state.sidebarResizeDragging = 0;
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
    m_state.v32c = 0;
    s.PlaybackActive() = 0;
    s.CameraReferenceMode() = CameraAttachmentReference::None;
    s.PlaybackLoopEnabled() = 0;
    m_state.v342 = 0;
    m_state.viewportToolHovered = 0;
    m_state.viewToolDragOperation = 0;
    m_state.interactionDragMode = 0;
    s.ClearModelSlots();
    s.EditMode() = ViewportEditMode::None;
    m_state.groundShadowEnabled = 1;
    m_state.aviBackgroundEnabled = 0;
    s.CaptureMode() = ScreenCaptureMode::Disabled;
    s.ViewportToolOperation() = ViewportToolAction::None;
    s.BoneBoxSelectionActive() = 0;
    s.TimelineStartFrame() = 0;
    s.CurrentFrame() = 0;
    s.PendingTimelineSelectionRow() = TimelineSelectionRow::None;

    // 6-iteration light-default loop
    for (int i = 0; i < 6; ++i) {
        m_state.lightA[i] = static_cast<unsigned char>(-64);
        m_state.lightB[i] = 0;
        m_state.lightC[i] = 64;
        m_state.lightD[i] = 127;
    }

    m_state.v350 = 0;
    for (std::int32_t& v : m_state.v9da24)
        v = 0;
    s.CameraKeys() = nullptr;
    s.LightKeys() = nullptr;
    s.ShadowKeys() = nullptr;
    s.GravityKeys() = nullptr;
    for (int i = 0; i < 255; ++i) {
        s.AccessoryKeys(i) = nullptr;
        s.AccessorySlot(i) = nullptr;
    }
    m_state.v9e650 = 0.0f;
    s.FpsLimit() = 60.0f;
    m_state.fpsA = 0.0f;
    s.LastRegisteredFrame() = 0;
    m_state.fpsB = 0.0f;
    s.AviBackgroundTexture() = nullptr;
    m_state.fpsC = 0.0f;
    s.AviBackgroundSurface() = nullptr;
    s.PictureBackgroundTexture() = nullptr;
    s.AviOverlayVertices() = nullptr;
    s.AviFile() = nullptr;
    s.AviStream() = nullptr;
    s.AviFrameReader() = nullptr;
    s.AviUsesThirtyFpsTiming() = 0;
    s.PictureBackgroundEnabled() = 0;
    m_state.v9eb7e = 0;
    m_state.v9eb7f = 0;
    s.DisplayObjectListMatchCount() = 0;
    s.DisplayObjectListScrollPosition() = 0;
    s.CaptureTexture() = nullptr;
    m_state.groundGridVertices = nullptr;
    s.CaptureRenderTarget() = nullptr;
    s.FrameStepPlayback() = 0;
    s.SceneModified() = 0;
    m_state.v9ed98 = 0;
    s.PlaybackStartsAtCurrentFrame() = 0;
    m_state.a0B10 = 0;
    m_state.a0B14OrPtr = 0;
    m_state.a0B44OrInt32 = nullptr;
    m_state.projectedShadowBlendEnabled = 1;
    m_state.v9ed9c = 0;
    m_state.englishUI = 1;
    m_state.a0B50OrPtr = 0;
    s.EnhancedModelDirty() = 0;
    m_state.a0b74OrInt32 = nullptr;
    m_state.a0B7COrInt32 = nullptr;
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
    m_state.a0CC8OrUint32 = 0;
    s.PhysicsResetPending() = 0;
    s.PlaybackFrameChanged() = 0;
    m_state.a0CD4 = 0;
    m_state.gravityNoise = 10;
    s.AccessoryRenderSplitOrder() = 1;
    s.GravityMagnitude() = 9.8000002f;
    m_state.v9edcc = 0.0f;
    s.GravityX() = 0.0f;
    s.GravityY() = -1.0f;
    s.GravityZ() = 0.0f;
    m_state.v9edd0 = 0;
    s.RecordingCompletionFlag() = nullptr;
    m_state.v9edd8 = 0;
    m_state.groundGridIndices = nullptr;
    m_state.v9ee10 = 0;
    m_state.v9ee0cOrUint32 = nullptr;
    m_state.toonTextures[10] = nullptr;
    sprintf_s(RecentFile(0), 0x100, "%s", g_Locale);
    sprintf_s(RecentFile(1), 0x100, "%s", g_Locale);
    sprintf_s(RecentFile(2), 0x100, "%s", g_Locale);
    s.OverlayTexture() = nullptr;
    m_state.toonTextures[8] = nullptr;
    m_state.toonTextures[7] = nullptr;
    m_state.lineOverlayPrimitiveCount = 0;
    m_state.groundPlaneVertices = nullptr;
    s.AviBackgroundTexture() = nullptr;                    // original writes twice
    s.PictureOverlayVertices() = nullptr;
    m_state.v9f12c = 0;
    s.RecordingWindow() = nullptr;
    s.CaptureReadbackPixels() = nullptr;
    m_state.projectedShadowRestoreTexture = nullptr;
    s.LeftViewportVertices() = nullptr;
    s.RightViewportVertices() = nullptr;
    m_state.selfShadowCompositionEnabled = 0;
    s.PhysicsInterval() = 0.01125f;
    s.FloatingWindow() = nullptr;
    m_state.selectionBoxDragging = 0;
    m_state.a0194 = 0;
    m_state.modelOutlineRenderingSuppressed = 0;
    m_state.a0196 = 0;
    m_state.a0197 = 1;
    m_state.modelOutlineColorRed = 0;
    m_state.modelOutlineColorGreen = 0;
    m_state.modelOutlineColorBlue = 0;
    std::memset(m_state.buf655780, 0, 0x40);
    m_state.a01E4 = 0;
    m_state.activeRenderObject = nullptr;
    m_state.activeRenderPass = 0;
    m_state.a0270 = 0;
    s.FullscreenMode() = 0;
    m_state.a027COrBuf_bytes = 44;
    m_state.a02A8 = 0;
    m_state.a02B5 = 0;
    m_state.a02B4 = 0;
    s.AudioSeekReady() = 0;
    s.AviStereoOutput() = 0;
    m_state.a03B7 = 0;
    s.AviStereoWidthMultiplier() = 2;
    m_state.depthDeviceEnabled = 0;
    m_state.a03BC = nullptr;
    m_state.a03C0 = nullptr;
    m_state.a03C4 = nullptr;
    m_state.a03C8 = 0;
    m_state.a03CC = nullptr;
    m_state.a03D0 = 0;
    m_state.a03D4 = 0;
    m_state.a03DC = 1;
    m_state.a03DD = 0;
    m_state.depthTextureCompositionEnabled = 1;
    m_state.a03DF = 0;
    s.SelectGlobalTimelineTrack(GlobalTimelineTrack::Camera);
    m_state.a03E5 = 0;
    m_state.a03E6 = 0;
    m_state.a03E7 = 0;
    m_state.a03E8 = 0;
    m_state.a03E9 = 0;
    m_state.messageSeen = 1;
    s.TimelineSelectionChanged() = 0;
    s.ViewModeComboSelection() = 0;
    s.CameraParentModel() = -1;
    s.CameraParentBone() = 0;
    m_state.displayClipboard = nullptr;
    s.FrameVolumeControlEnabled() = 0;
    s.FrameNormalization() = 100;
    m_state.sub04b0OrUint32 = nullptr;

    // 16-float colour cluster: zero all, then identity diagonals at
    // rows 0/2/4/6 (original writes 1.0 at +0/+20/+40/+60).
    for (float& v : m_state.cameraAttachmentBasis)
        v = 0.0f;
    m_state.cameraAttachmentBasis[15] = 1.0f;
    m_state.cameraAttachmentBasis[10] = 1.0f;
    m_state.cameraAttachmentBasis[5] = 1.0f;
    m_state.cameraAttachmentBasis[0] = 1.0f;

    s.CameraAttachmentTransformSuppressed() = 0;
    m_state.a04B8 = 0;
    s.WindowLayoutReady() = 1;
    std::memset(m_state.buf656632, 0, 0xC8);
    m_state.b6568480 = 0;
    m_state.b6568481 = 0;
    m_state.b6568482 = 0;
    m_state.b6568483 = 0;
    m_state.a0668OrUint32 = 0;
    m_state.a0665 = 0;
    m_state.a066C = 0;
    m_state.a066D = 0;
    m_state.a06B4 = 0;
    m_state.a06B5 = 0;
    m_state.a06B6 = 0;
}

}  // namespace mikudancestudio
