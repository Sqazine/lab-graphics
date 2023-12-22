#pragma once
#include <vulkan/vulkan.h>

constexpr uint64_t FENCE_WAIT_TIME_OUT = UINT64_MAX;

enum class FenceStatus
{
    SIGNALED,
    UNSIGNALED,
};

class Fence
{
public:
    Fence(const class Device &device, FenceStatus status);
    ~Fence();

    const VkFence &GetHandle() const;

    void Wait(bool waitAll = true, uint64_t timeout = FENCE_WAIT_TIME_OUT);
    void Reset();

    FenceStatus GetStatus() const;

private:
    const class Device &mDevice;
    VkFence mFenceHandle;
    FenceStatus mStatus;
};

class Semaphore
{
public:
    Semaphore(const class Device &device);
    ~Semaphore();

    const VkSemaphore &GetHandle() const;

private:
    const class Device &mDevice;
    VkSemaphore mSemaphoreHandle;
};
