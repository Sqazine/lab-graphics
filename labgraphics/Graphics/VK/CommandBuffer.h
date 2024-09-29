#pragma once
#include <vulkan/vulkan.h>
#include <functional>
#include "Image.h"
#include "RayTraceSBT.h"
#include "Pipeline.h"
#include "SyncObject.h"
#include "SwapChain.h"

class CommandBuffer
{
public:
	CommandBuffer(class Device &device, VkCommandPool cmdPool, VkCommandBufferLevel level);
	~CommandBuffer();

	const VkCommandBuffer &GetHandle() const;

	virtual void ExecuteImmediately(const std::function<void()> &func);

	virtual void Record(const std::function<void()> &func);

	virtual void ImageBarrier(const VkImage &image, Access srcAccess, Access dstAccess, ImageLayout oldLayout, ImageLayout newLayout, const VkImageSubresourceRange &subresourceRange) const;
	virtual void ImageBarrier(const VkImage &image, Format format, ImageLayout oldLayout, ImageLayout newLayout) const;
	virtual void PipelineBarrier(PipelineStage srcStage, PipelineStage dstStage, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) const;

	virtual void BindDescriptorSets(PipelineLayout *layout, uint32_t firstSet, const std::vector<const class DescriptorSet *> &descriptorSets, const std::vector<uint32_t> dynamicOffsets = {}) = 0;
	virtual void BindPipeline(Pipeline *pipeline) const = 0;

	virtual void CopyImage(VkImage srcImage, ImageLayout srcImageLayout, VkImage dstImage, ImageLayout dstImageLayout, const std::vector<VkImageCopy> &copyRegions);
	virtual void CopyImage(VkImage srcImage, ImageLayout rcImageLayout, VkImage dstImage, ImageLayout dstImageLayout, const VkImageCopy &copyRegion);

	virtual void CopyBuffer(const Buffer &dst, const Buffer &src, VkBufferCopy bufferCopy);

	virtual void CopyImageFromBuffer(Image2D *dst, Buffer *src);

	virtual void Reset();

	virtual void Submit(const std::vector<PipelineStage> &waitStages = {}, const std::vector<Semaphore *> waitSemaphores = {}, const std::vector<Semaphore *> signalSemaphores = {}, Fence *fence = nullptr) const = 0;

	void TransitionImageNewLayout(class Image2D *image, ImageLayout newLayout);

protected:
	virtual void BeginOnce();
	virtual void Begin();
	virtual void End();

	class Device &mDevice;
	VkCommandPool mRelatedCmdPoolHandle;
	VkCommandBuffer mHandle;
};

class RasterCommandBuffer : public CommandBuffer
{
public:
	RasterCommandBuffer(class Device &device, VkCommandBufferLevel level);
	~RasterCommandBuffer();

	void BindPipeline(Pipeline *pipeline) const override;
	void BindDescriptorSets(PipelineLayout *layout, uint32_t firstSet, const std::vector<const class DescriptorSet *> &descriptorSets, const std::vector<uint32_t> dynamicOffsets = {}) override;

	void BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const std::vector<Buffer *> &pBuffers, const std::vector<uint64_t> &pOffsets);
	void BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const std::vector<Buffer *> &pBuffers);

	void BeginRenderPass(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkRect2D renderArea, const std::vector<VkClearValue> &clearValues, VkSubpassContents subpassContents);
	void EndRenderPass();

	void SetViewport(const VkViewport &viewport);
	void SetScissor(const VkRect2D &scissor);
	void SetLineWidth(float lineWidth);

	void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

	void Submit(const std::vector<PipelineStage> &waitStages = {}, const std::vector<Semaphore *> waitSemaphores = {}, const std::vector<Semaphore *> signalSemaphores = {}, Fence *fence = nullptr) const override;
};

class ComputeCommandBuffer : public CommandBuffer
{
public:
	ComputeCommandBuffer(class Device &device, VkCommandBufferLevel level);
	~ComputeCommandBuffer();

	void BindPipeline(Pipeline *pipeline) const override;
	void BindDescriptorSets(PipelineLayout *layout, uint32_t firstSet, const std::vector<const class DescriptorSet *> &descriptorSets, const std::vector<uint32_t> dynamicOffsets = {}) override;

	void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

	void Submit(const std::vector<PipelineStage> &waitStages = {}, const std::vector<Semaphore *> waitSemaphores = {}, const std::vector<Semaphore *> signalSemaphores = {}, Fence *fence = nullptr) const override;
};

class RayTraceCommandBuffer : public CommandBuffer
{
public:
	RayTraceCommandBuffer(class Device &device, VkCommandBufferLevel level);
	~RayTraceCommandBuffer();

	void BindPipeline(Pipeline *pipeline) const override;
	void BindDescriptorSets(PipelineLayout *layout, uint32_t firstSet, const std::vector<const class DescriptorSet *> &descriptorSets, const std::vector<uint32_t> dynamicOffsets = {}) override;

	void TraceRaysKHR(const RayTraceSBT &sbt, uint32_t width, uint32_t height, uint32_t depth);

	void BuildAccelerationStructureKHR(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos);

	void Submit(const std::vector<PipelineStage> &waitStages = {}, const std::vector<Semaphore *> waitSemaphores = {}, const std::vector<Semaphore *> signalSemaphores = {}, Fence *fence = nullptr) const override;
};

class TransferCommandBuffer : public CommandBuffer
{
public:
	TransferCommandBuffer(class Device &device, VkCommandBufferLevel level);
	~TransferCommandBuffer();

	void BindPipeline(Pipeline *pipeline) const override;
	void BindDescriptorSets(PipelineLayout *layout, uint32_t firstSet, const std::vector<const class DescriptorSet *> &descriptorSets, const std::vector<uint32_t> dynamicOffsets = {}) override;

	void Submit(const std::vector<PipelineStage> &waitStages = {}, const std::vector<Semaphore *> waitSemaphores = {}, const std::vector<Semaphore *> signalSemaphores = {}, Fence *fence = nullptr) const override;
};