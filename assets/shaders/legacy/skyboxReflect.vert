#version 450

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexcoord;
layout(location=3) in vec3 inTangent;
layout(location=4) in vec3 inBinormal;

layout(location=0) out vec2 texcoord;
layout(location=1) out vec3 normalWS;
layout(location=2) out vec3 posWS;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
    gl_Position=projectionMatrix*viewMatrix*modelMatrix*vec4(inPosition,1.0);
    texcoord=inTexcoord;
    posWS=(modelMatrix*vec4(inPosition,1.0)).xyz;
    normalWS=transpose(inverse(mat3(modelMatrix)))*inNormal;
}