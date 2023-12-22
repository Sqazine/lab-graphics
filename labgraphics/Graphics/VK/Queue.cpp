#include "Queue.h"
#include "Device.h"
#include <iostream>
#include "VK/Utils.h"

Queue::Queue(Device& device, uint32_t familyIndex)
	:mDevice(device)
{
	vkGetDeviceQueue(mDevice.GetHandle(), familyIndex, 0, &mHandle);
}

void Queue::WaitIdle() const
{
	VK_CHECK(vkQueueWaitIdle(mHandle))
}

const VkQueue& Queue::GetHandle() const
{
	return mHandle;
}

GraphicsQueue::GraphicsQueue(Device& device, uint32_t familyIndex)
	:Queue(device, familyIndex)
{
}

void GraphicsQueue::Submit(const VkSubmitInfo& submitInfo, const Fence* fence) const
{
	if (fence)
		VK_CHECK(vkQueueSubmit(mHandle, 1, &submitInfo, fence->GetHandle()))
	else
		VK_CHECK(vkQueueSubmit(mHandle, 1, &submitInfo, VK_NULL_HANDLE))
}

PresentQueue::PresentQueue(Device& device, uint32_t familyIndex)
	:Queue(device, familyIndex)
{
}

void PresentQueue::Present(const VkPresentInfoKHR& info) const
{
	VK_CHECK(vkQueuePresentKHR(mHandle, &info))
}
