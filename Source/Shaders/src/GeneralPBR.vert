#version 460

#pragma shader_stage(vertex)

// Vertex input (from vertex buffer, via VkVertexInputAttributeDescription)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;
layout(location = 5) in vec4 inColor;

// Output to fragment shader
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragViewPos;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec3 fragTangent;
layout(location = 5) out vec3 fragBitangent;
layout(location = 6) out vec4 vertexColor;


// MVP uniform buffer
layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec3 camPos;
} global;

layout(push_constant) uniform PushModel {
    mat4 model;
} push;

void main()
{
    mat4 mvp = global.projection * global.view * push.model;
    gl_Position = mvp * vec4(inPosition, 1.0);

    fragPos = vec3(push.model * vec4(inPosition, 1.0f));
    fragTexCoord = inTexCoord;
    fragViewPos = global.camPos;
    fragNormal = inNormal;
    fragTangent = inTangent;
    fragBitangent = inBitangent;
    vertexColor = inColor;
}