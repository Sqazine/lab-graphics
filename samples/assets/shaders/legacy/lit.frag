#version 450 core

out vec4 fragColor;

layout(location=0) in vec3 normalWS;
layout(location=1) in vec3 fragPosWS;
layout(location=2) in vec2 uv;

uniform vec3 light;
uniform sampler2D tex0;


void main()
{
	vec4 diffuseColor=texture(tex0,uv);
	vec3 n=normalize(normalWS);
	vec3 l=normalize(light);
	float diffuseIntensity=max(dot(n,l),0.0)+0.1;
	fragColor=diffuseColor*diffuseIntensity;
}