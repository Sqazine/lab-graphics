vec3 mix3(vec3 a, vec3 b, vec3 c, vec3 barycentrics) 
{
	return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec2 mix3(vec2 a, vec2 b, vec2 c, vec3 barycentrics)
{
	return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec3 toneMap(in vec3 rgb, float limit)
{
	float luminance = 0.299 * rgb.x + 0.587 * rgb.y + 0.114 * rgb.z;
	return rgb / (1.0 + luminance / limit);
}

vec3 gammaCorrection(in vec3 ldr)
{
	return pow(ldr, vec3(0.45));
}

vec3 linear2Srgb(in vec3 hdr)
{
	return pow(hdr, vec3(2.2));
}

vec4 linear2Srgb(in vec4 hdr)
{
	return pow(hdr, vec4(2.2));
}

mat3 TBN(in vec3 N)
{
	vec3 Nt = normalize(((abs(N.z) > 0.99999f) ? vec3(-N.x * N.y, 1.0f - N.y * N.y, -N.y * N.z) :
                                          vec3(-N.x * N.z, -N.y * N.z, 1.0f - N.z * N.z)));
 vec3 Nb = cross(Nt, N);
	return mat3(Nt, Nb, N);
}

 /*
  * Schlick's approximation of the specular reflection coefficient R
  * (1 - cosTheta)^5
  */
float SchlickFresnel(float u)
{
	float m = clamp(1.0 - u, 0.0, 1.0);
	return m * m * m * m * m; // power of 5
}

float DielectricFresnel(float cos_theta_i, float eta)
{
	float sinThetaTSq = eta * eta * (1.0f - cos_theta_i * cos_theta_i);

	// Total internal reflection
	if (sinThetaTSq > 1.0)
		return 1.0;

	float cos_theta_t = sqrt(max(1.0 - sinThetaTSq, 0.0));

	float rs = (eta * cos_theta_t - cos_theta_i) / (eta * cos_theta_t + cos_theta_i);
	float rp = (eta * cos_theta_i - cos_theta_t) / (eta * cos_theta_i + cos_theta_t);

	return 0.5f * (rs * rs + rp * rp);
}

float GTR1(float NDotH, float a)
{
	float a2 = a * a;
	float t = 1.0 + (a2 - 1.0) * NDotH * NDotH;

	return (a2 - 1.0) / (PI * log(a2) * t);
}

 /*
  * Generalized-Trowbridge-Reitz (D)
  * Describes differential area of microfacets for the surface normal
  */
float GTR2(float NDotH, float a)
{
	float a2 = a * a;
	float t = 1.0 + (a2 - 1.0) * NDotH * NDotH;
	return a2 / (PI * t * t);
}


vec3 ImportanceSampleGTR1(float rgh, float r1, float r2)
{
	float a = max(0.001, rgh);
	float a2 = a * a;

	float phi = r1 * TWO_PI;

	float cosTheta = sqrt((1.0 - pow(a2, 1.0 - r1)) / (1.0 - a2));
	float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
	float sinPhi = sin(phi);
	float cosPhi = cos(phi);

	return vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

vec3 ImportanceSampleGTR2_aniso(float ax, float ay, float r1, float r2)
{
	float phi = r1 * TWO_PI;

	float sinPhi = ay * sin(phi);
	float cosPhi = ax * cos(phi);
	float tanTheta = sqrt(r2 / (1 - r2));

	return vec3(tanTheta * cosPhi, tanTheta * sinPhi, 1.0);
}

vec3 ImportanceSampleGTR2(float rgh, float r1, float r2)
{
	float a = max(0.001, rgh);

	float phi = r1 * TWO_PI;

	float cosTheta = sqrt((1.0 - r2) / (1.0 + (a * a - 1.0) * r2));
	float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
	float sinPhi = sin(phi);
	float cosPhi = cos(phi);

	return vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

 /*
  * The masking shadowing function Smith for GTR2 noraml distribution (G)
  */
float SmithGGX(float NDotv, float alphaG)	
{
	float a = alphaG * alphaG;
	float b = NDotv * NDotv;
	return 1.0 / (NDotv + sqrt(a + b - a * b));
}

 /*
  * Power heuristic often reduces variance even further for multiple importance sampling
  * Chapter 13.10.1 of pbrbook
  */
float powerHeuristic(float a, float b)
{
	float t = a * a;
	return t / (b * b + t);
}

/*
 * Max component of a vector3
 */
float max3(vec3 v) 
{
	return max (max (v.x, v.y), v.z);
}

vec3 cosineSampleHemisphere()
{
	float r1 = rand(seed);
	float r2 = rand(seed);

	vec3 dir;
	float r = sqrt(r1);
	float phi = 2.0 * PI * r2;

	dir.x = r * cos(phi);
	dir.y = r * sin(phi);
	dir.z = sqrt(max(0.0, 1.0 - dir.x*dir.x - dir.y*dir.y));

	return dir;
}

vec2 sampleUnitDisk()
{
	while(true)
	{
		float r1 = rand(seed);
		float r2 = rand(seed);
		vec2 p = 2.0 * vec2(r1, r2) - 1.0;
		if (dot(p, p) < 1.0) return p;
	}
}

vec3 uniformSampleSphere()
{
	float r1 = rand(seed);
	float r2 = rand(seed);

	 float z   = 1.0 - 2.0 * r1;
  float r   = sqrt(max(0.0, 1.0 - z * z));
  float phi = TWO_PI * r2;

  return vec3(r * cos(phi), r * sin(phi), z);
}

vec3 uniformSampleHemisphere()
{
	float r1 = rand(seed);
	float r2 = rand(seed);

  float r   = sqrt(max(0.0, 1.0 - r1 * r1));
  float phi = TWO_PI * r2;

  return vec3(r * cos(phi), r * sin(phi), r1);
}


vec3 offsetRay(in vec3 p, in vec3 n)
{
  const float intScale   = 256.0f;
  const float floatScale = 1.0f / 65536.0f;
  const float origin     = 1.0f / 32.0f;

  ivec3 of_i = ivec3(intScale * n.x, intScale * n.y, intScale * n.z);

  vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
                  intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
                  intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

  return vec3(abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,  //
              abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,  //
              abs(p.z) < origin ? p.z + floatScale * n.z : p_i.z);
}