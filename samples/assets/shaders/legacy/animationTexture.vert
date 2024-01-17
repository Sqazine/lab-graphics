#version 450 core

#define MAX_BONES 60

layout(location=0) in vec3 positionOS;
layout(location=1) in vec3 normalOS;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec3 tangentOS;
layout(location=4) in vec3 binormalOS;
layout(location=5) in vec4 color;
layout(location=6) in vec4 weights;
layout(location=7) in ivec4 boneIndex;

layout(location=0) out vec3 normalWS;
layout(location=1) out vec3 fragPosWS;
layout(location=2) out vec2 uv;

uniform sampler2D animationTexture;
uniform mat4 invBindPose[MAX_BONES];

uniform ivec2 frame;
uniform float time;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

vec3 QMulV(vec4 q,vec3 v)
{
    return q.xyz*2.0*dot(q.xyz,v)+v*(q.w*q.w-dot(q.xyz,q.xyz))+cross(q.xyz,v)*2.0*q.w;
}

mat4 GetPose(int bone)
{
    int x_now=frame.x;
    int x_next=frame.y;
    int y_pos=bone*3;

    vec4 pos0=texelFetch(animationTexture,ivec2(x_now,y_pos+0),0);
    vec4 rot0=texelFetch(animationTexture,ivec2(x_now,y_pos+1),0);
    vec4 scl0=texelFetch(animationTexture,ivec2(x_now,y_pos+2),0);

    vec4 pos1=texelFetch(animationTexture,ivec2(x_next,y_pos+0),0);
    vec4 rot1=texelFetch(animationTexture,ivec2(x_next,y_pos+1),0);
    vec4 scl1=texelFetch(animationTexture,ivec2(x_next,y_pos+2),0);

    if(dot(rot0,rot1)<0.0)
        rot1*=vec4(-1.0);
    vec4 position=mix(pos0,pos1,time);
    vec4 rotation=normalize(mix(rot0,rot1,time));
    vec4 scale=mix(scl0,scl1,time);

    vec3 xBasis=QMulV(rotation,vec3(scale.x,0.0,0.0));
    vec3 yBasis=QMulV(rotation,vec3(0.0,scale.y,0.0));
    vec3 zBasis=QMulV(rotation,vec3(0.0,0.0,scale.z));

    return mat4(
        xBasis.x,xBasis.y,xBasis.z,0.0,
        yBasis.x,yBasis.y,yBasis.z,0.0,
        zBasis.x,zBasis.y,zBasis.z,0.0,
        position.x,position.y,position.z,1.0
    );
}

void main()
{
    mat4 pose0=GetPose(boneIndex.x);
    mat4 pose1=GetPose(boneIndex.y);
    mat4 pose2=GetPose(boneIndex.z);
    mat4 pose3=GetPose(boneIndex.w);

    mat4 skin=(pose0*invBindPose[boneIndex.x])*weights.x;
    skin+=(pose1*invBindPose[boneIndex.y])*weights.y;
    skin+=(pose2*invBindPose[boneIndex.z])*weights.z;
    skin+=(pose3*invBindPose[boneIndex.w])*weights.w;

    gl_Position=projection*view*model*skin*vec4(positionOS,1.0);

    fragPosWS=vec3(model*skin*vec4(positionOS,1.0));

    normalWS=vec3(model*skin*vec4(normalOS,0.0));

    uv=texcoord;
}