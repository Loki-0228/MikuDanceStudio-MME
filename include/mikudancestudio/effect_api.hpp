#pragma once

namespace mikudancestudio {

// MME rebuilds its object/order table inside IDirect3DDevice9::BeginScene.
void BeginEffectObjectRegistration();

// The modern CRT does not pass through the host import that MME hooks.
void NotifyEffectFileOpen(const wchar_t* path);

}  // namespace mikudancestudio
