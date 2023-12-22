#version 460

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

// Replaced by ShaderCompiler.h
#DEFINE_BLOCK

#include "Structs.glsl"

hitAttributeEXT vec2 hit;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadEXT bool isShadowed;

layout(binding = 0, set = 0) uniform accelerationStructureEXT TLAS;
layout(binding = 3) readonly uniform UniformBufferObject { Uniform ubo; };
layout(binding = 4) readonly buffer VertexArray { float Vertices[]; };
layout(binding = 5) readonly buffer IndexArray { uint Indices[]; };
layout(binding = 6) readonly buffer MaterialArray { Material[] Materials; };
layout(binding = 7) readonly buffer OffsetArray { uvec2[] Offsets; };
layout(binding = 8) uniform sampler2D[] TextureSamplers;
layout(binding = 9) readonly buffer LightArray { Light[] Lights; };
#ifdef USE_HDR
layout(binding = 12) uniform sampler2D[] HDRs;
#endif

#include "Random.glsl"
#include "Math.glsl"
#ifdef USE_HDR
#include "HDR.glsl"
#endif
#include "Sampling.glsl"

struct Vertex
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
	int materialIndex;
};

Vertex unpack(uint index)
{
	const uint vertexSize = 9;
	const uint offset = index * vertexSize;
	
	Vertex vertex;
	
	vertex.position = vec3(Vertices[offset + 0], Vertices[offset + 1], Vertices[offset + 2]);
	vertex.normal = vec3(Vertices[offset + 3], Vertices[offset + 4], Vertices[offset + 5]);
	vertex.texcoord = vec2(Vertices[offset + 6], Vertices[offset + 7]);
	vertex.materialIndex = floatBitsToInt(Vertices[offset + 8]);

	return vertex;
}

vec3 EvalDielectricReflection(in Material material, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
{
    if (dot(N, L) < 0.0)
        return vec3(0.0);

    float F = DielectricFresnel(dot(V, H),  payload.eta);
    float D = GTR2(dot(N, H), material.roughness);

    pdf = D * dot(N, H) * F / (4.0 * dot(V, H));

    float G = SmithGGX(abs(dot(N, L)), material.roughness) * SmithGGX(dot(N, V), material.roughness);

    return pow(material.albedoColor.xyz,vec3(0.38)) * F * D * G;
}

vec3 EvalDielectricRefraction(in Material material, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
{
    float eta = payload.eta;

    pdf = 0.0;
    if (dot(N, L) >= 0.0)
        return vec3(0.0);

    float F = DielectricFresnel(abs(dot(V, H)), eta);
    float D = GTR2(dot(N, H), material.roughness);

    float denomSqrt = dot(L, H) + dot(V, H) * eta;
    pdf = D * dot(N, H) * (1.0 - F) * abs(dot(L, H)) / (denomSqrt * denomSqrt);

    float G = SmithGGX(abs(dot(N, L)), material.roughness) * SmithGGX(dot(N, V), material.roughness);

    return pow(material.albedoColor.xyz,vec3(0.38)) * (1.0 - F) * D * G * abs(dot(V, H)) * abs(dot(L, H)) * 4.0 * eta * eta / (denomSqrt * denomSqrt);
}

vec3 EvalSpecular(in Material material, in vec3 Cspec0, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
{
    pdf = 0.0;
    if (dot(N, L) <= 0.0)
        return vec3(0.0);

    float D = GTR2(dot(N, H), material.roughness);
    pdf = D * dot(N, H) / (4.0 * dot(V, H));

    float FH = SchlickFresnel(dot(L, H));
    vec3 F = mix(Cspec0, vec3(1.0), FH);
    float G = SmithGGX(abs(dot(N, L)), material.roughness) * SmithGGX(abs(dot(N, V)), material.roughness);

    return F * D * G * material.specularColor.xyz;
}

vec3 EvalClearcoat(in Material material, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
{
    pdf = 0.0;
    if (dot(N, L) <= 0.0)
        return vec3(0.0);

    float D = GTR2(dot(N, H), mix(0.001, 0.1, material.clearcoatGloss));
    pdf = D * dot(N, H) / (4.0 * dot(V, H));

    float FH = SchlickFresnel(dot(L, H));
    float F = mix(0.04, 1.0, FH);
    float G = SmithGGX(dot(N, L), 0.25) * SmithGGX(dot(N, V), 0.25);

    return vec3(material.clearcoat * F * D * G) * material.clearcoatColor.xyz;
}

vec3 EvalDiffuse(in Material material, in vec3 Csheen, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
{
    pdf = 0.0;
    if (dot(N, L) <= 0.0)
        return vec3(0.0);

    pdf = dot(N, L) * (1.0 /PI);

    // Diffuse
    float FL = SchlickFresnel(dot(N, L));
    float FV = SchlickFresnel(dot(N, V));
    float FH = SchlickFresnel(dot(L, H));
    float Fd90 = 0.5 + 2.0 * dot(L, H) * dot(L, H) * material.roughness;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

    // Fake Subsurface TODO: Replace with volumetric scattering
    float Fss90 = dot(L, H) * dot(L, H) * material.roughness;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / (dot(N, L) + dot(N, V)) - 0.5) + 0.5);

    vec3 Fsheen = FH * material.sheen * Csheen;

    return ((1.0 / PI) * mix(Fd, ss, material.subsurface)* material.albedoColor.xyz + Fsheen) * (1.0 - material.metallic);
}

vec3 DisneySample(in Material material, inout vec3 L, inout float pdf)
{
    pdf = 0.0;
    vec3 f = vec3(0.0);

    float albedoRatio = 0.5 * (1.0 - material.metallic);
    float transWeight = (1.0 - material.metallic) * material.transmission;

    vec3 Cdlin = material.albedoColor.xyz;
    float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // luminance approx.

    vec3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : vec3(1.0f); // normalize lum. to isolate hue+sat
    vec3 Cspec0 = mix(material.specular * 0.08 * mix(vec3(1.0), Ctint, material.specularTint), Cdlin, material.metallic);
    vec3 Csheen = mix(vec3(1.0), Ctint, material.sheenTint);

    vec3 N = payload.ffnormal;
    vec3 V = -gl_WorldRayDirectionEXT;

    mat3 frame = TBN(N);

    float r1 = rand(seed);
    float r2 = rand(seed);

    if (rand(seed) < transWeight)
    {
        vec3 H = ImportanceSampleGTR2(material.roughness, r1, r2);
        H = frame * H;

        if (dot(V, H) < 0.0)
            H = -H;

        vec3 R = reflect(-V, H);
        float F = DielectricFresnel(abs(dot(R, H)),payload.eta);

        // TODO: thinwalled material require correct TBN Matrix
        // if(material.thickness<0.01)
        // {
        //     if(dot(payload.ffnormal,payload.normal)<0.0)
        //         F=0.0f;
        //     payload.eta=1.001;
        // }

        // Reflection/Total internal reflection
        if (r2 < F)
        {
            L = normalize(R);
            f = EvalDielectricReflection(material, V, N, L, H, pdf);
        }
        else // Transmission
        {
            L = normalize(refract(-V, H, payload.eta));
            f = EvalDielectricRefraction(material, V, N, L, H, pdf);
        }

        f *= transWeight;
        pdf *= transWeight;
    }
    else
    {
        if (rand(seed) < albedoRatio)
        {
            L = cosineSampleHemisphere();
            L = frame * L;

            vec3 H = normalize(L + V);

            f = EvalDiffuse(material, Csheen, V, N, L, H, pdf);
            pdf *= albedoRatio;
        }
        else // Specular
        {
            float primarySpecRatio = 1.0 / (1.0 + material.clearcoat);

            // Sample primary specular lobe
            if (rand(seed) < primarySpecRatio)
            {
                // TODO: Implement http://jcgt.org/published/0007/04/01/
                vec3 H = ImportanceSampleGTR2(material.roughness, r1, r2);
                H = frame * H;

                if (dot(V, H) < 0.0)
                    H = -H;

                L = normalize(reflect(-V, H));

                f = EvalSpecular(material, Cspec0, V, N, L, H, pdf);
                pdf *= primarySpecRatio * (1.0 - albedoRatio);
            }
            else // Sample clearcoat lobe
            {
                vec3 H = ImportanceSampleGTR2(mix(0.001, 0.1, material.clearcoatGloss), r1, r2);
                H = frame * H;

                if (dot(V, H) < 0.0)
                    H = -H;

                L = normalize(reflect(-V, H));

                f = EvalClearcoat(material, V, N, L, H, pdf);
                pdf *= (1.0 - primarySpecRatio) * (1.0 - albedoRatio);
            }
        }

        f *= (1.0 - transWeight);
        pdf *= (1.0 - transWeight);
    }

    return f;
}

vec3 DisneyEval(Material material, vec3 L, inout float pdf)
{
    vec3 N = payload.ffnormal;
    vec3 V = -gl_WorldRayDirectionEXT;
    float eta = payload.eta;

    vec3 H;
    bool refl = dot(N, L) > 0.0;

    if (refl)
        H = normalize(L + V);
    else
        H = normalize(L + V * eta);

    if (dot(V, H) < 0.0)
        H = -H;

    float albedoRatio = 0.5 * (1.0 - material.metallic);
    float primarySpecRatio = 1.0 / (1.0 + material.clearcoat);
    float transWeight = (1.0 - material.metallic) * material.transmission;

    vec3 brdf = vec3(0.0);
    vec3 bsdf = vec3(0.0);
    float brdfPdf = 0.0;
    float bsdfPdf = 0.0;

    if (transWeight > 0.0)
    {
        // Reflection
        if (refl)
        {
            bsdf = EvalDielectricReflection(material, V, N, L, H, bsdfPdf);
        }
        else // Transmission
        {
            bsdf = EvalDielectricRefraction(material, V, N, L, H, bsdfPdf);
        }
    }

    float m_pdf;

    if (transWeight < 1.0)
    {
        vec3 Cdlin = material.albedoColor.xyz;
        float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // luminance approx.

        vec3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : vec3(1.0f); // normalize lum. to isolate hue+sat
        vec3 Cspec0 = mix(material.specular * 0.08 * mix(vec3(1.0), Ctint, material.specularTint), Cdlin, material.metallic);
        vec3 Csheen = mix(vec3(1.0), Ctint, material.sheenTint);

        // Diffuse
        brdf += EvalDiffuse(material, Csheen, V, N, L, H, m_pdf);
        brdfPdf += m_pdf * albedoRatio;

        // Specular
        brdf += EvalSpecular(material, Cspec0, V, N, L, H, m_pdf);
        brdfPdf += m_pdf * primarySpecRatio * (1.0 - albedoRatio);

        // Clearcoat
        brdf += EvalClearcoat(material, V, N, L, H, m_pdf);
        brdfPdf += m_pdf * (1.0 - primarySpecRatio) * (1.0 - albedoRatio);
    }

    pdf = mix(brdfPdf, bsdfPdf, transWeight);

    return mix(brdf, bsdf, transWeight);
}

vec3 directLight(in Material material)
{
	vec3 L = vec3(0);
	float tMin = MINIMUM;
	float tMax = INFINITY;
	uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

	BsdfSample bsdfSample;

	vec3 surfacePos = payload.worldPos;

	/* Environment Light */
	#ifdef USE_HDR
	{
		vec3 color = vec3(0);
		vec4 dirPdf = envSample(color);
		vec3 lightDir = dirPdf.xyz;
		float lightPdf = dirPdf.w;

		isShadowed = true;

		// Shadow ray (payload 1 is Shadow.miss)
		traceRayEXT(TLAS, flags, 0xFF, 0, 0, 1, surfacePos, tMin, lightDir, tMax, 1);

		if (!isShadowed)
		{
			vec3 F = DisneyEval(material, lightDir, bsdfSample.pdf);

			float cosTheta = abs(dot(lightDir, payload.ffnormal));
			float misWeight = powerHeuristic(lightPdf, bsdfSample.pdf);

			if (misWeight > 0.0)
				L += misWeight * F * cosTheta * color / (lightPdf + EPS);
		}
	}
	#endif

	if (ubo.lights > 0)
	{
		Light light;
		int index = int(rand(seed) * float(ubo.lights));
		light = Lights[index];

		// In the case there is only environmnet light and not analitic light
		// light type is set to -1 and discarded.
		if (light.type == -1)
			return L;

		LightSample sampled = sampleLight(light);	
		vec3 lightDir       = sampled.position - surfacePos;
		float lightDist     = length(lightDir);
		float lightDistSq   = lightDist * lightDist;
		lightDir = normalize(lightDir);

		isShadowed = true;

		// The light has to be visible from the surface. Less than 90� between vectors.
		if (dot(payload.ffnormal, lightDir) <= 0.0 || dot(lightDir, sampled.normal) >= 0.0)
			return L;

		// Shadow ray (payload 1 is Shadow.miss)
		traceRayEXT(TLAS, flags, 0xFF, 0, 0, 1, surfacePos, tMin, lightDir, lightDist, 1);
		
		if (!isShadowed)
		{
			vec3 F = DisneyEval(material, lightDir, bsdfSample.pdf);

			float lightPdf = lightDistSq / (light.area * abs(dot(sampled.normal, lightDir)));
			float cosTheta = abs(dot(payload.ffnormal, lightDir));
			float misWeight = powerHeuristic(lightPdf, bsdfSample.pdf);

			L += misWeight * F * cosTheta * sampled.emission / (lightPdf + EPS);
		}
	}

	return L;
}

void main()
{
	uvec2 offsets = Offsets[gl_InstanceCustomIndexEXT];
	uint indexOffset = offsets.x;
	uint vertexOffset = offsets.y;

	const Vertex v0 = unpack(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 0]);
	const Vertex v1 = unpack(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 1]);
	const Vertex v2 = unpack(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 2]);

	Material material = Materials[v0.materialIndex];

	const vec3 barycentrics = vec3(1.0 - hit.x - hit.y, hit.x, hit.y);
	const vec2 texcoord = mix3(v0.texcoord, v1.texcoord, v2.texcoord, barycentrics);
	const vec3 worldPos = mix3(v0.position, v1.position, v2.position, barycentrics);
	vec3 normal=normalize(mix3(v0.normal,v1.normal,v2.normal,barycentrics));

	payload.normal=normal;

	// face forward normal
	vec3 ffnormal = dot(normal, gl_WorldRayDirectionEXT) <= 0.00001 ? normal : -normal;
	
	float eta = dot(normal, ffnormal) > 0.0 ? (1.0 / material.ior) : material.ior;

	// albedo map
	if (material.albedoTexID >= 0)
		material.albedoColor.xyz = linear2Srgb(texture(TextureSamplers[material.albedoTexID], texcoord)).xyz;		

	//specular map
	if (material.specularTexID >= 0)
		material.specularColor.xyz = linear2Srgb(texture(TextureSamplers[material.specularTexID], texcoord)).xyz;

	// metallic map
	if (material.metallicTexID >= 0)
		material.metallic=texture(TextureSamplers[material.metallicTexID],texcoord).r;

	//roughness map
	if(material.roughnessTexID>=0)
		material.roughness=texture(TextureSamplers[material.roughnessTexID], texcoord).r;
	material.roughness=max(material.roughness,0.001);

	// normal map
	if (material.normalTexID >= 0)
	{
    	mat3 tbn = TBN(normal);
    	vec3 texNormal = texture(TextureSamplers[material.normalTexID], texcoord).xyz;
    	vec3 tangentSpaceNormal = texNormal * 2.0 - 1.0;
    	tangentSpaceNormal = normalize(tangentSpaceNormal);
    	tangentSpaceNormal = normalize(mix(vec3(0.0,0.0,1.0),tangentSpaceNormal,material.bumpiness));
    	vec3 worldSpaceNormal = tbn * tangentSpaceNormal;
		payload.normal=worldSpaceNormal;
		ffnormal = dot(worldSpaceNormal, gl_WorldRayDirectionEXT) <= 0.00001 ? worldSpaceNormal : -worldSpaceNormal;
	}

	//emission map
	if(material.emissionTexID>=0)
		material.emissionColor.xyz *= texture(TextureSamplers[material.emissionTexID], texcoord).xyz;
	//opacity map
	if(material.opacityTexID>=0)
	{
		float opacity=texture(TextureSamplers[material.opacityTexID],texcoord).r;
		material.transmission=1.0-opacity;
		material.ior=1.001;
		material.thickness=1.0;
		material.attenuationDistance=0.0;
		material.extinctionColor.rgb=vec3(1.0);
		if(opacity<0.01)
		{
			material.albedoColor.rgb=vec3(1.0);
			material.specularColor.rgb=vec3(1.0);
			material.specular=0.0;
			material.specularTint=0.0;
			material.metallic=0.0;
		}
	}

	payload.worldPos = worldPos;
	payload.ffnormal = ffnormal;
	payload.eta = eta;
	
	seed =initRandom(gl_LaunchSizeEXT.xy, gl_LaunchIDEXT.xy,ubo.frame);

    #ifdef DEBUG_AO_OUTPUT
	    float tMin = MINIMUM;
	    float tMax = 0.05;
	    uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

	    uint samples = 32;
	    float ao = 0.0;

	    for(uint i = 0; i < samples; ++i)
	    {
	    	mat3 tbn = TBN(normal);
	    	vec3 dir = normalize(tbn * cosineSampleHemisphere());
	    	isShadowed = true;

	    	traceRayEXT(TLAS, flags, 0xFF, 0, 0, 1, worldPos, tMin, dir, tMax, 1);

	    	if (isShadowed)
	    		ao++;
	    }

	    payload.radiance = vec3(1.0-ao/samples);
	    payload.stop = true;
    #elif defined(DEBUG_ALBEDO_OUTPUT)
        payload.radiance = material.albedoColor.xyz ;
	    payload.stop = true;
     #elif defined(DEBUG_NORMAL_OUTPUT)
        payload.radiance = ffnormal ;
	    payload.stop = true;
    #else
        BsdfSample bsdfSample;
        LightSample lightSample;	

        if (interesetsEmitter(lightSample, gl_HitTEXT))
        {
        	vec3 Le = sampleEmitter(lightSample, payload.bsdf);

        	if (dot(payload.ffnormal, lightSample.normal) > 0.f)
        		payload.radiance += Le * payload.beta;

        	payload.stop = true;
        	return;
        }

        payload.radiance+=material.emissionColor.xyz*material.emissionIntensity*payload.beta;
        payload.beta *= exp(-payload.absorption * gl_HitTEXT);
        payload.radiance +=(directLight(material)) * payload.beta;

        vec3 F = DisneySample(material, bsdfSample.bsdfDir, bsdfSample.pdf);

        float cosTheta = abs(dot(ffnormal, bsdfSample.bsdfDir));

        if (bsdfSample.pdf <= 0.0)
        {
        	payload.stop = true;
        	return;
        }

        payload.beta *= F * cosTheta / (bsdfSample.pdf + EPS);

           
        if (dot(ffnormal, bsdfSample.bsdfDir) < 0.0)
        	payload.absorption = -log(material.extinctionColor.rgb) / (material.attenuationDistance + EPS);

        payload.radiance = toneMap(payload.radiance, 1.0);
        payload.bsdf = bsdfSample;
        payload.distance=gl_HitTEXT;
        payload.ray.direction = bsdfSample.bsdfDir;
        payload.ray.origin=offsetRay(worldPos, dot(bsdfSample.bsdfDir, payload.ffnormal) > 0 ? payload.ffnormal : -payload.ffnormal);

        // Russian roulette
        if (payload.depth > 0)
        {
        	float q = max(0.001,max3(payload.beta));
        	if (rand(seed) > q) 
        		payload.stop = true;
        	payload.beta /= q;
        }
    #endif
}
