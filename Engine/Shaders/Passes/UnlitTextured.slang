struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION;
    [[vk::location(1)]]
    float3 color : COLOR0;
    [[vk::location(2)]]
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_Position;
    [[vk::location(0)]]
    float2 uv : TEXCOORD0;
};

struct PushConstants
{
    float4x4 modelViewProjection;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> g_PushConstants;

// Stage 6D manual convention:
// set 0, binding 0 = combined image sampler for material base color.
[[vk::binding(0, 0)]]
Sampler2D baseColorTexture;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.position = mul(g_PushConstants.modelViewProjection, float4(input.position, 1.0));
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target0
{
    return baseColorTexture.Sample(input.uv);
}
