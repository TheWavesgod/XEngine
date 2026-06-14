#pragma once

#include <XEngine/Math/MathTypes.h>

#include <algorithm>
#include <limits>

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

        float GetRadius() const
        {
            return glm::length(GetExtents());
        }

        bool IsValid() const
        {
            return Min.x <= Max.x && Min.y <= Max.y && Min.z <= Max.z;
        }

        void Encapsulate(const Vec3& point)
        {
            Min = glm::min(Min, point);
            Max = glm::max(Max, point);
        }

        void Encapsulate(const AABB& other)
        {
            Min = glm::min(Min, other.Min);
            Max = glm::max(Max, other.Max);
        }
    };

    inline AABB TransformAABB(const AABB& bounds, const Mat4& transform)
    {
        const Vec3 corners[] = {
            { bounds.Min.x, bounds.Min.y, bounds.Min.z },
            { bounds.Max.x, bounds.Min.y, bounds.Min.z },
            { bounds.Min.x, bounds.Max.y, bounds.Min.z },
            { bounds.Max.x, bounds.Max.y, bounds.Min.z },
            { bounds.Min.x, bounds.Min.y, bounds.Max.z },
            { bounds.Max.x, bounds.Min.y, bounds.Max.z },
            { bounds.Min.x, bounds.Max.y, bounds.Max.z },
            { bounds.Max.x, bounds.Max.y, bounds.Max.z }
        };

        AABB transformed;
        transformed.Min = Vec3 { std::numeric_limits<float>::max() };
        transformed.Max = Vec3 { std::numeric_limits<float>::lowest() };
        for (const Vec3& corner : corners)
        {
            transformed.Encapsulate(Vec3 { transform * Vec4 { corner, 1.0f } });
        }
        return transformed;
    }

    inline AABB CombineAABB(const AABB& a, const AABB& b)
    {
        AABB combined = a;
        combined.Encapsulate(b);
        return combined;
    }
}

