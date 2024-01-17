 #version 450 core
layout(location=0) out vec4 gPositionWS;
layout(location=1) out vec3 gAlbedo;
layout(location=2) out vec3 gAoMetallicRoughness;
layout(location=3) out vec3 gVertexNormalWS;
layout(location=4) out vec3 gNormalMapNormalWS;
layout(location=5) out vec3 gClearcoatColor;
layout(location=6) out vec2 gClearcoatRoughnessWeight;

layout(location=0) in vec3 positionWS;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec3 normalWS;
layout(location=3) in mat3 TBN;

layout(binding=0) uniform sampler2D uAlbedoMap;
layout(binding=1) uniform sampler2D uAoMap;
layout(binding=2) uniform sampler2D uMetallicMap;
layout(binding=3) uniform sampler2D uNormalMap;
layout(binding=4) uniform sampler2D uRoughnessMap;

uniform vec3 uClearCoatColor;
uniform float uClearCoatRoughness;
uniform float uClearCoatWeight;

void main()
{
    gPositionWS=vec4(positionWS,gl_FragCoord.z);
    gAlbedo=texture(uAlbedoMap,texcoord).rgb;
    gAoMetallicRoughness.x=texture(uAoMap,texcoord).r;
    gAoMetallicRoughness.y=texture(uMetallicMap,texcoord).r;
    gAoMetallicRoughness.z=texture(uRoughnessMap,texcoord).r;
    gVertexNormalWS=normalWS;
    gNormalMapNormalWS=normalize(TBN*(texture(uNormalMap,texcoord).xyz*2.0-1.0));
    gClearcoatColor=uClearCoatColor;
    gClearcoatRoughnessWeight=vec2(uClearCoatRoughness,uClearCoatWeight);
}