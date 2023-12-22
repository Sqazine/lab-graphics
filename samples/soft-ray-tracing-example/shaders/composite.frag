#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(input_attachment_index = 0, set = 0, binding = 1) uniform subpassInput accumulateImage;


layout(push_constant) uniform PushConstants
{
	float time;
	float frameCount;
	vec2 resolution;
	vec2 cursorPos;
	float mouseDown;
}constants;

layout(location=0) out vec4 outColor;

void main()
{
	vec2 uv=gl_FragCoord.xy/constants.resolution;

	vec3 accumlated=subpassLoad(accumulateImage).rgb;

	if(constants.mouseDown != 1.0f)
		accumlated /= constants.frameCount+1.0f;

	outColor=vec4(accumlated,1.0f);
}
