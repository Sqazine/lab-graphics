#version 450 core

layout(location=0) out vec4 fragColor;

layout(binding=0) uniform sampler2D text;

layout(location=0) in vec2 texcoord;

void main()
{
    fragColor=texture(text,clamp(texcoord/0.5,0.0,1.0));
    //fragColor=vec4(texcoord*0.5,0.0,1.0);
}