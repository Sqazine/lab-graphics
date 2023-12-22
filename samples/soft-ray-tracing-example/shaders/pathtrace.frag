#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable 
#extension GL_GOOGLE_include_directive : require

#include "utils.glsl"
#include "primitives.glsl"
#include "intersections.glsl"
#include "materials.glsl"

layout(set=0,binding=0) uniform sampler2D uImage;

layout(push_constant) uniform PushConstants
{
    float time;
    float frameCounter;
    vec2 resolution;
    vec4 cursorPosition;
    vec2 mouseDown;
}pushConstants;

layout(location=0) out vec4 oColor;

// Define the scene.
Material materials[] = 
{
	{ { 0.9f, 0.9f, 0.9f }, MAT_TYPE_METALLIC, 0.50f, false }, 
	{ { 0.90f, 0.10f, 0.20f }, MAT_TYPE_DIELECTRIC, 1.0f / 2.31f, false },
	{ { 0.97f, 1.00f, 0.97f }, MAT_TYPE_DIFFUSE, IGNORED, false }, 		// Off-white

	{ { 1.00f, 0.00f, 0.00f }, MAT_TYPE_DIFFUSE, IGNORED, false }, 		// Pink
    { { 0.54f, 0.79f, 0.15f }, MAT_TYPE_DIFFUSE, IGNORED, false }, 		// Mint
    { { 0.10f, 0.51f, 0.77f }, MAT_TYPE_DIFFUSE, IGNORED, false }, 		// Dark mint
	{ { 1.00f, 0.79f, 0.23f }, MAT_TYPE_DIFFUSE, IGNORED, false },		// Yellow
	{ { 0.42f, 0.30f, 0.58f }, MAT_TYPE_DIFFUSE, IGNORED, false }, 		// Purple

	{ { 5.00f, 4.80f, 4.80f }, MAT_TYPE_DIFFUSE, IGNORED, true },  		// *Light

	{ { 1.00f, 0.98f, 0.98f }, MAT_TYPE_METALLIC, 0.25f, false }, 
	{ { 1.00f, 0.98f, 0.98f }, MAT_TYPE_METALLIC, 0.50f, false }, 
	{ { 1.00f, 0.98f, 0.98f }, MAT_TYPE_METALLIC, 0.75f, false }, 
	{ { 1.00f, 0.98f, 0.98f }, MAT_TYPE_METALLIC, 1.00f, false }, 
	{ { 1.00f, 0.98f, 0.98f }, MAT_TYPE_METALLIC, 1.25f, false }, 
	{ { 1.00f, 0.98f, 0.98f }, MAT_TYPE_METALLIC, 1.50f, false }, 
	{ { 1.00f, 0.98f, 0.98f }, MAT_TYPE_METALLIC, 1.75f, false }, 

	{ { 0.90f, 0.10f, 0.20f }, MAT_TYPE_DIELECTRIC, 1.0f / 1.31f, false },

	{ { 0.3f, 0.3f, 0.3f }, MAT_TYPE_METALLIC, 0.50f, false },
};


Sphere spheres[] = 
{
	// Light source 
	{ 0.75f, vec3( -2.00f, -4.00f,  0.00f), 8 },

	// Spheres in the middle
	{ 1.500f, vec3( 0.00f, -1.500f,  0.000f), 1 },
	{ 0.750f, vec3( 0.00f, -0.750f, -2.250f), 4 },
    { 0.375f, vec3( 0.00f, -0.375f, -3.375f), 5 },
	{ 0.100f, vec3( 0.00f, -0.100f, -3.850f), 6 },

	// // Line of spheres on the left
	{ 0.30f, vec3(-3.00f, -0.30f, -2.00f),  9 },
	{ 0.30f, vec3(-3.00f, -0.30f, -1.00f), 10 },
	{ 0.30f, vec3(-3.00f, -0.30f,  0.00f), 11 },
	{ 0.30f, vec3(-3.00f, -0.30f,  1.00f), 12 },
	{ 0.30f, vec3(-3.00f, -0.30f,  2.00f), 13 },
	{ 0.30f, vec3(-3.00f, -0.30f,  3.00f), 14 },
	{ 0.30f, vec3(-3.00f, -0.30f,  4.00f), 15 }
};

Plane planes[] = 
{
	// Colored
	{ -Z_AXIS,  Z_AXIS * 5.50f, 5 }, // Back
	{  Z_AXIS, -Z_AXIS * 8.50f, 2 }, // Front
	{ -X_AXIS,  X_AXIS * 4.50f, 7 }, // Right
	{  X_AXIS, -X_AXIS * 4.50f, 4 }, // Left
	{  Y_AXIS, -Y_AXIS * 7.00f, 2 }, // Top
	{ -Y_AXIS,  Y_AXIS * 0.00f, 2 }  // Bottom
};

RayPayload IntersectScene(in Ray r)
{
    // This function simply iterates through all of the objects in our scene, 
	// keeping track of the closest intersection (if any). For shading and
	// subsequent ray bounces, we need to keep track of a bunch of information
	// about the object that was hit, like it's normal vector at the point
	// of intersection.
	//
	// Initially, we assume that we won't hit anything by setting the fields
	// of our `intersection` struct as follows:
    
	RayPayload intersection={
		-1,
		OBJECT_TYPE_MISS,
		-1,
		r.direction,
		r.origin,
		{-1.0,-1.0,-1.0},
		MAX_DISTANCE
	};

	// Now, let's iterate through the objects in our scene, starting with 
	// the spheres:
	for(uint i=0u;i<spheres.leneght();++i)
	{
		
	}
}

vec3 Trace()
{
	vec2 uv=gl_FragCoord.xy/pushConstants.resolution;

	float t=pushConstants.time;

	vec4 seed = {uv.x + t,uv.y + t,uv.x - t,uv.y - t};


}

void main()
{
    vec3 traceColor=Trace();

    // Perform gamma correction.
    traceColor=pow(traceColor,vec3(GAMMA));

    vec2 uv=gl_FragCoord.xy/pushConstants.resolution;
    vec3 prevFrame=texture(uImage,uv).rgb;
    vec3 currFrame=traceColor;

    if(any(bvec2(pushConstants.mouseDown)))
        oColor=vec4(currFrame,1.0);
    else
        oColor=vec4(prevFrame+currFrame,1.0);
}