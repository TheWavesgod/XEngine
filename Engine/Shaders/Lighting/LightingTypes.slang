#pragma once

static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;

static const uint MAX_GPU_LIGHTS = 16;

// Must match the C++ GPULight layout.
// Keep fields 16-byte aligned and update both sides together.
struct GPULight
{
    // xyz = world-space position, w = range.
    float4 PositionRange;

    // xyz = direction from shaded point to light, w = light type.
    float4 DirectionType;

    // rgb = color, w = intensity.
    float4 ColorIntensity;

    // x = inner cone angle radians, y = outer cone angle radians,
    // z = cast shadow flag, w = padding.
    float4 SpotAnglesShadow;
};

// Must match the C++ GPULightingData layout.
// Keep fields 16-byte aligned and update both sides together.
struct GPULightingData
{
    // rgb = ambient color, w = ambient intensity.
    float4 AmbientColorIntensity;

    // x = light count, yzw = padding / future.
    float4 LightCountAndPadding;

    GPULight Lights[MAX_GPU_LIGHTS];
};
