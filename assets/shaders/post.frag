#version 450

layout(std140,set=0, binding=0) buffer buf
{
    vec4 imageData[];
};

layout(std140,set=0, binding=1) uniform Uniform
{
    uint width;
    uint height;
};

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
    ivec2 intUV=ivec2(inUV.x*width,inUV.y*height);
    outFragColor = imageData[width*intUV.y+intUV.x];
}