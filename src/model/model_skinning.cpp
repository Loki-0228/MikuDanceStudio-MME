// ===========================================================================
// VA 0x004B0C50 / 0x004A9400 - per-frame model VB deformation
// ===========================================================================
// 0x4B0C50 restores and accumulates vertex morphs, locks model+8/model+12,
// and dispatches one of the 0x4A9400-family OpenMP skinning workers. This
// file currently closes only the original PMD path serially; the separate
// x64 PMX BDEF/SDEF/UV-morph worker family must be recovered from the x64
// reference before this function can claim full parity.
// ===========================================================================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/ported_funcs.hpp"

namespace mikudancestudio {
namespace {

bool ModelVbCaptureReady(char directory[MAX_PATH]) {
    const DWORD length = GetEnvironmentVariableA(
        "MIKUDANCESTUDIO_VB_DUMP_DIR", directory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return false;

    char stable[2]{};
    if (GetEnvironmentVariableA("MIKUDANCESTUDIO_AB_STABLE_CAPTURE", stable,
                                sizeof(stable)) != 1 || stable[0] != '1')
        return true;

    char requireLine[2]{};
    const bool refreshed =
        GetEnvironmentVariableA("MIKUDANCESTUDIO_AB_REQUIRE_LINE", requireLine,
                                sizeof(requireLine)) == 1 &&
        requireLine[0] == '1';
    char gate[MAX_PATH]{};
    std::snprintf(gate, sizeof(gate), "%s\\%s", directory,
                  refreshed ? "vb.capture.active" : "vb.capture.ready");
    const DWORD attributes = GetFileAttributesA(gate);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

void WriteBytes(const char* path, const void* bytes, DWORD size) {
    HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;
    DWORD written = 0;
    WriteFile(file, bytes, size, &written, nullptr);
    CloseHandle(file);
}

void DumpModelVertexBuffers(MMDApp* app, unsigned char* model,
                            const mdl::SkinnedVertexBase* mainVertices,
                            const mdl::EdgeVertex* edgeVertices,
                            std::uint32_t count,
                            std::uint32_t mainStride) {
    char directory[MAX_PATH]{};
    if (!ModelVbCaptureReady(directory))
        return;

    int slot = -1;
    for (int i = 0; i < 100; ++i) {
        if (app->ModelSlot(i) == model) {
            slot = i;
            break;
        }
    }
    if (slot < 0)
        return;

    static LONG dumped[100]{};
    if (InterlockedCompareExchange(&dumped[slot], 1, 0) != 0)
        return;

    CreateDirectoryA(directory, nullptr);
    char path[MAX_PATH]{};
    std::snprintf(path, sizeof(path), "%s\\model.%03d.main.bin",
                  directory, slot);
    WriteBytes(path, mainVertices, mainStride * count);
    std::snprintf(path, sizeof(path), "%s\\model.%03d.edge.bin",
                  directory, slot);
    WriteBytes(path, edgeVertices, sizeof(*edgeVertices) * count);
    std::snprintf(path, sizeof(path), "%s\\model.%03d.meta.json",
                  directory, slot);
    char metadata[160]{};
    const mdl::ModelRecord& state = *mdl::Mdl(model);
    const int chars = std::snprintf(
        metadata, sizeof(metadata),
        "{\"schema\":1,\"slot\":%d,\"format\":%u,"
        "\"vertex_count\":%u,\"main_stride\":%u,\"edge_stride\":16}\n",
        slot, static_cast<unsigned>(state.physicsMode),
        count, mainStride);
    WriteBytes(path, metadata, static_cast<DWORD>(chars));
}

void TransformPosition(float out[3], const float in[3], const float* m) {
    out[0] = in[0] * m[0] + in[1] * m[4] + in[2] * m[8] + m[12];
    out[1] = in[0] * m[1] + in[1] * m[5] + in[2] * m[9] + m[13];
    out[2] = in[0] * m[2] + in[1] * m[6] + in[2] * m[10] + m[14];
}

void TransformNormal(float out[3], const float in[3], const float* m) {
    out[0] = in[0] * m[0] + in[1] * m[4] + in[2] * m[8];
    out[1] = in[0] * m[1] + in[1] * m[5] + in[2] * m[9];
    out[2] = in[0] * m[2] + in[1] * m[6] + in[2] * m[10];
}

void Blend3(float out[3], const float a[3], const float b[3], float wa) {
    const float wb = 1.0f - wa;
    out[0] = a[0] * wa + b[0] * wb;
    out[1] = a[1] * wa + b[1] * wb;
    out[2] = a[2] * wa + b[2] * wb;
}

constexpr float kPmxIdentityMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

const float* PmxSkinMatrix(const mdl::BoneRecord* bones, int boneCount,
                           std::int32_t boneIndex) {
    return boneIndex >= 0 && boneIndex < boneCount
        ? bones[boneIndex].matInit
        : kPmxIdentityMatrix;
}

// x64 PMX worker 0x14011F97D: BDEF1 obtains one matInit matrix (or an
// identity fallback for a negative index), then writes position and normal
// into the 32-byte main vertex record. This helper is deliberately not wired
// into the PMX dispatcher until BDEF2/BDEF4/SDEF and the common edge tail are
// buffer-compared against the x64 reference.
[[maybe_unused]]
void SkinPmxBdef1(const mdl::PmxVertex& source,
                  const mdl::BoneRecord* bones, int boneCount,
                  mdl::SkinnedVertexBase& destination) {
    const float* matrix = PmxSkinMatrix(bones, boneCount, source.bone[0]);
    TransformPosition(destination.position, source.position, matrix);
    TransformNormal(destination.normal, source.normal, matrix);
}

float BlendTransformComponent(const float* input, const float* matrix0,
                              const float* matrix1, int component,
                              float weight0, bool position) {
#if defined(_M_IX86)
    float result = 0.0f;
    const int byteOffset = component * 4;
    const float weight1 = 1.0f - weight0;
    __asm {
        mov eax, input
        mov ecx, matrix0
        mov edx, matrix1
        mov ebx, byteOffset

        // Original 0x4A95E0 family: bone 1 is accumulated first, multiplied
        // by (1-weight), then bone 0 is accumulated and multiplied by
        // weight.  No transformed component is rounded to float in between.
        fld dword ptr [eax]
        fmul dword ptr [edx+ebx]
        fld dword ptr [eax+4]
        fmul dword ptr [edx+ebx+16]
        faddp st(1), st
        fld dword ptr [eax+8]
        fmul dword ptr [edx+ebx+32]
        faddp st(1), st
        cmp position, 0
        je no_translation_1
        fadd dword ptr [edx+ebx+48]
no_translation_1:
        fmul dword ptr [weight1]

        fld dword ptr [eax]
        fmul dword ptr [ecx+ebx]
        fld dword ptr [eax+4]
        fmul dword ptr [ecx+ebx+16]
        faddp st(1), st
        fld dword ptr [eax+8]
        fmul dword ptr [ecx+ebx+32]
        faddp st(1), st
        cmp position, 0
        je no_translation_0
        fadd dword ptr [ecx+ebx+48]
no_translation_0:
        fmul dword ptr [weight0]
        faddp st(1), st
        fstp dword ptr [result]
    }
    return result;
#else
    const float weight1 = 1.0f - weight0;
    const float a = input[0] * matrix0[component] +
                    input[1] * matrix0[component + 4] +
                    input[2] * matrix0[component + 8] +
                    (position ? matrix0[component + 12] : 0.0f);
    const float b = input[0] * matrix1[component] +
                    input[1] * matrix1[component + 4] +
                    input[2] * matrix1[component + 8] +
                    (position ? matrix1[component + 12] : 0.0f);
    return b * weight1 + a * weight0;
#endif
}

float EdgeComponent(float position, float normal, float amount) {
#if defined(_M_IX86)
    float result = 0.0f;
    __asm {
        fld dword ptr [normal]
        fmul dword ptr [amount]
        fadd dword ptr [position]
        fstp dword ptr [result]
    }
    return result;
#else
    return position + normal * amount;
#endif
}

// x64 PMX worker 0x14011F513: BDEF2 uses bone 1 first with (1-weight),
// then adds bone 0 scaled by weight. BlendTransformComponent preserves that
// source order on the x64 path.
[[maybe_unused]]
void SkinPmxBdef2(const mdl::PmxVertex& source,
                  const mdl::BoneRecord* bones, int boneCount,
                  mdl::SkinnedVertexBase& destination) {
    const float* matrix0 = PmxSkinMatrix(bones, boneCount, source.bone[0]);
    const float* matrix1 = PmxSkinMatrix(bones, boneCount, source.bone[1]);
    const float weight0 = source.weight[0];
    for (int component = 0; component < 3; ++component) {
        destination.position[component] = BlendTransformComponent(
            source.position, matrix0, matrix1, component, weight0, true);
        destination.normal[component] = BlendTransformComponent(
            source.normal, matrix0, matrix1, component, weight0, false);
    }
}

float TransformComponent(const float input[3], const float matrix[16],
                         int component, bool position) {
    float value = input[0] * matrix[component];
    value += input[1] * matrix[component + 4];
    value += input[2] * matrix[component + 8];
    if (position)
        value += matrix[component + 12];
    return value;
}

// x64 PMX worker 0x14011FB7F: BDEF4 retrieves all four matrices with the
// same negative-index identity fallback as BDEF1/2.  Its scalar additions
// start with bone 1, then add bones 0, 2, and 3.  The order matters for the
// final float bits, so it is named here rather than hidden in a generic loop.
[[maybe_unused]]
float BlendTransformComponent4(const float input[3],
                               const float matrix0[16],
                               const float matrix1[16],
                               const float matrix2[16],
                               const float matrix3[16],
                               const float weight[4], int component,
                               bool position) {
    float value = TransformComponent(input, matrix1, component, position) *
                  weight[1];
    value += TransformComponent(input, matrix0, component, position) *
             weight[0];
    value += TransformComponent(input, matrix2, component, position) *
             weight[2];
    value += TransformComponent(input, matrix3, component, position) *
             weight[3];
    return value;
}

// Not connected until the x64 BDEF4 fixture has a passing vertex-buffer
// comparison.  Keeping the worker arithmetic in this typed helper avoids
// spreading model-byte offsets into the actual skinning path.
[[maybe_unused]]
void SkinPmxBdef4(const mdl::PmxVertex& source,
                  const mdl::BoneRecord* bones, int boneCount,
                  mdl::SkinnedVertexBase& destination) {
    const float* matrix0 = PmxSkinMatrix(bones, boneCount, source.bone[0]);
    const float* matrix1 = PmxSkinMatrix(bones, boneCount, source.bone[1]);
    const float* matrix2 = PmxSkinMatrix(bones, boneCount, source.bone[2]);
    const float* matrix3 = PmxSkinMatrix(bones, boneCount, source.bone[3]);
    for (int component = 0; component < 3; ++component) {
        destination.position[component] = BlendTransformComponent4(
            source.position, matrix0, matrix1, matrix2, matrix3,
            source.weight, component, true);
        destination.normal[component] = BlendTransformComponent4(
            source.normal, matrix0, matrix1, matrix2, matrix3,
            source.weight, component, false);
    }
}

// x64 PMX common tail 0x140120ABF: edge vertices are a separate 16-byte
// stream.  The source worker multiplies in this precise order before adding
// the skinned position: normal * modelScale * vertexScale * materialSize.
// It remains disconnected with the other PMX helpers until its packed color
// branch has a passing original-vs-rebuilt buffer capture.
[[maybe_unused]]
void WritePmxEdgeVertex(const mdl::PmxVertex& source,
                        const mdl::SkinnedVertexBase& skinned,
                        const mdl::ModelMaterialRecord& material,
                        float frameEdgeScale, mdl::EdgeVertex& destination) {
    for (int component = 0; component < 3; ++component) {
        float amount = skinned.normal[component] * frameEdgeScale;
        amount *= source.edgeScale;
        amount *= material.edgeSize;
        destination.position[component] = skinned.position[component] + amount;
    }
}

// One common-tail color branch at 0x1401209AF converts the material's edge
// RGB channels to truncated 8-bit values and emits a D3DCOLOR-style opaque
// ARGB word. The alternate branch has a different, still-unidentified
// runtime mode and is intentionally not folded into this helper.
[[maybe_unused]]
std::uint32_t PackOpaquePmxEdgeColor(const mdl::ModelMaterialRecord& material) {
    const auto colorByte = [](float channel) -> std::uint32_t {
        return static_cast<std::uint8_t>(static_cast<std::int32_t>(
            channel * 255.0f));
    };
    return 0xFF000000u | (colorByte(material.edgeColor[0]) << 16) |
           (colorByte(material.edgeColor[1]) << 8) |
           colorByte(material.edgeColor[2]);
}

// The x64 worker ladder uses a 32-byte base record plus one contiguous
// float4 for every PMX additional UV set. Keeping this copy typed ensures a
// future UV1--UV4 worker cannot accidentally transpose the loader's
// component-first source storage.
template <std::size_t AdditionalUvCount>
[[maybe_unused]]
void CopyPmxAdditionalUvs(const mdl::PmxVertex& source,
                          mdl::SkinnedVertex<AdditionalUvCount>& destination) {
    for (std::size_t uv = 0; uv < AdditionalUvCount; ++uv)
        for (std::size_t component = 0; component < 4; ++component)
            destination.additionalUv[uv][component] =
                source.additionalUvByComponent[component][uv];
}

// 0x4B0C74..0x4B0CE9 and 0x4B1C9C..0x4B1D8B.  PMD morph zero is
// the base table; all following morphs contain offsets indexed through it.
void ApplyPmdVertexMorphs(unsigned char* model) {
    mdl::ModelRecord& record = *mdl::Mdl(model);
    mdl::PmdVertex* vertices = record.rawVertices;
    const mdl::PmdVertexMorphEntry* base = mdl::BaseVertexMorphTable(model);
    const std::uint32_t baseCount = mdl::BaseVertexMorphCount(model);
    if (vertices == nullptr || base == nullptr)
        return;

    for (std::uint32_t i = 0; i < baseCount; ++i) {
        const mdl::PmdVertexMorphEntry& entry = base[i];
        mdl::PmdVertex& vertex = vertices[entry.vertexIndex];
        vertex.position[0] = entry.offset[0];
        vertex.position[1] = entry.offset[1];
        vertex.position[2] = entry.offset[2];
    }

    mikudancestudio::mdl::MorphRecord* morphs = mikudancestudio::mdl::Morphs(model);
    const int morphCount = record.morphCount;
    if (morphs == nullptr || morphCount <= 1)
        return;
    for (int i = 1; i < morphCount; ++i) {
        mikudancestudio::mdl::MorphRecord* morph = &morphs[i];
        const float weight = morph->value;
        if (weight == 0.0f)
            continue;
        auto* entries = morph->vertexEntries;
        const std::uint32_t count = morph->offsetCount;
        if (entries == nullptr)
            continue;
        for (std::uint32_t k = 0; k < count; ++k) {
            const mdl::PmdVertexMorphEntry& entry = entries[k];
            mdl::PmdVertex& vertex = vertices[entry.vertexIndex];
            vertex.position[0] += entry.offset[0] * weight;
            vertex.position[1] += entry.offset[1] * weight;
            vertex.position[2] += entry.offset[2] * weight;
        }
    }
}

float PmdEdgeDistance(MMDApp* app, unsigned char* model,
                      const float frameWorld[16]) {
    const mdl::ModelRecord& record = *mdl::Mdl(model);
    auto* bones = mdl::Bones(model);
    const int boneCount = record.boneCount;
    float modelPoint[3]{};
    if (bones != nullptr && boneCount > 0) {
        mikudancestudio::mdl::BoneRecord* bone = &bones[(boneCount > 1 ? 1 : 0)];
        const float bind[3] = {bone->position[0], bone->position[1],
                               bone->position[2]};
        TransformPosition(modelPoint, bind,
                          bone->matInit);
    }
    float worldPoint[3];
    TransformPosition(worldPoint, modelPoint, frameWorld);
    const float* camera = app->CameraPosition();
    const double dx = static_cast<double>(worldPoint[0] - camera[0]);
    const double dy = static_cast<double>(worldPoint[1] - camera[1]);
    const double dz = static_cast<double>(worldPoint[2] - camera[2]);
    const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    return static_cast<float>(distance * 0.00004500000068219379 *
        (static_cast<double>(app->CameraFov()) *
             0.6000000238418579 +
         1.0) *
        static_cast<double>(record.edgeScale));
}

void SkinPmd(unsigned char* model, float edgeDistance,
             mdl::EdgeVertex* edgeVertices,
             mdl::SkinnedVertexBase* mainVertices) {
    const mdl::ModelRecord& record = *mdl::Mdl(model);
    const std::uint32_t vertexCount = record.vertexCount;
    auto* vertices = record.rawVertices;
    auto* bones = mdl::Bones(model);
    const int boneCount = record.boneCount;
    if (vertices == nullptr || bones == nullptr)
        return;

    for (std::uint32_t i = 0; i < vertexCount; ++i) {
        const mdl::PmdVertex& src = vertices[i];
        mdl::SkinnedVertexBase& main = mainVertices[i];
        mdl::EdgeVertex& edge = edgeVertices[i];
        const float* position = src.position;
        const float* normal = src.normal;
        const int bone0 = src.bone[0];
        const int bone1 = src.bone[1];
        const float weight0 = static_cast<float>(
            src.weightPercent) * 0.009999999776482582f;
        const float* matrix0 = bone0 >= 0 && bone0 < boneCount
            ? bones[bone0].matInit
            : nullptr;
        const float* matrix1 = bone1 >= 0 && bone1 < boneCount
            ? bones[bone1].matInit
            : nullptr;
        static const float zero[16]{};
        if (matrix0 == nullptr) matrix0 = zero;
        if (matrix1 == nullptr) matrix1 = zero;

        for (int component = 0; component < 3; ++component) {
            main.position[component] = BlendTransformComponent(
                position, matrix0, matrix1, component, weight0, true);
            main.normal[component] = BlendTransformComponent(
                normal, matrix0, matrix1, component, weight0, false);
        }
        main.uv[0] = src.uv[0];
        main.uv[1] = src.uv[1];

        const float amount = src.edgeDisabled != 0
            ? -0.004999999888241291f
            : edgeDistance;
        edge.position[0] = EdgeComponent(main.position[0], main.normal[0], amount);
        edge.position[1] = EdgeComponent(main.position[1], main.normal[1], amount);
        edge.position[2] = EdgeComponent(main.position[2], main.normal[2], amount);
        // edge.diffuse is the load-time 0xFF000000 and is intentionally retained.
    }
}

}  // namespace

void UpdateModelVertexBuffers(MMDApp* app, unsigned char* model,
                              const float frameWorld[16]) {
    if (app == nullptr || model == nullptr || frameWorld == nullptr)
        return;
    mdl::ModelRecord& state = *mdl::Mdl(model);
    if (state.loadComplete == 0 || state.vertexCount == 0)
        return;

    // The PMX branch selects four additional stride/SDEF workers.  Keep it
    // out of the PMD path until those workers are ported byte-for-byte.
    if (state.physicsMode == 2)
        return;

    ApplyPmdVertexMorphs(model);
    auto* mainVb = mdl::ResourceAs<IDirect3DVertexBuffer9>(
        state.vertexBuffer);
    auto* edgeVb = mdl::ResourceAs<IDirect3DVertexBuffer9>(
        state.vertexBuffer2);
    if (mainVb == nullptr || edgeVb == nullptr)
        return;

    const UINT count = state.vertexCount;
    mdl::SkinnedVertexBase* mainVertices = nullptr;
    mdl::EdgeVertex* edgeVertices = nullptr;
    if (FAILED(mainVb->Lock(0, 32 * count,
                            reinterpret_cast<void**>(&mainVertices), 0)))
        return;
    if (FAILED(edgeVb->Lock(0, 16 * count,
                            reinterpret_cast<void**>(&edgeVertices), 0))) {
        mainVb->Unlock();
        return;
    }

    SkinPmd(model, PmdEdgeDistance(app, model, frameWorld), edgeVertices,
            mainVertices);
    DumpModelVertexBuffers(app, model, mainVertices, edgeVertices, count, 32);
    mainVb->Unlock();
    edgeVb->Unlock();
}

}  // namespace mikudancestudio
