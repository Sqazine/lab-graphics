#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "Queue.h"
#include "Instance.h"
#include "Shader.h"
#include "SwapChain.h"
#include "Buffer.h"
#include "SyncObject.h"
#include "Image.h"
#include "Utils.h"
#include "DescriptorTable.h"

const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	VK_KHR_SPIRV_1_4_EXTENSION_NAME,
	VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
	VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
};

struct IndexBufferDesc
{
	std::unique_ptr<Buffer> buffer;
	VkIndexType type;
};

enum DeviceFeature
{
	NONE = 0x0000,
	BUFFER_ADDRESS = 0x0001,
	RAY_TRACE = 0x0011,
	ANISOTROPY_SAMPLER = 0x0100,
};
class Device
{
public:
	Device(const Instance &instance, uint64_t requiredFeature = DeviceFeature::NONE);
	~Device();

	const VkDevice &GetHandle() const;
	const VkPhysicalDevice &GetPhysicalHandle() const;
	const GraphicsQueue *GetGraphicsQueue();
	const ComputeQueue *GetComputeQueue();
	const PresentQueue *GetPresentQueue();
	const TransferQueue *GetTransferQueue();
	const Instance &GetInstance() const;

	uint64_t GetRequiredFeature() const;

	void WaitIdle() const;
	const QueueFamilyIndices &GetQueueFamilyIndices() const;

	Shader *CreateShader(ShaderStage type, std::string_view src);
	Shader *CreateShader(ShaderStage type, const std::vector<char> &src);
	std::unique_ptr<SwapChain> CreateSwapChain();
	std::unique_ptr<Fence> CreateFence(FenceStatus status = FenceStatus::UNSIGNALED) const;
	std::vector<std::unique_ptr<Fence>> CreateFences(size_t count, FenceStatus status = FenceStatus::UNSIGNALED) const;
	std::unique_ptr<Semaphore> CreateSemaphore();
	std::vector<std::unique_ptr<Semaphore>> CreateSemaphores(size_t count);

	std::unique_ptr<CpuImage2D> CreateCpuImage2D(uint32_t width, uint32_t height, Format format, ImageTiling tiling);
	std::unique_ptr<GpuImage2D> CreateGpuImage2D(uint32_t width, uint32_t height, Format format, ImageTiling tiling);

	class RasterCommandPool *GetRasterCommandPool();
	class ComputeCommandPool *GetComputeCommandPool();
	class RayTraceCommandPool *GetRayTraceCommandPool();
	class TransferCommandPool *GetTransferCommandPool();

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

	template <typename T>
	std::unique_ptr<Buffer> CreateRasterVertexBuffer(const std::vector<T> &vertices);
	template <typename T>
	std::unique_ptr<Buffer> CreateRayTraceVertexBuffer(const std::vector<T> &vertices) const;
	template <typename T>
	IndexBufferDesc CreateRasterIndexBuffer(const std::vector<T> &indices);
	template <typename T>
	IndexBufferDesc CreateRayTraceIndexBuffer(const std::vector<T> &indices) const;
	std::unique_ptr<CpuBuffer> CreateCPUBuffer(void *srcData, uint32_t bufferSize, BufferUsage usageFlags) const;
	std::unique_ptr<CpuBuffer> CreateCPUBuffer(uint32_t bufferSize, BufferUsage usage) const;
	std::unique_ptr<GpuBuffer> CreateGPUBuffer(uint64_t bufferSize, BufferUsage usage) const;
	std::unique_ptr<Buffer> CreateAccelerationStorageBuffer(uint64_t bufferSize) const;
	std::unique_ptr<GpuBuffer> CreateGPUStorageBuffer(uint64_t bufferSize) const;
	std::unique_ptr<CpuBuffer> CreateCPUStorageBuffer(void *srcData, uint64_t bufferSize) const;
	std::unique_ptr<CpuBuffer> CreateCPUStorageBuffer(uint64_t bufferSize) const;
	template <typename T>
	std::unique_ptr<UniformBuffer<T>> CreateUniformBuffer() const;

	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR &GetRayTracingPipelineProps() const;
	const VkPhysicalDeviceAccelerationStructureFeaturesKHR &GetRayTracingAccelerationFeatures() const;
	const VkPhysicalDeviceAccelerationStructurePropertiesKHR &GetRayTracingAccelerationProps() const;
	const VkPhysicalDeviceProperties &GetPhysicalProps() const;
	const VkPhysicalDeviceMemoryProperties &GetPhysicalMemoryProps() const;

	std::unique_ptr<DescriptorTable> CreateDescriptorTable();

	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR = nullptr;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = nullptr;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = nullptr;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR = nullptr;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;
	PFN_vkCmdCopyAccelerationStructureKHR vkCmdCopyAccelerationStructureKHR = nullptr;
	PFN_vkCmdWriteAccelerationStructuresPropertiesKHR vkCmdWriteAccelerationStructuresPropertiesKHR = nullptr;

private:
	friend class SwapChain;
	friend class Image2D;
	std::unique_ptr<ImageView2D> CreateImageView(VkImage image, Format format) const;

private:
	VkPhysicalDevice SelectPhyDevice();

	const Instance &mInstance;
	VkPhysicalDevice mPhysicalDevice;
	VkPhysicalDeviceProperties mPhysicalDeviceProps;
	VkPhysicalDeviceMemoryProperties mPhysicalDeviceMemoryProps;
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR mRayTracingPipelineProperties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR mRayTracingAccelerationFeatures{};
	VkPhysicalDeviceAccelerationStructurePropertiesKHR mRayTracingAccelerationProps{};

	VkDevice mHandle;

	uint64_t mRequiredFeature;

	QueueFamilyIndices mQueueFamilyIndices;

	std::unique_ptr<GraphicsQueue> mGraphicsQueue;
	std::unique_ptr<PresentQueue> mPresentQueue;
	std::unique_ptr<ComputeQueue> mComputeQueue;
	std::unique_ptr<TransferQueue> mTransferQueue;

	std::unique_ptr<class RasterCommandPool> mRasterCommandPool;
	std::unique_ptr<class ComputeCommandPool> mComputeCommandPool;
	std::unique_ptr<class RayTraceCommandPool> mRayTraceCommandPool;
	std::unique_ptr<class TransferCommandPool> mTransferCommandPool;
};

template <typename T>
inline std::unique_ptr<Buffer> Device::CreateRasterVertexBuffer(const std::vector<T> &vertices)
{
	auto result = std::make_unique<Buffer>(*this, sizeof(T) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	uint64_t bufferSize = sizeof(T) * vertices.size();
	auto stagingBuffer = CreateCPUBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	stagingBuffer.Fill(bufferSize, vertices.data());

	auto commandBuffer = GetRasterCommandPool()->CreatePrimaryCommandBuffer();
	commandBuffer->BeginOnce();
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = bufferSize;

	result->CopyFrom(commandBuffer->Get(), copyRegion, *stagingBuffer);

	commandBuffer->End();
	commandBuffer->Submit();

	return std::move(result);
}

template <typename T>
inline std::unique_ptr<Buffer> Device::CreateRayTraceVertexBuffer(const std::vector<T> &vertices) const
{
	return CreateCPUBuffer((void *)vertices.data(), sizeof(T) * (uint32_t)vertices.size(), BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY);
}

template <typename T>
inline IndexBufferDesc Device::CreateRasterIndexBuffer(const std::vector<T> &indices)
{
	IndexBufferDesc result;
	result.type = DataStr2VkIndexType(typeid(T).name());

	auto buffer = std::make_unique<Buffer>(sizeof(T) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	uint64_t bufferSize = sizeof(T) * indices.size();
	auto stagingBuffer = CreateCPUBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	stagingBuffer.Fill(bufferSize, indices.data());

	auto commandBuffer = GetRasterCommandPool()->CreatePrimaryCommandBuffer();
	commandBuffer->BeginOnce();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = bufferSize;

	buffer->CopyFrom(commandBuffer->Get(), copyRegion, *stagingBuffer);

	commandBuffer->End();
	commandBuffer->Submit();

	result.buffer = std::move(buffer);

	return result;
}
template <typename T>
inline IndexBufferDesc Device::CreateRayTraceIndexBuffer(const std::vector<T> &indices) const
{
	IndexBufferDesc result;
	result.type = DataStr2VkIndexType(typeid(T).name());
	result.buffer = CreateCPUBuffer((void *)indices.data(), sizeof(T) * indices.size(), BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY);
	return result;
}

template <typename T>
inline std::unique_ptr<UniformBuffer<T>> Device::CreateUniformBuffer() const
{
	return std::move(std::make_unique<UniformBuffer<T>>(const_cast<Device &>(*this)));
}