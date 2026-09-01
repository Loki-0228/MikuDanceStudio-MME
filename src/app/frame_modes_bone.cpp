// ===========================================================================
// Bone-drag stage of the frame driver (VA region 0x475A6E..0x477220)
// ===========================================================================
// When a drag is active and a model/bone is selected, modes 1..6 of the
// SECOND-stage mode field (app+844; the first-stage dispatch at 0x46B121
// reads app+836) are reinterpreted as bone rotation:
//   mode 1 : rotate around the camera-space X axis
//   mode 2 : around the camera-space Y axis
//   mode 3 : view-plane rotation - the delta of atan2(dx,dy) between the
//            drag origin->previous and origin->current mouse vectors,
//            wrapped to +-6.28 rad
//   modes 4/5/6 : around the selected bone's local axes (columns of the
//            bone matrix at +0xB4 in "direct" mode app+650076==1, else the
//            basis built by 0x40E670)
// The axis is transformed into the bone's frame by the rows of the bone
// matrix (+0xB4/+0xC4/+0xD4) and a delta quaternion (sin/cos of the scaled
// drag angle; 0x476D6D/0x476DA3 contain no additional half-angle multiply)
// is post-multiplied onto the bone's
// rotation quaternion at +0x14C via D3DXQuaternionMultiply, after setting
// the per-bone dirty flag (model+11672 array).
//
// Scale chain (doubles in .data, verified from the instruction stream):
//   base angle = mouse delta * 0.005   (dbl 0x52E9C0)
//   selector A (app+36==3)  -> *5.0    (dbl 0x52A270, coarse)
//   selector B (app+192==3) -> *0.1    (dbl 0x52BEA8, fine)
//   mode 3 wrap constants +-6.28       (dbl 0x52E8E0 / 0x52E8E8)
//   accessory/light drags   *0.01      (dbl 0x52E9C8)
//
// When app+657524 != 0 the same modes edit records instead of the bone:
//   accessory (app+650420==0): records at app+657532, stride 0xAC, fields
//     +0x40/+0x44/+0x48, "%3.2f" echo to edits 715/716/717 (0x2CB..0x2CD);
//   otherwise: records at app+657968, stride 0x8C, fields +0x30/+0x34/+0x38,
//     echo to edits 754/755/756 (0x2F2..0x2F4).
// The multi-selection propagation loop is ported below from
// 0x476E42..0x477257, including its selected-root filter.
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "mikudancestudio/d3dx_dyn.hpp"
#include "mikudancestudio/globals.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/ported_funcs.hpp"

namespace mikudancestudio {
namespace {

constexpr double kAngBase = 0.004999999888241291; // dbl 0x52E9C0
constexpr double kCoarse = 5.0;       // dbl 0x52A270
constexpr double kFine = 0.10000000149011612; // dbl 0x52BEA8 (0.1f image)
constexpr double kWrap = 6.28;        // dbl 0x52E8E8 (+) / 0x52E8E0 (-)
constexpr double kAccDrag = 0.01;     // dbl 0x52E9C8

float OriginalSinTimes(float angle, float factor) {
#if defined(_M_IX86)
    float sine;
    float result;
    __asm {
        fld angle
        fsin
        fstp sine
        fld sine
        fmul factor
        fstp result
    }
    return result;
#else
    return static_cast<float>(std::sin(angle) * factor);
#endif
}

float OriginalCos(float angle) {
#if defined(_M_IX86)
    float result;
    __asm {
        fld angle
        fcos
        fstp result
    }
    return result;
#else
    return static_cast<float>(std::cos(angle));
#endif
}

// SJIS bone-name needles (.rdata 0x52B7F0..0x52B81C); memcmp lengths in the
// original include the NUL byte.
const char kNeedleStrstr[] = "\x8E\x77";                    // 0x52B7F0
const char kNameR1[] = "\x89\x45\x8E\xE8\x8E\xF1";          // 0x52B7F4 (7)
const char kNameR2[] = "\x89\x45\x82\xD0\x82\xB6";          // 0x52B7FC (7)
const char kNameR3[] = "\x89\x45\x98\x72";                  // 0x52B804 (5)
const char kNameL1[] = "\x8D\xB6\x8E\xE8\x8E\xF1";          // 0x52B80C (7)
const char kNameL2[] = "\x8D\xB6\x82\xD0\x82\xB6";          // 0x52B814 (7)
const char kNameL3[] = "\x8D\xB6\x98\x72";                  // 0x52B81C (5)

float* Vec3Normalize(float v[3]) {
    auto& d = d3dx::Get();
    if (d.Load())
        return d.vec3Normalize(v, v);
    const float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len != 0.0f) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
    return v;
}

void QuatMultiply(float out[4], const float a[4], const float b[4]) {
    auto& d = d3dx::Get();
    if (d.Load()) {
        d.quatMultiply(out, a, b);
        return;
    }
    // D3DXQuaternionMultiply: out = b (x) a, layout x,y,z,w
    out[0] = b[3] * a[0] + b[0] * a[3] + b[1] * a[2] - b[2] * a[1];
    out[1] = b[3] * a[1] - b[0] * a[2] + b[1] * a[3] + b[2] * a[0];
    out[2] = b[3] * a[2] + b[0] * a[1] - b[1] * a[0] + b[2] * a[3];
    out[3] = b[3] * a[3] - b[0] * a[0] - b[1] * a[1] - b[2] * a[2];
}

unsigned char* CurrentModel(MMDApp* app) {
    const unsigned slot = app->raw<std::uint8_t>(2320);
    return app->raw<unsigned char*>(1920 + 4 * slot);
}

bool IsSelectedRoot(unsigned char* model, int index, bool excludeSelected) {
    const auto* selected = static_cast<const unsigned char*>(
        mikudancestudio::mdl::Mdl(model)->boneSelection);
    if (selected == nullptr || selected[index] == 0)
        return false;
    if (excludeSelected && index == mikudancestudio::mdl::Mdl(model)->selectedBone)
        return false;
    auto* bones = mikudancestudio::mdl::Bones(model);
    unsigned char* bone = mikudancestudio::mdl::BoneBytes(bones, index);
    const std::uint8_t type = bone[484];
    if (type != 1 && type != 2)
        return false;
    const int parent = mdl::At<std::int32_t>(bone, 48);
    return parent == -1 || selected[parent] == 0;
}

void TransformBoneVector(const unsigned char* bone, std::size_t matrix,
                         const float in[3], float out[3]) {
    out[0] = mdl::At<float>(const_cast<unsigned char*>(bone), matrix + 0) * in[0] +
             mdl::At<float>(const_cast<unsigned char*>(bone), matrix + 16) * in[1] +
             mdl::At<float>(const_cast<unsigned char*>(bone), matrix + 32) * in[2];
    out[1] = mdl::At<float>(const_cast<unsigned char*>(bone), matrix + 4) * in[0] +
             mdl::At<float>(const_cast<unsigned char*>(bone), matrix + 20) * in[1] +
             mdl::At<float>(const_cast<unsigned char*>(bone), matrix + 36) * in[2];
    out[2] = mdl::At<float>(const_cast<unsigned char*>(bone), matrix + 8) * in[0] +
             mdl::At<float>(const_cast<unsigned char*>(bone), matrix + 24) * in[1] +
             mdl::At<float>(const_cast<unsigned char*>(bone), matrix + 40) * in[2];
}

void BoneWorldPoint(const unsigned char* bone, float out[3]) {
    const float rest[3] = {
        mdl::At<float>(const_cast<unsigned char*>(bone), 308),
        mdl::At<float>(const_cast<unsigned char*>(bone), 312),
        mdl::At<float>(const_cast<unsigned char*>(bone), 316)};
    TransformBoneVector(bone, 52, rest, out);
    out[0] += mdl::At<float>(const_cast<unsigned char*>(bone), 100);
    out[1] += mdl::At<float>(const_cast<unsigned char*>(bone), 104);
    out[2] += mdl::At<float>(const_cast<unsigned char*>(bone), 108);
}

void ApplySelectedRootTranslation(unsigned char* model, const float delta[3]) {
    auto* bones = mikudancestudio::mdl::Bones(model);
    auto* dirty = mikudancestudio::mdl::Mdl(model)->bonePhysicsState;
    const int count = mikudancestudio::mdl::Mdl(model)->boneCount;
    if (bones == nullptr)
        return;
    for (int i = 0; i < count; ++i) {
        if (!IsSelectedRoot(model, i, false))
            continue;
        unsigned char* bone = mikudancestudio::mdl::BoneBytes(bones, i);
        float localDelta[3]{};
        TransformBoneVector(bone, 180, delta, localDelta);
        mdl::At<float>(bone, 320) += localDelta[0];
        mdl::At<float>(bone, 324) += localDelta[1];
        mdl::At<float>(bone, 328) += localDelta[2];
        if (dirty != nullptr)
            dirty[i] = 1;
    }
}

void EchoRecord(HWND dialog, int id, const char* format, float value) {
    char text[256]{};
    sprintf_s(text, sizeof(text), format, value);
    SetWindowTextA(GetDlgItem(dialog, id), text);
}

bool EditModeRecord(MMDApp* app, int mode) {
    HWND dialog = app->raw<HWND>(658292);
    if (dialog == nullptr)
        return false;
    const int dy = app->MouseY() - app->PreviousMouseY();

    if (mode >= 4 && mode <= 6) {
        const int axis = mode - 4;
        if (app->raw<std::uint8_t>(650676) != 0) {
            auto* base = app->raw<unsigned char*>(658480);
            const int index = app->raw<std::int32_t>(658624);
            if (base != nullptr && index >= 0) {
                float& value = *reinterpret_cast<float*>(
                    base + 140 * index + 48 + 4 * axis);
                value = static_cast<float>(value -
                    static_cast<double>(dy) * kAccDrag);
                EchoRecord(dialog, 754 + axis, "%3.2f",
                    value / g_ConvB52B768 * g_ConvA52B760);
            }
        } else {
            auto* base = app->raw<unsigned char*>(658300);
            const int index = app->raw<std::int32_t>(658476);
            if (base != nullptr && index >= 0) {
                float& value = *reinterpret_cast<float*>(
                    base + 172 * index + 64 + 4 * axis);
                value = static_cast<float>(value -
                    static_cast<double>(dy) * kAccDrag);
                EchoRecord(dialog, 715 + axis, "%3.2f",
                    value / g_ConvB52B768 * g_ConvA52B760);
            }
        }
        return true;
    }

    if (mode >= 10 && mode <= 12) {
        const int axis = mode - 10;
        constexpr double kStep = 0.05000000074505806;
        if (app->raw<std::uint8_t>(650676) != 0) {
            auto* base = app->raw<unsigned char*>(658480);
            const int index = app->raw<std::int32_t>(658624);
            if (base != nullptr && index >= 0) {
                float& value = *reinterpret_cast<float*>(
                    base + 140 * index + 36 + 4 * axis);
                value = static_cast<float>(value - dy * kStep);
                EchoRecord(dialog, 744 + axis, "%5.4f", value);
            }
        } else {
            auto* base = app->raw<unsigned char*>(658300);
            const int index = app->raw<std::int32_t>(658476);
            if (base != nullptr && index >= 0) {
                const bool scale = app->ShiftModifierActive();
                const std::size_t field = (scale ? 40u : 52u) + 4u * axis;
                float& value = *reinterpret_cast<float*>(
                    base + 172 * index + field);
                value = static_cast<float>(value - dy * kStep);
                if (scale && value < 0.1f)
                    value = 0.1f;
                EchoRecord(dialog, (scale ? 709 : 712) + axis, "%3.2f", value);
            }
        }
        return true;
    }
    return true;
}

double PrecisionMultiplier(MMDApp* app, double coarse) {
    if (app->ShiftModifierActive())
        return coarse;
    if (app->CtrlModifierActive())
        return 0.1;
    return 1.0;
}

void ApplyScreenPlaneBoneMove(MMDApp* app, unsigned char* model, int mode) {
    D3DRenderer* render = app->Renderer();
    if (render == nullptr)
        return;
    const int viewWidth = render->screenWidth;    // wrapper+120036
    const int viewHeight = render->screenHeight;  // wrapper+120040
    const float overlayScale = render->viewScale;  // wrapper+120048
    const int outputWidth = app->RenderWidth();
    const int outputHeight = app->RenderHeight();
    if (viewWidth == 0 || viewHeight == 0 || outputHeight == 0 ||
        overlayScale == 0.0f)
        return;

    const double mouseScale = app->raw<float>(2344);
    float delta[3]{};
    if (mode == 7 || mode == 9) {
        double amount = static_cast<double>(
            app->MouseX() - app->PreviousMouseX());
        amount *= mouseScale / viewWidth;
        amount *= static_cast<double>(outputWidth) / outputHeight;
        if (app->CameraPerspective() != 0)
            amount *= -static_cast<double>(app->CameraDistance()) * 0.9;
        amount /= overlayScale;
        amount *= PrecisionMultiplier(app, 10.0);
        const double yaw = app->CameraRotation()[1];
        delta[0] = static_cast<float>(std::cos(yaw) * amount);
        delta[2] = static_cast<float>(std::sin(yaw) * amount);
        ApplySelectedRootTranslation(model, delta);
    }

    if (mode == 8 || mode == 9) {
        double amount = static_cast<double>(
            app->PreviousMouseY() - app->MouseY());
        amount *= mouseScale / viewHeight;
        if (app->CameraPerspective() != 0)
            amount *= -static_cast<double>(app->CameraDistance()) * 0.95;
        else
            amount *= 1.05;
        amount /= overlayScale;
        amount *= PrecisionMultiplier(app, 10.0);
        const double pitch = -static_cast<double>(app->CameraRotation()[0]);
        const double yaw = -static_cast<double>(app->CameraRotation()[1]);
        delta[0] = static_cast<float>(std::sin(yaw) * std::sin(pitch) * amount);
        delta[1] = static_cast<float>(std::cos(pitch) * amount);
        delta[2] = static_cast<float>(std::cos(yaw) * std::sin(pitch) * amount);
        ApplySelectedRootTranslation(model, delta);
    }
}

void ApplyLocalAxisBoneMove(MMDApp* app, unsigned char* model, int mode) {
    const int selectedIndex = mikudancestudio::mdl::Mdl(model)->selectedBone;
    if (selectedIndex < 0)
        return;
    auto* bones = mikudancestudio::mdl::Bones(model);
    if (bones == nullptr)
        return;
    unsigned char* selected = mikudancestudio::mdl::BoneBytes(bones, selectedIndex);

    double step = 0.05000000074505806;
    if (app->ShiftModifierActive())
        step = 0.5;
    else if (app->CtrlModifierActive())
        step = 0.004999999888241291;
    const int mouseDelta = mode == 12
        ? app->MouseY() - app->PreviousMouseY()
        : app->PreviousMouseY() - app->MouseY();

    float delta[3]{};
    delta[mode - 10] = static_cast<float>(mouseDelta * step);
    if (app->raw<std::int32_t>(650652) != 1) {
        float basis[16]{};
        BoneLocalAxes(app, basis);
        const int axis = mode - 10;
        float local[3] = {
            basis[axis * 4 + 0] * delta[axis],
            basis[axis * 4 + 1] * delta[axis],
            basis[axis * 4 + 2] * delta[axis]};
        TransformBoneVector(selected, 52, local, delta);
    }
    ApplySelectedRootTranslation(model, delta);
}

}  // namespace

// VA 0x0040E670 - local-axis basis matrix of the selected bone.
// Fills a 16-float matrix: identity, unless the selected bone carries local
// axes (bone+520..528 non-zero and model physicsMode==2 -> orthonormalized
// basis from bone+520 and bone+532) or its name matches one of the arm
// prefixes (memcmp/strstr above) -> 2D basis from the bone->parent(+460)
// direction (bone+308/+312).
void BoneLocalAxes(MMDApp* app, float out[16]) {
    for (int i = 0; i < 16; ++i)
        out[i] = 0.0f;
    out[0] = out[5] = out[10] = out[15] = 1.0f;

    unsigned char* model = app->SelectedModel();
    if (model == nullptr || mikudancestudio::mdl::Mdl(model)->selectedBone < 0)
        return;
    auto* bones = mikudancestudio::mdl::Bones(model);
    unsigned char* bone =
        mikudancestudio::mdl::BoneBytes(bones,
                                mikudancestudio::mdl::Mdl(model)->selectedBone);

    const bool hasAxes = !(mdl::At<float>(bone, 520) == 0.0f &&
                           mdl::At<float>(bone, 524) == 0.0f &&
                           mdl::At<float>(bone, 528) == 0.0f);
    if (hasAxes && mdl::Mdl(model)->physicsMode == 2) {
        float a[3] = {mdl::At<float>(bone, 520), mdl::At<float>(bone, 524),
                      mdl::At<float>(bone, 528)};
        float b[3] = {mdl::At<float>(bone, 532), mdl::At<float>(bone, 536),
                      mdl::At<float>(bone, 540)};
        Vec3Normalize(a);
        Vec3Normalize(b);
        float c[3] = {b[1] * a[2] - b[2] * a[1],
                      b[2] * a[0] - b[0] * a[2],
                      b[0] * a[1] - b[1] * a[0]};
        Vec3Normalize(c);
        c[0] = -c[0];
        c[1] = -c[1];
        c[2] = -c[2];
        float e[3] = {c[2] * a[1] - c[1] * a[2],
                      a[2] * c[0] - c[2] * a[0],
                      a[0] * c[1] - c[0] * a[1]};
        Vec3Normalize(e);
        out[0] = a[0];
        out[1] = a[1];
        out[2] = a[2];
        out[4] = c[0];
        out[5] = c[1];
        out[6] = c[2];
        out[8] = e[0];
        out[9] = e[1];
        out[10] = e[2];
    } else {
        const char* name = reinterpret_cast<const char*>(bone);
        if (std::memcmp(name, kNameR1, 7) == 0 ||
            std::memcmp(name, kNameR2, 7) == 0 ||
            std::memcmp(name, kNameR3, 5) == 0 ||
            std::memcmp(name, kNameL1, 7) == 0 ||
            std::memcmp(name, kNameL2, 7) == 0 ||
            std::memcmp(name, kNameL3, 5) == 0 ||
            std::strstr(name, kNeedleStrstr) != nullptr) {
            const int p = mdl::At<std::int32_t>(bone, 460);
            const float dx = mdl::At<float>(mdl::BoneBytes(bones, p), 308) -
                             mdl::At<float>(bone, 308);
            const float dy = mdl::At<float>(mdl::BoneBytes(bones, p), 312) -
                             mdl::At<float>(bone, 312);
            const float len = std::sqrt(dx * dx + dy * dy);
            out[0] = dx / len;
            out[1] = dy / len;
            out[4] = -out[1];
            out[5] = out[0];
        }
    }
}

// VA region 0x475A6E..0x477220 - modes 1..6 of the bone-drag stage.
void BoneEditModes(MMDApp* app) {
    auto raw = [&app](std::size_t off) -> std::int32_t& {
        return *reinterpret_cast<std::int32_t*>(app->at(off));
    };
    const int mode = raw(844);
    if (mode <= 0 || mode > 12)
        return;

    if ((mode <= 6 || mode >= 10) && app->raw<HWND>(658292) != nullptr) {
        EditModeRecord(app, mode);
        return;
    }

    unsigned char* model = CurrentModel(app);
    if (model == nullptr)
        return;
    const int sel = mikudancestudio::mdl::Mdl(model)->selectedBone;
    if (sel < 0)
        return;

    if (mode >= 7 && mode <= 9) {
        ApplyScreenPlaneBoneMove(app, model, mode);
        return;
    }
    if (mode >= 10) {
        ApplyLocalAxisBoneMove(app, model, mode);
        return;
    }
    auto* bones = mikudancestudio::mdl::Bones(model);
    unsigned char* bone = mikudancestudio::mdl::BoneBytes(bones, sel);

    const auto M = [bone](std::size_t off) -> float& {
        return mdl::At<float>(bone, off);
    };
    const auto fraw = [&app](std::size_t off) -> float& {
        return *reinterpret_cast<float*>(app->at(off));
    };
    const bool selA = raw(36) == 3;      // coarse selector
    const bool selB = raw(192) == 3;     // fine selector
    const auto scaleOf = [&](float base) -> float {
        if (selA)
            return static_cast<float>(kCoarse * base);
        if (selB)
            return static_cast<float>(kFine * base);
        return base;
    };

    float ang = 0.0f;
    float axis[3] = {0.0f, 0.0f, 0.0f};

    if (mode == 1) {
        const float base = static_cast<float>(
            static_cast<double>(raw(12) - raw(4)) * kAngBase);
        ang = scaleOf(base);
        const double aX = -static_cast<double>(fraw(0x310));
        const double aY = -static_cast<double>(fraw(0x314));
        const float ax = static_cast<float>(std::cos(aX));
        const float ay = static_cast<float>(std::sin(aY) * std::sin(aX));
        const float az = static_cast<float>(std::cos(aY) * std::sin(aX));
        axis[0] = M(0xB8) * ax + M(0xB4) * ay + M(0xBC) * az;
        axis[1] = M(0xCC) * az + M(0xC4) * ay + M(0xC8) * ax;
        axis[2] = M(0xDC) * az + M(0xD8) * ax + M(0xD4) * ay;
    } else if (mode == 2) {
        const float base = static_cast<float>(
            static_cast<double>(raw(16) - raw(8)) * kAngBase);
        ang = scaleOf(base);
        const double aX = -static_cast<double>(fraw(0x310));
        const double aY = -static_cast<double>(fraw(0x314));
        const float ax = static_cast<float>(std::cos(aX));
        const float az = static_cast<float>(std::sin(aY));
        axis[0] = M(0xB4) * ax + M(0xBC) * az;
        axis[1] = M(0xC4) * ax + M(0xCC) * az;
        axis[2] = M(0xD4) * ax + M(0xDC) * az;
    } else if (mode == 3) {
        const double a1 = std::atan2(
            static_cast<double>(raw(4) - raw(2352)),
            static_cast<double>(raw(8) - raw(2356)));
        const double a2 = std::atan2(
            static_cast<double>(raw(12) - raw(2352)),
            static_cast<double>(raw(16) - raw(2356)));
        float ang1 = static_cast<float>(a1 * g_MouseScaleA);
        float ang2 = static_cast<float>(a2 * g_MouseScaleA);
        if (kWrap < static_cast<double>(ang1) - ang2)
            ang2 = static_cast<float>(ang2 + kWrap);
        if (static_cast<double>(ang1) - ang2 < -kWrap)
            ang1 = static_cast<float>(ang1 + kWrap);
        ang = scaleOf(ang1 - ang2);
        const double aX = -static_cast<double>(fraw(0x310));
        const double aY = -static_cast<double>(fraw(0x314));
        const float ax = -static_cast<float>(std::sin(aX));
        const float ay = static_cast<float>(std::sin(aY) * std::cos(aX));
        const float az = static_cast<float>(std::cos(aY) * std::cos(aX));
        axis[0] = M(0xB8) * ax + M(0xB4) * ay + M(0xBC) * az;
        axis[1] = M(0xCC) * az + M(0xC4) * ay + M(0xC8) * ax;
        axis[2] = M(0xDC) * az + M(0xD8) * ax + M(0xD4) * ay;
    } else {
        // modes 4/5/6 - local axes
        const float base = static_cast<float>(
            static_cast<double>(raw(16) - raw(8)) * kAngBase);
        ang = scaleOf(base);
        if (raw(650076) == 1) {
            const std::size_t o = static_cast<std::size_t>(mode - 4) * 4;
            axis[0] = M(0xB4 + o);
            axis[1] = M(0xC4 + o);
            axis[2] = M(0xD4 + o);
        } else {
            float basis[16];
            BoneLocalAxes(app, basis);
            const std::size_t r = static_cast<std::size_t>(mode - 4) * 4;
            const float mx = basis[r];
            const float my = basis[r + 1];
            const float mz = basis[r + 2];
            const float v0 = M(0x54) * mz + M(0x34) * mx + M(0x44) * my;
            const float v1 = M(0x58) * mz + M(0x38) * mx + M(0x48) * my;
            const float v2 = M(0x5C) * mz + M(0x4C) * my + M(0x3C) * mx;
            axis[0] = M(0xB8) * v1 + M(0xB4) * v0 + M(0xBC) * v2;
            axis[1] = M(0xCC) * v2 + M(0xC4) * v0 + M(0xC8) * v1;
            axis[2] = v2 * M(0xDC) + M(0xD8) * v1 + M(0xD4) * v0;
            Vec3Normalize(axis);
        }
    }

    {
        // ---- bone edit ------------------------------------------------------
        if ((mdl::At<std::uint32_t>(bone, 500) & 0x400) == 0x400 &&
            (bone[484] == 4 || bone[484] == 8)) {
            if (mdl::Mdl(model)->physicsMode == 2) {
                axis[0] = mdl::At<float>(bone, 0x1FC);
                axis[1] = mdl::At<float>(bone, 0x200);
                axis[2] = mdl::At<float>(bone, 0x204);
            } else {
                unsigned char* p =
                    mdl::BoneBytes(bones,
                        mdl::At<std::int32_t>(bone, 0x1CC));
                axis[0] = mdl::At<float>(p, 0x134) - M(0x134);
                axis[1] = mdl::At<float>(p, 0x138) - M(0x138);
                axis[2] = mdl::At<float>(p, 0x13C) - M(0x13C);
                Vec3Normalize(axis);
            }
        }
        unsigned char* dirty =
            mikudancestudio::mdl::Mdl(model)->bonePhysicsState;
        if (dirty != nullptr)
            dirty[sel] = 1;

        // 0x476D6D/0x476DA3: sin/cos of the FULL drag angle (the 0.5
        // half-angle multiplication does NOT exist in the instruction
        // stream; var_14D4 already carries the scaled full angle)
        float dq[4] = {OriginalSinTimes(ang, axis[0]),
                       OriginalSinTimes(ang, axis[1]),
                       OriginalSinTimes(ang, axis[2]),
                       OriginalCos(ang)};
        float out[4];
        QuatMultiply(out, reinterpret_cast<float*>(bone + 0x14C), dq);
        std::memcpy(bone + 0x14C, out, sizeof(out));

        // child-bone propagation loop (0x476E42..0x477257, lifted from
        // disassembly): every type-1/2 bone of the marked wave whose
        // parent is also marked rotates about the selected bone's world
        // position; kinematic pose slots +320/+332 are updated so the
        // transform chain picks the change up.
        {
            float piv[3]{};
            BoneWorldPoint(bone, piv);
            const float invq[4] = {-dq[0], -dq[1], -dq[2], dq[3]};
            const int n = mikudancestudio::mdl::Mdl(model)->boneCount;
            for (int i = 0; i < n; ++i) {
                unsigned char* ch = mdl::BoneBytes(bones, i);
                if (!IsSelectedRoot(model, i, true))
                    continue;
                if (dirty != nullptr)
                    dirty[i] = 1;                    // t4 mark (0x476FB2)
                float point[3]{};
                BoneWorldPoint(ch, point);
                const float off[4] = {
                    point[0] - piv[0], point[1] - piv[1], point[2] - piv[2],
                    0.0f};
                float r1[4], r2[4];
                QuatMultiply(r1, invq, off);         // 0x4770A6
                QuatMultiply(r2, r1, dq);            // 0x4770FB sandwich
                mdl::At<float>(ch, 320) += r2[0] - off[0];
                mdl::At<float>(ch, 324) += r2[1] - off[1];
                mdl::At<float>(ch, 328) += r2[2] - off[2];
                float cq[4], oq[4];
                std::memcpy(cq, ch + 0x14C, sizeof(cq));
                QuatMultiply(oq, cq, dq);            // 0x4771EB
                std::memcpy(ch + 0x14C, oq, sizeof(oq));
            }
        }
        return;
    }
}

}  // namespace mikudancestudio
