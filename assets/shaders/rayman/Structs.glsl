#define PI 3.14159265358979323
#define TWO_PI 6.28318530717958648
#define INV_PI 0.3183098861837907
#define INV_2PI 0.1591549430918953

#define EPS 0.001
#define INFINITY 1e32
#define MINIMUM 0.00001

#define AREA_LIGHT 0
#define SPHERE_LIGHT 1

// See Random.glsl for more details
uint seed = 0;

struct Material
{
	vec3 albedoColor;
	vec3 emissionColor;
	vec3 specularColor;
	vec3 clearcoatColor;
	vec3 extinctionColor;

	float metallic;
	float roughness;
	float bumpiness;
	float emissionIntensity;

	float sheen;
	float sheenTint;
	float clearcoat;
	float clearcoatGloss;

	float transmission;
	float ior;
	float attenuationDistance;
	float thickness;

	float specular;
	float specularTint;
	float subsurface;

	int albedoTexID;
	int specularTexID;
	int metallicTexID;
	int roughnessTexID;

	int normalTexID;
	int emissionTexID;
	int opacityTexID;
};

struct Ray
{
	vec3 origin;
	vec3 direction;
};

struct Light
{
	vec3 position;
	vec3 emission;
	vec3 u;
	vec3 v;
	float area;
	int type;
	float radius;
};

struct Uniform
{
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
	uint lights;
	uint frame;
	float hdrResolution;
	float hdrMultiplier;
};
struct LightSample
{
	vec3 normal;
	vec3 position;
	vec3 emission;
	float pdf;
};

struct BsdfSample
{
	vec3 bsdfDir;
	float pdf;
};

struct RayPayload
{
	Ray ray;
	BsdfSample bsdf;
	vec3 radiance;
	vec3 absorption;
	vec3 beta;
	vec3 worldPos;
	vec3 normal;
	vec3 ffnormal;
	uint depth;
	bool stop;
	float eta;
	float distance;
};