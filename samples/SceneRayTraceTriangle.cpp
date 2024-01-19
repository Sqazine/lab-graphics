#include "SceneRayTraceTriangle.h"
#include "App.h"
void SceneRayTraceTriangle::Init()
{
	// create bottom-level container
	mBlas = std::make_unique<BLAS>(*App::Instance().GetGraphicsContext()->GetDevice(), mTriangle.vertices, mTriangle.indices);

	auto inst = mBlas->CreateInstance();

	std::vector<VkAccelerationStructureInstanceKHR> insts;
	insts.emplace_back(inst);

	// create top-level container
	mTlas = std::make_unique<TLAS>(*App::Instance().GetGraphicsContext()->GetDevice(), insts);

	std::cout << "Creating Offscreen Buffer.." << std::endl;

	auto windowExtent = App::Instance().GetWindow()->GetExtent();

	mOffscreenImage2D = App::Instance().GetGraphicsContext()->GetDevice()->CreateCpuImage2D(windowExtent.x, windowExtent.y, App::Instance().GetGraphicsContext()->GetSwapChain()->GetFormat(), ImageTiling::LINEAR);

	std::cout << "Creating ray tracing descriptor set layout.." << std::endl;
	mDescriptorTable = std::make_unique<DescriptorTable>(*App::Instance().GetGraphicsContext()->GetDevice());
	mDescriptorTable->AddLayoutBinding(0, 1, DescriptorType::ACCELERATION_STRUCTURE_KHR, ShaderStage::RAYGEN)
		.AddLayoutBinding(1, 1, DescriptorType::STORAGE_IMAGE, ShaderStage::RAYGEN);

	std::cout << "Creating ray tracing descriptor set.." << std::endl;
	mDescriptorSet = mDescriptorTable->AllocateDescriptorSet();
	mDescriptorSet->WriteAccelerationStructure(0, mTlas->GetHandle())
		.WriteImage(1, mOffscreenImage2D->GetView(), ImageLayout::GENERAL)
		.Update();

	mPipelineLayout = std::make_unique<PipelineLayout>(*App::Instance().GetGraphicsContext()->GetDevice());
	mPipelineLayout->AddDescriptorSetLayout(mDescriptorTable->GetLayout());

	std::cout << "Creating ray tracing pipeline.." << std::endl;

	mRayTracePipeline = std::make_unique<RayTracePipeline>(*App::Instance().GetGraphicsContext()->GetDevice());
	mRayTracePipeline->SetRayGenShader(App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::RAYGEN, rayGenShaderSrc))
		.AddRayClosestHitShader(App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::CLOSEST_HIT, rayChitShaderSrc))
		.AddRayMissShader(App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::MISS, rayMissShaderSrc))
		.AddRayCallableShader(App::Instance().GetGraphicsContext()->GetDevice()->CreateShader(ShaderStage::CALLABLE, rayCallableShaderSrc))
		.SetPipelineLayout(mPipelineLayout.get());

	mSemaphoreImageAvailable = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphore();
	mSemaphoreRenderAvailable = App::Instance().GetGraphicsContext()->GetDevice()->CreateSemaphore();

	std::cout << "Recording frame commands.." << std::endl;

	mCommandBuffers = App::Instance().GetGraphicsContext()->GetDevice()->GetRayTraceCommandPool()->CreatePrimaryCommandBuffers(App::Instance().GetGraphicsContext()->GetSwapChain()->GetImageViews().size());
}
void SceneRayTraceTriangle::ProcessInput()
{
}
void SceneRayTraceTriangle::Update()
{
}
void SceneRayTraceTriangle::Render()
{
	auto imageIndex = App::Instance().GetGraphicsContext()->GetSwapChain()->AcquireNextImage(mSemaphoreImageAvailable.get());

	VkImage swapChainImage = App::Instance().GetGraphicsContext()->GetSwapChain()->GetImage(imageIndex);
	auto commandBuffer = mCommandBuffers[imageIndex].get();

	commandBuffer->Reset();

	VkImageSubresourceRange subResourceRange{};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1;

	auto windowExtent = App::Instance().GetWindow()->GetExtent();

	VkImageCopy copyRegion{};
	copyRegion.srcOffset = {0, 0, 0};
	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.mipLevel = 0;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.srcSubresource.baseArrayLayer = 0;
	copyRegion.dstOffset = {0, 0, 0};
	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.mipLevel = 0;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.dstSubresource.baseArrayLayer = 0;
	copyRegion.extent.depth = 1;
	copyRegion.extent.width = windowExtent.x;
	copyRegion.extent.height = windowExtent.y;

	commandBuffer->Record([&]()
						  {
		commandBuffer->ImageBarrier(mOffscreenImage2D->GetHandle(),
			Access::NONE,
			Access::SHADER_WRITE,
			ImageLayout::UNDEFINED,
			ImageLayout::GENERAL,
			subResourceRange);

		commandBuffer->BindPipeline(mRayTracePipeline.get());

		commandBuffer->BindDescriptorSets(mPipelineLayout.get(), 0, { mDescriptorSet});

		commandBuffer->TraceRaysKHR(mRayTracePipeline->GetSBT(),  windowExtent.x,  windowExtent.y, 1);

		commandBuffer->ImageBarrier(swapChainImage,
			Access::NONE,
			Access::TRANSFER_WRITE,
			ImageLayout::UNDEFINED,
			ImageLayout::TRANSFER_DST_OPTIMAL,
			subResourceRange);

		commandBuffer->ImageBarrier(mOffscreenImage2D->GetHandle(),
			Access::SHADER_WRITE,
			Access::TRANSFER_READ,
			ImageLayout::GENERAL,
			ImageLayout::TRANSFER_SRC_OPTIMAL,
			subResourceRange);

		commandBuffer->CopyImage(mOffscreenImage2D->GetHandle(),
			ImageLayout::TRANSFER_SRC_OPTIMAL,
			swapChainImage,
			ImageLayout::TRANSFER_DST_OPTIMAL,
			copyRegion);

		commandBuffer->ImageBarrier(swapChainImage,
			Access::NONE,
			Access::TRANSFER_WRITE,
			ImageLayout::TRANSFER_DST_OPTIMAL,
			ImageLayout::PRESENT_SRC_KHR,
			subResourceRange); });


	commandBuffer->Submit({PipelineStage::COLOR_ATTACHMENT_OUTPUT},{mSemaphoreImageAvailable.get()},{mSemaphoreRenderAvailable.get()});

	commandBuffer->Present(imageIndex,{mSemaphoreRenderAvailable.get()});
}

void SceneRayTraceTriangle::CleanUp()
{
}