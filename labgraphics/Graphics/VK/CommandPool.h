#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "Device.h"
#include "CommandBuffer.h"
template <typename T>
class CommandPool
{
public:
    CommandPool(class Device &device, uint32_t queueFamilyIndex);
    ~CommandPool();

    const VkCommandPool &GetHandle() const;

    std::unique_ptr<T> CreatePrimaryCommandBuffer() const;
    std::vector<std::unique_ptr<T>> CreatePrimaryCommandBuffers(uint32_t count) const;

    std::unique_ptr<T> CreateSecondaryCommandBuffer() const;
    std::vector<std::unique_ptr<T>> CreateSecondaryCommandBuffers(uint32_t count) const;

private:
    class Device &mDevice;
    VkCommandPool mHandle;
};

template <typename T>
inline CommandPool<T>::CommandPool(Device &device, uint32_t queueFamilyIndex)
    : mDevice(device)
{
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.queueFamilyIndex = queueFamilyIndex;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(mDevice.GetHandle(), &info, nullptr, &mHandle);
}

template <typename T>
inline CommandPool<T>::~CommandPool()
{
    vkDestroyCommandPool(mDevice.GetHandle(), mHandle, nullptr);
}

template <typename T>
inline const VkCommandPool &CommandPool<T>::GetHandle() const
{
    return mHandle;
}

template <typename T>
inline std::unique_ptr<T> CommandPool<T>::CreatePrimaryCommandBuffer() const
{
    return std::move(std::make_unique<T>(mDevice, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
}

template <typename T>
inline std::vector<std::unique_ptr<T>> CommandPool<T>::CreatePrimaryCommandBuffers(uint32_t count) const
{
    std::vector<std::unique_ptr<T>> result(count);
    for (auto &e : result)
        e = std::move(CreatePrimaryCommandBuffer());
    return std::move(result);
}

template <typename T>
inline std::unique_ptr<T> CommandPool<T>::CreateSecondaryCommandBuffer() const
{
    return std::move(std::make_unique<T>(mDevice, VK_COMMAND_BUFFER_LEVEL_SECONDARY));
}

template <typename T>
inline std::vector<std::unique_ptr<T>> CommandPool<T>::CreateSecondaryCommandBuffers(uint32_t count) const
{
    std::vector<std::unique_ptr<T>> result(count);
    for (auto &e : result)
        e = std::move(CreateSecondaryCommandBuffer());
    return result;
}

class RasterCommandPool : public CommandPool<RasterCommandBuffer>
{
public:
    RasterCommandPool(Device &device)
        : CommandPool<RasterCommandBuffer>(device, device.GetQueueFamilyIndices().graphicsFamily.value())
    {
    }

    ~RasterCommandPool()
    {
    }
};

class ComputeCommandPool : public CommandPool<ComputeCommandBuffer>
{
public:
    ComputeCommandPool(Device &device)
        : CommandPool<ComputeCommandBuffer>(device, device.GetQueueFamilyIndices().computeFamily.value())
    {
    }

    ~ComputeCommandPool()
    {
    }
};

class RayTraceCommandPool : public CommandPool<RayTraceCommandBuffer>
{
public:
    RayTraceCommandPool(class Device &device)
        : CommandPool<RayTraceCommandBuffer>(device, device.GetQueueFamilyIndices().graphicsFamily.value())
    {
    }

    ~RayTraceCommandPool()
    {
    }
};

class TransferCommandPool : public CommandPool<TransferCommandBuffer>
{
public:
    TransferCommandPool(class Device &device)
        : CommandPool<TransferCommandBuffer>(device, device.GetQueueFamilyIndices().transferFamily.value())
    {
    }

    ~TransferCommandPool()
    {
    }
};