#pragma once
#include <vector>
#include <memory>
#include "labgraphics.h"

const std::string rayGenShaderSrc = R"(
#version 460 core
#extension GL_EXT_ray_tracing:enable

layout(location=0) rayPayloadEXT vec4 payload;
layout(binding=0,set=0) uniform accelerationStructureEXT as;
layout(binding=1,rgba32f) uniform image2D img;

void main()
{
	vec2 pixelCenter=vec2(gl_LaunchIDEXT.xy)+vec2(0.5);
	vec2 uv=pixelCenter/vec2(gl_LaunchSizeEXT.xy);
	vec2 d=uv*2.0-1.0;
	float aspectRatio=float(gl_LaunchSizeEXT.x)/float(gl_LaunchSizeEXT.y);

	vec3 ro=vec3(0,0,-1.5);
	vec3 rd=normalize(vec3(d.x*aspectRatio,d.y,1));

	payload=vec4(0);
	
	traceRayEXT(
		as,
		gl_RayFlagsOpaqueEXT,
		0xFF,
		0,
		0,
		0,
		ro,
		0.001,
		rd,
		100.0,
		0
	);

	imageStore(img,ivec2(gl_LaunchIDEXT),vec4(payload.rgb,1.0));
}
)";

const std::string rayChitShaderSrc = R"(
#version 460 core
#extension GL_EXT_ray_tracing:enable

layout(location = 0) rayPayloadInEXT vec4 payload;
layout(location = 0) callableDataEXT vec3 outColor;

hitAttributeEXT vec3 attribs;//重心坐标空间下的相交点

void main()
{
	executeCallableEXT(gl_GeometryIndexEXT, 0);

	vec3 bary=vec3(1.0-attribs.x-attribs.y,attribs.x,attribs.y);
	vec3 color = (outColor == vec3(0,0,0) ? outColor:bary);
	
	payload=vec4(color,1.0);
}
)";

const std::string rayMissShaderSrc = R"(
#version 460 core
#extension GL_EXT_ray_tracing:enable
layout(location=0) rayPayloadInEXT vec4 payload;

void main()
{
	payload=vec4(vec3(0.3),0.0);
}
)";

const std::string rayCallableShaderSrc = R"(

#version 460 core
#extension GL_EXT_ray_tracing : enable

layout(location = 0) callableDataInEXT vec3 outColor;

void main()
{
	vec2 pos = vec2(gl_LaunchIDEXT / 2);
	outColor=vec3(mod(pos.y, 2.0));
}

)";

struct VertexDef
{
    float pos[3];
};

struct Mesh
{
    std::vector<VertexDef> vertices;
    std::vector<uint32_t> indices;
};

struct TriangleDef : Mesh
{
    TriangleDef()
    {
        vertices = {
            {{1.0f, 1.0f, 0.0f}},
            {{-1.0f, 1.0f, 0.0f}},
            {{0.0f, -1.0f, 0.0f}}};
        indices = {0, 1, 2};
    }
};

class SceneRayTraceTriangle : public Scene
{
public:
    void Init() override;
    void ProcessInput() override;
    void Update() override;
    void Render() override;
    void CleanUp() override;

private:
    std::unique_ptr<Image2D> mOffscreenImage2D;

	std::unique_ptr<RayTracePass> mRayTracePass;

    std::unique_ptr<RayTracePipeline> mRayTracePipeline;

    std::unique_ptr<BLAS> mBlas;
    std::unique_ptr<TLAS> mTlas;

    TriangleDef mTriangle;

    std::unique_ptr<DescriptorTable> mDescriptorTable;
    DescriptorSet* mDescriptorSet;
    std::unique_ptr<PipelineLayout> mPipelineLayout;
};