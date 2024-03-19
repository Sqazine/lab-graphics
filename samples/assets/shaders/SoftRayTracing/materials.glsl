#define MAT_TYPE_INVALID -1
#define MAT_TYPE_DIFFUSE 0
#define MAT_TYPE_METALLIC 1
#define MAT_TYPE_DIELECTRIC 2

// A material has three attributes:
//
// 1. The primary color (albedo)
// 2. An integer denoting the type of the material (diffuse, metallic, etc.)
// 3. The material roughness (only used for metals)
// 4. A flag indicating whether or not this material belongs to a light source
struct Material
{
	int type;
	vec3 reflectance;
	float roughness;
	bool isLight;
};