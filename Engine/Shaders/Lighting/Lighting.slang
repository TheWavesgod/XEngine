#pragma once

#include "LightingTypes.slang"
#include "BRDF.slang"

float3 EvaluateSceneLighting(
    GPULightingData lighting,
    SurfaceData surface,
    float3 worldPosition,
    float3 normal,
    float3 viewDir)
{
    float3 result = surface.BaseColor
        * lighting.AmbientColorIntensity.rgb
        * lighting.AmbientColorIntensity.w
        * surface.AmbientOcclusion;

    uint lightCount = min((uint)lighting.LightCountAndPadding.x, MAX_GPU_LIGHTS);

    // Stage 8C evaluates directional lights first.
    // Point and spot lighting are reserved for a later extension.
    for (uint i = 0; i < lightCount; ++i)
    {
        GPULight light = lighting.Lights[i];
        uint type = (uint)light.DirectionType.w;

        if (type == LIGHT_TYPE_DIRECTIONAL)
        {
            float3 L = normalize(light.DirectionType.xyz);
            float3 radiance = light.ColorIntensity.rgb * light.ColorIntensity.w;
            result += EvaluatePBRDirectional(surface, normal, viewDir, L, radiance);
        }
    }

    return result;
}
