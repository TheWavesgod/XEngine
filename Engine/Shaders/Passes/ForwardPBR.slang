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

    // Compute view-space depth for cascade selection.
    float4 viewPos4 = mul(g_FrameData.Camera.View, float4(input.worldPosition, 1.0));
    float viewDepth = viewPos4.x; // XEngine: camera forward is +X in view space.

    // Compute per-light shadow factors. Stage 8C has a small fixed-size light
    // array; in V0 only the first directional light is shadow-casting.
    float shadowFactors[MAX_GPU_LIGHTS];
    [unroll]
    for (uint li = 0; li < MAX_GPU_LIGHTS; ++li)
    {
        shadowFactors[li] = 1.0;
    }

    uint lightCount = (uint)g_FrameData.Lighting.LightCountAndPadding.x;
    [unroll]
    for (uint li = 0; li < MAX_GPU_LIGHTS && li < lightCount; ++li)
    {
        GPULight light = g_FrameData.Lighting.Lights[li];
        if (light.DirectionType.w == LIGHT_TYPE_DIRECTIONAL && // directional
            light.SpotAnglesShadow.z > 0.5) // CastShadow
        {
            float NdotL = saturate(dot(surface.Normal, light.DirectionType.xyz));
            int filterMode = (g_FrameData.Shadow.ShadowParams.y > 0.0) ? 1 : 0;
            shadowFactors[li] = ComputeShadowFactor(
                g_FrameData.Shadow,
                filterMode,
                input.worldPosition,
                viewDepth,
                NdotL);
        }
    }

    float3 color = EvaluateSceneLighting(
        g_FrameData.Lighting,
        surface,
        input.worldPosition,
        surface.Normal,
        V,
        shadowFactors);

    // TODO Stage 9:
    // Move HDR output and tonemapping into a dedicated post-process path.
    color = pow(Saturate3(color), float3(1.0 / 2.2));

    // DEBUG multi-channel viz. Channels mean:
    //   R = shadowFactors[0]      (1 = lit, 0 = in shadow)
    //   G = ShadowParams.x        (1 = shadow enabled, 0 = disabled)
    //   B = refDepth for cascade 0 (mapped to [0,1] for visibility)
    //   A = sample of shadow texture cascade 0 at fixed UV (probe of texture contents)
    if (g_FrameData.Shadow.ShadowParams.x > 0.5)
    {
        int cascadeIdx = SelectCascadeIndex(viewDepth, g_FrameData.Shadow);
        float refDepth = 0.0;
        {
            float4 clip = ProjectToCascadeClip(input.worldPosition, g_FrameData.Shadow.Cascades[cascadeIdx].LightViewProjection);
            refDepth = ClipToShadowDepth(clip);
        }
        // Probe shadow texture at 4 fixed points to confirm depth data exists.
        float probeCenter = g_ShadowMap.Sample(g_ShadowSampler, float3(0.5, 0.5, 0)).r;
        float probeCorner = g_ShadowMap.Sample(g_ShadowSampler, float3(0.25, 0.25, 0)).r;
        return float4(
            shadowFactors[0],
            g_FrameData.Shadow.ShadowParams.x,
            saturate(refDepth),
            probeCenter + 0.5 * probeCorner
        );
    }

    // This is a debug overlay. It is applied after the main lighting evaluation and overrides the final color.
    if (g_FrameData.Shadow.ShadowParams.w > 0.5)
    {
        int cascadeIndex = SelectCascadeIndex(viewDepth, g_FrameData.Shadow);
        float3 cascadeColor = float3(
            cascadeIndex == 0 ? 1.0 : 0.0,
            cascadeIndex == 1 ? 1.0 : 0.0,
            cascadeIndex == 2 ? 1.0 : 0.0);
        return float4(cascadeColor, 1.0);
    }

    return float4(color, baseColor.a);
}
