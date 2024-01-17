#version 450 core
layout(location=0) in vec3 inPosition;

layout(location=0) out vec3 posOS;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;


void main()
{
	posOS=inPosition;
	gl_Position=projectionMatrix*viewMatrix*vec4(inPosition,1.0);
}