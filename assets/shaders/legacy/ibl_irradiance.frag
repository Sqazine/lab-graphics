 #version 450 core

out vec4 fragColor;

layout(location=0) in vec3 posOS;

layout(binding=0) uniform samplerCube environmentMap;

const float PI=3.14159265359;

void main()
{
	vec3 normal=normalize(posOS);

	vec3 irradiance=vec3(0.0);

	//对天空盒进行卷积,还不太理解
	vec3 up=vec3(0.0,1.0,0.0);
	vec3 right=cross(up,normal);
	up=cross(normal,right);

	float sampleDelta=0.025;
	float nrSamples=0.0;

	for(float phi=0.0;phi<2.0*PI;phi+=sampleDelta)
	{
		for(float theta=0.0;theta<0.5*PI;theta+=sampleDelta)
		{
			vec3 tangentSample=vec3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta));

			vec3 sampleVec=tangentSample.x*right+tangentSample.y*up+tangentSample.z*normal;

			irradiance+=texture(environmentMap,sampleVec).rgb*cos(theta)*sin(theta);

			nrSamples++;
		}
	}

	irradiance=PI*irradiance*(1.0/float(nrSamples));

	fragColor=vec4(irradiance,1.0);
}