#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable 
#extension GL_GOOGLE_include_directive : require

layout(location=0) out vec4 outColor;


#include "utils.glsl"
#include "primitives.glsl"

layout(set=0,binding=0) uniform sampler2D uImage;

layout(push_constant) uniform PushConstants
{
    float time;
    float frameCounter;
    vec2 resolution;
    vec4 cursorPosition;
    vec2 mouseDown;
}pushConstants;

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

// These intersection routines are quite slow, so let's disable them for now:
//
// #define QUADS
// #define BOXES
//
// Quad quads[] =
// {
// 	BuildQuad(7.00f, 1.00f, vec3(3.00f, -0.60f, 0.00f), y_axis, to_radians(90.0f),  9),
// 	BuildQuad(7.00f, 1.00f, vec3(3.00f, -1.70f, 0.00f), y_axis, to_radians(90.0f), 12),
// 	BuildQuad(7.00f, 1.00f, vec3(3.00f, -2.80f, 0.00f), y_axis, to_radians(90.0f), 15),
// };

// box boxes[] =
// {
// 	BuildBox(vec3(2.0f, 4.0f, 2.0f), -y_axis * 2.0f, 3)
// };


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
    
	RayPayload rayPayload={
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
	for(uint i=0u;i<spheres.length();++i)
	{
		float tmpT;
		if(IntersectSphere(spheres[i],r,tmpT))
		{
			// Was the intersection closer than any previous one?
			if(tmpT < rayPayload.t)
			{
				rayPayload.materialIdx = spheres[i].materialIdx;
				rayPayload.objectType = OBJECT_TYPE_SPHERE;
				rayPayload.objectIdx=int(i);
				rayPayload.pos=r.origin + r.direction * tmpT;
				rayPayload.normal = normalize(rayPayload.pos - spheres[i].center);
				rayPayload.t = tmpT;
			}
		}
	}

	//then planes
	for(uint i=0u;i < planes.length(); ++i)
	{
		float tmpT;
		if(IntersectPlane(planes[i],r,tmpT))
		{
			// Was the intersection closer than any previous one?
			if(tmpT < rayPayload.t)
			{
				rayPayload.materialIdx = planes[i].materialIdx;
				rayPayload.objectType = OBJECT_TYPE_PLANE;
				rayPayload.objectIdx=int(i);
				rayPayload.pos=r.origin + r.direction * tmpT;
				rayPayload.normal = normalize(planes[i].normal);
				rayPayload.t = tmpT;
			}
		}
	}

	// then quads:
#ifdef QUADS
	for(uint i = 0;i < quads.length(); ++i)
	{
		// Did the ray intersect this object?
		float tmpT;
		if(IntersectQuad(quads[i],r,tmpT))
		{
			// Was the intersection closer than any previous one?
			if(tmpT < rayPayload.t)
			{
				const vec3 edge0 = quads[i].ur - quads[i].ul;
				const vec3 edge1 = quads[i].ll - quads[i].ul;
				const vec3 normal = normalize(cross(edge0, edge1));

				rayPayload.materialIdx = quads[i].materialIdx;
				rayPayload.objectType = OBJECT_TYPE_QUAD;
				rayPayload.objectIdx=int(i);
				rayPayload.pos=r.origin + r.direction * tmpT;
				rayPayload.normal = normal;
				rayPayload.t = tmpT;
			}
		}
	}
#endif

	// finally, boxes:
#ifdef BOXES
	for(uint i = 0u; i < boxes.length(); ++i)
	{	
		// Did the ray intersect this object?
		float tmpT;
		if(IntersectBox(boxes[i],r,tmpT))
		{
			// Was the intersection closer than any previous one?
			if(tmpT < rayPayload.t)
			{
				rayPayload.materialIdx = quads[i].materialIdx;
				rayPayload.objectType = OBJECT_TYPE_QUAD;
				rayPayload.objectIdx=int(i);
				rayPayload.pos=r.origin + r.direction * tmpT;

				vec3 normal;
				{
					vec3 center = (boxes[i].bounds[0] + boxes[i].bounds[1]) * 0.5f;
					vec3 size = abs(boxes[i].bounds[1] + boxes[i].bounds[0]);
					vec3 localPoint=rayPayload.pos-center;
					
					float min=10000.0f;

					// Find which face is the closest to the point of 
					// intersection (x, y, or z).
					float dist=abs(size.x - abs(localPoint.x));
					if(dist < min)
					{
						min = dist;
						normal = X_AXIS;
						normal *= sign(localPoint.x);
					}

					dist=abs(size.y - abs(localPoint.y));
					if(dist < min)
					{
						min = dist;
						normal = Y_AXIS;
						normal *= sign(localPoint.y);
					}

					dist=abs(size.z - abs(localPoint.z));
					if(dist < min)
					{
						min = dist;
						normal = Z_AXIS;
						normal *= sign(localPoint.z);
					}
				}

				rayPayload.normal = normalize(normal);
				rayPayload.t = tmpT;
			}
		}
	}
#endif

	return rayPayload;
}

vec3 Scatter(in material mat, in RayPayload rayPayload)
{
	// TODO: return the new ray direction
	// ...

	return vec3(0.0f);
}

Ray GetRay(inout vec4 seed,in vec2 uv)
{
	// We want to make sure that we correct for the window's aspect ratio
	// so that our final render doesn't look skewed or stretched when the
	// resolution changes.

	const float aspectRatio = pushConstants.resolution.x / pushConstants.resolution.y;
	const vec3 offset = vec3(pushConstants.cursorPosition.xy * 2.0f-1.0f,0.0f)* 8.0f;
	const vec3 cameraPosition=vec3(0.0f,-4.0f,-10.5f) + offset;

	const float verticalFov=60.0f;
	const float aperture=0.5f;
	const float lensRadius=aperture * 0.5f;
	const float theta =	ToRadians(verticalFov);
	const float halfHeight=tan(theta * 0.5f);
	const float halfWeight=aspectRatio * halfHeight;

	const mat3 lookAt=LookAt(cameraPosition,origin + vec3(0.0f, -1.25f, 0.0f));
	const float distToFocus=Map(pushConstants.cursorPosition.w, 0.0f, 1.0f, 10.0f, 5.0f);
	const vec3 lowerLeftCorner=cameraPosition - lookAt * vec3(halfWeight,halfHeight,1.0f) * distToFocus;
	const vec3 horizontal = 2.0f *halfWeight * distToFocus * lookAt[0];
	const vec3 vertical = 2.0f *halfHeight * distToFocus * lookAt[1];
	
	// By jittering the uv-coordinates a tiny bit here, we get "free" anti-aliasing.
	vec2 jitter={ GpuRand(seed), GpuRand(seed) };
	jitter = jitter *2.0f -1.0f;
	uv += (jitter / pushConstants.resolution) * ANTI_ALIASING;

	// Depth-of-field calculation.
	vec3 rd = lensRadius * vec3(RandomOnDisk(seed), 0.0f);
	vec3 lensOffset=lookAt * vec3(rd.xy,0.0f);
	vec3 ro = cameraPosition + lensOffset;
	rd = lowerLeftCorner +uv.x * horizontal +uv.y * vertical - cameraPosition -lensOffset;
	rd=normalize(rd);

	return Ray(ro,rd);
}

#define RUSSIAN_ROULETTE

vec3 Trace()
{
	vec2 uv=gl_FragCoord.xy/pushConstants.resolution;

	float t=pushConstants.time;

	vec4 seed = {uv.x + t,uv.y + t,uv.x - t,uv.y - t};

	vec3 final = COLOR_BLACK;
	const vec3 sky = COLOR_BLACK;
	const bool debug = false;

	for(uint i=0; i < NUMBER_OF_ITERATIONS; ++i)
	{
		Ray r = GetRay(seed, uv);

		vec3 radiance = COLOR_BLACK;
		vec3 throughput = COLOR_WHITE;

		// For explicit light sampling (next event estimation), we need to keep track of the 
		// previous material that was hit, as explained below.
		int previousMaterialType = 0;

		// This is the main path tracing loop.
		for(uint j = 0; j < NUMBER_OF_BOUNCES; ++j)
		{
			RayPayload rayPayload = IntersectScene(r);

			// There were no intersections: simply accumulate the background color and break.
			if(rayPayload.objectType == OBJECT_TYPE_MISS)
				radiance += throughput;
			else
			{
				Material mat=materials[rayPayload.materialIdx];

				const vec3 hitLocation=r.origin + r.direction * rayPayload.t;

				// When using explicit light sampling, we have to account for a number of edge cases:
                //
                // 1. If this is the first bounce, and the object we hit is a light, we need to add its
                //    color (otherwise, lights would appear black in the final render).
                // 2. If the object we hit is a light, and the PREVIOUS object we hit was specular (a
                //    metal or dielectric), we need to add its color (otherwise, lights would appear
                //    black in mirror-like objects).
				if((j==0 || previousMaterialType == MAT_TYPE_DIELECTRIC || previousMaterialType == MAT_TYPE_METALLIC) && mat.isLight)
				{
					const vec3 lightColor = COLOR_WHITE;
					radiance += throughput * lightColor;
					break;
				}

				if(debug)
				{
					radiance += rayPayload.normal * 0.5 + 0.5;
					break;
				}

				// Set the new ray origin.
				r.origin = hitLocation +rayPayload.normal * EPSILON;

				// Choose a new ray direction based on the material type. 
				if(mat.type == MAT_TYPE_DIFFUSE)
				{
					// Here, we explicitly sample each of the light sources in our scene.
                    // Obviously, for scenes with many lights, this would be prohibitively 
                    // expensive. So, if we're using `n` lights, we randomly choose one
                    // light to sample and divide its contribution by `n`.
                    // ...

					
				}
			}
		}
	}
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
        outColor=vec4(currFrame,1.0);
    else
        outColor=vec4(prevFrame+currFrame,1.0);
}