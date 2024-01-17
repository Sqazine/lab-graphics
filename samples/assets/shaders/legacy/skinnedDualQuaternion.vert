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

uniform mat2x4 pose[MAX_BONES];
uniform mat2x4 invBindPose[MAX_BONES];

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


vec4 QuaternionMultiply(vec4 left,vec4 right)
{
   return vec4(right.x * left.w + right.y * left.z - right.z * left.y + right.w * left.x,
		-right.x * left.z + right.y * left.w + right.z * left.x + right.w * left.y,
		right.x * left.y - right.y * left.x + right.z * left.w + right.w * left.z,
		-right.x * left.x - right.y * left.y - right.z * left.z + right.w * left.w);
}

mat2x4 DualQuaternionNormalize(mat2x4 dq)
{
    float invMag=1.0f/length(dq[0]);
    dq[0]*=invMag;
    dq[1]*=invMag;
    return dq;
}

mat2x4 DualQuaternionCombine(mat2x4 left,mat2x4 right)
{
    left=DualQuaternionNormalize(left);
    right=DualQuaternionNormalize(right);

    vec4 real=QuaternionMultiply(left[0],right[0]);
    vec4 dual=QuaternionMultiply(left[0],right[1])+QuaternionMultiply(left[1],right[0]);
    return mat2x4(real,dual);
}

vec4 TransformVector(mat2x4 dq,vec3 v)
{
    vec4 real=dq[0];
    vec3 r_vector=real.xyz;
    float r_scalar=real.w;

    vec3 rotated=r_vector*2.0f*dot(r_vector,v)+v*(r_scalar*r_scalar-dot(r_vector,r_vector))+cross(r_vector,v)*2.0f*r_scalar;

    return vec4(rotated,0);
}

vec4 TransformPoint(mat2x4 dq,vec3 v)
{
    vec4 real=dq[0];
    vec4 dual=dq[1];

    vec3 rotated=TransformVector(dq,v).xyz;
    vec4 conjugate=vec4(-real.xyz,real.w);
    vec3 t=QuaternionMultiply(conjugate,dual*2.0).xyz;
    return vec4(rotated+t,1);    
}

void main()
{
    vec4 w=weights;

    if(dot(pose[boneIndex.x][0],pose[boneIndex.y][0])<0.0)
        w.y*=-1.0;
      if(dot(pose[boneIndex.x][0],pose[boneIndex.z][0])<0.0)
        w.z*=-1.0;
      if(dot(pose[boneIndex.x][0],pose[boneIndex.w][0])<0.0)
        w.w*=-1.0;

    mat2x4 dq0=DualQuaternionCombine(invBindPose[boneIndex.x],pose[boneIndex.x]);
    mat2x4 dq1=DualQuaternionCombine(invBindPose[boneIndex.y],pose[boneIndex.y]);
    mat2x4 dq2=DualQuaternionCombine(invBindPose[boneIndex.z],pose[boneIndex.z]);
    mat2x4 dq3=DualQuaternionCombine(invBindPose[boneIndex.w],pose[boneIndex.w]);

    mat2x4 skinDq=w.x*dq0+w.y*dq1+w.z*dq2+w.w*dq3;

    skinDq=DualQuaternionNormalize(skinDq);

    vec4 v=TransformPoint(skinDq,positionOS);

    gl_Position=projection*view*model*v;

    fragPosWS=vec3(model*v);

    normalWS=vec3(model*TransformVector(skinDq,normalOS));

    uv=texcoord;
}