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

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexcoord;
layout(location=3) in vec3 inTangent;
layout(location=4) in vec3 inBinormal;

layout(location=0) out vec3 posWS;
layout(location=1) out vec4 uv;
layout(location=2) out vec3 vPos;

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

uniform uvParam textureMappingParam;

vec4 GetUV_UV()
{
    return vec4(inTexcoord,0.0,0.0);
}

vec4 GetUV_Sphere_Cylinder_Cube(mat4 mappingMatrix,vec3 vPos,vec3 vNormal)
{
    if(textureMappingParam.mappingType==UV_MAPPING_TYPE_CUBE)
            return vec4(vNormal,1.0);
     return mappingMatrix*vec4(vPos,1.0);
}
vec4 GetUV_Plane_Camera_AdaptiveCamera(mat4 projViewMatrix,mat4 modelMat,vec3 vPos)
{
    return projViewMatrix*modelMat*vec4(vPos,1.0);
}
vec4 GetUV(uvParam param,mat4 modelMat,vec3 vPos,vec3 vNormal)
{
	if(param.mappingType==UV_MAPPING_TYPE_CUBE||param.mappingType==UV_MAPPING_TYPE_SPHERE||param.mappingType==UV_MAPPING_TYPE_CYLINDER)
		return GetUV_Sphere_Cylinder_Cube(param.mappingMatrix,vPos,vNormal);
	else if(param.mappingType==UV_MAPPING_TYPE_PLANE||param.mappingType==UV_MAPPING_TYPE_CAMERA||param.mappingType==UV_MAPPING_TYPE_CAMERA_ADAPTIVE)
		return GetUV_Plane_Camera_AdaptiveCamera(param.mappingMatrix,modelMat,vPos);
	else return GetUV_UV();
}

void main()
{
    vPos=inPosition;
    posWS=vec3(modelMatrix*vec4(inPosition,1.0));
    vec3 normalWS=normalize(mat3(transpose(inverse(modelMatrix)))*inNormal);
    gl_Position=projectionMatrix*viewMatrix*vec4(posWS,1.0);
    uv=GetUV(textureMappingParam,modelMatrix,inPosition,inNormal);
}
