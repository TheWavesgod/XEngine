#pragma once

#include "../Common/Constants.slang"
#include "../Common/Math.slang"
#include "../Materials/MaterialTypes.slang"

// BRDF helpers are pure shading math.
// They must not declare resource bindings.
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = Saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = Pi * denominator * denominator;
    return a2 / max(denominator, Epsilon);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, Epsilon);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = Saturate(dot(N, V));
    float NdotL = Saturate(dot(N, L));
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (float3(1.0) - F0) * pow(1.0 - Saturate(cosTheta), 5.0);
}

float3 EvaluatePBRDirectional(
    SurfaceData surface,
    float3 normal,
    float3 viewDir,
    float3 lightDir,
    float3 radiance)
{
    float3 H = normalize(viewDir + lightDir);
    float3 F0 = lerp(float3(0.04), surface.BaseColor, surface.Metallic);

    float NDF = DistributionGGX(normal, H, surface.Roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, surface.Roughness);
    float3 F = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + Epsilon;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = (float3(1.0) - kS) * (1.0 - surface.Metallic);

    float NdotL = max(dot(normal, lightDir), 0.0);
    return (kD * surface.BaseColor / Pi + specular) * radiance * NdotL;
}
