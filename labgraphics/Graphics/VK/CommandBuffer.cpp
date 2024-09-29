#include "CommandBuffer.h"
#include "Device.h"
#include "Utils.h"
#include <iostream>
#include "CommandPool.h"
#include "DescriptorSet.h"
#include "Image.h"
#include "App.h"

CommandBuffer::CommandBuffer(Device &device, VkCommandPool cmdPool, VkCommandBufferLevel level)
	: mDevice(device), mRelatedCmdPoolHandle(cmdPool)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = mRelatedCmdPoolHandle;
	commandBufferAllocateInfo.level = level;
	commandBufferAllocateInfo.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(mDevice.GetHandle(), &commandBufferAllocateInfo, &mHandle));
}
CommandBuffer::~CommandBuffer()
{
	vkFreeCommandBuffers(mDevice.GetHandle(), mRelatedCmdPoolHandle, 1, &mHandle);
}

const VkCommandBuffer &CommandBuffer::GetHandle() const
{
	return mHandle;
}

void CommandBuffer::Begin()
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	VK_CHECK(vkBeginCommandBuffer(mHandle, &commandBufferBeginInfo));
}
void CommandBuffer::BeginOnce()
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(mHandle, &commandBufferBeginInfo));
}

void CommandBuffer::End()
{
	VK_CHECK(vkEndCommandBuffer(mHandle));
}

void CommandBuffer::ExecuteImmediately(const std::function<void()> &func)
{
	BeginOnce();

	func();

	End();
	Submit();
}

void CommandBuffer::Record(const std::function<void()> &func)
{
	Reset();
	Begin();

	func();

	End();
}

void CommandBuffer::ImageBarrier(const VkImage &image, Access srcAccess,
								 Access dstAccess,
								 ImageLayout oldLayout,
								 ImageLayout newLayout,
								 const VkImageSubresourceRange &subresourceRange) const
{

	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = ACCESS_CAST(srcAccess);
	imageMemoryBarrier.dstAccessMask = ACCESS_CAST(dstAccess);
	imageMemoryBarrier.oldLayout = IMAGE_LAYOUT_CAST(oldLayout);
	imageMemoryBarrier.newLayout = IMAGE_LAYOUT_CAST(newLayout);
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	this->PipelineBarrier(
		PipelineStage::ALL_COMMANDS,
		PipelineStage::ALL_COMMANDS,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&imageMemoryBarrier);
}

void CommandBuffer::ImageBarrier(const VkImage &image, Format format, ImageLayout oldLayout, ImageLayout newLayout) const
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = IMAGE_LAYOUT_CAST(oldLayout);
	barrier.newLayout = IMAGE_LAYOUT_CAST(newLayout);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	PipelineStage sourceStage;
	PipelineStage destinationStage;

	if (newLayout == ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = IMAGE_ASPECT_CAST(ImageAspect::DEPTH);

		if (format.HasStencil())
			barrier.subresourceRange.aspectMask |= IMAGE_ASPECT_CAST(ImageAspect::STENCIL);
	}
	else
		barrier.subresourceRange.aspectMask = IMAGE_ASPECT_CAST(ImageAspect::COLOR);

	if (oldLayout == ImageLayout::UNDEFINED && newLayout == ImageLayout::TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = ACCESS_CAST(Access::NONE);
		barrier.dstAccessMask = ACCESS_CAST(Access::TRANSFER_WRITE);
		sourceStage = PipelineStage::TOP_OF_PIPE;
		destinationStage = PipelineStage::TRANSFER;
	}
	else if (oldLayout == ImageLayout::TRANSFER_DST_OPTIMAL && newLayout == ImageLayout::SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = ACCESS_CAST(Access::TRANSFER_WRITE);
		barrier.dstAccessMask = ACCESS_CAST(Access::SHADER_READ);
		sourceStage = PipelineStage::TRANSFER;
		destinationStage = PipelineStage::FRAGMENT_SHADER;
	}
	else if (oldLayout == ImageLayout::UNDEFINED && newLayout == ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = ACCESS_CAST(Access::NONE);
		barrier.dstAccessMask = ACCESS_CAST(Access::DEPTH_STENCIL_ATTACHMENT_READ | Access::DEPTH_STENCIL_ATTACHMENT_WRITE);
		sourceStage = PipelineStage::TOP_OF_PIPE;
		destinationStage = PipelineStage::EARLY_FRAGMENT_TESTS;
	}
	else
	{
		std::cout << "[ERROR] Unsupported layout transition!" << std::endl;
		exit(1);
	}

	PipelineBarrier(
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);
}

void CommandBuffer::PipelineBarrier(PipelineStage srcStage, PipelineStage dstStage, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) const
{
	vkCmdPipelineBarrier(mHandle, PIPELINE_STAGE_CAST(srcStage), PIPELINE_STAGE_CAST(dstStage), dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void CommandBuffer::CopyImage(VkImage srcImage, ImageLayout srcImageLayout, VkImage dstImage, ImageLayout dstImageLayout, const std::vector<VkImageCopy> &copyRegions)
{
	vkCmdCopyImage(mHandle, srcImage, IMAGE_LAYOUT_CAST(srcImageLayout), dstImage, IMAGE_LAYOUT_CAST(dstImageLayout), copyRegions.size(), copyRegions.data());
}

void CommandBuffer::CopyImage(VkImage srcImage, ImageLayout srcImageLayout, VkImage dstImage, ImageLayout dstImageLayout, const VkImageCopy &copyRegion)
{
	vkCmdCopyImage(mHandle, srcImage, IMAGE_LAYOUT_CAST(srcImageLayout), dstImage, IMAGE_LAYOUT_CAST(dstImageLayout), 1, &copyRegion);
}

void CommandBuffer::CopyBuffer(const Buffer &dst, const Buffer &src, VkBufferCopy bufferCopy)
{
	vkCmdCopyBuffer(mHandle, src.GetHandle(), dst.GetHandle(), 1, &bufferCopy);
}

void CommandBuffer::CopyImageFromBuffer(Image2D *dst, Buffer *src)
{
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {dst->GetWidth(), dst->GetHeight(), 1};

	vkCmdCopyBufferToImage(mHandle, src->GetHandle(), dst->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CommandBuffer::Reset()
{
	VK_CHECK(vkResetCommandBuffer(mHandle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

void CommandBuffer::TransitionImageNewLayout(Image2D *image, ImageLayout newLayout)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.oldLayout = IMAGE_LAYOUT_CAST(image->GetImageLayout());
	barrier.newLayout = IMAGE_LAYOUT_CAST(newLayout);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image->GetHandle();
	barrier.subresourceRange.aspectMask = IMAGE_ASPECT_CAST(ImageAspect::COLOR);
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = image->GetMipLevel();
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	auto oldLayout = barrier.oldLayout;
}

RasterCommandBuffer::RasterCommandBuffer(class Device &device, VkCommandBufferLevel level)
	: CommandBuffer(device, device.GetRasterCommandPool()->GetHandle(), level)
{
}
RasterCommandBuffer::~RasterCommandBuffer()
{
}

void RasterCommandBuffer::BindPipeline(Pipeline *pipeline) const
{
	vkCmdBindPipeline(mHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
}

void RasterCommandBuffer::BindDescriptorSets(PipelineLayout *layout, uint32_t firstSet, const std::vector<const DescriptorSet *> &descriptorSets, const std::vector<uint32_t> dynamicOffsets)
{
	std::vector<VkDescriptorSet> rawDescSets(descriptorSets.size());
	for (int32_t i = 0; i < rawDescSets.size(); ++i)
		rawDescSets[i] = descriptorSets[i]->GetHandle();

	vkCmdBindDescriptorSets(mHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, layout->GetHandle(), 0, rawDescSets.size(), rawDescSets.data(), dynamicOffsets.size(), dynamicOffsets.data());
}

void RasterCommandBuffer::BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const std::vector<Buffer *> &pBuffers, const std::vector<uint64_t> &pOffsets)
{
	for (uint32_t i = 0; i < bindingCount; ++i)
		vkCmdBindVertexBuffers(mHandle, firstBinding + i, 1, &pBuffers[i]->GetHandle(), &pOffsets[i]);
}

void RasterCommandBuffer::BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const std::vector<Buffer *> &pBuffers)
{
	std::vector<uint64_t> offsets(pBuffers.size(), 0);
	BindVertexBuffers(firstBinding, bindingCount, pBuffers, offsets);
}

void RasterCommandBuffer::BeginRenderPass(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkRect2D renderArea, const std::vector<VkClearValue> &clearValues, VkSubpassContents subpassContents)
{
	VkRenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = frameBuffer;
	renderPassBeginInfo.renderArea = renderArea;
	renderPassBeginInfo.clearValueCount = clearValues.size();
	renderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(mHandle, &renderPassBeginInfo, subpassContents);
}

void RasterCommandBuffer::EndRenderPass()
{
	vkCmdEndRenderPass(mHandle);
}

void RasterCommandBuffer::SetViewport(const VkViewport &viewport)
{
	vkCmdSetViewport(mHandle, 0, 1, &viewport);
}

void RasterCommandBuffer::SetScissor(const VkRect2D &scissor)
{
	vkCmdSetScissor(mHandle, 0, 1, &scissor);
}

void RasterCommandBuffer::SetLineWidth(float lineWidth)
{
	vkCmdSetLineWidth(mHandle, lineWidth);
}

void RasterCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	vkCmdDraw(mHandle, vertexCount, instanceCount, firstVertex, firstInstance);
}

void RasterCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
	vkCmdDrawIndexed(mHandle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void RasterCommandBuffer::Submit(const std::vector<PipelineStage> &waitStages, const std::vector<Semaphore *> waitSemaphores, const std::vector<Semaphore *> signalSemaphores, Fence *fence) const
{
	std::vector<VkSemaphore> rawSignal(signalSemaphores.size());
	std::vector<VkSemaphore> rawWait(waitSemaphores.size());
	std::vector<VkPipelineStageFlags> rawWaitStages(waitStages.size());

	for (size_t i = 0; i < rawSignal.size(); ++i)
		rawSignal[i] = signalSemaphores[i]->GetHandle();

	for (size_t i = 0; i < rawWait.size(); ++i)
		rawWait[i] = waitSemaphores[i]->GetHandle();

	for (size_t i = 0; i < rawWaitStages.size(); ++i)
		rawWaitStages[i] = PIPELINE_STAGE_CAST(waitStages[i]);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mHandle;
	submitInfo.waitSemaphoreCount = rawWait.size();
	submitInfo.pWaitSemaphores = rawWait.data();
	submitInfo.pWaitDstStageMask = rawWaitStages.data();
	submitInfo.signalSemaphoreCount = rawSignal.size();
	submitInfo.pSignalSemaphores = rawSignal.data();

	if (fence == nullptr)
	{
		auto tmpfence = mDevice.CreateFence();
		mDevice.GetGraphicsQueue()->Submit(submitInfo, tmpfence.get());
		tmpfence->Wait();
		tmpfence.reset(nullptr);
	}
	else
	{
		mDevice.GetGraphicsQueue()->Submit(submitInfo, fence);
		if (fence->GetStatus() == FenceStatus::UNSIGNALED)
			fence->Wait();
	}
}

ComputeCommandBuffer::ComputeCommandBuffer(class Device &device, VkCommandBufferLevel level)
	: CommandBuffer(device, device.GetComputeCommandPool()->GetHandle(), level)
{
}
ComputeCommandBuffer::~ComputeCommandBuffer()
{
}

void ComputeCommandBuffer::BindPipeline(Pipeline *pipeline) const
{
	vkCmdBindPipeline(mHandle, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
}

void ComputeCommandBuffer::BindDescriptorSets(PipelineLayout *layout, uint32_t firstSet, const std::vector<const DescriptorSet *> &descriptorSets, const std::vector<uint32_t> dynamicOffsets)
{
	std::vector<VkDescriptorSet> rawDescSets(descriptorSets.size());
	for (int32_t i = 0; i < rawDescSets.size(); ++i)
		rawDescSets[i] = descriptorSets[i]->GetHandle();
	vkCmdBindDescriptorSets(mHandle, VK_PIPELINE_BIND_POINT_COMPUTE, layout->GetHandle(), 0, rawDescSets.size(), rawDescSets.data(), dynamicOffsets.size(), dynamicOffsets.data());
}

void ComputeCommandBuffer::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
	vkCmdDispatch(mHandle, groupCountX, groupCountY, groupCountZ);
}

void ComputeCommandBuffer::Submit(const std::vector<PipelineStage> &waitStages, const std::vector<Semaphore *> waitSemaphores, const std::vector<Semaphore *> signalSemaphores, Fence *fence) const
{
	std::vector<VkSemaphore> rawSignal(signalSemaphores.size());
	std::vector<VkSemaphore> rawWait(waitSemaphores.size());

	for (size_t i = 0; i < rawSignal.size(); ++i)
		rawSignal[i] = signalSemaphores[i]->GetHandle();

	for (size_t i = 0; i < rawWait.size(); ++i)
		rawWait[i] = waitSemaphores[i]->GetHandle();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mHandle;
	submitInfo.waitSemaphoreCount = rawWait.size();
	submitInfo.pWaitSemaphores = rawWait.data();
	submitInfo.signalSemaphoreCount = rawSignal.size();
	submitInfo.pSignalSemaphores = rawSignal.data();

	if (fence == nullptr)
	{
		auto tmpfence = mDevice.CreateFence();
		mDevice.GetComputeQueue()->Submit(submitInfo, tmpfence.get());
		tmpfence->Wait();
		tmpfence.reset(nullptr);
	}
	else
	{
		mDevice.GetComputeQueue()->Submit(submitInfo, fence);
		if (fence->GetStatus() == FenceStatus::UNSIGNALED)
			fence->Wait();
	}
}

RayTraceCommandBuffer::RayTraceCommandBuffer(Device &device, VkCommandBufferLevel level)
	: CommandBuffer(device, device.GetRayTraceCommandPool()->GetHandle(), level)

{
}

RayTraceCommandBuffer::~RayTraceCommandBuffer()
{
}

void RayTraceCommandBuffer::BindPipeline(Pipeline *pipeline) const
{
	vkCmdBindPipeline(mHandle, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline->GetHandle());
}

void RayTraceCommandBuffer::BindDescriptorSets(PipelineLayout *layout, uint32_t firstSet, const std::vector<const DescriptorSet *> &descriptorSets, const std::vector<uint32_t> dynamicOffsets)
{
	std::vector<VkDescriptorSet> rawDescSets(descriptorSets.size());
	for (int32_t i = 0; i < rawDescSets.size(); ++i)
		rawDescSets[i] = descriptorSets[i]->GetHandle();
	vkCmdBindDescriptorSets(mHandle, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, layout->GetHandle(), 0, rawDescSets.size(), rawDescSets.data(), dynamicOffsets.size(), dynamicOffsets.data());
}

void RayTraceCommandBuffer::TraceRaysKHR(const RayTraceSBT &sbt, uint32_t width, uint32_t height, uint32_t depth)
{
	mDevice.vkCmdTraceRaysKHR(mHandle, &sbt.GetRayGenAddressRegion(), &sbt.GetRayMissAddressRegion(), &sbt.GetRayClosestHitAddressRegion(), &sbt.GetRayCallableAddressRegion(), width, height, depth);
}

void RayTraceCommandBuffer::BuildAccelerationStructureKHR(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos)
{
	mDevice.vkCmdBuildAccelerationStructuresKHR(mHandle, infoCount, pInfos, ppBuildRangeInfos);
}

void RayTraceCommandBuffer::Submit(const std::vector<PipelineStage> &waitStages, const std::vector<Semaphore *> waitSemaphores, const std::vector<Semaphore *> signalSemaphores, Fence *fence) const
{
	std::vector<VkSemaphore> rawSignal(signalSemaphores.size());
	std::vector<VkSemaphore> rawWait(waitSemaphores.size());
	std::vector<VkPipelineStageFlags> rawWaitStages(waitStages.size());

	for (size_t i = 0; i < rawSignal.size(); ++i)
		rawSignal[i] = signalSemaphores[i]->GetHandle();

	for (size_t i = 0; i < rawWait.size(); ++i)
		rawWait[i] = waitSemaphores[i]->GetHandle();

	for (size_t i = 0; i < rawWaitStages.size(); ++i)
		rawWaitStages[i] = PIPELINE_STAGE_CAST(waitStages[i]);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mHandle;
	submitInfo.waitSemaphoreCount = rawWait.size();
	submitInfo.pWaitSemaphores = rawWait.data();
	submitInfo.signalSemaphoreCount = rawSignal.size();
	submitInfo.pSignalSemaphores = rawSignal.data();
	submitInfo.pWaitDstStageMask = rawWaitStages.data();

	if (fence == nullptr)
	{
		auto tmpfence = mDevice.CreateFence();
		mDevice.GetGraphicsQueue()->Submit(submitInfo, tmpfence.get());
		tmpfence->Wait();
		tmpfence.reset(nullptr);
	}
	else
	{
		mDevice.GetGraphicsQueue()->Submit(submitInfo, fence);
		if (fence->GetStatus() == FenceStatus::UNSIGNALED)
			fence->Wait();
	}
}

TransferCommandBuffer::TransferCommandBuffer(Device &device, VkCommandBufferLevel level)
	: CommandBuffer(device, device.GetTransferCommandPool()->GetHandle(), level)
{
}

TransferCommandBuffer::~TransferCommandBuffer()
{
}

void TransferCommandBuffer::BindPipeline(Pipeline *pipeline) const
{
	vkCmdBindPipeline(mHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
}
void TransferCommandBuffer::BindDescriptorSets(PipelineLayout *layout, uint32_t firstSet, const std::vector<const DescriptorSet *> &descriptorSets, const std::vector<uint32_t> dynamicOffsets)
{
	std::vector<VkDescriptorSet> rawDescSets(descriptorSets.size());
	for (int32_t i = 0; i < rawDescSets.size(); ++i)
		rawDescSets[i] = descriptorSets[i]->GetHandle();
	vkCmdBindDescriptorSets(mHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, layout->GetHandle(), 0, rawDescSets.size(), rawDescSets.data(), dynamicOffsets.size(), dynamicOffsets.data());
}

void TransferCommandBuffer::Submit(const std::vector<PipelineStage> &waitStages, const std::vector<Semaphore *> waitSemaphores, const std::vector<Semaphore *> signalSemaphores, Fence *fence) const
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mHandle;

	if (fence == nullptr)
	{
		auto tmpfence = mDevice.CreateFence();
		mDevice.GetComputeQueue()->Submit(submitInfo, tmpfence.get());
		tmpfence->Wait();
		tmpfence.reset(nullptr);
	}
	else
	{
		mDevice.GetComputeQueue()->Submit(submitInfo, fence);
		fence->Wait();
	}
}