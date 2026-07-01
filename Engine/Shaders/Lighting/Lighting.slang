#pragma once

#include "LightingTypes.slang"
#include "BRDF.slang"

float3 EvaluateSceneLighting(
    GPULightingData lighting,
    SurfaceData surface,
    float3 worldPosition,
    float3 normal,
    float3 viewDir,
    float shadowFactors[MAX_GPU_LIGHTS] = {})
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
        if (shadowFactors[i] == 0.0f) continue;

        GPULight light = lighting.Lights[i];
        uint type = (uint)light.DirectionType.w;
        float3 lightContribution = 0;

        if (type == LIGHT_TYPE_DIRECTIONAL)
        {
            float3 L = normalize(light.DirectionType.xyz);
            float3 radiance = light.ColorIntensity.rgb * light.ColorIntensity.w;
            lightContribution = EvaluatePBRDirectional(surface, normal, viewDir, L, radiance);
        }

        result += lightContribution * shadowFactors[i];
    }

    return result;
}
