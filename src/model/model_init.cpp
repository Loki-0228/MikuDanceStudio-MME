// ===========================================================================
// VA 0x004A8DC0 - ModelInitDefaults  (original: sub_4A8DC0, 1117 bytes)
// VA 0x004A89B0 - ModelInitMorphSlots (original: sub_4A89B0, 522 bytes)
// ===========================================================================
// Default-state initializer of the freshly allocated 0x4CCF4 model block
// (called from 0x460430 add-model, 0x450000 and 0x458F80 scene load).
// Every store below mirrors the original one-to-one; only the 21 identity
// matrices and the 30x23 morph-slot zeroing loops are expressed as loops
// instead of unrolled stores (deviation noted in docs/ARCHITECTURE.md).
// =========================================================================//
#include <cstdint>
#include <cstring>

#include "mikudancestudio/model.hpp"

namespace mikudancestudio {

void ModelInitMorphSlots(unsigned char* m);                  // 0x4A89B0

void ModelInitDefaults(unsigned char* m) {                   // 0x4A8DC0
    using mdl::At;
    static const std::size_t kZ1[] = {9388, 9392, 9396, 9400};
    static const std::size_t kNeg999[] = {14296, 14404, 14416, 14428, 14440,
                                          14452, 14464, 14476, 14488, 14500,
                                          14512, 14524, 14536, 14548};
    mikudancestudio::mdl::Mdl(m)->physicsMode = 0;
    for (std::size_t off : kZ1)
        At<std::uint32_t>(m, off) = 0;
    mdl::ModelRecord& model = *mdl::Mdl(m);
    for (std::int32_t& count : mdl::UvMorphCounts(m).byFamily)
        count = 0;
    for (auto& table : mdl::UvMorphTables(m).byFamily)
        table = nullptr;
    mdl::MaterialMorphBase(m) = nullptr;
    mdl::MaterialMorphAdd(m) = nullptr;
    mdl::MaterialMorphMul(m) = nullptr;
    mdl::BaseVertexMorphCount(m) = 0;
    mdl::BaseVertexMorphTable(m) = nullptr;
    mdl::BoneMorphOffsetCount(m) = 0;
    mdl::BoneMorphOffsets(m) = nullptr;
    model.rbGroups = nullptr;
    model.groupNames = nullptr;
    model.displayFrames = nullptr;
    model.morphs = nullptr;
    model.ikChains = nullptr;
    model.boneTable = nullptr;
    model.materials = nullptr;
    model.indices = nullptr;
    model.rawVertices = nullptr;
    model.pmxVertices = nullptr;
    model.vertexBuffer2 = nullptr;
    model.vertexBuffer = nullptr;
    mdl::Mdl(m)->boneKeyCursors = nullptr;
    mdl::Mdl(m)->boneTrackActive = nullptr;
    mdl::Mdl(m)->morphKeyCursors = nullptr;
    mdl::Mdl(m)->morphTrackActive = nullptr;
    mdl::Mdl(m)->displayKeyCursor = 0;
    mdl::Mdl(m)->displayTrackActive = 0;
    mikudancestudio::mdl::Mdl(m)->displayState = 0;
    mikudancestudio::mdl::Mdl(m)->selectedBone = 0;
    mdl::Mdl(m)->boneSelection = nullptr;
    mdl::Mdl(m)->bonePhysicsState = nullptr;
    mikudancestudio::mdl::Mdl(m)->loadComplete = 1;
    for (std::int32_t& selectedMorph : mdl::Mdl(m)->selectedMorphs)
        selectedMorph = -1;
    mikudancestudio::mdl::Mdl(m)->edgeScale = 1.0f;
    mdl::BoneKeys(m) = nullptr;
    mdl::MorphKeys(m) = nullptr;
    mdl::DisplayKeys(m) = nullptr;
    mdl::BoneKeyIndices(m) = nullptr;
    mdl::MorphKeyIndices(m) = nullptr;
    mikudancestudio::mdl::Mdl(m)->boneListPos = 0;
    mikudancestudio::mdl::Mdl(m)->maxFrame = 0;
    mikudancestudio::mdl::Mdl(m)->undoState[0] = 0;
    mikudancestudio::mdl::Mdl(m)->undoState[1] = 0;
    mikudancestudio::mdl::Mdl(m)->postLoadFlag2 = 0;
    std::memset(mikudancestudio::mdl::Mdl(m)->undoRings, 0,
                sizeof(mikudancestudio::mdl::Mdl(m)->undoRings));
    std::memset(m + 10804, 0, 0x348);
    mdl::Mdl(m)->undoDirty = 0;
    mdl::Mdl(m)->redoDirty = 0;
    mikudancestudio::mdl::Mdl(m)->physicsFlags = 0;
    mdl::Mdl(m)->rigidTable = nullptr;
    mikudancestudio::mdl::Mdl(m)->rigidCount = 0;
    mikudancestudio::mdl::Mdl(m)->jointCount = 0;
    mdl::Mdl(m)->jointTable = nullptr;
    mdl::Mdl(m)->indexBuffer = nullptr;
    mikudancestudio::mdl::Mdl(m)->toonFlag = 1;
    mikudancestudio::mdl::Mdl(m)->toonShared = 0xFFFFFFFFu;
    model.displayKeyframesPresent = 0;
    std::memset(m + 8632, 0, 0x88);
    mikudancestudio::mdl::Mdl(m)->matFloat2 = -999.0f;
    mikudancestudio::mdl::Mdl(m)->boneCount = 0;
    mikudancestudio::mdl::Mdl(m)->matColumn[0] = -999.0f;
    mikudancestudio::mdl::Mdl(m)->morphCount = 0;
    mikudancestudio::mdl::Mdl(m)->matColumn[3] = -999.0f;
    mikudancestudio::mdl::Mdl(m)->ikChainCount = 0;
    mikudancestudio::mdl::Mdl(m)->matColumn[6] = -999.0f;
    mikudancestudio::mdl::Mdl(m)->displayRootBone = 0;
    mikudancestudio::mdl::Mdl(m)->matColumn[9] = -999.0f;
    mikudancestudio::mdl::Mdl(m)->pmxAdditionalUvCount = 0;
    mikudancestudio::mdl::Mdl(m)->matColumn[12] = -999.0f;
    mdl::Mdl(m)->maxBoneLayer = 0;
    mikudancestudio::mdl::Mdl(m)->matColumn[15] = -999.0f;
    mdl::Mdl(m)->boneOrderTable = nullptr;
    At<float>(m, 14380) = -999.0f;
    mikudancestudio::mdl::Mdl(m)->centerBone = 0;
    At<float>(m, 14392) = -999.0f;
    mdl::Mdl(m)->frameRegistrationSelection = 3;
    for (std::size_t off : kNeg999)
        At<float>(m, off) = -999.0f;

    // 17 identity quaternions {0,0,0,1} at 64..332 (4 floats per slot)
    for (int i = 0; i < 17; ++i) {
        float* q = reinterpret_cast<float*>(m + 64 + 16 * i);
        q[0] = 0.0f;
        q[1] = 0.0f;
        q[2] = 0.0f;
        q[3] = 1.0f;
    }

    ModelInitMorphSlots(m);                                // 0x4A89B0

    mdl::PoseTraceFlag(m) = 0;
    mikudancestudio::mdl::Mdl(m)->lightDir[0] = -1.0f;
    mikudancestudio::mdl::Mdl(m)->lightDir[1] = 90.0f;
    mikudancestudio::mdl::Mdl(m)->lightDir[2] = 10.0f;
    mdl::Mdl(m)->legIkXOffset = 1.0f;
    mdl::PoseTraceBuffer(m) = nullptr;
    mikudancestudio::mdl::Mdl(m)->matMisc = 0;
}

void ModelInitMorphSlots(unsigned char* m) {               // 0x4A89B0
    // 30 iterations x 23 slots (float stride 90): the original's unrolled
    // store list spans result-180 .. result+1800 floats, i.e. slot k sits
    // at p + 90*k - 180 - the table is this+336..this+8616.  (Ported with
    // k*90 instead of k*90-180 in an earlier phase, shifting the table
    // 720 bytes right into the IK pointer singles at this+8724..8764 -
    // the comment-box cancel crash of phase 14, fixed in phase 19.)
    float* base = reinterpret_cast<float*>(m + 1056);
    for (int i = 0; i < 30; ++i) {
        float* p = base + 3 * i;  // result advanced by 3 floats each pass
        for (int k = 0; k < 23; ++k) {
            float* slot = p + (90 * k - 180);
            slot[0] = 0.0f;    // (result-180 .. result+1800 pattern)
            slot[1] = -999.0f;
            slot[2] = 0.0f;
        }
    }
    mikudancestudio::mdl::Mdl(m)->lightDir[0] = -1.0f;
    m[8616] = 0;
    mikudancestudio::mdl::Mdl(m)->lightDir[1] = 90.0f;
    mikudancestudio::mdl::Mdl(m)->lightDir[2] = 10.0f;
    mdl::Mdl(m)->legIkXOffset = 1.0f;
}

}  // namespace mikudancestudio
