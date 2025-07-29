#version 450

// const Value
const uint FLAG_USE_ALBEDO = 1 << 0;
const uint FLAG_USE_NORMAL = 1 << 1;
const uint FLAG_USE_METALLIC = 1 << 2;
const uint FLAG_USE_ROUGHTNESS = 1 << 3;
const uint FLAG_USE_AO = 1 << 4;
const uint FLAG_USE_HEIGHT = 1 << 5;

const float PI = 3.14159265359;

vec3 sampleOffsetDirections[20] = vec3[]
(
    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
    vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
    vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
); 

// input from vertex shader
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 viewPos;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;
layout(location = 6) in vec4 verColor;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushModel {
    mat4 model;
} push;

layout(set = 1, binding = 0) uniform sampler2D m_albedo;
layout(set = 1, binding = 1) uniform sampler2D m_normal;
layout(set = 1, binding = 2) uniform sampler2D m_metallic;
layout(set = 1, binding = 3) uniform sampler2D m_roughness;
layout(set = 1, binding = 4) uniform sampler2D m_ao;
layout(set = 1, binding = 5) uniform sampler2D m_height;

layout(set = 1, binding = 6) uniform materialFlag {
    uint val;
} m_flag;


// Function declaration
vec2 ParallaxMapping(mat3 TBN);
vec3 CalNormalFromMap(vec2 texCoord, mat3 TBN);

vec3 CalcOutputColor(vec3 radiance, vec3 N, vec3 V, vec3 L, vec3 F0, vec3 albedo, float metallic, float roughness);

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(push.model)));
    vec3 sT = normalize(normalMatrix * tangent);
    vec3 sB = normalize(normalMatrix * bitangent);
    vec3 sN = normalize(normalMatrix * normal);
    mat3 TBN = mat3(sT, sB, sN);

    vec2 texCoord = (m_flag.val & FLAG_USE_HEIGHT) != 0 ? ParallaxMapping(TBN) : fragTexCoord;

    vec3 albedo = (m_flag.val & FLAG_USE_ALBEDO) != 0 ? texture(m_albedo, texCoord).rgb : verColor.rgb;
    vec3 normal = (m_flag.val & FLAG_USE_NORMAL) != 0 ? CalNormalFromMap(texCoord, TBN) : sN;
    float metallic = (m_flag.val & FLAG_USE_METALLIC) != 0 ? texture(m_metallic, texCoord).r : 0.0f;
    float roughness = (m_flag.val & FLAG_USE_ROUGHTNESS) != 0 ? texture(m_roughness, texCoord).r : 0.0f;
    float ao = (m_flag.val & FLAG_USE_AO) != 0 ? texture(m_ao, texCoord).r : 1.0f;

    // Get the genaric variables
    vec3 N = normalize(normal);
    vec3 V = normalize(viewPos - fragPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 result = vec3(0.0f, 0.0f, 0.0f);

    vec3 L = vec3(-80.0f, 0.0f, 0.0f);
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
    result +=  CalcOutputColor(lightColor, N, V, L, F0, albedo, metallic, roughness) * ao;

    outColor = vec4(result, 1.0f);
}

vec2 ParallaxMapping(mat3 TBN)
{
    mat3 inverseTBN = transpose(TBN);

    vec3 tangentViewPos = inverseTBN * viewPos;
    vec3 tangentFragPos = inverseTBN * fragPos;
    vec3 viewDir = normalize(tangentViewPos - tangentFragPos);
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * /*heightScale*/ 2.0f; 
    vec2 deltaTexCoord = P / numLayers;
    // Get initial values
    vec2 currentTexCoord = fragTexCoord;
    float currentDepthMapValue = 1.0f - texture(m_height, fragTexCoord).r;

    while (currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoord -= deltaTexCoord;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = 1.0f - texture(m_height, currentTexCoord).r;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoord = currentTexCoord + deltaTexCoord;

    // get depth after and before collision for linear interpolation
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = 1.0f - texture(m_height, prevTexCoord).r - currentLayerDepth + layerDepth;
    // Interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoord = prevTexCoord * weight + currentTexCoord * (1.0 - weight);
    return finalTexCoord;
}

vec3 CalNormalFromMap(vec2 texCoord, mat3 TBN)
{
    vec3 normal = texture(m_normal, texCoord).rgb;
    normal = normal * 2.0f - 1.0f;
    normal = normalize(TBN * normal);
    return normal;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

vec3 CalcOutputColor(vec3 radiance, vec3 N, vec3 V, vec3 L, vec3 F0, vec3 albedo, float metallic, float roughness)
{
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0f * max(dot(N, V), 0.0) * max(dot(N, L), 0.0)  + 0.0001;
    vec3 specular = numerator / denominator;     

    vec3 kS = F;
    vec3 kD = vec3(1.0f) - kS;
    kD *= 1.0f - metallic;

    float NdotL = max(dot(N, L), 0.0); 
    vec3 OutputColor = (kD * albedo / PI + specular) * radiance * NdotL;

    return OutputColor;
}