#pragma once

#include <Windows.h>

struct IDirect3DDevice9;

namespace mikudancestudio {
class MMDApp;

// Present the side margins too, without changing the scene's aspect ratio.
RECT FramePresentationRect(MMDApp* app);
void FillFramePresentationMargins(MMDApp* app, IDirect3DDevice9* device);

}  // namespace mikudancestudio
