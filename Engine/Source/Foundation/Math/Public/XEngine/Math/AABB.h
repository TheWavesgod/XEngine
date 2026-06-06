#pragma once

#include <XEngine/Math/MathTypes.h>

namespace XEngine
{
    struct AABB
    {
        Vec3 Min { 0.0f, 0.0f, 0.0f };
        Vec3 Max { 0.0f, 0.0f, 0.0f };

        Vec3 GetCenter() const
        {
            return (Min + Max) * 0.5f;
        }

        Vec3 GetExtents() const
        {
            return (Max - Min) * 0.5f;
        }
    };
}

