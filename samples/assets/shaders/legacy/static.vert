#version 450 core

layout(location=0) in vec3 positionOS;
layout(location=1) in vec3 normalOS;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec3 tangentOS;
layout(location=4) in vec3 binormalOS;
layout(location=5) in vec4 color;

layout(location=0) out vec3 normalWS;
layout(location=1) out vec3 fragPosWS;
layout(location=2) out vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position=projection*view*model*vec4(positionOS,1.0);
	fragPosWS=vec3(model*vec4(positionOS,1.0));
	normalWS=vec3(model*vec4(normalOS,0.0));
	uv=texcoord;
}