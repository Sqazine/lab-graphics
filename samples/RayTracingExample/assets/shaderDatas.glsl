#define MAX_RECURSION 10

#define PI        3.14159265358979323846
#define TWO_PI    6.28318530717958647692
#define INV_PI    0.31830988618379067154
#define INV_2PI   0.15915494309189533577

#define EPS       0.001
#define INFINITY  1000000.0
#define MINIMUM   0.00001


struct RayPayload
{
    vec3 color;
    float distance;
    vec3 normal;
    vec3 scatterDir;
    uint randomSeed;
};

struct VertexAttribute
{
    vec4 normal;
    vec4 uv;
};

struct UniformParams
{
    vec4 camPos;
    vec4 camDir;
    vec4 camUp;
    vec4 camSide;
    vec4 camNearFarFov;
    uint currentSamplesCount;
};

vec2 BaryLerp(vec2 a,vec2 b,vec2 c,vec3 barycentrics)
{
    return a*barycentrics.x+b*barycentrics.y+c*barycentrics.z;
}

vec3 BaryLerp(vec3 a,vec3 b,vec3 c,vec3 barycentrics)
{
    return a*barycentrics.x+b*barycentrics.y+c*barycentrics.z;
}

float LinearToSRGB(float channel)
{
    if(channel<=0.0031308f)
        return 12.92f*channel;
    else
        return 1.055f*pow(channel,1.0f/2.4f)-0.055f;
}

vec3 LinearToSRGB(vec3 linear)
{
    return vec3(LinearToSRGB(linear.r),LinearToSRGB(linear.g),LinearToSRGB(linear.b));
}
