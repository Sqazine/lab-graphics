#include "SyncObject.h"
#include <iostream>
#include "Utils.h"
#include "Device.h"

Fence::Fence(const Device& device, FenceStatus status)
	: mDevice(device), mStatus(status)
{
	VkFenceCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = mStatus == FenceStatus::SIGNALED ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	VK_CHECK(vkCreateFence(mDevice.GetHandle(), &info, nullptr, &mFenceHandle));
}
Fence::~Fence()
{
	Wait();
	Reset();
	vkDestroyFence(mDevice.GetHandle(), mFenceHandle, nullptr);
}

const VkFence& Fence::GetHandle() const
{
	return mFenceHandle;
}

void Fence::Wait(bool waitAll, uint64_t timeout)
{
	VK_CHECK(vkWaitForFences(mDevice.GetHandle(), 1, &mFenceHandle, waitAll, timeout));
}

void Fence::Reset()
{
	VK_CHECK(vkResetFences(mDevice.GetHandle(), 1, &mFenceHandle));
}

FenceStatus Fence::GetStatus() const
{
	return mStatus;
}

Semaphore::Semaphore(const Device& device)
	: mDevice(device)
{
	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;

	VK_CHECK(vkCreateSemaphore(mDevice.GetHandle(), &info, nullptr, &mSemaphoreHandle));
}
Semaphore::~Semaphore()
{
	vkDestroySemaphore(mDevice.GetHandle(), mSemaphoreHandle, nullptr);
}

const VkSemaphore& Semaphore::GetHandle() const
{
	return mSemaphoreHandle;
}
