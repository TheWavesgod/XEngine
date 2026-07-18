#pragma once

#include <XEngine/Math/MathTypes.h>

namespace XEngine
{
    // TODO Stage 12 Culling: this type is reserved for frustum culling.
    // The intended API is roughly:
    //   - static Frustum ExtractFromViewProjection(const Mat4& view, const Mat4& proj);
    //   - bool Contains(AABB) const / Intersects(AABB) const;
    // Until RenderGraph V1 / culling lane lands, callers should NOT rely on
    // this type; frustum culling currently runs as full-scene AABB iteration.
    struct Frustum
    {
    };
}



