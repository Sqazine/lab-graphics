#version 450 core

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexcoord;
layout(location=3) in vec3 inTangent;
layout(location=4) in vec3 inBinormal;

layout(location=0) out vec3 vPosWS;
layout(location=1) out vec3 vNormalWS;
layout(location=2) out vec3 vTangentWS;
layout(location=3) out vec3 vBinormalWS;
layout(location=4) out vec2 vUv;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
     vUv=inTexcoord;
    vPosWS=vec3(modelMatrix*vec4(inPosition,1.0));
    vNormalWS=mat3(transpose(inverse(modelMatrix)))*inNormal;
    vTangentWS=mat3(transpose(inverse(modelMatrix)))*inTangent;
    vBinormalWS=mat3(transpose(inverse(modelMatrix)))*inBinormal;
    
    gl_Position=projectionMatrix*viewMatrix*vec4(vPosWS,1.0);
}
