// ===========================================================================
// MikuDanceStudio - the MikuMikuDance v932 application state object
// ===========================================================================
// Original form:
//   WinMain (VA 0x004C4460) performs
//       void* p = operator new(0xA4530);
//       Block = p;                        // .data global @ 0x0054593C
//       memset(p, 0, 0xA4530);
//   The whole program is a single 0xA4530-byte object; every subsystem
//   addresses fields through fixed byte offsets (see offsets.hpp).
//
// Restoration strategy ("1:1"):
//   * The class owns one MMDAppState (app_layout.hpp) whose layout is
//     pinned to the original binary by static_asserts.
//   * Named accessors return typed members; fields whose semantics are
//     still being recovered are reached through raw<T>(offset) until
//     they are promoted to members.  The goal is for raw<T>() to
//     disappear entirely (scripts/promote.py tracks the remainder).
// ===========================================================================
#pragma once

#include <cstddef>
#include <cstdint>

#include "mikudancestudio/app_layout.hpp"
#include "mikudancestudio/accessory_layout.hpp"
#include "mikudancestudio/clipboard_layout.hpp"
#include "mikudancestudio/d3d_wrapper.hpp"
#include "mikudancestudio/dshow_recorder.hpp"
#include "mikudancestudio/global_key_layout.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/offsets_xlate.hpp"
#include "mikudancestudio/physics_scene.hpp"
#include "mikudancestudio/path_workspace.hpp"
#include "mikudancestudio/wave_audio_context.hpp"

namespace mikudancestudio {

enum class UiThemeColor : int {
    WindowFill = 0,
    WindowBorder = 1,
    ControlLight = 2,
    ControlDark = 3,
    TimelineBase = 22,
    TimelineRows = 23,
    TimelineGrid = 24,
    PanelHeader = 25,
    PanelBody = 26,
    HeaderBand = 27,
    RowBand = 28,
    LabelGrid = 29,
    SelectionText = 30,
    SelectionFill = 31,
    SelectionBorder = 32,
    DisabledText = 33,
    Accent = 34,
};

enum class AccessoryRenderPass : std::int32_t {
    None = 0,
    FixedFunction = 1,
    Effect = 2,
    ProjectedGroundShadow = 3,
    ModelOutline = 4,
    ModelEffect = 5,
};

enum class ViewportEditMode : std::int32_t {
    Bone = 0,
    BoneBox = 1,
    None = 2,
    Camera = 3,
    Light = 4,
    ToolDrag = 5,
};

enum class GlobalTimelineTrack : std::uint8_t {
    Camera = 0,
    Light = 1,
    SelfShadow = 2,
    Gravity = 3,
};

enum class TimelineSelectionBand : std::uint8_t {
    Camera = 0,
    Light = 1,
    SelfShadow = 2,
    Gravity = 3,
    Accessory = 4,
    ModelIk = 5,
    ModelMorph = 6,
    ModelBone = 7,
};

enum class TimelineSelectionRow : std::uint8_t {
    None = 0,
    Bone = 1,
    Morph = 2,
};

struct TimelineSelectionRecord {
    std::int32_t words[4];
};

enum class ScreenCaptureMode : std::int32_t {
    Disabled = 0,
    FullFrame = 1,
    CropFourByThree = 2,
    BackgroundRefresh = 3,
};

using DepthTextureProvider = void(__stdcall*)(IDirect3DBaseTexture9**);

enum class ViewportDragMode : std::int32_t {
    None = 0,
    ViewAxisRotateX = 1,
    ViewAxisRotateY = 2,
    ViewAxisRotateZ = 3,
    LocalAxisTranslateX = 4,
    LocalAxisTranslateY = 5,
    LocalAxisTranslateZ = 6,
    BoneScale = 7,
    BoneMoveVertical = 8,
    BoneMoveScreenPlane = 9,
    PhysicsAxisX = 10,
    PhysicsAxisY = 11,
    PhysicsAxisZ = 12,
    CameraAdjustX = 13,
    CameraAdjustY = 14,
    CameraAdjustZ = 15,
    AngleAdjustX = 16,
    AngleAdjustY = 17,
    AngleAdjustZ = 18,
};

enum class ViewportToolAction : std::int32_t {
    None = 0,
    CameraOrbit = 1,
    CameraPan = 2,
    CameraGizmoHorizontal = 3,
    CameraGizmoVertical = 4,
    CameraGizmoRing = 5,
    LightGizmoHorizontal = 6,
    LightGizmoVertical = 7,
    LightGizmoCenter = 8,
    TransformAxisX = 9,
    TransformAxisY = 10,
    TransformAxisZ = 11,
    PhysicsAxisX = 12,
    PhysicsAxisY = 13,
    PhysicsAxisZ = 14,
    CameraAdjustX = 15,
    CameraAdjustY = 16,
    CameraAdjustZ = 17,
    AngleAdjustX = 18,
    AngleAdjustY = 19,
    AngleAdjustZ = 20,
    CycleCoordinateSystem = 21,
};

enum class CameraAttachmentReference : std::uint8_t {
    None = 0,
    ModelRoot = 1,
    SelectedBone = 2,
};

class MMDApp {
public:
    static constexpr std::size_t kSize = offsets::kObjectSize;

    MMDApp();                                // VA 0x0042AE60
    void InitDefaults();                     // VA 0x0040A730

    // The restored application state.  Public by design: the original was
    // one giant class whose fields every subsystem touched directly; free
    // functions taking MMDApp* read and write `app->state.<field>` exactly
    // like the original's `this-><field>`.
    MMDAppState state;

    // Frame-range output dialog HWND (x86 blob slot 0xA0B50).
    HWND& FrameRangeDialog() {
#if defined(_M_X64)
        return m_frameRangeDialog;
#else
        return state.frameRangeDialog;
#endif
    }

    // Sub-window torn down on scene reset (x86 blob slot 0xA0A6C).
    HWND& SceneResetSubWindow() {
#if defined(_M_X64)
        return m_hwndA0A6C;
#else
        return state.hwndA0A6C;
#endif
    }

    // -- raw field access, byte offsets as in the decompilation ----------
    // On x64 the state blob uses the original x64 layout: the x86 offsets
    // from the decompilation are translated through offsets_xlate.hpp
    // (generated by the IDB cross-matcher; unmapped offsets pass through
    // unchanged and get fixed by crash iteration).
    template <typename T>
    T& raw(std::size_t byteOffset) {
        return *reinterpret_cast<T*>(storage() + MIKUDANCESTUDIO_XLATE(byteOffset));
    }
    template <typename T>
    const T& raw(std::size_t byteOffset) const {
        return *reinterpret_cast<const T*>(storage() + MIKUDANCESTUDIO_XLATE(byteOffset));
    }

    // Byte address of an x86-decompilation offset, translated for the
    // running architecture.  Slot arithmetic computed in x86 units
    // (`kSlotArray + 4 * slot`) must go through this - and the xlate
    // carries element-wise entries for the known pointer-array families,
    // so `at(base + 4*i)` lands on element i of the x64 pointer array.
    unsigned char* at(std::size_t x86Offset) {
        return storage() + MIKUDANCESTUDIO_XLATE(x86Offset);
    }
    const unsigned char* at(std::size_t x86Offset) const {
        return storage() + MIKUDANCESTUDIO_XLATE(x86Offset);
    }

    PathResolutionWorkspace& PathWorkspace() {
#if defined(_M_X64)
        return m_pathWorkspace;
#else
        return raw<PathResolutionWorkspace>(offsets::kBufFontsub);
#endif
    }
    const PathResolutionWorkspace& PathWorkspace() const {
#if defined(_M_X64)
        return m_pathWorkspace;
#else
        return raw<PathResolutionWorkspace>(offsets::kBufFontsub);
#endif
    }

    mdl::AccessoryRecord*& AccessorySlot(int index) {
        return AccessorySlots()[index];
    }
    mdl::AccessoryRecord* AccessorySlot(int index) const {
        return AccessorySlots()[index];
    }
    mdl::AccessoryRecord** AccessorySlots() {
        return reinterpret_cast<mdl::AccessoryRecord**>(
            at(offsets::kBufBuf9ddx));
    }
    mdl::AccessoryRecord* const* AccessorySlots() const {
        return reinterpret_cast<mdl::AccessoryRecord* const*>(
            at(offsets::kBufBuf9ddx));
    }
    // The 255-entry display-object list shares storage with accessory
    // records, but also contains objects selected by the model timeline.
    void*& ObjectSlot(int index) {
        return reinterpret_cast<void*&>(AccessorySlots()[index]);
    }
    const void* ObjectSlot(int index) const {
        return reinterpret_cast<void* const&>(AccessorySlots()[index]);
    }
    unsigned char*& ModelSlot(int index) {
        return ModelSlots()[index];
    }
    unsigned char* ModelSlot(int index) const {
        return ModelSlots()[index];
    }
    unsigned char** ModelSlots() {
        return reinterpret_cast<unsigned char**>(state.modelSlots);
    }
    unsigned char* const* ModelSlots() const {
        return reinterpret_cast<unsigned char* const*>(state.modelSlots);
    }
    void ClearModelSlots() {
        for (int index = 0; index < 100; ++index)
            ModelSlot(index) = nullptr;
    }
    std::uint8_t& SelectedModelSlot() {
        return raw<std::uint8_t>(offsets::kByteSlotidx);
    }
    std::uint8_t SelectedModelSlot() const {
        return raw<std::uint8_t>(offsets::kByteSlotidx);
    }
    void SetSelectedModelSlot(std::uint8_t slot) {
        SelectedModelSlot() = slot;
    }
    unsigned char*& SelectedModel() {
        return ModelSlot(SelectedModelSlot());
    }
    unsigned char* SelectedModel() const {
        return ModelSlot(SelectedModelSlot());
    }
    mdl::BoneClipboardRecord*& BoneClipboard() {
        return raw<mdl::BoneClipboardRecord*>(0x354);
    }
    mdl::MorphClipboardRecord*& MorphClipboard() {
        return raw<mdl::MorphClipboardRecord*>(0x358);
    }
    mdl::DisplayClipboardRecord*& DisplayClipboard() {
        return raw<mdl::DisplayClipboardRecord*>(0x35C);
    }
    mdl::CameraClipboardRecord*& CameraClipboard() {
        return raw<mdl::CameraClipboardRecord*>(0x360);
    }
    mdl::LightClipboardRecord*& LightClipboard() {
        return raw<mdl::LightClipboardRecord*>(0x364);
    }
    mdl::ShadowClipboardRecord*& ShadowClipboard() {
        return raw<mdl::ShadowClipboardRecord*>(0x368);
    }
    mdl::GravityClipboardRecord*& GravityClipboard() {
        return raw<mdl::GravityClipboardRecord*>(0x36C);
    }
    mdl::AccessoryClipboardKey*& AccessoryClipboard() {
        return raw<mdl::AccessoryClipboardKey*>(0x370);
    }
    mdl::CameraKey*& CameraKeys() {
        return raw<mdl::CameraKey*>(0x374);
    }
    mdl::LightKey*& LightKeys() {
        return raw<mdl::LightKey*>(0x378);
    }
    mdl::SelfShadowKey*& ShadowKeys() {
        return raw<mdl::SelfShadowKey*>(0x37C);
    }
    mdl::GravityKey*& GravityKeys() {
        return raw<mdl::GravityKey*>(0x380);
    }
    mdl::AccessoryKey*& AccessoryKeys(int slot) {
        return AccessoryKeyTracks()[slot];
    }
    mdl::AccessoryKey** AccessoryKeyTracks() {
        return reinterpret_cast<mdl::AccessoryKey**>(
            at(offsets::kBufAcctrk));
    }
    mdl::AccessoryKey* const* AccessoryKeyTracks() const {
        return reinterpret_cast<mdl::AccessoryKey* const*>(
            at(offsets::kBufAcctrk));
    }
    std::uint8_t& GlobalTrackSelected(GlobalTimelineTrack track) {
        return raw<std::uint8_t>(offsets::kByteA03E4 +
                                 static_cast<std::uint8_t>(track));
    }
    std::uint8_t GlobalTrackSelected(GlobalTimelineTrack track) const {
        return raw<std::uint8_t>(offsets::kByteA03E4 +
                                 static_cast<std::uint8_t>(track));
    }
    void SelectGlobalTimelineTrack(GlobalTimelineTrack track) {
        for (std::uint8_t index = 0; index < 4; ++index)
            raw<std::uint8_t>(offsets::kByteA03E4 + index) =
                index == static_cast<std::uint8_t>(track) ? 1 : 0;
    }
    void ClearGlobalTimelineTrackSelection() {
        for (std::uint8_t index = 0; index < 4; ++index)
            raw<std::uint8_t>(offsets::kByteA03E4 + index) = 0;
    }
    std::uint8_t& TimelineSelectionChanged() {
        return state.timelineSelectionChanged;
    }
    std::uint8_t TimelineSelectionChanged() const {
        return state.timelineSelectionChanged;
    }
    std::uint8_t& SceneModified() {
        return state.sceneModified;
    }
    std::uint8_t SceneModified() const {
        return state.sceneModified;
    }
    TimelineSelectionRow& PendingTimelineSelectionRow() {
        return reinterpret_cast<TimelineSelectionRow&>(state.pendingTimelineSelectionRow);
    }
    TimelineSelectionRow PendingTimelineSelectionRow() const {
        return static_cast<TimelineSelectionRow>(state.pendingTimelineSelectionRow);
    }
    std::uint8_t& SelectionBoxDragging() { return raw<std::uint8_t>(0xA0189); }
    std::int32_t& MouseX() { return raw<std::int32_t>(0x4); }
    std::int32_t& MouseY() { return raw<std::int32_t>(0x8); }
    std::int32_t& PreviousMouseX() { return raw<std::int32_t>(0xC); }
    std::int32_t& PreviousMouseY() { return raw<std::int32_t>(0x10); }
    std::uint8_t& ViewportInputActive() {
        return state.viewportInputActive;
    }
    std::int32_t& ShiftModifierState() { return raw<std::int32_t>(0x24); }
    std::int32_t& CtrlModifierState() { return raw<std::int32_t>(0xC0); }
    bool ShiftModifierActive() const {
        return raw<std::int32_t>(0x24) == 3;
    }
    bool CtrlModifierActive() const {
        return raw<std::int32_t>(0xC0) == 3;
    }
    std::uint8_t& SidebarResizeDragging() {
        return state.sidebarResizeDragging;
    }
    std::int32_t& LeftMouseButtonState() { return raw<std::int32_t>(0x84); }
    std::int32_t& RightMouseButtonState() { return raw<std::int32_t>(0x88); }
    std::int32_t& MiddleMouseButtonState() { return raw<std::int32_t>(0x8C); }
    bool LeftMouseButtonHeld() const { return raw<std::int32_t>(0x84) == 3; }
    bool RightMouseButtonHeld() const { return raw<std::int32_t>(0x88) == 3; }
    bool MiddleMouseButtonHeld() const { return raw<std::int32_t>(0x8C) == 3; }
    std::int32_t& SelectionBoxAnchorX() { return raw<std::int32_t>(0xA018C); }
    std::int32_t& SelectionBoxAnchorY() { return raw<std::int32_t>(0xA0190); }
    std::int32_t& TimelineRangeFirstOffset() { return raw<std::int32_t>(0xA05C0); }
    std::int32_t& TimelineRangeFirstBase() { return raw<std::int32_t>(0xA05C4); }
    std::int32_t& TimelineRangeLastOffset() { return raw<std::int32_t>(0xA05C8); }
    std::int32_t& TimelineRangeLastBase() { return raw<std::int32_t>(0xA05CC); }
    std::uint8_t& TimelineRangeApplyEnabled() {
        return raw<std::uint8_t>(0xA05D0);
    }
    void ClearTimelineRange() {
        TimelineRangeFirstOffset() = 0;
        TimelineRangeFirstBase() = 0;
        TimelineRangeLastOffset() = 0;
        TimelineRangeLastBase() = 0;
        TimelineRangeApplyEnabled() = 0;
    }
    std::int32_t& TimelineSelectionCount(TimelineSelectionBand band) {
        return raw<std::int32_t>(0xA03EC +
                                 8 * static_cast<std::uint8_t>(band));
    }
    TimelineSelectionRecord*& TimelineSelectionRecords(
        TimelineSelectionBand band) {
        return raw<TimelineSelectionRecord*>(
            0xA03F0 + 8 * static_cast<std::uint8_t>(band));
    }
    mdl::ClipboardSelectionCounts& ClipboardCounts() {
        return raw<mdl::ClipboardSelectionCounts>(offsets::kDword9DA28);
    }
    const mdl::ClipboardSelectionCounts& ClipboardCounts() const {
        return raw<mdl::ClipboardSelectionCounts>(offsets::kDword9DA28);
    }
    std::int32_t& CurrentFrame() {
        // The generated layout stores the frame counter unsigned; keep the
        // historical signed view so signed comparisons at call sites are
        // unchanged.
        return reinterpret_cast<std::int32_t&>(state.currentFrame);
    }
    HWND& MainWindow() {
        return state.hwnd;
    }
    float* LightDirection() {
        return &state.lightDirection;
    }
    float* LightColor() {
        return &state.lightColor;
    }
    D3DLIGHT9& SceneLight() {
#if defined(_M_X64)
        return m_sceneLight;
#else
        return raw<D3DLIGHT9>(647552);  // original state block +0x9E180
#endif
    }
    const D3DLIGHT9& SceneLight() const {
#if defined(_M_X64)
        return m_sceneLight;
#else
        return raw<D3DLIGHT9>(647552);
#endif
    }
    // The PMM light track stores only RGB and direction.  Its target is the
    // application's directional key light, whose fixed-function fields must
    // remain coherent whenever a key is applied or interpolated.
    void ApplyTimelineLightState() {
        D3DLIGHT9& light = SceneLight();
        light.Type = D3DLIGHT_DIRECTIONAL;
        light.Direction.x = LightDirection()[0];
        light.Direction.y = LightDirection()[1];
        light.Direction.z = LightDirection()[2];
        light.Specular.r = LightColor()[0];
        light.Specular.g = LightColor()[1];
        light.Specular.b = LightColor()[2];
        light.Ambient.r = LightColor()[0];
        light.Ambient.g = LightColor()[1];
        light.Ambient.b = LightColor()[2];
    }
    std::uint8_t& GroundShadowEnabled() {
        return raw<std::uint8_t>(offsets::kByte918);
    }
    IDirect3DVertexBuffer9*& GroundGridVertices() {
        return raw<IDirect3DVertexBuffer9*>(768);
    }
    IDirect3DIndexBuffer9*& GroundGridIndices() {
        return raw<IDirect3DIndexBuffer9*>(772);
    }
    ViewportEditMode& EditMode() {
        return reinterpret_cast<ViewportEditMode&>(state.editMode);
    }
    const ViewportEditMode& EditMode() const {
        return const_cast<const ViewportEditMode&>(
            reinterpret_cast<const ViewportEditMode&>(state.editMode));
    }
    bool UsesViewportTool() const {
        return static_cast<std::int32_t>(EditMode()) >=
               static_cast<std::int32_t>(ViewportEditMode::None);
    }
    std::int32_t& ViewportToolCenterX() { return raw<std::int32_t>(2336); }
    std::int32_t& ViewportToolCenterY() { return raw<std::int32_t>(2340); }
    std::uint32_t& ViewportToolHovered() {
        return raw<std::uint32_t>(offsets::kDwordToolhover);
    }
    ViewportToolAction& ViewportToolOperation() {
        return reinterpret_cast<ViewportToolAction&>(state.viewportToolOperation);
    }
    ViewportToolAction& ViewToolDragOperation() {
        return raw<ViewportToolAction>(offsets::kDwordViewdragmode);
    }
    ViewportDragMode& InteractionDragMode() {
        return raw<ViewportDragMode>(offsets::kDwordInteractionmode);
    }
    std::int32_t& ViewportToolDragOriginX() {
        return state.dragOriginX;
    }
    std::int32_t& ViewportToolDragOriginY() {
        return state.dragOriginY;
    }
    std::int32_t& BoneBoxStartX() { return raw<std::int32_t>(2360); }
    std::int32_t& BoneBoxStartY() { return raw<std::int32_t>(2364); }
    std::int32_t& BoneBoxSelectionActive() {
        return state.boneBoxSelectionActive;
    }
    D3DMATRIX& LightViewProjection() {
        return raw<D3DMATRIX>(655848);  // 0xA0188
    }
    const D3DMATRIX& LightViewProjection() const {
        return raw<D3DMATRIX>(655848);
    }
    D3DMATRIX& WorldViewProjection() {
        return raw<D3DMATRIX>(655912);  // 0xA01C8
    }
    const D3DMATRIX& WorldViewProjection() const {
        return raw<D3DMATRIX>(655912);
    }
    IDirect3DTexture9*& AviBackgroundTexture() {
        return reinterpret_cast<IDirect3DTexture9*&>(state.aviBackgroundTexture);
    }
    IDirect3DSurface9*& AviBackgroundSurface() {
        return reinterpret_cast<IDirect3DSurface9*&>(state.aviBackgroundSurface);
    }
    void*& AviDrawDib() {
        return raw<void*>(offsets::kDword9e3ec);
    }
    void*& AviFile() {
        return state.aviFile;
    }
    void*& AviStream() {
        return raw<void*>(offsets::kDword9E400);
    }
    void*& AviFrameReader() {
        return state.aviFrameReader;
    }
    HWND& FloatingWindow() {
        return state.floatingWindow;
    }
    HWND& RecordingWindow() {
        return state.recordingWindow;
    }
    IDirect3DTexture9*& ToonTexture(int index) {
        return raw<IDirect3DTexture9*>(offsets::kPtrToontex + 4 * index);
    }
    IDirect3DTexture9*& SceneFontTexture() {
        return raw<IDirect3DTexture9*>(offsets::kPtrFonttex);
    }
    IDirect3DVertexBuffer9*& AviOverlayVertices() {
#if defined(_M_X64)
        return m_overlayVertexBuffers.avi;
#else
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(state.v9e3f8OrPtr);
#endif
    }
    IDirect3DTexture9*& PictureBackgroundTexture() {
        return raw<IDirect3DTexture9*>(offsets::kDword9E42C);
    }
    IDirect3DVertexBuffer9*& PictureOverlayVertices() {
#if defined(_M_X64)
        return m_overlayVertexBuffers.picture;
#else
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(state.v9e430OrPtr);
#endif
    }
    wchar_t* AviBackgroundPath() {
        return reinterpret_cast<wchar_t*>(at(offsets::kWcs9e1ec));
    }
    wchar_t* PictureBackgroundPath() {
        return state.pictureBackgroundPath;
    }
    std::int32_t& AviStreamStartFrame() {
        return state.aviStreamStart;
    }
    std::int32_t& AviStreamEndFrame() {
        return state.aviStreamEnd;
    }
    std::uint8_t& AviUsesThirtyFpsTiming() {
        return state.aviUsesThirtyFpsTiming;
    }
    std::int32_t& AviOffsetX() { return state.aviOffsetX; }
    std::int32_t& AviOffsetY() { return state.aviOffsetY; }
    float& AviScale() { return state.aviScale; }
    std::int32_t& AviFrameWidth() { return state.aviFrameWidth; }
    std::int32_t& AviFrameHeight() { return state.aviFrameHeight; }
    std::uint8_t& PictureBackgroundEnabled() {
        return raw<std::uint8_t>(offsets::kByte9E428);
    }
    std::int32_t& PictureOffsetX() { return state.pictureOffsetX; }
    std::int32_t& PictureOffsetY() { return state.pictureOffsetY; }
    float& PictureScale() { return state.pictureScale; }
    std::int32_t& PictureWidth() { return state.pictureWidth; }
    std::int32_t& PictureHeight() { return state.pictureHeight; }
    std::uint32_t& AviBackgroundEnabled() {
        return state.aviBackgroundEnabled;
    }
    IDirect3DTexture9*& CaptureTexture() {
        return reinterpret_cast<IDirect3DTexture9*&>(state.captureTexture);
    }
    IDirect3DSurface9*& CaptureRenderTarget() {
        return reinterpret_cast<IDirect3DSurface9*&>(state.captureRenderTarget);
    }
    IDirect3DSurface9*& CaptureSystemSurface() {
        return raw<IDirect3DSurface9*>(650124);  // original +0x9EB8C
    }
    ScreenCaptureMode& CaptureMode() {
        return raw<ScreenCaptureMode>(offsets::kDword9EB84);
    }
    void*& CaptureReadbackPixels() {
        return state.captureReadbackPixels;
    }
    std::uint8_t*& RecordingCompletionFlag() {
        return reinterpret_cast<std::uint8_t*&>(state.recordingCompletionFlag);
    }
    IDirect3DTexture9*& OverlayTexture() {
        return reinterpret_cast<IDirect3DTexture9*&>(state.overlayTexture);
    }
    IDirect3DVertexBuffer9*& SpriteOverlayVertices() {
        return raw<IDirect3DVertexBuffer9*>(offsets::kDword9EE0C);
    }
    std::uint32_t& SpriteOverlayPrimitiveCount() {
        return state.spriteOverlayPrimitiveCount;
    }
    IDirect3DTexture9*& ProjectedShadowRestoreTexture() {
        return raw<IDirect3DTexture9*>(offsets::kDword9F130);
    }
    std::uint8_t& ProjectedShadowBlendEnabled() {
        return state.projectedShadowBlendEnabled;
    }
    void*& ActiveRenderObject() {
        return raw<void*>(offsets::kDwordA0268);
    }
    AccessoryRenderPass& ActiveRenderPass() {
        return raw<AccessoryRenderPass>(offsets::kDwordA026C);
    }
    std::uint8_t& ModelOutlineRenderingSuppressed() {
        return state.modelOutlineRenderingSuppressed;
    }
    std::int32_t& ModelOutlineColorRed() {
        return state.modelOutlineColorRed;
    }
    std::int32_t& ModelOutlineColorGreen() {
        return state.modelOutlineColorGreen;
    }
    std::int32_t& ModelOutlineColorBlue() {
        return state.modelOutlineColorBlue;
    }
    std::uint8_t& WireframeRenderingEnabled() {
        return raw<std::uint8_t>(0xA01D4);
    }
    std::int32_t& AccessoryRenderSplitOrder() {
        // Generated field is uint32_t; preserve the signed accessor view.
        return reinterpret_cast<std::int32_t&>(state.accessoryRenderSplitOrder);
    }
    std::uint8_t& SelectedAccessorySlot() {
        return raw<std::uint8_t>(offsets::kByte9e170);
    }
    // The accessory and model-display lists share this historical selection
    // byte.  Use this neutral name outside accessory-specific code.
    std::uint8_t& SelectedObjectSlot() {
        return SelectedAccessorySlot();
    }
    std::uint8_t SelectedObjectSlot() const {
        return raw<std::uint8_t>(offsets::kByte9e170);
    }
    std::int32_t& DisplayObjectListScrollPosition() {
        return state.displayObjectListScrollPosition;
    }
    std::int32_t DisplayObjectListScrollPosition() const {
        return state.displayObjectListScrollPosition;
    }
    std::int32_t& DisplayObjectListMatchCount() {
        return state.displayObjectListMatchCount;
    }
    std::int32_t DisplayObjectListMatchCount() const {
        return state.displayObjectListMatchCount;
    }
    std::uint32_t& LastRegisteredFrame() {
        return state.lastRegisteredFrame;
    }
    IDirect3DVertexBuffer9*& OverlayVertices() {
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(state.overlayVertices);
    }
    std::uint32_t& TextOverlayPrimitiveCount() {
        return state.textOverlayPrimitiveCount;
    }
    IDirect3DTexture9*& TextOverlayTexture() {
        return reinterpret_cast<IDirect3DTexture9*&>(state.sceneFontTexture);
    }
    std::uint32_t& LineOverlayPrimitiveCount() {
        return state.lineOverlayPrimitiveCount;
    }
    IDirect3DVertexBuffer9*& GroundPlaneVertices() {
        return raw<IDirect3DVertexBuffer9*>(offsets::kDword9F128);
    }
    IDirect3DVertexBuffer9*& LeftViewportVertices() {
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(state.leftViewportVertices);
    }
    IDirect3DVertexBuffer9*& RightViewportVertices() {
        return reinterpret_cast<IDirect3DVertexBuffer9*&>(state.rightViewportVertices);
    }
    std::uint8_t& SelfShadowCompositionEnabled() {
        return state.selfShadowCompositionEnabled;
    }
    std::int32_t& SelfShadowMode() {
        return state.selfShadowMode;
    }
    std::uint8_t& DepthTextureCompositionEnabled() {
        return raw<std::uint8_t>(offsets::kByteA03DE);
    }
    std::uint8_t& DepthDeviceEnabled() {
        return state.depthDeviceEnabled;
    }
    DepthTextureProvider& DepthTextureCallback() {
        return reinterpret_cast<DepthTextureProvider&>(state.depthTextureCallback);
    }
    HDC& PanelDC() { return state.hdcMainPanel; }
    HDC& TimelineDC() { return state.hdcTimeline; }
    HDC& CurveDC() { return state.hdcInterpCurve; }
    HBITMAP& PanelBitmap() { return reinterpret_cast<HBITMAP&>(state.bmpPanel); }
    HBITMAP& PanelSpareBitmap() { return reinterpret_cast<HBITMAP&>(state.bmpPanelSpare); }
    HBITMAP& TimelineBitmap() { return reinterpret_cast<HBITMAP&>(state.bmpTimelineStrip); }
    HBITMAP& CurveBitmap() { return reinterpret_cast<HBITMAP&>(state.bmpInterpCurve); }
    HBRUSH UiBrush(int index) const {
        // brush handles are kept in 32-bit slots, as in the original state
        return reinterpret_cast<HBRUSH>(
            static_cast<std::uintptr_t>(state.brushes[index]));
    }
    void SetUiBrush(int index, HBRUSH brush) {
        state.brushes[index] =
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(brush));
    }
    COLORREF& ThemeColor(UiThemeColor color) {
        return reinterpret_cast<COLORREF&>(
            state.themeColors[static_cast<int>(color)]);
    }
    const COLORREF& ThemeColor(UiThemeColor color) const {
        return reinterpret_cast<const COLORREF&>(
            state.themeColors[static_cast<int>(color)]);
    }
    COLORREF& ThemeColorAt(int index) {
        return reinterpret_cast<COLORREF&>(state.themeColors[index]);
    }
    const COLORREF& ThemeColorAt(int index) const {
        return reinterpret_cast<const COLORREF&>(state.themeColors[index]);
    }
    std::uint8_t& UiTextRed() { return state.uiTextRed; }
    std::uint8_t& UiTextGreen() { return state.uiTextGreen; }
    std::uint8_t& UiTextBlue() { return state.uiTextBlue; }
    wchar_t* WavePath() {
        return reinterpret_cast<wchar_t*>(at(offsets::kWcsWavpath));
    }
    std::uint8_t& WaveEnabled() {
        return state.waveEnabled;
    }
    // AVI export options filled by the output dialog (0x40F2F0).
    wchar_t* AviOutputPath() {
#if defined(_M_X64)
        return m_aviOutputPath;
#else
        return state.aviOutputPath;
#endif
    }
    std::int32_t& AviRecordStartFrame() {
        return state.aviRecordStartFrame;
    }
    std::int32_t& AviRecordEndFrame() {
        return state.aviRecordEndFrame;
    }
    float& AviRecordFps() { return state.aviRecordFps; }
    std::uint8_t& AviIncludeWave() {
        return state.aviIncludeWave;
    }
    std::int32_t& AviCodecSelection() {
#if defined(_M_X64)
        return m_aviCodecSelection;
#else
        return state.aviCodecSelection;
#endif
    }
    std::uint8_t& AviStereoOutput() {
        return state.aviStereoOutput;
    }
    std::int32_t& AviStereoWidthMultiplier() {
        return state.aviStereoWidthMultiplier;
    }
    float& ProjectedShadowDiffuseAlpha() {
        return state.projectedShadowDiffuseAlpha;
    }
    float& ProjectedShadowAmbientIntensity() {
        return state.projectedShadowAmbientIntensity;
    }
    void SetProjectedShadowAmbientRgb(float intensity) {
        ProjectedShadowAmbientIntensity() = intensity;
        state.projectedShadowAmbientG = intensity;
        state.projectedShadowAmbientB = intensity;
    }
    void SetProjectedShadowAmbient(float intensity) {
        SetProjectedShadowAmbientRgb(intensity);
        state.projectedShadowAmbientA = intensity;
    }
    float& ProjectedShadowSpecularAlpha() {
        return state.projectedShadowSpecularAlpha;
    }
    D3DMATERIAL9 ProjectedShadowMaterial() const {
        D3DMATERIAL9 material{};
        material.Diffuse.a = state.projectedShadowDiffuseAlpha;
        material.Ambient = {state.projectedShadowAmbientIntensity,
                            state.projectedShadowAmbientG,
                            state.projectedShadowAmbientB,
                            state.projectedShadowAmbientA};
        material.Specular.a = state.projectedShadowSpecularAlpha;
        return material;
    }
    std::uint8_t& DirectSoundAvailable() {
        return state.directSoundAvailable;
    }
    std::int32_t& TimelineStartFrame() {
        return state.timelineStartFrame;
    }
    std::uint8_t& PlaybackActive() {
        return state.playbackActive;
    }
    std::uint8_t PlaybackActive() const {
        return state.playbackActive;
    }
    std::uint8_t& PlaybackLoopEnabled() {
        return state.playbackLoopEnabled;
    }
    std::uint8_t PlaybackLoopEnabled() const {
        return state.playbackLoopEnabled;
    }
    std::uint8_t& FrameStepPlayback() {
        return raw<std::uint8_t>(offsets::kByte9ED90);
    }
    std::uint8_t FrameStepPlayback() const {
        return raw<std::uint8_t>(offsets::kByte9ED90);
    }
    float& PlaybackStartSeconds() {
        return raw<float>(offsets::kFloatF9e654);
    }
    float& PlaybackCursorSeconds() {
        return raw<float>(offsets::kDwordF9e64c);
    }
    float& PlaybackEndSeconds() {
        return raw<float>(offsets::kFloatF9e658);
    }
    std::uint32_t& PlaybackClockAnchorLow() {
        return state.playbackClockAnchorLow;
    }
    std::uint32_t& PlaybackClockAnchorHigh() {
        return state.playbackClockAnchorHigh;
    }
    std::int32_t& SavedPlaybackPhysicsMode() {
        return state.savedPlaybackPhysicsMode;
    }
    std::uint8_t& PlaybackStartsAtCurrentFrame() {
        return raw<std::uint8_t>(offsets::kByte9ED99);
    }
    std::uint8_t& PlaybackFrameChanged() {
        return state.playbackFrameChanged;
    }
    std::uint8_t& PhysicsResetPending() {
        return state.physicsResetPending;
    }
    std::uint8_t PhysicsResetPending() const {
        return state.physicsResetPending;
    }
    std::int32_t& PlaybackPhysicsMode() {
        return state.playbackPhysicsMode;
    }
    std::int32_t& SidebarWidth() {
        return state.sidebarWidth;
    }
    std::int32_t& RenderWidth() { return raw<std::int32_t>(offsets::kDwordRenderw); }
    std::int32_t& RenderHeight() { return raw<std::int32_t>(offsets::kDwordRenderh); }
    std::int32_t& SeparateWindowSidebarWidth() {
        return raw<std::int32_t>(offsets::kDwordV658748);
    }
    std::int32_t& SeparateWindowX() { return state.separateWindowX; }
    std::int32_t& SeparateWindowY() { return state.separateWindowY; }
    std::int32_t& SeparateWindowWidth() { return state.separateWindowWidth; }
    std::int32_t& SeparateWindowHeight() { return state.separateWindowHeight; }
    std::uint8_t& SeparateWindowMaximized() {
        return state.separateWindowMaximized;
    }
    std::uint8_t& FrameVolumeControlEnabled() {
        return raw<std::uint8_t>(offsets::kByteFlag672800);
    }
    float& SidebarRatio() { return state.sidebarRatio; }
    std::int32_t& FrameNormalization() {
        return raw<std::int32_t>(offsets::kDwordVal672804);
    }
    RECT& ViewportRect() {
        return reinterpret_cast<RECT&>(state.hideRight);
    }
    const RECT& ViewportRect() const {
        return reinterpret_cast<const RECT&>(state.hideRight);
    }
    std::uint8_t& FullscreenMode() {
        return state.fullscreenMode;
    }
    std::uint32_t& MessageSeen() {
        return state.messageSeen;
    }
    std::uint8_t& WindowLayoutReady() {
        return raw<std::uint8_t>(offsets::kByteA442C);
    }
    std::uint8_t& EnhancedModelDirty() {
        return raw<std::uint8_t>(offsets::kByteA0B64);
    }
    std::uint8_t& AutoRepeatCount() {
        return raw<std::uint8_t>(offsets::kByteAutorep);
    }
    std::uint8_t& UiOptionFlag(int index) {
        return raw<std::uint8_t>(offsets::kByteOptflag0 + index);
    }
    std::uint8_t& CameraMode() { return UiOptionFlag(0); }
    std::uint32_t& ViewModeComboSelection() {
        return state.a042C;
    }
    std::uint8_t& GroundGridEnabled() { return raw<std::uint8_t>(797); }
    std::uint8_t& FpsOverlayEnabled() { return raw<std::uint8_t>(798); }
    float& FpsOverlayElapsedSeconds() { return raw<float>(800); }
    std::int32_t& FpsOverlayFrameCount() { return raw<std::int32_t>(804); }
    std::int32_t& FramesPerSecond() { return raw<std::int32_t>(808); }
    float& BoneRotationEditDegreesX() {
        return state.eulerX;
    }
    float& BoneRotationEditDegreesY() {
        return state.eulerY;
    }
    float& BoneRotationEditDegreesZ() {
        return state.eulerZ;
    }
    std::uint8_t& AudioSeekReady() {
        return raw<std::uint8_t>(offsets::kByteA02B6);
    }
    std::uint8_t& AutomaticFrameAdvanceEnabled() {
        return state.automaticFrameAdvanceEnabled;
    }
    WNDPROC& OriginalEditProc() {
        return reinterpret_cast<WNDPROC&>(state.origEditProc);
    }
    WNDPROC& OriginalTrackbarProc() {
        return reinterpret_cast<WNDPROC&>(state.origTrackProc);
    }
    std::uint32_t& CameraTrackCursor() { return raw<std::uint32_t>(648796); }
    std::uint8_t& CameraTrackActive() { return raw<std::uint8_t>(648800); }
    std::uint32_t& LightTrackCursor() { return raw<std::uint32_t>(648804); }
    std::uint8_t& LightTrackActive() { return raw<std::uint8_t>(648808); }
    std::uint32_t& ShadowTrackCursor() { return raw<std::uint32_t>(648812); }
    std::uint8_t& ShadowTrackActive() { return raw<std::uint8_t>(648816); }
    std::uint32_t& GravityTrackCursor() { return raw<std::uint32_t>(648820); }
    std::uint8_t& GravityTrackActive() { return raw<std::uint8_t>(648824); }
    std::uint32_t& AccessoryTrackCursor(int slot) {
        return raw<std::uint32_t>(648828 + 4 * slot);
    }
    std::uint8_t& AccessoryTrackActive(int slot) {
        return raw<std::uint8_t>(649848 + slot);
    }
    float* CameraPosition() { return &state.cameraPosX; }
    float* CameraRotation() { return &state.cameraPitch; }
    float& CameraPositionX() { return state.cameraPosX; }
    float& CameraPositionY() { return state.cameraPosY; }
    float& CameraPositionZ() { return state.cameraPosZ; }
    float& CameraPitch() { return state.cameraPitch; }
    float& CameraYaw() { return state.cameraYaw; }
    float& CameraRoll() { return state.cameraRoll; }
    float& ViewOffsetX() { return state.viewOffsetX; }
    float& ViewOffsetY() { return state.viewOffsetY; }
    float& CameraDistance() { return state.cameraDistance; }
    float& CameraFov() { return state.cameraFov; }
    std::uint8_t& CameraPerspective() {
        return state.cameraPerspective;
    }
    std::int32_t& CameraParentModel() {
        // Generated field is uint32_t; preserve the signed accessor view.
        return reinterpret_cast<std::int32_t&>(state.cameraParentModel);
    }
    std::int32_t& CameraParentBone() {
        return state.cameraParentBone;
    }
    CameraAttachmentReference& CameraReferenceMode() {
        return reinterpret_cast<CameraAttachmentReference&>(
            state.cameraReferenceMode);
    }
    D3DMATRIX& CameraAttachmentBasis() {
        return reinterpret_cast<D3DMATRIX&>(state.cameraAttachmentBasis);
    }
    const D3DMATRIX& CameraAttachmentBasis() const {
        return reinterpret_cast<const D3DMATRIX&>(state.cameraAttachmentBasis);
    }
    std::uint8_t& CameraAttachmentTransformSuppressed() {
        return raw<std::uint8_t>(offsets::kByteA0478);
    }
    D3DMATRIX& ViewRotationTransform() {
        return raw<D3DMATRIX>(0xA0674);
    }
    const D3DMATRIX& ViewRotationTransform() const {
        return raw<D3DMATRIX>(0xA0674);
    }
    std::int32_t& ShadowMode() {
        return state.selfShadowMode;
    }
    float& ShadowDistance() {
        return state.physicsInterval;
    }
    float* GravityDirection() { return &GravityX(); }
    std::int32_t& GravityNoise() {
        return reinterpret_cast<std::int32_t&>(state.gravityNoise);
    }
    std::uint8_t& GravityNoiseEnabled() {
        return state.a0CD4;
    }

    // -- named fields (semantic names verified so far) -------------------
    // Main loop timing cluster (WinMain 0x004C4460)
    float& FpsLimit()               { return state.fpsLimit; }   // 657632
    float& DeltaTime()              { return raw<float>(offsets::kFloatDeltatime); }  // 657084
    std::uint32_t& TimeNowLow()     { return raw<std::uint32_t>(offsets::kDwordTimenowlo); }
    std::uint32_t& TimeNowHigh()    { return raw<std::uint32_t>(offsets::kDwordTimenowhi); }
    float& MilliToSec()             { return state.milliToSec; }  // 0.001

    // Environment / startup scene file (wchar_t[256] @ 657664)
    wchar_t* EnvFileName()          { return reinterpret_cast<wchar_t*>(storage() + offsets::kWcsEnvfile); }

    // Locale/font subsystem pointer consumed by ConvertAnsiToWide (0x00407A70)
    void*& LocaleTablePtr()         { return raw<void*>(offsets::kPtrSub1d574); }    // 657092

    // Main window (0x0047A5B0)
    void*& Hwnd()                   { return reinterpret_cast<void*&>(state.hwnd); }        // 657080
    void*& HInstance()              { return raw<void*>(0); }                        // this+0
    // Render/locale subsystem ("0x1D574 object"), allocated in
    // InitMainWindowAndD3D; layout restored in d3d_wrapper.hpp.
    D3DRenderer*& Renderer()        { return raw<D3DRenderer*>(offsets::kPtrSub1d574); }
    WaveAudioContext*& Audio() {
        return raw<WaveAudioContext*>(offsets::kPtrSub025c);
    }
    void*& Sub025C() { return reinterpret_cast<void*&>(Audio()); }
    DShowRecorder*& Recorder() {
        return raw<DShowRecorder*>(offsets::kPtrSub06c);
    }
    void*& Sub06C() { return reinterpret_cast<void*&>(Recorder()); }
    void*& Sub04B0()                { return raw<void*>(offsets::kPtrSub04b0); }     // 0x4B0 obj
    // Physics scene wrapper ("0x48 object"), allocated in
    // InitMainWindowAndD3D, filled by SceneConstruct; see physics_scene.hpp.
    PhysicsScene*& Physics()        { return raw<PhysicsScene*>(offsets::kPtrSub048); }
    wchar_t* ExeDir()               { return reinterpret_cast<wchar_t*>(storage() + offsets::kWcsExedir); }
    unsigned char& EnglishUI()      { return state.englishUI; }  // 658252

    // User directory names (wchar_t[1000] each, 0x0047A5B0)
    wchar_t* DirModel()   { return state.dirModel; }
    wchar_t* DirUser()    { return state.dirUser; }
    wchar_t* DirAccs()    { return state.dirAccs; }
    wchar_t* DirMotion()  { return state.dirMotion; }
    wchar_t* DirPose()    { return state.dirPose; }
    wchar_t* DirWave()    { return state.dirWave; }
    wchar_t* DirBg()      { return state.dirBg; }

    // Physics gravity (defaults 0.0 / -1.0 / 0.0, magnitude 9.8)
    float& GravityX()               { return state.gravityX; }
    float& GravityY()               { return state.gravityY; }
    float& GravityZ()               { return state.gravityZ; }
    float& GravityMagnitude()       { return state.gravityMagnitude; }
    float& PhysicsInterval()        { return state.physicsInterval; }  // 0.01125

    // Recent-file ANSI buffers (char[256] each)
    char* RecentFile(int index) {
        return index == 0 ? reinterpret_cast<char*>(state.recentFile0)
             : index == 1 ? reinterpret_cast<char*>(state.recentFile1)
                          : reinterpret_cast<char*>(state.recentFile2);
    }

    unsigned char* storage() { return reinterpret_cast<unsigned char*>(&state); }
    const unsigned char* storage() const {
        return reinterpret_cast<const unsigned char*>(&state);
    }

private:

#if defined(_M_X64)
    // In the original x86 blob, 0x9E180 is a D3DLIGHT9 overlay spanning
    // several scalar mirrors.  The provisional x64 compatibility layout
    // represented those mirrors independently, so an in-blob D3DLIGHT9
    // would overlap unrelated fields.  Keep this transient device object
    // outside the serialized blob; LightDirection/LightColor remain the
    // PMM-facing scalar state and ApplyTimelineLightState synchronizes both.
    D3DLIGHT9 m_sceneLight{};

    // This scratch workspace is 3,536 bytes in the original x86 state.  The
    // provisional x64 blob reserves only 3,240 bytes before the next live
    // field, so retaining it in the blob lets resolvedPath overwrite the
    // viewport vertex-buffer slots.  It is process-local path scratch data,
    // never PMM state, and therefore belongs beside the x64 runtime objects.
    PathResolutionWorkspace m_pathWorkspace{};

    // The x86 application state stores each overlay-buffer pointer in one
    // 32-bit slot.  Its following scalar fields are adjacent in the PMM
    // layout, so widening either slot in-place would overlap data PMM reads
    // and writes (notably picture X offset at 0x9E434).  These are transient
    // D3D resources, not project state; keep their x64 ownership outside the
    // serialized compatibility blob.
    struct OverlayVertexBuffers {
        IDirect3DVertexBuffer9* avi = nullptr;
        IDirect3DVertexBuffer9* picture = nullptr;
    } m_overlayVertexBuffers;

    // The x86 blob holds the AVI output path (wchar_t[256] at 0x9EE80) and
    // the codec selection (int at 0xA0CD8) inline.  The provisional x64
    // blob reserves less room before the next live field in both regions,
    // so these dialog-local values live outside the compat blob on x64.
    wchar_t m_aviOutputPath[256]{};
    std::int32_t m_aviCodecSelection{};
    HWND m_hwndA0A6C = nullptr;
    HWND m_frameRangeDialog = nullptr;
#endif
};

static_assert(sizeof(MMDApp) >= sizeof(MMDAppState),
              "MMDApp must retain the complete compatibility state");
// size truth (exact x86 / bounded x64) is pinned inside app_layout.hpp

// The single instance - mirrors the `Block` global at VA 0x0054593C.
extern MMDApp* g_Block;

}  // namespace mikudancestudio
