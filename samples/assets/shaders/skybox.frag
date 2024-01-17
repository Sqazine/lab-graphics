#version 450

layout(set = 1, binding = 0) uniform samplerCube envTexture;

layout(location = 0) in vec3 localPosition;
layout(location = 0) out vec4 color;

void main()
{
    vec3 envVector = normalize(localPosition);
    color = textureLod(envTexture, envVector, 0.0);
}

