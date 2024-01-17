#version 450

layout(location=0) out vec4 fragColor;

layout(location=0) in vec2 texcoord;
layout(location=1) in vec3 normalWS;
layout(location=2) in vec3 posWS;

layout(binding=0) uniform samplerCube cubemap;

uniform vec3 viewPosWS;

vec4 ReinhardToneMapping(vec4 color)
{
	color.rgb/=(color.rgb+vec3(1.0));
	return color;
}

void main()
{
    vec3 i=normalize(posWS-viewPosWS);
    vec3 r=reflect(i,normalize(normalWS));

    fragColor=ReinhardToneMapping(texture(cubemap,r));
    fragColor.rgb=pow(fragColor.rgb,vec3(0.45));
}