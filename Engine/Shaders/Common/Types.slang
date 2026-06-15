#pragma once

#include "../Lighting/LightingTypes.slang"

struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION;
    [[vk::location(1)]]
    float3 normal : NORMAL0;
    [[vk::location(2)]]
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_Position;
    [[vk::location(0)]]
    float3 worldPosition : TEXCOORD0;
    [[vk::location(1)]]
    float3 normal : NORMAL0;
    [[vk::location(2)]]
    float2 uv : TEXCOORD1;
};

// Must match the C++ GPUFrameData layout.
// Keep fields 16-byte aligned and update both sides together.
struct GPUCameraData
{
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float4 CameraPosition;
};

// Must match the C++ GPUFrameData layout.
// Keep fields 16-byte aligned and update both sides together.
struct GPUFrameData
{
    GPUCameraData Camera;
    GPULightingData Lighting;
};
