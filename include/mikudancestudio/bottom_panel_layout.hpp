#pragma once

#include <cstdint>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/offsets.hpp"

namespace mikudancestudio {

// Horizontal-collapse state shared by the bottom-panel background and its
// GDI labels. The original advances the offset only for collapsed panels.
struct BottomPanelLayout {
    int leading;
    int afterLightOrFace;
    int afterSelfShadow;
    int accessory;
    int final;
};

inline BottomPanelLayout ComputeBottomPanelLayout(const MMDApp* app) {
    const bool camera =
        app->raw<std::uint8_t>(offsets::kByteOptflag0) != 0;
    const bool cameraExpanded =
        app->raw<std::uint8_t>(offsets::kByteOptflag1) != 0;
    const bool lightExpanded =
        app->raw<std::uint8_t>(offsets::kByteOptflag2) != 0;
    const bool accessoryExpanded =
        app->raw<std::uint8_t>(offsets::kByteOptflag3) != 0;
    const bool boneExpanded =
        app->raw<std::uint8_t>(offsets::kByteOptflag4) != 0;
    const bool facialExpanded =
        app->raw<std::uint8_t>(offsets::kByteOptflag5) != 0;
    const bool shadowExpanded =
        app->raw<std::uint8_t>(offsets::kByteOptflag6) != 0;

    BottomPanelLayout result{};
    if (camera)
        result.leading = cameraExpanded ? -18 : 117;
    else
        result.leading = boneExpanded ? 35 : 214;

    result.afterLightOrFace = result.leading;
    if (camera) {
        if (!lightExpanded)
            result.afterLightOrFace += 162;
    } else if (!facialExpanded) {
        result.afterLightOrFace += 251;
    }

    result.afterSelfShadow = result.afterLightOrFace;
    if (camera && !shadowExpanded)
        result.afterSelfShadow += 159;

    result.accessory = camera
        ? result.afterSelfShadow - 180
        : result.afterSelfShadow;
    result.final = result.accessory;
    if (camera && !accessoryExpanded)
        result.final += 178;
    if (!camera)
        result.final += 13;
    return result;
}

}  // namespace mikudancestudio
