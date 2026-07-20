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

// Converts clip-space to shadow UV. Vulkan NDC: [-1, 1] -> UV [0, 1] (Y inverted).
float2 ClipToShadowUV(float4 clip)
{
    return clip.xy / clip.w * float2(0.5, -0.5) + float2(0.5, 0.5); // Y-flip for Vulkan
}

// Converts our orthographic projection's clip.z (which is in [1, 0] because
// we swapped near/far of `Math::OrthographicLH_ZO` for CSM reverse-Z) into the
// depth-buffer convention that Vulkan writes to the depth attachment when
// we run ShadowDepthPass. The actual storage is `clip.z * 0.5 + 0.5`, mapping
// the ortho [1, 0] -> [1.0, 0.5] depth range. To match this, the fragment
// shader must compare in the SAME range, so it too calls `clip.z * 0.5 + 0.5`.
// Closer-to-light fragments get larger depth; farther-from-light get smaller.
float ClipToShadowDepth(float4 clip)
{
    return clip.z / clip.w * 0.5 + 0.5;
}

// Single-tap shadow test. Returns 1.0 if lit, 0.0 if in shadow.
// Our cascade projection swaps near/far so that:
//   - Larger depth value (closer to 1) = fragment is closer to the light.
//   - Smaller depth value (closer to 0.5) = fragment is farther from the light.
// For LIT: the surface that the light ray hits at this fragment's UV must be at
// or beyond the fragment's own depth from the light, i.e. shadowMapDepth <= refDepth.
// We add `bias` (and slope term) to refDepth so surface self-shadowing (float
// rounding around zero) tolerates the comparison.
float SampleShadow1Tap(
    GPUShadowData shadow, int cascadeIndex,
    float3 worldPos, float refDepth, float NdotL)
{
    float4 clip = ProjectToCascadeClip(worldPos, shadow.Cascades[cascadeIndex].LightViewProjection);
    float2 uv = saturate(ClipToShadowUV(clip));
    float layer = (float)cascadeIndex;
    float shadowMapDepth = g_ShadowMap.Sample(g_ShadowSampler, float3(uv, layer)).r;
    float biasedRef = refDepth
                    + shadow.Cascades[cascadeIndex].Params.y
                    + max(0.0, 1.0 - NdotL) * shadow.Cascades[cascadeIndex].Params.z;
    return (shadowMapDepth <= biasedRef) ? 1.0 : 0.0;
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
    float biasedRef = refDepth
                    + shadow.Cascades[cascadeIndex].Params.y
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
            sum += (sampleDepth <= biasedRef) ? 1.0 : 0.0;
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