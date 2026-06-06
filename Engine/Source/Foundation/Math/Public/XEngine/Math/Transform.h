#pragma once

#include <XEngine/Math/MathFunctions.h>

namespace XEngine
{
    struct Transform
    {
        Vec3 Position { 0.0f, 0.0f, 0.0f };
        Quat Rotation { 1.0f, 0.0f, 0.0f, 0.0f };
        Vec3 Scale { 1.0f, 1.0f, 1.0f };

        Mat4 ToMatrix() const
        {
            return Translate(Position) * Rotate(Rotation) * XEngine::Scale(Scale);
        }
    };
}
