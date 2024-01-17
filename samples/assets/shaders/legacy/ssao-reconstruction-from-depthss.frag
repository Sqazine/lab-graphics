#version 450 core

layout(location=0) out float ao;

layout(location=0) in vec2 texcoord;

layout(binding=0) uniform sampler2D gDepthSS;
layout(binding=1) uniform sampler2D noiseTexture;

const int kernelSize=64;

uniform mat4 projectionMatrix;
uniform vec3 samples[kernelSize];
uniform vec2 noiseScale;

uniform float kernelRadius;
uniform float kernelBias;

float ReconstructDepthVSFromDepthSS(vec2 uv)
{
    return -projectionMatrix[3][2]/(2.0*texture(gDepthSS,uv).w-1.0+projectionMatrix[2][2]);
}

vec3 ReconstructPositionVSFromDepthVS(vec2 uv)
{
    float tangentHalfFOV=1.0/(projectionMatrix[1][1]);
    float aspect=1.0/(tangentHalfFOV*projectionMatrix[0][0]);
    //1.0-uv还是不太懂
    return vec3(((1.0-uv.x)*2.0-1.0)*tangentHalfFOV*aspect,((1.0-uv.y)*2.0-1.0)*tangentHalfFOV,1.0)*ReconstructDepthVSFromDepthSS(uv);
}

vec3 ReconstructNormalVSFromPositionVS(vec3 posVS)
{
    return normalize(cross(dFdx(posVS),dFdy(posVS)));
}

void main()
{
    vec3 fragPosVS=ReconstructPositionVSFromDepthVS(texcoord);
    vec3 normalVS=ReconstructNormalVSFromPositionVS(fragPosVS);
    vec3 randomVec=normalize(texture(noiseTexture,texcoord*noiseScale).xyz);

    vec3 tangent=normalize(randomVec-normalVS*dot(randomVec,normalVS));
    vec3 bitangent=cross(normalVS,tangent);
    mat3 TBN=mat3(tangent,bitangent,normalVS);

    float occlusion=0.0;
    for(int i=0;i<kernelSize;++i)
    {
        vec3 s=TBN*samples[i];
        s=fragPosVS+s*kernelRadius;

        vec4 offset=vec4(s,1.0);
        offset=projectionMatrix*offset;//view->projection
        offset.xyz/=offset.w;//projection->NDC
        offset.xyz=offset.xyz*0.5+0.5;//NDC->screen space

        float sampleDepth=ReconstructDepthVSFromDepthSS(offset.xy);
        float rangeCheck=smoothstep(0.0,1.0,kernelRadius/abs(fragPosVS.z-sampleDepth));
        occlusion+=(sampleDepth>=s.z+kernelBias?1.0:0.0)*rangeCheck;
    }
    occlusion=1.0-(occlusion/float(kernelSize));
    ao=occlusion;
}