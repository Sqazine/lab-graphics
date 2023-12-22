#version 450 core

layout(location=0) out float blurAo; 

layout(binding=0) uniform sampler2D ssaoTexture; 

layout(location=0) in vec2 texcoord;

uniform float blurRadius;

void main()
{
    vec2 texelSize=1.0/vec2(textureSize(ssaoTexture,0));
    float result=0.0;
    for(float x=-blurRadius;x<blurRadius;++x)
    {
        for(float y=-blurRadius;y<blurRadius;++y)
        {
            vec2 offset=vec2(x,y)*texelSize;
            result+=texture(ssaoTexture,texcoord+offset).r; 
        }
    }
    blurAo=result/(blurRadius*blurRadius*4);
}


