#pragma once

#include <d3d9.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace mikudancestudio {
// Keep one indexed draw per material (MME may replace that draw). Ordering its
// triangles back to front lets alpha blend against the surface behind it while
// retaining the depth writes required by MMD's later outline pass.
template<class Index>
bool SortTransparentTriangles(const void* vertices, UINT stride, UINT vertexCount,
                              Index* indices, UINT indexCount,
                              const D3DMATRIX& worldView) {
    if (vertices == nullptr || indices == nullptr || stride < 3 * sizeof(float) ||
        indexCount % 3 != 0)
        return false;
    struct Triangle { Index indices[3]; float depth; };
    std::vector<Triangle> triangles;
    triangles.reserve(indexCount / 3);
    for (UINT i = 0; i < indexCount; i += 3) {
        Triangle triangle{{indices[i], indices[i + 1], indices[i + 2]}, 0};
        for (auto index : triangle.indices) {
            if (static_cast<UINT>(index) >= vertexCount)
                return false;
            const auto* p = reinterpret_cast<const float*>(
                static_cast<const unsigned char*>(vertices) +
                static_cast<std::size_t>(index) * stride);
            const float depth = p[0] * worldView._13 + p[1] * worldView._23 +
                p[2] * worldView._33 + worldView._43;
            if (!std::isfinite(depth))
                return false;
            triangle.depth += depth / 3.0f;
        }
        triangles.push_back(triangle);
    }
    std::stable_sort(triangles.begin(), triangles.end(),
        [](const Triangle& a, const Triangle& b) { return a.depth > b.depth; });
    UINT i = 0;
    for (const auto& triangle : triangles)
        for (auto index : triangle.indices)
            indices[i++] = index;
    return true;
}
}
