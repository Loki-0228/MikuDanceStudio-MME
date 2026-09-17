#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "mikudancestudio/bone_projection.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {
void Check(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
}

int main() {
    using namespace mikudancestudio;
    // Place one bone immediately before inaccessible memory. The former
    // bone + 308 / bone + 464 reads land in the reserved guard region.
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    const SIZE_T page = info.dwPageSize;
    auto* memory = static_cast<unsigned char*>(VirtualAlloc(
        nullptr, page + 512 * sizeof(mdl::BoneRecord), MEM_RESERVE, PAGE_NOACCESS));
    Check(memory != nullptr, "reserve guarded bone allocation");
    Check(VirtualAlloc(memory, page, MEM_COMMIT, PAGE_READWRITE) != nullptr,
          "commit one bone page");
    auto* bone = new (memory + page - sizeof(mdl::BoneRecord)) mdl::BoneRecord{};
    D3DMATRIX identity{};
    identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
    std::memcpy(bone->matInit, &identity, sizeof identity);
    bone->position[0] = 0.25f;
    bone->position[1] = 0.5f;
    bone->position[2] = 0.75f;
    bone->tailOffset[0] = -0.5f;
    bone->tailOffset[1] = -0.25f;
    bone->tailOffset[2] = 0.5f;
    const RECT viewport{100, 50, 900, 650};
    int x = 0, y = 0;
    float w = 0;
    Check(ProjectBonePoint(*bone, BoneOverlayPoint::Origin, identity, identity,
                           identity, viewport, &x, &y, &w), "origin visible");
    Check(x == 600 && y == 200 && w == 1.0f, "origin uses model-space position");
    Check(ProjectBonePoint(*bone, BoneOverlayPoint::Tail, identity, identity,
                           identity, viewport, &x, &y, &w), "tail visible");
    Check(x == 300 && y == 425 && w == 1.0f, "tail uses its own named field");
    bone->matInit[12] = 0.25f;
    D3DMATRIX world = identity;
    world._42 = -0.5f;
    Check(ProjectBonePoint(*bone, BoneOverlayPoint::Origin, world, identity,
                           identity, viewport, &x, &y, &w), "transformed origin visible");
    Check(x == 700 && y == 350, "bone and world transformations both apply");
    D3DMATRIX behind = identity;
    behind._44 = -1;
    Check(!ProjectBonePoint(*bone, BoneOverlayPoint::Origin, identity, identity,
                            behind, viewport, &x, &y, &w), "behind-camera bone rejected");
    Check(x == 393939 && y == 393939, "hidden bone retains selection sentinel");
    bone->~BoneRecord();
    VirtualFree(memory, 0, MEM_RELEASE);
    std::puts("Bone projection regressions passed");
}
