#version 460

#extension GL_EXT_ray_tracing:enable
#extension GL_GOOGLE_include_directive:require
#extension GL_EXT_nonuniform_qualifier : require

#include "ShaderDatas.glsl"

layout(set=2,binding=0) uniform sampler2D EnvTexture;

layout(location=0) rayPayloadInEXT RayPayload PrimaryRay;

vec2 DirToLatLong(vec3 dir)
{
    float phi=atan(dir.x,dir.z);
    float theta=acos(dir.y);

    return vec2((PI+phi)*(0.5/PI),theta*INV_PI);
}

void main()
{
   vec2 uv = DirToLatLong(gl_WorldRayDirectionEXT);
    vec3 envColor = textureLod(EnvTexture, uv, 0.0).rgb;
    PrimaryRay.color =envColor;
    PrimaryRay.distance=-1.0;
}