
#version 450 core
out vec4 fragColor;

layout(location=0) in vec3 posOS;

layout(binding=0) uniform samplerCube environmentMap;
uniform float roughness;

const float PI=3.14159265359;

float NormalDistributionGGX(vec3 n,vec3 h,float roughness)
{
	float a=roughness*roughness;
	float a2=a*a;
	float NdotH=max(dot(n,h),0.0);
	float NdotH2=NdotH*NdotH;

	return a2/max((PI*pow((NdotH2*(a2-1.0)+1.0),2.0)),0.000001);
}


float RadicalInverse_Vdc(uint bits)
{
	bits=(bits<<16u)|(bits>>16u);
	bits=((bits&0x55555555u)<<1u)|((bits&0xAAAAAAAAu)>>1u);
	bits=((bits&0x33333333u)<<2u)|((bits&0xCCCCCCCCu)>>2u);
	bits=((bits&0x0F0F0F0Fu)<<4u)|((bits&0xF0F0F0F0u)>>4u);
	bits=((bits&0x00FF00FFu)<<8u)|((bits&0xFF00FF00u)>>8u);
	return float(bits)*2.3283064365386963e-10;
}

vec2 Hammersley(uint i,uint N)
{
	return vec2(float(i)/float(N),RadicalInverse_Vdc(i));
}

vec3 importanceSampleGGX(vec2 Xi,vec3 N,float roughness)
{
	float a=roughness*roughness;

	float phi=2.0*PI*Xi.x;
	float cosTheta=sqrt((1.0-Xi.y)/(1.0+(a*a-1.0)*Xi.y));
	float sinTheta=sqrt(1.0-cosTheta*cosTheta);

	//从球体坐标转换到笛卡尔坐标
	vec3 H;
	H.x=cos(phi)*sinTheta;
	H.y=sin(phi)*sinTheta;
	H.z=cosTheta;

	//从切线坐标转换到世界坐标
	vec3 up=abs(N.z)<0.999?vec3(0.0,0.0,1.0):vec3(1.0,0.0,0.0);
	vec3 tangent=normalize(cross(up,N));
	vec3 bitangent=cross(N,tangent);

	vec3 sampleVec=tangent*H.x+bitangent*H.y+N*H.z;
	return normalize(sampleVec);
}

void main()
{
	vec3 N=normalize(posOS);
	vec3 R=N;
	vec3 V=R;

	const uint SAMPLE_COUNT=1024u;
	float totalWeight=0.0;
	vec3 prefilteredColor=vec3(0.0);
	for(uint i=0u;i<SAMPLE_COUNT;++i)
	{
		vec2 Xi=Hammersley(i,SAMPLE_COUNT);
		vec3 H=importanceSampleGGX(Xi,N,roughness);
		vec3 L=normalize(2.0*dot(V,H)*H-V);

		float NdotL=max(dot(N,L),0.0);
		if(NdotL>0.0)
		{
			float NDF=NormalDistributionGGX(N,H,roughness);

			float NdotH=max(dot(N,H),0.0);
			float HdotV=max(dot(H,V),0.0);

			float pdf=(NDF*NdotH/(4.0*HdotV))+0.0001;

			float resolution=512.0;
			float saTexel=4.0*PI/(6.0*resolution*resolution);
			float saSample=1.0/(float(SAMPLE_COUNT)*pdf+0.0001);

			float mipLevel=roughness==0.0?0.0:0.5*log2(saSample/saTexel);

			prefilteredColor+=textureLod(environmentMap,L,mipLevel).rgb*NdotL;
			totalWeight+=NdotL;
		}
	}
	prefilteredColor=prefilteredColor/totalWeight;
	fragColor=vec4(prefilteredColor,1.0);
}