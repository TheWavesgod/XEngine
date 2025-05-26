#version 450

// input from vertex shader
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragViewPos;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;
layout(location = 6) in vec4 verColor;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushModel {
    mat4 model;
} push;

const float PI = 3.14159265359;

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(push.model)));
    vec3 sT = normalize(normalMatrix * tangent);
    vec3 sB = normalize(normalMatrix * bitangent);
    vec3 sN = normalize(normalMatrix * normal);
    mat3 TBN = mat3(sT, sB, sN);

    vec2 texCoord = fragTexCoord;

    outColor = verColor;
}