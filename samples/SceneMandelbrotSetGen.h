#pragma once
#include "labgraphics.h"

constexpr uint32_t WORKGROUP_SIZE = 32;

class SceneMandelbrotSetGen : public Scene
{
public:
	SceneMandelbrotSetGen() = default;
	~SceneMandelbrotSetGen() = default;

	void Init() override;
	void Update() override;
	void Render() override;

private:
	struct Uniform
	{
		uint32_t width;
		uint32_t height;
	};

	Vector2i32 mWindowExtent;

	std::unique_ptr<DescriptorTable> mDescriptorTable;
	DescriptorSet *mDescriptorSet;
	std::unique_ptr<PipelineLayout> mPipelineLayout;

	Uniform mUniform;
	std::unique_ptr<UniformBuffer<Uniform>> mUniformBuffer;

	std::unique_ptr<ComputePipeline> mComputePipeline;
	std::unique_ptr<ComputeCommandBuffer> mComputeCommandBuffer;
	std::unique_ptr<Buffer> mComputeImgBuffer;

	std::unique_ptr<RasterPipeline> mRasterPipeline;
	std::unique_ptr<RasterPass> mRasterPass;
};