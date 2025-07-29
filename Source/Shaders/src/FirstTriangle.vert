#version 460

#pragma shader_stage(vertex)

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 5) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    gl_Position = vec4(inPosition, 1);
    outColor = inColor;
}