#version 460 core
#extension GL_ARB_separate_shader_objects:enable

out gl_PerVertex 
{
    vec4 gl_Position;
};

const vec2 positions[6]=vec2[](  vec2(-1.0f,  1.0f),   // Lower left
								 vec2(-1.0f, -1.0f),   // Upper left
								 vec2( 1.0f, -1.0f),   // Upper right

								 vec2(-1.0f,  1.0f),   // Lower left
								 vec2( 1.0f, -1.0f),   // Upper right
								 vec2( 1.0f,  1.0f)   // Lower right
                                ); 
void main()
{
    gl_Position=vec4(positions[gl_VertexIndex],0.0,1.0);
}