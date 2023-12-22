#version 450 core
layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inTexcoord;
layout(location=2) in vec3 inNormal;

layout(location=0) out vec3 posOS;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;


void main()
{
	posOS=inPosition;

	gl_Position=projectionMatrix*viewMatrix*vec4(inPosition,1.0);
}