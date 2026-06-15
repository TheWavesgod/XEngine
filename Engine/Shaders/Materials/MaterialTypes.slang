#pragma once

// SurfaceData is the shading input passed into lighting/BRDF code.
struct SurfaceData
{
    float3 BaseColor;
    float Metallic;
    float Roughness;
    float3 Normal;
    float Alpha;
    float AmbientOcclusion;
};
