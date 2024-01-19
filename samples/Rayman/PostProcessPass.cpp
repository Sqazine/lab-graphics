#include "PostProcessPass.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <array>

#include "VK/Device.h"
#include "VK/SwapChain.h"
#include "VK/Image.h"
#include "VK/ImageView.h"
#include "VK/CommandBuffer.h"
#include "VK/CommandPool.h"
#include "VK/SyncObject.h"
#include "VK/Shader.h"
#include "VK/Buffer.h"
#include "VK/DescriptorSetLayout.h"
#include "VK/DescriptorPool.h"
#include "Logger.h"

PostProcessPass::PostProcessPass(const SwapChain &swapChain,
								 Device &device,
								 const Image2D &inputImage,
								 const Image2D &normalsImage,
								 const Image2D &positionsImage) : swapChain(swapChain), mDevice(device),
																  mInputImage(inputImage),
																  mNormalsImage(normalsImage),
																  mPositionsImage(positionsImage)
{
	mComputeCommandBuffer = device.GetComputeCommandPool()->CreatePrimaryCommandBuffer();

	const auto extent = swapChain.GetExtent();
	const auto outputFormat = swapChain.GetFormat();

	mOutputImage = std::make_unique<GpuImage2D>(mDevice, extent.x, extent.y, outputFormat, ImageTiling::OPTIMAL, ImageUsage::STORAGE | ImageUsage::TRANSFER_SRC);
	mUniformBuffer = mDevice.CreateUniformBuffer<Uniforms::Compute>();

	mDescriptorTable = std::make_unique<DescriptorTable>(device);
	mDescriptorTable->AddLayoutBinding(0, 1, DescriptorType::STORAGE_IMAGE, ShaderStage::COMPUTE)
		.AddLayoutBinding(1, 1, DescriptorType::STORAGE_IMAGE, ShaderStage::COMPUTE)
		.AddLayoutBinding(2, 1, DescriptorType::STORAGE_IMAGE, ShaderStage::COMPUTE)
		.AddLayoutBinding(3, 1, DescriptorType::STORAGE_IMAGE, ShaderStage::COMPUTE)
		.AddLayoutBinding(4, 1, DescriptorType::UNIFORM_BUFFER, ShaderStage::COMPUTE);

	mDescriptorSet = mDescriptorTable->AllocateDescriptorSet();

	mDescriptorSet->WriteImage(0, inputImage.GetView(), ImageLayout::GENERAL) // Input image
		.WriteImage(1, mOutputImage->GetView(), ImageLayout::GENERAL)		  // Output image
		.WriteImage(2, mNormalsImage.GetView(), ImageLayout::GENERAL)		  // Normals image
		.WriteImage(3, mPositionsImage.GetView(), ImageLayout::GENERAL)		  // Positions image
		.WriteBuffer(4, mUniformBuffer.get(), 0, sizeof(Uniforms::Compute))	  // Uniforms descriptor
		.Update();

	mPipelineLayout = std::make_unique<PipelineLayout>(device);
	mPipelineLayout->AddDescriptorSetLayout(mDescriptorTable->GetLayout());

	mPipelines.resize(3);

	mPipelines[0] = std::make_unique<ComputePipeline>(device);
	mPipelines[0]->SetShader(device.CreateShader(ShaderStage::COMPUTE, ReadBinary("Denoiser.comp.spv"))).SetPipelineLayout(mPipelineLayout.get());

	mPipelines[1] = std::make_unique<ComputePipeline>(device);
	mPipelines[1]->SetShader(device.CreateShader(ShaderStage::COMPUTE, ReadBinary("Edgedetect.comp.spv"))).SetPipelineLayout(mPipelineLayout.get());

	mPipelines[2] = std::make_unique<ComputePipeline>(device);
	mPipelines[2]->SetShader(device.CreateShader(ShaderStage::COMPUTE, ReadBinary("Sharpen.comp.spv"))).SetPipelineLayout(mPipelineLayout.get());
}

PostProcessPass::~PostProcessPass()
{
}

void PostProcessPass::Process(int32_t shaderId)
{
	if (shaderId != mCurrentShader)
	{
		mCurrentShader = shaderId;

		mDevice.GetComputeQueue()->WaitIdle();

		const auto extent = swapChain.GetExtent();

		mComputeCommandBuffer->Record([&]()
									  {
			mComputeCommandBuffer->ImageBarrier(mOutputImage->GetHandle(),Access::NONE,Access::SHADER_WRITE,ImageLayout::UNDEFINED,ImageLayout::GENERAL,mOutputImage->GetView()->GetSubresourceRange());
			mComputeCommandBuffer->ImageBarrier(mNormalsImage.GetHandle(),Access::NONE,Access::SHADER_WRITE,ImageLayout::UNDEFINED,ImageLayout::GENERAL,mNormalsImage.GetView()->GetSubresourceRange());
			mComputeCommandBuffer->ImageBarrier(mPositionsImage.GetHandle(),Access::NONE,Access::SHADER_WRITE,ImageLayout::UNDEFINED,ImageLayout::GENERAL,mPositionsImage.GetView()->GetSubresourceRange());
			mComputeCommandBuffer->ImageBarrier(mInputImage.GetHandle(),Access::NONE,Access::SHADER_WRITE,ImageLayout::UNDEFINED,ImageLayout::GENERAL,mInputImage.GetView()->GetSubresourceRange());

			mComputeCommandBuffer->BindPipeline(mPipelines[mCurrentShader].get());
			mComputeCommandBuffer->BindDescriptorSets(mPipelineLayout.get(),0,{mDescriptorSet});

			mComputeCommandBuffer->Dispatch(extent.x/32, extent.y/32, 1); });
	}

	mComputeCommandBuffer->Submit({PipelineStage::COMPUTE_SHADER});
}