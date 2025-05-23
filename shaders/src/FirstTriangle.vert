#version 460

#pragma shader_stage(vertex)

vec2 positions[3] = {
    {    0, -.5f },
    { -.5f,  .5f },
    {  .5f,  .5f }
};

int add(int i)
{
    return i;
}

void main()
{
    int b = add(1);
    gl_Position = vec4(positions[gl_VertexIndex], 0, 1);   
}