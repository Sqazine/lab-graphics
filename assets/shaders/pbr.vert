#version 450 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texcoord;

layout(set=0,binding=0) uniform TransformUniforms
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    mat4 skyboxRotationMatrix;
};

layout(location=0) out Vertex
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
}outVertex;

void main()
{
    outVertex.position=position;
    outVertex.texcoord=vec2(texcoord.x,texcoord.y);
    outVertex.normal=normal;

    gl_Position=projectionMatrix*viewMatrix*vec4(position,1.0);
}