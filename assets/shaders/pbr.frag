#version 450 core
// This implementation is based on "Real Shading in Unreal Engine 4" SIGGRAPH 2013 course notes by Epic Games.
// See: http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf

const float PI = 3.141592;
const float EPSILON = 0.00001;

#define NUM_LIGHTS  3

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

struct Light
{
    vec3 direction;
    vec3 radiance;
    bool enabled;
};

layout(location=0) in Vertex
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
} inVertex;

layout(location=0) out vec4 color;

layout(set=0,binding=0) uniform TransformUniforms
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    mat4 skyboxRotationMatrix;
};

layout(set=0, binding=1) uniform ShadingUniforms
{
    Light lights[NUM_LIGHTS];
    vec3 eyePosition;
};

layout(set=1, binding=0) uniform sampler2D albedoTexture;
layout(set=1, binding=1) uniform sampler2D normalTexture;
layout(set=1, binding=2) uniform sampler2D metalnessTexture;
layout(set=1, binding=3) uniform sampler2D roughnessTexture;
layout(set=1, binding=4) uniform samplerCube specularTexture;
layout(set=1, binding=5) uniform samplerCube irradianceTexture;
layout(set=1, binding=6) uniform sampler2D specularBRDF_LUT;

vec3 GetNormal(sampler2D map,vec2 uv)
{
    vec3 tangentNormal=texture(map,uv).xyz*2.0-1.0;

    vec3 q1=dFdx(inVertex.position);
    vec3 q2=dFdy(inVertex.position);
    vec2 st1=dFdx(uv);
    vec2 st2=dFdy(uv);

    vec3 N=normalize(inVertex.normal);
    vec3 T=normalize(q1*st2.t-q2*st1.t);
    vec3 B=-normalize(cross(N,T));
    mat3 TBN=mat3(T,B,N);

    return normalize(TBN*tangentNormal);
}

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float NdfGGX(float cosLh,float roughness)
{
    float alpha=roughness*roughness;
    float alphaSq=alpha*alpha;
    float denom=(cosLh*cosLh)*(alphaSq-1.0)+1.0;
    return alphaSq/(PI*denom*denom);
}

// Single term for separable Schlick-GGX below.
float GaSchlickG1(float cosTheta,float k)
{
    return cosTheta/(cosTheta*(1.0-k)+k);
}


// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float GaSchlickGGX(float cosLi,float cosLo,float roughness)
{
    float r=roughness+1.0;
    float k=(r*r)/8.0;
    return GaSchlickG1(cosLi,k)*GaSchlickG1(cosLo,k);
}

vec3 FresnelSchlick(vec3 F0,float cosTheta)
{
    return F0+(vec3(1.0)-F0)*pow(1.0-cosTheta,5.0);
}

void main()
{
    vec3 albedo=texture(albedoTexture,inVertex.texcoord).rgb;
    float metalness=texture(metalnessTexture,inVertex.texcoord).r;
    float roughness=texture(roughnessTexture,inVertex.texcoord).r;

    vec3 Lo=normalize(eyePosition-inVertex.position);

    vec3 N=GetNormal(normalTexture,inVertex.texcoord);

    float cosLo=max(0.0,dot(N,Lo));

    vec3 Lr=2.0*cosLo*N-Lo;

    vec3 F0=mix(Fdielectric,albedo,metalness);

    vec3 directLighting=vec3(0);
    for(int i=0;i<NUM_LIGHTS;i++)
    {
        vec3 Li=-lights[i].direction;
        vec3 Lradiance=lights[i].radiance;

        vec3 Lh=normalize(Li+Lo);

        float cosLi=max(0.0,dot(N,Li));
        float cosLh=max(0.0,dot(N,Lh));

        vec3 F=FresnelSchlick(F0,max(0.0,dot(Lh,Lo)));

        float D=NdfGGX(cosLh,roughness);

        float G=GaSchlickGGX(cosLi,cosLo,roughness);

        vec3 kd=mix(vec3(1.0)-F,vec3(0.0),metalness);

        vec3 diffuseBRDF=kd*albedo;

        vec3 specularBRDF=(F*D*G)/max(EPSILON,4.0*cosLi*cosLo);
    
        directLighting+=(diffuseBRDF+specularBRDF)*Lradiance*cosLi;
    }

    vec3 ambientLighting;
    {
        vec3 diffuseDir=normalize(vec3(skyboxRotationMatrix*vec4(N,1.0)));

        vec3 irradiance=texture(irradianceTexture,diffuseDir).rgb;
        vec3 F=FresnelSchlick(F0,cosLo);
        vec3 kd=mix(vec3(1.0)-F,vec3(0.0),metalness);
        vec3 diffuseIBL=kd*albedo*irradiance;

         vec3 specularDir=normalize(vec3(skyboxRotationMatrix*vec4(Lr,1.0)));

        int specularTextureLevels=textureQueryLevels(specularTexture);
        vec3 specularIrradiance=textureLod(specularTexture,specularDir,roughness*specularTextureLevels).rgb;

        vec2 specularBRDF=texture(specularBRDF_LUT,vec2(cosLo,roughness)).rg;
        vec3 specularIBL=(F0*specularBRDF.x+specularBRDF.y)*specularIrradiance;
        ambientLighting=diffuseIBL+specularIBL;
    }

    color=vec4(directLighting+ambientLighting,1.0);
}
