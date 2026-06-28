#pragma once

static const uint MAX_SHADOW_CASCADES = 4;

struct GPUCascadeShadowData
{
    float4x4 LightViewProjection;

    // x = split far in view-space depth
    // y = depth bias
    // z = normal bias
    // w = texel size
    float4 Params;
};

struct GPUShadowData
{
    // x = enabled
    // y = cascade count
    // z = shadow resolution
    // w = visualize cascades
    float4 ShadowParams;

    GPUCascadeShadowData Cascades[MAX_SHADOW_CASCADES];
};