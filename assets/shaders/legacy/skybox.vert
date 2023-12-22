#version 450 core
layout(location=0) in vec3 inPosition;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

layout(location=0) out vec3 posOS;

void main()
{
	posOS=inPosition;

	mat4 rotView=mat4(mat3(viewMatrix));
	vec4 posCS=projectionMatrix*rotView*vec4(inPosition,1.0);
	gl_Position=posCS.xyww;
}