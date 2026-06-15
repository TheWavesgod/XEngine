#pragma once

#include "MaterialTypes.slang"
#include "../Common/Math.slang"

SurfaceData MakeSurfaceData(
    float4 baseColor,
    float metallic,
    float roughness,
    float3 normal,
    float ambientOcclusion)
{
    SurfaceData surface;
    surface.BaseColor = baseColor.rgb;
    surface.Metallic = Saturate(metallic);
    surface.Roughness = max(Saturate(roughness), 0.04);
    surface.Normal = normalize(normal);
    surface.Alpha = baseColor.a;
    surface.AmbientOcclusion = ambientOcclusion;
    return surface;
}
