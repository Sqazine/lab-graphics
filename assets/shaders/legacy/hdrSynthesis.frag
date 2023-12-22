#version 450 

layout(location=0) out vec4 fragColor;

layout(location=0) in vec2 texcoord;

layout(binding=0) uniform sampler2D hdrTexture;
layout(binding=1) uniform sampler2D additiveTexture;

uniform vec2 additiveTexturePos;
uniform vec2 additiveTextureSize;

void main()
{
    vec4 addColor=texture(additiveTexture,texcoord*additiveTextureSize+additiveTexturePos);
    fragColor=addColor;
}