#pragma once

#include <Windows.h>
#include <d3d9.h>
#include "mikudancestudio/bone_layout.hpp"

namespace mikudancestudio {
enum class BoneOverlayPoint { Origin, Tail };

bool ProjectBonePoint(const mdl::BoneRecord& bone, BoneOverlayPoint pointKind,
                      const D3DMATRIX& world, const D3DMATRIX& view,
                      const D3DMATRIX& projection, const RECT& viewport,
                      int* screenX, int* screenY, float* clipW);
}
