// ===========================================================================
// Command implementations (0x0047E8A0 cases; dialog chains restored)
// ===========================================================================
// The file-menu commands share one pattern (verified across the 19 recovered
// cases): SetCurrentDirectoryW(UserFile dir) -> GetOpenFileNameW/GetSaveFileNameW
// with the original filter/title strings -> ExtractDirFromPath into the
// corresponding directory buffer -> invoke the loader (stubbed, VA recorded)
// -> dirty flag.  Loaders themselves remain ported-stubs this phase.
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>

#include <cwchar>
#include <cstring>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/ported_funcs.hpp"

namespace mikudancestudio {
namespace {

struct FileDialogSpec {
    const wchar_t* filter;      // embedded double-NUL terminated
    const wchar_t* titleEn;
    MMDApp* (MMDApp::*dirBuf)();  // not used - see accessor below
};

HWND MainHwnd(MMDApp* app) {
    return static_cast<HWND>(app->Hwnd());
}

bool OpenDialog(MMDApp* app, const wchar_t* filter, const wchar_t* title,
                wchar_t* path, DWORD pathLen) {
    OPENFILENAMEW ofn;
    std::memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = MainHwnd(app);
    ofn.lpstrFilter = filter;
    ofn.lpstrTitle = title;
    ofn.lpstrFile = path;
    ofn.nMaxFile = pathLen;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    return GetOpenFileNameW(&ofn) != FALSE;
}

bool SaveDialog(MMDApp* app, const wchar_t* filter, const wchar_t* title,
                wchar_t* path, DWORD pathLen) {
    OPENFILENAMEW ofn;
    std::memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = MainHwnd(app);
    ofn.lpstrFilter = filter;
    ofn.lpstrTitle = title;
    ofn.lpstrFile = path;
    ofn.nMaxFile = pathLen;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    return GetSaveFileNameW(&ofn) != FALSE;
}

template <typename Key>
bool AnyAllocatedKey(const Key* keys, std::size_t count) {
    if (keys == nullptr)
        return false;
    for (std::size_t i = 0; i < count; ++i) {
        if (keys[i].allocated != 0)
            return true;
    }
    return false;
}

void MarkDirty(MMDApp* app) {
    app->SceneModified() = 1;
}

}  // namespace

// 0xCA - File: load VPD pose (filter/title from rdata 0x52C6xx region)
void CmdLoadPose(MMDApp* app) {
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[MAX_PATH] = L"";
    const bool english = app->EnglishUI() != 0;
    if (OpenDialog(app,
                   L"Vocaloid Pose Data files(*.vpd)\0*.vpd\0\0",
                   english ? L"load pose data" : L"\x30dd\x30fc\x30ba\x30c7\x30fc\x30bf\x3092\x8aad\x307f\x8fbc\x3080",
                   path, MAX_PATH)) {
        wchar_t dir[1000];
        wcscpy_s(app->DirPose(), 0x3E8, ExtractDirFromPath(dir, path));
        LoadVpdFile(path);                                        // 0x418A10
        MarkDirty(app);
    }
}

// 0xCB - File: save VPD pose (only when bones are selected)
void CmdSavePose(MMDApp* app) {
    // Bone-selection check on the active model's selection bitmap.
    const bool english = app->EnglishUI() != 0;
    bool anySelected = false;
    unsigned char* const model = app->SelectedModel();
    if (model != nullptr) {
        const int count = static_cast<int>(mdl::Mdl(model)->boneCount);
        unsigned char* const selBits = mdl::Mdl(model)->boneSelection;
        if (selBits != nullptr)
            for (int b = 0; b < count; ++b)
                if (selBits[b] != 0) { anySelected = true; break; }
    }
    if (!anySelected) {
        MessageBoxA(MainHwnd(app),
            english ? "Pose data will be saved only selected bone.\nPlease select bone."
                    : "(JP 0x53178C region)",
            "save pose data", MB_OK | MB_ICONWARNING);
        return;
    }
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[MAX_PATH] = L"";
    if (SaveDialog(app,
                   L"Vocaloid Pose Data files(*.vpd)\0*.vpd\0\0",
                   app->EnglishUI() ? L"save pose data"
                                    : L"\x30dd\x30fc\x30ba\x30c7\x30fc\x30bf\x3092\x95\xdb\x91\xb6",
                   path, MAX_PATH)) {
        wchar_t dir[1000];
        wcscpy_s(app->DirPose(), 0x3E8, ExtractDirFromPath(dir, path));
        SaveVpdFile(path);                                        // 0x418750
        MarkDirty(app);
    }
}

// 0xCC - Edit: reset state after confirmation
void CmdResetState(MMDApp* app) {
    const bool english = app->EnglishUI() != 0;
    int r = MessageBoxA(MainHwnd(app),
        english ? "The existing state will be annulled.\n\nAre you OK?"
                : "(JP 0x5317D0 region)",
        "Reset", MB_OKCANCEL | MB_ICONQUESTION);
    if (r == IDOK)
        ResetAppState(app);                                       // 0x44E540
}

// 0xCD - File: open PMM scene
void CmdOpenScene(MMDApp* app) {
    if (app->SceneModified() != 0) {
        int r = MessageBoxA(MainHwnd(app),
            app->EnglishUI()
                ? "There is a change point not preserved.\n\nAre you OK?"
                : "(JP)",
            "Open File", MB_OKCANCEL | MB_ICONWARNING);
        if (r != IDOK)
            return;
    }
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[MAX_PATH] = L"";
    if (OpenDialog(app,
                   L"PolygonMovieMaker files(*.pmm)\0*.pmm\0\0",
                   app->EnglishUI() ? L"open file data"
                                    : L"\x30d5\x30a1\x30a4\x30eb\x30c7\x30fc\x30bf\x3092\x8a4a\x304f",
                   path, MAX_PATH)) {
        wchar_t dir[1000];
        wcscpy_s(app->DirUser(), 0x3E8, ExtractDirFromPath(dir, path));
        wcscpy_s(app->EnvFileName(), 0x100, path);
        LoadSceneFile();                                          // 0x458F80
        MarkDirty(app);
    }
}

// 0xCE - File: open WAV
void CmdOpenWave(MMDApp* app) {
    app->state.bC = 1;   // 0x487754 [0x2F]=1
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[MAX_PATH] = L"";
    if (OpenDialog(app,
                   L"Wave files(*.wav)\0*.wav\0\0",
                   app->EnglishUI() ? L"open WAVE File"
                                    : L"WAVE File \x3092\x8a4a\x304f",
                   path, MAX_PATH)) {
        wchar_t dir[1000];
        wcscpy_s(app->DirWave(), 0x3E8, ExtractDirFromPath(dir, path));
        CopyPathW(reinterpret_cast<wchar_t*>(
                      app->state.wavPath),
                  path);                                  // 0x42AE40 -> app+0xD0
        LoadWaveFile(app);                                       // 0x418500
        MarkDirty(app);
    }
}

// 0xCF / 0xD3 - Play: play/pause toggle
void CmdPlayPause(MMDApp* app) {
    reinterpret_cast<unsigned char&>(app->state.physicsInterval) ^= 1;  // 658348
}

// 0xD0 - File: save PMM scene
void CmdSaveScene(MMDApp* app) {
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[MAX_PATH] = L"";
    if (SaveDialog(app,
                   L"PolygonMovieMaker files(*.pmm)\0*.pmm\0\0",
                   app->EnglishUI() ? L"save file data" : L"(JP)",
                   path, MAX_PATH)) {
        wchar_t dir[1000];
        wcscpy_s(app->DirUser(), 0x3E8, ExtractDirFromPath(dir, path));
        wcscpy_s(app->EnvFileName(), 0x100, path);   // dialog path -> 0xA0900
        SaveSceneFile(app);                                      // 0x41B080
        app->SceneModified() = 0;
    }
}

// 0xD1 - File: load VMD motion
void CmdLoadMotion(MMDApp* app) {
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[MAX_PATH] = L"";
    if (OpenDialog(app,
                   L"Vocaloid Motion Data files(*.vmd)\0*.vmd\0\0",
                   app->EnglishUI() ? L"load motion data" : L"(JP)",
                   path, MAX_PATH)) {
        wchar_t dir[1000];
        wcscpy_s(app->DirMotion(), 0x3E8, ExtractDirFromPath(dir, path));
        LoadVmdFile(path);                                        // 0x434B60
        MarkDirty(app);
    }
}

// 0xD2 - File: save VMD motion
void CmdSaveMotion(MMDApp* app) {
    // 0x487BF0..0x487D11: any-selected-frame-point gate.  Scans the app
    // camera (0x374/+0x48, stride 0x54) and light (0x378/+0x24, stride
    // 0x28) tables (cap 10000 each), the shadow table (0x37C/+0x14,
    // stride 0x18 - a hit there proceeds directly), and the active
    // model's bone (0x26E0/+0x38, stride 0x3C, cap 300000), morph
    // (0x26E4/+0x10, stride 0x14, cap 0x4E20 = 20000 - NOT 300000;
    // both builds keep allocated-but-unselected garbage past 20000) and
    // IK (0x26E8/+0x14, stride 0x1C, cap 1000) selection tables.
    // Without any selection the original shows the notice and returns
    // (0x487CC6).
    auto anySelectedByte = [](unsigned char* table, std::size_t off,
                              std::size_t stride, int cap) -> bool {
        if (table == nullptr)
            return false;
        for (int i = 0; i < cap; ++i)
            if (table[off + stride * i] != 0)
                return true;
        return false;
    };
    bool any = false;
    if (app->raw<std::uint8_t>(0x2F8) != 0) {
        // 0x487BE8 jz: with the camera/light/shadow mode flag set, only
        // the app tables are scanned; the shadow table falling through
        // with no hit jumps straight to the test (0x487C4B ->
        // 0x487CC2) - the model tables are NEVER scanned in this mode.
        any = anySelectedByte(reinterpret_cast<unsigned char*>(
                                  app->raw<void*>(0x374)), 0x48, 0x54, 10000) ||
            anySelectedByte(reinterpret_cast<unsigned char*>(
                                app->raw<void*>(0x378)), 0x24, 0x28, 10000) ||
            anySelectedByte(reinterpret_cast<unsigned char*>(
                                app->raw<void*>(0x37C)), 0x14, 0x18, 10000);
    } else {
        unsigned char* const model = app->SelectedModel();
        if (model != nullptr) {
            any = AnyAllocatedKey(mdl::BoneKeys(model),
                                  mdl::kBoneKeyCapacity) ||
                AnyAllocatedKey(mdl::MorphKeys(model),
                                mdl::kMorphKeyCapacity) ||
                AnyAllocatedKey(mdl::DisplayKeys(model),
                                mdl::kDisplayKeyCapacity);
        }
    }
    if (!any) {
        MessageBoxA(MainHwnd(app),
            app->EnglishUI() != 0
                ? "Motion data will be saved only selected frame point.\n"
                  "Please select frame point."
                : "\x83\x82\x81[\x83V\x83\x87\x83\x93\x83" "f\x81[\x83^"
                  "\x82\xcd\x91I\x91\xf0\x82\xb3\x82\xea\x82\xbd"
                  "\x83t\x83\x8c\x81[\x83\x80\x83|\x83" "C\x83\x93\x83g"
                  "\x82\xaa\x83Z\x81[\x83u\x82\xb3\x82\xea\x82\xdc"
                  "\x82\xb7\n",
            "save motion data", 0x40000);
        return;
    }
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[MAX_PATH] = L"";
    if (SaveDialog(app,
                   L"Vocaloid Motion Data files(*.vmd)\0*.vmd\0\0",
                   app->EnglishUI() ? L"save motion data" : L"(JP)",
                   path, MAX_PATH)) {
        wchar_t dir[1000];
        wcscpy_s(app->DirMotion(), 0x3E8, ExtractDirFromPath(dir, path));
        SaveVmdFile(path);                                        // 0x419370
        MarkDirty(app);
    }
}

// 0xD5 - File: load AVI background
void CmdLoadAvi(MMDApp* app) {
    app->raw<std::uint32_t>(0x74) = 1;                // 0x4870B4 [0x1D]=1
    app->state.bC = 1;   // 0x4870B7 [0x2F]=1
    SetCurrentDirectoryW(app->ExeDir());
    wchar_t path[MAX_PATH] = L"";
    if (OpenDialog(app,
                   L"avi files(*.avi)\0*.avi\0\0",
                   app->EnglishUI() ? L"open AVI File" : L"(JP)",
                   path, MAX_PATH)) {
        wchar_t dir[1000];
        wcscpy_s(app->DirBg(), 0x3E8, ExtractDirFromPath(dir, path));
        CopyPathW(reinterpret_cast<wchar_t*>(
                      app->at(offsets::kWcs9e1ec)),
                  path);                                  // 0x42AE40 -> app+0x9E1EC
        LoadAviFile(app);                                        // 0x433250
        MarkDirty(app);
    }
}

// 0xD6/0xD7/0xD8 - display toggles (menu check state in the original)
void CmdToggleBoneDisplay(MMDApp* app) {
    app->state.v9ed98 ^= 1;            // 650136
}
void CmdToggleMorphDisplay(MMDApp* app) {
    app->state.playbackStartsAtCurrentFrame ^= 1;            // 650137
}
void CmdTogglePhysicsDisplay(MMDApp* app) {
    app->state.projectedShadowBlendEnabled ^= 1;            // 650138
}

// 0xD9/0xDA/0xDC - select-all frame groups
void CmdSelectAllBoneFrames(MMDApp* app)  { SelectFrameGroup(app, 0); }
void CmdSelectAllEyeFrames(MMDApp* app)   { SelectFrameGroup(app, 1); }
void CmdSelectAllLipFrames(MMDApp* app)   { SelectFrameGroup(app, 2); }

}  // namespace mikudancestudio
