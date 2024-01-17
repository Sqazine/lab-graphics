#version 450 core

#define MAX_DYNAMIC_LIGHT_COUNT 256

#define PARAMETER_ONLY 0x01
#define TEXTURE_ONLY 0x10
#define BLEND 0x11

struct Material
{
	vec3 albedoColor;
	sampler2D albedoMap;
	int albedoBlendType;

	sampler2D metalMap;
	float metallic;
	int metallicBlendType;

	sampler2D roughnessMap;
	float roughness;
	int roughnessBlendType;
	
	sampler2D normalMap;
	float bumpiness;
	int bumpinessBlendType;

	sampler2D aoMap;
	float ao;
	int aoBlendType;

	sampler2D emissiveMap;
	vec4 emissiveColor;
	int emissiveBlendType;

	float transparency;

	vec3 clearcoatColor;
 	float clearCoatGlossiness;
 	float clearcoatWeight;
 	float clearcoatThickness;

	vec3 fresnelColor;
	float fresnelWeight;
	float reflectance;

	float ior;

	samplerCube skyboxMap;
	samplerCube irradianceMap;
	samplerCube reflectionMap;
	vec3 environmentColor;
	int environmentBlendType;

	float alphaTransparency;
};

struct PointLight
{
	vec3 position;
	vec3 color;
};

layout(location=0) out vec4 fragColor;

layout(location=0) in vec3 vPosWS;
layout(location=1) in vec3 vNormalWS;
layout(location=2) in vec3 vTangentWS;
layout(location=3) in vec3 vBinormalWS;
layout(location=4) in vec2 vUv;

uniform sampler2D uBrdfLut;

uniform vec3 uViewPosWS;
uniform PointLight uPointLights[MAX_DYNAMIC_LIGHT_COUNT];
uniform Material pbrMaterial;

uniform float uExposure;
uniform int uToneMapType;

const float PI=3.14159265359;
const float ONE_OVER_PI = 0.3183099;

vec3 FresnelSchlick(float cosTheta,vec3 F0,float fresnelWeight)
{
	return F0+(fresnelWeight-F0)*pow(1.0-cosTheta,5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta,vec3 F0,float roughness,float fresnelWeight)
{
	return F0+(max(vec3(fresnelWeight-roughness),F0)-F0)*pow(1.0-cosTheta,5.0);
}

float NormalDistributionGGX(vec3 n,vec3 h,float roughness)
{
	float a=roughness*roughness;
	float a2=a*a;
	float NdotH=max(dot(n,h),0.0);
	float NdotH2=NdotH*NdotH;

	return a2/max((PI*pow((NdotH2*(a2-1.0)+1.0),2.0)),0.000001);
}

float GeometrySchlickGGX(float NdotV,float roughness)
{
	float r=(roughness+1.0);
	float k=(r*r)/8.0;

	return NdotV/(NdotV*(1.0-k)+k);
}

float GeometrySmith(vec3 n,vec3 v,vec3 l,float roughness)
{
	float NdotV=max(dot(n,v),0.0);
	float NdotL=max(dot(n,l),0.0);

	return GeometrySchlickGGX(NdotV,roughness)*GeometrySchlickGGX(NdotL,roughness);
}

vec3 ACESToneMapping(vec3 color,float exposure)
{
	 float tA = 2.51;
    float tB = 0.03;
    float tC = 2.43;
    float tD = 0.59;
    float tE = 0.14;
    vec3 x = color * exposure;
    return (x*(tA*x+tB))/(x*(tC*x+tD)+tE);
}

// ACES approximation by Stephen Hill
// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat = mat3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat = mat3(
     1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
);

vec3 RRTAndODTFit(vec3 v) {
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACES2ToneMapping(vec3 color,float exposure)
{
	color *= exposure;
    color = color * ACESInputMat;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);
    color = color * ACESOutputMat;

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return color;
}

vec3 ReinhardToneMapping(vec3 color,float exposure)
{
	color*=exposure;
	color/=(color+vec3(1.0));
	return color;
}

vec3 Uncharted2ToneMapping(vec3 x) {
	const float A =  0.15;
	const float B =  0.50;
	const float C =  0.10;
	const float D =  0.20;
	const float E =  0.02;
	const float F =  0.30;
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 FilmicToneMapping(vec3 color,float exposure)
{
	const float W =  11.2;

	color = Uncharted2ToneMapping(color * exposure);
    vec3 whiteScale = 1.0 / Uncharted2ToneMapping(vec3(W,W,W));
    color = color * whiteScale;

    return color;
}

vec3 LinearToneMapping(vec3 color,float exposure)
{
	return color*exposure;
}

vec3 GetNormal(sampler2D map,vec2 uv)
{
    vec3 tangentNormal=texture(map,uv).xyz*2.0-1.0;

    vec3 q1=dFdx(vPosWS);
    vec3 q2=dFdy(vPosWS);
    vec2 st1=dFdx(uv);
    vec2 st2=dFdy(uv);

    vec3 N=normalize(vNormalWS);
    vec3 T=normalize(q1*st2.t-q2*st1.t);
    vec3 B=-normalize(cross(N,T));
    mat3 TBN=mat3(T,B,N);

    return normalize(TBN*tangentNormal);
}

void main()
{
		vec3 uAlbedo=vec3(0.0);
		if(pbrMaterial.albedoBlendType==BLEND)
			uAlbedo=pow(texture(pbrMaterial.albedoMap,vUv),vec4(2.2)).rgb*pbrMaterial.albedoColor;
		else if(pbrMaterial.albedoBlendType==TEXTURE_ONLY)
			uAlbedo=pow(texture(pbrMaterial.albedoMap,vUv),vec4(2.2)).rgb;
		else if(pbrMaterial.albedoBlendType==PARAMETER_ONLY)
			uAlbedo=pbrMaterial.albedoColor;
		
		float uRoughness=texture(pbrMaterial.roughnessMap,vUv).r;
		if(pbrMaterial.roughnessBlendType==BLEND)
			uRoughness=texture(pbrMaterial.roughnessMap,vUv).r*pbrMaterial.roughness;
		else if(pbrMaterial.roughnessBlendType==TEXTURE_ONLY)
			uRoughness=texture(pbrMaterial.roughnessMap,vUv).r;
		else if(pbrMaterial.roughnessBlendType==PARAMETER_ONLY)
			uRoughness=pbrMaterial.roughness;

		float uAo=1.0;
		if(pbrMaterial.aoBlendType==BLEND)
			uAo=texture(pbrMaterial.aoMap,vUv).r*pbrMaterial.ao;
		else if(pbrMaterial.aoBlendType==TEXTURE_ONLY)
			uAo=texture(pbrMaterial.aoMap,vUv).r;
		else if(pbrMaterial.aoBlendType==PARAMETER_ONLY)
			uAo=pbrMaterial.ao;

		float uMetallic=1.0;
		if(pbrMaterial.metallicBlendType==BLEND)
			uMetallic=texture(pbrMaterial.metalMap,vUv).r*pbrMaterial.metallic;
		else if(pbrMaterial.metallicBlendType==TEXTURE_ONLY)
			uMetallic=texture(pbrMaterial.metalMap,vUv).r;
		else if(pbrMaterial.metallicBlendType==PARAMETER_ONLY)
			uMetallic=pbrMaterial.metallic;

		vec3 uEmissive=vec3(0.0,0.0,0.0);
		if(pbrMaterial.emissiveBlendType==BLEND)
			uEmissive=texture(pbrMaterial.emissiveMap,vUv).rgb*pbrMaterial.emissiveColor.rgb;
		else if(pbrMaterial.emissiveBlendType==TEXTURE_ONLY)
			uEmissive=texture(pbrMaterial.emissiveMap,vUv).rgb;
		else if(pbrMaterial.emissiveBlendType==PARAMETER_ONLY)
			uEmissive=pbrMaterial.emissiveColor.rgb;

		float transparency=1.0-pbrMaterial.transparency;

		//vec3 N=GetNormal(pbrMaterial.normalMap,vUv);
		vec3 N=vNormalWS;
		if(pbrMaterial.bumpinessBlendType==BLEND)
		{
			vec3 tangentNormal=mix(vec3(0.0,0.0,1.0),texture(pbrMaterial.normalMap,vUv).xyz*2.0-1.0,pbrMaterial.bumpiness);
    		vec3 q1=dFdx(vPosWS);
    		vec3 q2=dFdy(vPosWS);
    		vec2 st1=dFdx(vUv);
    		vec2 st2=dFdy(vUv);
    		vec3 N_=normalize(vNormalWS);
    		vec3 T=normalize(q1*st2.t-q2*st1.t);
    		vec3 B=-normalize(cross(N_,T));
    		mat3 TBN=mat3(T,B,N_);
    		N=normalize(TBN*tangentNormal);
		}
		else if(pbrMaterial.bumpinessBlendType==TEXTURE_ONLY)
			N=GetNormal(pbrMaterial.normalMap,vUv);
		else if(pbrMaterial.bumpinessBlendType==PARAMETER_ONLY)
			N=normalize(mix(vec3(0.0, 0.0, 1.0), N, pbrMaterial.bumpiness));

		vec3 irradiance_env=vec3(0.0);
		if(pbrMaterial.environmentBlendType==BLEND)
			irradiance_env=texture(pbrMaterial.irradianceMap,N).rgb*pbrMaterial.environmentColor;
		else if(pbrMaterial.environmentBlendType==TEXTURE_ONLY)
			irradiance_env=texture(pbrMaterial.irradianceMap,N).rgb;
		else if(pbrMaterial.environmentBlendType==PARAMETER_ONLY)
			irradiance_env=pbrMaterial.environmentColor;

		const float MAX_REFLECTION_LOD=4.0;
		vec3 V=normalize(uViewPosWS-vPosWS);
		vec3 R=reflect(-V,N);

		vec3 reflection_env=vec3(0.0);
		if(pbrMaterial.environmentBlendType==BLEND)
			reflection_env=textureLod(pbrMaterial.reflectionMap,R,uRoughness*MAX_REFLECTION_LOD).rgb*pbrMaterial.environmentColor;
		else if(pbrMaterial.environmentBlendType==TEXTURE_ONLY)
			reflection_env=textureLod(pbrMaterial.reflectionMap,R,uRoughness*MAX_REFLECTION_LOD).rgb;
		else if(pbrMaterial.environmentBlendType==PARAMETER_ONLY)
			reflection_env=pbrMaterial.environmentColor;

		vec3 skyboxColor=vec3(0.0);
		if(pbrMaterial.environmentBlendType==BLEND)
			skyboxColor=textureLod(pbrMaterial.skyboxMap,refract(-V,N,1.0/pbrMaterial.ior),pbrMaterial.roughness*MAX_REFLECTION_LOD).rgb*pbrMaterial.environmentColor;
		else if(pbrMaterial.environmentBlendType==TEXTURE_ONLY)
			skyboxColor=textureLod(pbrMaterial.skyboxMap,refract(-V,N,1.0/pbrMaterial.ior),pbrMaterial.roughness*MAX_REFLECTION_LOD).rgb.rgb;
		else if(pbrMaterial.environmentBlendType==PARAMETER_ONLY)
			skyboxColor=pbrMaterial.environmentColor;
	
		float NdotV_c=max(dot(vNormalWS,V),0.0);
		vec3 R_c=reflect(-V,vNormalWS);
		float clearcoatRoughness=(1.0-pbrMaterial.clearCoatGlossiness);

		vec3 refl_c=vec3(0.0);

		if(pbrMaterial.environmentBlendType==BLEND)
			refl_c=textureLod(pbrMaterial.reflectionMap,R_c,clearcoatRoughness*MAX_REFLECTION_LOD).rgb*pbrMaterial.environmentColor;
		else if(pbrMaterial.environmentBlendType==TEXTURE_ONLY)
			refl_c=textureLod(pbrMaterial.reflectionMap,R_c,clearcoatRoughness*MAX_REFLECTION_LOD).rgb;
		else if(pbrMaterial.environmentBlendType==PARAMETER_ONLY)
			refl_c=pbrMaterial.environmentColor;

		vec3 F0=vec3(0.16*pbrMaterial.reflectance*pbrMaterial.reflectance)*(1.0-uMetallic)+uAlbedo*uMetallic;

		vec3 diffuse_l=vec3(0.0);
		vec3 specular_l=vec3(0.0);
		for(int j=0;j<4;++j)
		{
			vec3 L=normalize(uPointLights[j].position-vPosWS);
			vec3 H=normalize(V+L);

			float distance=length(uPointLights[j].position-vPosWS);
			float attenuation=1.0/(distance*distance);
			vec3 irradiance_l=uPointLights[j].color*attenuation;

			float NDF=NormalDistributionGGX(N,H,uRoughness);
			float G=GeometrySmith(N,V,L,uRoughness);
			vec3 F_l=FresnelSchlick(max(dot(V,H),0.0),F0,pbrMaterial.fresnelWeight);

			vec3 kS_l=F_l;
			vec3 kD_l=vec3(1.0)-kS_l;
			kD_l*=1.0-uMetallic;

			float NdotL=max(dot(N,L),0.0);

			diffuse_l+=(kD_l*uAlbedo/PI)*irradiance_l*NdotL;
			specular_l+=(NDF*F_l*G/(4.0*max(dot(N,V),0.0)*max(dot(N,L),0.0)+0.001))*irradiance_l*NdotL;
		}
		vec3 light_dir=vec3(0.03)*uAlbedo*uAo+diffuse_l+specular_l;

		float NdotV=max(dot(N,V),0.0);

		vec3 F_env=FresnelSchlickRoughness(NdotV,F0,uRoughness,pbrMaterial.fresnelWeight);
		vec3 kD_env=1.0-F_env;
		kD_env*=1.0-uMetallic;

		vec3 diffuse_env=irradiance_env*uAlbedo;
	
		vec2 brdf_env=texture(uBrdfLut,vec2(NdotV,uRoughness)).rg;
		vec3 specular_env=reflection_env*(F_env*brdf_env.x+brdf_env.y);
		
		vec3 refractColor=skyboxColor*(1.0-F_env);

		vec3 light_env=(mix(kD_env*diffuse_env*uAo,refractColor,1.0-transparency)+specular_env*uAo);

		vec3 F_c=FresnelSchlickRoughness(NdotV_c,F0,clearcoatRoughness,pbrMaterial.clearcoatThickness);
		vec2 brdf_c=texture(uBrdfLut,vec2(NdotV_c,clearcoatRoughness)).rg;
		vec3 clearcoat=pbrMaterial.clearcoatWeight*refl_c*pbrMaterial.clearcoatColor*(F_c*brdf_c.x+brdf_c.y);

		vec3 color=clearcoat+(1-pbrMaterial.clearcoatWeight*F_c.x)*(light_env+light_dir);

		color.rgb+=uEmissive;

		if(uToneMapType==0)
			color=LinearToneMapping(color,uExposure);
		else if(uToneMapType==1)
			color=ReinhardToneMapping(color,uExposure);
		else if(uToneMapType==2)
			color=FilmicToneMapping(color,uExposure);
		else if(uToneMapType==3)
			color=ACESToneMapping(color,uExposure);
		else if(uToneMapType==4)
			color=ACES2ToneMapping(color,uExposure);


		fragColor.rgb=pow(color,vec3(1.0/2.2));

		fragColor.a=(1.0-pbrMaterial.alphaTransparency);
}