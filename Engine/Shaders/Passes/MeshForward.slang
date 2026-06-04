struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION;
    [[vk::location(1)]]
    float3 color : COLOR0;
};

struct VSOutput
{
    float4 position : SV_Position;
    [[vk::location(0)]]
    float3 color : COLOR0;
};

struct PushConstants
{
    float4x4 modelViewProjection;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> g_PushConstants;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.position = mul(g_PushConstants.modelViewProjection, float4(input.position, 1.0));
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target0
{
    return float4(input.color, 1.0);
}
