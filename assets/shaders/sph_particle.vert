#version 450 core

#define PARTICLE_NUM 20000

layout(location=0) in vec2 position;

layout(location=0) out vec4 color;

void main()
{
    gl_Position=vec4(position.x,position.y,0,1);
    gl_PointSize=1;

    if(gl_VertexIndex<PARTICLE_NUM/4)
        color=vec4(1.0,0.0,0.0,1.0);
    else if(gl_VertexIndex<PARTICLE_NUM/2)
        color=vec4(0.0,1.0,0.0,1.0);
    else if(gl_VertexIndex<PARTICLE_NUM/1.5)
        color=vec4(0.0,0.0,1.0,1.0);
    else
        color=vec4(1.0,1.0,1.0,1.0);
}