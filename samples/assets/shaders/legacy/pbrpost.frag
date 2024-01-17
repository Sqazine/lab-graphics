#version 450 core

layout(location=0) out vec4 fragColor;

layout(location=0) in vec2 texcoord;

layout(binding=0) uniform samplerCube uIrradianceMap;
layout(binding=1) uniform samplerCube uReflectionMap;
layout(binding=2) uniform sampler2D uBrdfLut;

layout(binding=3) uniform sampler2D gPositionWS;
layout(binding=4) uniform sampler2D gAlbedo;
layout(binding=5) uniform sampler2D gAoMetallicRoughness;
layout(binding=6) uniform sampler2D gVertexNormalWS;
layout(binding=7) uniform sampler2D gNormalMapNormalWS;
layout(binding=8) uniform sampler2D gClearcoatColor;
layout(binding=9) uniform sampler2D gClearcoatRoughnessWeight;

// layout(binding=10) uniform sampler2D ssao;
// layout(binding=11) uniform sampler2D ssr;

uniform vec3 uViewPosWS;
uniform vec3 uLightPositions[4];
uniform vec3 uLightColors[4];

uniform float uExposure;
uniform int uToneMapType;

const float PI=3.14159265359;
const float ONE_OVER_PI = 0.3183099;

vec3 FresnelSchlick(float cosTheta,vec3 F0)
{
	return F0+(1-F0)*pow(1.0-cosTheta,5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta,vec3 F0,float roughness)
{
	return F0+(max(vec3(1.0-roughness),F0)-F0)*pow(1.0-cosTheta,5.0);
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

void main()
{
	vec3 normalWS=texture(gVertexNormalWS,texcoord).xyz;
	vec3 posWS=texture(gPositionWS,texcoord).xyz;

	vec3 uAlbedo=texture(gAlbedo,texcoord).xyz;
	float uAo=texture(gAoMetallicRoughness,texcoord).r;
	vec3 uMetallic=vec3(texture(gAoMetallicRoughness,texcoord).g);
	float uRoughness=texture(gAoMetallicRoughness,texcoord).b;

	vec3 uClearCoatColor=texture(gClearcoatColor,texcoord).rgb;
	float uClearCoatRoughness=texture(gClearcoatRoughnessWeight,texcoord).r;
	float uClearCoatWeight=texture(gClearcoatRoughnessWeight,texcoord).g;

	vec3 N=texture(gNormalMapNormalWS,texcoord).xyz;
	vec3 V=normalize(uViewPosWS-posWS);
	vec3 R=reflect(-V,N);

	vec3 F0=vec3(0.04);
	F0=mix(F0,uAlbedo,uMetallic);

	vec3 diffuse_l=vec3(0.0f);
	vec3 specular_l=vec3(0.0f);
	for(int i=0;i<4;++i)
	{
		vec3 L=normalize(uLightPositions[i]-posWS);
		vec3 H=normalize(V+L);

		float distance=length(uLightPositions[i]-posWS);
		float attenuation=1.0/(distance*distance);
		vec3 irradiance_l=uLightColors[i]*attenuation;

		float NDF=NormalDistributionGGX(N,H,uRoughness);
		float G=GeometrySmith(N,V,L,uRoughness);
		vec3 F_l=FresnelSchlick(max(dot(V,H),0.0),F0);

		vec3 kS_l=F_l;
		vec3 kD_l=vec3(1.0)-kS_l;
		kD_l*=1.0-uMetallic;

		float NdotL=max(dot(N,L),0.0);

		diffuse_l+=kD_l*uAlbedo/PI*irradiance_l*NdotL;
		specular_l+=NDF*F_l*G/(4.0*max(dot(N,V),0.0)*max(dot(N,L),0.0)+0.001)*irradiance_l*NdotL;
	}
	vec3 light_dir=diffuse_l+specular_l;

	float NdotV=max(dot(N,V),0.0);

	vec3 F_env=FresnelSchlickRoughness(NdotV,F0,uRoughness);
	vec3 kD_env=1.0-F_env;
	kD_env*=1.0-uMetallic;

	vec3 irradiance_env=texture(uIrradianceMap,N).rgb;
	vec3 diffuse_env=irradiance_env*uAlbedo;
	
	const float MAX_REFLECTION_LOD=4.0;
	vec3 reflection_env=textureLod(uReflectionMap,R,uRoughness*MAX_REFLECTION_LOD).rgb;
	vec2 brdf_env=texture(uBrdfLut,vec2(NdotV,uRoughness)).rg;
	vec3 specular_env=reflection_env*(F_env*brdf_env.x+brdf_env.y);
	
	vec3 light_env=(kD_env*diffuse_env+specular_env)*uAo;

	float NdotV_c=max(dot(normalWS,V),0.0);
	vec3 R_c=reflect(-V,normalWS);

	vec3 clearcoat=vec3(0.0,0.0,0.0);

	vec3 F_c=FresnelSchlickRoughness(NdotV_c,F0,uClearCoatRoughness);
	vec3 refl_c=textureLod(uReflectionMap,R_c,uClearCoatRoughness*MAX_REFLECTION_LOD).rgb;
	vec2 brdf_c=texture(uBrdfLut,vec2(NdotV_c,uClearCoatRoughness)).rg;
	clearcoat=uClearCoatWeight*refl_c*uClearCoatColor*(F_c*brdf_c.x+brdf_c.y);

	vec3 color=clearcoat+(1-uClearCoatWeight*F_c.x)*(light_env+light_dir)/*+texture(ssr,texcoord).rgb*/;

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

	color=pow(color,vec3(1.0/2.2));

	fragColor=vec4(color,1.0);
}