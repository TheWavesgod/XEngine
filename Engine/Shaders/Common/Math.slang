#pragma once

float Saturate(float value)
{
    return clamp(value, 0.0, 1.0);
}

float3 Saturate3(float3 value)
{
    return clamp(value, float3(0.0), float3(1.0));
}
