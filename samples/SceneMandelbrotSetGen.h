#pragma once
#include "labgraphics.h"
#define MAX_FRAMES_IN_FLIGHT 2

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
	std::vector<std::unique_ptr<RasterCommandBuffer>> mRasterCommandBuffers;
	std::vector<std::unique_ptr<Semaphore>> mImageAvailableSemaphores;
	std::vector<std::unique_ptr<Semaphore>> mRenderFinishedSemaphores;
	std::vector<std::unique_ptr<Fence>> mInFlightFences;
	std::vector<Fence *> mImagesInFlight;
	size_t currentFrame = 0;
};