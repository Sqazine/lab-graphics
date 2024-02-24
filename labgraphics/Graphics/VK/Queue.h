#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include "SyncObject.h"
class Queue
{
public:
	Queue(class Device &device, uint32_t familyIndex);
	~Queue() {}

	void WaitIdle() const;

	const VkQueue &GetHandle() const;

protected:
	class Device &mDevice;
	VkQueue mHandle;
};

class GraphicsQueue : public Queue
{
public:
	GraphicsQueue(class Device &device, uint32_t familyIndex);
	~GraphicsQueue() {}
private:
	friend class ComputeCommandBuffer;
	friend class RasterCommandBuffer;
	friend class RayTraceCommandBuffer;
	friend class TransferCommandBuffer;
	void Submit(const VkSubmitInfo &submitInfo, const Fence *fence = nullptr) const;
};

using ComputeQueue = GraphicsQueue;
using TransferQueue = GraphicsQueue;

class PresentQueue : public Queue
{
public:
	PresentQueue(class Device &device, uint32_t familyIndex);
	~PresentQueue() {}

private:
	friend class SwapChain;

	void Present(const VkPresentInfoKHR &info) const;
};