#include "../Common/Math.slang"
#include "../Common/Types.slang"
#include "../Lighting/BRDF.slang"
#include "../Lighting/Lighting.slang"
#include "../Lighting/LightingTypes.slang"
#include "../Lighting/ShadowSampling.slang"
#include "../Materials/MaterialTypes.slang"
#include "../Materials/PBRMaterial.slang"

struct PushConstants
{
    float4x4 model;
    float4 baseColorFactor;
    // x = metallic, y = roughness, z = alpha cutoff, w = reserved.
    float4 materialFactors;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> g_PushConstants;

// Set 0: per-frame data.
// Contains camera and scene lighting.
[[vk::binding(0, 0)]]
ConstantBuffer<GPUFrameData> g_FrameData;

[[vk::binding(0, 1)]]
Sampler2D baseColorTexture;

[[vk::binding(1, 1)]]
Sampler2D normalTexture;

[[vk::binding(2, 1)]]
Sampler2D metallicRoughnessTexture;

[[vk::binding(3, 1)]]
Sampler2D aoTexture;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.worldPosition = mul(g_PushConstants.model, float4(input.position, 1.0)).xyz;
    output.position = mul(g_FrameData.Camera.ViewProjection, float4(output.worldPosition, 1.0));
    output.normal = normalize(mul((float3x3)g_PushConstants.model, input.normal));
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target0
{
    float4 sampledBaseColor = baseColorTexture.Sample(input.uv);
    float4 baseColor = sampledBaseColor * g_PushConstants.baseColorFactor;
    if (baseColor.a < g_PushConstants.materialFactors.z)
    {
        discard;
    }

    float4 metallicRoughnessSample = metallicRoughnessTexture.Sample(input.uv);
    float ao = aoTexture.Sample(input.uv).r;

    // TODO Stage 7:
    // Use loaded/generated tangents for tangent-space normal mapping.
    // Stage 6E binds and samples the normal slot safely, but keeps vertex normals.
    float3 sampledNormal = normalTexture.Sample(input.uv).xyz * 2.0 - 1.0;
    float3 N = normalize(input.normal + sampledNormal * 0.0001);

    // CameraPosition is in XEngine world space.
    float3 V = normalize(g_FrameData.Camera.CameraPosition.xyz - input.worldPosition);

    float metallic = Saturate(g_PushConstants.materialFactors.x * metallicRoughnessSample.b);
    float roughness = Saturate(g_PushConstants.materialFactors.y * metallicRoughnessSample.g);
    SurfaceData surface = MakeSurfaceData(baseColor, metallic, roughness, N, ao);

    float3 color = EvaluateSceneLighting(
        g_FrameData.Lighting,
        surface,
        input.worldPosition,
        surface.Normal,
        V);

    // TODO Stage 9:
    // Move HDR output and tonemapping into a dedicated post-process path.
    color = pow(Saturate3(color), float3(1.0 / 2.2));
    return float4(color, baseColor.a);
}
