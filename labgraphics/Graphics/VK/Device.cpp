#include "Device.h"
#include <iostream>
#include "Utils.h"
#include "CommandPool.h"

Device::Device(const Instance &instance, uint64_t requiredFeature)
    : mInstance(instance), mRequiredFeature(requiredFeature)
{
    mPhysicalDevice = SelectPhyDevice();

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(mPhysicalDevice, &deviceProperties);
    std::cout << "GPU: " << deviceProperties.deviceName << std::endl;

    vkGetPhysicalDeviceProperties(mPhysicalDevice, &mPhysicalDeviceProps);
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mPhysicalDeviceMemoryProps);

    mRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2 deviceProperties2 = {};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &mRayTracingPipelineProperties;
    vkGetPhysicalDeviceProperties2(mPhysicalDevice, &deviceProperties2);

    mRayTracingAccelerationFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &mRayTracingAccelerationFeatures;
    vkGetPhysicalDeviceFeatures2(mPhysicalDevice, &deviceFeatures2);

    mRayTracingAccelerationProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 deviceProps2{};
    deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps2.pNext = &mRayTracingAccelerationProps;
    vkGetPhysicalDeviceProperties2(mPhysicalDevice, &deviceProps2);

    const float queuePriority = 0.0f;
    VkDeviceQueueCreateInfo deviceQueueInfo{};
    deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueInfo.queueCount = 1;
    deviceQueueInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = nullptr;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &deviceQueueInfo;
    deviceInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkPhysicalDeviceFeatures requiredDeviceFeature{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR deviceAccelerationStructureFeatures{};
    if (requiredFeature & DeviceFeature::ANISOTROPY_SAMPLER)
    {
        requiredDeviceFeature.samplerAnisotropy = VK_TRUE;
        requiredDeviceFeature.shaderStorageImageExtendedFormats = VK_TRUE;
    }

    if (requiredFeature & DeviceFeature::RAY_TRACE)
    {
        VkPhysicalDeviceBufferDeviceAddressFeatures deviceBufferDeviceAddressFeatures{};
        deviceBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        deviceBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
        deviceBufferDeviceAddressFeatures.pNext = nullptr;

        VkPhysicalDeviceRayTracingPipelineFeaturesKHR deviceRayTracingPipelineFeatures{};
        deviceRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        deviceRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
        deviceRayTracingPipelineFeatures.pNext = &deviceBufferDeviceAddressFeatures;

        deviceAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        deviceAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
        deviceAccelerationStructureFeatures.pNext = &deviceRayTracingPipelineFeatures;

        deviceInfo.pNext = &deviceAccelerationStructureFeatures;
    }
    else if (requiredFeature & DeviceFeature::BUFFER_ADDRESS)
    {
        VkPhysicalDeviceBufferDeviceAddressFeatures deviceBufferDeviceAddressFeatures{};
        deviceBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        deviceBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
        deviceBufferDeviceAddressFeatures.pNext = nullptr;
        deviceInfo.pNext = &deviceBufferDeviceAddressFeatures;
    }

    deviceInfo.pEnabledFeatures = &requiredDeviceFeature;

    VK_CHECK(vkCreateDevice(mPhysicalDevice, &deviceInfo, nullptr, &mHandle));

    mQueueFamilyIndices = FindQueueFamilies(mPhysicalDevice, mInstance.GetSurface());

    GET_VK_DEVICE_PFN(mHandle, vkGetBufferDeviceAddressKHR);
    GET_VK_DEVICE_PFN(mHandle, vkSetDebugUtilsObjectNameEXT);
    GET_VK_DEVICE_PFN(mHandle, vkCreateAccelerationStructureKHR);
    GET_VK_DEVICE_PFN(mHandle, vkCreateRayTracingPipelinesKHR);
    GET_VK_DEVICE_PFN(mHandle, vkCmdBuildAccelerationStructuresKHR);
    GET_VK_DEVICE_PFN(mHandle, vkGetAccelerationStructureBuildSizesKHR);
    GET_VK_DEVICE_PFN(mHandle, vkDestroyAccelerationStructureKHR);
    GET_VK_DEVICE_PFN(mHandle, vkGetRayTracingShaderGroupHandlesKHR);
    GET_VK_DEVICE_PFN(mHandle, vkCmdTraceRaysKHR);
    GET_VK_DEVICE_PFN(mHandle, vkGetAccelerationStructureDeviceAddressKHR);
    GET_VK_DEVICE_PFN(mHandle, vkCmdWriteAccelerationStructuresPropertiesKHR);
    GET_VK_DEVICE_PFN(mHandle, vkCmdCopyAccelerationStructureKHR);
}

Device::~Device()
{
    WaitIdle();

    mRasterCommandPool.reset(nullptr);
    mComputeCommandPool.reset(nullptr);
    mRayTraceCommandPool.reset(nullptr);
    mTransferCommandPool.reset(nullptr);
    vkDestroyDevice(mHandle, nullptr);
}

const VkDevice &Device::GetHandle() const
{
    return mHandle;
}
const VkPhysicalDevice &Device::GetPhysicalHandle() const
{
    return mPhysicalDevice;
}

const GraphicsQueue *Device::GetGraphicsQueue()
{
    if (mGraphicsQueue == nullptr)
        mGraphicsQueue = std::make_unique<GraphicsQueue>(*this, (uint32_t)mQueueFamilyIndices.graphicsFamily.value());
    return mGraphicsQueue.get();
}

const ComputeQueue *Device::GetComputeQueue()
{
    if (mComputeQueue == nullptr)
        mComputeQueue = std::make_unique<ComputeQueue>(*this, (uint32_t)mQueueFamilyIndices.computeFamily.value());
    return mComputeQueue.get();
}

const PresentQueue *Device::GetPresentQueue()
{
    if (mPresentQueue == nullptr)
        mPresentQueue = std::make_unique<PresentQueue>(*this, (uint32_t)mQueueFamilyIndices.presentFamily.value());
    return mPresentQueue.get();
}

const TransferQueue *Device::GetTransferQueue()
{
    if (mTransferQueue == nullptr)
        mTransferQueue = std::make_unique<TransferQueue>(*this, (uint32_t)mQueueFamilyIndices.transferFamily.value());
    return mTransferQueue.get();
}

const Instance &Device::GetInstance() const
{
    return mInstance;
}

uint64_t Device::GetRequiredFeature() const
{
    return mRequiredFeature;
}

void Device::WaitIdle() const
{
    vkDeviceWaitIdle(mHandle);
}

const QueueFamilyIndices &Device::GetQueueFamilyIndices() const
{
    return mQueueFamilyIndices;
}

Shader *Device::CreateShader(ShaderStage type, std::string_view src)
{
    return new Shader(*this, type, src);
}

Shader *Device::CreateShader(ShaderStage type, const std::vector<char> &src)
{
    return new Shader(*this, type, src);
}

uint32_t Device::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

std::unique_ptr<CpuBuffer> Device::CreateCPUBuffer(void *srcData, uint32_t bufferSize, BufferUsage usage) const
{
    return std::move(std::make_unique<CpuBuffer>(const_cast<Device &>(*this), srcData, bufferSize, usage));
}

std::unique_ptr<CpuBuffer> Device::CreateCPUBuffer(uint32_t bufferSize, BufferUsage usage) const
{
    return std::move(std::make_unique<CpuBuffer>(const_cast<Device &>(*this), bufferSize, usage));
}

std::unique_ptr<SwapChain> Device::CreateSwapChain()
{
    return std::move(std::make_unique<SwapChain>(*this));
}

std::unique_ptr<Fence> Device::CreateFence(FenceStatus status) const
{
    return std::move(std::make_unique<Fence>(*this, status));
}

std::vector<std::unique_ptr<Fence>> Device::CreateFences(size_t count, FenceStatus status) const
{
     std::vector<std::unique_ptr<Fence>> result(count);
     for (auto& e : result)
         e = std::move(CreateFence(status));
     return result;
}

std::unique_ptr<Semaphore> Device::CreateSemaphore()
{
    return std::move(std::make_unique<Semaphore>(*this));
}

std::vector<std::unique_ptr<Semaphore>> Device::CreateSemaphores(size_t count)
{
	std::vector<std::unique_ptr<Semaphore>> result(count);
	for (auto& e : result)
		e = std::move(CreateSemaphore());
	return result;
}

std::unique_ptr<CpuImage2D> Device::CreateCpuImage2D(uint32_t width, uint32_t height, Format format, ImageTiling tiling)
{
    return std::move(std::make_unique<CpuImage2D>(*this, width, height, format, tiling, ImageUsage::STORAGE | ImageUsage::TRANSFER_SRC));
}

std::unique_ptr<GpuImage2D> Device::CreateGpuImage2D(uint32_t width, uint32_t height, Format format, ImageTiling tiling)
{
    return std::move(std::make_unique<GpuImage2D>(*this, width, height, format, tiling, ImageUsage::TRANSFER_DST | ImageUsage::STORAGE));
}

std::unique_ptr<ImageView2D> Device::CreateImageView(VkImage image, Format format) const
{
    return std::move(std::make_unique<ImageView2D>(*this, image, format));
}

RasterCommandPool *Device::GetRasterCommandPool()
{
    if (mRasterCommandPool == nullptr)
        mRasterCommandPool = std::make_unique<RasterCommandPool>(*this);
    return mRasterCommandPool.get();
}

ComputeCommandPool *Device::GetComputeCommandPool()
{
    if (mComputeCommandPool == nullptr)
        mComputeCommandPool = std::make_unique<ComputeCommandPool>(*this);
    return mComputeCommandPool.get();
}

RayTraceCommandPool *Device::GetRayTraceCommandPool()
{
    if (mRayTraceCommandPool == nullptr)
        mRayTraceCommandPool = std::make_unique<RayTraceCommandPool>(*this);
    return mRayTraceCommandPool.get();
}

TransferCommandPool *Device::GetTransferCommandPool()
{
    if (mTransferCommandPool == nullptr)
        mTransferCommandPool = std::make_unique<TransferCommandPool>(*this);
    return mTransferCommandPool.get();
}

std::unique_ptr<GpuBuffer> Device::CreateGPUBuffer(uint64_t bufferSize, BufferUsage usage) const
{
    return std::move(std::make_unique<GpuBuffer>(const_cast<Device &>(*this), bufferSize, usage));
}

std::unique_ptr<Buffer> Device::CreateAccelerationStorageBuffer(uint64_t bufferSize) const
{
    return CreateGPUBuffer(bufferSize, BufferUsage::ACCELERATION_STRUCTURE_STORAGE);
}

std::unique_ptr<GpuBuffer> Device::CreateGPUStorageBuffer(uint64_t bufferSize) const
{
    return CreateGPUBuffer(bufferSize, BufferUsage::STORAGE);
}

std::unique_ptr<CpuBuffer> Device::CreateCPUStorageBuffer(void *srcData, uint64_t bufferSize) const
{
    return CreateCPUBuffer(srcData, bufferSize, BufferUsage::STORAGE);
}

std::unique_ptr<CpuBuffer> Device::CreateCPUStorageBuffer(uint64_t bufferSize) const
{
    return CreateCPUBuffer(bufferSize, BufferUsage::STORAGE);
}

VkPhysicalDevice Device::SelectPhyDevice()
{
    VkPhysicalDevice result = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance.GetHandle(), &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> phyDevices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance.GetHandle(), &deviceCount, phyDevices.data());
    for (uint32_t i = 0; i < phyDevices.size(); ++i)
    {
        
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(phyDevices[i], &deviceProperties);

        VkPhysicalDeviceAccelerationStructureFeaturesKHR rtAccelerationFeatures{};
        rtAccelerationFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &rtAccelerationFeatures;
        vkGetPhysicalDeviceFeatures2(phyDevices[i], &deviceFeatures2);

        if (rtAccelerationFeatures.accelerationStructure == VK_TRUE && deviceProperties.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            result = phyDevices[i];
            break;
        }
    }

    if (result == VK_NULL_HANDLE)
    {
        std::cout << "No ray tracing compatible GPU found" << std::endl;
        abort();
    }

    if (mInstance.GetSurface() != VK_NULL_HANDLE)
    {

        VkBool32 surfaceSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(result, 0, mInstance.GetSurface(), &surfaceSupport);

        if (!surfaceSupport)
        {
            std::cout << "No surface rendering support" << std::endl;
            abort();
        }
    }

    QueueFamilyIndices indices = FindQueueFamilies(result, mInstance.GetSurface());
    if (!indices.IsComplete())
    {
        std::cout << "No full family index support" << std::endl;
        abort();
    }

    return result;
}

const VkPhysicalDeviceRayTracingPipelinePropertiesKHR &Device::GetRayTracingPipelineProps() const
{
    return mRayTracingPipelineProperties;
}
const VkPhysicalDeviceAccelerationStructureFeaturesKHR &Device::GetRayTracingAccelerationFeatures() const
{
    return mRayTracingAccelerationFeatures;
}

const VkPhysicalDeviceAccelerationStructurePropertiesKHR &Device::GetRayTracingAccelerationProps() const
{
    return mRayTracingAccelerationProps;
}

const VkPhysicalDeviceProperties &Device::GetPhysicalProps() const
{
    return mPhysicalDeviceProps;
}

const VkPhysicalDeviceMemoryProperties &Device::GetPhysicalMemoryProps() const
{
    return mPhysicalDeviceMemoryProps;
}

std::unique_ptr<DescriptorTable> Device::CreateDescriptorTable()
{
    return std::move(std::make_unique<DescriptorTable>(*this));
}
