#define OBJECT_TYPE_MISS -1
#define OBJECT_TYPE_SPHERE=0
#define OBJECT_TYPE_PLANE=1
#define OBJECT_TYPE_BOX=2
#define OBJECT_TYPE_QUAD=3

struct Sphere
{
	float radius;
	vec3 center;
	int materialIdx;
};

struct Plane
{
	vec3 normal;
	vec3 center;
	int materialIdx;
};

struct Box
{
	vec3 bounds[2];
	int materialIdx;
};

struct Quad
{
	vec3 upLeft;
	vec3 upRight;
	vec3 lowRight;
	vec3 lowLeft;
	int materialIdx;
};

Quad BuildQuad(float w, float h, in vec3 center, in vec3 axis, float ang, int materialIdx)
{
	float half_w = w * 0.5f;
	float half_h = h * 0.5f;

	vec3 ul = { -half_w,  -half_h, 0.0f }; 
	vec3 ur = {  half_w, -half_h, 0.0f };
	vec3 lr = {  half_w, half_h, 0.0f }; 
	vec3 ll = { -half_w, half_h, 0.0f };

	mat4 rot = Rotation(axis, ang);
	ul = (rot * vec4(ul, 1.0f)).xyz + center;
	ur = (rot * vec4(ur, 1.0f)).xyz + center;
	lr = (rot * vec4(lr, 1.0f)).xyz + center;
	ll = (rot * vec4(ll, 1.0f)).xyz + center;

	return Quad(ul, ur, lr, ll, materialIdx);
}

Box BuildBox(in vec3 size, in vec3 center, int materialIdx)
{
	// Note that positive y is actually down.
	const vec3 b0 = vec3(-0.5f,  0.5f, -0.5f) * size + center; // Min (LB)
	const vec3 b1 = vec3( 0.5f, -0.5f,  0.5f) * size + center; // Max (RT)

	return Box(vec3[2](b0, b1), materialIdx);
}