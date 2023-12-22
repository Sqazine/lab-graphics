#version 450 core
out vec4 fragColor;

layout(location=0) in vec3 posOS;

layout(binding=0)uniform samplerCube environmentMap;

void main()
{
	vec3 envColor=textureLod(environmentMap,posOS,1).rgb;

	envColor=envColor/(envColor+vec3(1.0));
	envColor=pow(envColor,vec3(1/2.2));
	fragColor=vec4(envColor,1.0);
}