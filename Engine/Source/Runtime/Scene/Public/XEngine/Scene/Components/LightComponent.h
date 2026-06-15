#pragma once

#include <XEngine/Math/MathTypes.h>

namespace XEngine
{
    enum class LightType
    {
        Directional,
        Point,
        Spot,
    };

    struct LightComponent
    {
        LightType Type = LightType::Directional;

        Vec3 Color { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;

        float Range = 10.0f;

        float InnerConeAngleDegree = 30.0f;
        float OuterConeAngleDegree = 60.0f;

        bool CastShadow = true;
        bool Enabled = true;
    };
}
