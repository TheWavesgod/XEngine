#pragma once

#include <XEngine/Math/MathTypes.h>

namespace XEngine
{
    // Flat transform component for Scene entities.
    // Stage 7F has no hierarchy; parent/child propagation is left for a later stage.
    struct TransformComponent
    {
        Vec3 Position { 0.0f, 0.0f, 0.0f };
        Quat Rotation { 1.0f, 0.0f, 0.0f, 0.0f };
        Vec3 Scale { 1.0f, 1.0f, 1.0f };

        Mat4 LocalMatrix { 1.0f };
        Mat4 WorldMatrix { 1.0f };
        Mat4 PreviousWorldMatrix { 1.0f };

        bool Dirty = true;

        void UpdateMatrices();
    };
}
