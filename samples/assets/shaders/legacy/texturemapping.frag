#version 450 core
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

layout(location=0) out vec4 fragColor;

layout(location=0) in vec3 posWS;
layout(location=1) in vec4 uv;
layout(location=2) in vec3 vPos;

layout(binding=0) uniform sampler2D mappingTexture;

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

uniform uvParam textureMappingParam;

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

bool isClampToBorder=false;
//前提，传进来的纹理的wrap模式得是repeat
vec2 uvTiling(vec2 originUV,int tilingMode)
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

vec2 GetUV_UV(uvParam param,vec4 texcoord)
{
     vec2 uv=uvRotate((texcoord.xy)*param.tiling+param.offset,param.radian);
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(uv,param.tilingMode);
    return uv;
}

vec2 GetUV_Sphere(uvParam param,vec4 texcoord)
{
	const vec2 invAtan=vec2(0.1591,0.3183);
	vec2 uv=vec2(atan(texcoord.z,texcoord.x),asin(texcoord.y));
	uv*=invAtan;
	uv+=0.5;
	
    uv=uvRotate(uv*param.tiling+param.offset,param.radian);
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(uv,param.tilingMode);

	return uv;
}

vec2 GetUV_Plane(uvParam param,vec4 texcoord,vec3 posWS)
{
    //将当前片元与物体中中心坐标结合起来，投影到相机的前向向量中，获取当前片元在相机前向向量上的投影标量，并与之对比depth值
	float cos_theta=dot(normalize(param.planeMappingCameraFront),normalize(posWS-param.gizmoPlanePosWS));
	float width=abs(length(posWS-param.gizmoPlanePosWS)*cos_theta);

	if(width>param.planeMappingDepth&&param.planeMappingDepth>0.0)
		isClampToBorder=true;

	vec2 uv=uvRotate(texcoord.xy+param.offset,param.radian);
	uv=uv*0.5+0.5;//先旋转再平移
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(uv,param.tilingMode);
    return uv;
}


vec2 GetUV_Cylinder(uvParam param,vec4 texcoord)
{
    const float PI=3.1415926535;
	vec2 uv=vec2(atan(texcoord.z,texcoord.x)+PI,texcoord.y);

    uv=uvRotate(uv*param.tiling+param.offset,param.radian);
    uv=uv*0.5+0.5;//先旋转再平移
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(uv,param.tilingMode);

	return uv;
}

vec2 GetUV_Cube(uvParam param,vec4 texcoord)
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
	uv=uvTiling(uv,param.tilingMode);
	return uv;
}

vec2 GetUV_Camera(uvParam param,vec4 texcoord)
{
	vec4 tmpTexcoord=texcoord;
	tmpTexcoord.xyz/=tmpTexcoord.w;
	tmpTexcoord.xyz=tmpTexcoord.xyz*0.5+0.5;
    vec2 tmpUV=(tmpTexcoord.xy-0.5)*param.tiling;
	vec2 uv=uvRotate(tmpUV-param.offset,param.radian);
	uv+=0.5;
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(uv,param.tilingMode);
    return uv;
}

vec2 GetUV_CameraAdaptive(uvParam param,vec4 texcoord)
{
    vec2 uv=uvRotate(texcoord.xy+param.offset,param.radian);
	uv=uv*0.5+0.5;//先旋转再平移
	uv=uvReverse(uv,param.reverseMode);
	uv=uvTiling(uv,param.tilingMode);
    return uv;
}

vec2 GetUV(uvParam param,vec4 texcoord,vec3 posWS)
{
	if(param.mappingType==UV_MAPPING_TYPE_CUBE)
		return GetUV_Cube(param,texcoord);
	else if(param.mappingType==UV_MAPPING_TYPE_SPHERE)
		return GetUV_Sphere(param,texcoord);
	else if(param.mappingType==UV_MAPPING_TYPE_CYLINDER)
		return GetUV_Cylinder(param,texcoord);
	else if(param.mappingType==UV_MAPPING_TYPE_PLANE)
		return GetUV_Plane(param,texcoord,posWS);
	else if(param.mappingType==UV_MAPPING_TYPE_CAMERA)
		return GetUV_Camera(param,texcoord);
	else if(param.mappingType==UV_MAPPING_TYPE_CAMERA_ADAPTIVE)
		return GetUV_CameraAdaptive(param,texcoord);
	else return GetUV_UV(param,texcoord);
}

void main()
{
	vec2 texcoord=GetUV(textureMappingParam,uv,posWS);
    fragColor= texture(mappingTexture,texcoord);

	if(isClampToBorder)
		fragColor=vec4(0.0,0.0,0.0,0.0);
}