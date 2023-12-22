 #version 450 core

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexcoord;
layout(location=3) in vec3 inTangent;
layout(location=4) in vec3 inBinormal;

layout(location=0) out vec3 positionWS;
layout(location=1) out vec2 texcoord;
layout(location=2) out vec3 normalWS;
layout(location=3) out mat3 TBN;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
   	positionWS=vec3(modelMatrix*vec4(inPosition,1.0));
	normalWS=normalize(transpose(inverse(mat3(modelMatrix)))*inNormal);
	texcoord=inTexcoord;

	vec3 T=normalize(transpose(inverse(mat3(modelMatrix)))*inTangent);
	vec3 B=normalize(transpose(inverse(mat3(modelMatrix)))*inBinormal);
	vec3 N=normalWS;

	TBN=mat3(T,B,N);

	gl_Position=projectionMatrix*viewMatrix*vec4(positionWS,1.0);
}