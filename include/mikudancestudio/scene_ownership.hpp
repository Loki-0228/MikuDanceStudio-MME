// ===========================================================================
// MikuDanceStudio - ownership of scene-resident heap allocations
// ===========================================================================
#pragma once

namespace mikudancestudio {

class MMDApp;

// Preserve the reference executable's release order.  These are raw,
// zero-initialized blocks, so their matching release operation is free().
void ReleaseSceneModels(MMDApp& app);
void ReleaseGlobalTimelineTracks(MMDApp& app);
void ReleaseAccessoriesAndTracks(MMDApp& app);

}  // namespace mikudancestudio
