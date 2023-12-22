#version 450 core
#define MAX_MATERIAL_COUNT 4

#define UV_REVERSE_HORIZONTAL 0
#define UV_REVERSE_VERTICAL 1
#define UV_REVERSE_BOTH 2
#define UV_REVERSE_NONE 3

#define UV_TILING_HORIZONTAL 0
#define UV_TILING_VERTICAL 1
#define UV_TILING_BOTH 2
#define UV_TILING_NONE 3

#define UV_MAPPING_TYPE_UV 0
#define UV_MAPPING_TYPE_SPHERE 1
#define UV_MAPPING_TYPE_CUBE 2
#define UV_MAPPING_TYPE_PLANE 3
#define UV_MAPPING_TYPE_CYLINDER 4
#define UV_MAPPING_TYPE_CAMERA 5
#define UV_MAPPING_TYPE_CAMERA_ADAPTIVE 6

struct MaterialsUV
{
    vec4 albedoMapUv[MAX_MATERIAL_COUNT];
    vec4 normalMapUv[MAX_MATERIAL_COUNT];
    vec4 metalMapUv[MAX_MATERIAL_COUNT];
    vec4 roughnessMapUv[MAX_MATERIAL_COUNT];
    vec4 aoMapUv[MAX_MATERIAL_COUNT];
};

struct Material
{
	sampler2D albedoMap;
	sampler2D normalMap;
	sampler2D metalMap;
	sampler2D roughnessMap;
	sampler2D aoMap;

	 vec3 uClearCoatColor;
 	float uClearCoatRoughness;
 	float uClearCoatWeight;
};


struct uvParam
{
	int mappingType;
    vec2 tiling;
	vec2 offset;
    float radian;

    mat4 mappingMatrix;

	vec3 planeMappingCameraFront;
	float planeMappingDepth;
	vec3 gizmoPlanePosWS;//平面映射的原点

    int reverseMode;
    int tilingMode;
};
layout(location=0) out vec4 fragColor;

layout(location=0) in vec3 vPosWS;
layout(location=1) in vec3 vNormalWS;
layout(location=2) in vec3 vTangentWS;
layout(location=3) in vec3 vBinormalWS;
layout(location=4) in vec3 vPos;
layout(location=5) in MaterialsUV materialsUV; 


uniform Material materials[MAX_MATERIAL_COUNT];
uniform samplerCube uIrradianceMap;
uniform samplerCube uReflectionMap;
uniform sampler2D uBrdfLut;

uniform int materialCount;
uniform uvParam albedoMapParam[MAX_MATERIAL_COUNT];
uniform uvParam normalMapParam[MAX_MATERIAL_COUNT];
uniform uvParam metalMapParam[MAX_MATERIAL_COUNT];
uniform uvParam roughnessMapParam[MAX_MATERIAL_COUNT];
uniform uvParam aoMapParam[MAX_MATERIAL_COUNT];

bool albedoMapClampToBorder[MAX_MATERIAL_COUNT];
bool normalMapClampToBorder[MAX_MATERIAL_COUNT];
bool metalMapClampToBorder[MAX_MATERIAL_COUNT];
bool roughnessMapClampToBorder[MAX_MATERIAL_COUNT];
bool aoMapClampToBorder[MAX_MATERIAL_COUNT];

uniform vec3 uViewPosWS;
uniform vec3 uLightPositions[4];
uniform vec3 uLightColors[4];

uniform float uExposure;
uniform int uToneMapType;

vec2 uvReverse(vec2 originUV,int reverseMode)
{
	if(reverseMode==UV_REVERSE_HORIZONTAL)
		return vec2(1.0-originUV.x,originUV.y);
	else if(reverseMode==UV_REVERSE_VERTICAL)
		return vec2(originUV.x,1.0-originUV.y);
	else if(reverseMode==UV_REVERSE_BOTH)
		return 1.0-originUV;
	else return originUV;
}

//前提，传进来的纹理的wrap模式得是repeat
vec2 uvTiling(inout bool isClampToBorder,vec2 originUV,int tilingMode)
{
	if(tilingMode==UV_TILING_HORIZONTAL)
	{
		if(originUV.y<0.0||originUV.y>=1.0)
			isClampToBorder=true;
	}
	else if(tilingMode==UV_TILING_VERTICAL)
	{
		if(originUV.x<0.0||originUV.x>=1.0)
			isClampToBorder=true;
	}
	else if(tilingMode==UV_TILING_BOTH)
		return originUV;
	else 
	{
		if(originUV.y<0.0||originUV.y>=1.0||originUV.x<0.0||originUV.x>=1.0)
			isClampToBorder=true;
	}
	return originUV;
}

vec2 uvRotate(vec2 originUV,float radian)
{
	mat2 rotMat;
	rotMat[0][0]=rotMat[1][1]=cos(radian);
	rotMat[1][0]=sin(radian);
	rotMat[0][1]=-sin(radian);
	return rotMat*originUV;
}

vec2 GetUV_UV(uvParam param,inout bool isClampToBorder,vec4 texcoord)
{
     vec2 uv=uvRotate((texcoord.xy)*param.tiling+param.offset,param.radian);
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(isClampToBorder,uv,param.tilingMode);
    return uv;
}

vec2 GetUV_Sphere(uvParam param,inout bool isClampToBorder,vec4 texcoord)
{
	const vec2 invAtan=vec2(0.1591,0.3183);
	vec2 uv=vec2(atan(texcoord.z,texcoord.x),asin(texcoord.y));
	uv*=invAtan;
	uv+=0.5;
	
    uv=uvRotate(uv*param.tiling+param.offset,param.radian);
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(isClampToBorder,uv,param.tilingMode);

	return uv;
}

vec2 GetUV_Plane(uvParam param,inout bool isClampToBorder,vec4 texcoord,vec3 posWS)
{
    //将当前片元与物体中中心坐标结合起来，投影到相机的前向向量中，获取当前片元在相机前向向量上的投影标量，并与之对比depth值
	float cos_theta=dot(normalize(param.planeMappingCameraFront),normalize(posWS-param.gizmoPlanePosWS));
	float width=abs(length(posWS-param.gizmoPlanePosWS)*cos_theta);

	if(width>param.planeMappingDepth&&param.planeMappingDepth>0.0)
		isClampToBorder=true;

	vec2 uv=uvRotate(texcoord.xy+param.offset,param.radian);
	uv=uv*0.5+0.5;//先旋转再平移
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(isClampToBorder,uv,param.tilingMode);
    return uv;
}


vec2 GetUV_Cylinder(uvParam param,inout bool isClampToBorder,vec4 texcoord)
{
    const float PI=3.1415926535;
	vec2 uv=vec2(atan(texcoord.z,texcoord.x)+PI,texcoord.y);

    uv=uvRotate(uv*param.tiling+param.offset,param.radian);
    uv=uv*0.5+0.5;//先旋转再平移
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(isClampToBorder,uv,param.tilingMode);

	return uv;
}

vec2 GetUV_Cube(uvParam param,inout bool isClampToBorder,vec4 texcoord)
{
	vec3 mappingPosition3=vec3(param.mappingMatrix[3][0],param.mappingMatrix[3][1],param.mappingMatrix[3][2]);
 	param.mappingMatrix[3][0]=param.mappingMatrix[3][1]=param.mappingMatrix[3][2]=0.0;
	vec2 uv;
	texcoord=transpose(inverse(param.mappingMatrix))*texcoord;
	float mag=max(max(abs(texcoord.x),abs(texcoord.y)),abs(texcoord.z));
	if(mag==abs(texcoord.x))
	{
		vec2 mappingPosition=vec2(mappingPosition3.z,mappingPosition3.y);
		vec3 fullUv=mat3(param.mappingMatrix)*(vPos)+texcoord.xyz;
		if(fullUv.x>0)
		{
			uv=uvRotate(vec2(fullUv.z,fullUv.y)*param.tiling+param.offset+mappingPosition,param.radian);
			uv=uv*0.5+0.5;
			uv.x=1.0-uv.x;
		}
		else if(fullUv.x<0)
		{
			uv=uvRotate(vec2(fullUv.z,fullUv.y)*param.tiling+param.offset+mappingPosition,param.radian);
			uv=uv*0.5+0.5;
		}
	}
	else if(mag==abs(texcoord.y))
	{
		vec2 mappingPosition=vec2(mappingPosition3.x,mappingPosition3.z);
	vec3 fullUv=mat3(param.mappingMatrix)*vPos+texcoord.xyz;
		if(fullUv.y>0)
		{
			uv=uvRotate(vec2(fullUv.x,fullUv.z)*param.tiling+param.offset+mappingPosition,param.radian);
			uv=uv*0.5+0.5;
			uv.y=1.0-uv.y;
		}
		else if(fullUv.y<0)
		{
			uv=uvRotate(vec2(fullUv.x,fullUv.z)*param.tiling+param.offset+mappingPosition,param.radian);
			uv=uv*0.5+0.5;
		}
	}
	else if(mag==abs(texcoord.z))
	{
		vec2 mappingPosition=vec2(mappingPosition3.x,mappingPosition3.y);
		vec3 fullUv=mat3(param.mappingMatrix)*vPos+texcoord.xyz;
		if(fullUv.z>0)
		{
			uv=uvRotate(vec2(fullUv.x,fullUv.y)*param.tiling+param.offset+mappingPosition,param.radian);
			uv=uv*0.5+0.5;
		}
		else if(fullUv.z<0)
		{
			uv=uvRotate(vec2(fullUv.x,fullUv.y)*param.tiling+param.offset+mappingPosition,param.radian);
			uv=uv*0.5+0.5;
			uv.x=1.0-uv.x;
		}
	}

	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(isClampToBorder,uv,param.tilingMode);
	return uv;
}

vec2 GetUV_Camera(uvParam param,inout bool isClampToBorder,vec4 texcoord)
{
	vec4 tmpTexcoord=texcoord;
	tmpTexcoord.xyz/=tmpTexcoord.w;
	tmpTexcoord.xyz=tmpTexcoord.xyz*0.5+0.5;
    vec2 tmpUV=(tmpTexcoord.xy-0.5)*param.tiling;
	vec2 uv=uvRotate(tmpUV-param.offset,param.radian);
	uv+=0.5;
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(isClampToBorder,uv,param.tilingMode);
    return uv;
}

vec2 GetUV_CameraAdaptive(uvParam param,inout bool isClampToBorder,vec4 texcoord)
{
    vec2 uv=uvRotate(texcoord.xy+param.offset,param.radian);
	uv=uv*0.5+0.5;//先旋转再平移
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(isClampToBorder,uv,param.tilingMode);
    return uv;
}

vec2 GetUV(uvParam param,inout bool isClampToBorder,vec4 texcoord,vec3 posWS)
{
	if(param.mappingType==UV_MAPPING_TYPE_CUBE)
		return GetUV_Cube(param,isClampToBorder,texcoord);
	else if(param.mappingType==UV_MAPPING_TYPE_SPHERE)
		return GetUV_Sphere(param,isClampToBorder,texcoord);
	else if(param.mappingType==UV_MAPPING_TYPE_CYLINDER)
		return GetUV_Cylinder(param,isClampToBorder,texcoord);
	else if(param.mappingType==UV_MAPPING_TYPE_PLANE)
		return GetUV_Plane(param,isClampToBorder,texcoord,posWS);
	else if(param.mappingType==UV_MAPPING_TYPE_CAMERA)
		return GetUV_Camera(param,isClampToBorder,texcoord);
	else if(param.mappingType==UV_MAPPING_TYPE_CAMERA_ADAPTIVE)
		return GetUV_CameraAdaptive(param,isClampToBorder,texcoord);
	else return GetUV_UV(param,isClampToBorder,texcoord);
}

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

vec3 GetNormal(sampler2D map,uvParam param,inout bool isClampToBorder,vec4 uv)
{
    vec2 normalUv=GetUV(param,isClampToBorder,uv,vPosWS);
	if(isClampToBorder)
		return vNormalWS;

    vec3 tangentNormal=texture(map,normalUv).xyz*2.0-1.0;

    vec3 q1=dFdx(vPosWS);
    vec3 q2=dFdy(vPosWS);
    vec2 st1=dFdx(normalUv);
    vec2 st2=dFdy(normalUv);

    vec3 N=normalize(vNormalWS);
    vec3 T=normalize(q1*st2.t-q2*st1.t);
    vec3 B=-normalize(cross(N,T));
    mat3 TBN=mat3(T,B,N);

    return normalize(TBN*tangentNormal);
}

void main()
{
	for(int i=0;i<materialCount;++i)
	{
		albedoMapClampToBorder[i]=false;
		normalMapClampToBorder[i]=false;
		metalMapClampToBorder[i]=false;
		roughnessMapClampToBorder[i]=false;
		aoMapClampToBorder[i]=false;
	}

	vec4 allColors[MAX_MATERIAL_COUNT];//留一个alpha通道用作颜色混合
	for(int i=0;i<materialCount;++i)
	{
		vec2 albedoUV=GetUV(albedoMapParam[i],albedoMapClampToBorder[i],materialsUV.albedoMapUv[i],vPosWS);
		vec3 uAlbedo=vec3(-1.0);
		if(!albedoMapClampToBorder[i])
		    uAlbedo=texture(materials[i].albedoMap,albedoUV).rgb;

		vec2 metallicUV=GetUV(metalMapParam[i], metalMapClampToBorder[i],materialsUV.metalMapUv[i],vPosWS);
		float uMetallic=-1.0;
		if(!metalMapClampToBorder[i])
		    uMetallic=texture(materials[i].metalMap,metallicUV).r;
		
		vec2 roughnessUV=GetUV(roughnessMapParam[i],roughnessMapClampToBorder[i],materialsUV.roughnessMapUv[i],vPosWS);
		float uRoughness=-1.0;
		if(!roughnessMapClampToBorder[i])
		    uRoughness=texture(materials[i].roughnessMap,roughnessUV).r;

		vec2 aoUV=GetUV(aoMapParam[i],aoMapClampToBorder[i],materialsUV.aoMapUv[i],vPosWS);
		float uAo=-1.0;
		if(!aoMapClampToBorder[i])
			uAo= texture(materials[i].aoMap,aoUV).r;
		
		vec3 N=GetNormal(materials[i].normalMap,normalMapParam[i],normalMapClampToBorder[i],materialsUV.normalMapUv[i]);

		vec3 V=normalize(uViewPosWS-vPosWS);
		vec3 R=reflect(-V,N);

		vec3 F0=vec3(0.04);
		F0=mix(F0,uAlbedo,uMetallic);

		vec3 diffuse_l=vec3(0.0);
		vec3 specular_l=vec3(0.0);
		for(int j=0;j<4;++j)
		{
			vec3 L=normalize(uLightPositions[j]-vPosWS);
			vec3 H=normalize(V+L);

			float distance=length(uLightPositions[i]-vPosWS);
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

		float NdotV_c=max(dot(vNormalWS,V),0.0);
		vec3 R_c=reflect(-V,vNormalWS);

		vec3 clearcoat=vec3(0.0,0.0,0.0);

		vec3 F_c=FresnelSchlickRoughness(NdotV_c,F0,materials[i].uClearCoatRoughness);
		vec3 refl_c=textureLod(uReflectionMap,R_c,materials[i].uClearCoatRoughness*MAX_REFLECTION_LOD).rgb;
		vec2 brdf_c=texture(uBrdfLut,vec2(NdotV_c,materials[i].uClearCoatRoughness)).rg;
		clearcoat=materials[i].uClearCoatWeight*refl_c*materials[i].uClearCoatColor*(F_c*brdf_c.x+brdf_c.y);

		vec3 color=clearcoat+(1-materials[i].uClearCoatWeight*F_c.x)*(light_env+light_dir)/*+texture(ssr,texcoord).rgb*/;

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

		allColors[i].rgb=pow(color,vec3(1.0/2.2));
		if(uAlbedo==vec3(-1.0))
			allColors[i].a=0.0;
		else allColors[i].a=1.0;
	}

	for(int i=materialCount-1;i>=0;--i)
		if(allColors[i].a!=0.0)
		{	
			fragColor=allColors[i];
			break;
		}

	//避免出现黑色情况
	if(allColors[0].a==0.0)
		fragColor=vec4(allColors[0].rgb,1.0);
}