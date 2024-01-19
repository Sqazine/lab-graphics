#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include "VK/Buffer.h"
#include "VK/CommandBuffer.h"
#include "VK/DescriptorTable.h"
#include "VK/SwapChain.h"

enum class PostProcessType
{
	NONE = -1,
	DENOISER = 0,
	EDGE_DETECTER = 1,
	SHARPENER = 2,
};

namespace Uniforms
{
	struct Compute final
	{
		uint32_t iteration;
		float colorPhi{};
		float normalPhi{};
		float positionPhi{};
		float stepWidth{};
	};
}

class PostProcessPass
{
public:
	PostProcessPass(
		const SwapChain &swapChain,
		Device &device,
		const Image2D &inputImageView,
		const Image2D &normalsImageView,
		const Image2D &positionsImageView);
	~PostProcessPass();

	void Process(int32_t shaderId);

	 Image2D *GetOutputImage() const
	{
		return mOutputImage.get();
	}
private:
	int32_t mCurrentShader = -1;

	const SwapChain &swapChain;
	Device &mDevice;

	std::vector<std::unique_ptr<ComputePipeline>> mPipelines;
	std::unique_ptr<PipelineLayout> mPipelineLayout{};

	std::unique_ptr<DescriptorTable> mDescriptorTable;
	DescriptorSet* mDescriptorSet;

	const Image2D &mInputImage;
	const Image2D &mNormalsImage;
	const Image2D &mPositionsImage;

	std::unique_ptr<GpuImage2D> mOutputImage;

	std::unique_ptr<UniformBuffer<Uniforms::Compute>> mUniformBuffer;

	std::unique_ptr<ComputeCommandBuffer> mComputeCommandBuffer;
};
