#include "RtxRayTracePass.h"
#include <string>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <stb/stb_image_write.h>
#include <fstream>
#include "VK/Image.h"
#include "VK/CommandBuffer.h"
#include "VK/CommandPool.h"
#include "VK/Buffer.h"
#include "VK/SwapChain.h"
#include "VK/ImageView.h"
#include "VK/Shader.h"
#include "VK/SyncObject.h"
#include "VK/Utils.h"
#include "VK/Device.h"
#include "VK/DescriptorPool.h"
#include "VK/DescriptorSet.h"
#include "VK/DescriptorSetLayout.h"
#include "Memory.h"
#include "RaymanScene.h"
#include "ShaderCompiler.h"
#include "Model.h"
#include "InputSystem.h"
#include <array>
#include "RtxRayTraceScene.h"
#include "VK/CommandPool.h"

RtxRayTracePass::RtxRayTracePass(const Instance &instance, Device &device)
	: mDevice(device), mState(RenderState::FINAL), mPrevState(mState)
{
	auto frameCount = App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages().size();
	mRayTracePass = std::make_unique<RayTracePass>(frameCount);

	for (const auto &_ : App::Instance().GetGraphicsContext()->GetSwapChain()->GetImageViews())
		mUniformBuffers.emplace_back(mDevice.CreateUniformBuffer<Uniform>());

	const auto extent = App::Instance().GetGraphicsContext()->GetSwapChain()->GetExtent();

	const auto outputFormat = App::Instance().GetGraphicsContext()->GetSwapChain()->GetFormat();
	const auto accumulationFormat = Format::R32G32B32A32_SFLOAT;
	const auto tiling = ImageTiling::OPTIMAL;

	mAccumulationImage = std::make_unique<GpuImage2D>(mDevice, extent.x, extent.y, accumulationFormat, tiling, ImageUsage::STORAGE | ImageUsage::TRANSFER_SRC | ImageUsage::TRANSFER_DST);
	mOutputImage = std::make_unique<GpuImage2D>(mDevice, extent.x, extent.y, outputFormat, tiling, ImageUsage::STORAGE | ImageUsage::TRANSFER_SRC | ImageUsage::TRANSFER_DST);
	mNormalsImage = std::make_unique<GpuImage2D>(mDevice, extent.x, extent.y, accumulationFormat, tiling, ImageUsage::STORAGE);
	mPositionsImage = std::make_unique<GpuImage2D>(mDevice, extent.x, extent.y, accumulationFormat, tiling, ImageUsage::STORAGE);

	mCompiler.reset(new ShaderCompiler());
}

RtxRayTracePass::~RtxRayTracePass()
{
	mBLASs.clear();
}

Device &RtxRayTracePass::GetDevice() const
{
	return mDevice;
}

void RtxRayTracePass::CreateRayTracerPipeline()
{
	mDescriptorTable = std::make_unique<DescriptorTable>(mDevice);
	mDescriptorTable->AddLayoutBinding(mDescriptorTable->GetBindingCount(), 1, DescriptorType::ACCELERATION_STRUCTURE_KHR, ShaderStage::RAYGEN | ShaderStage::CLOSEST_HIT)	   // Top level acceleration structure.
		.AddLayoutBinding(mDescriptorTable->GetBindingCount(), 1, DescriptorType::STORAGE_IMAGE, ShaderStage::RAYGEN)														   // Image accumulation
		.AddLayoutBinding(mDescriptorTable->GetBindingCount(), 1, DescriptorType::STORAGE_IMAGE, ShaderStage::RAYGEN)														   // Output
		.AddLayoutBinding(mDescriptorTable->GetBindingCount(), 1, DescriptorType::UNIFORM_BUFFER, ShaderStage::RAYGEN | ShaderStage::MISS | ShaderStage::CLOSEST_HIT)		   // Uniforms
		.AddLayoutBinding(mDescriptorTable->GetBindingCount(), 1, DescriptorType::STORAGE_BUFFER, ShaderStage::MISS | ShaderStage::CLOSEST_HIT)								   // Vertex buffer
		.AddLayoutBinding(mDescriptorTable->GetBindingCount(), 1, DescriptorType::STORAGE_BUFFER, ShaderStage::CLOSEST_HIT)													   // Index buffer
		.AddLayoutBinding(mDescriptorTable->GetBindingCount(), 1, DescriptorType::STORAGE_BUFFER, ShaderStage::CLOSEST_HIT)													   // Material buffer
		.AddLayoutBinding(mDescriptorTable->GetBindingCount(), 1, DescriptorType::STORAGE_BUFFER, ShaderStage::CLOSEST_HIT)													   // Offset buffer
		.AddLayoutBinding(mDescriptorTable->GetBindingCount(), mScene->GetTextureSize(), DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStage::CLOSEST_HIT | ShaderStage::MISS) // Textures
		.AddLayoutBinding(mDescriptorTable->GetBindingCount(), 1, DescriptorType::STORAGE_BUFFER, ShaderStage::CLOSEST_HIT | ShaderStage::MISS)								   // Lights buffer
		.AddLayoutBinding(mDescriptorTable->GetBindingCount(), 1, DescriptorType::STORAGE_IMAGE, ShaderStage::RAYGEN)														   // Normal
		.AddLayoutBinding(mDescriptorTable->GetBindingCount(), 1, DescriptorType::STORAGE_IMAGE, ShaderStage::RAYGEN)														   // World position
		;
	if (mScene->UseHDR())
		mDescriptorTable->AddLayoutBinding(mDescriptorTable->GetBindingCount(), mScene->GetHDRTextures().size(), DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStage::CLOSEST_HIT | ShaderStage::MISS);

	mDescriptorSets = mDescriptorTable->AllocateDescriptorSets(App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages().size());

	for (size_t imageIndex = 0; imageIndex < App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages().size(); imageIndex++)
	{
		mDescriptorSets[imageIndex]->WriteAccelerationStructure(0, mTLAS->GetHandle());					 // Top level acceleration structure.
		mDescriptorSets[imageIndex]->WriteImage(1, mAccumulationImage->GetView(), ImageLayout::GENERAL); // Accumulation image
		mDescriptorSets[imageIndex]->WriteImage(2, mOutputImage->GetView(), ImageLayout::GENERAL);		 // Output image
		mDescriptorSets[imageIndex]->WriteBuffer(3, mUniformBuffers[imageIndex].get());					 // Uniform buffer
		mDescriptorSets[imageIndex]->WriteBuffer(4, mScene->GetVertexBuffer());							 // Vertex buffer
		mDescriptorSets[imageIndex]->WriteBuffer(5, mScene->GetIndexBuffer());							 // Index buffer
		mDescriptorSets[imageIndex]->WriteBuffer(6, mScene->GetMaterialBuffer());						 // Material buffer
		mDescriptorSets[imageIndex]->WriteBuffer(7, mScene->GetOffsetBuffer());							 // Offsets buffer

		// Texture descriptor
		std::vector<DescriptorImageInfo> imageInfos(mScene->GetTextures().size());
		for (size_t t = 0; t < imageInfos.size(); ++t)
		{
			imageInfos[t].imageLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
			imageInfos[t].imageView = mScene->GetTextures()[t]->GetImageView();
			imageInfos[t].sampler = mScene->GetTextures()[t]->GetSampler()->GetHandle();
		}
		mDescriptorSets[imageIndex]->WriteImageArray(8, imageInfos);								   // Texture descriptor
		mDescriptorSets[imageIndex]->WriteBuffer(9, mScene->GetLightsBuffer());						   // Lights buffer
		mDescriptorSets[imageIndex]->WriteImage(10, mNormalsImage->GetView(), ImageLayout::GENERAL);   // Normal image
		mDescriptorSets[imageIndex]->WriteImage(11, mPositionsImage->GetView(), ImageLayout::GENERAL); // Position image

		// HDR descriptor
		// Outside the block because of RAII
		std::vector<DescriptorImageInfo> hdrInfos(mScene->GetHDRTextures().size());
		if (mScene->UseHDR())
		{
			for (size_t t = 0; t < hdrInfos.size(); ++t)
			{
				hdrInfos[t].imageLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
				hdrInfos[t].imageView = mScene->GetHDRTextures()[t]->GetImageView();
				hdrInfos[t].sampler = mScene->GetHDRTextures()[t]->GetSampler()->GetHandle();
			}

			mDescriptorSets[imageIndex]->WriteImageArray(12, hdrInfos); // Position image
		}

		mDescriptorSets[imageIndex]->Update();
	}

	mPipelineLayout = std::make_unique<PipelineLayout>(mDevice);
	mPipelineLayout->AddDescriptorSetLayout(mDescriptorTable->GetLayout());

	mPipeline = std::make_unique<RayTracePipeline>(mDevice);
	mPipeline->SetRayGenShader(mDevice.CreateShader(ShaderStage::RAYGEN, ReadBinary("Raytracing.compiled.rgen.spv")))
		.AddRayClosestHitShader(mDevice.CreateShader(ShaderStage::CLOSEST_HIT, ReadBinary("Raytracing.compiled.rchit.spv")))
		.AddRayMissShader(mDevice.CreateShader(ShaderStage::MISS, ReadBinary("Raytracing.compiled.rmiss.spv")))
		.AddRayMissShader(mDevice.CreateShader(ShaderStage::MISS, ReadBinary("Shadow.compiled.rmiss.spv")))
		.SetPipelineLayout(mPipelineLayout.get());
}

void RtxRayTracePass::CreateComputePipeline()
{
	mPostProcessPass = std::make_unique<PostProcessPass>(*App::Instance().GetGraphicsContext()->GetSwapChain(), mDevice, *mOutputImage, *mNormalsImage, *mPositionsImage);
}

void RtxRayTracePass::Copy(RayTraceCommandBuffer *commandBuffer, Image2D *src, VkImage dst) const
{
	auto extent = App::Instance().GetGraphicsContext()->GetSwapChain()->GetExtent();
	VkImageSubresourceRange subresourceRange = src->GetView()->GetSubresourceRange();
	VkImageCopy copyRegion = src->GetImageCopy(extent.x, extent.y);

	commandBuffer->ImageBarrier(src->GetHandle(), Access::SHADER_WRITE, Access::TRANSFER_READ, ImageLayout::GENERAL, ImageLayout::TRANSFER_SRC_OPTIMAL, subresourceRange);
	commandBuffer->ImageBarrier(dst, Access::NONE, Access::TRANSFER_WRITE, ImageLayout::UNDEFINED, ImageLayout::TRANSFER_DST_OPTIMAL, subresourceRange);
	commandBuffer->CopyImage(src->GetHandle(), ImageLayout::TRANSFER_SRC_OPTIMAL, dst, ImageLayout::TRANSFER_DST_OPTIMAL, copyRegion);
	commandBuffer->ImageBarrier(src->GetHandle(), Access::TRANSFER_READ, Access::SHADER_WRITE, ImageLayout::TRANSFER_SRC_OPTIMAL, ImageLayout::GENERAL, subresourceRange);
	commandBuffer->ImageBarrier(dst, Access::TRANSFER_WRITE, Access::NONE, ImageLayout::TRANSFER_DST_OPTIMAL, ImageLayout::PRESENT_SRC_KHR, subresourceRange);
}

const Image2D *RtxRayTracePass::GetOutputImage() const
{
	return mOutputImage.get();
}

void RtxRayTracePass::SetScene(RtxRayTraceScene *scene)
{
	mScene = scene;
	mScene->Build();

	CreateBLAS();
	CreateTLAS();

	BuildPipeline();
}

void RtxRayTracePass::SetPostProcessType(PostProcessType t)
{
	mPostProcessType = t;
}

void RtxRayTracePass::SetRenderState(RenderState state)
{
	mState = state;
}

void RtxRayTracePass::BuildPipeline()
{
	CompileShaders();
	CreateRayTracerPipeline();
	CreateComputePipeline();
	ResetAccumulation();
}

void RtxRayTracePass::Render()
{
	if (mScene->Get()->GetCamera().HaveUpdate())
		ResetAccumulation();
	mRayTracePass->RecordCurrentCommand([&](RayTraceCommandBuffer *rayTraceCmd, size_t frameIdx)
										{
											const auto extent = App::Instance().GetGraphicsContext()->GetSwapChain()->GetExtent();
									
											rayTraceCmd->ImageBarrier( mAccumulationImage->GetHandle(),Access::NONE,Access::SHADER_WRITE,ImageLayout::UNDEFINED,ImageLayout::GENERAL,mAccumulationImage->GetView()->GetSubresourceRange());
											rayTraceCmd->ImageBarrier( mOutputImage->GetHandle(),Access::NONE,Access::SHADER_WRITE,ImageLayout::UNDEFINED,ImageLayout::GENERAL,mOutputImage->GetView()->GetSubresourceRange());
											rayTraceCmd->ImageBarrier( mNormalsImage->GetHandle(),Access::NONE,Access::SHADER_WRITE,ImageLayout::UNDEFINED,ImageLayout::GENERAL,mNormalsImage->GetView()->GetSubresourceRange());
											rayTraceCmd->ImageBarrier( mPositionsImage->GetHandle(),Access::NONE,Access::SHADER_WRITE,ImageLayout::UNDEFINED,ImageLayout::GENERAL,mPositionsImage->GetView()->GetSubresourceRange());
											rayTraceCmd->BindPipeline(mPipeline.get());
											rayTraceCmd->BindDescriptorSets(mPipelineLayout.get(),0,{mDescriptorSets[frameIdx]});
											rayTraceCmd->TraceRaysKHR(mPipeline->GetSBT(),extent.x,extent.y,1);
											++mFrame;
											if (mPostProcessType == PostProcessType::NONE)
												Copy(rayTraceCmd, mOutputImage.get(), App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages()[frameIdx]);
											else
											{
												mPostProcessPass->Process((int32_t)mPostProcessType);
												Copy(rayTraceCmd, mPostProcessPass->GetOutputImage(), App::Instance().GetGraphicsContext()->GetSwapChain()->GetImages()[frameIdx]);
											}

											UpdateUniformBuffer(frameIdx); 
										});

	mRayTracePass->Render();
}

void RtxRayTracePass::Update()
{
	if (mPrevState != mState)
	{
		mPrevState = mState;
		mDevice.WaitIdle();
		BuildPipeline();
	}
}

void RtxRayTracePass::CompileShaders() const
{
	std::vector<ShaderDefine> defines;

	if (mScene->UseHDR())
		defines.emplace_back(ShaderDefine::USE_HDR);

	if (mState == RenderState::AO)
		defines.emplace_back(ShaderDefine::DEBUG_AO_OUTPUT);

	if (mState == RenderState::ALBEDO)
		defines.emplace_back(ShaderDefine::DEBUG_ALBEDO_OUTPUT);

	if (mState == RenderState::NORMAL)
		defines.emplace_back(ShaderDefine::DEBUG_NORMAL_OUTPUT);

	mCompiler->Compile(defines);
}

void RtxRayTracePass::ResetAccumulation()
{
	mFrame = 0;
}

void RtxRayTracePass::CreateBLAS()
{
	for (const auto &model : mScene->Get()->GetMeshInstances())
	{
		const auto &mesh = mScene->Get()->GetMeshes()[model.meshId];
		mBLASs.emplace_back(std::make_unique<BLAS>(mDevice, mesh->vertices, mesh->indices));
	}
}

void RtxRayTracePass::CreateTLAS()
{
	std::vector<VkAccelerationStructureInstanceKHR> geometryInstances;

	geometryInstances.reserve(mScene->Get()->GetMeshes().size());

	for (auto instanceId = 0; instanceId < mScene->Get()->GetMeshInstances().size(); ++instanceId)
		geometryInstances.emplace_back(mBLASs[instanceId]->CreateInstance());

	mTLAS = std::make_unique<TLAS>(mDevice, geometryInstances);
}

void RtxRayTracePass::UpdateUniformBuffer(size_t frameIdx)
{
	Uniform uniform{};

	uniform.view = mScene->Get()->GetCamera().GetView();
	uniform.projection = mScene->Get()->GetCamera().GetProjection();
	uniform.cameraPos = mScene->Get()->GetCamera().GetPosition();
	uniform.lights = mScene->Get()->GetLightsSize();
	uniform.hdrResolution = mScene->UseHDR() ? mScene->Get()->GetHDRResolution() : 0.f;
	uniform.hdrMultiplier = mScene->UseHDR() ? mScene->Get()->hdrMultiplier : 1.0f;
	uniform.frame = mFrame;

	mUniformBuffers[frameIdx]->Set(uniform);
}

void RtxRayTracePass::SaveOutputImageToDisk()
{
	auto now = std::chrono::system_clock::now();
	uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
							   std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
	time_t tt = std::chrono::system_clock::to_time_t(now);
	auto time_tm = localtime(&tt);
	char strTime[25] = {0};
	sprintf(strTime, "%d%02d%02d%02d%2d%02d%03d", time_tm->tm_year + 1900,
			time_tm->tm_mon + 1,
			time_tm->tm_mday,
			time_tm->tm_hour,
			time_tm->tm_min,
			time_tm->tm_sec,
			(int)dis_millseconds);
	std::string fileName = "rayman_ScreenShot" + std::string(strTime) + ".png";

	auto dstImage = std::make_unique<CpuImage2D>(mDevice, mOutputImage->GetWidth(), mOutputImage->GetHeight(), Format::R8G8B8A8_UNORM, ImageTiling::LINEAR, ImageUsage::TRANSFER_DST);
	auto cmd = mDevice.GetTransferCommandPool()->CreatePrimaryCommandBuffer();
	cmd->ExecuteImmediately([&]()
							{
								cmd->ImageBarrier(dstImage->GetHandle(),Access::NONE,Access::TRANSFER_WRITE,ImageLayout::UNDEFINED,ImageLayout::TRANSFER_DST_OPTIMAL,dstImage->GetView()->GetSubresourceRange());
							 	VkImageCopy imageCopy = mOutputImage->GetImageCopy(mOutputImage->GetWidth(), mOutputImage->GetHeight());
								cmd->CopyImage(mOutputImage->GetHandle(),ImageLayout::TRANSFER_SRC_OPTIMAL,dstImage->GetHandle(),ImageLayout::TRANSFER_DST_OPTIMAL,imageCopy);
								cmd->ImageBarrier(dstImage->GetHandle(),Access::TRANSFER_WRITE,Access::MEMORY_READ,ImageLayout::TRANSFER_DST_OPTIMAL,ImageLayout::GENERAL, dstImage->GetView()->GetSubresourceRange()); });

	VkSubresourceLayout subResourceLayout = dstImage->GetSubResourceLayout();

	uint8_t *data;
	vkMapMemory(mDevice.GetHandle(), dstImage->GetMemory(), 0, VK_WHOLE_SIZE, 0, (void **)&data);
	data += subResourceLayout.offset;

	uint32_t width = mOutputImage->GetWidth();
	uint32_t height = mOutputImage->GetHeight();

	// B8G8R8A8->R8G8B8A8
	std::vector<uint8_t> rgba8Pixels;
	for (size_t i = 0; i < width * height * 4; i += 4)
	{
		rgba8Pixels.emplace_back(data[i + 2]);
		rgba8Pixels.emplace_back(data[i + 1]);
		rgba8Pixels.emplace_back(data[i + 0]);
		rgba8Pixels.emplace_back(data[i + 3]);
	}

	stbi_write_png(fileName.c_str(), width, height, 4, rgba8Pixels.data(), width * 4);

	std::cout << fileName << " complete!" << std::endl;
}