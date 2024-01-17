#version 450 core

layout(set=0,binding=0) uniform TransformUniforms
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    mat4 skyboxRotationMatrix;
};

layout(location=0) in vec3 position;
layout(location=0) out vec3 localPosition;

void main()
{
    localPosition=position.xyz;
   
    gl_Position = projectionMatrix*mat4(mat3(viewMatrix)) *skyboxRotationMatrix* vec4(position, 1.0);
    gl_Position=gl_Position.xyww;
}