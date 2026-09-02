// ===========================================================================
// MikuDanceStudio - layout pin wall
// ===========================================================================
// Regression guard for MMDAppState, split from app_layout.hpp so the
// struct definition reads like the original author's code:
//   * x86: every member pinned to the original binary's byte offset
//     (the ground truth this port mirrors; recovered from IDA + probes).
//   * x64: anchor members pinned; the rest keep their spacing relative to
//     the last anchor, and the size is bounded near the original truth.
// Included at the bottom of app_layout.hpp, inside namespace
// mikudancestudio - requires MMDAppState in scope.  Hand-maintained
// alongside the struct.
// ===========================================================================
#pragma once

#include <cstddef>

#ifndef _M_X64
// every field pinned to its position in the original x86 binary
static_assert(offsetof(MMDAppState, hInstance) == 0,
              "hInstance x86");
static_assert(offsetof(MMDAppState, mouseX) == 4,
              "mouseX x86");
static_assert(offsetof(MMDAppState, mouseY) == 8,
              "mouseY x86");
static_assert(offsetof(MMDAppState, previousMouseX) == 12,
              "previousMouseX x86");
static_assert(offsetof(MMDAppState, previousMouseY) == 16,
              "previousMouseY x86");
static_assert(offsetof(MMDAppState, upKeyState) == 20,
              "upKeyState x86");
static_assert(offsetof(MMDAppState, downKeyState) == 24,
              "downKeyState x86");
static_assert(offsetof(MMDAppState, leftKeyState) == 28,
              "leftKeyState x86");
static_assert(offsetof(MMDAppState, rightKeyState) == 32,
              "rightKeyState x86");
static_assert(offsetof(MMDAppState, shiftModifierState) == 36,
              "shiftModifierState x86");
static_assert(offsetof(MMDAppState, spaceKeyState) == 40,
              "spaceKeyState x86");
static_assert(offsetof(MMDAppState, escKeyState) == 44,
              "escKeyState x86");
static_assert(offsetof(MMDAppState, dialogFlags) == 48,
              "dialogFlags x86");
static_assert(offsetof(MMDAppState, dialogFlags[16]) == 112,
              "dialogFlags[16] x86");
static_assert(offsetof(MMDAppState, dialogFlags[17]) == 116,
              "dialogFlags[17] x86");
static_assert(offsetof(MMDAppState, keyState221) == 120,
              "keyState221 x86");
static_assert(offsetof(MMDAppState, keyState226) == 124,
              "keyState226 x86");
static_assert(offsetof(MMDAppState, tabKeyState) == 128,
              "tabKeyState x86");
static_assert(offsetof(MMDAppState, leftMouseButtonState) == 132,
              "leftMouseButtonState x86");
static_assert(offsetof(MMDAppState, rightMouseButtonState) == 136,
              "rightMouseButtonState x86");
static_assert(offsetof(MMDAppState, middleMouseButtonState) == 140,
              "middleMouseButtonState x86");
static_assert(offsetof(MMDAppState, numpadKeyState) == 144,
              "numpadKeyState x86");
static_assert(offsetof(MMDAppState, numpadKeyState[9]) == 180,
              "numpadKeyState[9] x86");
static_assert(offsetof(MMDAppState, deleteKeyState) == 184,
              "deleteKeyState x86");
static_assert(offsetof(MMDAppState, bC) == 188,
              "bC x86");
static_assert(offsetof(MMDAppState, ctrlModifierState) == 192,
              "ctrlModifierState x86");
static_assert(offsetof(MMDAppState, menuKeyState) == 196,
              "menuKeyState x86");
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
static_assert(offsetof(MMDAppState, v350Clipboard) == 848,
              "v350Clipboard x86");
#ifndef _M_X64
static_assert(offsetof(MMDAppState, boneClipboard) == 852,
              "boneClipboard x86");
static_assert(offsetof(MMDAppState, morphClipboard) == 856,
              "morphClipboard x86");
static_assert(offsetof(MMDAppState, cameraClipboard) == 864,
              "cameraClipboard x86");
static_assert(offsetof(MMDAppState, lightClipboard) == 868,
              "lightClipboard x86");
static_assert(offsetof(MMDAppState, shadowClipboard) == 872,
              "shadowClipboard x86");
static_assert(offsetof(MMDAppState, gravityClipboard) == 876,
              "gravityClipboard x86");
static_assert(offsetof(MMDAppState, accessoryClipboard) == 880,
              "accessoryClipboard x86");
#endif
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
static_assert(offsetof(MMDAppState, viewportToolCenterX) == 2336,
              "viewportToolCenterX x86");
static_assert(offsetof(MMDAppState, viewportToolCenterY) == 2340,
              "viewportToolCenterY x86");
static_assert(offsetof(MMDAppState, selectedClipW) == 2344,
              "selectedClipW x86");
static_assert(offsetof(MMDAppState, boneBoxStartX) == 2360,
              "boneBoxStartX x86");
static_assert(offsetof(MMDAppState, boneBoxStartY) == 2364,
              "boneBoxStartY x86");
static_assert(offsetof(MMDAppState, cameraTrackCursor) == 648796,
              "cameraTrackCursor x86");
static_assert(offsetof(MMDAppState, cameraTrackActive) == 648800,
              "cameraTrackActive x86");
static_assert(offsetof(MMDAppState, lightTrackCursor) == 648804,
              "lightTrackCursor x86");
static_assert(offsetof(MMDAppState, lightTrackActive) == 648808,
              "lightTrackActive x86");
static_assert(offsetof(MMDAppState, shadowTrackCursor) == 648812,
              "shadowTrackCursor x86");
static_assert(offsetof(MMDAppState, shadowTrackActive) == 648816,
              "shadowTrackActive x86");
static_assert(offsetof(MMDAppState, gravityTrackCursor) == 648820,
              "gravityTrackCursor x86");
static_assert(offsetof(MMDAppState, gravityTrackActive) == 648824,
              "gravityTrackActive x86");
static_assert(offsetof(MMDAppState, accessoryTrackCursor) == 648828,
              "accessoryTrackCursor x86");
static_assert(offsetof(MMDAppState, accessoryTrackCursor[54]) == 649044,
              "accessoryTrackCursor[54] x86");
static_assert(offsetof(MMDAppState, accessoryTrackActive) == 649848,
              "accessoryTrackActive x86");
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
static_assert(offsetof(MMDAppState, timelineScrollNPage) == 2416,
              "timelineScrollNPage x86");
static_assert(offsetof(MMDAppState, timelineScrollNMin) == 2420,
              "timelineScrollNMin x86");
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
static_assert(offsetof(MMDAppState, interpCurveUniformFound) == 645636,
              "interpCurveUniformFound x86");
static_assert(offsetof(MMDAppState, interpCurveControlCache) == 645637,
              "interpCurveControlCache x86");
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
static_assert(offsetof(MMDAppState, pictureBackgroundEnabled) == 648232,
              "pictureBackgroundEnabled x86");
static_assert(offsetof(MMDAppState, pictureBackgroundTexture) == 648236,
              "pictureBackgroundTexture x86");
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
static_assert(offsetof(MMDAppState, playbackEnabledSnapshot) == 650103,
              "playbackEnabledSnapshot x86");
static_assert(offsetof(MMDAppState, v9eb7e) == 650110,
              "v9eb7e x86");
static_assert(offsetof(MMDAppState, blinkPhase) == 650111,
              "blinkPhase x86");
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
static_assert(offsetof(MMDAppState, captureSavePath) == 651572,
              "captureSavePath x86");
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
static_assert(offsetof(MMDAppState, selectionBoxAnchorX) == 655756,
              "selectionBoxAnchorX x86");
static_assert(offsetof(MMDAppState, selectionBoxAnchorY) == 655760,
              "selectionBoxAnchorY x86");
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
static_assert(offsetof(MMDAppState, wireframeRenderingEnabled) == 655828,
              "wireframeRenderingEnabled x86");
static_assert(offsetof(MMDAppState, a01E4) == 655844,
              "a01E4 x86");
static_assert(offsetof(MMDAppState, lightViewProjectionMatrix) == 655848,
              "lightViewProjectionMatrix x86");
static_assert(offsetof(MMDAppState, worldViewProjectionMatrix) == 655912,
              "worldViewProjectionMatrix x86");
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
#ifndef _M_X64
static_assert(offsetof(MMDAppState, savedPlacement) == 655996,
              "savedPlacement x86");
#endif
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
static_assert(offsetof(MMDAppState, openniTrackingCallback) == 656340,
              "openniTrackingCallback x86");
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
#ifndef _M_X64
static_assert(offsetof(MMDAppState, timelineSelectionSlots) == 656364,
              "timelineSelectionSlots x86");
static_assert(offsetof(MMDAppState, timelineSelectionSlots[15]) == 656424,
              "timelineSelectionSlots[15] x86");
#endif
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
static_assert(offsetof(MMDAppState, timelineRangeFirstOffset) == 656832,
              "timelineRangeFirstOffset x86");
static_assert(offsetof(MMDAppState, timelineRangeLastBase) == 656844,
              "timelineRangeLastBase x86");
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
static_assert(offsetof(MMDAppState, accessoryApplyGate) == 656996,
              "accessoryApplyGate x86");
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
#ifndef _M_X64
static_assert(offsetof(MMDAppState, viewRotationTransform) == 657012,
              "viewRotationTransform x86");
#endif
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
static_assert(offsetof(MMDAppState, modelOffsetX) == 657636,
              "modelOffsetX x86");
static_assert(offsetof(MMDAppState, modelOffsetY) == 657640,
              "modelOffsetY x86");
static_assert(offsetof(MMDAppState, modelOffsetZ) == 657644,
              "modelOffsetZ x86");
static_assert(offsetof(MMDAppState, morphFrameShift) == 657652,
              "morphFrameShift x86");
static_assert(offsetof(MMDAppState, blinkStartFrame) == 657656,
              "blinkStartFrame x86");
static_assert(offsetof(MMDAppState, blinkEndFrame) == 657660,
              "blinkEndFrame x86");
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
static_assert(offsetof(MMDAppState, groundShadowColorDialog) == 658196,
              "groundShadowColorDialog x86");
static_assert(offsetof(MMDAppState, groundShadowColorEditProc) == 658200,
              "groundShadowColorEditProc x86");
static_assert(offsetof(MMDAppState, accessoryOrderArray) == 658204,
              "accessoryOrderArray x86");
static_assert(offsetof(MMDAppState, accessoryEditArray) == 658212,
              "accessoryEditArray x86");
static_assert(offsetof(MMDAppState, accessoryRenderSplitOrder) == 658208,
              "accessoryRenderSplitOrder x86");
static_assert(offsetof(MMDAppState, rotationDialogTemp) == 658216,
              "rotationDialogTemp x86");
static_assert(offsetof(MMDAppState, modelInfoDialog) == 658244,
              "modelInfoDialog x86");
static_assert(offsetof(MMDAppState, modelInfoEditProc) == 658248,
              "modelInfoEditProc x86");
#ifndef _M_X64
static_assert(offsetof(MMDAppState, frameRangeDialog) == 658256,
              "frameRangeDialog x86");
static_assert(offsetof(MMDAppState, modelEdgeEditProc) == 658260,
              "modelEdgeEditProc x86");
static_assert(offsetof(MMDAppState, modelEdgeComboCursor) == 658264,
              "modelEdgeComboCursor x86");
#endif
static_assert(offsetof(MMDAppState, englishUI) == 658252,
              "englishUI x86");
static_assert(offsetof(MMDAppState, a0B64) == 658276,
              "a0B64 x86");
static_assert(offsetof(MMDAppState, timeNowLow) == 658280,
              "timeNowLow x86");
static_assert(offsetof(MMDAppState, timeNowHigh) == 658284,
              "timeNowHigh x86");
static_assert(offsetof(MMDAppState, milliToSec) == 658288,
              "milliToSec x86");
static_assert(offsetof(MMDAppState, frameCopyDialog) == 658292,
              "frameCopyDialog x86");
static_assert(offsetof(MMDAppState, frameCopyEditProc) == 658296,
              "frameCopyEditProc x86");
static_assert(offsetof(MMDAppState, cameraRecordArray) == 658300,
              "cameraRecordArray x86");
static_assert(offsetof(MMDAppState, cameraFrameScratch) == 658304,
              "cameraFrameScratch x86");
static_assert(offsetof(MMDAppState, selAcc) == 658476,
              "selAcc x86");
static_assert(offsetof(MMDAppState, boneRecordArray) == 658480,
              "boneRecordArray x86");
static_assert(offsetof(MMDAppState, boneFrameScratch) == 658484,
              "boneFrameScratch x86");
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
static_assert(offsetof(MMDAppState, accessoryFrameEditProc) == 658640,
              "accessoryFrameEditProc x86");
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
