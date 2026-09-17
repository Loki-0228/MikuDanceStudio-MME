#pragma once

#include <d3d9.h>
#include <cstddef>

namespace mikudancestudio {
enum class ModelAlphaPass { Original, Opaque, Translucent };

// Partition materials, not pixels: submit every material exactly once, including
// draws intercepted by MME. Preserve authored order within each group and the
// depth writes which keep inverted-hull outlines behind transparent hair.
inline bool ModelMaterialMatchesPass(ModelAlphaPass pass, bool usesAlpha) {
    return pass == ModelAlphaPass::Original ||
        (pass == ModelAlphaPass::Translucent) == usesAlpha;
}

// resourcePool tag's low three bytes store the toon color. The fourth byte is
// host-only alpha metadata, computed once from the actual loaded texture.
constexpr unsigned char kTextureAlphaKnown = 1;
constexpr unsigned char kTextureUsesAlpha = 2;

inline bool TexturePixelsUseAlpha(const D3DLOCKED_RECT& pixels, UINT width,
                                 UINT height) {
    for (UINT y = 0; y < height; ++y) {
        const auto* row = static_cast<const unsigned char*>(pixels.pBits) +
            static_cast<std::size_t>(y) * pixels.Pitch;
        for (UINT x = 0; x < width; ++x)
            if (row[4 * x + 3] != 255)
                return true;
    }
    return false;
}

inline void CacheTextureColorAndAlpha(IDirect3DTexture9* texture,
                                     unsigned char* tag) {
    tag[3] = kTextureAlphaKnown | kTextureUsesAlpha;
    D3DSURFACE_DESC desc{};
    D3DLOCKED_RECT pixels{};
    if (texture == nullptr || FAILED(texture->GetLevelDesc(0, &desc)) ||
        desc.Format != D3DFMT_A8R8G8B8 || desc.Width == 0 || desc.Height == 0 ||
        FAILED(texture->LockRect(0, &pixels, nullptr, D3DLOCK_READONLY)))
        return;
    const auto* lastRow = static_cast<const unsigned char*>(pixels.pBits) +
        static_cast<std::size_t>(desc.Height - 1) * pixels.Pitch;
    tag[0] = lastRow[2];
    tag[1] = lastRow[1];
    tag[2] = lastRow[0];
    tag[3] = kTextureAlphaKnown |
        (TexturePixelsUseAlpha(pixels, desc.Width, desc.Height) ? kTextureUsesAlpha : 0);
    texture->UnlockRect(0);
}
}
