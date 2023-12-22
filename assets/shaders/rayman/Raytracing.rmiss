#version 460

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

// Replaced by ShaderCompiler.h
#DEFINE_BLOCK

#include "Structs.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

layout(binding = 3) readonly uniform UniformBufferObject { Uniform ubo; };
layout(binding = 4) readonly buffer VertexArray { float Vertices[]; };
layout(binding = 8) uniform sampler2D[] TextureSamplers;
layout(binding = 9) readonly buffer LightArray { Light[] Lights; };
#ifdef USE_HDR
layout(binding = 12) uniform sampler2D[] HDRs;
#endif

#include "Random.glsl"
#include "Math.glsl"
#include "Sampling.glsl"
#ifdef USE_HDR
#include "HDR.glsl"
#endif


void main()
{
	#ifdef USE_HDR
	{
		// Stop path tracing loop from rgen shader
		payload.stop = true;
		payload.ffnormal = vec3(0.0);
		payload.worldPos = vec3(0.0);

		LightSample lightSample;

		if (interesetsEmitter(lightSample, INFINITY))
		{
			vec3 Le = sampleEmitter(lightSample, payload.bsdf);
			payload.radiance += Le * payload.beta;
			return;
		}
		float misWeight = 1.0f;
		vec2 uv = vec2((PI + atan(gl_WorldRayDirectionEXT.z, gl_WorldRayDirectionEXT.x)) * INV_2PI, acos(gl_WorldRayDirectionEXT.y) * INV_PI);
		
		if (payload.depth > 0)
		{
			float lightPdf = envPdf();
			misWeight = powerHeuristic(payload.bsdf.pdf, lightPdf);
		}

		payload.radiance += misWeight * texture(HDRs[0], uv).xyz * payload.beta * ubo.hdrMultiplier;
	}
	#endif
}

