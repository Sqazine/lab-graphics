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


layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexcoord;
layout(location=3) in vec3 inTangent;
layout(location=4) in vec3 inBinormal;

layout(location=0) out vec3 vPosWS;
layout(location=1) out vec3 vNormalWS;
layout(location=2) out vec3 vTangentWS;
layout(location=3) out vec3 vBinormalWS;
layout(location=4) out vec3 vPos;
layout(location=5) out MaterialsUV materialsUV; 

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

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

uniform int materialCount;
uniform uvParam albedoMapParam[MAX_MATERIAL_COUNT];
uniform uvParam normalMapParam[MAX_MATERIAL_COUNT];
uniform uvParam metalMapParam[MAX_MATERIAL_COUNT];
uniform uvParam roughnessMapParam[MAX_MATERIAL_COUNT];
uniform uvParam aoMapParam[MAX_MATERIAL_COUNT];

vec4 GetUV_UV()
{
    return vec4(inTexcoord,0.0,0.0);
}

vec4 GetUV_Sphere_Cylinder_Cube(uvParam param,vec3 vPos,vec3 vNormal)
{
    if(param.mappingType==UV_MAPPING_TYPE_CUBE)
            return vec4(vNormal,1.0);
     return param.mappingMatrix*vec4(vPos,1.0);
}
vec4 GetUV_Plane_Camera_AdaptiveCamera(uvParam param,mat4 modelMat,vec3 vPos)
{
    return param.mappingMatrix*modelMat*vec4(vPos,1.0);
}
vec4 GetUV(uvParam param,mat4 modelMat,vec3 vPos,vec3 vNormal)
{
	if(param.mappingType==UV_MAPPING_TYPE_CUBE||param.mappingType==UV_MAPPING_TYPE_SPHERE||param.mappingType==UV_MAPPING_TYPE_CYLINDER)
		return GetUV_Sphere_Cylinder_Cube(param,vPos,vNormal);
	else if(param.mappingType==UV_MAPPING_TYPE_PLANE||param.mappingType==UV_MAPPING_TYPE_CAMERA||param.mappingType==UV_MAPPING_TYPE_CAMERA_ADAPTIVE)
		return GetUV_Plane_Camera_AdaptiveCamera(param,modelMat,vPos);
	else return GetUV_UV();
}

void main()
{
     vPos=inPosition;
    vPosWS=vec3(modelMatrix*vec4(inPosition,1.0));
    vNormalWS=mat3(transpose(inverse(modelMatrix)))*inNormal;
    vTangentWS=mat3(transpose(inverse(modelMatrix)))*inTangent;
    vBinormalWS=mat3(transpose(inverse(modelMatrix)))*inBinormal;
    
    gl_Position=projectionMatrix*viewMatrix*vec4(vPosWS,1.0);

    for(int i=0;i<materialCount;++i)
    {
        materialsUV.albedoMapUv[i]=GetUV(albedoMapParam[i],modelMatrix,inPosition,normalize(inNormal));
        materialsUV.normalMapUv[i]=GetUV(normalMapParam[i],modelMatrix,inPosition,normalize(inNormal));
        materialsUV.metalMapUv[i]=GetUV(metalMapParam[i],modelMatrix,inPosition,normalize(inNormal));
        materialsUV.roughnessMapUv[i]=GetUV(roughnessMapParam[i],modelMatrix,inPosition,normalize(inNormal));
        materialsUV.aoMapUv[i]=GetUV(aoMapParam[i],modelMatrix,inPosition,normalize(inNormal));
    }
}
