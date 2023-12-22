struct Ray
{
	vec3 origin;
	vec3 direction;
};

vec3 GetPointAt(Ray r, float t)
{
	// This is the parametric equation for a ray. Given a scalar value
	// `t`, it returns a point along the ray. We will use this throughout
	// the path tracing algorithm.
	return r.origin+r.direction*t;
}


// An intersection holds several values:
// 1. The material index of the object that was hit (`material_index`)
// 2. The type of object that was hit (or a miss)
// 3. The direction vector of the `incident` ray
// 4. The location of intersection in world space (`position`)
// 5. The `normal` vector of the object that was hit, calculated at `position`
// 6. A scalar value `t`, which denotes a point along the incident ray
struct RayPayload
{
	int materialIdx;
	int objectType;
	int objectIdx;
	vec3 inDir;
	vec3 pos;
	vec3 normal;
	float t;
};

bool IntersectSphere(in Sphere sphere,in Ray r,out float t)
{
	vec3 oc=r.origin-sphere.center;
	float a=dot(r.direction,r.direction);
	float b=dot(oc,r.direction);
	float c=dot(oc,oc)-sphere.radius * sphere.radius;

	float discriminant = b * b - a * c;

	// Avoid taking the square root of a negative number.
	if(discriminant<0.0)
		return false;

	discriminant=sqrt(discriminant);
	float t0=(-b+discriminant)/a;
	float t1=(-b-discriminant)/a;

	if(t1>EPSILON)
		t=t1;
	else if(t0>EPSILON)
		t=t0;
	else
		return false;

	return true;
}

bool IntersectPlane(in Plane plane,in Ray r,out float t)
{
	// We can write the equation for a plane, given its normal vector and
	// a single point `p` that lies on the plane. We can test if a different
	// point `v` is on the plane via the following formula:
	//
	//		dot(v - p, n) = 0
	// 
	// Now, we can substitute in the equation for our ray:
	//
	//		dot((o - t * d) - p, n) = 0
	//
	// Solving for `t` in the equation above, we obtain:
	//
	// 		numerator = dot(p - o, n)
	//		denominator = dot(d, n)
	//		t = numerator / denominator
	//
	// If the ray is perfectly parallel to the plane, then the denominator
	// will be zero. If the ray intersects the plane from behind, then `t`
	// will be negative. We want to avoid both of these cases.

	float denominator=dot(r.direction,plane.normal);
	float numerator=dot(plane.center-r.origin,plane.normal);

	if(denominator>EPSILON)
		return false;

	t=numerator/denominator;

	if(t>0.0)
		return true;

	return false;
}

bool IntersectBox(in Box box,in Ray r,out float t)
{
	// See: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
	// and: http://blog.johnnovak.net/2016/10/22/the-nim-raytracer-project-part-4-calculating-box-normals/
	// and: https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
	const vec3 invDirection=1.0/r.direction;

	// `bounds[0]` is the corner of the AABB with minimal coordinates - left bottom
	// `bounds[1]` is the corner of the AABB with maximal coordinates - right top

	const float t1=(box.bounds[0].x - r.origin.x) * invDirection.x;
	const float t2=(box.bounds[1].x - r.origin.x) * invDirection.x;
	const float t3=(box.bounds[0].y - r.origin.y) * invDirection.y;
	const float t4=(box.bounds[1].y - r.origin.y) * invDirection.y;
	const float t5=(box.bounds[0].z - r.origin.z) * invDirection.z;
	const float t6=(box.bounds[1].z - r.origin.z) * invDirection.z;

	const float tmin=max(max(min(t1,t2),min(t3,t4),min(t5,t6)));
	const float tmax=min(min(max(t1,t2),max(t3,t4),max(t5,t6)));

	if(tmax<0)
	{
		t=max;
		return false;
	}

	if(tmin>tmax)
	{
		t=tmax;
		return false;
	}

	t=tmin;
	return true;
}

bool IntersectQuad(in Quad quad,in Ray r,out float t)
{
	// See: `https://stackoverflow.com/questions/21114796/3d-ray-quad-intersection-test-in-java`
	const vec3 edge0=quad.upRight-quad.upLeft;
	const vec3 edge1=quad.lowLeft-quad.upLeft;
	const vec3 normal=normalize(cross(edge0,edge1));

	Plane plane=Plane(normal,vec3(quad.upRight),0);

	float temp_t;
	if(IntersectPlane(plane,r,temp_t))
	{
		vec3 m=r.origin+r.direction*temp_t;

		const float u=dot(m-quad.upLeft,edge0);
		const float v=dot(m-quad.upLeft,edge1);

		if(u>=EPSILON&&v>=EPSILON&&u<=dot(edge0,edge0)&&v<=dot(edge1,edge1))
		{
			t=temp_t;
			return true;
		}
	}

	return false;
}