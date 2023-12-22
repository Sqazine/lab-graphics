#version 450 core

layout(location=0) out vec2 texcoord;

const vec3 position[6]={
    vec3(-1.0f, 1.0f, 0.0f),
    vec3(-1.0f, -1.0f, 0.0f),
    vec3(1.0f, -1.0f, 0.0f),
    vec3(-1.0f, 1.0f, 0.0f),
    vec3(1.0f, -1.0f, 0.0f),
    vec3(1.0f, 1.0f, 0.0f),
};

void main()
{
    gl_Position=vec4(position[gl_VertexID],1.0);
    texcoord=vec2(position[gl_VertexID].x,position[gl_VertexID].y)*0.5+0.5;
}