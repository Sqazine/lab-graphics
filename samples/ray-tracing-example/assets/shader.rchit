#version 460

#extension GL_EXT_ray_tracing:enable
#extension GL_GOOGLE_include_directive:require
#extension GL_EXT_nonuniform_qualifier : require

#include "shaderDatas.glsl"
#include "random.glsl"

layout(set=1,binding=0,std430) readonly buffer MaterialIDBuffer
{
    uint MaterialIDs[];
} MaterialIDArray[];

layout(set=1,binding=1,std430) readonly buffer AttribBuffer
{
    VertexAttribute VertexAttributes[];
}AttribArray[];

layout(set=1,binding=2,std430) readonly buffer FaceBuffer
{
    uvec4 Faces[];
}FaceArray[];

layout(set=1,binding=3) uniform sampler2D TexturesArray[];

layout(location=0) rayPayloadInEXT RayPayload PrimaryRay;
hitAttributeEXT vec2 HitAttribs;

void main()
{
    const vec3 barycentrics=vec3(1.0f-HitAttribs.x-HitAttribs.y,HitAttribs.x,HitAttribs.y);
    const uint matID=MaterialIDArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].MaterialIDs[gl_PrimitiveID];
    const uvec4 face=FaceArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].Faces[gl_PrimitiveID];

    VertexAttribute v0=AttribArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].VertexAttributes[int(face.x)];
    VertexAttribute v1=AttribArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].VertexAttributes[int(face.y)];
    VertexAttribute v2=AttribArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].VertexAttributes[int(face.z)];

    const vec3 normal=normalize(BaryLerp(v0.normal.xyz,v1.normal.xyz,v2.normal.xyz,barycentrics));
    const vec2 uv=BaryLerp(v0.uv.xy,v1.uv.xy,v2.uv.xy,barycentrics);

    const vec3 texel=textureLod(TexturesArray[nonuniformEXT(matID)],uv,0.0f).rgb;

    PrimaryRay.color=texel;
    PrimaryRay.distance=gl_HitTEXT;
    PrimaryRay.normal=normal;
    PrimaryRay.scatterDir=normal+RandomInUnitSphere(PrimaryRay.randomSeed);

}