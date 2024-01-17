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

uniform mat4 pose[MAX_BONES];
uniform mat4 invBindPose[MAX_BONES];

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
    mat4 skin=(pose[boneIndex.x]*invBindPose[boneIndex.x])*weights.x+
                (pose[boneIndex.y]*invBindPose[boneIndex.y])*weights.y+
                (pose[boneIndex.z]*invBindPose[boneIndex.z])*weights.z+
                (pose[boneIndex.w]*invBindPose[boneIndex.w])*weights.w;

    gl_Position=projection*view*model*skin*vec4(positionOS,1.0);

    fragPosWS=vec3(model*skin*vec4(positionOS,1.0));

    normalWS=vec3(model*skin*vec4(normalOS,0.0f));

    uv=texcoord;
}