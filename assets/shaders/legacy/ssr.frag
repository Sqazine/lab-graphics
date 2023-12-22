 
#version 450 core
layout(location=0) out vec4 fragColor;

layout(location=0) in vec2 texcoord;

layout(binding=0) uniform sampler2D gDepthSS;
layout(binding=1) uniform sampler2D gAlbedo;
layout(binding=2) uniform sampler2D gNormalMapNormalWS;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uniform float threshold;

uniform float rayStep;
uniform float maxSteps;

float ReconstructDepthVSFromDepthSS(vec2 uv)
{
    return -projectionMatrix[3][2]/(2.0*texture(gDepthSS,uv).w-1.0+projectionMatrix[2][2]);
}

vec3 ReconstructPositionVSFromDepthVS(vec2 uv)
{
    float aspect=1.0/(projectionMatrix[1][1]);
    float tangentHalfFOV=1.0/(aspect*projectionMatrix[0][0]);
    //1.0-uv还是不太懂
    return vec3(((1.0-uv.x)*2.0-1.0)*tangentHalfFOV*aspect,((1.0-uv.y)*2.0-1.0)*aspect,1.0)*ReconstructDepthVSFromDepthSS(uv);
}

vec3 ReconstructNormalVSFromPositionVS(vec3 posVS)
{
    return normalize(cross(dFdx(posVS),dFdy(posVS)));
}

vec3 PosVSToSS(vec3 posVS)
{
    vec4 posClip=projectionMatrix*vec4(posVS,1.0);
    vec3 posNDC=posClip.xyz/posClip.w;
    return posNDC*0.5+0.5;
}

bool rayTrace(vec3 origin,vec3 ref,out vec2 hitUV)
{
    const mat4 dither = mat4(
   0,       0.5,    0.125,  0.625,
   0.75,    0.25,   0.875,  0.375,
   0.1875,  0.6875, 0.0625, 0.5625,
   0.9375,  0.4375, 0.8125, 0.3125
);


    for(int i=1;i<maxSteps;++i)
    {
        int sampleCoordX = int(mod((textureSize(gDepthSS,0).x*texcoord.x),4));
        int sampleCoordY = int(mod((textureSize(gDepthSS,0).y*texcoord.y),4));
        float offset = dither[sampleCoordX][sampleCoordY];

        vec3 curPoint=origin+ref*(i*rayStep+offset);
        vec3 curPointSS=PosVSToSS(curPoint);
     vec3 lastPoint=curPoint;
        if(curPointSS.x>0&&curPointSS.x<1&&curPointSS.y>0&&curPointSS.y<1)
        {
            float reflectPointDepth=-ReconstructDepthVSFromDepthSS(curPointSS.xy);
            float negCurPointDepth=-curPoint.z;
            if(negCurPointDepth>reflectPointDepth)
            {
                if(abs(negCurPointDepth-reflectPointDepth)<threshold)
                {
                    hitUV=curPointSS.xy;
                    return true;   
                }

                vec3 begin=lastPoint;
                vec3 end=curPoint;
                vec3 midPoint=(begin+end)/2;

                int  count=0;

                while(true)
                {
                    count++;
                    negCurPointDepth=-midPoint.z;
                    vec3 midPointSS=PosVSToSS(midPoint);
                    float reflectPointDepthMiddle=-ReconstructDepthVSFromDepthSS(midPointSS.xy);
                    if(abs(negCurPointDepth-reflectPointDepthMiddle)<0.3*threshold)
                    {
                         hitUV=curPointSS.xy;
                    return true;   
                    }
                    if(negCurPointDepth>reflectPointDepthMiddle)
                        end=midPoint;
                    else if(negCurPointDepth<reflectPointDepthMiddle)
                        begin=midPoint;
                    midPoint=(begin+end)/2.0;
                    if(count>5)
                        break;
                }
            }
        }
        else lastPoint=curPoint;
    }
    return false;
}


float gauss[] = float[]
(
    0.00000067, 0.00002292, 0.00019117, 0.00038771, 0.00019117, 0.00002292, 0.00000067,
    0.00002292, 0.00078633, 0.00655965, 0.01330373, 0.00655965, 0.00078633, 0.00002292,
    0.00019117, 0.00655965, 0.05472157, 0.11098164, 0.05472157, 0.00655965, 0.00019117,
    0.00038771, 0.01330373, 0.11098164, 0.22508352, 0.11098164, 0.01330373, 0.00038771,
    0.00019117, 0.00655965, 0.05472157, 0.11098164, 0.05472157, 0.00655965, 0.00019117,
    0.00002292, 0.00078633, 0.00655965, 0.01330373, 0.00655965, 0.00078633, 0.00002292,
    0.00000067, 0.00002292, 0.00019117, 0.00038771, 0.00019117, 0.00002292, 0.00000067
);
 
 
vec3 GetGaussColor(vec2 uv)
{
    const int size = 7;
 
    vec3 finalColor = vec3(0,0,0);
 
    int idx = 0;
    for(int i = -3;i <= 3;i++)
    {
        for(int j = -3; j <= 3;j++)
        {
            vec2 offset_uv = uv + vec2(5.0 * i /textureSize(gDepthSS,0).x, 5.0 * j /textureSize(gDepthSS,0).y);
            vec3 color = texture(gAlbedo, offset_uv).xyz;
            float weight = gauss[idx++];
            finalColor = finalColor + weight * color;
        }
    }
 
    return finalColor;
}

void main()
{
    vec3 posVS=ReconstructPositionVSFromDepthVS(texcoord);
    vec3 normalVS=ReconstructNormalVSFromPositionVS(posVS);
    // vec3 posVS=mat3(viewMatrix)*texture(gDepthSS,texcoord).xyz;
    // vec3 normalVS=mat3(viewMatrix)*texture(gNormalMapNormalWS,texcoord).xyz;
    vec3 reflectedVS=normalize(reflect(normalize(posVS),normalize(normalVS)));

    vec2 hitUV;
    if(rayTrace(posVS,reflectedVS,hitUV))
        fragColor=vec4(GetGaussColor(hitUV),1.0);
    else fragColor=vec4(0.0);
}