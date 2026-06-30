#pragma once

#include "ShadowTypes.slang"
#include "../Common/Types.slang"
#include "../Common/Math.slang"

// Set 0, binding 1: shadow texture array (Texture2DArray).
[[vk::binding(1, 0)]]
Texture2DArray<float> g_ShadowMap;

// Set 0, binding 2: shadow sampler.
[[vk::binding(2, 0)]]
SamplerState g_ShadowSampler;

// Selects the cascade index for a given view-space depth.
// Returns 0 if depth is below the first split, last cascade if depth is past
// the last split. Returns 0 if shadows are disabled.
int SelectCascadeIndex(float viewSpaceDepth, GPUShadowData shadow)
{
    uint cascadeCount = (uint)shadow.ShadowParams.y;
    if (cascadeCount == 0) return 0;
    uint lastCascade = cascadeCount - 1;

    int selected = (int)lastCascade;
    for (uint i = 0; i < lastCascade; ++i)
    {
        if (viewSpaceDepth < shadow.Cascades[i].Params.x)
        {
            selected = (int)i;
            break;
        }
    }
    return selected;
}

// Projects a world-space position into the cascade's clip space.
float4 ProjectToCascadeClip(float3 worldPos, float4x4 lightViewProj)
{
    return mul(lightViewProj, float4(worldPos, 1.0));
}

// Converts clip-space to shadow UV. Reverse-Z: the depth test passes when
// the shadow map's depth is greater than or equal to the reference depth.
float2 ClipToShadowUV(float4 clip)
{
    return clip.xy / clip.w * float2(0.5, -0.5) + float2(0.5, 0.5); // flip Y for Vulkan
}

float ClipToShadowDepth(float4 clip)
{
    return clip.z / clip.w;
}

// Single-tap shadow test. Returns 1.0 if lit, 0.0 if in shadow.
float SampleShadow1Tap(
    GPUShadowData shadow, int cascadeIndex,
    float3 worldPos, float refDepth, float NdotL)
{
    float4 clip = ProjectToCascadeClip(worldPos, shadow.Cascades[cascadeIndex].LightViewProjection);
    float2 uv = saturate(ClipToShadowUV(clip));
    float layer = (float)cascadeIndex;
    float shadowMapDepth = g_ShadowMap.Sample(g_ShadowSampler, float3(uv, layer)).r;
    float biasedRef = refDepth + shadow.Cascades[cascadeIndex].Params.y
                    + max(0.0, 1.0 - NdotL) * shadow.Cascades[cascadeIndex].Params.z;
    return (shadowMapDepth >= biasedRef) ? 1.0 : 0.0;
}

// 3x3 PCF. Average 9 taps for a soft shadow.
float SampleShadowPCF3x3(
    GPUShadowData shadow, int cascadeIndex,
    float3 worldPos, float refDepth, float NdotL)
{
    float4 clip = ProjectToCascadeClip(worldPos, shadow.Cascades[cascadeIndex].LightViewProjection);
    float2 uv = ClipToShadowUV(clip);
    if (any(uv < 0.0) || any(uv > 1.0))
    {
        // Outside the shadow map: treat as fully lit (avoid border artifacts).
        return 1.0;
    }
    float texelSize = shadow.Cascades[cascadeIndex].Params.w;
    float layer = (float)cascadeIndex;
    float biasedRef = refDepth + shadow.Cascades[cascadeIndex].Params.y
                    + max(0.0, 1.0 - NdotL) * shadow.Cascades[cascadeIndex].Params.z;

    float sum = 0.0;
    [unroll]
    for (int dy = -1; dy <= 1; ++dy)
    {
        [unroll]
        for (int dx = -1; dx <= 1; ++dx)
        {
            float2 offset = float2(dx, dy) * texelSize;
            float2 sampleUV = saturate(uv + offset);
            float sampleDepth = g_ShadowMap.Sample(g_ShadowSampler, float3(sampleUV, layer)).r;
            sum += (sampleDepth >= biasedRef) ? 1.0 : 0.0;
        }
    }
    return sum / 9.0;
}

// Returns the shadow factor in [0, 1]. 1.0 = fully lit, 0.0 = fully shadowed.
float ComputeShadowFactor(
    GPUShadowData shadow, int filterMode,
    float3 worldPos, float viewSpaceDepth, float NdotL)
{
    if (shadow.ShadowParams.x < 0.5) return 1.0; // shadows disabled

    int cascadeIndex = SelectCascadeIndex(viewSpaceDepth, shadow);

    // refDepth in [0, 1] reverse-Z, derived from world position projected to clip.
    float4 clip = ProjectToCascadeClip(worldPos, shadow.Cascades[cascadeIndex].LightViewProjection);
    float refDepth = ClipToShadowDepth(clip);

    if (filterMode == 0) // Hard
    {
        return SampleShadow1Tap(shadow, cascadeIndex, worldPos, refDepth, NdotL);
    }
    else // PCF3x3
    {
        return SampleShadowPCF3x3(shadow, cascadeIndex, worldPos, refDepth, NdotL);
    }
}