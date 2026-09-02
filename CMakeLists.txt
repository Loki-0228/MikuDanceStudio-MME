cmake_minimum_required(VERSION 3.21)

project(MikuDanceStudio LANGUAGES CXX)

# ---------------------------------------------------------------------------
# MikuDanceStudio: behavioural restoration of MikuMikuDance v932.
# MikuMikuDanceE_v932x64 is the x64 reference.  The x86 binary remains a
# source-recovery aid only; see docs/X64_RECONSTRUCTION.md for the evidence
# and layout rules.  The porting ledger lives in docs/PORTING_STATUS.md.
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W3 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    # The original binary tolerates implicit narrowing everywhere; keep the
    # port close to VC9 behaviour rather than modern strictness.
    add_compile_options(/wd4267 /wd4244 /wd4245)
    # VC9 had no C++17 aligned operator new/delete (P0035).  Without this
    # switch our TUs allocate over-aligned Bullet objects (e.g. the 1456-byte
    # btGeneric6DofSpringConstraint, align 16) through _aligned_malloc while
    # the bullet275 static library's deleting destructors free them with the
    # plain operator delete - free() on an _aligned_malloc block, which
    # corrupts the heap and kills the process at teardown (0xC0000374,
    # see ShutdownCleanup -> DisposePhysicsWorld).  The original shipped
    # exactly the plain-new pairing, so this is also the faithful setting.
    add_compile_options(/Zc:alignedNew-)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS
        "Configuring the x64 build (reference: MikuMikuDanceE_v932x64, "
        "VC9 SSE2, d3dx9_43).  Fields are admitted only through the x64 "
        "evidence and parity contract in docs/X64_RECONSTRUCTION.md.")
endif()

if (NOT WIN32)
    message(FATAL_ERROR "MikuDanceStudio targets Windows only (Win32/DirectX9).")
endif()

# ---------------------------------------------------------------------------
# Dependencies (provisioned by Conan; see conanfile.py / recipes/)
# ---------------------------------------------------------------------------
find_package(BULLET275 REQUIRED)

# ---------------------------------------------------------------------------
# Core static library: application state, math, startup chain, effect API.
# ---------------------------------------------------------------------------
add_library(mikudancestudio_core STATIC
    src/app/mmd_app.cpp
    src/app/globals.cpp
    src/app/app_ctor.cpp
    src/app/app_defaults.cpp
    src/main/charset_conv.cpp
    src/math/quat_to_mat3x4.cpp
    src/math/color_lerp.cpp
    src/math/kernel_gaps.cpp
    src/window/path_utils.cpp
    src/window/window_init.cpp
    src/window/ui_init.cpp
    src/window/ui_font.cpp
    src/window/wndproc.cpp
    src/window/localize_ui.cpp
    src/window/wndproc_stubs.cpp
    src/window/command_dispatch.cpp
    src/window/frame_line_edit.cpp
    src/window/accessory_paste.cpp
    src/media/wave_audio.cpp
    src/media/media_load.cpp
    src/window/command_impl.cpp
    src/app/frame_driver.cpp
    src/app/playback_catchup.cpp
    src/app/playback_state.cpp
    src/app/timeline_advance.cpp
    src/app/frame_modes.cpp
    src/app/frame_modes_bone.cpp
    src/app/key_ladder.cpp
    src/physics/physics_create.cpp
    src/physics/physics_frame.cpp
    src/physics/scene_create.cpp
    src/render/d3d_init.cpp
    src/render/render_states.cpp
    src/render/stereo_nvapi.cpp
    src/render/frame_scene.cpp
    src/render/sprite_overlay.cpp
    src/render/overlay_producers.cpp
    src/render/capture_downsample.cpp
    src/render/debug_geometry.cpp
    src/render/accessory.cpp
    src/render/model_renderers.cpp
    src/render/bg_overlay.cpp
    src/render/device_reset.cpp
    src/app/avi_record_start.cpp
    src/app/record_readback.cpp
    src/app/dshow_record_graph.cpp
    src/model/model_init.cpp
    src/model/path_resolve.cpp
    src/model/pmd_load.cpp
    src/model/bone_transform.cpp
    src/model/model_keyframe_advance.cpp
    src/model/model_keyframe_edit.cpp
    src/model/bone_edit_undo.cpp
    src/model/model_frame_seek.cpp
    src/model/vmd_load.cpp
    src/model/track_apply.cpp
    src/model/key_registrars.cpp
    src/model/morph_apply.cpp
    src/model/model_skinning.cpp
    src/model/pmx_load.cpp
    src/model/add_model.cpp
    src/model/model_query_gaps.cpp
    src/model/model_dispose.cpp
    src/model/bone_sort.cpp
    src/model/post_load_init.cpp
    src/render/fill_panel.cpp
    src/window/wm_paint.cpp
    src/window/ui_timeline_gfx.cpp
    src/window/ui_control_text.cpp
    src/window/ui_ctlcolor.cpp
    src/window/ui_edit_commit.cpp
    src/window/dialog_procs.cpp
    src/window/frame_range_apply.cpp
    src/io/pmm_save.cpp
    src/io/io_gaps.cpp
    src/io/vpd_file.cpp
    src/io/vmd_save.cpp
    src/io/pmm_load_shell.cpp
    src/io/pmm_load_v2.cpp
    src/io/pmm_load_v1.cpp
    src/window/ui_scroll_mouse.cpp
    src/window/ui_panel_paint.cpp
    src/window/ui_refresh.cpp
    src/window/ui_notify.cpp
    src/window/ui_hscroll.cpp
    src/window/ui_dropfiles.cpp
    src/window/ui_panel_sweep.cpp
      src/window/ui_view_refresh.cpp
      src/window/ui_model_reload.cpp
      src/window/ui_selection_reeval.cpp
    src/window/ui_editor_click.cpp
    src/window/ui_frame_step.cpp
    src/window/ui_frame_refresh.cpp
    src/window/ui_mousemove.cpp
    src/window/ui_windowsize.cpp
    src/window/mic_window.cpp
    src/app/subsystem_init.cpp
    src/app/reset_app_state.cpp
    src/app/scene_ownership.cpp
    src/app/app_gap_bodies.cpp
    src/app/shutdown_cleanup.cpp
    src/app/oni_kinect.cpp
    src/io/vsq_load.cpp
    src/window/dialog_select_ops.cpp
    src/window/ui_viewport_refresh.cpp
    src/window/ui_viewport_layout.cpp
    src/window/ui_mouse_misc.cpp
    src/window/ui_palette.cpp
    src/window/command_file_menu.cpp
    src/window/command_view_menu.cpp
    src/window/physics_model_dialog.cpp
    src/window/model_edge_dialog.cpp
    src/window/enhance_model_io.cpp
    src/window/misc_dialogs.cpp
    src/window/command_control_400.cpp
    src/window/command_control_450.cpp
    src/window/command_control_500.cpp
    src/features/dialog_gaps.cpp
    src/features/query_audio_gaps.cpp
    src/features/subsystem_gaps.cpp
    src/render/draw_glyph.cpp
    src/render/toon_textures.cpp
    src/render/scene_font.cpp
    src/exports/effect_api.cpp
    src/unported/stubs.cpp
)

target_include_directories(mikudancestudio_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(mikudancestudio_core PUBLIC
    BULLET275::BULLET275
    user32      # RegisterClass / CreateWindow / MessageBox
    gdi32       # CreateSolidBrush / GetStockObject
    msimg32     # GradientFill (0x40DF10 panel fill)
    comctl32    # InitCommonControls
    shell32     # DragAcceptFiles
    d3d9        # IDirect3DDevice9 (device slot at sub1d574+120032)
    vfw32       # DrawDibDraw (0x4168D0) + AVIFile*/AVIStream* (0x433250 background AVI)
    avifil32    # AVIFile/AVIStream background playback (0x4168D0)
    ole32       # CoInitialize/CoUninitialize (0x467313 / 0x4096C0)
    strmiids    # DirectShow IIDs/CLSIDs (recording graph 0x408F20..0x409A80)
    uuid        # IID_IPropertyBag / IID_IAMVfwCompressDialogs etc.
)

# ---------------------------------------------------------------------------
# Main executable.  WIN32 subsystem matches the original PE header
# (Subsystem 2, GUI).  Exports (Exp*) come from exports/MikuMikuDance.def
# exactly like the original export table at RVA 0x00141328.
# ---------------------------------------------------------------------------
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # x64 build embeds the original x64 manifest (amd64 dependencies +
    # Data\MMDxShow.dll reg-free COM) - the x86 manifest's x86 assemblies
    # make the 64-bit loader fail with STATUS_INVALID_IMAGE_FORMAT.
    set(MIKUDANCESTUDIO_GEN_RES res/gen_resources_x64.res)
else()
    set(MIKUDANCESTUDIO_GEN_RES res/gen_resources.res)
endif()
add_executable(MikuDanceStudio WIN32
    src/main/winmain.cpp
    res/MMD.rc
    ${MIKUDANCESTUDIO_GEN_RES}
    ${CMAKE_CURRENT_SOURCE_DIR}/exports/MikuMikuDance.def
)

target_link_libraries(MikuDanceStudio PRIVATE
    mikudancestudio_core
    winmm      # timeGetTime - main loop timing
    dsound     # original timeline/audio subsystem initialization (0x4C2470)
    user32
)

# gen_resources.res re-embeds the original .rsrc byte-exact; link.exe accepts
# .res directly as an input file (CMake itself would classify it as <None>).
# /MANIFEST:NO keeps the linker from adding a default manifest on top of the
# original one embedded through the .res.
target_link_options(MikuDanceStudio PRIVATE
    "/MAP:${CMAKE_CURRENT_BINARY_DIR}/MikuMikuDanceE.map"
    "/MANIFEST:NO"
    "${CMAKE_CURRENT_SOURCE_DIR}/${MIKUDANCESTUDIO_GEN_RES}"
)
set_target_properties(MikuDanceStudio PROPERTIES
    OUTPUT_NAME "MikuMikuDanceE"
    # Export the 37 Exp* symbols from the .def file (ordinals 1..37).
    ENABLE_EXPORTS ON
)

# ---------------------------------------------------------------------------
# MMDxShow.dll - the companion DirectShow push-source filter (original at
# ../MikuMikuDanceE_v932/Data/MMDxShow.dll, VC8 x86).  Self-registration
# entry points match the original export table (ordinals 1..5).
# ---------------------------------------------------------------------------
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    # The layout static_asserts pin the VC8 x86 binary layout; the x64
    # companion DLL is a separate port (the original x64 exe ships its own
    # MMDxShow.dll flavour) and is not built here yet.
    add_library(MMDxShow SHARED
        src/mmdxshow/dll_main.cpp
        src/mmdxshow/enumerators.cpp
        src/mmdxshow/source_base.cpp
        src/mmdxshow/push_source.cpp
        src/mmdxshow/push_pin.cpp
        src/mmdxshow/guids.cpp
        src/mmdxshow/crt_ledger.cpp
    )
    target_include_directories(MMDxShow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/mmdxshow)
    target_link_libraries(MMDxShow PRIVATE
        kernel32 advapi32 gdi32 user32 ole32 uuid
    )
    target_link_options(MMDxShow PRIVATE
        "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/mmdxshow/MMDxShow.def")
    set_target_properties(MMDxShow PROPERTIES
        OUTPUT_NAME "MMDxShow"
    )
endif()
