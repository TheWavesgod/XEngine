#version 450

layout(binding = 0) uniform sampler2D hdrImage;

layout(push_constant) uniform PushConstants {
    float exposure;
} pushConstants;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

vec3 ACESFilm(vec3 x) 
{
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main()
{
    vec3 hdrColor = texture(hdrImage, fragUV).rgb;

    vec3 color = hdrColor * pushConstants.exposure;

    // Tone Mapping (ACES)
    color = ACESFilm(color);

    // Gamma correction (sRGB)
    const float gamma = 2.2f;
    color = pow(color, vec3(1.0f / gamma));

    outColor = vec4(color, 1.0f);
}