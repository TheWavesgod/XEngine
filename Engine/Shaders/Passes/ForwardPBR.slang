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

struct PushConstants
{
    float4x4 modelViewProjection;
    float4 baseColorFactor;
    // x = metallic, y = roughness, z = alpha cutoff, w = reserved.
    float4 materialFactors;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> g_PushConstants;

[[vk::binding(0, 0)]]
Sampler2D baseColorTexture;

[[vk::binding(1, 0)]]
Sampler2D normalTexture;

[[vk::binding(2, 0)]]
Sampler2D metallicRoughnessTexture;

[[vk::binding(3, 0)]]
Sampler2D aoTexture;

static const float Pi = 3.14159265359;

float Saturate(float value)
{
    return clamp(value, 0.0, 1.0);
}

float3 Saturate3(float3 value)
{
    return clamp(value, float3(0.0), float3(1.0));
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = Saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = Pi * denominator * denominator;
    return a2 / max(denominator, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, 0.0001);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = Saturate(dot(N, V));
    float NdotL = Saturate(dot(N, L));
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (float3(1.0) - F0) * pow(1.0 - Saturate(cosTheta), 5.0);
}

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.position = mul(g_PushConstants.modelViewProjection, float4(input.position, 1.0));
    output.worldPosition = input.position;
    output.normal = normalize(input.normal);
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

    float3 V = normalize(float3(0.0, 0.0, 3.0) - input.worldPosition);

    // TODO Stage 8:
    // Move lighting into LightSystem and real light buffers.
    float3 L = normalize(float3(0.5, 1.0, 0.3));
    float3 H = normalize(V + L);
    float3 lightColor = float3(1.0, 0.96, 0.9);

    float metallic = Saturate(g_PushConstants.materialFactors.x * metallicRoughnessSample.b);
    float roughness = Saturate(g_PushConstants.materialFactors.y * metallicRoughnessSample.g);
    roughness = max(roughness, 0.04);

    float3 albedo = baseColor.rgb;
    float3 F0 = lerp(float3(0.04), albedo, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = (float3(1.0) - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    float3 directLighting = (kD * albedo / Pi + specular) * lightColor * NdotL;
    float3 ambient = 0.03 * albedo * ao;

    float3 color = ambient + directLighting;

    // TODO Stage 9:
    // Move HDR output and tonemapping into a dedicated post-process path.
    color = pow(Saturate3(color), float3(1.0 / 2.2));
    return float4(color, baseColor.a);
}
