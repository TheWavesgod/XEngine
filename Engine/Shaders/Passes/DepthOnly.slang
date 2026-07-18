#include "../Common/Types.slang"

struct ShadowDepthPushConstants
{
    float4x4 Model;
    float4x4 LightViewProjection;
    uint CascadeIndex;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

[[vk::push_constant]]
ConstantBuffer<ShadowDepthPushConstants> g_Push;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(g_Push.Model, float4(input.position, 1.0));
    output.position = mul(g_Push.LightViewProjection, worldPos);

    return output;
}

[shader("fragment")]
[ForceInline]
void fragmentMain(VSOutput input)
{
    // Depth-only: just write SV_Depth (no color write).
    // Slang/SPIRV backend handles SV_Depth emission from position.
}

