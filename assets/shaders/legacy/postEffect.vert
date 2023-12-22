#version 450 core

layout(location=0) in vec3 inPosition;

layout(location=0) out vec2 texcoord;

void main()
{
    gl_Position=vec4(inPosition,1.0);
    texcoord=vec2(inPosition.x,inPosition.y)*0.5+0.5;
}